/*
 * arch/arm/mach-dove/dove-rd-avng-z0-setup.c
 *
 * Marvell Dove MV88F6781-RD Z0 Avengers Mobile Internet Device Board Setup
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
extern int mvmpp_sys_init(void);

static unsigned int use_hal_giga = 1;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif


/*****************************************************************************
 * BACKLIGHT
 ****************************************************************************/
static struct dovebl_platform_data backlight_data = {
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
static struct dovefb_mach_info dove_avng_lcd0_dmi = {
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

static struct dovefb_mach_info dove_anvg_lcd0_vid_dmi = {
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

static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 1,
	.spdif_rec	= 1,
};

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

#if 0
/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_rd_avng_pci_init(void)
{
	if (machine_is_dove_rd_avng_z0()) {
		dove_pcie_init(1, 1);
	}

	return 0;
}

subsys_initcall(dove_rd_avng_pci_init);
#endif

/*****************************************************************************
 * I2C devices:
 * 	Audio codec CS42L51-CNZ, address 0x4A
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_avng_i2c_devs[] = {
	{
		I2C_BOARD_INFO("i2s_i2c", 0x4A),
	},
#ifdef CONFIG_MV_TOUCH_KEY
	{
		I2C_BOARD_INFO("touch_key_i2c", 0x0b),
		.irq = 18 + IRQ_DOVE_GPIO_START,
	},
#endif
#ifdef CONFIG_MV_TOUCH_SLIDER
	{
		I2C_BOARD_INFO("touch_slider_i2c", 0x0a),
		.irq = 19 + IRQ_DOVE_GPIO_START,
	},
#endif
#ifdef CONFIG_APM_EMU_DS2782
	{
		I2C_BOARD_INFO("ds2782", 0x34),
	},
#endif
};

/*****************************************************************************
 * Camera
 ****************************************************************************/
static struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 2, //CTL1 connected to the sensor power down
	.reset		= 1, //CTL0 connected to the sensor reset
};

static int __init dove_rd_avng_cam_init(void)
{
	if (machine_is_dove_rd_avng_z0())
		dove_cam_init(&dove_cafe_cam_data);
	
	return 0;
}

late_initcall(dove_rd_avng_cam_init);

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
	{ 3, MPP_GPIO },	/* STBY_DETECTED */
	{ 4, MPP_GPIO },	/* SHDN_DETECTED */
	{ 5, MPP_GPIO },	/* STBY_REQUEST */
	{ 6, MPP_GPIO },	/* SHDN_REQUEST */

	{ 7, MPP_GPIO },	/* KEY_F1 */
	{ 8, MPP_GPIO },	/* KEY_F2 */
	{ 9, MPP_GPIO },	/* KEY_HOLD */
	{ 10, MPP_GPIO },	/* KEY_UP*/
	{ 11, MPP_GPIO },	/* KEY_DOWN */
	{ 12, MPP_GPIO },	/* TKEY_INT0 */
	{ 13, MPP_GPIO },	/* TKEY_INT1 */

	{ 14, MPP_GPIO },	/* DOCK_DET# */
	{ 15, MPP_GPIO },	/* AC_PlugIn */

	{ 18, MPP_GPIO },	/* TSC_RST */
	{ 19, MPP_GPIO },	/* TSC_INTR */
	{ 20, MPP_SPI1 },	/* TSC_MISO */
	{ 21, MPP_SPI1 },	/* TSC_CS */
	{ 22, MPP_SPI1 },	/* TSC_MOSI */
	{ 23, MPP_SPI1 },	/* TSC_SCK */

	{ 24, MPP_CAM },	/* will configure MPPs 24-39*/
	{ 40, MPP_SDIO0 },	/* will configure MPPs 40-45 */
	{ 46, MPP_SDIO1 },	/* will configure MPPs 46-51 */
	{ 52, MPP_GPIO },	/* AU1 Group to GPIO */
	{ 58, MPP_SPI0 },	/* will configure MPPs 58-61 */
	{ 62, MPP_GPIO },	/* UA1 Group to GPIO */
	{ -1 },
};

/*****************************************************************************
 * GPIO
 ****************************************************************************/
static void dove_rd_avng_shutdown(void)
{
	gpio_set_value(6, 1);
	mdelay(10);
	gpio_set_value(6, 0);
}

static void dove_rd_avng_gpio_init(void)
{
	orion_gpio_set_valid(3, 1);
	if (gpio_request(3, "STBY_DETECTED") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for STBY_DETECTED\n");	
	gpio_direction_input(3);
	orion_gpio_set_valid(4, 1);
	if (gpio_request(4, "SHDN_DETECTED") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SHDN_DETECTED\n");	
	gpio_direction_input(4);
	if (gpio_request(5, "STBY_REQUEST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for STBY_REQUEST\n");	
	gpio_direction_output(5, 0);
	orion_gpio_set_valid(6, 1);
	if (gpio_request(6, "SHDN_REQUEST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SHDN_REQUEST\n");	
	gpio_direction_output(6, 0);

	/* TOUCH_KEY */
	orion_gpio_set_valid(7, 1);
	if (gpio_request(7, "Key_F1") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Key_F1\n");	
	gpio_direction_input(7);
	orion_gpio_set_valid(8, 1);
	if (gpio_request(8, "Key_F2") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Key_F2\n");	
	gpio_direction_input(8);
	orion_gpio_set_valid(9, 1);
	if (gpio_request(9, "Key_Hold") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Key_Hold\n");	
	gpio_direction_input(9);
	orion_gpio_set_valid(10, 1);
	if (gpio_request(10, "Key_Up") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Key_Up\n");	
	gpio_direction_input(10);
	orion_gpio_set_valid(11, 1);
	if (gpio_request(11, "Key_Down") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Key_Down\n");	
	gpio_direction_input(11);
	orion_gpio_set_valid(12, 1);
	if (gpio_request(12, "TKEY_INT0") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for TKEY_INT0\n");	
	gpio_direction_input(12);
	orion_gpio_set_valid(13, 1);
	if (gpio_request(13, "TKEY_INT1") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for TKEY_INT1\n");	
	gpio_direction_input(13);

	orion_gpio_set_valid(14, 1);
	if (gpio_request(14, "DOCK_DETn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for DOCK_DETn\n");	
	gpio_direction_input(14);
	orion_gpio_set_valid(15, 1);
	if (gpio_request(15, "AC_PlugIn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for AC_PlugIn\n");	
	gpio_direction_input(15);

	orion_gpio_set_valid(53, 1);
	if (gpio_request(53, "Ph_LineINn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Ph_LineINn\n");	
	gpio_direction_input(53);

	orion_gpio_set_valid(62, 1);
	if (gpio_request(62, "GP_VTTON") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_VTTON\n");	
	gpio_direction_output(62, 1);
	orion_gpio_set_valid(63, 1);
	if (gpio_request(63, "CMRA_V2.8_EN") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for CMRA_V2.8_EN\n");	
	gpio_direction_output(63, 1);
}


/*****************************************************************************
 * Touch screen
 ****************************************************************************/
#include <linux/spi/tsc200x.h>

#define DOVE_AVNG_TS_RESET_GPIO	(18)
#define DOVE_AVNG_TS_PEN_GPIO	(19)
#define DOVE_AVNG_TS_PEN_IRQ	(DOVE_AVNG_TS_PEN_GPIO + IRQ_DOVE_GPIO_START)


static struct tsc2005_platform_data ts_info = {

	.model			= 2005,
	.x_plate_ohms		= 450,
	.y_plate_ohms		= 250,
};


struct spi_board_info __initdata dove_avng_spi_devs[] = {
	{
		.modalias  		= "tsc2005",
		.irq			= DOVE_AVNG_TS_PEN_IRQ,
		.max_speed_hz		= 10000000, //10MHz
		.bus_num		= 1,
		.chip_select		= 0,
		.mode		        = SPI_MODE_0,      
		.platform_data          = &ts_info,
	},
};

static void __init dove_avng_ts_gpio_setup(void)
{
	/* Reset the touch screen controller */
	orion_gpio_set_valid(DOVE_AVNG_TS_RESET_GPIO, 1);
	if (gpio_request(DOVE_AVNG_TS_RESET_GPIO,"DOVE_TS_RESET") != 0)
		printk(KERN_ERR "Dove: failed to setup TS RESET GPIO\n");	
	gpio_direction_output(DOVE_AVNG_TS_RESET_GPIO, 0);
	mdelay(10);
	gpio_direction_output(DOVE_AVNG_TS_RESET_GPIO, 1);

	/* Configure IRQ for the touch screen controller */
	orion_gpio_set_valid(DOVE_AVNG_TS_PEN_GPIO, 1);
	if (gpio_request(DOVE_AVNG_TS_PEN_GPIO, "DOVE_TS_PEN_IRQ") != 0)
		printk(KERN_ERR "Dove: failed to setup TS IRQ GPIO\n");
	if (gpio_direction_input(DOVE_AVNG_TS_PEN_GPIO) != 0)
		printk(KERN_ERR "%s failed "
		       "to set output pin %d\n", __func__,
		       DOVE_AVNG_TS_PEN_GPIO);
	set_irq_type(DOVE_AVNG_TS_PEN_IRQ, IRQ_TYPE_LEVEL_LOW);
}

/*****************************************************************************
 * Board Init
 ****************************************************************************/
static void __init dove_rd_avng_init(void)
{
	arm_shut_down = dove_rd_avng_shutdown;

	/*
	 * Basic Dove setup (needs to be called early).
	 */
	dove_init();
	dove_mpp_conf(dove_rd_avng_mpp_modes);
	dove_rd_avng_gpio_init();

	dove_avng_ts_gpio_setup();

	/* card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);
	dove_sd_card_int_wa_setup(1);

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

	/* dove_sata_init(&dove_rd_sata_data); */
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
	dove_rd_avng_nfc_init();
	dove_i2s_init(0, &i2s0_data);

	dove_vpro_init();
	dove_gpu_init();
	dove_cesa_init();
	dove_hwmon_init();

#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_avng_lcd0_dmi, 
			   &dove_anvg_lcd0_vid_dmi,
			   NULL, NULL, &backlight_data);
#endif /* CONFIG_FB_DOVE */
	/* dove_tact_init(&tact_dove_fp_data); gpio-mouse driver, not in use */
	/*
	 * On-board device registration
	 */

	spi_register_board_info(dove_rd_avng_spi_flash_info,
				ARRAY_SIZE(dove_rd_avng_spi_flash_info));
	spi_register_board_info(dove_avng_spi_devs,
				ARRAY_SIZE(dove_avng_spi_devs));
	i2c_register_board_info(0, dove_rd_avng_i2c_devs,
				ARRAY_SIZE(dove_rd_avng_i2c_devs));

	mvmpp_sys_init();
}

MACHINE_START(DOVE_RD_AVNG_Z0, "Marvell MV88F6781-RD Avengers Z0 MID Board")
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
