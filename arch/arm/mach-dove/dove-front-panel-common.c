/*
 * arch/arm/mach-dove/dove-front-panel-common.c
 *
 * Marvell Dove MV88F6781-RD/DB Board Setup for device on front panel
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio_mouse.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <linux/gpio.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>
#include <plat/cafe-orion.h>
#include "dove-front-panel-common.h"

/*
 * LCD input clock.
 */
#define LCD_SCLK	(CONFIG_FB_DOVE_CLCD_SCLK_VALUE*1000*1000)

static struct dovefb_mach_info dove_lcd0_dmi = {
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
	.io_pin_allocation	= IOPAD_DUMB18SPI,
	.panel_rgb_type		= DUMB18_RGB666_0,
#endif
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

static struct dovefb_mach_info dove_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
	.sclk_clock		= LCD_SCLK,
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB18SPI,
	.panel_rgb_type		= DUMB18_RGB666_0,
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

static struct dovefb_mach_info dove_lcd1_dmi = {
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

static struct dovefb_mach_info dove_lcd1_vid_dmi = {
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

static struct dovebl_platform_data fp_backlight_data = {
	.default_intensity = 1,
	.max_brightness = 0xE,
};

struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 1, //CTL0 connected to the sensor power down
	.reset		= 2, //CTL1 connected to the sensor reset
};

struct gpio_mouse_platform_data tact_dove_fp_data = {
	.scan_ms = 10,
	.polarity = 1,
	{
		{
			.up = 15,
			.down = 12,
			.left = 13,
			.right = 14,
			.bleft = 18,
			.bright = -1,
			.bmiddle = -1
		}
	}
};

#define DOVE_FP_TS_PEN_GPIO	(53)
#define DOVE_FP_TS_PEN_IRQ	(DOVE_FP_TS_PEN_GPIO + 64)

static int dove_fp_get_pendown_state(void)
{
	return !gpio_get_value(irq_to_gpio(DOVE_FP_TS_PEN_IRQ));
}

static const struct ads7846_platform_data dove_fp_ts_data = {
	.model			= 7846,
	.x_min			= 80,
	.x_max			= 3940,
	.y_min			= 330,
	.y_max			= 3849,
	.get_pendown_state	= dove_fp_get_pendown_state,
};

struct spi_board_info __initdata dove_fp_spi_devs[] = {
	{
		.modalias	= "ads7846",
		.platform_data	= &dove_fp_ts_data,
		.irq		= DOVE_FP_TS_PEN_IRQ,
		.max_speed_hz	= 125000 * 4,
		.bus_num	= 2,
		.chip_select	= 0,
	},
};

int __init dove_fp_spi_devs_num(void)
{
	return ARRAY_SIZE(dove_fp_spi_devs);
}

int __init dove_fp_ts_gpio_setup(void)
{
	set_irq_chip(DOVE_FP_TS_PEN_IRQ, &orion_gpio_irq_chip);
	set_irq_handler(DOVE_FP_TS_PEN_IRQ, handle_edge_irq);
	set_irq_type(DOVE_FP_TS_PEN_IRQ, IRQ_TYPE_EDGE_FALLING);
	orion_gpio_set_valid(DOVE_FP_TS_PEN_GPIO, 1);
	if (gpio_request(DOVE_FP_TS_PEN_GPIO, "DOVE_TS_PEN_IRQ") != 0)
		pr_err("Dove: failed to setup TS IRQ GPIO\n");
	if (gpio_direction_input(DOVE_FP_TS_PEN_GPIO) != 0) {
		printk(KERN_ERR "%s failed "
		       "to set output pin %d\n", __func__,
		       DOVE_FP_TS_PEN_GPIO);
		gpio_free(DOVE_FP_TS_PEN_GPIO);
		return -1;
	}
	return 0;
}

void __init dove_fp_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_lcd0_dmi, &dove_lcd0_vid_dmi,
			   &dove_lcd1_dmi, &dove_lcd1_vid_dmi,
			   &fp_backlight_data);
#endif /* CONFIG_FB_DOVE */
}
