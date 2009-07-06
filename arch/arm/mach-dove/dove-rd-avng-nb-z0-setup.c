/*
 * arch/arm/mach-dove/dove-rd-avng-nb-setup.c
 *
 * Marvell Dove MV88F6781-RD Avengers Net Book Board Setup
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
#include <linux/delay.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/nand.h>
#include <linux/timer.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <linux/i2c.h>
#include <linux/i2c/kb3310.h>
#include <linux/pci.h>
#include <linux/gpio_mouse.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/flash.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <plat/i2s-orion.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/mach/arch.h>
#include <mach/dove.h>
//#include <asm/mach/dma.h>
#include <asm/hardware/pxa-dma.h>
#include <mach/dove_nand.h>
#include <mach/dove_bl.h>
#include <plat/cafe-orion.h>
#include "common.h"
#include "clock.h"
#include "mpp.h"

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);
extern void (*arm_shut_down)(void);

static unsigned int use_hal_giga = 1;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif


/*****************************************************************************
 * BACKLIGHT
 ****************************************************************************/
static struct dovebl_platform_data dove_anvg_nb_backlight_data = {
	.default_intensity = 0xa,
	.max_brightness = 0xe,
	.gpio_pm_control = 1,
};

/*****************************************************************************
 * LCD
 ****************************************************************************/
/*
 * LCD input clock.
 */
#define LCD_SCLK	(CONFIG_FB_DOVE_CLCD_SCLK_VALUE*1000*1000)

static struct dovefb_mach_info dove_avng_nb_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.sclk_clock		= LCD_SCLK,
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
#if defined(CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0)
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
#else
	.io_pin_allocation	= IOPAD_DUMB18GPIO,
	.panel_rgb_type		= DUMB18_RGB666_0,
#endif
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_i2c_adapter	= 0,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_anvg_nb_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
	.sclk_clock		= LCD_SCLK,
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB18GPIO,
	.panel_rgb_type		= DUMB18_RGB666_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 0,
	.enable_lcd0		= 0,
};

static struct dovefb_mach_info dove_anvg_nb_lcd1_dmi = {
	.id_gfx			= "GFX Layer 1",
	.id_ovly		= "Video Layer 1",
	.sclk_clock		= LCD_SCLK,
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB565,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
	.ddc_i2c_adapter	= 0,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
#ifndef CONFIG_FB_DOVE_CLCD
	.enable_lcd0		= 1,
#else
	.enable_lcd0		= 0,
#endif
};

static struct dovefb_mach_info dove_anvg_nb_lcd1_vid_dmi = {
	.id_ovly		= "Video Layer 1",
	.sclk_clock		= LCD_SCLK,
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 0,
};

static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 1,
	.spdif_rec	= 1,
};

static struct mv643xx_eth_platform_data dove_rd_avng_nb_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};

/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data dove_rd_avng_nb_sata_data = {
	.n_ports	= 1,
};

/*****************************************************************************
 * SPI Devices:
 *     SPI0: 4M Flash MX25L3205D
 ****************************************************************************/
static const struct flash_platform_data dove_rd_avng_nb_spi_flash_data = {
	.type           = "mx25l3205",
};

static struct spi_board_info __initdata dove_rd_avng_nb_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_rd_avng_nb_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * Keyboard/Mouse devices:
 * 	KB3310, address 0x58
 ****************************************************************************/
static struct kb3310_platform_data __initdata dove_rd_avng_nb_kb_data = {
	.enabled_ifaces = (KB3310_USE_PS2_KEYBOARD | KB3310_USE_PS2_MOUSE),
};

/*****************************************************************************
 * I2C devices:
 * 	Audio codec ALC5623, address 0x1A
 * 	KB3310, address 0x58
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_avng_nb_i2c_devs[] = {
		/* tzachi: add I2C_BOARD_INFO(), (and platform_data?) */
	{
		I2C_BOARD_INFO("i2s_i2c", 0x1A),
	},
	{
		I2C_BOARD_INFO("kb3310", 0x58),
		.irq		= 7 + IRQ_DOVE_GPIO_START,
		.platform_data  = &dove_rd_avng_nb_kb_data,
	},
};

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_rd_avng_nb_pci_init(void)
{
	if (machine_is_dove_rd_avng_nb_z0()) {
		dove_pcie_init(1, 1);
	}

	return 0;
}

subsys_initcall(dove_rd_avng_nb_pci_init);

/*****************************************************************************
 * Camera
 ****************************************************************************/
static struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 2, //CTL1 connected to the sensor power down
	.reset		= 1, //CTL0 connected to the sensor reset
};

/*****************************************************************************
 * MPP
 ****************************************************************************/
static struct dove_mpp_mode dove_rd_avng_nb_mpp_modes[] __initdata = {
	{ 2, MPP_GPIO },	/* GP_PD_WLAN/Amp_PwrDn/GePhy_PwrDn */
	{ 3, MPP_GPIO },	/* STBY_DETECTED */
	{ 4, MPP_GPIO },	/* SHDN_DETECTED */
	{ 5, MPP_GPIO },	/* STBY_REQUEST */
	{ 6, MPP_GPIO },	/* SHDN_REQUEST */

	{ 7, MPP_GPIO },	/* KB_INT */
	{ 8, MPP_GPIO },	/* 3G_RF_DISABLE */
	{ 9, MPP_GPIO },	/* WiFi_ON/OFF */
	{ 10, MPP_GPIO },	/* DOVE_CKE */

	{ 14, MPP_GPIO },	/* SD_ON */
	{ 15, MPP_GPIO },	/* AC_PlugIn */

	{ 18, MPP_GPIO },	/* CMMB_RST# */
	{ 19, MPP_GPIO },	/* CMMB_INTR */
	{ 20, MPP_SPI1 },	/* TSC_MISO */
	{ 21, MPP_SPI1 },	/* TSC_CS */
	{ 22, MPP_SPI1 },	/* TSC_MOSI */
	{ 23, MPP_SPI1 },	/* TSC_SCK */

	{ 52, MPP_GPIO },	/* AU1 Group to GPIO */
	{ 62, MPP_GPIO },	/* UA1 Group to GPIO */
	{ -1 },
};

/*****************************************************************************
 * GPIO
 ****************************************************************************/
static void dove_rd_avng_nb_shutdown(void)
{
	gpio_set_value(6, 1);
	mdelay(10);
	gpio_set_value(6, 0);
}

static void dove_rd_avng_nb_gpio_init(void)
{
	/* GP_PD_WLAN or Amp_PwrDn or GePhy_PwrDn */
	orion_gpio_set_valid(2, 1);
	if (gpio_request(2, "IO_P2") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for IO_P2\n");	
	gpio_direction_output(2, 1);
	/* Standby request detected (from external MCU) */
	orion_gpio_set_valid(3, 1);
	if (gpio_request(3, "STBY_DETECTED") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for STBY_DETECTED\n");	
	gpio_direction_input(3);
	/* Shut-down request detected (from external MCU) */
	orion_gpio_set_valid(4, 1);
	if (gpio_request(4, "SHDN_DETECTED") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SHDN_DETECTED\n");	
	gpio_direction_input(4);
	/* Standby request (to external MCU) */
	orion_gpio_set_valid(5, 1);
	if (gpio_request(5, "STBY_REQUEST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for STBY_REQUEST\n");	
	gpio_direction_output(5, 0);
	/* Shut-down request (to external MCU) */
	orion_gpio_set_valid(6, 1);
	if (gpio_request(6, "SHDN_REQUEST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SHDN_REQUEST\n");	
	gpio_direction_output(6, 0);
	/* Keyboard interrupt */
	orion_gpio_set_valid(7, 1);
	if (gpio_request(7, "KB_INT") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for KB_INT\n");	
	gpio_direction_input(7);
	/* 3G RF Disable (active low) */
	orion_gpio_set_valid(8, 1);
	if (gpio_request(8, "3G_RF_DISABLE") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for 3G_RF_DISABLEn\n");	
	gpio_direction_output(8, 1);
	/* Wifi on/off (connected to an external switch) */
	orion_gpio_set_valid(9, 1);
	if (gpio_request(9, "WiFi_ON_OFF") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for WiFi_ON_OFF\n");	
	gpio_direction_input(9);
	/* CKE to external MCU (high to enable DDR) */
	orion_gpio_set_valid(10, 1);
	if (gpio_request(10, "DOVE_CKE") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for DOVE_CKE\n");	
	gpio_direction_output(10, 1);
	/* High to enable power on SD interface */
	orion_gpio_set_valid(14, 1);
	if (gpio_request(14, "SD_ON") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SD_ON\n");	
	gpio_direction_output(14, 1);
	/* AC power plug-in */
	orion_gpio_set_valid(15, 1);
	if (gpio_request(15, "AC_PlugIn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for AC_PlugIn\n");	
	gpio_direction_input(15);
	/* CMMB_RST# (active low) */
	orion_gpio_set_valid(18, 1);
	if (gpio_request(18, "CMMB_RSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMMB_RSTn\n");	
	gpio_direction_output(18, 1);
	/* CMMB interrupt */
	orion_gpio_set_valid(19, 1);
	if (gpio_request(19, "CMMB_INTR") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMMB_INTR\n");	
	gpio_direction_input(19);

	/* GP_WLAN_RST# (active low) */
	orion_gpio_set_valid(52, 1);
	if (gpio_request(52, "GP_WLAN_RST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_WLAN_RSTn\n");	
	gpio_direction_output(52, 1);
	/* SSD_PRST# (active low) */
	orion_gpio_set_valid(53, 1);
	if (gpio_request(53, "SSD_PRSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SSD_PRSTn\n");	
	gpio_direction_output(53, 1);
	/* Hub_Reset# or Audio_RST# (active low) */
	orion_gpio_set_valid(55, 1);
	if (gpio_request(55, "IO_RSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for IO_RSTn\n");	
	gpio_direction_output(55, 1);
	/* 3GCARD_PRST# (active low) */
	orion_gpio_set_valid(57, 1);
	if (gpio_request(57, "3GCARD_PRSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for 3GCARD_PRSTn\n");	
	gpio_direction_output(57, 1);
	/* GP_VTTON */
	orion_gpio_set_valid(62, 1);
	if (gpio_request(62, "GP_VTTON") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_VTTON\n");	
	gpio_direction_output(62, 1);
	/* CMRA_V2.8_EN */
	orion_gpio_set_valid(63, 1);
	if (gpio_request(63, "CMRA_V2.8_EN") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMRA_V2.8_EN\n");	
	gpio_direction_output(63, 1);
}

/*****************************************************************************
 * General
 ****************************************************************************/
static void __init dove_rd_avng_nb_init(void)
{
	arm_shut_down = dove_rd_avng_nb_shutdown;

	/*
	 * Basic Dove setup (needs to be called early).
	 */
	dove_init();
	dove_mpp_conf(dove_rd_avng_nb_mpp_modes);
	dove_rd_avng_nb_gpio_init();

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
#ifdef CONFIG_MV_ETHERNET
	if(use_hal_giga)
		dove_mv_eth_init();
	else
#endif
	dove_ge00_init(&dove_rd_avng_nb_ge00_data);

	dove_ehci0_init();
	dove_ehci1_init();
	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
	ds_clks_disable_all(0, 0);

	dove_sata_init(&dove_rd_avng_nb_sata_data);
	dove_spi0_init(0);
	dove_spi1_init(1); /* use interrupt mode */
	/* uart0 is the debug port, register it first so it will be */
	/* represented by device ttyS0, root filesystems usually expect the */
	/* console to be on that device */
	dove_uart0_init();
	/* dove_uart1_init(); not in use */
	/* dove_uart2_init(); not in use */
	/* dove_uart3_init(); not in use */
	dove_i2c_init();
	dove_i2c_exp_init(0);
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_i2s_init(0, &i2s0_data);

	dove_vpro_init();
	dove_gpu_init();
	dove_cesa_init();
	dove_hwmon_init();

	dove_cam_init(&dove_cafe_cam_data);
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_avng_nb_lcd0_dmi, 
			   &dove_anvg_nb_lcd0_vid_dmi,
			   &dove_anvg_nb_lcd1_dmi,
			   &dove_anvg_nb_lcd1_vid_dmi,
			   &dove_anvg_nb_backlight_data);
#endif /* CONFIG_FB_DOVE */
	/* dove_tact_init(&tact_dove_fp_data); gpio-mouse driver, not in use */
	/*
	 * On-board device registration
	 */

	spi_register_board_info(dove_rd_avng_nb_spi_flash_info,
				ARRAY_SIZE(dove_rd_avng_nb_spi_flash_info));
	i2c_register_board_info(0, dove_rd_avng_nb_i2c_devs,
				ARRAY_SIZE(dove_rd_avng_nb_i2c_devs));
}

MACHINE_START(DOVE_RD_AVNG_NB_Z0, "Marvell MV88F6781-RD Z0 Avengers Net Book Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_rd_avng_nb_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VPRO and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
