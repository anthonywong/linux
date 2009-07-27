/*
 * drivers/mtd/nand/pxa3xx_nand.c
 *
 * Copyright c 2005 Intel Corporation
 * Copyright c 2006 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <asm/dma.h>

#include <mach/dove_nand.h>

#include "mvCommon.h"
#include "mvOs.h"
#include "pdma/mvPdma.h"
#include "nfc/mvNfc.h"
#include "nfc/mvNfcRegs.h"
//#include "pdma/mvPdmaRegs.h"

int do_dbg = 0;
#define MY_DBG(x) 
//if(do_dbg) printk x

#define	CHIP_DELAY_TIMEOUT	(20 * HZ/10)

#define NFC_SR_MASK		(0xfff)
#define NFC_SR_CMDD_MASK	(NFC_SR_CS0_CMDD_MASK)
#define NFC_SR_BBD_MASK		(NFC_SR_CS0_BBD_MASK)

/* error code and state */
enum {
	ERR_NONE	= 0,
	ERR_DMABUSERR	= -1,
	ERR_SENDCMD	= -2,
	ERR_DBERR	= -3,
	ERR_BBERR	= -4,
};

enum {
	STATE_READY	= 0,
	STATE_CMD_HANDLE,
	STATE_DMA_READING,
	STATE_DMA_WRITING,
	STATE_DMA_DONE,
	STATE_PIO_READING,
	STATE_PIO_WRITING,
};

struct pxa3xx_nand_flash {
	uint32_t page_per_block;/* Pages per block (PG_PER_BLK) */
	uint32_t page_size;	/* Page size in bytes (PAGE_SZ) */
	uint32_t flash_width;	/* Width of Flash memory (DWIDTH_M) */
	size_t	 read_id_bytes;
};

struct pxa3xx_nand_info {
	struct nand_chip	nand_chip;

	struct platform_device	 *pdev;
	struct pxa3xx_nand_flash flash_info;

	struct clk		*clk;
	void __iomem		*mmio_base;
	unsigned int		mmio_phys_base;

	unsigned int 		buf_start;
	unsigned int		buf_count;

	unsigned char		*data_buff;
	dma_addr_t 		data_buff_phys;
	size_t			data_buff_size;

	/* saved column/page_addr during CMD_SEQIN */
	int			seqin_column;
	int			seqin_page_addr;

	/* relate to the command */
	unsigned int		state;

	int			use_ecc;	/* use HW ECC ? */
	int			use_dma;	/* use DMA ? */
	int			use_bch;	/* use BCH if use_ecc == 1 */

	size_t			data_size;	/* data size in FIFO */
	int 			retcode;
	struct completion 	cmd_complete;

	uint32_t		column;
	uint32_t		page_addr;
	MV_NFC_CMD_TYPE		cmd;
	MV_NFC_CTRL		nfcCtrl;
};

static int use_dma = 0;
module_param(use_dma, bool, 0444);
MODULE_PARM_DESC(use_dma, "enable DMA for data transfering to/from NAND HW");
static int use_ecc = 0;
module_param(use_ecc, bool, 0444);
MODULE_PARM_DESC(use_ecc, "enable ECC calculation/checking for data transfering to/from NAND HW");
static int use_bch = 0;
module_param(use_bch, bool, 0444);
MODULE_PARM_DESC(use_bch, "enable 16bit BCH ECC calculation if ECC operations are enabled");

static int prepare_read_prog_cmd(struct pxa3xx_nand_info *info,
			int column, int page_addr)
{
	struct pxa3xx_nand_flash *f = &info->flash_info;

	switch (f->page_size) {
	case 2048:
		info->data_size = (info->use_ecc) ? 2088 : 2112;
		break;
	case 512:
		info->data_size = (info->use_ecc) ? 520 : 528;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int handle_data_pio(struct pxa3xx_nand_info *info)
{
	int ret, timeout = CHIP_DELAY_TIMEOUT;
MY_DBG((KERN_INFO "handle_data_pio() - state = %d.\n",info->state));
	switch (info->state) {
	case STATE_PIO_WRITING:
		mvNfcReadWrite(&info->nfcCtrl, info->cmd, (MV_U32*)info->data_buff, info->data_buff_phys);
		mvNfcIntrEnable(&info->nfcCtrl,  NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK, MV_TRUE);

		ret = wait_for_completion_timeout(&info->cmd_complete, timeout);
		if (!ret) {
			printk(KERN_ERR "program command time out\n");
			return -1;
		}
		break;
	case STATE_PIO_READING:
MY_DBG((KERN_INFO "handle_data_pio() - data_size = %d.\n",info->data_size));
		mvNfcReadWrite(&info->nfcCtrl, info->cmd, (MV_U32*)info->data_buff, info->data_buff_phys);
		break;
	default:
		printk(KERN_ERR "%s: invalid state %d\n", __func__,
				info->state);
		return -EINVAL;
	}

	info->state = STATE_READY;
	return 0;
}
#if 0
static irqreturn_t pxa3xx_nand_data_dma_irq(int irq, void *data)
{
	struct pxa3xx_nand_info *info = data;
	uint32_t dcsr;
	int channel = info->nfcCtrl.dataChanHndl.chanNumber;

	dcsr = MV_REG_READ(PDMA_CTRL_STATUS_REG(channel));
	MV_REG_WRITE(PDMA_CTRL_STATUS_REG(channel), dcsr);

	printk(KERN_INFO "pxa3xx_nand_data_dma_irq(0x%x) - 1.\n", dcsr);

	if (dcsr & DCSR_BUSERRINTR) {
		info->retcode = ERR_DMABUSERR;
		complete(&info->cmd_complete);
	}

	if (info->state == STATE_DMA_WRITING) {
		info->state = STATE_DMA_DONE;
		mvNfcIntrEnable(&info->nfcCtrl,  NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK, MV_TRUE);
	} else {
		info->state = STATE_READY;
		complete(&info->cmd_complete);
	}
	return IRQ_HANDLED;
}

#warning "Check PDMA interrupt mask bits...................."
static void set_pdma_intr(struct pxa3xx_nand_info *info, uint32_t int_mask, bool enable)
{
	if(enable)
		MV_REG_BIT_SET(PDMA_CTRL_STATUS_REG(info->nfcCtrl.dataChanHndl.chanNumber),
				int_mask);
	else
		MV_REG_BIT_RESET(PDMA_CTRL_STATUS_REG(info->nfcCtrl.dataChanHndl.chanNumber),
				int_mask);
	return;
}
#endif

static irqreturn_t pxa3xx_nand_irq(int irq, void *devid)
{
	struct pxa3xx_nand_info *info = devid;
	unsigned int status;

	status = MV_REG_READ(NFC_STATUS_REG);
	MY_DBG((KERN_INFO "pxa3xx_nand_irq(0x%x) - 1.\n", status));
	if (status & (NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK)) {
		if (status & NFC_SR_UNCERR_MASK)
			info->retcode = ERR_DBERR;
		mvNfcIntrEnable(&info->nfcCtrl, NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK, MV_FALSE);

		if (info->use_dma) {
			info->state = STATE_DMA_READING;
			/* Enable NFC interrupts */
			mvNfcIntrEnable(&info->nfcCtrl,  NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK, MV_TRUE);
			mvNfcReadWrite(&info->nfcCtrl, info->cmd, (MV_U32*)info->data_buff, info->data_buff_phys);
		} else {
			MY_DBG((KERN_INFO "pxa3xx_nand_irq() Reading state.\n"));
			info->state = STATE_PIO_READING;
			complete(&info->cmd_complete);
		}
	} else if (status & NFC_SR_WRDREQ_MASK) {
		mvNfcIntrEnable(&info->nfcCtrl, NFC_SR_WRDREQ_MASK, MV_FALSE);
		if (info->use_dma) {
			info->state = STATE_DMA_WRITING;
			/* Enable NFC interrupts */
			mvNfcIntrEnable(&info->nfcCtrl,  NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK, MV_TRUE);
			MY_DBG((KERN_INFO "Calling mvNfcReadWrite().\n"));
			if(mvNfcReadWrite(&info->nfcCtrl, info->cmd, (MV_U32*)info->data_buff, 
						info->data_buff_phys) != MV_OK)
				printk(KERN_ERR "mvNfcReadWrite() failed.\n");
		} else {
			info->state = STATE_PIO_WRITING;
			complete(&info->cmd_complete);
		}
	} else if (status & (NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK)) {
		if (status & NFC_SR_BBD_MASK)
			info->retcode = ERR_BBERR;
		mvNfcIntrEnable(&info->nfcCtrl,  NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK, MV_FALSE);
		info->state = STATE_READY;
		complete(&info->cmd_complete);
	}
	MV_REG_WRITE(NFC_STATUS_REG, status); //nand_writel(info, NDSR, status);
	return IRQ_HANDLED;
}


static int pxa3xx_nand_do_cmd(struct pxa3xx_nand_info *info, uint32_t event)
{
	uint32_t ndcr;
	int ret, timeout = CHIP_DELAY_TIMEOUT;
	MV_STATUS status;

	/* Clear all status bits. */
	MV_REG_WRITE(NFC_STATUS_REG, NFC_SR_MASK);

	mvNfcIntrEnable(&info->nfcCtrl, NFC_SR_WRCMDREQ_MASK, MV_FALSE);
	mvNfcIntrEnable(&info->nfcCtrl, event, MV_TRUE);

	MY_DBG((KERN_INFO "About to issue command %d - 0x%x.\n", info->cmd, MV_REG_READ(NFC_CONTROL_REG)));
	info->state = STATE_CMD_HANDLE;

	status = mvNfcCommandIssue(&info->nfcCtrl,info->cmd,
				   info->page_addr,info->column);
	if(status != MV_OK) {
		printk(KERN_ERR "mvNfcCommandIssue() failed for command %d (%d).\n",info->cmd, status);
		goto fail;
	}
	MY_DBG((KERN_INFO "After issue command %d - 0x%x.\n", info->cmd, MV_REG_READ(NFC_STATUS_REG)));

	ret = wait_for_completion_timeout(&info->cmd_complete, timeout);
	if (!ret) {
		printk(KERN_ERR "command %d execution timed out (0x%x).\n",info->cmd, MV_REG_READ(NFC_STATUS_REG));
		info->retcode = ERR_SENDCMD;
		goto fail_stop;
	}

	if (info->use_dma == 0 && info->data_size > 0)
		if (handle_data_pio(info))
			goto fail_stop;

//	while(MV_PDMA_CHANNEL_STOPPED != mvPdmaChannelStateGet(&info->nfcCtrl.dataChanHndl));

	return 0;

fail_stop:
	ndcr = MV_REG_READ(NFC_CONTROL_REG);
	MV_REG_WRITE(NFC_CONTROL_REG, ndcr & ~NFC_CTRL_ND_RUN_MASK);
	udelay(10);
fail:
	return -ETIMEDOUT;
}

static int pxa3xx_nand_dev_ready(struct mtd_info *mtd)
{
	return (MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK)) ? 1 : 0;
}

static inline int is_buf_blank(uint8_t *buf, size_t len)
{
	for (; len > 0; len--)
		if (*buf++ != 0xff)
			return 0;
	return 1;
}

static void pxa3xx_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
				int column, int page_addr)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	struct pxa3xx_nand_flash *flash_info = &info->flash_info;
	int ret;

	info->use_dma = (use_dma) ? 1 : 0;
	info->use_bch = (use_bch) ? 1 : 0;
	info->use_ecc = 0;
	info->data_size = 0;
	info->state = STATE_READY;
	init_completion(&info->cmd_complete);

	switch (command) {
	case NAND_CMD_READOOB:
		/* disable HW ECC to get all the OOB data */
		info->buf_count = mtd->writesize + mtd->oobsize;
		info->buf_start = mtd->writesize + column;
		info->cmd = MV_NFC_CMD_READ_MONOLITHIC;
		info->column = column;
		info->page_addr = page_addr;
		if (prepare_read_prog_cmd(info, column, page_addr))
			break;
		pxa3xx_nand_do_cmd(info, NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK);

		/* We only are OOB, so if the data has error, does not matter */
		if (info->retcode == ERR_DBERR)
			info->retcode = ERR_NONE;
		break;

	case NAND_CMD_READ0:
		info->use_ecc = 1;
		info->retcode = ERR_NONE;
		info->buf_start = column;
		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff, 0xFF, info->buf_count);
		info->cmd = MV_NFC_CMD_READ_MONOLITHIC;
		info->column = column;
		info->page_addr = page_addr;
		if (prepare_read_prog_cmd(info, column, page_addr))
			break;
		pxa3xx_nand_do_cmd(info, NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK);

		if (info->retcode == ERR_DBERR) {
			/* for blank page (all 0xff), HW will calculate its ECC as
			 * 0, which is different from the ECC information within
			 * OOB, ignore such double bit errors
			 */
			if (is_buf_blank(info->data_buff, mtd->writesize))
				info->retcode = ERR_NONE;
		}
		break;
	case NAND_CMD_SEQIN:
		info->buf_start = column;
		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff, 0xff, info->buf_count);

		/* save column/page_addr for next CMD_PAGEPROG */
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		info->use_ecc = (info->seqin_column >= mtd->writesize) ? 0 : 1;
		info->column = info->seqin_column;
		info->page_addr = info->seqin_page_addr;
		info->cmd = MV_NFC_CMD_WRITE_MONOLITHIC;
		if (prepare_read_prog_cmd(info,
				info->seqin_column, info->seqin_page_addr)) {
			printk(KERN_ERR "prepare_read_prog_cmd() failed.\n");
			break;
		}
		pxa3xx_nand_do_cmd(info, NFC_SR_WRDREQ_MASK);
		break;
	case NAND_CMD_ERASE1:
		info->column = 0;
		info->page_addr = page_addr;
		info->cmd = MV_NFC_CMD_ERASE;
		MY_DBG((KERN_INFO "Erasing %d, %d.\n", info->page_addr, info->column));
		pxa3xx_nand_do_cmd(info, NFC_SR_BBD_MASK | NFC_SR_CMDD_MASK);
		break;
	case NAND_CMD_ERASE2:
		break;
	case NAND_CMD_READID:
	case NAND_CMD_STATUS:
//		info->use_dma = 0;	/* force PIO read */
		info->buf_start = 0;
		info->buf_count = (command == NAND_CMD_READID) ?
				flash_info->read_id_bytes : 1;
		info->data_size = 8;
		info->column = 0;
		info->page_addr = 0;
		info->cmd = (command == NAND_CMD_READID) ? 
			MV_NFC_CMD_READ_ID : MV_NFC_CMD_READ_STATUS;
		pxa3xx_nand_do_cmd(info, NFC_SR_RDDREQ_MASK);

		break;
	case NAND_CMD_RESET:
		info->column = 0;
		info->page_addr = 0;
		info->cmd = MV_NFC_CMD_RESET;
		ret = pxa3xx_nand_do_cmd(info, NFC_SR_CMDD_MASK);
		if (ret == 0) {
			int timeout = 2;
			uint32_t ndcr;

			while (timeout--) {
				if (MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK))
					break;
				msleep(10);
			}

			ndcr = MV_REG_READ(NFC_CONTROL_REG);
			MV_REG_WRITE(NFC_CONTROL_REG, ndcr & ~NFC_CTRL_ND_RUN_MASK);
		}
		break;
	default:
		printk(KERN_ERR "non-supported command.\n");
		break;
	}

	if (info->retcode == ERR_DBERR) {
		printk(KERN_ERR "double bit error @ page %08x\n", page_addr);
		info->retcode = ERR_NONE;
	}
}

static uint8_t pxa3xx_nand_read_byte(struct mtd_info *mtd)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		/* Has just send a new command? */
		retval = info->data_buff[info->buf_start++];

	return retval;
}

static u16 pxa3xx_nand_read_word(struct mtd_info *mtd)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	u16 retval = 0xFFFF;

	if (!(info->buf_start & 0x01) && info->buf_start < info->buf_count) {
		retval = *((u16 *)(info->data_buff+info->buf_start));
		info->buf_start += 2;
	}
	return retval;
}

static void pxa3xx_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);
	info->buf_start += real_len;
}

static void pxa3xx_nand_write_buf(struct mtd_info *mtd,
		const uint8_t *buf, int len)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(info->data_buff + info->buf_start, buf, real_len);
	info->buf_start += real_len;
}

static int pxa3xx_nand_verify_buf(struct mtd_info *mtd,
		const uint8_t *buf, int len)
{
	return 0;
}

static void pxa3xx_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_0 + chip);
	return;
}

static int pxa3xx_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	struct pxa3xx_nand_info *info = mtd->priv;

	/* pxa3xx_nand_send_command has waited for command complete */
	if (this->state == FL_WRITING || this->state == FL_ERASING) {
		if (info->retcode == ERR_NONE)
			return 0;
		else {
			/*
			 * any error make it return 0x01 which will tell
			 * the caller the erase and write fail
			 */
			return 0x01;
		}
	}

	return 0;
}

static void pxa3xx_nand_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	return;
}

static int pxa3xx_nand_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	return 0;
}

static int pxa3xx_nand_ecc_correct(struct mtd_info *mtd,
		uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct pxa3xx_nand_info *info = mtd->priv;
	/*
	 * Any error include ERR_SEND_CMD, ERR_DBERR, ERR_BUSERR, we
	 * consider it as a ecc error which will tell the caller the
	 * read fail We have distinguish all the errors, but the
	 * nand_read_ecc only check this function return value
	 */
	if (info->retcode != ERR_NONE)
		return -1;

	return 0;
}

static int pxa3xx_nand_detect_flash(struct pxa3xx_nand_info *info)
{
	struct platform_device *pdev = info->pdev;
	struct dove_nand_platform_data *pdata = pdev->dev.platform_data;
	struct pxa3xx_nand_flash *f = &info->flash_info;

	mvNfcFlashPageSizeGet(&info->nfcCtrl, &f->page_size);
	if (f->page_size != 2048 && f->page_size != 512)
		return -EINVAL;
	
	f->flash_width = pdata->nfc_width;
	if (f->flash_width != 16 && f->flash_width != 8)
		return -EINVAL;

	/* calculate flash information */
	f->read_id_bytes = (f->page_size == 2048) ? 4 : 2;

	return 0;
}

/* the maximum possible buffer size for large page with OOB data
 * is: 2048 + 64 = 2112 bytes, allocate a page here for both the
 * data buffer and the DMA descriptor
 */
#define MAX_BUFF_SIZE	PAGE_SIZE

static int pxa3xx_nand_init_buff(struct pxa3xx_nand_info *info)
{
	struct platform_device *pdev = info->pdev;
//	int ret;

	if (use_dma == 0) {
		info->data_buff = kmalloc(MAX_BUFF_SIZE, GFP_KERNEL);
		if (info->data_buff == NULL)
			return -ENOMEM;
		return 0;
	}

	info->data_buff = dma_alloc_coherent(&pdev->dev, MAX_BUFF_SIZE,
				&info->data_buff_phys, GFP_KERNEL);
	if (info->data_buff == NULL) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		return -ENOMEM;
	}

#if 0
	ret = request_irq(IRQ_DMA, pxa3xx_nand_data_dma_irq, IRQF_DISABLED,
			"nand-data", info);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request PDMA IRQ\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

static struct nand_ecclayout hw_smallpage_ecclayout = {
	.eccbytes = 6,
	.eccpos = {8, 9, 10, 11, 12, 13 },
	.oobfree = { {2, 6} }
};

static struct nand_ecclayout hw_largepage_ecclayout = {
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

static uint8_t mv_bbt_pattern[] = {'M', 'V', 'B', 'b', 't', '0' };
static uint8_t mv_mirror_pattern[] = {'1', 't', 'b', 'B', 'V', 'M' };

static struct nand_bbt_descr mvbbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = mv_bbt_pattern
};

static struct nand_bbt_descr mvbbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = mv_mirror_pattern
};

static void pxa3xx_nand_init_mtd(struct mtd_info *mtd,
				 struct pxa3xx_nand_info *info)
{
	struct pxa3xx_nand_flash *f = &info->flash_info;
	struct nand_chip *this = &info->nand_chip;

	this->options = (f->flash_width == 16) ? NAND_BUSWIDTH_16: 0;
	this->options |= NAND_USE_FLASH_BBT; 

	this->waitfunc		= pxa3xx_nand_waitfunc;
	this->select_chip	= pxa3xx_nand_select_chip;
	this->dev_ready		= pxa3xx_nand_dev_ready;
	this->cmdfunc		= pxa3xx_nand_cmdfunc;
	this->read_word		= pxa3xx_nand_read_word;
	this->read_byte		= pxa3xx_nand_read_byte;
	this->read_buf		= pxa3xx_nand_read_buf;
	this->write_buf		= pxa3xx_nand_write_buf;
	this->verify_buf	= pxa3xx_nand_verify_buf;

	this->ecc.mode		= NAND_ECC_HW;
	this->ecc.hwctl		= pxa3xx_nand_ecc_hwctl;
	this->ecc.calculate	= pxa3xx_nand_ecc_calculate;
	this->ecc.correct	= pxa3xx_nand_ecc_correct;
	this->ecc.size		= f->page_size;

	if (f->page_size == 2048)
		this->ecc.layout = &hw_largepage_ecclayout;
	else
		this->ecc.layout = &hw_smallpage_ecclayout;

	this->bbt_td = &mvbbt_main_descr;
	this->bbt_md = &mvbbt_mirror_descr;

	this->chip_delay = 25;
}

static int pxa3xx_nand_probe(struct platform_device *pdev)
{
	struct dove_nand_platform_data *pdata;
	struct pxa3xx_nand_info *info;
	struct nand_chip *this;
	struct mtd_info *mtd;
	struct resource *r;
	int ret = 0, irq;
	char * stat[2] = {"Disabled", "Enabled"};
	MV_NFC_INFO nfcInfo;

	pdata = pdev->dev.platform_data;

	if (!pdata) {
		dev_err(&pdev->dev, "no platform data defined\n");
		return -ENODEV;
	}

	/* Set global parameters based on platform data */
	if (pdata->use_dma) use_dma = 1;
	if (pdata->use_ecc) use_ecc = 1;
	if (pdata->use_bch) use_bch = 1;

	dev_info(&pdev->dev, "Initialize HAL based NFC in %dbit mode with DMA %s, ECC %s, BCH %s\n", 
				pdata->nfc_width, stat[use_dma], stat[use_ecc], stat[use_bch]);

	mtd = kzalloc(sizeof(struct mtd_info) + sizeof(struct pxa3xx_nand_info),
			GFP_KERNEL);
	if (!mtd) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	info = (struct pxa3xx_nand_info *)(&mtd[1]);
	info->pdev = pdev;

	this = &info->nand_chip;
	mtd->priv = info;
	mtd->owner = THIS_MODULE;

        info->clk = clk_get_sys("dove-nand", NULL);
        if (IS_ERR(info->clk)) {
		dev_err(&pdev->dev, "failed to get nand clock\n");
		ret = PTR_ERR(info->clk);
		goto fail_free_mtd;
        }
        clk_enable(info->clk);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		ret = -ENXIO;
		goto fail_put_clk;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no IO memory resource defined\n");
		ret = -ENODEV;
		goto fail_put_clk;
	}

	r = devm_request_mem_region(&pdev->dev, r->start, r->end - r->start + 1,
				    pdev->name);
	if (r == NULL) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto fail_put_clk;
	}

	info->mmio_base = devm_ioremap(&pdev->dev, r->start,
				       r->end - r->start + 1);
	if (info->mmio_base == NULL) {
		dev_err(&pdev->dev, "ioremap() failed\n");
		ret = -ENODEV;
		goto fail_put_clk;
	}

	info->mmio_phys_base = r->start;

	if (mvPdmaHalInit(MV_PDMA_MAX_CHANNELS_NUM) != MV_OK) {
		dev_err(&pdev->dev, "mvPdmaHalInit() failed.\n");
		goto fail_put_clk;
	}

	/* Initialize NFC HAL */
	nfcInfo.ioMode = (use_dma ? MV_NFC_PDMA_ACCESS : MV_NFC_PIO_ACCESS);
	if(use_ecc) {
		if(use_bch)
			nfcInfo.eccMode = MV_NFC_ECC_BCH;
		else
			nfcInfo.eccMode = MV_NFC_ECC_HAMMING;
	} else
		nfcInfo.eccMode = MV_NFC_ECC_DISABLE;
	nfcInfo.ifMode = ((pdata->nfc_width == 8) ? MV_NFC_IF_1X8 : MV_NFC_IF_1X16);
	nfcInfo.autoStatusRead = MV_FALSE;
	nfcInfo.tclk = pdata->tclk;
	nfcInfo.readyBypass = MV_FALSE;
	nfcInfo.osHandle = NULL;
	nfcInfo.regsPhysAddr = info->mmio_phys_base;
	if (mvNfcInit(&nfcInfo, &info->nfcCtrl) != MV_OK) {
		dev_err(&pdev->dev, "mvNfcInit() failed.\n");
		goto fail_put_clk;
	}

#if 0
	/* Clear PDMA interrupts */
	MV_REG_BIT_SET(PDMA_CTRL_STATUS_REG(info->nfcCtrl.dataChanHndl.chanNumber),
			DCSR_BUSERRINTR	| DCSR_STARTINTR | DCSR_ENDINTR | 
			DCSR_STOPINTR);
#endif
	mvNfcIntrEnable(&info->nfcCtrl,  0xFFF, MV_FALSE);

	ret = pxa3xx_nand_init_buff(info);
	if (ret)
		goto fail_put_clk;

	/* Clear all old events on the status register */
	MV_REG_WRITE(NFC_STATUS_REG, MV_REG_READ(NFC_STATUS_REG));
	ret = request_irq(IRQ_NAND, pxa3xx_nand_irq, IRQF_DISABLED,
				pdev->name, info);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
		goto fail_free_buf;
	}

	ret = pxa3xx_nand_detect_flash(info);
	if (ret) {
		dev_err(&pdev->dev, "failed to detect flash\n");
		ret = -ENODEV;
		goto fail_free_irq;
	}

	pxa3xx_nand_init_mtd(mtd, info);

	platform_set_drvdata(pdev, mtd);

	if (nand_scan(mtd, 1)) {
		dev_err(&pdev->dev, "failed to scan nand\n");
		ret = -ENXIO;
		goto fail_free_irq;
	}

	return add_mtd_partitions(mtd, pdata->parts, pdata->nr_parts);

fail_free_irq:
	free_irq(IRQ_NAND, info);
fail_free_buf:
	if (use_dma) {
		dma_free_coherent(&pdev->dev, info->data_buff_size,
			info->data_buff, info->data_buff_phys);
	} else
		kfree(info->data_buff);
fail_put_clk:
	clk_disable(info->clk);
	clk_put(info->clk);
fail_free_mtd:
	kfree(mtd);
	return ret;
}

static int pxa3xx_nand_remove(struct platform_device *pdev)
{
	struct mtd_info *mtd = platform_get_drvdata(pdev);
	struct pxa3xx_nand_info *info = mtd->priv;

	platform_set_drvdata(pdev, NULL);

	del_mtd_device(mtd);
	del_mtd_partitions(mtd);
	free_irq(IRQ_NAND, info);
	if (use_dma) {
		dma_free_writecombine(&pdev->dev, info->data_buff_size,
				info->data_buff, info->data_buff_phys);
	} else
		kfree(info->data_buff);

	clk_disable(info->clk);
	clk_put(info->clk);
	kfree(mtd);
	return 0;
}

#ifdef CONFIG_PM
static int pxa3xx_nand_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mtd_info *mtd = (struct mtd_info *)platform_get_drvdata(pdev);
	struct pxa3xx_nand_info *info = mtd->priv;

	if (info->state != STATE_READY) {
		dev_err(&pdev->dev, "driver busy, state = %d\n", info->state);
		return -EAGAIN;
	}

	return 0;
}

static int pxa3xx_nand_resume(struct platform_device *pdev)
{
	struct mtd_info *mtd = (struct mtd_info *)platform_get_drvdata(pdev);
	struct pxa3xx_nand_info *info = mtd->priv;

	clk_enable(info->clk);

	return pxa3xx_nand_detect_flash(info);
}
#else
#define pxa3xx_nand_suspend	NULL
#define pxa3xx_nand_resume	NULL
#endif

static struct platform_driver pxa3xx_nand_driver = {
	.driver = {
		.name	= "dove-nand-hal",
		.owner	= THIS_MODULE,
	},
	.probe		= pxa3xx_nand_probe,
	.remove		= pxa3xx_nand_remove,
	.suspend	= pxa3xx_nand_suspend,
	.resume		= pxa3xx_nand_resume,
};

static int __init pxa3xx_nand_init(void)
{
	return platform_driver_register(&pxa3xx_nand_driver);
}
module_init(pxa3xx_nand_init);

static void __exit pxa3xx_nand_exit(void)
{
	platform_driver_unregister(&pxa3xx_nand_driver);
}
module_exit(pxa3xx_nand_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Dove NAND controller driver");
