/* arch/arm/mach-msm/include/mach/board.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_MSM_BOARD_H
#define __ASM_ARCH_MSM_BOARD_H

#include <linux/types.h>
#include <linux/input.h>
#include <linux/clk.h>

/* platform device data structures */

struct msm_mddi_platform_data
{
	void (*panel_power)(int on);
	unsigned has_vsync_irq:1;
};

struct msm_acpu_clock_platform_data
{
	uint32_t acpu_switch_time_us;
	uint32_t max_speed_delta_khz;
	uint32_t vdd_switch_time_us;
	unsigned long power_collapse_khz;
	unsigned long wait_for_irq_khz;
	unsigned int max_axi_khz;
	unsigned int max_vdd;
	int (*acpu_set_vdd) (int mvolts);
};

struct msm_panel_common_pdata {
	int gpio;
	int (*backlight_level)(int level, int max, int min);
	int (*pmic_backlight)(int level);
	int (*panel_num)(void);
	void (*panel_config_gpio)(int);
	int *gpio_num;
};

struct lcdc_platform_data {
	int (*lcdc_gpio_config)(int on);
	void (*lcdc_power_save)(int);
};

struct tvenc_platform_data {
	int (*pm_vid_en)(int on);
};

struct mddi_platform_data {
	void (*mddi_power_save)(int on);
	int (*mddi_sel_clk)(u32 *clk_rate);
	int (*mddi_power_on)(int);
};

struct msm_fb_platform_data {
	int (*detect_client)(const char *name);
	int mddi_prescan;
};

struct msm_i2c_platform_data {
	int clk_freq;
};

/* common init routines for use by arch/arm/mach-msm/board-*.c */

void __init msm_add_devices(void);
void __init msm_map_common_io(void);
void __init msm_map_qsd8x50_io(void);
void __init msm_map_msm7x30_io(void);
void __init msm_map_comet_io(void);
void __init msm_init_irq(void);
void __init msm_clock_init(struct clk *clock_tbl, unsigned num_clocks);
void __init msm_acpu_clock_init(struct msm_acpu_clock_platform_data *);

#if defined(CONFIG_USB_FUNCTION_MSM_HSUSB)
void msm_hsusb_set_vbus_state(int online);
#else
static inline void msm_hsusb_set_vbus_state(int online) {}
#endif

extern int msm_shared_ram_phys; /* defined in arch/arm/mach-msm/io.c */

#endif
