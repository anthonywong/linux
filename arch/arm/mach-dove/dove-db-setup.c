/*
 * arch/arm/mach-dove/dove-db-setup.c
 *
 * Marvell DB-MV88F6781-BP Development Board Setup
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/nand.h>
#include <linux/timer.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <linux/i2c.h>
#include <linux/pci.h>
#include <linux/gpio_mouse.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/flash.h>
#include <linux/spi/ads7846.h>
#include <video/dovefb.h>
#include <plat/i2s-orion.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/mach/arch.h>
#include <mach/dove.h>
#include <asm/hardware/pxa-dma.h>
#include <mach/dove_nand.h>
#include "common.h"
#include "clock.h"
#include "mpp.h"
#include "dove-front-panel-common.h"

#define DOVE_DB_WAKEUP_GPIO	(3)
static unsigned int front_panel = 0;
module_param(front_panel, uint, 0);
MODULE_PARM_DESC(front_panel, "set to 1 if the dove DB front panel connected");

static unsigned int left_tact = 0;
module_param(left_tact, uint, 0);
MODULE_PARM_DESC(left_tact, "Use left tact as mouse. this will disable I2S"
		 "JPR 3 should be removed, JPR 6 on 2-3");

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info * dram);

static struct orion_i2s_platform_data i2s1_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 1,
};

#if 1
static struct mv643xx_eth_platform_data dove_db_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};

#endif

static struct mv_sata_platform_data dove_db_sata_data = {
        .n_ports        = 1,
};

static struct gpio_mouse_platform_data tact_dove_db_data = {
        .scan_ms = 10,
        .polarity = 1,
        {
                {
                        .up = 54,
                        .down = 52,
                        .left = 55,
                        .right = 56,
                        .bleft = 57,
                        .bright = -1,
                        .bmiddle = -1
                } 
        }
};
/*****************************************************************************
 * SPI Devices:
 * 	SPI0: 4M Flash ST-M25P32-VMF6P
 ****************************************************************************/
static const struct flash_platform_data dove_db_spi_flash_data = {
	.type		= "m25p64",
};

static struct spi_board_info __initdata dove_db_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_db_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * 7-Segment on GPIO 14,15,18,19
 * Dummy counter up to 7 every 1 sec
 ****************************************************************************/

static struct timer_list dove_db_timer;
static int dove_db_7seg_gpios[] = {14,15, 62, 63};

static void dove_db_7seg_event(unsigned long data)
{
     static int count = 0;

     gpio_set_value(dove_db_7seg_gpios[0], count & 1);
     gpio_set_value(dove_db_7seg_gpios[1], count & (1 << 1));
     gpio_set_value(dove_db_7seg_gpios[2], count & (1 << 2));
     gpio_set_value(dove_db_7seg_gpios[3], count & (1 << 3));

     count = (count + 1) & 7;
     mod_timer(&dove_db_timer, jiffies + 1 * HZ);
}

static int __init dove_db_7seg_init(void)
{
	if (machine_is_dove_db()) {
		int i;
		
		for(i = 0; i < 4; i++){
			int pin = dove_db_7seg_gpios[i];

			if (gpio_request(pin, "7seg LED") == 0) {
				if (gpio_direction_output(pin, 0) != 0) {
					printk(KERN_ERR "%s failed "
					       "to set output pin %d\n", __func__,
					       pin);
					gpio_free(pin);
					return 0;
				}
			} else {
				printk(KERN_ERR "%s failed "
				       "to request gpio %d\n", __func__, pin);
				return 0;
			}
		}
		setup_timer(&dove_db_timer, dove_db_7seg_event, 0);
		mod_timer(&dove_db_timer, jiffies + 1 * HZ);
	}
	return 0;
}

__initcall(dove_db_7seg_init);

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_db_pci_init(void)
{
	if (machine_is_dove_db())
			dove_pcie_init(1, 1);

	return 0;
}

subsys_initcall(dove_db_pci_init);

/*****************************************************************************
 * A2D on I2C bus
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_a2d = {
	I2C_BOARD_INFO("i2s_i2c", 0x4A),
};

/*****************************************************************************
 * NAND
 ****************************************************************************/
static struct mtd_partition partition_dove[] = {
	{ .name		= "UBoot",
	  .offset	= 0,
	  .size		= 1 * SZ_1M },
	{ .name		= "UImage",
	  .offset	= MTDPART_OFS_APPEND,
	  .size		= 4 * SZ_1M },
	{ .name		= "Root",
	  .offset	= MTDPART_OFS_APPEND,
	  .size		= 1019 * SZ_1M },
};
static u64 nfc_dmamask = DMA_BIT_MASK(32);
static struct dove_nand_platform_data dove_db_nfc_data = {
	.nfc_width	= 8,
	.use_dma	= 1,
	.use_ecc	= 1,
	.use_bch	= 1,
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
};

static struct resource dove_nfc_resources[]  = {
	[0] = {
		.start	= (DOVE_NFC_PHYS_BASE),
		.end	= (DOVE_NFC_PHYS_BASE + 0xFF),
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_NAND,
		.end	= IRQ_NAND,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		/* DATA DMA */
		.start	= 97,
		.end	= 97,
		.flags	= IORESOURCE_DMA,
	},
	[3] = {
		/* COMMAND DMA */
		.start	= 99,
		.end	= 99,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device dove_nfc = {
	.name		= "dove-nand",
	.id		= -1,
	.dev		= {
		.dma_mask		= &nfc_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dove_db_nfc_data,
	},
	.resource	= dove_nfc_resources,
	.num_resources	= ARRAY_SIZE(dove_nfc_resources),
};

static void __init dove_db_nfc_init(void)
{
	dove_db_nfc_data.tclk = dove_tclk_get();
	platform_device_register(&dove_nfc);
}

static struct dove_mpp_mode dove_db_mpp_modes[] __initdata = {
	{ 0, MPP_SDIO0 },           /* SDIO0 */
	{ 1, MPP_SDIO0 },           /* SDIO0 */

	{ 2, MPP_PMU },
	{ 3, MPP_PMU },
	{ 4, MPP_PMU },
	{ 5, MPP_PMU },
	{ 6, MPP_PMU },
	{ 7, MPP_PMU },

	{ 8, MPP_GPIO },

	{ 9, MPP_PMU },
	{ 10, MPP_PMU },

	{ 11, MPP_SATA_ACT },           /* SATA active */
	{ 12, MPP_SDIO1 },           /* SDIO1 */
	{ 13, MPP_SDIO1 },           /* SDIO1 */

        { 14, MPP_GPIO },               /* 7segDebug Led */
        { 15, MPP_GPIO },               /* 7segDebug Led */
	{ 16, MPP_GPIO },
	{ 17, MPP_TWSI },
	{ 18, MPP_GPIO },
	{ 19, MPP_TWSI },

	{ 20, MPP_SPI1 },
	{ 21, MPP_SPI1 },
	{ 22, MPP_SPI1 },
	{ 23, MPP_SPI1 },

	{ 24, MPP_CAM }, /* will configure MPPs 24-39*/

	{ 40, MPP_SDIO0 }, /* will configure MPPs 40-45 */

	{ 46, MPP_SDIO1 }, /* SD0 Group */
	{ 47, MPP_SDIO1 }, /* SD0 Group */
	{ 48, MPP_SDIO1 }, /* SD0 Group */
	{ 49, MPP_SDIO1 }, /* SD0 Group */
	{ 50, MPP_SDIO1 }, /* SD0 Group */
	{ 51, MPP_SDIO1 }, /* SD0 Group */

        { 52, MPP_AUDIO1 }, /* AU1 Group */
        { 53, MPP_AUDIO1 }, /* AU1 Group */
        { 54, MPP_AUDIO1 }, /* AU1 Group */
        { 55, MPP_AUDIO1 }, /* AU1 Group */
        { 56, MPP_AUDIO1 }, /* AU1 Group */
        { 57, MPP_AUDIO1 }, /* AU1 Group */

	{ 58, MPP_SPI0 }, /* will configure MPPs 58-61 */

        { 62, MPP_GPIO }, /* 7segDebug Led */
        { 63, MPP_GPIO }, /* 7segDebug Led */
        { -1 },
};

static struct dove_mpp_mode dove_db_mpp_modes_ltact[] __initdata = {
        { 52, MPP_GPIO },               /* AU1 Group to GPIO */
        { -1 },
};

//extern struct mbus_dram_target_info dove_mbus_dram_info;
//extern int __init pxa_init_dma_wins(struct mbus_dram_target_info * dram);
static void __init dove_db_init(void)
{
	/*
	 * Basic Dove setup. Needs to be called early.
	 */
	dove_init();
	dove_mpp_conf(dove_db_mpp_modes);

        /* the (SW1) button is for use as a "wakeup" button */
	dove_wakeup_button_setup(DOVE_DB_WAKEUP_GPIO);

	/* card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);
	dove_sd_card_int_wa_setup(1);

	if(front_panel) {
		if(left_tact)
			dove_mpp_conf(dove_db_mpp_modes_ltact);
		/* JPR6 shoud be on 1-2 for touchscreen irq line */

		if (dove_fp_ts_gpio_setup() != 0)
			return;
	}

	/* Initialize AC'97 related regs.	*/
	dove_ac97_setup();

	dove_rtc_init();
	pxa_init_dma_wins(&dove_mbus_dram_info);
	pxa_init_dma(16);
	dove_xor0_init();
	dove_xor1_init();
#ifdef CONFIG_MV_ETHERNET
	dove_mv_eth_init();
#endif
	dove_ehci0_init();
	dove_ehci1_init();

	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
	ds_clks_disable_all(0, 0);
#if 1
	dove_ge00_init(&dove_db_ge00_data);
#endif
	dove_sata_init(&dove_db_sata_data);
	dove_spi0_init(0);
	dove_spi1_init(0);

	dove_uart0_init();
	dove_uart1_init();
	dove_i2c_init();
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_db_nfc_init();
	dove_fp_clcd_init();
	dove_vpro_init();
	dove_gpu_init();
	dove_cesa_init();

	if(front_panel) {
		dove_lcd_spi_init();
		dove_cam_init(&dove_cafe_cam_data);
	}
	if(front_panel && left_tact)
		dove_tact_init(&tact_dove_db_data);
	else
		dove_i2s_init(1, &i2s1_data);
	i2c_register_board_info(0, &i2c_a2d, 1);
	spi_register_board_info(dove_db_spi_flash_info,
				ARRAY_SIZE(dove_db_spi_flash_info));
	spi_register_board_info(dove_fp_spi_devs, dove_fp_spi_devs_num());
}

MACHINE_START(DOVE_DB, "Marvell DB-MV88F6781-BP Development Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_db_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VPRO and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
