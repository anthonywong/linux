/*
 * Board support file for OMAP4430 SDP.
 *
 * Copyright (C) 2009 Texas Instruments
 *
 * Author: Santosh Shilimkar <santosh.shilimkar@ti.com>
 *
 * Based on mach-omap2/board-3430sdp.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/usb/otg.h>
#include <linux/i2c/twl.h>
#include <linux/i2c/cma3000.h>
#include <linux/i2c/bq2415x.h>
#include <linux/regulator/machine.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/input/sfh7741.h>

#include <mach/hardware.h>
#include <mach/omap4-common.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/control.h>
#include <plat/timer-gp.h>
#include <plat/display.h>
#include <linux/delay.h>
#include <plat/usb.h>
#include <plat/omap_device.h>
#include <plat/omap_hwmod.h>
#include <plat/syntm12xx.h>
#include <plat/mmc.h>
#include <plat/omap4-keypad.h>
#include "hsmmc.h"

#define ETH_KS8851_IRQ			34
#define ETH_KS8851_POWER_ON		48
#define ETH_KS8851_QUART		138

#define OMAP4_SFH7741_SENSOR_OUTPUT_GPIO	184
#define OMAP4_SFH7741_ENABLE_GPIO		188
static void omap_prox_activate(int state);
static int omap_prox_read(void);

static struct sfh7741_platform_data omap_sfh7741_data = {
		.irq = OMAP_GPIO_IRQ(OMAP4_SFH7741_SENSOR_OUTPUT_GPIO),
		.prox_enable = 1,
		.activate_func = omap_prox_activate,
		.read_prox = omap_prox_read,
};

static struct platform_device sdp4430_proximity_device = {
	.name		= "sfh7741",
	.id		= 1,
	.dev		= {
		.platform_data = &omap_sfh7741_data,
	},
};

#define OMAP4_CMA3000ACCL_GPIO		186

static int sdp4430_keymap[] = {
	KEY(0, 0, KEY_E),
	KEY(0, 1, KEY_R),
	KEY(0, 2, KEY_T),
	KEY(0, 3, KEY_HOME),
	KEY(0, 4, KEY_F5),
	KEY(0, 5, KEY_UNKNOWN),
	KEY(0, 6, KEY_I),
	KEY(0, 7, KEY_LEFTSHIFT),

	KEY(1, 0, KEY_D),
	KEY(1, 1, KEY_F),
	KEY(1, 2, KEY_G),
	KEY(1, 3, KEY_SEND),
	KEY(1, 4, KEY_F6),
	KEY(1, 5, KEY_UNKNOWN),
	KEY(1, 6, KEY_K),
	KEY(1, 7, KEY_ENTER),

	KEY(2, 0, KEY_X),
	KEY(2, 1, KEY_C),
	KEY(2, 2, KEY_V),
	KEY(2, 3, KEY_END),
	KEY(2, 4, KEY_F7),
	KEY(2, 5, KEY_UNKNOWN),
	KEY(2, 6, KEY_DOT),
	KEY(2, 7, KEY_CAPSLOCK),

	KEY(3, 0, KEY_Z),
	KEY(3, 1, KEY_KPPLUS),
	KEY(3, 2, KEY_B),
	KEY(3, 3, KEY_F1),
	KEY(3, 4, KEY_F8),
	KEY(3, 5, KEY_UNKNOWN),
	KEY(3, 6, KEY_O),
	KEY(3, 7, KEY_SPACE),

	KEY(4, 0, KEY_W),
	KEY(4, 1, KEY_Y),
	KEY(4, 2, KEY_U),
	KEY(4, 3, KEY_F2),
	KEY(4, 4, KEY_VOLUMEUP),
	KEY(4, 5, KEY_UNKNOWN),
	KEY(4, 6, KEY_L),
	KEY(4, 7, KEY_LEFT),

	KEY(5, 0, KEY_S),
	KEY(5, 1, KEY_H),
	KEY(5, 2, KEY_J),
	KEY(5, 3, KEY_F3),
	KEY(5, 4, KEY_F9),
	KEY(5, 5, KEY_VOLUMEDOWN),
	KEY(5, 6, KEY_M),
	KEY(5, 7, KEY_RIGHT),

	KEY(6, 0, KEY_Q),
	KEY(6, 1, KEY_A),
	KEY(6, 2, KEY_N),
	KEY(6, 3, KEY_BACK),
	KEY(6, 4, KEY_BACKSPACE),
	KEY(6, 5, KEY_UNKNOWN),
	KEY(6, 6, KEY_P),
	KEY(6, 7, KEY_UP),

	KEY(7, 0, KEY_PROG1),
	KEY(7, 1, KEY_PROG2),
	KEY(7, 2, KEY_PROG3),
	KEY(7, 3, KEY_PROG4),
	KEY(7, 4, KEY_F4),
	KEY(7, 5, KEY_UNKNOWN),
	KEY(7, 6, KEY_OK),
	KEY(7, 7, KEY_DOWN),
};

static struct matrix_keymap_data sdp4430_keymap_data = {
	.keymap			= sdp4430_keymap,
	.keymap_size		= ARRAY_SIZE(sdp4430_keymap),
};

static struct omap4_keypad_platform_data sdp4430_keypad_data = {
	.keymap_data		= &sdp4430_keymap_data,
	.rows			= 8,
	.cols			= 8,
	.device_enable		= omap_device_enable,
	.device_shutdown	= omap_device_shutdown,
	.device_idle		= omap_device_idle,
};

static struct platform_device sdp4430_keypad_device = {
	.name		= "omap4-keypad",
	.id		= -1,
	.dev		= {
		.platform_data = &sdp4430_keypad_data,
	},
};

struct omap_device_pm_latency omap_keyboard_latency[] = {
	{
		.deactivate_func = omap_device_idle_hwmods,
		.activate_func   = omap_device_enable_hwmods,
		.flags = OMAP_DEVICE_LATENCY_AUTO_ADJUST,
	},
};

static int __init sdp4430_keypad_init(void)
{
	struct omap_hwmod *oh;
	struct omap_device *od;
	struct omap4_keypad_platform_data *pdata;

	unsigned int length = 0, id = 0;
	int hw_mod_name_len = 16;
	char oh_name[hw_mod_name_len];
	char *name = "omap4-keypad";

	length = snprintf(oh_name, hw_mod_name_len, "kbd");

	oh = omap_hwmod_lookup(oh_name);
	if (!oh) {
		pr_err("Could not look up %s\n", oh_name);
		return -EIO;
	}

	pdata = kzalloc(sizeof(struct omap4_keypad_platform_data), GFP_KERNEL);
	if (!pdata) {
		WARN(1, "Keyboard pdata memory allocation failed\n");
		return -ENOMEM;
	}

	pdata = &sdp4430_keypad_data;

	pdata->base = oh->_rt_va;
	pdata->irq = oh->mpu_irqs[0].irq;
	pdata->device_enable = omap_device_enable;
	pdata->device_idle = omap_device_idle;
	pdata->device_shutdown = omap_device_shutdown;

	od = omap_device_build(name, id, oh, pdata,
			sizeof(struct matrix_keypad_platform_data),
			omap_keyboard_latency,
			ARRAY_SIZE(omap_keyboard_latency), 1);
	WARN(IS_ERR(od), "Could not build omap_device for %s %s\n",
		name, oh_name);

	return 0;
}

static struct spi_board_info sdp4430_spi_board_info[] __initdata = {
	{
		.modalias               = "ks8851",
		.bus_num                = 1,
		.chip_select            = 0,
		.max_speed_hz           = 24000000,
		.irq                    = ETH_KS8851_IRQ,
	},
	{
		.modalias		= "spitst",
		.bus_num		= 4,
		.chip_select		= 0,
		.max_speed_hz		= 15000000,
	},
};

static int omap_ethernet_init(void)
{
	int status;

	/* Request of GPIO lines */

	status = gpio_request(ETH_KS8851_POWER_ON, "eth_power");
	if (status) {
		pr_err("Cannot request GPIO %d\n", ETH_KS8851_POWER_ON);
		return status;
	}

	status = gpio_request(ETH_KS8851_QUART, "quart");
	if (status) {
		pr_err("Cannot request GPIO %d\n", ETH_KS8851_QUART);
		goto error1;
	}

	status = gpio_request(ETH_KS8851_IRQ, "eth_irq");
	if (status) {
		pr_err("Cannot request GPIO %d\n", ETH_KS8851_IRQ);
		goto error2;
	}

	/* Configuration of requested GPIO lines */

	status = gpio_direction_output(ETH_KS8851_POWER_ON, 1);
	if (status) {
		pr_err("Cannot set output GPIO %d\n", ETH_KS8851_IRQ);
		goto error3;
	}

	status = gpio_direction_output(ETH_KS8851_QUART, 1);
	if (status) {
		pr_err("Cannot set output GPIO %d\n", ETH_KS8851_QUART);
		goto error3;
	}

	status = gpio_direction_input(ETH_KS8851_IRQ);
	if (status) {
		pr_err("Cannot set input GPIO %d\n", ETH_KS8851_IRQ);
		goto error3;
	}

	return 0;

error3:
	gpio_free(ETH_KS8851_IRQ);
error2:
	gpio_free(ETH_KS8851_QUART);
error1:
	gpio_free(ETH_KS8851_POWER_ON);
	return status;
}

static char *tm12xx_idev_names[] = {
	"Synaptic TM12XX TouchPoint 1",
	"Synaptic TM12XX TouchPoint 2",
	"Synaptic TM12XX TouchPoint 3",
	"Synaptic TM12XX TouchPoint 4",
	"Synaptic TM12XX TouchPoint 5",
	"Synaptic TM12XX TouchPoint 6",
	NULL,
};

static u8 tm12xx_button_map[] = {
	KEY_F1,
	KEY_F2,
};

static struct tm12xx_ts_platform_data tm12xx_platform_data[] = {
	{ /* Primary Controller */
		.gpio_intr = 35,
		.idev_name = tm12xx_idev_names,
		.button_map = tm12xx_button_map,
		.num_buttons = ARRAY_SIZE(tm12xx_button_map),
		.repeat = 0,
		.swap_xy = 1,
	},
	{ /* Secondary Controller */
		.gpio_intr = 36,
		.idev_name = tm12xx_idev_names,
		.button_map = tm12xx_button_map,
		.num_buttons = ARRAY_SIZE(tm12xx_button_map),
		.repeat = 0,
		.swap_xy = 1,
	},
};
/* Begin Synaptic Touchscreen TM-01217 */
static int sdp4430_taal_enable(struct omap_dss_device *dssdev)
{
	if (dssdev->channel == OMAP_DSS_CHANNEL_LCD) {
		gpio_request(102, "dsi1_en_gpio");	/* DSI1_GPIO_102*/
		gpio_direction_output(102, 0);
		mdelay(500);
		gpio_set_value(102, 1);
		mdelay(500);
		gpio_set_value(102, 0);
		mdelay(500);
		gpio_set_value(102, 1);

		twl_i2c_write_u8(TWL_MODULE_PWM, 0xFF, 0x03);
		twl_i2c_write_u8(TWL_MODULE_PWM, 0x7F, 0x04);
		twl_i2c_write_u8(TWL6030_MODULE_ID1, 0x30, 0x92);

		gpio_request(27, "dsi1_bl_gpio");	/*DSI1_GPIO_27*/
		gpio_direction_output(27, 1);
		mdelay(120);
		gpio_set_value(27, 0);
		mdelay(120);
		gpio_set_value(27, 1);
	} else {
		gpio_request(104, "dsi2_en_gpio");	/* DSI2_GPIO_104 */
		gpio_direction_output(104, 0);
		mdelay(500);
		gpio_set_value(104, 1);
		mdelay(500);
		gpio_set_value(104, 0);
		mdelay(500);
		gpio_set_value(104, 1);

		twl_i2c_write_u8(TWL_MODULE_PWM, 0xFF, 0x03);
		twl_i2c_write_u8(TWL_MODULE_PWM, 0x7F, 0x04);
		twl_i2c_write_u8(TWL6030_MODULE_ID1, 0x30, 0x92);

		gpio_request(59, "dsi2_bl_gpio");	/* DSI2_GPIO_59 */
		gpio_direction_output(59, 1);
		mdelay(120);
		gpio_set_value(59, 0);
		mdelay(120);
		gpio_set_value(59, 1);
	}
	return 0;
}

static void sdp4430_taal_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->channel == OMAP_DSS_CHANNEL_LCD) {
		gpio_set_value(102, 1);	/*DSI1_GPIO_102*/
		gpio_set_value(27, 0);	/*DSI1_GPIO_27*/
	} else {
		gpio_set_value(104, 1);	/* DSI2_GPIO_104 */
		gpio_set_value(59, 0);	/* DSI2_GPIO_59 */
	}
}

static struct omap_dss_device sdp4430_lcd_device = {
	.name			= "lcd",
	.driver_name		= "taal",
	.type			= OMAP_DISPLAY_TYPE_DSI,
	.reset_gpio		= 102,
	.phy.dsi		= {
		.clk_lane	= 1,
		.clk_pol	= 0,
		.data1_lane	= 2,
		.data1_pol	= 0,
		.data2_lane	= 3,
		.data2_pol	= 0,
		.ext_te		= true,
		.ext_te_gpio	= 101,
		.div		= {
			.lck_div	= 1,
			.pck_div	= 6,
			.regm		= 200,
			.regn		= 19,
			.regm3		= 4,
			.regm4		= 5,
			.lp_clk_div	= 6,
		},
	},
	.platform_enable	=	sdp4430_taal_enable,
	.platform_disable	=	sdp4430_taal_disable,
	.channel			=	OMAP_DSS_CHANNEL_LCD,
};

static struct omap_dss_device sdp4430_lcd2_device = {
	.name			= "2lcd",
	.driver_name		= "taal2",
	.type			= OMAP_DISPLAY_TYPE_DSI,
	.reset_gpio		= 102,
	.phy.dsi		= {
		.clk_lane	= 1,
		.clk_pol	= 0,
		.data1_lane	= 2,
		.data1_pol	= 0,
		.data2_lane	= 3,
		.data2_pol	= 0,
		.ext_te		= true,
		.ext_te_gpio	= 103,
		.div		= {
			.lck_div	= 1,
			.pck_div	= 6,
			.regm		= 200,
			.regn		= 19,
			.regm3		= 4,
			.regm4		= 5,
			.lp_clk_div	= 6,
		},
	},
	.platform_enable	=	sdp4430_taal_enable,
	.platform_disable	=	sdp4430_taal_disable,
	.channel			=	OMAP_DSS_CHANNEL_LCD2,
 };

static int sdp4430_panel_enable_hdmi(struct omap_dss_device *dssdev)
{
	gpio_request(HDMI_GPIO_60 , "hdmi_gpio_60");
	gpio_request(HDMI_GPIO_41 , "hdmi_gpio_41");
	gpio_direction_output(HDMI_GPIO_60, 0);
	gpio_direction_output(HDMI_GPIO_41, 0);
	gpio_set_value(HDMI_GPIO_60, 1);
	gpio_set_value(HDMI_GPIO_41, 1);
	gpio_set_value(HDMI_GPIO_60, 0);

	return 0;
}

static void sdp4430_panel_disable_hdmi(struct omap_dss_device *dssdev)
{
	gpio_set_value(HDMI_GPIO_60, 1);
	gpio_set_value(HDMI_GPIO_41, 1);

}

static __attribute__ ((unused)) void __init sdp4430_hdmi_init(void)
{
	return;
}

static struct omap_dss_device sdp4430_hdmi_device = {
	.name = "hdmi",
	.driver_name = "hdmi_panel",
	.type = OMAP_DISPLAY_TYPE_HDMI,
	.phy.dpi.data_lines = 24,
	.platform_enable = sdp4430_panel_enable_hdmi,
	.platform_disable = sdp4430_panel_disable_hdmi,
};

static int sdp4430_panel_enable_pico_DLP(struct omap_dss_device *dssdev)
{
	int i = 0;
	gpio_request(DLP_4430_GPIO_59, "DLP DISPLAY SEL");
	gpio_direction_output(DLP_4430_GPIO_59, 0);
	gpio_request(DLP_4430_GPIO_45, "DLP PARK");
	gpio_direction_output(DLP_4430_GPIO_45, 0);
	gpio_request(DLP_4430_GPIO_40, "DLP PHY RESET");
	gpio_direction_output(DLP_4430_GPIO_40, 0);
	gpio_request(DLP_4430_GPIO_44, "DLP READY RESET");
	gpio_direction_input(DLP_4430_GPIO_44);
	mdelay(500);

	gpio_set_value(DLP_4430_GPIO_59, 1);
	gpio_set_value(DLP_4430_GPIO_45, 1);
	mdelay(1000);

	gpio_set_value(DLP_4430_GPIO_40, 1);
	mdelay(1000);

	/*FIXME with the MLO gpio changes , gpio read is not retuning correct value even though 		it is  set in hardware so the check is comment till the problem is fixed */
	/*while(i == 0){
	i=gpio_get_value(DLP_4430_GPIO_44);
	printk("wait for ready bit %d\n",i);
	}*/
	printk("%d ready bit ", i);
	mdelay(2000);
	return 0;
}

static void sdp4430_panel_disable_pico_DLP(struct omap_dss_device *dssdev)
{
	gpio_set_value(DLP_4430_GPIO_40, 0);
	gpio_set_value(DLP_4430_GPIO_45, 0);

}

static struct omap_dss_device sdp4430_picoDLP_device = {
	.name			            = "pico_DLP",
	.driver_name		        = "picoDLP_panel",
	.type			            = OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines	        = 24,
	.platform_enable	        = sdp4430_panel_enable_pico_DLP,
	.platform_disable	        = sdp4430_panel_disable_pico_DLP,
	.channel			= OMAP_DSS_CHANNEL_LCD2,
};

/* wl128x BT, FM, GPS connectivity chip */
static int gpios[] = {55, -1, -1};
static struct platform_device wl128x_device = {
	.name           = "kim",
	.id             = -1,
	.dev.platform_data = &gpios,
};

static struct omap_dss_device *sdp4430_dss_devices[] = {
        &sdp4430_lcd_device,
        &sdp4430_lcd2_device,
#ifdef CONFIG_OMAP2_DSS_HDMI
	&sdp4430_hdmi_device,
#endif
#ifdef CONFIG_PANEL_PICO_DLP
	&sdp4430_picoDLP_device,
#endif
};

static struct omap_dss_board_info sdp4430_dss_data = {
	.num_devices	=	ARRAY_SIZE(sdp4430_dss_devices),
	.devices	=	sdp4430_dss_devices,
	.default_device	=	&sdp4430_lcd_device,
};

static struct platform_device sdp4430_dss_device = {
	.name	=	"omapdss",
	.id	=	-1,
	.dev	= {
		.platform_data = &sdp4430_dss_data,
	},
};

static struct platform_device *sdp4430_devices[] __initdata = {
        &sdp4430_dss_device,
        &sdp4430_keypad_device,
        &sdp4430_proximity_device,
        &wl128x_device,
};

static struct omap_lcd_config sdp4430_lcd_config __initdata = {
	.ctrl_name	= "internal",
};

static struct omap_board_config_kernel sdp4430_config[] __initdata = {
	{ OMAP_TAG_LCD,		&sdp4430_lcd_config },
};

static void __init omap_4430sdp_init_irq(void)
{
	omap_board_config = sdp4430_config;
	omap_board_config_size = ARRAY_SIZE(sdp4430_config);
	omap2_init_common_hw(NULL, NULL);
#ifdef CONFIG_OMAP_32K_TIMER
	omap2_gp_clockevent_set_gptimer(1);
#endif
	gic_init_irq();
}

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_UTMI,
#ifdef CONFIG_USB_MUSB_OTG
	.mode			= MUSB_OTG,
#elif defined(CONFIG_USB_MUSB_HDRC_HCD)
	.mode			= MUSB_HOST,
#elif defined(CONFIG_USB_GADGET_MUSB_HDRC)
	.mode			= MUSB_PERIPHERAL,
#endif
	.power			= 100,
};

static struct omap2_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.wires		= 8,
		.cd_type	= NON_GPIO,
		.gpio_wp	= -EINVAL,
		.power_saving   = true,
	},
	{
		.mmc		= 2,
		.wires		= 8,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.nonremovable   = true,
		.power_saving   = true,
	},
#if 1
	{
		.mmc            = 3,
		.wires          = -EINVAL,
		.gpio_cd        = -EINVAL,
		.gpio_wp        = -EINVAL,
	},
	{
		.mmc            = 4,
		.wires          = -EINVAL,
		.gpio_cd        = -EINVAL,
		.gpio_wp        = -EINVAL,
	},
#endif
	{
		.mmc            = 5,
		.wires          = 8,
		.gpio_cd        = -EINVAL,
		.gpio_wp        = 4,
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply sdp4430_vmmc_supply[] = {
	{
		.supply = "vmmc",
		.dev_name = "mmci-omap-hs.0",
	},
	{
		.supply = "vmmc",
		.dev_name = "mmci-omap-hs.5",
	},
};

static struct regulator_consumer_supply sdp4430_vaux_supply[] = {
	{
		.supply = "vmmc",
		.dev_name = "mmci-omap-hs.1",
	},
};

static int omap4_twl6030_hsmmc_late_init(struct device *dev)
{
	int ret = 0;
	struct platform_device *pdev = container_of(dev,
				struct platform_device, dev);
	struct omap_mmc_platform_data *pdata = dev->platform_data;

	/* MMC1 Card detect Configuration */
	if (pdev->id == 0) {
		ret = omap4_hsmmc1_card_detect_config();
		if (ret < 0)
			pr_err("Unable to configure Card detect for MMC1\n");
		pdata->slots[0].card_detect_irq = TWL6030_IRQ_BASE +
						MMCDETECT_INTR_OFFSET;
	}
	return ret;
}

static __init void omap4_twl6030_hsmmc_set_late_init(struct device *dev)
{
	struct omap_mmc_platform_data *pdata;

	/* dev can be null if CONFIG_MMC_OMAP_HS is not set */
	if (!dev)
		return;

	pdata = dev->platform_data;
	pdata->init = omap4_twl6030_hsmmc_late_init;
}

static int __init omap4_twl6030_hsmmc_init(struct omap2_hsmmc_info *controllers)
{
	struct omap2_hsmmc_info *c;

	omap2_hsmmc_init(controllers);
	for (c = controllers; c->mmc; c++)
		omap4_twl6030_hsmmc_set_late_init(c->dev);

#ifdef CONFIG_TIWLAN_SDIO
	/* The controller that is connected to the 128x device
	should have the card detect gpio disabled. This is
	achieved by initializing it with a negative value */
	c[CONFIG_TIWLAN_MMC_CONTROLLER - 1].gpio_cd = -EINVAL;
#endif

	return 0;
}

static struct regulator_init_data sdp4430_vaux1 = {
	.constraints = {
		.min_uV			= 1000000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = sdp4430_vaux_supply,
};

static struct regulator_init_data sdp4430_vaux2 = {
	.constraints = {
		.min_uV			= 1200000,
		.max_uV			= 2800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_init_data sdp4430_vaux3 = {
	.constraints = {
		.min_uV			= 1000000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

/* VMMC1 for MMC1 card */
static struct regulator_init_data sdp4430_vmmc = {
	.constraints = {
		.min_uV			= 1200000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 2,
	.consumer_supplies      = sdp4430_vmmc_supply,
};

static struct regulator_init_data sdp4430_vpp = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 2500000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_init_data sdp4430_vusim = {
	.constraints = {
		.min_uV			= 1200000,
		.max_uV			= 2900000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_init_data sdp4430_vana = {
	.constraints = {
		.min_uV			= 2100000,
		.max_uV			= 2100000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_init_data sdp4430_vcxio = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_init_data sdp4430_vdac = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_init_data sdp4430_vusb = {
	.constraints = {
		.min_uV			= 3300000,
		.max_uV			= 3300000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask	 =	REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
};

static struct twl4030_madc_platform_data sdp4430_gpadc_data = {
	.irq_line	= 1,
};

static struct twl4030_bci_platform_data sdp4430_bci_data = {
	.monitoring_interval		= 10,
	.max_charger_currentmA		= 1500,
	.max_charger_voltagemV		= 4560,
	.max_bat_voltagemV		= 4200,
	.low_bat_voltagemV		= 3300,
};
static struct twl4030_codec_data twl6040_codec = {
#ifdef CONFIG_OMAP4_AUDIO_PWRON
	.audpwron_gpio  = 127,
#else
	/* provide GPIO number above the valid value
	 * to mean there is no GPIO connected. */
	.audpwron_gpio  = 1024,
	.naudint_irq    = OMAP44XX_IRQ_SYS_2N,
#endif
};

static struct twl4030_platform_data sdp4430_twldata = {
	.irq_base	= TWL6030_IRQ_BASE,
	.irq_end	= TWL6030_IRQ_END,

	/* Regulators */
	.vmmc		= &sdp4430_vmmc,
	.vpp		= &sdp4430_vpp,
	.vusim		= &sdp4430_vusim,
	.vana		= &sdp4430_vana,
	.vcxio		= &sdp4430_vcxio,
	.vdac		= &sdp4430_vdac,
	.vusb		= &sdp4430_vusb,
	.vaux1		= &sdp4430_vaux1,
	.vaux2		= &sdp4430_vaux2,
	.vaux3		= &sdp4430_vaux3,

	/* children */
	.codec		= &twl6040_codec,
	.madc           = &sdp4430_gpadc_data,
	.bci            = &sdp4430_bci_data,

	/* children */
	.codec		= &twl6040_codec,
};

static struct bq2415x_platform_data sdp4430_bqdata = {
	.max_charger_voltagemA = 4200,
	.max_charger_currentmA = 1550,
};

static struct cma3000_platform_data cma3000_platform_data = {
	.fuzz_x = 25,
	.fuzz_y = 25,
	.fuzz_z = 25,
	.g_range = CMARANGE_8G,
	.mode = CMAMODE_MOTDET,
	.mdthr = 0x8,
	.mdfftmr = 0x33,
	.ffthr = 0x8,
	.irqflags = IRQF_TRIGGER_HIGH,
};

static struct pico_platform_data picodlp_platform_data[] = {
	[0] = { /* DLP Controller */
		.gpio_intr = 40,
	},
};

static struct i2c_board_info __initdata sdp4430_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("twl6030", 0x48),
		.flags = I2C_CLIENT_WAKE,
		.irq = OMAP44XX_IRQ_SYS_1N,
		.platform_data = &sdp4430_twldata,
	},
	{
		I2C_BOARD_INFO("bq24156", 0x6a),
		.platform_data = &sdp4430_bqdata,
	},
};

static struct i2c_board_info __initdata sdp4430_i2c_2_boardinfo[] = {
	{
		I2C_BOARD_INFO("tm12xx_ts_primary", 0x4b),
		.platform_data = &tm12xx_platform_data[0],
	},
    {
		I2C_BOARD_INFO("picoDLP_i2c_driver", 0x1b),
		.platform_data = &picodlp_platform_data[0],
	},
};

static struct i2c_board_info __initdata sdp4430_i2c_3_boardinfo[] = {
	{
		I2C_BOARD_INFO("tm12xx_ts_secondary", 0x4b),
		.platform_data = &tm12xx_platform_data[1],
	},
	{
		I2C_BOARD_INFO("bh1780", 0x29),
	},
	{
		I2C_BOARD_INFO("tmp105", 0x48),
	},
};

static struct i2c_board_info __initdata sdp4430_i2c_4_boardinfo[] = {
	{
		I2C_BOARD_INFO("cma3000_accl", 0x1c),
		.platform_data = &cma3000_platform_data,
		.irq = OMAP_GPIO_IRQ(OMAP4_CMA3000ACCL_GPIO),
	},
	{
		I2C_BOARD_INFO("bmp085", 0x77),
	},
};

static int __init omap4_i2c_init(void)
{
	/* Phoenix Audio IC needs I2C1 to start with 400 KHz and less */
	omap_register_i2c_bus(1, 400, sdp4430_i2c_boardinfo,
				ARRAY_SIZE(sdp4430_i2c_boardinfo));
	omap_register_i2c_bus(2, 400, sdp4430_i2c_2_boardinfo,
				ARRAY_SIZE(sdp4430_i2c_2_boardinfo));
	omap_register_i2c_bus(3, 400, sdp4430_i2c_3_boardinfo,
				ARRAY_SIZE(sdp4430_i2c_3_boardinfo));
	omap_register_i2c_bus(4, 400, sdp4430_i2c_4_boardinfo,
				ARRAY_SIZE(sdp4430_i2c_4_boardinfo));
	return 0;
}

static void omap_prox_activate(int state)
{
	gpio_set_value(OMAP4_SFH7741_ENABLE_GPIO , state);
}

static int omap_prox_read(void)
{
	int proximity;
	proximity = gpio_get_value(OMAP4_SFH7741_SENSOR_OUTPUT_GPIO);
	return proximity;
}

static void omap_sfh7741prox_init(void)
{
	int  error;

	error = gpio_request(OMAP4_SFH7741_SENSOR_OUTPUT_GPIO, "sfh7741");
	if (error < 0) {
		pr_err("%s: GPIO configuration failed: GPIO %d, error %d\n"
			, __func__, OMAP4_SFH7741_SENSOR_OUTPUT_GPIO, error);
		return ;
	}

	error = gpio_direction_input(OMAP4_SFH7741_SENSOR_OUTPUT_GPIO);
	if (error < 0) {
		pr_err("Proximity GPIO input configuration failed\n");
		goto fail1;
	}

	error = gpio_request(OMAP4_SFH7741_ENABLE_GPIO, "sfh7741");
	if (error < 0) {
		pr_err("failed to request GPIO %d, error %d\n",
			OMAP4_SFH7741_ENABLE_GPIO, error);
		goto fail1;
	}

	error = gpio_direction_output(OMAP4_SFH7741_ENABLE_GPIO , 1);
	if (error < 0) {
		pr_err("%s: GPIO configuration failed: GPIO %d,\
			error %d\n",__func__, OMAP4_SFH7741_ENABLE_GPIO, error);
		goto fail3;
	}
	return;

fail3:
	gpio_free(OMAP4_SFH7741_ENABLE_GPIO);
fail1:
	gpio_free(OMAP4_SFH7741_SENSOR_OUTPUT_GPIO);
}

static void __init omap4_display_init(void)
{
	void __iomem *phymux_base = NULL;
	unsigned int dsimux = 0xFFFFFFFF;
	phymux_base = ioremap(0x4A100000, 0x1000);
	/* Turning on DSI PHY Mux*/
	__raw_writel(dsimux, phymux_base+0x618);
	dsimux = __raw_readl(phymux_base+0x618);
}

#ifdef CONFIG_TIWLAN_SDIO
static void pad_config(unsigned long pad_addr, u32 andmask, u32 ormask)
{
        int val;
        u32 *addr;

        addr = (u32 *) ioremap(pad_addr, 4);
        if (!addr) {
                printk(KERN_ERR"OMAP_pad_config: ioremap failed with addr %lx\n",
                        pad_addr);
        return;
        }

        val =  __raw_readl(addr);
        val &= andmask;
        val |= ormask;
        __raw_writel(val, addr);

        iounmap(addr);
}

void wlan_1283_config()
{
        pad_config(0x4A100078, 0xFFECFFFF, 0x00030000);
        pad_config(0x4A10007C, 0xFFFFFFEF, 0x0000000B);
        if (gpio_request(54, NULL) != 0)
                printk(KERN_ERR "GPIO 54 request failed\n");
        gpio_direction_output(54, 0);
        return ;
}
#endif

static void omap_cma3000accl_init(void)
{
	if (gpio_request(OMAP4_CMA3000ACCL_GPIO, "Accelerometer") < 0) {
		pr_err("Accelerometer GPIO request failed\n");
		return;
	}
	gpio_direction_input(OMAP4_CMA3000ACCL_GPIO);
}

static void __init omap_4430sdp_init(void)
{
	int status;

	omap4_i2c_init();
	omap4_display_init();
	platform_add_devices(sdp4430_devices, ARRAY_SIZE(sdp4430_devices));
	omap_serial_init();
	omap4_twl6030_hsmmc_init(mmc);
#ifdef CONFIG_TIWLAN_SDIO
        wlan_1283_config();
#endif
	/* OMAP4 SDP uses internal transceiver so register nop transceiver */
	usb_nop_xceiv_register();
	usb_musb_init(&musb_board_data);

	status = sdp4430_keypad_init();
	if (status)
		pr_err("Keypad initialization failed: %d\n", status);

	status = omap_ethernet_init();
	if (status) {
		pr_err("Ethernet initialization failed: %d\n", status);
	} else {
		sdp4430_spi_board_info[0].irq = gpio_to_irq(ETH_KS8851_IRQ);
		spi_register_board_info(sdp4430_spi_board_info,
				ARRAY_SIZE(sdp4430_spi_board_info));
	}
	omap_sfh7741prox_init();
	omap_cma3000accl_init();
}

static void __init omap_4430sdp_map_io(void)
{
	omap2_set_globals_443x();
	omap44xx_map_common_io();
}

MACHINE_START(OMAP_4430SDP, "OMAP4430 4430SDP board")
	/* Maintainer: Santosh Shilimkar - Texas Instruments Inc */
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_4430sdp_map_io,
	.init_irq	= omap_4430sdp_init_irq,
	.init_machine	= omap_4430sdp_init,
	.timer		= &omap_timer,
MACHINE_END
