/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "nfc/mvNfcRegs.h"
#include "nfc/mvNfc.h"
#include "pdma/mvPdma.h"

/*************/
/* Constants */
/*************/

#define NFC_NATIVE_READ_ID_CMD		0x0090
#define NFC_READ_ID_ADDR_LEN		1
#define NFC_ERASE_ADDR_LEN		3
#define NFC_SP_READ_ADDR_LEN		3
#define NFC_SP_BIG_READ_ADDR_LEN	4
#define NFC_LP_READ_ADDR_LEN		5
#define NFC_BLOCK_ADDR_BITS		0xFFFFFF
#define NFC_SP_COL_OFFS			0
#define NFC_SP_COL_MASK			(0xFF << NFC_SP_COL_OFFS)
#define NFC_LP_COL_OFFS			0
#define NFC_LP_COL_MASK			(0xFFFF << NFC_SP_COL_OFFS)
#define NFC_SP_PG_OFFS			8
#define NFC_SP_PG_MASK			(0xFFFFFF << NFC_SP_PG_OFFS)
#define NFC_LP_PG_OFFS			16
#define NFC_LP_PG_MASK			(0xFFFF << NFC_LP_PG_OFFS)

#define NFC_READ_ID_PDMA_DATA_LEN	32
#define NFC_READ_STATUS_PDMA_DATA_LEN	32
#define NFC_READ_ID_PIO_DATA_LEN	8
#define NFC_READ_STATUS_PIO_DATA_LEN	8
#define NFC_RW_SP_PDMA_DATA_LEN		544
#define NFC_RW_SP_NO_ECC_DATA_LEN	528
#define NFC_RW_SP_HMNG_ECC_DATA_LEN	520
#define NFC_RW_LP_PDMA_DATA_LEN		2112
#define NFC_RW_LP_NO_ECC_DATA_LEN	2112
#define NFC_RW_LP_HMNG_ECC_DATA_LEN	2088
#define NFC_RW_LP_BCH_ECC_DATA_LEN	2080

#define NFC_CMD_BUFF_SIZE		12
#define NFC_CMD_BUFF_ADDR		(INTER_REGS_BASE + NFC_COMMAND_BUFF_0_REG)
#define NFC_DATA_BUFF_ADDR		(INTER_REGS_BASE + NFC_DATA_BUFF_REG)

/**********/
/* Macros */
/**********/
#define ns_clk(ns, clk)	(MV_U32)(((ns) * (clk / 1000000) / 1000) + 1)

/***********/
/* Typedef */
/***********/

/* Flash Timing Parameters */
typedef struct {
	/* Flash Timing */
	MV_U32		tCH;  		/* Enable signal hold time */
	MV_U32		tCS;  		/* Enable signal setup time */
	MV_U32		tWH;  		/* ND_nWE high duration */
	MV_U32		tWP;  		/* ND_nWE pulse time */
	MV_U32		tRH;  		/* ND_nRE high duration */
	MV_U32		tRP;  		/* ND_nRE pulse width */
	MV_U32		tR;   		/* ND_nWE high to ND_nRE low for read */
	MV_U32		tWHR; 		/* ND_nWE high to ND_nRE low for status read */
	MV_U32		tAR;  		/* ND_ALE low to ND_nRE low delay */
	/* Physical Layout */
	MV_U32 		pgPrBlk;	/* Pages per block */
	MV_U32 		pgSz;		/* Page size */
	MV_U32 		oobSz;		/* Page size */
	MV_U32 		blkNum;		/* Number of blocks per device */
	MV_U32		id;		/* Manufacturer and device IDs */
	MV_U32		seqDis;		/* Enable/Disable sequential multipage read */
	MV_8 *		model;		/* Flash Model string */
}MV_NFC_FLASH_INFO;

/* Flash command set */
typedef struct {
	MV_U16		read1;
	MV_U16		read2;
	MV_U16		program;
	MV_U16		read_status;
	MV_U16		read_id;
	MV_U16		erase;
	MV_U16		reset;
	MV_U16		lock;
	MV_U16		unlock;
	MV_U16		lock_status;
} MV_NFC_FLASH_CMD_SET;

/********/
/* Data */
/********/

/* Defined Flash Types */
MV_NFC_FLASH_INFO flashDeviceInfo[] = {
	{			/* Samsung 32Gb */
		.tCH = 5,	/* tCH, Enable signal hold time */
		.tCS = 20,	/* tCS, Enable signal setup time */
		.tWH = 10,	/* tWH, ND_nWE high duration */
		.tWP = 12,	/* tWP, ND_nWE pulse time */
		.tRH = 10,	/* tRH, ND_nRE high duration */
		.tRP = 12,	/* tRP, ND_nRE pulse width */			
		.tR = 25121, 	/* tR = tR+tRR+tWB+1, ND_nWE high to ND_nRE low for read - 25000+20+100+1 */
		.tWHR = 60,	/* tWHR, ND_nWE high to ND_nRE low delay for status read */
		.tAR = 10,	/* tAR, ND_ALE low to ND_nRE low delay */
		.pgPrBlk = 64,	/* Pages per block - detected */
		.pgSz = 2048,	/* Page size */
		.oobSz = 64,	/* Spare size */ 
		.blkNum = 2048,	/* Number of blocks/sectors in the flash */
		.id = 0xD320,	/* Device ID 0xVendor,device */
		.model = "Samsung 8Gb 8bit",
	}
	};

/* Defined Command set */
#define 	MV_NFC_FLASH_SP_CMD_SET_IDX		0
#define		MV_NFC_FLASH_LP_CMD_SET_IDX		1
static MV_NFC_FLASH_CMD_SET	flashCmdSet[] = {
	{
	.read1		= 0x0000,
	.read2		= 0x0050,
	.program	= 0x1080,
	.read_status	= 0x0070,
	.read_id	= 0x0090,
	.erase		= 0xD060,
	.reset		= 0x00FF,
	.lock		= 0x002A,
	.unlock		= 0x2423,
	.lock_status	= 0x007A,
	}, 
	{
	.read1		= 0x3000,
	.read2		= 0x0050,
	.program	= 0x1080,
	.read_status	= 0x0070,
	.read_id	= 0x0090,
	.erase		= 0xD060,
	.reset		= 0x00FF,
	.lock		= 0x002A,
	.unlock		= 0x2423,
	.lock_status	= 0x007A,
	}};

#ifdef MV_NFC_DBG
extern MV_U32 mvNfcDbgFlag;

MV_U32 nfc_dbg_read(MV_U32 addr)
{
	MV_U32 reg = MV_MEMIO_LE32_READ(INTER_REGS_BASE + (addr));
	if (mvNfcDbgFlag) mvOsPrintf("NFC read  0x%08x = %08x\n", addr, reg);
	return reg;
}

MV_VOID nfc_dbg_write(MV_U32 addr, MV_U32 val)
{
	MV_MEMIO_LE32_WRITE((INTER_REGS_BASE + addr), (val));
	if (mvNfcDbgFlag) mvOsPrintf("NFC write 0x%08x = %08x\n", addr, val);
}

#undef MV_REG_READ
#undef MV_REG_WRITE
#define MV_REG_READ(x)		nfc_dbg_read(x)
#define MV_REG_WRITE(x,y)	nfc_dbg_write(x,y)
#endif

/**************/
/* Prototypes */
/**************/
static MV_STATUS mvDfcWait4Complete(MV_U32 statMask, MV_U32 usec);
static MV_STATUS mvNfcReadIdNative(MV_U16 *id);
static MV_STATUS mvNfcTimingSet(MV_U32 tclk, MV_NFC_FLASH_INFO *flInfo);
static MV_U32 	 mvNfcColBits(MV_U32 pg_size);

/*******************************************************************************
* mvNfcInit
*
* DESCRIPTION:
*       Initialize the NAND controller unit, and perform a detection of the 
*	attached NAND device.
*
* INPUT:
*	nfcInfo  - Flash information parameters.
*
* OUTPUT:
*	nfcCtrl  - Nand control and status information to be held by the user
*		    and passed to all other APIs.
*
* RETURN:
*       MV_OK		- On success,
*	MV_BAD_PARAM 	- The required ECC mode not supported by flash.
*	MV_NOT_SUPPORTED- The underlying flash device is not supported by HAL. 
*	MV_TIMEOUT 	- Error accessing the underlying flahs device.
*	MV_FAIL		- On failure
*******************************************************************************/
MV_STATUS mvNfcInit(MV_NFC_INFO *nfcInfo, MV_NFC_CTRL *nfcCtrl)
{
	MV_U32 ctrl_reg, ecc_reg;
	MV_STATUS ret;
	MV_U16 read_id=0;
	MV_U32 i;
	/* Initial register values */
	ctrl_reg = 0;
	ecc_reg = 0;
	
	/* Enable spare */
	ctrl_reg |= NFC_CTRL_SPARE_EN_MASK;

	/* Configure ECC mode */
	if (nfcInfo->eccMode != MV_NFC_ECC_DISABLE)
	{
		ctrl_reg |= NFC_CTRL_ECC_EN_MASK;
		if (nfcInfo->eccMode == MV_NFC_ECC_BCH)
			ecc_reg |= NFC_ECC_BCH_EN_MASK;
	}	

	/* Configure flash interface */
	if (nfcInfo->ifMode == MV_NFC_IF_1X16)
	{
		nfcCtrl->flashWidth = 16;
		nfcCtrl->dfcWidth = 16;
		ctrl_reg |= (NFC_CTRL_DWIDTH_M_MASK | NFC_CTRL_DWIDTH_C_MASK);
	}
	else if (nfcInfo->ifMode == MV_NFC_IF_2X8)
	{
		nfcCtrl->flashWidth = 8;
		nfcCtrl->dfcWidth = 16;
		ctrl_reg |= NFC_CTRL_DWIDTH_C_MASK;
	}
	else
	{
		nfcCtrl->flashWidth = 8;
		nfcCtrl->dfcWidth = 8;
	}		

	/* Configure initial READ-ID byte count */
	ctrl_reg |= (0x2 <<  NFC_CTRL_RD_ID_CNT_OFFS);

	/* Configure the Arbiter */
	ctrl_reg |= NFC_CTRL_ND_ARB_EN_MASK;

	/* Write registers before device detection */
	MV_REG_WRITE(NFC_CONTROL_REG, ctrl_reg);
	MV_REG_WRITE(NFC_ECC_CONTROL_REG, ecc_reg);

	/* Read the device ID */
	if ((ret = mvNfcReadIdNative(&read_id)) != MV_OK)
		return ret;

	/* Look for device ID in knwon device table */
	for (i=0; i<(sizeof(flashDeviceInfo)/sizeof(MV_NFC_FLASH_INFO)); i++)
	{
		if (flashDeviceInfo[i].id == read_id)
			break;
	}
	if (i == (sizeof(flashDeviceInfo)/sizeof(MV_NFC_FLASH_INFO)))
		return MV_NOT_SUPPORTED;
	else
		nfcCtrl->flashIdx = i;

	/* Configure the command set based on page size */
	if (flashDeviceInfo[i].pgSz < MV_NFC_2KB_PAGE)
		nfcCtrl->cmdsetIdx = MV_NFC_FLASH_SP_CMD_SET_IDX;
	else
		nfcCtrl->cmdsetIdx = MV_NFC_FLASH_LP_CMD_SET_IDX;

	/* calculate Timing parameters */	
	if ((ret = mvNfcTimingSet(nfcInfo->tclk, &flashDeviceInfo[i])) != MV_OK)
		return ret;	

	/* Configure the control register based on the device detected */
	ctrl_reg = MV_REG_READ(NFC_CONTROL_REG);

	/* Configure DMA */
	if (nfcInfo->ioMode == MV_NFC_PDMA_ACCESS)
		ctrl_reg |= NFC_CTRL_DMA_EN_MASK;
	else
		ctrl_reg &= ~NFC_CTRL_DMA_EN_MASK;

	/* Configure Page size */
	ctrl_reg &= ~NFC_CTRL_PAGE_SZ_MASK;
	switch (flashDeviceInfo[i].pgSz)
	{
		case MV_NFC_512B_PAGE:
			ctrl_reg |= NFC_CTRL_PAGE_SZ_512B;
			break;

		case MV_NFC_2KB_PAGE:
			ctrl_reg |= NFC_CTRL_PAGE_SZ_2KB;
			break;

		default:
			return MV_BAD_PARAM;
	}

	/* Disable sequential read if indicated */
	if (flashDeviceInfo[i].seqDis)
		ctrl_reg |= NFC_CTRL_SEQ_DIS_MASK;
	else
		ctrl_reg &= ~NFC_CTRL_SEQ_DIS_MASK;

	/* Configure the READ-ID count and row address start based on page size */
	ctrl_reg &= ~(NFC_CTRL_RD_ID_CNT_MASK | NFC_CTRL_RA_START_MASK);
	if (flashDeviceInfo[i].pgSz >= MV_NFC_2KB_PAGE)
	{
		ctrl_reg |= NFC_CTRL_RD_ID_CNT_LP;
		ctrl_reg |= NFC_CTRL_RA_START_MASK;
	}
	else
	{
		ctrl_reg |= NFC_CTRL_RD_ID_CNT_SP;
	}

	/* Confiugre pages per block */
	ctrl_reg &= ~NFC_CTRL_PG_PER_BLK_MASK;
	switch (flashDeviceInfo[i].pgPrBlk)
	{
		case 32:
			ctrl_reg |= NFC_CTRL_PG_PER_BLK_32;
			break;

		case 64:
			ctrl_reg |= NFC_CTRL_PG_PER_BLK_64;
			break;
	
		case 128:
			ctrl_reg |= NFC_CTRL_PG_PER_BLK_128;
			break;

		case 256:
			ctrl_reg |= NFC_CTRL_PG_PER_BLK_256;
			break;	

		default:
			return MV_BAD_PARAM;
	}
	
	/* Write the updated control register */
	MV_REG_WRITE(NFC_CONTROL_REG, ctrl_reg);

	/* DMA resource allocation */
	if (nfcInfo->ioMode == MV_NFC_PDMA_ACCESS)
	{
		/* Allocate DMA command buffer */
		if ((nfcCtrl->cmdBuff.bufVirtPtr = mvOsIoUncachedMalloc(nfcInfo->osHandle, NFC_CMD_BUFF_SIZE, 
				&nfcCtrl->cmdBuff.bufPhysAddr, &nfcCtrl->cmdBuff.memHandle)) == NULL)
			return MV_OUT_OF_CPU_MEM;
		nfcCtrl->cmdBuff.bufSize = NFC_CMD_BUFF_SIZE;
		nfcCtrl->cmdBuff.dataSize = NFC_CMD_BUFF_SIZE;
	
		/* Allocate Data DMA channel */
		if (mvPdmaChanAlloc(MV_PDMA_NAND_DATA, 0, &nfcCtrl->dataChanHndl) != MV_OK)
			return MV_NO_RESOURCE;

		/* Allocate Command DMA channel */
		if (mvPdmaChanAlloc(MV_PDMA_NAND_COMMAND, 0, &nfcCtrl->cmdChanHndl) != MV_OK)
			return MV_NO_RESOURCE;
	} 

	/* Initialize remaining fields in the CTRL structure */
	nfcCtrl->autoStatusRead = nfcInfo->autoStatusRead;	
	nfcCtrl->readyBypass = nfcInfo->readyBypass;
	nfcCtrl->ioMode = nfcInfo->ioMode;
	nfcCtrl->eccMode = nfcInfo->eccMode; 
	nfcCtrl->ifMode = nfcInfo->ifMode; 
	nfcCtrl->currCs = MV_NFC_CS_NONE;

	return MV_OK;
}


/*******************************************************************************
* mvNfcSelectChip
*
* DESCRIPTION:
*       Set the currently active chip for next commands.
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*	chip	 - The chip number to operate on.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_OK	- On success,
*	MV_FAIL	- On failure
*******************************************************************************/
MV_STATUS mvNfcSelectChip(MV_NFC_CTRL *nfcCtrl, MV_NFC_CHIP_SEL chip)
{
	nfcCtrl->currCs = chip;
	return MV_OK;
}


/*******************************************************************************
* mvNfcCommandIssue
*
* DESCRIPTION:
*       Issue a command to the NAND controller.
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*	cmd	 - The command to issue.
*	pageAddr - The page number to perform the command on (If the command 
*		   requires a flash offset), block address in erase.
*	columnOffs - The column offset to perform the command on (If the command 
*		   requires a flash offset)
*		   Currently, this parameter should always be 0.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_OK	   - On success,
*	MV_TIMEOUT - Timeout while waiting for command request.
*	MV_FAIL	   - On failure
*******************************************************************************/
MV_STATUS mvNfcCommandIssue(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 pageAddr, MV_U32 columnOffs)
{
	MV_U32 reg;
	MV_U32 errCode = MV_OK;
	MV_U32   cmdb_pio[3];
	MV_U32 * cmdb;

	/* Check that a chip was selected */
	if (nfcCtrl->currCs == MV_NFC_CS_NONE)
		return MV_FAIL; 	
	
	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

   	/* Setting ND_RUN bit to start the new transaction - verify that controller in idle state */
	reg = MV_REG_READ(NFC_CONTROL_REG);
	if (reg & NFC_CTRL_ND_RUN_MASK)
		return MV_BAD_STATE;
	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
   	if ((errCode = mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1)) != MV_OK)
		return errCode;
   	/* Build 12 byte Command */
	if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
		cmdb = (MV_U32*)nfcCtrl->cmdBuff.bufVirtPtr;
	else /* PIO mode */
		cmdb = cmdb_pio;

	cmdb[0] = 0;
	cmdb[1] = 0;
	cmdb[2] = 0;
	if (nfcCtrl->autoStatusRead)
		cmdb[0] |= NFC_CB0_AUTO_RS_MASK;

	if ((nfcCtrl->currCs == MV_NFC_CS_1) || (nfcCtrl->currCs == MV_NFC_CS_3))
		cmdb[0] |= NFC_CB0_CSEL_MASK;

	if ((nfcCtrl->currCs == MV_NFC_CS_2) || (nfcCtrl->currCs == MV_NFC_CS_3))
		cmdb[2] |= NFC_CB2_CS_2_3_SELECT_MASK;

	if (nfcCtrl->readyBypass)
		cmdb[0] |= NFC_CB0_RDY_BYP_MASK;

	switch (cmd)
	{
		case MV_NFC_CMD_READ_ID:
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].read_id & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
			cmdb[0] |= ((NFC_READ_ID_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);			
			cmdb[0] |= NFC_CB0_CMD_TYPE_READ_ID;
			break;

		case MV_NFC_CMD_READ_STATUS:
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].read_status & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
			cmdb[0] |= NFC_CB0_CMD_TYPE_STATUS; 
			break;

		case MV_NFC_CMD_ERASE:
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].erase & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
			cmdb[0] |= ((NFC_ERASE_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[0] |= NFC_CB0_DBC_MASK;
			cmdb[0] |= NFC_CB0_CMD_TYPE_ERASE;

			cmdb[1] |= (pageAddr & NFC_BLOCK_ADDR_BITS); 
			break;

		case MV_NFC_CMD_RESET:
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].reset & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
			cmdb[0] |= NFC_CB0_CMD_TYPE_RESET;
			break;

		case MV_NFC_CMD_READ_MONOLITHIC: /* Read a single 512B or 2KB page */
		case MV_NFC_CMD_READ_MULTIPLE:
		case MV_NFC_CMD_READ_NAKED:
		case MV_NFC_CMD_READ_LAST_NAKED:
		case MV_NFC_CMD_READ_DISPATCH:
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].read1 & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
			if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE)
			{
				if (pageAddr & ~NFC_SP_PG_MASK)
					cmdb[0] |= ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
				else
					cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);

				cmdb[1] |= ((columnOffs << NFC_SP_COL_OFFS) & NFC_SP_COL_MASK);
				cmdb[1] |= ((pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
			}
			else
			{
				cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
				cmdb[0] |= NFC_CB0_DBC_MASK;
				cmdb[1] |= ((columnOffs << NFC_LP_COL_OFFS) & NFC_LP_COL_MASK);
				cmdb[1] |= ((pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
				cmdb[2] |= (pageAddr >> (32 - NFC_LP_PG_OFFS));
			}
			cmdb[0] |= NFC_CB0_CMD_TYPE_READ;
		
			/* Check for extended command syntax */
			switch (cmd)
			{
				case MV_NFC_CMD_READ_MULTIPLE:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_MULTIPLE;
					break;
				case MV_NFC_CMD_READ_NAKED:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_NAKED;
					break;
				case MV_NFC_CMD_READ_LAST_NAKED:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_LAST_NAKED;
					break;
				case MV_NFC_CMD_READ_DISPATCH:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_DISPATCH;
					break;
				default:
					break;
			};
			break;

		case MV_NFC_CMD_WRITE_MONOLITHIC: /* Program a single page of 512B or 2KB */
		case MV_NFC_CMD_WRITE_MULTIPLE:
		case MV_NFC_CMD_WRITE_NAKED:
		case MV_NFC_CMD_WRITE_LAST_NAKED:
		case MV_NFC_CMD_WRITE_DISPATCH:
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].program & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
			if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE)
			{
				if (pageAddr & ~NFC_SP_PG_MASK)
					cmdb[0] |= ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
				else
					cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);

				cmdb[1] |= ((columnOffs << NFC_SP_COL_OFFS) & NFC_SP_COL_MASK);
				cmdb[1] |= ((pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
			}
			else
			{
				cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
				cmdb[0] |= NFC_CB0_DBC_MASK;
				cmdb[1] |= ((columnOffs << NFC_LP_COL_OFFS) & NFC_LP_COL_MASK);
				cmdb[1] |= ((pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
				cmdb[2] |= (pageAddr >> (32 - NFC_LP_PG_OFFS));
			}
			cmdb[0] |= NFC_CB0_CMD_TYPE_WRITE;	
			
			/* Check for extended syntax */
			switch (cmd)
			{
				case MV_NFC_CMD_WRITE_MULTIPLE:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_MULTIPLE;	
					break;
				case MV_NFC_CMD_WRITE_NAKED:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_NAKED;	
					break;
				case MV_NFC_CMD_WRITE_LAST_NAKED:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_LAST_NAKED;
					break;
				case MV_NFC_CMD_WRITE_DISPATCH:
					cmdb[0] |= NFC_CB0_CMD_XTYPE_DISPATCH;
					break;
				default:
					break;
			};
			break;

		default:
			return MV_BAD_PARAM; 
	};

	/* Issue command based on IO mode */
	if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
	{
		/* operate the DMA */
		if (mvPdmaChanTransfer(&nfcCtrl->cmdChanHndl, MV_PDMA_MEM_TO_PERIPH, 
				nfcCtrl->cmdBuff.bufPhysAddr, NFC_CMD_BUFF_ADDR, 
				NFC_CMD_BUFF_SIZE, 0) != MV_OK)
			return MV_HW_ERROR;
				
	}
	else /* PIO mode */
	{
	   	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[0]);
   		MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[1]);
	   	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[2]);
	}

	return MV_OK;
}


/*******************************************************************************
* mvNfcStatusGet
*
* DESCRIPTION:
*       Retrieve the NAND controller status to monitor the NAND access sequence.
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*	cmd	 - The last issued command to get the status for.
*
* OUTPUT:
*	value	- Relevant only if one of the MV_NFC_STATUS_BBD OR 
*		  MV_NFC_STATUS_COR_ERROR errors is turned on.
*		  For MV_NFC_STATUS_COR_ERROR: Holds the errors count.
*		  For MV_NFC_STATUS_BBD: Holds the bad block address.
*		  If error value is not desired, pass NULL as input.
*
* RETURN:
*	A bitmask of the MV_NFC_STATUS_XXX status bits.
*******************************************************************************/
MV_U32 mvNfcStatusGet(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *value)
{
	MV_U32 reg, ret;
	
	reg = MV_REG_READ(NFC_STATUS_REG);

	if (value)
		*value = ((reg & NFC_SR_ERR_CNT_MASK) >> NFC_SR_ERR_CNT_OFFS);

	if ((nfcCtrl->currCs == MV_NFC_CS_0) || (nfcCtrl->currCs == MV_NFC_CS_2))
	{
		/* Clear out all non related interrupts */
		reg &= (NFC_SR_CS0_BBD_MASK | NFC_SR_CS0_CMDD_MASK | NFC_SR_CS0_PAGED_MASK |
			NFC_SR_RDY0_MASK | NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | 
			NFC_SR_WRDREQ_MASK | NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK);

		ret = (reg & (NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | 
			NFC_SR_WRDREQ_MASK | NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK));

		if (reg & NFC_SR_CS0_BBD_MASK)
			ret |= MV_NFC_STATUS_BBD;
		if (reg & NFC_SR_CS0_CMDD_MASK)
			ret |= MV_NFC_STATUS_CMDD;
		if (reg & NFC_SR_CS0_PAGED_MASK)
			ret |= MV_NFC_STATUS_PAGED;
		if (reg & NFC_SR_RDY0_MASK)
			ret |= MV_NFC_STATUS_RDY;
	}
	else if ((nfcCtrl->currCs == MV_NFC_CS_1) || (nfcCtrl->currCs == MV_NFC_CS_3))
	{
		reg &= (NFC_SR_CS1_BBD_MASK | NFC_SR_CS1_CMDD_MASK | NFC_SR_CS1_PAGED_MASK |
			NFC_SR_RDY1_MASK | NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | 
			NFC_SR_WRDREQ_MASK | NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK);

		ret = (reg & (NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | 
			NFC_SR_WRDREQ_MASK | NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK));

		if (reg & NFC_SR_CS1_BBD_MASK)
			ret |= MV_NFC_STATUS_BBD;
		if (reg & NFC_SR_CS1_CMDD_MASK)
			ret |= MV_NFC_STATUS_CMDD;
		if (reg & NFC_SR_CS1_PAGED_MASK)
			ret |= MV_NFC_STATUS_PAGED;
		if (reg & NFC_SR_RDY1_MASK)
			ret |= MV_NFC_STATUS_RDY;
	}
	else
	{
		reg &= (NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | 
			NFC_SR_WRDREQ_MASK | NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK);

		ret = reg;
	}	

	/* Clear out all reported events */
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	return ret;
}


/*******************************************************************************
* mvNfcIntrEnable
*
* DESCRIPTION:
*       Enable / Disable a given set of the Nand controller interrupts.
*
* INPUT:
*	inatMask - A bitmask of the interrupts to enable / disable.
*	enable	 - MV_TRUE: Unmask the interrupts
*		   MV_FALSE: Mask the interrupts.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_OK	- On success,
*	MV_FAIL	- On failure
*******************************************************************************/
MV_STATUS mvNfcIntrEnable(MV_NFC_CTRL *nfcCtrl, MV_U32 intMask, MV_BOOL enable)
{
	MV_U32 reg;
	MV_U32 msk = (intMask & (NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | NFC_SR_WRDREQ_MASK | 
		NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK));

	if ((nfcCtrl->currCs == MV_NFC_CS_0) || (nfcCtrl->currCs == MV_NFC_CS_2))
	{
		if (intMask & MV_NFC_STATUS_BBD)
			msk |= NFC_SR_CS0_BBD_MASK;
		if (intMask & MV_NFC_STATUS_CMDD)
			msk |= NFC_SR_CS0_CMDD_MASK;
		if (intMask & MV_NFC_STATUS_PAGED)
			msk |= NFC_SR_CS0_PAGED_MASK;
		if (intMask & MV_NFC_STATUS_RDY)
			msk |= NFC_SR_RDY0_MASK;
	}
	else if ((nfcCtrl->currCs == MV_NFC_CS_1) || (nfcCtrl->currCs == MV_NFC_CS_3))
	{
		if (intMask & MV_NFC_STATUS_BBD)
			msk |= NFC_SR_CS1_BBD_MASK;
		if (intMask & MV_NFC_STATUS_CMDD)
			msk |= NFC_SR_CS1_CMDD_MASK;
		if (intMask & MV_NFC_STATUS_PAGED)
			msk |= NFC_SR_CS1_PAGED_MASK;
		if (intMask & MV_NFC_STATUS_RDY)
			msk |= NFC_SR_RDY1_MASK;
	}

	reg = MV_REG_READ(NFC_CONTROL_REG);
	if (enable)
		reg |= msk;
	else
		reg &= ~msk;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);
	
	return MV_OK;
}


/*******************************************************************************
* mvNfcReadWrite
*
* DESCRIPTION:
*       Perform a read / write operation of a previously issued command.
*	When working in PIO mode, this function will perform the read / write 
*	operation from / to the supplied buffer.
*	when working in PDMA mode, this function will trigger the PDMA to start
*	the data transfer.
*	In all cases, the user is responsible to make sure that the data 
*	transfer operation was done successfully by polling the command done bit.
*	Before calling this function, the Data-Read/Write request interrupts 
*	should be disabled (the one relevant to the command being processed).
*
* INPUT:
*	nfcCtrl     - Nand control structure.
*	cmd	    - The previously issued command.
*	virtBufAddr - [Relevant only when working in PIO mode]
*		      The virtual address of the buffer to read to / write from.
*	physBufAddr - [Relevant only when working in PDMA mode]
*		      The physical address of the buffer to read to / write from.
*		      The buffer should be cache coherent for PDMA access.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_OK	- On success,
*	MV_FAIL	- On failure
*******************************************************************************/
MV_STATUS mvNfcReadWrite(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *virtBufAddr, MV_U32 physBuffAddr)
{
	MV_U32 data_len;
	MV_U32 i;

	switch (cmd)
	{
		case MV_NFC_CMD_READ_ID:
			if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
				data_len = NFC_READ_ID_PDMA_DATA_LEN;
			else
				data_len = NFC_READ_ID_PIO_DATA_LEN;
			break;

		case MV_NFC_CMD_READ_STATUS:
			if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
				data_len =  NFC_READ_STATUS_PDMA_DATA_LEN;
			else
				data_len =  NFC_READ_STATUS_PIO_DATA_LEN;
			break;		

		case MV_NFC_CMD_READ_MONOLITHIC: /* Read a single 512B or 2KB page */
		case MV_NFC_CMD_READ_MULTIPLE:
		case MV_NFC_CMD_READ_NAKED:
		case MV_NFC_CMD_READ_LAST_NAKED:
		case MV_NFC_CMD_READ_DISPATCH:	
		case MV_NFC_CMD_WRITE_MONOLITHIC: /* Program a single page of 512B or 2KB */
		case MV_NFC_CMD_WRITE_MULTIPLE:
		case MV_NFC_CMD_WRITE_NAKED:
		case MV_NFC_CMD_WRITE_LAST_NAKED:
		case MV_NFC_CMD_WRITE_DISPATCH:
			if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
			{
				/* Decide read data size based on page size */
				if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) /* Small Page */
				{
					data_len = NFC_RW_SP_PDMA_DATA_LEN;				
				}
				else /* Large Page */
				{
					if (nfcCtrl->eccMode == MV_NFC_ECC_BCH)
						data_len = NFC_RW_LP_BCH_ECC_DATA_LEN;
					else /* Hamming and No-Ecc */
						data_len = NFC_RW_LP_PDMA_DATA_LEN;
				}
			}
			else /* PIO mode */
			{
				/* Decide read data size based on page size */
				if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) /* Small Page */
				{	
					if (nfcCtrl->eccMode == MV_NFC_ECC_HAMMING)
						data_len = NFC_RW_SP_HMNG_ECC_DATA_LEN;
					else /* No ECC */
						data_len = NFC_RW_SP_NO_ECC_DATA_LEN;				
				}
				else /* Large Page */
				{
					if (nfcCtrl->eccMode == MV_NFC_ECC_BCH)
						data_len = NFC_RW_LP_BCH_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_HAMMING)
						data_len = NFC_RW_LP_HMNG_ECC_DATA_LEN;
					else /* No ECC */
						data_len = NFC_RW_LP_NO_ECC_DATA_LEN;
				} 
			}
			break;

		case MV_NFC_CMD_ERASE:
		case MV_NFC_CMD_RESET:
			return MV_BAD_PARAM;

		default:
			return MV_BAD_PARAM;

	};

	switch (cmd)
	{
		case MV_NFC_CMD_READ_ID:
		case MV_NFC_CMD_READ_STATUS:
		case MV_NFC_CMD_READ_MONOLITHIC: /* Read a single 512B or 2KB page */
		case MV_NFC_CMD_READ_MULTIPLE:
		case MV_NFC_CMD_READ_NAKED:
		case MV_NFC_CMD_READ_LAST_NAKED:
		case MV_NFC_CMD_READ_DISPATCH:
			/* Issue command based on IO mode */
			if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
			{
				/* operate the DMA */
				if (mvPdmaChanTransfer(&nfcCtrl->dataChanHndl, MV_PDMA_PERIPH_TO_MEM, 
						NFC_DATA_BUFF_ADDR, physBuffAddr, data_len, 0) != MV_OK)
					return MV_HW_ERROR;
			}
			else /* PIO mode */
			{
				for (i=0; i<data_len; i+=4)
				{
					*virtBufAddr = MV_REG_READ(NFC_DATA_BUFF_REG);
					virtBufAddr++;
				}
			}
			break;

		case MV_NFC_CMD_WRITE_MONOLITHIC: /* Program a single page of 512B or 2KB */
		case MV_NFC_CMD_WRITE_MULTIPLE:
		case MV_NFC_CMD_WRITE_NAKED:
		case MV_NFC_CMD_WRITE_LAST_NAKED:
		case MV_NFC_CMD_WRITE_DISPATCH:
			/* Issue command based on IO mode */
			if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
			{
				/* operate the DMA */
				if (mvPdmaChanTransfer(&nfcCtrl->dataChanHndl, MV_PDMA_MEM_TO_PERIPH, 
						physBuffAddr, NFC_DATA_BUFF_ADDR, data_len, 0) != MV_OK)
					return MV_HW_ERROR;
			}
			else /* PIO mode */
			{
				for (i=0; i<data_len; i+=4)
				{
					MV_REG_WRITE(NFC_DATA_BUFF_REG, *virtBufAddr);
					virtBufAddr++;
				}
			}
			break;

		default:
			return MV_BAD_PARAM;
	};

	return MV_OK;
}


/*******************************************************************************
* mvNfcAddress2RowConvert
*
* DESCRIPTION:
*       Convert an absolute flash address to row index.
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*	address	 - The absolute flash address.
*
* OUTPUT:
*	row	 - The row number corresponding to the given address.
*	colOffset- The column offset within the row.
*
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvNfcAddress2RowConvert(MV_NFC_CTRL *nfcCtrl, MV_U32 address, MV_U32 *row, MV_U32 *colOffset)
{
	
	if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) /* Small Page */
	{	
		*colOffset = (address & 0xFF);
		*row = (address >> 9);
	}
	else /* Large Page */
	{
		*colOffset = (address & (flashDeviceInfo[nfcCtrl->flashIdx].pgSz - 1));

		/* Calculate the page bits */
		*row = (address >> mvNfcColBits(flashDeviceInfo[nfcCtrl->flashIdx].pgSz));
	}
}

/*******************************************************************************
* mvNfcAddress2BlockConvert
*
* DESCRIPTION:
*       Convert an absolute flash address to erasable block address
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*	address	 - The absolute flash address.
*
* OUTPUT:
*	blk - block address
*
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvNfcAddress2BlockConvert(MV_NFC_CTRL *nfcCtrl, MV_U32 address, MV_U32 *blk)
{
	*blk = (address / (flashDeviceInfo[nfcCtrl->flashIdx].pgSz * flashDeviceInfo[nfcCtrl->flashIdx].pgPrBlk));
}

/*******************************************************************************
* mvNfcAddress2BlockConvert
*
* DESCRIPTION:
*       Convert an absolute flash address to erasable block address
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*	address	 - The absolute flash address.
*
* OUTPUT:
*	blk - block address
*
* RETURN:
*	None
*******************************************************************************/
MV_8 * mvNfcFlashModelGet(MV_NFC_CTRL *nfcCtrl)
{
	static MV_8 * unk_dev = "Unknown Flash Device";

	if (nfcCtrl->flashIdx >= (sizeof(flashDeviceInfo)/sizeof(MV_NFC_FLASH_INFO)))
		return unk_dev;
	
	return flashDeviceInfo[nfcCtrl->flashIdx].model;
}

/*******************************************************************************
* mvNfcFlashPageSizeGet
*
* DESCRIPTION:
*       Retrieve the logical page size of a given flash.
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*
* OUTPUT:
*	size - Flash size in bytes.
*
* RETURN:
*	MV_NOT_FOUND - Bad flash index.
*******************************************************************************/
MV_STATUS mvNfcFlashPageSizeGet(MV_NFC_CTRL *nfcCtrl, MV_U32 *size)
{
        if (nfcCtrl->flashIdx >= (sizeof(flashDeviceInfo)/sizeof(MV_NFC_FLASH_INFO)))
		return MV_NOT_FOUND;
	if(size == NULL)
		return MV_BAD_PTR;

	if(nfcCtrl->ifMode == MV_NFC_IF_2X8)
		*size = flashDeviceInfo[nfcCtrl->flashIdx].pgSz << 1;
	else
		*size = flashDeviceInfo[nfcCtrl->flashIdx].pgSz;

	return MV_OK;
}

/*******************************************************************************
* mvNfcFlashBlockSizeGet
*
* DESCRIPTION:
*       Retrieve the logical block size of a given flash.
*
* INPUT:
*	nfcCtrl  - Nand control structure.
*
* OUTPUT:
*	size - Flash size in bytes.
*
* RETURN:
*	MV_NOT_FOUND - Bad flash index.
*******************************************************************************/
MV_STATUS mvNfcFlashBlockSizeGet(MV_NFC_CTRL *nfcCtrl, MV_U32 *size)
{
        if (nfcCtrl->flashIdx >= (sizeof(flashDeviceInfo)/sizeof(MV_NFC_FLASH_INFO)))
		return MV_NOT_FOUND;
	if(size == NULL)
		return MV_BAD_PTR;

	if(nfcCtrl->ifMode == MV_NFC_IF_2X8)
		*size = ((flashDeviceInfo[nfcCtrl->flashIdx].pgSz << 1) * flashDeviceInfo[nfcCtrl->flashIdx].pgPrBlk);
	else
		*size = (flashDeviceInfo[nfcCtrl->flashIdx].pgSz * flashDeviceInfo[nfcCtrl->flashIdx].pgPrBlk);

	return MV_OK;
}

/*******************************************************************************
* mvDfcWait4Complete
*
* DESCRIPTION:
*  	Wait for event or process to complete
*
* INPUT:
*	statMask: bit to wait from in status register NDSR
*	usec: Max uSec to wait for event
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_OK		- On success,
*	MV_TIMEOUT 	- Error accessing the underlying flahs device.
*******************************************************************************/
static MV_STATUS mvDfcWait4Complete(MV_U32 statMask, MV_U32 usec)
{
	MV_U32 i, sts;

	for (i=0; i<usec; i++)
	{
		sts = (MV_REG_READ(NFC_STATUS_REG) & statMask);
		if (sts)
		{
			MV_REG_WRITE(NFC_STATUS_REG, sts);
			return MV_OK;
		}
		mvOsUDelay(1);
	}
	
	return MV_TIMEOUT;
}

/*******************************************************************************
* mvNfcReadIdNative
*
* DESCRIPTION:
*       Read the flash Manufacturer and device ID in PIO mode.
*
* INPUT:
*	None.
*
* OUTPUT:
*	id: Manufacturer and Device Id detected (valid only if return is MV_OK).
*
* RETURN:
*       MV_OK		- On success,
*	MV_TIMEOUT 	- Error accessing the underlying flahs device.
*	MV_FAIL		- On failure
*******************************************************************************/
static MV_STATUS mvNfcReadIdNative(MV_U16 *id)
{
	MV_U32 reg;
	MV_U32 errCode = MV_OK;
	
	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);
	
   	/* Setting ND_RUN bit to start the new transaction */
	reg = MV_REG_READ(NFC_CONTROL_REG);
	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
   	if ((errCode = mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1)) != MV_OK)
		return errCode;

   	/* Send Command */
	reg =  NFC_NATIVE_READ_ID_CMD;
	reg |= (0x1 <<  NFC_CB0_ADDR_CYC_OFFS);
	reg |= NFC_CB0_CMD_TYPE_READ_ID;
   	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, reg);
   	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
   	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);

	/* Wait for Data READ request */
   	if ((errCode = mvDfcWait4Complete(NFC_SR_RDDREQ_MASK, 10)) != MV_OK)
		return errCode;	

	/*  Read the read ID bytes. + read 4 bogus bytes */	
	*id = (MV_U16)(MV_REG_READ(NFC_DATA_BUFF_REG) & 0xFFFF);
	reg = MV_REG_READ(NFC_DATA_BUFF_REG); /* dummy read to complete 8 bytes */

	reg = MV_REG_READ(NFC_CONTROL_REG);
	if (reg & NFC_CTRL_ND_RUN_MASK)
	{
		MV_REG_WRITE(NFC_CONTROL_REG, (reg & ~NFC_CTRL_ND_RUN_MASK));
		return MV_BAD_STATE;
	}

	return MV_OK;
} 

/*******************************************************************************
* mvNfcTimingSet
*
* DESCRIPTION:
*       Set all flash timing parameters for optimized operation
*
* INPUT:
*	tclk: Tclk frequency,
	flInfo: timing information
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_OK		- On success,
*	MV_FAIL		- On failure
*******************************************************************************/
static MV_STATUS mvNfcTimingSet(MV_U32 tclk, MV_NFC_FLASH_INFO *flInfo)
{
	MV_U32 reg;

	/* Configure the Timing-0 register */
	reg = 0;
	reg |= NFC_TMNG0_SEL_CNTR_MASK;
	reg |= ((ns_clk(flInfo->tCH, tclk) << NFC_TMNG0_TCH_OFFS) & NFC_TMNG0_TCH_MASK);
	reg |= ((ns_clk(flInfo->tCS, tclk) << NFC_TMNG0_TCS_OFFS) & NFC_TMNG0_TCS_MASK);
	reg |= ((ns_clk(flInfo->tWH, tclk) << NFC_TMNG0_TWH_OFFS) & NFC_TMNG0_TWH_MASK);
	reg |= ((ns_clk(flInfo->tWP, tclk) << NFC_TMNG0_TWP_OFFS) & NFC_TMNG0_TWP_MASK);
	reg |= ((ns_clk(flInfo->tRH, tclk) << NFC_TMNG0_TRH_OFFS) & NFC_TMNG0_TRH_MASK);
	reg |= ((ns_clk(flInfo->tRP, tclk) << NFC_TMNG0_TRP_OFFS) & NFC_TMNG0_TRP_MASK);
	MV_REG_WRITE(NFC_TIMING_0_REG, reg);

	/* Configure the Timing-1 register */
	reg = 0;
	reg |= ((ns_clk(flInfo->tR, tclk) << NFC_TMNG1_TR_OFFS) & NFC_TMNG1_TR_MASK);
	reg |= ((ns_clk(flInfo->tWHR, tclk) << NFC_TMNG1_TWHR_OFFS) & NFC_TMNG1_TWHR_MASK);
	reg |= ((ns_clk(flInfo->tAR, tclk) << NFC_TMNG1_TAR_OFFS) & NFC_TMNG1_TAR_MASK);
	MV_REG_WRITE(NFC_TIMING_1_REG, reg);

	return MV_OK;
}

/*******************************************************************************
* mvNfcColBits
*
* DESCRIPTION:
*       Calculate number of bits representing column part of the address
*
* INPUT:
	pg_size: page size
*
* OUTPUT:
*	None.
*
* RETURN:
*	Number of bits representing a column
*******************************************************************************/
static MV_U32 mvNfcColBits(MV_U32 pg_size)
{
	MV_U32 shift = 0;
	while(pg_size)
	{
        	++shift;
		pg_size >>=1;
	};

	return (shift-1);
}
