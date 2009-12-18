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
#include <mach/pm.h>
#include <plat/cafe-orion.h>
#include "common.h"
#include "clock.h"
#include "mpp.h"
#include "mach/pm.h"
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"
#include "pdma/mvPdma.h"
#include "gpp/mvGppRegs.h"
#include <ctrlEnv/mvCtrlEnvRegs.h>
#include <audio/mvAudioRegs.h>

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);
extern int mvmpp_sys_init(void);

#ifdef CONFIG_CPU_ENDIAN_BE8
static unsigned int use_hal_giga = 1;
#else
/* FIXME:GE00 in BE8 mode still can't work now. 
 */
static unsigned int use_hal_giga = 0;
#endif

#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif

extern unsigned int useHalDrivers;
extern char *useNandHal;

/*****************************************************************************
 * LCD
 ****************************************************************************/
/*
 * LCD input clock.
 */
#define LCD_SCLK	(CONFIG_FB_DOVE_CLCD_SCLK_VALUE*1000*1000)

void dovefb_config_lcd_power(unsigned int reg_offset, int on)
{
		unsigned int x;
		volatile unsigned int* addr;

		addr = DOVE_SB_REGS_VIRT_BASE+GPP_DATA_OUT_REG(0);
		/*
		 * LCD Power Control through dedicated pins.
	 	 * Suppose MPP has been set to right mode.
		 * We just set the value directly.
		 */
		x = *addr;
		x &= ~(0x1 << 11);
		x |= (on << 11);
		*addr = x;
}
EXPORT_SYMBOL(dovefb_config_lcd_power);

static struct dovefb_mach_info dove_rd_avng_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.sclk_clock		= LCD_SCLK,
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
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
};

static struct dovefb_mach_info dove_rd_avng_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
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
	.enable_lcd0		= 0,
};

static struct dovefb_mach_info dove_rd_avng_lcd1_dmi = {
	.id_gfx			= "GFX Layer 1",
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
	.ddc_i2c_adapter	= 3,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_rd_avng_lcd1_vid_dmi = {
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

void dovefb_config_backlight_power(unsigned int reg_offset, int on)
{
	if (reg_offset) {
		/*
		 * BL Power Control through bit of register.
		 */
	} else {
		unsigned int x;
		volatile unsigned int* addr;

		addr = DOVE_SB_REGS_VIRT_BASE+GPP_DATA_OUT_REG(0);
		/*
		 * BL Power Control through dedicated pins.
	 	 * Suppose MPP has been set to right mode.
		 * We just set the value directly.
		 */
		x = *addr;
		x &= ~(0x1 << 17);
		x |= (on << 17);
		*addr = x;
	}

}
EXPORT_SYMBOL(dovefb_config_backlight_power);

void __init dove_rd_avng_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_rd_avng_lcd0_dmi, &dove_rd_avng_lcd0_vid_dmi,
			   &dove_rd_avng_lcd1_dmi, &dove_rd_avng_lcd1_vid_dmi,
			   &dove_rd_avng_backlight_data);
#endif /* CONFIG_FB_DOVE */
}

/*****************************************************************************
 * I2C devices:
 * 	ALC5630 codec, address 0x
 * 	Battery charger, address 0x??
 * 	G-Sensor, address 0x??
 * 	MCU PIC-16F887, address 0x??
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_avng_i2c_devs[] = {
	{
		I2C_BOARD_INFO("i2s_i2c", 0x1F),
	},
	{
		I2C_BOARD_INFO("bma020", 0x38),	//JP
		.irq = IRQ_DOVE_GPIO_16_23,	//JP
	},
	{
		I2C_BOARD_INFO("anx7150_i2c", 0x3B),//0x3B*2->0x76..For ANX7150

	},
	{
		I2C_BOARD_INFO("kg2_i2c", 0x10),	//0x10 for KG2

	},
#ifdef CONFIG_CH7025_COMPOSITE
	{
		I2C_BOARD_INFO("ch7025_i2c",0x76),
	},
#endif
#if 0
	{
		I2C_BOARD_INFO("pic-16f887", 0x??),
	},
#endif
};

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_rd_avng_pci_init(void)
{
	if (machine_is_dove_rd_avng())
			dove_pcie_init(1, 1);

	return 0;
}

subsys_initcall(dove_rd_avng_pci_init);

/*****************************************************************************
 * Camera
 ****************************************************************************/
static struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 2, //CTL0 connected to the sensor power down
	.reset		= 1, //CTL1 connected to the sensor reset
	.numbered_i2c_bus = 1,
	.i2c_bus_id	= 3,
};

static int __init dove_rd_avng_cam_init(void)
{
	if (machine_is_dove_rd_avng())
		dove_cam_init(&dove_cafe_cam_data);
	
	return 0;
}

late_initcall(dove_rd_avng_cam_init);

/*****************************************************************************
 * Audio I2S
 ****************************************************************************/
static struct orion_i2s_platform_data i2s1_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 1,
};

/*****************************************************************************
 * Ethernet
 ****************************************************************************/
static struct mv643xx_eth_platform_data dove_rd_avng_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};

/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data dove_rd_avng_sata_data = {
        .n_ports        = 1,
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
static struct dove_nand_platform_data dove_rd_avng_nfc_hal_data = {
	.nfc_width      = 8,
	.num_devs       = 1,
	.num_cs         = 2,
	.use_dma	= 1,
	.use_ecc	= 1,
	.use_bch	= 1,
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
};

static struct dove_nand_platform_data dove_rd_avng_nfc_gang_hal_data = {
	.nfc_width      = 16,
	.num_devs       = 2,
	.num_cs         = 2,
	.use_dma	= 1,
	.use_ecc	= 1,
	.use_bch	= 1,
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
};

static struct dove_nand_platform_data dove_rd_avng_nfc_data = {
	.nfc_width      = 8,
	.num_devs       = 1,
	.use_dma        = 1,
	.use_ecc        = 1,
	.use_bch        = 0,
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

	if(useHalDrivers || useNandHal) {
		dove_nfc.name = "dove-nand-hal";
		if(useNandHal && (strcmp(useNandHal, "ganged") == 0)) {
			dove_rd_avng_nfc_gang_hal_data.tclk = dove_tclk_get();
			dove_nfc.dev.platform_data = &dove_rd_avng_nfc_gang_hal_data;
		} else {
			dove_rd_avng_nfc_hal_data.tclk = dove_tclk_get();
			dove_nfc.dev.platform_data = &dove_rd_avng_nfc_hal_data;
		}
	}

	platform_device_register(&dove_nfc);
}

/*****************************************************************************
 * MPP
 ****************************************************************************/
static struct dove_mpp_mode dove_rd_avng_mpp_modes[] __initdata = {
	{ 0, MPP_GPIO },	/* MCU_INTRn */
	{ 1, MPP_GPIO },	/* I2S_CODEC_IRQ */
	{ 2, MPP_PMU },		/* Standby power control */
	{ 3, MPP_PMU },		/* Power button - standby wakeup */
	{ 4, MPP_PMU },		/* Core power good indication */
	{ 5, MPP_PMU },		/* DeepIdle power control */
	{ 6, MPP_GPIO },	/* PMU - DDR termination control */
	{ 7, MPP_PMU  },	/* Standby led */

	{ 8, MPP_GPIO },	/* OFF_CTRL */

	{ 9, MPP_GPIO },	/* HUB_RESETn */
	{ 10, MPP_PMU },	/* DVS SDI control */

	{ 11, MPP_GPIO },	/* LCM_DCM */
	{ 12, MPP_SDIO1 },	/* SD1_CDn */
	{ 13, MPP_GPIO },	/* WLAN_WAKEUP_HOST */
	{ 14, MPP_GPIO },	/* HOST_WAKEUP_WLAN */
	{ 15, MPP_GPIO },	/* BT_WAKEUP_HOST */
	{ 16, MPP_GPIO },	/* HOST_WAKEUP_BT */
	{ 17, MPP_GPIO },	/* LCM_BL_CTRL */

	{ 18, MPP_LCD },	/* LCD0_PWM */
	{ 19, MPP_GPIO },	/* AU_IRQOUT */
	{ 20, MPP_GPIO },	/* GP_WLAN_RSTn */
	{ 21, MPP_UART1 },	/* UA1_RTSn */
	{ 22, MPP_UART1 },	/* UA1_CTSn */
	{ 23, MPP_GPIO },	/* G_INT */

	{ 24, MPP_CAM },	/* will configure MPPs 24-39*/

	{ 40, MPP_SDIO0 },	/* will configure MPPs 40-45 */

	{ 46, MPP_SDIO1 },	/* SD1 Group */
	{ 47, MPP_SDIO1 },	/* SD1 Group */
	{ 48, MPP_SDIO1 },	/* SD1 Group */
	{ 49, MPP_SDIO1 },	/* SD1 Group */
	{ 50, MPP_SDIO1 },	/* SD1 Group */
	{ 51, MPP_SDIO1 },	/* SD1 Group */

	{ 52, MPP_AUDIO1 },	/* AU1 Group */
	{ 53, MPP_AUDIO1 },	/* AU1 Group */
	{ 54, MPP_AUDIO1 },	/* AU1 Group */
	{ 55, MPP_AUDIO1 },	/* AU1 Group */
	{ 56, MPP_AUDIO1 },	/* AU1 Group */
	{ 57, MPP_AUDIO1 },	/* AU1 Group */

	{ 58, MPP_SPI0 },	/* will configure MPPs 58-61 */
	{ 62, MPP_UART1 },	/* UART1 active */
	{ -1 },
};

/*****************************************************************************
 * GPIO
 ****************************************************************************/
static void dove_rd_avng_power_off(void)
{
	/* XXX, need to tell MCU via I2C command */
}

static void dove_rd_avng_gpio_init(void)
{
	orion_gpio_set_valid(0, 1);
	if (gpio_request(0, "MCU_INTRn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MCU_INTRn\n");	
	gpio_direction_input(0);	/* MCU interrupt */
	orion_gpio_set_valid(1, 1);
	if (gpio_request(1, "I2S_CODEC_IRQ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for I2S_CODEC_IRQ\n");	
	gpio_direction_input(1);	/* Interrupt from ALC5632 */

	orion_gpio_set_valid(6, 1);
	if (gpio_request(6, "MPP_DDR_TERM") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MPP_DDR_TERM\n");	
	gpio_direction_output(6, 1);	/* Enable DDR 1.8v */
	orion_gpio_set_valid(8, 1);
	if (gpio_request(8, "OFF_CTRL") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for OFF_CTRL\n");	
	gpio_direction_output(8, 0);	/* Power off */
	orion_gpio_set_valid(9, 1);
	if (gpio_request(9, "HUB_RESETn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for HUB_RESETn\n");	
	gpio_direction_output(9, 1);	/* HUB_RESETn */

	orion_gpio_set_valid(11, 1);
	if (gpio_request(11, "LCM_DCM") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for LCM_DCM\n");	
	gpio_direction_output(11,1);	/* Enable LCD power */
	orion_gpio_set_valid(13, 1);
	if (gpio_request(13, "WLAN_WAKEUP_HOST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for WLAN_WAKEUP_HOST\n");	
	gpio_direction_input(13);
	orion_gpio_set_valid(14, 1);
	if (gpio_request(14, "HOST_WAKEUP_WLAN") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for HOST_WAKEUP_WLAN\n");	
	gpio_direction_output(14, 0);
	orion_gpio_set_valid(15, 1);
	if (gpio_request(15, "BT_WAKEUP_HOST") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for BT_WAKEUP_HOSTn");	
	gpio_direction_input(15);
	orion_gpio_set_valid(16, 1);
	if (gpio_request(16, "HOST_WAKEUP_BT") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for HOST_WAKEUP_BT\n");	
	gpio_direction_output(16, 0);
	orion_gpio_set_valid(17, 1);
	if (gpio_request(17, "LCM_BL_CTRL") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for LCM_BL_CTRL\n");	
	gpio_direction_output(17, 1);	/* Enable LCD back light */

	orion_gpio_set_valid(20, 1);
	if (gpio_request(20, "GP_WLAN_RSTn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GP_WLAN_RSTn\n");	
	gpio_direction_output(20, 1);
	orion_gpio_set_valid(23, 1);
	if (gpio_request(23, "G_INT") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for G_INT\n");	
	gpio_direction_input(23);	/* Interrupt from G-sensor */
}

/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
static int __init dove_rd_avng_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;	
	u32 dev, rev;

	if (!machine_is_dove_rd_avng())
		return 0;

	dove_pcie_id(&dev, &rev);

	pmuInitInfo.batFltMngDis = MV_FALSE;			/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;			/* Keep exit from STANDBY on battery fail enabled */
	pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_SLP_PWRDWN;	/* STANDBY => 0: I/O off, 1: I/O on */
	pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;	/* power on push button */
	if (rev >= DOVE_REV_X0) /* For X0 and higher Power Good indication is not needed */
		pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_NC;
	else
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
	pmuInitInfo.dvsDelay = 0x4200;				/* ~100us in 166MHz cc - delay for DVS change */
	pmuInitInfo.ddrTermGpioNum = 6;				/* GPIO 6 used to disable terminations */
	if (rev >= DOVE_REV_X0) /* For X0 and higher wait at least 150ms + spare */
		pmuInitInfo.standbyPwrDelay = 0x2000;		/* 250ms delay to wait for complete powerup */
	else
		pmuInitInfo.standbyPwrDelay = 0x140;		/* 10ms delay after getting the power good indication */

	/* Initialize the PMU HAL */
	if (mvPmuInit(&pmuInitInfo) != MV_OK)
	{
		printk(KERN_NOTICE "Failed to initialise the PMU!\n");
		return 0;
	}

	/* Configure wakeup events */
	mvPmuWakeupEventSet(PMU_STBY_WKUP_CTRL_EXT0_FALL | PMU_STBY_WKUP_CTRL_RTC_MASK);

	/* Register the PM operation in the Linux stack */
	dove_pm_register();

	return 0;
}

__initcall(dove_rd_avng_pm_init);

/*****************************************************************************
 * AC97 Interface
 ****************************************************************************/

void __init dove_avng_ac97_init(void)
{
	uint32_t reg;

	/* Enable AC97 Control 			*/
	reg = readl(DOVE_SB_REGS_VIRT_BASE + MPP_GENERAL_CONTROL_REG);
	
	reg = (reg |0x10000);
	writel(reg, DOVE_SB_REGS_VIRT_BASE + MPP_GENERAL_CONTROL_REG);

	/* Set DCO clock to 24.576		*/
	reg = readl(DOVE_SB_REGS_VIRT_BASE + MV_AUDIO_DCO_CTRL_REG(0));
	reg = (reg & ~0x3) | 0x2;
	writel(reg, DOVE_SB_REGS_VIRT_BASE + MV_AUDIO_DCO_CTRL_REG(0));

}

/*****************************************************************************
 * AC97 Touch Panel Control
 ****************************************************************************/

#define DOVE_AVNG_AC97_TS_PEN_GPIO	(19)
#define DOVE_AVNG_AC97_TS_PEN_IRQ	(DOVE_AVNG_AC97_TS_PEN_GPIO+IRQ_DOVE_GPIO_START)

int __init dove_avng_ac97_ts_gpio_setup(void)
{
     
	orion_gpio_set_valid(DOVE_AVNG_AC97_TS_PEN_GPIO, 1);
	if (gpio_request(DOVE_AVNG_AC97_TS_PEN_GPIO, "DOVE_AVNG_AC97_TS_PEN_IRQ") != 0)
		pr_err("Dove: failed to setup TS IRQ GPIO\n");
	if (gpio_direction_input(DOVE_AVNG_AC97_TS_PEN_GPIO) != 0) {
		printk(KERN_ERR "%s failed "
		       "to set output pin %d\n", __func__,
		       DOVE_AVNG_AC97_TS_PEN_GPIO);
		gpio_free(DOVE_AVNG_AC97_TS_PEN_GPIO);
		return -1;
	}
	/* IRQ */
	set_irq_chip(DOVE_AVNG_AC97_TS_PEN_IRQ, &orion_gpio_irq_chip);
	set_irq_handler(DOVE_AVNG_AC97_TS_PEN_IRQ, handle_level_irq);
	set_irq_type(DOVE_AVNG_AC97_TS_PEN_IRQ, IRQ_TYPE_LEVEL_HIGH);

	return 0;
}

static struct resource dove_ac97_ts_resources[] = {

	[0] = {
		.start	= DOVE_AVNG_AC97_TS_PEN_IRQ,
		.end		= DOVE_AVNG_AC97_TS_PEN_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};
static struct platform_device dove_ac97_touch = {
	.name           = "rt5611_ts",
	.id             = 0,
	.num_resources  = 1,
	.resource       = dove_ac97_ts_resources,
};


void __init dove_ac97_ts_init(void)
{
	dove_avng_ac97_ts_gpio_setup();
	platform_device_register(&dove_ac97_touch);
}


/*****************************************************************************
 * Board Init
 ****************************************************************************/
static void __init dove_rd_avng_init(void)
{
	/*
	 * Basic Dove setup. Needs to be called early.
	 */
	dove_init();
	dove_mpp_conf(dove_rd_avng_mpp_modes);
	dove_rd_avng_gpio_init();

	pm_power_off = dove_rd_avng_power_off;

	/* sdio card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);
	dove_sd_card_int_wa_setup(1);

	/* Initialize AC'97 related regs. */
	dove_avng_ac97_init();
	dove_ac97_setup();
	dove_ac97_ts_init();
	dove_rtc_init();
	pxa_init_dma_wins(&dove_mbus_dram_info);
	pxa_init_dma(16);

	if(useHalDrivers || useNandHal) {
		if (mvPdmaHalInit(MV_PDMA_MAX_CHANNELS_NUM) != MV_OK) {
			printk(KERN_ERR "mvPdmaHalInit() failed.\n");
			BUG();
		}
		/* reserve channels for NAND Data and command PDMA */
		pxa_reserve_dma_channel(MV_PDMA_NAND_DATA);
		pxa_reserve_dma_channel(MV_PDMA_NAND_COMMAND);
	}

	dove_xor0_init();
	dove_xor1_init();
#ifdef CONFIG_MV_ETHERNET
	if(useHalDrivers || use_hal_giga)
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
	dove_sata_init(&dove_rd_avng_sata_data);
	dove_spi0_init(0);
	/* dove_spi1_init(1); */

	/* uart0 is the debug port, register it first so it will be */
	/* represented by device ttyS0, root filesystems usually expect the */
	/* console to be on that device */
	dove_uart0_init();
	dove_uart1_init();
	dove_i2c_init();
	dove_i2c_exp_init(0);
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_rd_avng_nfc_init();
	dove_rd_avng_clcd_init();
	dove_vmeta_init();
	dove_gpu_init();
	dove_cesa_init();
	dove_hwmon_init();

	dove_i2s_init(1, &i2s1_data);
	i2c_register_board_info(0, dove_rd_avng_i2c_devs,
				ARRAY_SIZE(dove_rd_avng_i2c_devs));
	spi_register_board_info(dove_rd_avng_spi_flash_info,
				ARRAY_SIZE(dove_rd_avng_spi_flash_info));

	//mvmpp_sys_init();
}

MACHINE_START(DOVE_RD_AVNG, "Marvell MV88F6781-RD Avengers MID Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_rd_avng_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
