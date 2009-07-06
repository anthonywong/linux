/*
 * arch/arm/mach-dove/dove-rd-avng-setup.c
 *
 * Marvell Dove MV88F6781-RD Avengers Mobile Internet Device Board Setup
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
#include <linux/pci.h>
#include <linux/gpio_mouse.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/flash.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>
#include <plat/i2s-orion.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/mach/arch.h>
#include <mach/dove.h>
#include <asm/hardware/pxa-dma.h>
#include <mach/dove_nand.h>
#include <plat/cafe-orion.h>
#include "common.h"
#include "clock.h"
#include "mpp.h"
#include "pm.h"
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);
extern void (*arm_shut_down)(void);
extern int mvmpp_sys_init(void);

static unsigned int use_hal_giga = 1;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif

/*****************************************************************************
 * LCD
 ****************************************************************************/
/*
 * LCD input clock.
 */
#define LCD_SCLK	(CONFIG_FB_DOVE_CLCD_SCLK_VALUE*1000*1000)

static struct dovefb_mach_info dove_rd_avng_lcd0_dmi = {
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

static struct dovefb_mach_info dove_rd_avng_lcd0_vid_dmi = {
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

/*****************************************************************************
 * BACKLIGHT
 ****************************************************************************/
static struct dovebl_platform_data dove_rd_avng_backlight_data = {
	.default_intensity = 0xa,
	.max_brightness = 0xe,
	.gpio_pm_control = 1,
};

void __init dove_rd_avng_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_rd_avng_lcd0_dmi, &dove_rd_avng_lcd0_vid_dmi,
			   NULL, NULL,
			   &dove_rd_avng_backlight_data);
#endif /* CONFIG_FB_DOVE */
}

/*****************************************************************************
 * I2C devices:
 * 	MCU PIC-xx, address 0x??
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_avng_i2c_devs[] = {
#if 0
	{
		I2C_BOARD_INFO("pic-xx", 0x??),
	},
#endif
};

/*****************************************************************************
 * Camera
 ****************************************************************************/
static struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 2, //CTL0 connected to the sensor power down
	.reset		= 1, //CTL1 connected to the sensor reset
};

static int __init dove_rd_avng_cam_init(void)
{
	if (machine_is_dove_rd_avng())
		dove_cam_init(&dove_cafe_cam_data);
	
	return 0;
}

late_initcall(dove_rd_avng_cam_init);

/*****************************************************************************
 * Ethernet
 ****************************************************************************/
static struct mv643xx_eth_platform_data dove_rd_avng_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};

/*****************************************************************************
 * SPI Devices:
 *     SPI0: 4M Flash MX25L3205D
 ****************************************************************************/
static const struct flash_platform_data dove_rd_avng_spi_flash_data = {
	.type           = "mx25l3205",
};

static struct spi_board_info __initdata dove_rd_avng_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_rd_avng_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * NAND
 ****************************************************************************/
static struct mtd_partition partition_dove[] = {
	{ .name		= "Root",
	  .offset	= 0,
	  .size		= MTDPART_SIZ_FULL },
};
static u64 nfc_dmamask = DMA_BIT_MASK(32);
static struct dove_nand_platform_data dove_rd_avng_nfc_data = {
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
		.platform_data		= &dove_rd_avng_nfc_data,
	},
	.resource	= dove_nfc_resources,
	.num_resources	= ARRAY_SIZE(dove_nfc_resources),
};

static void __init dove_rd_avng_nfc_init(void)
{
	dove_rd_avng_nfc_data.tclk = dove_tclk_get();
	platform_device_register(&dove_nfc);
}

/*****************************************************************************
 * MPP
 ****************************************************************************/
static struct dove_mpp_mode dove_rd_avng_mpp_modes[] __initdata = {
	{ 0, MPP_GPIO },	/* MCU_INTRn */
	{ 1, MPP_GPIO },	/* MCU_REQUEST */
	{ 2, MPP_PMU },		/* Standby power control */
	{ 3, MPP_PMU },		/* Power button - standby wakeup */
	{ 4, MPP_PMU },		/* Core power good indication */
	{ 5, MPP_PMU },		/* DeepIdle power control */
	{ 6, MPP_GPIO },	/* PMU - DDR termination control */
	{ 7, MPP_GPIO },	/* DOVE_REQUEST */

	{ 8, MPP_GPIO },	/* OFF_CTRL */

	{ 9, MPP_PMU },		/* Cpu power good indication */
	{ 10, MPP_PMU },	/* DVS SDI control */

	{ 11, MPP_GPIO },	/* KEY_HOLD */
	{ 12, MPP_GPIO },	/* 3G_RF_DISABLE */
	{ 13, MPP_GPIO },	/* MINICARD_WAKE */
	{ 14, MPP_GPIO },	/* GP_PD_WLANn */
	{ 15, MPP_GPIO },	/* AC_PlugIn */
	{ 16, MPP_GPIO },	/* GP_WKUP_WLAN */
	{ 17, MPP_GPIO },	/* GP_SLP_WLANn */

	{ 18, MPP_GPIO },	/* CMMB_RST */
	{ 19, MPP_GPIO },	/* CMMB_INTR */
	{ 20, MPP_SPI1 },	/* CMMB_MISO */
	{ 21, MPP_SPI1 },	/* CMMB_CS */
	{ 22, MPP_SPI1 },	/* CMMB_MOSI */
	{ 23, MPP_SPI1 },	/* CMMB_SCK */

	{ 52, MPP_GPIO },	/* AU1 Group to GPIO */
	{ 62, MPP_GPIO },	/* UA1 Group to GPIO */
	{ -1 },
};

/*****************************************************************************
 * GPIO
 ****************************************************************************/
static void dove_rd_avng_shutdown(void)
{
	gpio_set_value(7, 1);
	mdelay(10);
	gpio_set_value(7, 0);
}

static void dove_rd_avng_gpio_init(void)
{
	orion_gpio_set_valid(0, 1);
	if (gpio_request(0, "MCU_INTRn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MCU_INTRn\n");	
	gpio_direction_input(0);	/* MCU interrupt */
	orion_gpio_set_valid(1, 1);
	if (gpio_request(1, "MCU_REQUEST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MCU_REQUEST\n");	
	gpio_direction_input(1);	/* MCU request */

	orion_gpio_set_valid(6, 1);
	if (gpio_request(6, "MPP_DDR_TERM") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MPP_DDR_TERM\n");	
	gpio_direction_output(6, 1);	/* Enable DDR 1.8v */
	orion_gpio_set_valid(7, 1);
	if (gpio_request(7, "DOVE_REQUEST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for DOVE_REQUEST\n");	
	gpio_direction_output(7, 0);	/* Dove request */
	orion_gpio_set_valid(8, 1);
	if (gpio_request(8, "OFF_CTRL") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for OFF_CTRL\n");	
	gpio_direction_output(8, 0);	/* Power off */

	orion_gpio_set_valid(11, 1);
	if (gpio_request(11, "Key_Hold") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Key_Hold\n");	
	gpio_direction_input(11);
	orion_gpio_set_valid(12, 1);
	if (gpio_request(12, "3G_RF_DISABLE") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for 3G_RF_DISABLE\n");	
	gpio_direction_output(12, 1);
	orion_gpio_set_valid(13, 1);
	if (gpio_request(13, "MINICARD_WAKE") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MINICARD_WAKE\n");	
	gpio_direction_input(13);
	orion_gpio_set_valid(14, 1);
	if (gpio_request(14, "GP_PD_WLANn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_PD_WLANn\n");	
	gpio_direction_output(14, 1);
	orion_gpio_set_valid(16, 1);
	if (gpio_request(16, "GP_WKUP_WLAN") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_WKUP_WLAN\n");	
	gpio_direction_input(16);
	orion_gpio_set_valid(17, 1);
	if (gpio_request(17, "GP_SLP_WLANn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_SLP_WLANn\n");	
	gpio_direction_output(17, 1);
	orion_gpio_set_valid(18, 1);
	if (gpio_request(18, "CMMB_RSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMMB_RSTn\n");	
	gpio_direction_output(18, 1);
	orion_gpio_set_valid(19, 1);
	if (gpio_request(19, "CMMB_INTRn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMMB_INTRn\n");	
	gpio_direction_input(19);

	orion_gpio_set_valid(52, 1);
	if (gpio_request(52, "GP_WLAN_RSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_WLAN_RSTn\n");	
	gpio_direction_input(52);
	orion_gpio_set_valid(53, 1);
	if (gpio_request(53, "Ph_LineINn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Ph_LineINn\n");	
	gpio_direction_input(53);	/* Ph_LineINn */
	orion_gpio_set_valid(54, 1);
	if (gpio_request(54, "GEPHY_PWRDWNn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GEPHY_PWRDWNn\n");	
	gpio_direction_output(54, 1);	/* GEPHY_PWRDWNn */
	orion_gpio_set_valid(55, 1);
	if (gpio_request(55, "HUB_RESETn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for HUB_RESETn\n");	
	gpio_direction_output(55, 1);	/* HUB_RESETn */
	orion_gpio_set_valid(56, 1);
	if (gpio_request(56, "CMMB_EN") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMMB_EN\n");	
	gpio_direction_output(56, 1);	/* CMMB_EN */
	orion_gpio_set_valid(57, 1);
	if (gpio_request(57, "MINICARD_PRSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MINICARD_PRSTn\n");	
	gpio_direction_output(57, 1);	/* MINICARD_PRSTn */
	orion_gpio_set_valid(62, 1);
	if (gpio_request(62, "AU_IRQOUTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for AU_IRQOUTn\n");	
	gpio_direction_input(62);	/* AU_IRQOUTn */
	orion_gpio_set_valid(63, 1);
	if (gpio_request(63, "DOCK_DETn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for DOCK_DETn\n");	
	gpio_direction_input(63);	/* DOCK_DETn */
}

/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
static int __init dove_rd_avng_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;	

	pmuInitInfo.deepIdleStatus = MV_FALSE; 			/* Disable L2 retention */
	pmuInitInfo.cpuPwrGoodEn = MV_FALSE;			/* Don't wait for external power good signal */
	pmuInitInfo.batFltMngDis = MV_FALSE;			/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;			/* Keep exit from STANDBY on battery fail enabled */
	pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_SLP_PWRDWN;	/* STANDBY => 0: I/O off, 1: I/O on */
	pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;	/* power on push button */
	pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CPU_PWRGOOD;	/* CORE power good used as Standby PG */
	pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_CPU_PWRDWN;	/* DEEP-IdLE => 0: CPU off, 1: CPU on */
	pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_NC;		/* Charger interrupt - not used */
	pmuInitInfo.sigSelctor[7] = PMU_SIGNAL_1;		/* Standby Led - inverted */
	pmuInitInfo.sigSelctor[8] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[9] = PMU_SIGNAL_NC;		/* CPU power good  - not used */
	pmuInitInfo.sigSelctor[10] = PMU_SIGNAL_SDI;		/* Voltage regulator control */
	pmuInitInfo.sigSelctor[11] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[12] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[13] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[14] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[15] = PMU_SIGNAL_NC;
	pmuInitInfo.dvsDelay = 0;				/* PMU cc delay for DVS change */
	pmuInitInfo.ddrTermGpioNum = 6;				/* GPIO 6 used to disable terminations */

	/* Initialize the PMU HAL */
	if (mvPmuInit(&pmuInitInfo) != MV_OK)
		printk(KERN_NOTICE "Failed to initialive the PMU!\n");

	/* Configure wakeup events */
	mvPmuWakeupEventSet(PMU_STBY_WKUP_CTRL_EXT0_FALL | PMU_STBY_WKUP_CTRL_RTC_MASK);

	/* Register the PM operation in the Linux stack */
	dove_pm_register();

	return 0;
}

__initcall(dove_rd_avng_pm_init);

/*****************************************************************************
 * Board Init
 ****************************************************************************/
static void __init dove_rd_avng_init(void)
{
	arm_shut_down = dove_rd_avng_shutdown;

	/*
	 * Basic Dove setup. Needs to be called early.
	 */
	dove_init();
	dove_mpp_conf(dove_rd_avng_mpp_modes);
	dove_rd_avng_gpio_init();

	/* sdio card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);
	dove_sd_card_int_wa_setup(1);

	/* Initialize AC'97 related regs. */
	dove_ac97_setup();

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
	dove_ge00_init(&dove_rd_avng_ge00_data);
	dove_ehci0_init();
	dove_ehci1_init();
	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
	ds_clks_disable_all(0, 0);
	dove_spi0_init(0);
	dove_spi1_init(1);

	/* uart0 is the debug port, register it first so it will be */
	/* represented by device ttyS0, root filesystems usually expect the */
	/* console to be on that device */
	dove_uart0_init();
	/* dove_uart1_init(); not in use */
	dove_i2c_init();
	dove_i2c_exp_init(0);
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_rd_avng_nfc_init();
	dove_rd_avng_clcd_init();
	dove_vpro_init();
	dove_gpu_init();
	dove_cesa_init();
	dove_hwmon_init();

	i2c_register_board_info(0, dove_rd_avng_i2c_devs,
				ARRAY_SIZE(dove_rd_avng_i2c_devs));
	spi_register_board_info(dove_rd_avng_spi_flash_info,
				ARRAY_SIZE(dove_rd_avng_spi_flash_info));

	mvmpp_sys_init();
}

MACHINE_START(DOVE_RD_AVNG, "Marvell MV88F6781-RD Avengers MID Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_rd_avng_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VPRO and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
