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

#define MAX_HWC_SIZE		(64*64*2)
#define DEFAULT_REFRESH		60	/* Hz */

static int dovefb_fill_edid(struct fb_info *fi,
				struct dovefb_mach_info *dmi);
static int wait_for_vsync(struct dovefb_layer_info *dfli);
static void dovefb_set_defaults(struct dovefb_layer_info *dfli);

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
static void set_clock_divider(struct dovefb_layer_info *dfli,
	const struct fb_videomode *m)
{
	int divider_int;
	int needed_pixclk;
	u64 div_result;
	u32 x = 0;
	struct dovefb_mach_info *dmi = dfli->dev->platform_data;

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
	do_div(div_result, m->pixclock);
	needed_pixclk = (u32)div_result;

	divider_int = dmi->sclk_clock / needed_pixclk;

	/* check whether divisor is too small. */
	if (divider_int < 2) {
		printk(KERN_WARNING "Warning: clock source is too slow."
				 "Try smaller resolution\n");
		divider_int = 2;
	}

	/*
	 * Set setting to reg.
	 */
	x |= divider_int;
	writel(x, dfli->reg_base + LCD_CFG_SCLK_DIV);
}

static void set_dma_control0(struct dovefb_layer_info *dfli)
{
	u32 x;

	/*
	 * Set bit to enable graphics DMA.
	 */
	x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0);
	x |= (dfli->active && dfli->enabled) ? CFG_GRA_ENA_MASK : 0;
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

	writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
}

static void set_dma_control1(struct dovefb_layer_info *dfli, int sync)
{
	u32 x;

	/*
	 * Configure default bits: vsync triggers DMA, gated clock
	 * enable, power save enable, configure alpha registers to
	 * display 100% graphics, and set pixel command.
	 */
	x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL1);

	/*
	 * We trigger DMA on the falling edge of vsync if vsync is
	 * active low, or on the rising edge if vsync is active high.
	 */
	if (!(sync & FB_SYNC_VERT_HIGH_ACT))
		x |= 0x08000000;


	writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL1);
}

static int wait_for_vsync(struct dovefb_layer_info *dfli)
{
	if (dfli) {
		wait_event_interruptible(dfli->w_intr_wq,
				atomic_read(&dfli->w_intr));
		atomic_set(&dfli->w_intr, 0);
		return 0;
	}

	return 0;
}

static void set_graphics_start(struct fb_info *fi, int xoffset, int yoffset)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;
	int pixel_offset;
	unsigned long addr;

	pixel_offset = (yoffset * var->xres_virtual) + xoffset;

	addr = dfli->fb_start_dma + (pixel_offset * (var->bits_per_pixel >> 3));
	writel(addr, dfli->reg_base + LCD_CFG_GRA_START_ADDR0);
}

static int dovefb_pan_display(struct fb_var_screeninfo *var,
    struct fb_info *fi)
{
	set_graphics_start(fi, var->xoffset, var->yoffset);

	return 0;
}

static void set_dumb_panel_control(struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_mach_info *dmi = dfli->dev->platform_data;
	u32 x;

	/*
	 * Preserve enable flag.
	 */
	x = readl(dfli->reg_base + LCD_SPU_DUMB_CTRL) & 0x00000001;

	x |= (dfli->is_blanked ? 0x7 : dmi->panel_rgb_type) << 28;
	x |= dmi->gpio_output_data << 20;
	x |= dmi->gpio_output_mask << 12;
	x |= dmi->panel_rgb_reverse_lanes ? 0x00000080 : 0;
	x |= dmi->invert_composite_blank ? 0x00000040 : 0;
	x |= (fi->var.sync & FB_SYNC_COMP_HIGH_ACT) ? 0x00000020 : 0;
	x |= dmi->invert_pix_val_ena ? 0x00000010 : 0;
	x |= (fi->var.sync & FB_SYNC_VERT_HIGH_ACT) ? 0 : 0x00000008;
	x |= (fi->var.sync & FB_SYNC_HOR_HIGH_ACT) ? 0 : 0x00000004;
	x |= dmi->invert_pixclock ? 0x00000002 : 0;

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

	if (info->fixed_output) {
		x = ov->xres + ov->right_margin + ov->hsync_len +
			ov->left_margin;
		y = ov->yres + ov->lower_margin + ov->vsync_len +
			ov->upper_margin;
	} else {
		x = v->xres + v->right_margin + v->hsync_len + v->left_margin;
		y = v->yres + v->lower_margin + v->vsync_len + v->upper_margin;
	}

	writel((y << 16) | x, dfli->reg_base + LCD_SPUT_V_H_TOTAL);
}

static int dovefb_gfx_set_par(struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_info *info = dfli->info;
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
	 * Set additional mode info.
	 */
	if (pix_fmt == PIX_FMT_PSEUDOCOLOR)
		fi->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		fi->fix.visual = FB_VISUAL_TRUECOLOR;
	fi->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;

	/*
	 * Disable panel output while we setup the display.
	 */
	x = readl(dfli->reg_base + LCD_SPU_DUMB_CTRL);
	writel(x & ~1, dfli->reg_base + LCD_SPU_DUMB_CTRL);

	/*
	 * Configure global panel parameters.
	 */
	if (info->fixed_output)
		writel((info->out_vmode.yres << 16) | info->out_vmode.xres,
			dfli->reg_base + LCD_SPU_V_H_ACTIVE);
	else
		writel((var->yres << 16) | var->xres,
			dfli->reg_base + LCD_SPU_V_H_ACTIVE);

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
	x = readl(dfli->reg_base + LCD_CFG_GRA_PITCH);
	x = (x & ~0xFFFF) | ((var->xres_virtual * var->bits_per_pixel) >> 3);
	writel(x, dfli->reg_base + LCD_CFG_GRA_PITCH);
	writel((var->yres << 16) | var->xres,
			dfli->reg_base + LCD_SPU_GRA_HPXL_VLN);
	writel((var->yres << 16) | var->xres,
			dfli->reg_base + LCD_SPU_GRA_HPXL_VLN);

	if (info->fixed_output)
		writel((info->out_vmode.yres << 16) | info->out_vmode.xres,
				dfli->reg_base + LCD_SPU_GZM_HPXL_VLN);
	else
		writel((var->yres << 16) | var->xres,
				dfli->reg_base + LCD_SPU_GZM_HPXL_VLN);

	/*
	 * Configure dumb panel ctrl regs & timings.
	 */
	set_dumb_panel_control(fi);
	set_dumb_screen_dimensions(fi);

	if (info->fixed_output) {
		writel((info->out_vmode.left_margin << 16) |
				info->out_vmode.right_margin,
				dfli->reg_base + LCD_SPU_H_PORCH);
		writel((info->out_vmode.upper_margin << 16) |
				info->out_vmode.lower_margin,
				dfli->reg_base + LCD_SPU_V_PORCH);
	} else {
		writel((var->left_margin << 16) | var->right_margin,
				dfli->reg_base + LCD_SPU_H_PORCH);
		writel((var->upper_margin << 16) | var->lower_margin,
				dfli->reg_base + LCD_SPU_V_PORCH);
	}

	/*
	 * Re-enable panel output.
	 */
	x = readl(dfli->reg_base + LCD_SPU_DUMB_CTRL);
	writel(x | 1, dfli->reg_base + LCD_SPU_DUMB_CTRL);

	return 0;
}

static int dovefb_blank(int blank, struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;

	dfli->is_blanked = (blank == FB_BLANK_UNBLANK) ? 0 : 1;
	set_dumb_panel_control(fi);

	return 0;
}

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

static u8 *dove_read_edid(struct fb_info *info, struct dovefb_mach_info *dmi)
{
#ifdef CONFIG_FB_DOVE_CLCD_EDID
	struct i2c_adapter *dove_i2c;
	struct dovefb_info *dfli = info->par;
	int loop = 0;
#endif
	char *edid_data = NULL;

	if (NULL == dmi->ddc_i2c_adapter)
		return edid_data;

#ifdef CONFIG_FB_DOVE_CLCD_EDID
	if (dfli->edid_en) {
		/*
		 * Loop through all I2C adapters and find the matching one
		 * for Dove.
		 */
		while ((dove_i2c = i2c_get_adapter(loop))) {
			/*
			 * check null ptr.
			 */
			if (NULL == dev_name(&dove_i2c->dev))
				return edid_data;

			/*
			 * Check match or not.
			 */
			if (!strcmp(dmi->ddc_i2c_adapter,
						dev_name(&dove_i2c->dev))) {
				printk(KERN_INFO "  o Found i2c bus number %d"
				" for EDID detection\n", loop);
				break;
			}
			loop++;
		}
		if (!dove_i2c) {
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
	struct dovefb_info *dfli = fi->par;
	int ret = 0;
	char *edid;

	/*
	 * check edid is ready.
	 */
	if (dfli->edid)
		return ret;

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
			dfli->edid = 1;
		} else {
			ret = -2;
		}
	}

	kfree(edid);
	return ret;
}


static void dovefb_set_defaults(struct dovefb_layer_info *dfli)
{
	writel(0x00000000, dfli->reg_base + LCD_SPU_BLANKCOLOR);
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
	 * Check if we are in fixed output mode.
	 */
	if (info->fixed_output) {
		var->xres = info->out_vmode.xres;
		var->yres = info->out_vmode.yres;
		m = fb_find_best_mode(&fi->var, &fi->modelist);
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
	m = fb_find_best_mode(&fi->var, &fi->modelist);
	if (m)
		fb_videomode_to_var(&fi->var, m);

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

	printk(KERN_INFO "dovefb_gfx_suspend(): state = %d.\n", mesg.event);

	if (mesg.event & PM_EVENT_SLEEP) {
		fb_set_suspend(fi, 1);
		dovefb_blank(FB_BLANK_POWERDOWN, fi);
	}

	return 0;
}

int dovefb_gfx_resume(struct dovefb_layer_info *dfli)
{
	struct fb_info *fi = dfli->fb_info;

	printk(KERN_INFO "dovefb_gfx_resume().\n");

	dovefb_set_defaults(dfli);
	dfli->active = 1;

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
	 * init video mode data.
	 */
	dovefb_set_mode(dfli, &fi->var, dmi->modes, dmi->pix_fmt, 0);
	ret = dovefb_init_mode(fi, dmi);
	if (ret)
		goto failed;

	/*
	 * Fill in sane defaults.
	 */
	ret = dovefb_gfx_set_par(fi);
	if (ret)
		goto failed;

	/*
	 * Configure default register values.
	 */
	dovefb_set_defaults(dfli);

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
	.fb_cursor	= dovefb_cursor,
	.fb_sync	= dovefb_fb_sync,
	.fb_ioctl	= dovefb_gfx_ioctl,
};

MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_AUTHOR("Lennert Buytenhek <buytenh@marvell.com>");
MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_DESCRIPTION("Framebuffer driver for Dove");
MODULE_LICENSE("GPL");
