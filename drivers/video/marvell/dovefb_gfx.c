/*
 * linux/drivers/video/dovefb.c -- Marvell DOVE LCD Controller
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by:
 *	Green Wan <gwan@marvell.com>
 *	Shadi Ammouri <shadi@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

/*
 * 1. Adapted from:  linux/drivers/video/skeletonfb.c
 * 2. Merged code base from: linux/drivers/video/dovefb.c (Lennert Buytenhek)
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/console.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/err.h>
//#include <asm/hardware.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/irq.h>

#include "../edid.h"
#include <video/dovefb.h>
#include <video/dovefbreg.h>

#include "dovefb_if.h"
#include <video/ch7025_composite.h>
#include <video/kg2.h>
#define MAX_HWC_SIZE		(64*64*2)
#define DEFAULT_REFRESH		60	/* Hz */

static int dovefb_fill_edid(struct fb_info *fi,
				struct dovefb_mach_info *dmi);
static int wait_for_vsync(struct dovefb_layer_info *dfli);
static void dovefb_set_defaults(struct dovefb_layer_info *dfli);
extern unsigned int lcd_accurate_clock;

#define AXI_BASE_CLK	(2000000000ll)	/* 2000MHz */

static void set_external_lcd_clock(u32 clock_div, u32 is_half_div)
{
	u32	reg;
	u32	old_clock_div, old_half_div;

	/* disable preemption, the gen conf regs might be accessed by other
	** drivers.
	*/
	preempt_disable();

	/*
	 * If current setting is right, just return.
	 */
	reg = readl(DOVE_GLOBAL_CONFIG_1);
	old_clock_div = (reg & (0x3F << 10)) >> 10;
	old_half_div = (reg & (1 << 16)) >> 16;

	if (clock_div == old_clock_div && is_half_div == old_half_div)
		return;

	/* Clear LCD_Clk_Enable (Enable LCD Clock).			*/
	reg &= ~(1 << 17);
	writel(reg, DOVE_GLOBAL_CONFIG_1);

	/* Set LCD_CLK_DIV_SEL in LCD TWSI and CPU Configuration 1	*/
	reg = readl(DOVE_GLOBAL_CONFIG_1);
	reg &= ~(1 << 9);
	writel(reg, DOVE_GLOBAL_CONFIG_1);


	/* Configure division factor (N = LCD_EXT_DIV[5:0], N<32) in 	*/
	/* Config 1 Register.						*/
	reg &= ~(0x3F << 10);
	reg |= (clock_div << 10);

	/* Set LCD_Half_integer_divider = 1 in LCD TWSI and CPU Config 1*/
	if (is_half_div)
		reg |= (1 << 16);
	else
		reg &= ~(1 << 16);

	writel(reg, DOVE_GLOBAL_CONFIG_1);

	/* Set LCD_Ext_Clk_Div_Load in LCD TWSI and CPU Config 2.	*/
	reg = readl(DOVE_GLOBAL_CONFIG_2);
	reg |= (1 << 24);
	writel(reg, DOVE_GLOBAL_CONFIG_2);

	preempt_enable();

	/* Insert S/W delay of at least 200 nsec.			*/
	udelay(1);

	preempt_disable();
	/* Clear LCD_Ext_Clk_Div_Load.					*/
	reg = readl(DOVE_GLOBAL_CONFIG_2);
	reg &= ~(1 << 24);
	writel(reg, DOVE_GLOBAL_CONFIG_2);

	/* Set LCD_Clk_Enable (Enable LCD Clock).			*/
	reg = readl(DOVE_GLOBAL_CONFIG_1);
	reg |= (1 << 17);
	writel(reg, DOVE_GLOBAL_CONFIG_1);
	preempt_enable();

	return;
}

static inline u64 calc_diff(u64 a, u64 b)
{
	if (a > b)
		return a - b;
	else
		return b - a;
}
static void calc_best_clock_div(u32 tar_freq, u32 *axi_div,
		u32 *lcd_div, u32 *is_ext_rem)
{
	u64 req_div;
	u64 best_rem = 0xFFFFFFFFFFFFFFFFll;
	unsigned int best_axi_div = 0;
	unsigned int best_lcd_div = 0;
	u64 tmp_lcd_div;
	int ext_rem = 0;
	u32 i, borders;
	u64 rem;
	u64 temp;
	int override = 0; 	/* Used to mark special cases where the LCD */
	int div_2_skip = 3;	/* divider value is not recommended.	    */
				/* (in our case it's divider 3).	    */

	/* Calculate required dividor */
	req_div = AXI_BASE_CLK;
	do_div(req_div, tar_freq);

	/* Look for the whole division with the smallest remainder */
	for (i = 5; i < 64; i++) {
		temp = (u64)tar_freq * (u64)i;
		borders = req_div;
		do_div(borders, i);
		/* The LCD divsion must be smaller than 64K */
		if (borders < SZ_64K) {
			tmp_lcd_div = AXI_BASE_CLK;
			/* We cannot do 64-bit / 64-bit operations,
			** thus... */
			do_div(tmp_lcd_div, i);
			do_div(tmp_lcd_div, tar_freq);
			rem = calc_diff(AXI_BASE_CLK, (temp * tmp_lcd_div));
			if ((rem < best_rem) ||
			    ((override == 1) && (rem == best_rem))) {
				best_rem = rem;
				best_axi_div = i;
				best_lcd_div = tmp_lcd_div;
				override = ((best_lcd_div == div_2_skip) ?
						1 : 0);
			}
			if ((best_rem == 0) && (override == 0))
				break;
			/* Check the next LCD divider */
			tmp_lcd_div++;
			rem = calc_diff((temp * tmp_lcd_div), AXI_BASE_CLK);
			if ((rem < best_rem) ||
			    ((override == 1) && (rem == best_rem))) {
				best_rem = rem;
				best_axi_div = i;
				best_lcd_div = tmp_lcd_div;
				override = ((best_lcd_div == div_2_skip) ?
						1 : 0);
			}
			if ((best_rem == 0) && (override == 0))
				break;
		}
	}

	/* Look for the extended division with the smallest remainder */
	if (best_rem != 0) {
		req_div = AXI_BASE_CLK * 10;
		do_div(req_div, tar_freq);
		/* Half div can be between 12.5 & 31.5 */
		for (i = 55; i <= 315; i += 10) {
			temp = (u64)tar_freq * (u64)i;
			borders = req_div;
			do_div(borders, i);
			if (borders < SZ_64K) {
				tmp_lcd_div = AXI_BASE_CLK * 10;
				/* We cannot do 64-bit / 64-bit operations,
				** thus... */
				do_div(tmp_lcd_div, i);
				do_div(tmp_lcd_div, tar_freq);

				rem = calc_diff(AXI_BASE_CLK * 10,
						(tmp_lcd_div * temp));
				do_div(rem, 10);
				if ((rem < best_rem) ||
				    ((override == 1) && (rem == best_rem))) {
					ext_rem = 1;
					best_rem = rem;
					best_axi_div = i / 10;
					best_lcd_div = tmp_lcd_div;
					override = ((best_lcd_div == div_2_skip)
							? 1 : 0);
				}
				if ((best_rem == 0) && (override == 0))
					break;
				/* Check next LCD divider */
				tmp_lcd_div++;
				rem = calc_diff((tmp_lcd_div * temp),
						AXI_BASE_CLK * 10);
				do_div(rem, 10);
				if ((rem < best_rem) ||
				    ((override == 1) && (rem == best_rem))) {
					ext_rem = 1;
					best_rem = rem;
					best_axi_div = i / 10;
					best_lcd_div = tmp_lcd_div;
					override = ((best_lcd_div == div_2_skip)
							? 1 : 0);
				}
				if ((best_rem == 0) && (override == 0))
					break;
			}
		}
	}

	*is_ext_rem = ext_rem;
	*lcd_div = best_lcd_div;
	*axi_div = best_axi_div;
	return;
}

/*
 * The hardware clock divider has an integer and a fractional
 * stage:
 *
 *	clk2 = clk_in / integer_divider
 *	clk_out = clk2 * (1 - (fractional_divider >> 12))
 *
 * Calculate integer and fractional divider for given clk_in
 * and clk_out.
 */
static int init_ext_divider = 0;

static void set_clock_divider(struct dovefb_layer_info *dfli,
	const struct fb_videomode *m)
{
	int divider_int;
	int needed_pixclk;
	u64 div_result;
	u32 x = 0, x_bk;
	struct dovefb_info *info = dfli->info;
	u32 axi_div = 1, lcd_div, is_ext = 0;
	struct dovefb_mach_info *dmi = dfli->dev->platform_data;
	u32 isInterlaced = 0;

	/*
	 * Notice: The field pixclock is used by linux fb
	 * is in pixel second. E.g. struct fb_videomode &
	 * struct fb_var_screeninfo
	 */

	/*
	 * Check input values.
	 */
	if (!m || !m->pixclock || !m->refresh) {
		printk(KERN_ERR "Input refresh or pixclock is wrong.\n");
		return;
	}

	isInterlaced = (m->vmode & FB_VMODE_INTERLACED) ? 1 : 0;

	/*
	 * Using PLL/AXI clock.
	 */
#ifdef CONFIG_FB_DOVE_CLCD_USE_PLL_CLK
	x = 0x80000000;
#endif

	/*
	 * Calc divider according to refresh rate.
	 */
	div_result = 1000000000000ll;

	if(info->fixed_output)
		m = &info->out_vmode;

	do_div(div_result, m->pixclock);
	needed_pixclk = (u32)(isInterlaced ? (div_result/2) : div_result);

	if (lcd_accurate_clock) {
		calc_best_clock_div(needed_pixclk, &axi_div, &lcd_div, &is_ext);
		//printk(KERN_INFO "pix_clock = %d, axi_div = %d, lcd_div = %d, is_ext = %d.\n",
		//		needed_pixclk, axi_div, lcd_div, is_ext);
		divider_int = lcd_div;
	} else {
		divider_int = (dmi->sclk_clock + (needed_pixclk / 2)) / needed_pixclk;
		
		/* check whether divisor is too small. */
		if (divider_int < 2) {
			printk(KERN_WARNING "Warning: clock source is too slow."
					 "Try smaller resolution\n");
			divider_int = 2;
		}
	}

	if (lcd_accurate_clock) {
		set_external_lcd_clock(axi_div, is_ext);
	} else {
		if (0 == init_ext_divider) {
			init_ext_divider = 1;
			printk(KERN_ERR "fix to (2G/%d) without half divider.\n", (2000000000/dmi->sclk_clock));
			set_external_lcd_clock((2000000000/dmi->sclk_clock), 0);
		}
	}

	/*
	 * Set setting to reg.
	 */
	x_bk = readl(dfli->reg_base + LCD_CFG_SCLK_DIV);
	x |= divider_int;

	if (x != x_bk)
		writel(x, dfli->reg_base + LCD_CFG_SCLK_DIV);
}

static void set_dma_control0(struct dovefb_layer_info *dfli)
{
	u32 x, x_bk;
	struct fb_var_screeninfo *var = &dfli->fb_info->var;

	/*
	 * Set bit to enable graphics DMA.
	 */
	x_bk = x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0);
	//x |= (dfli->active && dfli->enabled) ? CFG_GRA_ENA_MASK : 0;
	x |= CFG_GRA_ENA_MASK;
	dfli->active = 0;

	/*
	 * If we are in a pseudo-color mode, we need to enable
	 * palette lookup.
	 */
	if (dfli->pix_fmt == PIX_FMT_PSEUDOCOLOR)
		x |= 0x10000000;
	else
		x &= ~0x10000000;

	/*
	 * enable horizontal smooth scaling.
	 */
	x |= 0x1 << 14;

	/*
	 * Cursor enabled?
	 */
	if (dfli->cursor_enabled)
		x |= 0x01000000;

	/*
	 * Configure hardware pixel format.
	 */
	x &= ~(0xF << 16);
	x |= (dfli->pix_fmt >> 1) << 16;

	/*
	 * Check red and blue pixel swap.
	 * 1. source data swap
	 * 2. panel output data swap
	 */
	x &= ~(1 << 12);
	x |= ((dfli->pix_fmt & 1) ^ (dfli->info->panel_rbswap)) << 12;

	/*
	 * Enable toogle to generate interlace mode.
	 */
	if (FB_VMODE_INTERLACED & var->vmode) {
		x |= CFG_GRA_FTOGGLE_MASK;
		dfli->reserved |= 0x1;
	} else {
		x &= ~CFG_GRA_FTOGGLE_MASK;
		dfli->reserved &= ~0x1;
	}

	if (x != x_bk)
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
}

static void set_dma_control1(struct dovefb_layer_info *dfli, int sync)
{
	u32 x, x_bk;

	/*
	 * Configure default bits: vsync triggers DMA, gated clock
	 * enable, power save enable, configure alpha registers to
	 * display 100% graphics, and set pixel command.
	 */
	x_bk = x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL1);

	/*
	 * We trigger DMA on the falling edge of vsync if vsync is
	 * active low, or on the rising edge if vsync is active high.
	 */
	if (!(sync & FB_SYNC_VERT_HIGH_ACT))
		x |= 0x08000000;

	if (x != x_bk)
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL1);
}

static int wait_for_vsync(struct dovefb_layer_info *dfli)
{
	if (dfli) {
		u32 irq_ena = readl(dfli->reg_base + SPU_IRQ_ENA);
		int rc = 0;

		writel(irq_ena | DOVEFB_GFX_INT_MASK | DOVEFB_VSYNC_INT_MASK, 
		       dfli->reg_base + SPU_IRQ_ENA);
		
		rc = wait_event_interruptible_timeout(dfli->w_intr_wq,
						      atomic_read(&dfli->w_intr), 4);
		if ( rc < 0)
			printk(KERN_ERR "%s: gfx wait for vsync timed out, rc %d\n",
				__func__, rc);

		writel(irq_ena, 
		       dfli->reg_base + SPU_IRQ_ENA);
		atomic_set(&dfli->w_intr, 0);
		return 0;
	}

	return 0;
}

static void set_dumb_panel_control(struct fb_info *fi, int gpio_only)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_mach_info *dmi = dfli->dev->platform_data;
	u32	mask = 0x1;
	u32 x, x_bk;

	/*
	 * Preserve enable flag.
	 */
	if (gpio_only)
		mask |= ~(0xffff << 12);
	x_bk = readl(dfli->reg_base + LCD_SPU_DUMB_CTRL) & mask;
	x = dfli->is_blanked ? 0x0:0x1;

	x |= dmi->gpio_output_data << 20;
	x |= dmi->gpio_output_mask << 12;
	if (!gpio_only) {
		if (dfli->is_blanked &&
			(dmi->panel_rgb_type == DUMB24_RGB888_0))
			x |= 0x7 << 28;
		else
			/*
			 * When dumb interface isn't under 24bit
			 * It might be under SPI or GPIO. If set
			 * to 0x7 will force LCD_D[23:0] output
			 * blank color and damage GPIO and SPI
			 * behavior.
			 */
			x |= dmi->panel_rgb_type << 28;
		x |= dmi->panel_rgb_reverse_lanes ? 0x00000080 : 0;
		x |= dmi->invert_composite_blank ? 0x00000040 : 0;
		x |= (fi->var.sync & FB_SYNC_COMP_HIGH_ACT) ? 0x00000020 : 0;
		x |= dmi->invert_pix_val_ena ? 0x00000010 : 0;
		x |= (fi->var.sync & FB_SYNC_VERT_HIGH_ACT) ? 0 : 0x00000008;
		x |= (fi->var.sync & FB_SYNC_HOR_HIGH_ACT) ? 0 : 0x00000004;
		x |= dmi->invert_pixclock ? 0x00000002 : 0;
	}

	if (x != x_bk)
		writel(x, dfli->reg_base + LCD_SPU_DUMB_CTRL);
}

static void set_dumb_screen_dimensions(struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *v = &fi->var;
	struct dovefb_info *info = dfli->info;
	struct fb_videomode	 *ov = &info->out_vmode;
	int x;
	int y;
	u32 reg, reg_bk;

	reg_bk = readl(dfli->reg_base + LCD_SPUT_V_H_TOTAL);

	if (info->fixed_output) {
		x = ov->xres + ov->right_margin + ov->hsync_len +
			ov->left_margin;
		y = ov->yres + ov->lower_margin + ov->vsync_len +
			ov->upper_margin;
	} else {
		x = v->xres + v->right_margin + v->hsync_len + v->left_margin;
		y = v->yres + v->lower_margin + v->vsync_len + v->upper_margin;
	}

	reg = (y << 16)|x;

	if (reg != reg_bk)
		writel((y << 16) | x, dfli->reg_base + LCD_SPUT_V_H_TOTAL);
}

static void set_graphics_start(struct fb_info *fi, int xoffset, int yoffset)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;
	int pixel_offset0, pixel_offset1;
	unsigned long addr0, addr1, x;

	pixel_offset0 = (yoffset * var->xres_virtual) + xoffset;
	addr0 = dfli->fb_start_dma +
		(pixel_offset0 * (var->bits_per_pixel >> 3));
	/*
	 * Configure interlace mode.
	 */
	if ( FB_VMODE_INTERLACED & var->vmode) {
		/*
		 * Calc offset. (double offset).
		 * frame0 point to odd line,
		 * frame1 point to even line.
		 */ 
		pixel_offset1 = pixel_offset0 + var->xres_virtual;
		addr1 = dfli->fb_start_dma +
			(pixel_offset1 * (var->bits_per_pixel >> 3));
		
		/*
		 * Calc Pitch. (double pitch length)
		 */
		x = readl(dfli->reg_base + LCD_CFG_GRA_PITCH);
		x = (x & ~0xFFFF) | ((var->xres_virtual * var->bits_per_pixel) >> 2);
		
	} else {
		/*
		 * Calc offset.
		 */ 
		addr1 = addr0;

		/*
		 * Calc Pitch.
		 */
		x = readl(dfli->reg_base + LCD_CFG_GRA_PITCH);
		x = (x & ~0xFFFF) | ((var->xres_virtual * var->bits_per_pixel) >> 3);
	}

	writel(addr0, dfli->reg_base + LCD_CFG_GRA_START_ADDR0);
	writel(addr1, dfli->reg_base + LCD_CFG_GRA_START_ADDR1);
	writel(x, dfli->reg_base + LCD_CFG_GRA_PITCH);
}

static int set_frame_timings(const struct dovefb_layer_info *dfli,
	const struct fb_var_screeninfo *var)
{
	struct dovefb_info *info = dfli->info;
	unsigned int active_w, active_h;
	unsigned int ow, oh;
	unsigned int zoomed_w, zoomed_h;
	unsigned int lem, rim, lom, upm, hs, vs;
	unsigned int x;
	int total_w, total_h;
	unsigned int reg, reg_bk;

	/*
	 * Calc active size, zoomed size, porch.
	 */
	if (info->fixed_output) {
		active_w = info->out_vmode.xres;
		active_h = info->out_vmode.yres;
		zoomed_w = info->out_vmode.xres;
		zoomed_h = info->out_vmode.yres;
		lem = info->out_vmode.left_margin;
		rim = info->out_vmode.right_margin;
		upm = info->out_vmode.upper_margin;
		lom = info->out_vmode.lower_margin;
		hs = info->out_vmode.hsync_len;
		vs = info->out_vmode.vsync_len;
	} else {
		active_w = var->xres;
		active_h = var->yres;
		zoomed_w = var->xres;
		zoomed_h = var->yres;
		lem = var->left_margin;
		rim = var->right_margin;
		upm = var->upper_margin;
		lom = var->lower_margin;
		hs = var->hsync_len;
		vs = var->vsync_len;
	}
	
	/*
	 * Calc original size.
	 */
	ow = var->xres;
	oh = var->yres;

	/* interlaced workaround */
	if (FB_VMODE_INTERLACED & var->vmode) {
		active_h /= 2;
		zoomed_h /= 2;
		oh /= 2;
	}

	/* calc total width and height.*/
	total_w = active_w + rim + hs + lem;
	total_h = active_h + lom + vs + upm;

	/*
	 * Apply setting to registers.
	 */
	writel((active_h << 16) | active_w,
		dfli->reg_base + LCD_SPU_V_H_ACTIVE);

	writel((oh << 16) | ow,
		dfli->reg_base + LCD_SPU_GRA_HPXL_VLN);

	writel((zoomed_h << 16) | zoomed_w,
		dfli->reg_base + LCD_SPU_GZM_HPXL_VLN);
	
	writel((lem << 16) | rim, dfli->reg_base + LCD_SPU_H_PORCH);
	writel((upm << 16) | lom, dfli->reg_base + LCD_SPU_V_PORCH);

	reg_bk = readl(dfli->reg_base + LCD_SPUT_V_H_TOTAL);
	reg = (total_h << 16)|total_w;

	if (reg != reg_bk)
		writel(reg, dfli->reg_base + LCD_SPUT_V_H_TOTAL);
	/*
	 * Configure vsync adjust logic
	 */
	x = readl(dfli->reg_base+LCD_SPU_ADV_REG);
	x &= ~((0x1 << 12) | (0xfff << 20) | 0xfff);
	writel( (0x1 << 12) |
		(active_w+rim) << 20 |
		(active_w+rim), dfli->reg_base+LCD_SPU_ADV_REG);

	return 0;
}

static int dovefb_gfx_set_par(struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;
	const struct fb_videomode *m = 0;
	struct fb_videomode mode;
	int pix_fmt;
	u32 x;
	struct dovefb_mach_info *dmi;

	dmi = dfli->info->dev->platform_data;

	/*
	 * Determine which pixel format we're going to use.
	 */
	pix_fmt = dovefb_determine_best_pix_fmt(&fi->var, dfli);
	if (pix_fmt < 0)
		return pix_fmt;
	dfli->pix_fmt = pix_fmt;

	/*
	 * Set RGB bit field info.
	 */
	dovefb_set_pix_fmt(var, pix_fmt);

	/*
	 * Set additional mode info.
	 */
	if (pix_fmt == PIX_FMT_PSEUDOCOLOR)
		fi->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		fi->fix.visual = FB_VISUAL_TRUECOLOR;
	fi->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;

	x = readl(dfli->reg_base + SPU_IRQ_ENA);
	if (x & DOVEFB_GFX_INT_MASK)
		wait_for_vsync(dfli);

	/*
	 * Configure frame timings.
	 */
	set_frame_timings(dfli, var);

	/*
	 * convet var to video mode
	 */
	fb_var_to_videomode(&mode, &fi->var);
	m = &mode;

	/* Calculate clock divisor. */
	set_clock_divider(dfli, &mode);

	/* Configure dma ctrl regs. */
	set_dma_control0(dfli);
	set_dma_control1(dfli, fi->var.sync);

	/*
	 * Configure graphics DMA parameters.
	 */
	set_graphics_start(fi, fi->var.xoffset, fi->var.yoffset);

	/*
	 * Configure dumb panel ctrl regs & timings.
	 */
	set_dumb_panel_control(fi, 0);

	/*
	 * Re-enable panel output.
	 */
	x = readl(dfli->reg_base + LCD_SPU_DUMB_CTRL);
	if ((x & 0x1) == 0)
		writel(x | 1, dfli->reg_base + LCD_SPU_DUMB_CTRL);
#ifdef CONFIG_CH7025_COMPOSITE
	{
		struct input_stream_info info;
		if (strstr(fi->fix.id, "GFX Layer 0")) {
			info.xres = var->xres;
			info.yres = var->yres;
			info.iformat = CH7025_RGB888;
			info.oformat = CH7025_NTSC_M;
			info.swap = CH7025_RGB_ORDER;

			printk(KERN_INFO "video ch7025 enable.......\n");
			ch7025_set_input_stream(&info);
			ch7025_enable(1);
		}
	}
#endif
#ifdef CONFIG_KG2_ANX7150
	{
		AVC_CMD_TIMING_PARAM kg2_timing_param;

		if (strstr(fi->fix.id, "GFX Layer 0"))
		{
			kg2_timing_param.HTotal = var->left_margin + var->xres + var->right_margin + var->hsync_len;
			kg2_timing_param.HActive = var->xres;
			kg2_timing_param.HFrontPorch = var->right_margin;
			kg2_timing_param.HSyncWidth = var->hsync_len;
			kg2_timing_param.HPolarity = (var->sync & FB_SYNC_HOR_HIGH_ACT) ? 0: 1;
			kg2_timing_param.VTotal = var->upper_margin + var->yres + var->lower_margin + var->vsync_len;
			kg2_timing_param.VActive = var->yres;
			kg2_timing_param.VFrontPorch = var->lower_margin;
			kg2_timing_param.VSyncWidth = var->vsync_len;
			kg2_timing_param.VPolarity = (var->sync & FB_SYNC_VERT_HIGH_ACT) ? 0: 1;

			kg2_set_input_timing(&kg2_timing_param);
		}
	}
#endif

	return 0;
}

static int dovefb_pan_display(struct fb_var_screeninfo *var,
    struct fb_info *fi)
{
	wait_for_vsync(fi->par);
	set_graphics_start(fi, var->xoffset, var->yoffset);

	return 0;
}

static int dovefb_pwr_off_sram(struct dovefb_layer_info *dfli)
{
	unsigned int x;

	if (dfli) {
		x = readl(dfli->reg_base + LCD_SPU_SRAM_PARA1);
		x |=	CFG_PDWN256x32_MASK |
			CFG_PDWN256x24_MASK |
			CFG_PDWN256x8_MASK;
		writel(x, dfli->reg_base + LCD_SPU_SRAM_PARA1);
	}

	return 0;
}

static int dovefb_pwr_on_sram(struct dovefb_layer_info *dfli)
{
	unsigned int x;

	if (dfli) {
		x = readl(dfli->reg_base + LCD_SPU_SRAM_PARA1);
		x &=	~(CFG_PDWN256x32_MASK |
			CFG_PDWN256x24_MASK |
			CFG_PDWN256x8_MASK);
		writel(x, dfli->reg_base + LCD_SPU_SRAM_PARA1);
	}

	return 0;
}

static int dovefb_blank(int blank, struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;

	dfli->is_blanked = (blank == FB_BLANK_UNBLANK) ? 0 : 1;
	set_dumb_panel_control(fi, 0);

	return 0;
}

#ifdef DOVE_USING_HWC
static int dovefb_cursor(struct fb_info *fi, struct fb_cursor *cursor)
{
	struct dovefb_layer_info *dfli = fi->par;
	unsigned int x;

	/*
	 * in some case we don't want to anyone access hwc.
	 */
	if (dfli->cursor_cfg == 0)
		return 0;

	/* Too large of a cursor :-(*/
	if (cursor->image.width > 64 || cursor->image.height > 64) {
		printk(KERN_INFO "Error: cursor->image.width = %d"
				", cursor->image.height = %d\n",
				cursor->image.width, cursor->image.height);
		return -ENXIO;
	}

	/* 1. Disable cursor for updating anything. */
	if (!cursor->enable) {
		/* Disable cursor */
		x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) &
				~CFG_HWC_ENA_MASK;
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
		dfli->cursor_enabled = 0;
	}

	/* 2. Set Cursor Image */
	if (cursor->set & FB_CUR_SETIMAGE) {
		/*
		 * Not supported
		 */
	}

	/* 3. Set Cursor position */
	if (cursor->set & FB_CUR_SETPOS) {
		/* set position */
		x =  ((CFG_HWC_OVSA_VLN((cursor->image.dy - fi->var.yoffset)))|
			(cursor->image.dx - fi->var.xoffset));
		writel(x, dfli->reg_base + LCD_SPU_HWC_OVSA_HPXL_VLN);
	}

	/* 4. Set Cursor Hot spot */
	if (cursor->set & FB_CUR_SETHOT);
	/* set input value to HW register */

	/* 5. set Cursor color, Hermon2 support 2 cursor color. */
	if (cursor->set & FB_CUR_SETCMAP) {
		u32 bg_col_idx;
		u32 fg_col_idx;
		u32 fg_col, bg_col;

		bg_col_idx = cursor->image.bg_color;
		fg_col_idx = cursor->image.fg_color;
		fg_col = (((u32)fi->cmap.red[fg_col_idx] & 0xff00) << 8) |
			 (((u32)fi->cmap.green[fg_col_idx] & 0xff00) << 0) |
			 (((u32)fi->cmap.blue[fg_col_idx] & 0xff00) >> 8);
		bg_col = (((u32)fi->cmap.red[bg_col_idx] & 0xff00) << 8) |
			 (((u32)fi->cmap.green[bg_col_idx] & 0xff00) << 0) |
			 (((u32)fi->cmap.blue[bg_col_idx] & 0xff00) >> 8);

		/* set color1. */
		writel(fg_col, dfli->reg_base + LCD_SPU_ALPHA_COLOR1);

		/* set color2. */
		writel(bg_col, dfli->reg_base + LCD_SPU_ALPHA_COLOR2);
	}

	/* don't know how to set bitmask yet. */
	if (cursor->set & FB_CUR_SETSHAPE);

	if (cursor->set & FB_CUR_SETSIZE) {
		int i;
		int data_len;
		u32 data_tmp;
		u32 addr = 0;
		int width;
		int height;

		width = cursor->image.width;
		height = cursor->image.height;

		data_len = ((width*height*2) + 31) >> 5;

		/* 2. prepare cursor data */
		for (i = 0; i < data_len; i++) {
			/* Prepare data */
			writel(0xffffffff, dfli->reg_base+LCD_SPU_SRAM_WRDAT);

			/* write hwc sram */
			data_tmp = CFG_SRAM_INIT_WR_RD(SRAMID_INIT_WRITE)|
					CFG_SRAM_ADDR_LCDID(SRAMID_hwc)|
					addr;
			writel(data_tmp, dfli->reg_base+LCD_SPU_SRAM_CTRL);

			/* increasing address */
			addr++;
		}

		/* set size to register */
		x = CFG_HWC_VLN(cursor->image.height)|cursor->image.width;
		writel(x, dfli->reg_base + LCD_SPU_HWC_HPXL_VLN);
	}

	if (cursor->set & FB_CUR_SETALL);
	/* We have set all the flag above. Enable H/W Cursor. */

	/* if needed, enable cursor again */
	if (cursor->enable) {
		/* Enable cursor. */
		x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) |
			CFG_HWC_ENA(0x1);
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
		dfli->cursor_enabled = 1;
	}
	return 0;
}
#endif

static int dovefb_fb_sync(struct fb_info *info)
{
	/*struct dovefb_layer_info *dfli = info->par;*/

	return 0; /*wait_for_vsync(dfli);*/
}


/*
 *  dovefb_handle_irq(two lcd controllers)
 */
int dovefb_gfx_handle_irq(u32 isr, struct dovefb_layer_info *dfli)
{
	if (0x1 & dfli->reserved) {
		unsigned int vs_adj, x;
		unsigned int active_w, h_fp;

		active_w = 0xffff & readl(dfli->reg_base + LCD_SPU_V_H_ACTIVE); 
		h_fp = 0xffff & readl(dfli->reg_base + LCD_SPU_H_PORCH); 

		/* interlace mode workaround. */
		if (GRA_FRAME_IRQ0_ENA_MASK & isr) {
			vs_adj = active_w + h_fp;
		} else {
			vs_adj = (active_w/2) + h_fp;
		}

		x = readl(dfli->reg_base+LCD_SPU_ADV_REG);
		x &= ~((0x1 << 12) | (0xfff << 20) | 0xfff);
		x |= (0x1 << 12) | (vs_adj << 20) | vs_adj;
		writel( x, dfli->reg_base+LCD_SPU_ADV_REG);
	
	}

	/* wake up queue. */
	atomic_set(&dfli->w_intr, 1);
	wake_up(&dfli->w_intr_wq);
	return 1;
}


static u32 dovefb_moveHWC2SRAM(struct dovefb_layer_info *dfli, u32 size)
{
	u32 i, addr, count, sramCtrl, data_length, count_mod;
	u32 *tmpData = 0;
	u8 *pBuffer = dfli->hwc_buf;

	/* Initializing value. */
	addr = 0;
	sramCtrl = 0;
	data_length = size ; /* in byte. */
	count = data_length / 4;
	count_mod = data_length % 4;
	tmpData  = (u32 *)pBuffer;

	/* move new cursor bitmap to SRAM */
	/* printk(KERN_INFO "\n-------< cursor binary data >--------\n\n"); */
	for (i = 0; i < count; i++) {
		/* Prepare data */
		writel(*tmpData, dfli->reg_base + LCD_SPU_SRAM_WRDAT);

		/* printk(KERN_INFO "%d %8x\n\n", i, tmpData); */
		/* write hwc sram */
		sramCtrl = CFG_SRAM_INIT_WR_RD(SRAMID_INIT_WRITE)|
				CFG_SRAM_ADDR_LCDID(SRAMID_hwc)|
				addr;
		writel(sramCtrl, dfli->reg_base + LCD_SPU_SRAM_CTRL);

		/* increasing address */
		addr++;
		tmpData++;
	}
	/* printk(KERN_INFO "\n---< end of cursor binary data >---\n\n");
	*/

	/* not multiple of 4 bytes. */
	if (count_mod != 0) {
		printk(KERN_INFO "Error: move_count_mod != 0 \n");
		return -ENXIO;
	}

	return 0;
}


static u32 dovefb_set_cursor(struct fb_info *info,
			      struct _sCursorConfig *cursor_cfg)
{
	u16 w;
	u16 h;
	unsigned int x;
	struct dovefb_layer_info *dfli = info->par;

	/* Disable cursor */
	dfli->cursor_enabled = 0;
	dfli->cursor_cfg = 0;
	x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) &
		~(CFG_HWC_ENA_MASK|CFG_HWC_1BITENA_MASK|CFG_HWC_1BITMOD_MASK);
	writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);

	if (cursor_cfg->enable) {
		dfli->cursor_enabled = 1;
		dfli->cursor_cfg = 1;
		w  = cursor_cfg->width;
		h = cursor_cfg->height;

		/* set cursor display mode */
		if (DOVEFB_HWCMODE_1BITMODE == cursor_cfg->mode) {
			/* set to 1bit mode */
			x |= CFG_HWC_1BITENA(0x1);

			/* copy HWC buffer */
			if (cursor_cfg->pBuffer) {
				memcpy(dfli->hwc_buf, cursor_cfg->pBuffer,
					(w*h)>>3);
				dovefb_moveHWC2SRAM(dfli, (w*h)>>3);
			}
		} else if (DOVEFB_HWCMODE_2BITMODE == cursor_cfg->mode) {
			/* set to 2bit mode */
			/* do nothing, clear to 0 is 2bit mode. */

			/* copy HWC buffer */
			if (cursor_cfg->pBuffer) {
				memcpy(dfli->hwc_buf, cursor_cfg->pBuffer,
					(w*h*2)>>3) ;
				dovefb_moveHWC2SRAM(dfli, (w*h*2)>>3);
			}
		} else {
			printk(KERN_INFO "dovefb: Unsupported HWC mode\n");
			return -ENXIO;
		}

		/* set color1 & color2 */
		writel(cursor_cfg->color1,
			dfli->reg_base + LCD_SPU_ALPHA_COLOR1);
		writel(cursor_cfg->color2,
			dfli->reg_base + LCD_SPU_ALPHA_COLOR2);

		/* set position on screen */
		writel((CFG_HWC_OVSA_VLN(cursor_cfg->yoffset))|
			(cursor_cfg->xoffset),
			dfli->reg_base + LCD_SPU_HWC_OVSA_HPXL_VLN);

		/* set cursor size */
		writel(CFG_HWC_VLN(cursor_cfg->height)|cursor_cfg->width,
			dfli->reg_base + LCD_SPU_HWC_HPXL_VLN);

		/* Enable cursor */
		x |= CFG_HWC_ENA(0x1);
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
	} else {
		/* do nothing, we disabled it already. */
	}

	return 0;
}


static int dovefb_gfx_ioctl(struct fb_info *info, unsigned int cmd,
		unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dovefb_layer_info *dfli = info->par;

	switch (cmd) {
	case DOVEFB_IOCTL_WAIT_VSYNC:
		wait_for_vsync(dfli);
		break;
	case DOVEFB_IOCTL_CONFIG_CURSOR:
	{
		struct _sCursorConfig cursor_cfg;

		if (copy_from_user(&cursor_cfg, argp, sizeof(cursor_cfg)))
			return -EFAULT;

		return dovefb_set_cursor(info, &cursor_cfg);
	}
	case DOVEFB_IOCTL_DUMP_REGS:
		dovefb_dump_regs(dfli->info);
		break;
	default:
		;
	}

	return 0;
}

static u8 *dove_read_edid(struct fb_info *fi, struct dovefb_mach_info *dmi)
{
#ifdef CONFIG_FB_DOVE_CLCD_EDID
	struct i2c_adapter *dove_i2c;
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_info *info = dfli->info;
#endif
	char *edid_data = NULL;
	if (-1 == dmi->ddc_i2c_adapter)
		return edid_data;

#ifdef CONFIG_FB_DOVE_CLCD_EDID
	if (info->edid_en) {
		dove_i2c = i2c_get_adapter(dmi->ddc_i2c_adapter);
		/*
		 * Check match or not.
		 */
		if (dove_i2c)
			printk(KERN_INFO "  o Found i2c adapter for EDID detection\n");
		else {
			printk(KERN_WARNING "Couldn't find any I2C bus for EDID"
			       " provider\n");
			return NULL;
		}
		/* Look for EDID data on the selected bus */
		edid_data = fb_ddc_read(dove_i2c);
	}
#endif
	return edid_data;
}

static int dovefb_fill_edid(struct fb_info *fi,
				struct dovefb_mach_info *dmi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_info *info = dfli->info;
	int ret = 0;
	char *edid;

	/*
	 * check edid is ready.
	 */
	if (info->edid)
		return ret;

	if (!info->edid_en)
		return -3;

	/*
	 * Try to read EDID
	 */
	edid = dove_read_edid(fi, dmi);
	if (edid == NULL) {
		printk(KERN_INFO "  o Failed to read EDID information,"
		    "using driver resolutions table.\n");
		ret = -1;
	} else {
		/*
		 * parse and store edid.
		 */
		struct fb_monspecs *specs = &fi->monspecs;

		fb_edid_to_monspecs(edid, specs);

		if (specs->modedb) {
			fb_videomode_to_modelist(specs->modedb,
						specs->modedb_len,
						&fi->modelist);
			info->edid = 1;
		} else {
			ret = -2;
		}
	}

	kfree(edid);
	return ret;
}


static void dovefb_set_defaults(struct dovefb_layer_info *dfli)
{
	writel(0x80000001, dfli->reg_base + LCD_CFG_SCLK_DIV);
	writel(0x00000000, dfli->reg_base + LCD_SPU_BLANKCOLOR);
	/* known h/w issue. The bit [18:19] might
	 * 1. make h/w irq not workable. */
	/* 2. make h/w output skew data. */
	writel(dfli->info->io_pin_allocation,
			dfli->reg_base + SPU_IOPAD_CONTROL);
	writel(0x00000000, dfli->reg_base + LCD_CFG_GRA_START_ADDR1);
	writel(0x00000000, dfli->reg_base + LCD_SPU_GRA_OVSA_HPXL_VLN);
	writel(0x0, dfli->reg_base + LCD_SPU_SRAM_PARA0);
	writel(CFG_CSB_256x32(0x1)|CFG_CSB_256x24(0x1)|CFG_CSB_256x8(0x1),
		dfli->reg_base + LCD_SPU_SRAM_PARA1);
	writel(0x2032FF81, dfli->reg_base + LCD_SPU_DMA_CTRL1);
	return;
}

const struct fb_videomode *dovefb_find_nearest_mode(const struct fb_videomode *mode,
					        struct list_head *head)
{
	struct list_head *pos;
	struct fb_modelist *modelist;
	struct fb_videomode *cmode, *best = NULL;
	u32 diff = -1, diff_refresh = -1;
	u32 idx = 0;

	list_for_each(pos, head) {
		u32 d;

		modelist = list_entry(pos, struct fb_modelist, list);
		cmode = &modelist->mode;
		d = abs(cmode->xres - mode->xres) +
			abs(cmode->yres - mode->yres);
		if (diff > d) {
			diff = d;
			best = cmode;
			diff_refresh = -1;

			d = abs(cmode->refresh - mode->refresh);
			if (diff_refresh > d) {
				diff_refresh = d;
			}
		} else if (diff == d) {
			d = abs(cmode->refresh - mode->refresh);
			if (diff_refresh > d) {
				diff_refresh = d;
				best = cmode;
			}
		}

		idx++;
	}

	return best;
}

static void dovefb_list_vmode(const char *id, struct list_head *head)
{
	struct list_head *pos;
	struct fb_modelist *modelist;
	struct fb_videomode *m = NULL;
	u32 idx = 0;

	printk(KERN_INFO "------------<%s video mode database>-----------\n", id);
	list_for_each(pos, head) {

		modelist = list_entry(pos, struct fb_modelist, list);
		m = &modelist->mode;

		if (m)
			printk(KERN_INFO "mode %d: <%4dx%4d@%d> pico=%d\n"
				"\tfb timings   %4d %4d %4d %4d %4d %4d\n"
				"\txorg timings %4d %4d %4d %4d %4d %4d %4d %4d\n",
				idx++, m->xres, m->yres, m->refresh, m->pixclock,
				m->left_margin, m->right_margin, m->upper_margin, m->lower_margin, m->hsync_len, m->vsync_len,
				m->xres,
				m->xres + m->right_margin,
				m->xres + m->right_margin + m->hsync_len,
				m->xres + m->right_margin + m->hsync_len + m->left_margin,
				m->yres,
				m->yres + m->lower_margin,
				m->yres + m->lower_margin + m->vsync_len,
				m->yres + m->lower_margin + m->vsync_len + m->upper_margin);
				
	}
}

static int dovefb_init_mode(struct fb_info *fi,
				struct dovefb_mach_info *dmi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_info *info = dfli->info;
	struct fb_var_screeninfo *var = &fi->var;
	int ret = 0;
	u32 total_w, total_h, refresh;
	u64 div_result;
	const struct fb_videomode *m;
	
	/*
	 * Set default value
	 */
	refresh = DEFAULT_REFRESH;

	/*
	 * Fill up mode data to modelist.
	 */
	if (dovefb_fill_edid(fi, dmi))
		fb_videomode_to_modelist(dmi->modes,
					dmi->num_modes,
					&fi->modelist);

	/*
	 * Print all video mode in current mode list.
	 */
	dovefb_list_vmode(fi->fix.id, &fi->modelist);

	/*
	 * Check if we are in fixed output mode.
	 */
	if (info->fixed_output) {
		var->xres = info->out_vmode.xres;
		var->yres = info->out_vmode.yres;
		m = dovefb_find_nearest_mode(&info->out_vmode, &fi->modelist);
		if (m)
			info->out_vmode = *m;
		else {
			info->fixed_output = 0;
			printk(KERN_WARNING "DoveFB: Unsupported "
					"output resolution.\n");
		}
	}

	/*
	 * If has bootargs, apply it first.
	 */
	if (info->dft_vmode.xres && info->dft_vmode.yres &&
	    info->dft_vmode.refresh) {
		/* set data according bootargs */
		var->xres = info->dft_vmode.xres;
		var->yres = info->dft_vmode.yres;
		refresh = info->dft_vmode.refresh;
	}

	/* try to find best video mode. */
	m = dovefb_find_nearest_mode(&info->dft_vmode, &fi->modelist);
	if (m) {
		printk(KERN_INFO "found <%dx%d@%d>, pixclock=%d\n", m->xres, m->yres, m->refresh, m->pixclock);
		fb_videomode_to_var(&fi->var, m);
	} else {
		printk("Video mode list doesn't contain %dx%d, "
			"Please check display's edid support this mode "
			"or add built-in Mode\n"
			"Now we try 1024x768 mode.",
			var->xres, var->yres);
		fi->var.xres = 1024;
		fi->var.yres = 768;
	
		m = fb_find_best_mode(&fi->var, &fi->modelist);
		if (!m) {
			printk("Can't find 1024x768 either!!!\n");
			return -1;
		}
		fb_videomode_to_var(&fi->var, m);
	}

	/*
	 * if not using fixed output mode, fixed output mode should be
	 * equal to current video mode.
	 */
	if(!info->fixed_output)
		info->out_vmode = *m;

	/* Init settings. */
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;

	/* correct pixclock. */
	total_w = var->xres + var->left_margin + var->right_margin +
		  var->hsync_len;
	total_h = var->yres + var->upper_margin + var->lower_margin +
		  var->vsync_len;

	div_result = 1000000000000ll;
	do_div(div_result, total_w * total_h * refresh);
	var->pixclock = (u32)div_result;

	return ret;
}


#ifdef CONFIG_PM
int dovefb_gfx_suspend(struct dovefb_layer_info *dfli, pm_message_t mesg)
{
	struct fb_info *fi = dfli->fb_info;
	struct fb_var_screeninfo *var = &fi->var;

	printk(KERN_INFO "dovefb_gfx: dovefb_gfx_suspend(): state = %d.\n",
		mesg.event);
	printk(KERN_INFO "dovefb_gfx: suspend lcd %s\n", fi->fix.id);
	printk(KERN_INFO "dovefb_gfx: save resolution: <%dx%d>\n",
		var->xres, var->yres);

	init_ext_divider = 0;

	if (mesg.event & PM_EVENT_SLEEP) {
		fb_set_suspend(fi, 1);
		dovefb_blank(FB_BLANK_POWERDOWN, fi);
		dovefb_pwr_off_sram(dfli);
	}

	return 0;
}

int dovefb_gfx_resume(struct dovefb_layer_info *dfli)
{
	struct fb_info *fi = dfli->fb_info;
	struct fb_var_screeninfo *var = &fi->var;

	printk(KERN_INFO "dovefb_gfx: dovefb_gfx_resume().\n");
	printk(KERN_INFO "dovefb_gfx: resume lcd %s\n", fi->fix.id);
	printk(KERN_INFO "dovefb_gfx: restore resolution: <%dx%d>\n",
		var->xres, var->yres);

	dovefb_set_defaults(dfli);
	dfli->active = 1;

	dovefb_pwr_on_sram(dfli);
	if (dovefb_gfx_set_par(fi) != 0) {
		printk(KERN_INFO "dovefb_gfx_resume(): Failed in "
				"dovefb_gfx_set_par().\n");
		return -1;
	}
	
	fb_set_suspend(fi, 0);
	dovefb_blank(FB_BLANK_UNBLANK, fi);

	return 0;
}
#endif

int dovefb_gfx_init(struct dovefb_info *info, struct dovefb_mach_info *dmi)
{
	int ret;
	struct dovefb_layer_info *dfli = info->gfx_plane;
	struct fb_info *fi = dfli->fb_info;

	init_waitqueue_head(&dfli->w_intr_wq);

	/*
	 * Configure default register values.
	 */
	dovefb_set_defaults(dfli);

	/*
	 * init video mode data.
	 */
	dovefb_set_mode(dfli, &fi->var, dmi->modes, dmi->pix_fmt, 0);

	/* 
	 * configure GPIO's in order to enable the LCD panel to be ready for
	 * reading the edid data
	 */
	set_dumb_panel_control(fi, 1);
	ret = dovefb_init_mode(fi, dmi);
	if (ret)
		goto failed;

	/*
	 * Fill in sane defaults.
	 */
	ret = dovefb_gfx_set_par(fi);
	if (ret)
		goto failed;

	return 0;
failed:
	printk(KERN_ERR "dovefb_gfx_init() returned %d.\n", ret);
	return ret;
}

struct fb_ops dovefb_gfx_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= dovefb_check_var,
	.fb_set_par	= dovefb_gfx_set_par,
	.fb_setcolreg	= dovefb_setcolreg,
	.fb_blank	= dovefb_blank,
	.fb_pan_display	= dovefb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
#ifdef DOVE_USING_HWC
	.fb_cursor	= dovefb_cursor,
#endif
	.fb_sync	= dovefb_fb_sync,
	.fb_ioctl	= dovefb_gfx_ioctl,
};

MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_AUTHOR("Lennert Buytenhek <buytenh@marvell.com>");
MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_DESCRIPTION("Framebuffer driver for Dove");
MODULE_LICENSE("GPL");
