/*
 * arch/arm/mach-dove/dove-rd-setup.c
 *
 * Marvell Dove MV88F6781-RD Board Setup
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
#include "common.h"
#include "mpp.h"
#include "dove-front-panel-common.h"

#define DOVE_RD_WAKEUP_GPIO	(3)

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);
#ifdef CONFIG_MV_ETHERNET
#else
static struct mv643xx_eth_platform_data dove_rd_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};
#endif
static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 0, /* RD doesn't have SPDIF ports */
	.spdif_rec	= 0,
};

/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data dove_rd_sata_data = {
	.n_ports	= 1,
};

/*****************************************************************************
 * NAND 16GB
 ****************************************************************************/
/* tzachi: wait for dove support in pxa nand driver.
 * e.g. pxa3xx_nand_platform_data is under include/asm/arch-pxa/pxa3xx_nand.h
 */

/*****************************************************************************
 * SPI Devices:
 * 	SPI0: 4M Flash ST-M25P32-VMF6P
 ****************************************************************************/
static const struct flash_platform_data dove_rd_spi_flash_data = {
	.type		= "m25p32",
};

static struct spi_board_info __initdata dove_rd_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_rd_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * I2C devices:
 * 	Audio codec CS42L51-CNZ, address 0x94
 * 	eeprom CAT24C64LI-G, address 0xa0
 * 	VGA connector, address ??
 * 	LCD board, address ??
 * 	Battery, address ??
 * 	Charger, address ??
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_i2c_devs[] = {
	{ I2C_BOARD_INFO("i2s_i2c", 0x4A) },
	{ I2C_BOARD_INFO("bq2084_i2c", 0x0B) },		/* Battery Gauge */
	{ I2C_BOARD_INFO("max1535_i2c", 0x09) },	/* SMBus Charger */
};

static struct dove_mpp_mode dove_rd_mpp_modes[] __initdata = {
	{ 12, MPP_GPIO },
	{ 13, MPP_GPIO },
	{ 14, MPP_GPIO },
	{ 15, MPP_GPIO },
	{ 18, MPP_GPIO },
	{ 27, MPP_GPIO },	/* AU1 Group to GPIO */
	{ -1 },
};

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_rd_pci_init(void)
{
	if (machine_is_dove_rd()) {
		/*
		 * Init both PCIe ports.
		 * tzachi: really need both?
		 */
		dove_pcie_init(1, 1);
	}

	return 0;
}

subsys_initcall(dove_rd_pci_init);


void __init dove_battery_init(void)
{
	platform_device_register_simple("bq2084-battery", 0, NULL, 0);
}

/*****************************************************************************
 * General
 ****************************************************************************/
static void __init dove_rd_init(void)
{
	/*
	 * Basic Dove setup (needs to be called early).
	 */
	dove_init();

	if (dove_fp_ts_gpio_setup() != 0)
		return;

	/*
	 * Mux pins and GPIO pins setup
	 */
	dove_mpp_conf(dove_rd_mpp_modes);

        /* the (SW1) button is for use as a "wakeup" button */
	dove_wakeup_button_setup(DOVE_RD_WAKEUP_GPIO);

	/* card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);
	dove_sd_card_int_wa_setup(1);

	/*
	 * On-chip device registration
	 */
	dove_rtc_init();
	pxa_init_dma_wins(&dove_mbus_dram_info);
	pxa_init_dma(16);
	dove_xor0_init();
	dove_xor1_init();
	dove_ehci0_init();
	dove_ehci1_init();
#ifdef CONFIG_MV_ETHERNET
	dove_mv_eth_init();
#else
	dove_ge00_init(&dove_rd_ge00_data);
#endif
	dove_sata_init(&dove_rd_sata_data);
	dove_spi0_init(0);
	dove_spi1_init(0);
	/* uart1 is the debug port, register it first so it will be */
	/* represented by device ttyS0, root filesystems usually expect the */
	/* console to be on that device */
	dove_uart1_init();
	dove_uart0_init();
	/* dove_uart2_init(); not in use (?) */
	/* dove_uart3_init(); not in use (?) */
	dove_i2c_init();
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_i2s_init(0, &i2s0_data);

	dove_cam_init(&dove_cafe_cam_data);
	dove_lcd_spi_init();
	dove_fp_clcd_init();
	dove_vpro_init();
	dove_gpu_init();
	dove_tact_init(&tact_dove_fp_data);
	/*
	 * On-board device registration
	 */

	spi_register_board_info(dove_rd_spi_flash_info,
				ARRAY_SIZE(dove_rd_spi_flash_info));
	spi_register_board_info(dove_fp_spi_devs, dove_fp_spi_devs_num());
	i2c_register_board_info(0, dove_rd_i2c_devs, ARRAY_SIZE(dove_rd_i2c_devs));
	dove_battery_init();
}

MACHINE_START(DOVE_RD, "Marvell MV88F6781-RD Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_rd_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VPRO and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
