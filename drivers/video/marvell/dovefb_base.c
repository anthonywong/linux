/*
 * linux/drivers/video/marvell/marvell_base.c -- Marvell DOVE LCD Controller
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

#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include "dovefb_if.h"

#define MAX_HWC_SIZE		(64*64*2)
#define DEFAULT_REFRESH		60	/* Hz */
#define DOVEFB_INT_MASK (GRA_FRAME_IRQ0_ENA(0x1) | DMA_FRAME_IRQ0_ENA(0x1))

extern int enable_ebook;

static int dovefb_init_layer(struct platform_device *pdev,
		enum dovefb_type type, struct dovefb_info *info,
		struct resource *res);
static int dovefb_enable_lcd0(struct platform_device *pdev);


int dovefb_determine_best_pix_fmt(struct fb_var_screeninfo *var,
		struct dovefb_layer_info *dfli)
{
	/*
	 * Pseudocolor mode?
	 */
	if (var->bits_per_pixel == 8)
		return PIX_FMT_PSEUDOCOLOR;

	/* YUV for video layer only. */
	if (dfli->type == DOVEFB_OVLY_PLANE) {
		/*
		 * Check for YUV422PACK.
		 */
		if (var->bits_per_pixel == 16 && var->red.length == 16 &&
		    var->green.length == 16 && var->blue.length == 16) {
			if (var->red.offset >= var->blue.offset) {
				if (var->red.offset == 4)
					return PIX_FMT_UYVY422PACK;
				else
					return PIX_FMT_YUV422PACK;
			} else
				return PIX_FMT_YVU422PACK;
		}

		/*
		 * Check for YUV422PLANAR.
		 */
		if (var->bits_per_pixel == 16 && var->red.length == 8 &&
		    var->green.length == 4 && var->blue.length == 4) {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_YUV422PLANAR;
			else
				return PIX_FMT_YVU422PLANAR;
		}

		/*
		 * Check for YUV420PLANAR.
		 */
		if (var->bits_per_pixel == 12 && var->red.length == 8 &&
		    var->green.length == 2 && var->blue.length == 2) {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_YUV420PLANAR;
			else
				return PIX_FMT_YVU420PLANAR;
		}
	}

	/*
	 * Check for 565/1555.
	 */
	if (var->bits_per_pixel == 16 && var->red.length <= 5 &&
	    var->green.length <= 6 && var->blue.length <= 5) {
		if (var->transp.length == 0) {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_RGB565;
			else
				return PIX_FMT_BGR565;
		}

		if (var->transp.length == 1 && var->green.length <= 5) {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_RGB1555;
			else
				return PIX_FMT_BGR1555;
		}

		/* fall through */
	}

	/*
	 * Check for 888/A888.
	 */
	if (var->bits_per_pixel <= 32 && var->red.length <= 8 &&
	    var->green.length <= 8 && var->blue.length <= 8) {
		if (var->bits_per_pixel == 24 && var->transp.length == 0) {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_RGB888PACK;
			else
				return PIX_FMT_BGR888PACK;
		}

		if (var->bits_per_pixel == 32 && var->transp.length == 8) {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_RGBA888;
			else
				return PIX_FMT_BGRA888;
		} else {
			if (var->red.offset >= var->blue.offset)
				return PIX_FMT_RGB888UNPACK;
			else
				return PIX_FMT_BGR888UNPACK;
		}


		/* fall through */
	}

	return -EINVAL;
}


void dovefb_set_pix_fmt(struct fb_var_screeninfo *var, int pix_fmt)
{
	switch (pix_fmt) {
	case PIX_FMT_RGB565:
		var->bits_per_pixel = 16;
		var->red.offset = 11;    var->red.length = 5;
		var->green.offset = 5;   var->green.length = 6;
		var->blue.offset = 0;    var->blue.length = 5;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_BGR565:
		var->bits_per_pixel = 16;
		var->red.offset = 0;     var->red.length = 5;
		var->green.offset = 5;   var->green.length = 6;
		var->blue.offset = 11;   var->blue.length = 5;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB1555:
		var->bits_per_pixel = 16;
		var->red.offset = 10;    var->red.length = 5;
		var->green.offset = 5;   var->green.length = 5;
		var->blue.offset = 0;    var->blue.length = 5;
		var->transp.offset = 15; var->transp.length = 1;
		break;
	case PIX_FMT_BGR1555:
		var->bits_per_pixel = 16;
		var->red.offset = 0;     var->red.length = 5;
		var->green.offset = 5;   var->green.length = 5;
		var->blue.offset = 10;   var->blue.length = 5;
		var->transp.offset = 15; var->transp.length = 1;
		break;
	case PIX_FMT_RGB888PACK:
		var->bits_per_pixel = 24;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_BGR888PACK:
		var->bits_per_pixel = 24;
		var->red.offset = 0;     var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 16;   var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB888UNPACK:
		var->bits_per_pixel = 32;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_BGR888UNPACK:
		var->bits_per_pixel = 32;
		var->red.offset = 0;     var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 16;   var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGBA888:
		var->bits_per_pixel = 32;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 24; var->transp.length = 8;
		break;
	case PIX_FMT_BGRA888:
		var->bits_per_pixel = 32;
		var->red.offset = 0;     var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 16;   var->blue.length = 8;
		var->transp.offset = 24; var->transp.length = 8;
		break;
	case PIX_FMT_PSEUDOCOLOR:
		var->bits_per_pixel = 8;
		var->red.offset = 0;     var->red.length = 8;
		var->green.offset = 0;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;

	/* Video layer only */
	case PIX_FMT_YUV422PACK:
		var->bits_per_pixel = 16;
		var->red.offset = 8;	 var->red.length = 16;
		var->green.offset = 4;   var->green.length = 16;
		var->blue.offset = 0;   var->blue.length = 16;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_YVU422PACK:
		var->bits_per_pixel = 16;
		var->red.offset = 0;	 var->red.length = 16;
		var->green.offset = 8;   var->green.length = 16;
		var->blue.offset = 12;   var->blue.length = 16;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_YUV422PLANAR:
		var->bits_per_pixel = 16;
		var->red.offset = 8;	 var->red.length = 8;
		var->green.offset = 4;   var->green.length = 4;
		var->blue.offset = 0;   var->blue.length = 4;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_YVU422PLANAR:
		var->bits_per_pixel = 16;
		var->red.offset = 0;	 var->red.length = 8;
		var->green.offset = 8;   var->green.length = 4;
		var->blue.offset = 12;   var->blue.length = 4;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_YUV420PLANAR:
		var->bits_per_pixel = 12;
		var->red.offset = 4;	 var->red.length = 8;
		var->green.offset = 2;   var->green.length = 2;
		var->blue.offset = 0;   var->blue.length = 2;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_YVU420PLANAR:
		var->bits_per_pixel = 12;
		var->red.offset = 0;	 var->red.length = 8;
		var->green.offset = 8;   var->green.length = 2;
		var->blue.offset = 10;   var->blue.length = 2;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_UYVY422PACK:
		var->bits_per_pixel = 16;
		var->red.offset = 4;     var->red.length = 16;
		var->green.offset = 12;   var->green.length = 16;
		var->blue.offset = 0;    var->blue.length = 16;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	default:
	     printk(KERN_ERR "Unknown pixel format (%d).\n", pix_fmt);
	     dump_stack();
	}
}

void dovefb_set_mode(struct dovefb_layer_info *dfli,
		struct fb_var_screeninfo *var, struct fb_videomode *mode,
		int pix_fmt, int ystretch)
{
	dovefb_set_pix_fmt(var, pix_fmt);

	var->xres = mode->xres;
	var->yres = mode->yres;
	var->xres_virtual = max(var->xres, var->xres_virtual);
	if (ystretch)
		var->yres_virtual = dfli->fb_size /
			(var->xres_virtual * (var->bits_per_pixel >> 3));
	else
		var->yres_virtual = max(var->yres, var->yres_virtual);
	var->grayscale = 0;
	var->accel_flags = FB_ACCEL_NONE;
	var->pixclock = mode->pixclock;
	var->left_margin = mode->left_margin;
	var->right_margin = mode->right_margin;
	var->upper_margin = mode->upper_margin;
	var->lower_margin = mode->lower_margin;
	var->hsync_len = mode->hsync_len;
	var->vsync_len = mode->vsync_len;
	var->sync = mode->sync;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->rotate = FB_ROTATE_UR;
}

int dovefb_check_var(struct fb_var_screeninfo *var, struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;

	/*
	 * Basic geometry sanity checks.
	 */
	if (var->xoffset + var->xres > var->xres_virtual)
		return -EINVAL;
	if (var->yoffset + var->yres > var->yres_virtual)
		return -EINVAL;

	/*
	 * Check size of framebuffer.
	 */
	if (var->xres_virtual * var->yres_virtual *
	    (var->bits_per_pixel >> 3) > dfli->fb_size)
		return -EINVAL;

	return 0;
}

static unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	return ((chan & 0xffff) >> (16 - bf->length)) << bf->offset;
}


static u32 to_rgb(u16 red, u16 green, u16 blue)
{
	red >>= 8;
	green >>= 8;
	blue >>= 8;

	return (red << 16) | (green << 8) | blue;
}

int dovefb_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
		unsigned int blue, unsigned int trans, struct fb_info *fi)
{
	struct dovefb_layer_info *dlfi = fi->par;
	u32 val;

	if (fi->fix.visual == FB_VISUAL_TRUECOLOR && regno < 16) {
		val =  chan_to_field(red,   &fi->var.red);
		val |= chan_to_field(green, &fi->var.green);
		val |= chan_to_field(blue , &fi->var.blue);
		dlfi->pseudo_palette[regno] = val;
	}

	if (fi->fix.visual == FB_VISUAL_PSEUDOCOLOR && regno < 256) {
		val = to_rgb(red, green, blue);
		writel(val, dlfi->reg_base + LCD_SPU_SRAM_WRDAT);
		writel(0x8300 | regno, dlfi->reg_base + LCD_SPU_SRAM_CTRL);
	}

	return 0;
}

/*
 *  dovefb_handle_irq (two lcd controllers)
 */
static irqreturn_t dovefb_handle_irq(int irq, void *dev_id)
{
	struct dovefb_info *dfi = (struct dovefb_info *)dev_id;
	u32     isr;
	u32	ret = 0;

	isr = readl(dfi->reg_base + SPU_IRQ_ISR);

	if (isr & GRA_FRAME_IRQ0_ENA_MASK)
		ret += dovefb_gfx_handle_irq(isr, dfi->gfx_plane);

	if (isr & DMA_FRAME_IRQ0_ENA_MASK)
		ret += dovefb_ovly_handle_irq(isr, dfi->vid_plane);

	isr &= (GRA_FRAME_IRQ0_ENA_MASK | DMA_FRAME_IRQ0_ENA_MASK);

	writel(~isr, dfi->reg_base + SPU_IRQ_ISR);

	return IRQ_RETVAL(ret);
}


unsigned int dovefb_dump_regs(struct dovefb_info *dfi)
{
	u32 i;
	u32 reg;
	printk(KERN_INFO "Inside dovefb_lcd_dump:\n");

	for (i = 0xC0; i <= 0x01C4; i += 4) {
		reg = readl(dfi->reg_base + i);
		printk(KERN_INFO "Dumping LCD: offset=0x%08x, "
			"reg=0x%08x\n", i, reg);
	}

	return 0;
}


#ifndef MODULE
int __init dovefb_parse_options(char *options, struct dovefb_info *info,
		struct dovefb_mach_info *dmi, int id)
{
	char *name;
	unsigned int namelen;
	unsigned int xres = 0, yres = 0, bpp = 16, refresh = 0;
	char devname[8];

	info->mode_option = NULL;

	if (!options)
		return -1;

	sprintf(devname, "lcd%d:", id);
	name = strstr(options, devname);
	if (name == NULL)
		return -1;
	name += strlen(devname);

	namelen = strlen(name);
	info->mode_option = options;

	xres = simple_strtoul(name, &name, 10);
	if (xres == 0)
		goto bad_options;
	if ((name[0] != 'x') && (name[0] != 'X'))
		goto bad_options;
	name++;

	yres = simple_strtoul(name, &name, 10);
	if (yres == 0)
		goto bad_options;
	if (name[0] != '-')
		goto bad_options;
	name++;

	bpp = simple_strtoul(name, &name, 10);
	if (bpp == 0)
		goto bad_options;
	if (name[0] != '@')
		goto bad_options;
	name++;

	refresh = simple_strtoul(name, &name, 10);
	if (refresh == 0)
		goto bad_options;

	if (name[0] == '-') {
		/* This can be either "edid" or the fixed output resolution */
		if (strncmp("-edid", name, 5) == 0) {
			name += 5;
			info->edid_en = 1;
		} else {
			info->edid_en = 0;
		}

		if (name[0] == '-') {
			/* Get the fixed output resolution */
			name++;
			info->out_vmode.xres = simple_strtoul(name, &name, 10);
			if (info->out_vmode.xres == 0)
				goto bad_options;
			if ((name[0] != 'x') && (name[0] != 'X'))
				goto bad_options;
			name++;

			info->out_vmode.yres = simple_strtoul(name, &name, 10);
			if (info->out_vmode.yres == 0)
				goto bad_options;
			info->fixed_output = 1;
		}
	}
	printk(KERN_INFO "  o Kernel parameter: %dx%d-%d@%d.\n", xres, yres,
			bpp, refresh);
	if (info->fixed_output)
		printk(KERN_INFO "  o Fixed output resolution %dx%d.\n",
				info->out_vmode.xres, info->out_vmode.yres);
	info->dft_vmode.xres = xres;
	info->dft_vmode.yres = yres;
	info->dft_vmode.refresh = refresh;
	info->gfx_plane->fb_info->var.bits_per_pixel = bpp;
	switch (bpp) {
	case (8):
		dmi->pix_fmt = PIX_FMT_PSEUDOCOLOR;
		break;
	case(16):
		dmi->pix_fmt = PIX_FMT_RGB565;
		break;
	case (24):
		dmi->pix_fmt = PIX_FMT_RGB888PACK;
		break;
	case (32):
		dmi->pix_fmt = PIX_FMT_RGBA888;
		break;
	default:
		goto bad_options;
	}

	return 0;

bad_options:
	printk(KERN_INFO "  o Bad FB driver option %s , \
			use <xres>x<yres>-<bpp>@<refresh>[-edid][-out res]"
			"[-<outx>x<outy>].\n", options);
	return -1;
}
#endif


static int dovefb_init_layer(struct platform_device *pdev,
		enum dovefb_type type, struct dovefb_info *info,
		struct resource *res)
{
	struct dovefb_mach_info *dmi = pdev->dev.platform_data;
	struct fb_info *fi = 0;
	struct dovefb_layer_info *dfli = 0;
	int ret;

	fi = framebuffer_alloc(sizeof(struct dovefb_layer_info), &pdev->dev);
	if (fi == NULL) {
		ret = -ENOMEM;
		goto failed;
	}

	/* Initialize private data */
	dfli = fi->par;
	dfli->fb_info = fi;
	dfli->dev = &pdev->dev;
	dfli->is_blanked = 0;
	dfli->active = dmi->active;
	dfli->enabled = 1;
	dfli->type = type;
	dfli->info = info;
	dfli->reg_base = info->reg_base;

	if (type == DOVEFB_GFX_PLANE) {
		dfli->cursor_enabled = 0;
		dfli->cursor_cfg = 1;
		dfli->hwc_buf = (u8 *)__get_free_pages(GFP_DMA|GFP_KERNEL,
						get_order(MAX_HWC_SIZE));
		strcpy(fi->fix.id, dmi->id_gfx);
	} else {
		strcpy(fi->fix.id, dmi->id_ovly);
	}

	/*
	 * Initialise static fb parameters.
	 */
	fi->flags = FBINFO_DEFAULT | FBINFO_PARTIAL_PAN_OK |
		    FBINFO_HWACCEL_XPAN | FBINFO_HWACCEL_YPAN;
	fi->node = -1;
	fi->fix.type = FB_TYPE_PACKED_PIXELS;
	fi->fix.type_aux = 0;
	fi->fix.xpanstep = 1;
	fi->fix.ypanstep = 1;
	fi->fix.ywrapstep = 0;
	fi->fix.mmio_start = res->start;
	fi->fix.mmio_len = res->end - res->start + 1;
	fi->fix.accel = FB_ACCEL_NONE;
	fi->fbops = ((type == DOVEFB_GFX_PLANE) ?
			&dovefb_gfx_ops : &dovefb_ovly_ops);
	fi->pseudo_palette = dfli->pseudo_palette;

	/*
	 * Allocate framebuffer memory.
	 */
	dfli->fb_size = PAGE_ALIGN(DEFAULT_FB_SIZE);

	/*
	 * fix me, currently, vpro occupy a very large dma.
	 * It's better not to alloc DMA buffer from dma_alloc_xxx
	 */
#ifdef CONFIG_ARCH_DOVE
	dfli->fb_start = dma_alloc_writecombine(dfli->dev, dfli->fb_size,
						&dfli->fb_start_dma,
						GFP_KERNEL);
	if (!dfli->fb_start || !dfli->fb_start_dma) {
#else
	{
#endif
		dfli->new_addr = 0;
		dfli->mem_status = 1;
		dfli->fb_start = (void *)__get_free_pages(GFP_DMA | GFP_KERNEL,
					get_order(dfli->fb_size));
		dfli->fb_start_dma = (dma_addr_t)__virt_to_phys(dfli->fb_start);
	}

	if (dfli->fb_start == NULL) {
		ret = -ENOMEM;
		goto failed;
	}

	memset(dfli->fb_start, 0, dfli->fb_size);
	fi->fix.smem_start = dfli->fb_start_dma;
	fi->fix.smem_len = dfli->fb_size;
	fi->screen_base = dfli->fb_start;
	fi->screen_size = dfli->fb_size;

	/*
	 * Set video mode according to platform data.
	 */
	dovefb_set_mode(dfli, &fi->var, dmi->modes, dmi->pix_fmt, 0);

	/*
	 * Allocate color map.
	 */
	if (fb_alloc_cmap(&fi->cmap, 256, 0) < 0) {
		ret = -ENOMEM;
		goto failed;
	}

	if (type == DOVEFB_GFX_PLANE)
		info->gfx_plane = dfli;
	else
		info->vid_plane = dfli;

	return 0;

failed:
	printk(KERN_INFO "DoveFB: dovefb_init_layer() failed (%d).\n", ret);
	fb_dealloc_cmap(&fi->cmap);

	if (dfli->fb_start != NULL) {
		if (dfli->mem_status)
			free_pages((unsigned long)dfli->fb_start,
				get_order(dfli->fb_size));
#ifdef CONFIG_ARCH_DOVE
		else
			dma_free_writecombine(dfli->dev, dfli->fb_size,
				dfli->fb_start, dfli->fb_start_dma);
#endif
	}

	if (type == DOVEFB_GFX_PLANE)
		free_pages((unsigned long)dfli->hwc_buf,
				get_order(MAX_HWC_SIZE));

	if (fi != NULL)
		framebuffer_release(fi);

	return ret;
}

/*
 * Enable LCD0 output, this is a needed till the issue is fixed in future
 * revisions.
 */
static int dovefb_enable_lcd0(struct platform_device *pdev)
{
	struct dovefb_mach_info *dmi;
	struct resource *res;
	void *lcd0_base;

	dmi = pdev->dev.platform_data;
	if (dmi == NULL)
		return -EINVAL;

	if (dmi->enable_lcd0) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		if (res == NULL)
			return -EINVAL;
		lcd0_base = ioremap_nocache(res->start,
				res->end - res->start);
		if (lcd0_base == NULL)
			return -ENOMEM;
		writel(0x01, lcd0_base + LCD_CFG_SCLK_DIV);
		iounmap(lcd0_base);
	}

	return 0;
}

static int __init dovefb_probe(struct platform_device *pdev)
{
	struct dovefb_mach_info *dmi;
	struct dovefb_info *info = NULL;
	struct resource *res;
	int ret;
	char *option = NULL;

	dmi = pdev->dev.platform_data;
	if (dmi == NULL)
		return -EINVAL;

	printk(KERN_INFO "Dove FB driver:\n");
	ret = dovefb_enable_lcd0(pdev);
	if (ret)
		return -EINVAL;

	info = kzalloc(sizeof(struct dovefb_info), GFP_KERNEL);
	if (info == NULL) {
		printk(KERN_ERR "DoveFB: Failed to allocate mem.\n");
		ret = -ENOMEM;
		goto failed;
	}

	/*
	 * Map LCD controller registers.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "DoveFB: Failed to retrieve resources.\n");
		ret = -EINVAL;
		goto failed;
	}

	info->reg_base = ioremap_nocache(res->start, res->end - res->start);
	if (info->reg_base == NULL) {
		printk(KERN_ERR "DoveFB: Failed to map regs memory.\n");
		ret = -ENOMEM;
		goto failed;
	}

	if (dovefb_init_layer(pdev, DOVEFB_GFX_PLANE, info, res)) {
		printk(KERN_ERR "dovefb_init_layer() for GFX layer failed.\n");
		ret = -EINVAL;
		goto failed;
	}

	if (dovefb_init_layer(pdev, DOVEFB_OVLY_PLANE, info, res)) {
		printk(KERN_ERR "dovefb_init_layer() for VID layer failed.\n");
		ret = -EINVAL;
		goto failed;
	}

	/* Initialize private data */
	platform_set_drvdata(pdev, info);
	info->dev = &pdev->dev;

	/* General FB info */
	info->id = pdev->id;
	info->edid = 0;
	info->io_pin_allocation = dmi->io_pin_allocation;
	info->pix_fmt = dmi->pix_fmt;
	info->panel_rbswap = dmi->panel_rbswap;

	/* get LCD clock information. */
	info->clk = clk_get(&pdev->dev, "LCD");

#ifndef MODULE
	/*
	 * Get video option from boot args
	 */
	if (fb_get_options("dovefb", &option))
		return -ENODEV;

	if (!option) {
		printk(KERN_WARNING "DoveFB: No kernel parameters provided, "
				"using default.\n");
		option = CONFIG_FB_DOVE_CLCD_DEFAULT_OPTION;
	}
	/*
	 * Parse video mode out and save into var&par.
	 */
	ret = dovefb_parse_options(option, info, dmi, pdev->id);
#endif

	ret = dovefb_gfx_init(info, dmi);
	if (ret) {
		printk(KERN_ERR "DoveFB: dovefb_gfx_init() "
				"returned %d.\n", ret);
		goto failed;
	}

	ret = dovefb_ovly_init(info, dmi);
	if (ret) {
		printk(KERN_ERR "DoveFB: dovefb_ovly_init() "
				"returned %d.\n", ret);
		goto failed;
	}

	/*
	 * Get IRQ number.
	 */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		ret = -EINVAL;
		goto failed;
	}

	/*
	 * Register irq handler.
	 */
	ret = request_irq(res->start, dovefb_handle_irq, IRQF_SHARED,
			pdev->name, info);
	if (ret < 0) {
		printk(KERN_ERR "DoveFB: Failed to register IRQ.\n");
		goto failed_irq;
	}

	/*
	 * Enable interrupts
	 */
	writel(DOVEFB_INT_MASK, info->reg_base + SPU_IRQ_ENA);

	/*
	 * Register framebuffers.
	 */
	ret = register_framebuffer(info->gfx_plane->fb_info);
	if (ret < 0) {
		printk(KERN_ERR "DoveFB: Failed to register GFX FB.\n");
		goto failed_irq;
	}

	ret = register_framebuffer(info->vid_plane->fb_info);
	if (ret < 0) {
		printk(KERN_ERR "doveFB: Failed to register VID FB.\n");
		ret = -ENXIO;
		goto failed_fb;
	}

	printk(KERN_INFO "  o dovefb: frame buffer device was successfully "
			"loaded.\n");
	return 0;


failed_fb:
	unregister_framebuffer(info->vid_plane->fb_info);
failed_irq:
	free_irq(res->start, info);
failed:

	if (info && info->clk)
		clk_put(info->clk);

	platform_set_drvdata(pdev, NULL);

	if (info && info->reg_base)
		iounmap(info->reg_base);

	kfree(info);

	printk(KERN_INFO "dovefb: frame buffer device init failed (%d)\n", ret);

	return ret;
}

#ifdef CONFIG_PM
static int dovefb_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct dovefb_info *dfi = platform_get_drvdata(pdev);
	unsigned int reg;

	printk(KERN_INFO "dovefb_suspend(): state = %d ebook %d.\n",
	       mesg.event, enable_ebook);

	/* Disable interrupts */
	reg = readl(dfi->reg_base+SPU_IRQ_ENA);
	reg &= ~DOVEFB_INT_MASK;
	writel(reg, dfi->reg_base+SPU_IRQ_ENA);

	if (enable_ebook) 
		return 0;

	acquire_console_sem();

	if (dovefb_gfx_suspend(dfi->gfx_plane, mesg)) {
		printk(KERN_INFO "dovefb_suspend(): "
				"dovefb_gfx_suspend() failed.\n");
		release_console_sem();
		return -1;
	}

	if (dovefb_ovly_suspend(dfi->vid_plane, mesg)) {
		printk(KERN_INFO "dovefb_suspend(): "
				"dovefb_ovly_suspend() failed.\n");
		release_console_sem();
		return -1;
	}

	pdev->dev.power.power_state = mesg;

	clk_disable(dfi->clk);
	release_console_sem();

	return 0;
}

static int dovefb_resume(struct platform_device *pdev)
{
	struct dovefb_info *dfi = platform_get_drvdata(pdev);

	if (enable_ebook) {
		/* Enable interrupts */
		writel(readl(dfi->reg_base + SPU_IRQ_ENA) | DOVEFB_INT_MASK,
		       dfi->reg_base + SPU_IRQ_ENA);
		return 0;
	}

	printk(KERN_INFO "dovefb_resume().\n");

	acquire_console_sem();
	clk_enable(dfi->clk);

	if (dovefb_enable_lcd0(pdev)) {
		printk(KERN_INFO "dovefb_resume(): "
				"dovefb_enable_lcd0() failed.\n");
		return -1;
	}

	if (dovefb_gfx_resume(dfi->gfx_plane)) {
		printk(KERN_INFO "dovefb_resume(): "
				"dovefb_gfx_resume() failed.\n");
		return -1;
	}

	if (dovefb_ovly_resume(dfi->vid_plane)) {
		printk(KERN_INFO "dovefb_resume(): "
				"dovefb_ovly_resume() failed.\n");
		return -1;
	}

	/* Enable interrupts */
	writel(readl(dfi->reg_base + SPU_IRQ_ENA) | DOVEFB_INT_MASK,
			dfi->reg_base + SPU_IRQ_ENA);

	release_console_sem();

	return 0;
}
#endif

static struct platform_driver dovefb_driver = {
	.probe		= dovefb_probe,
#ifdef CONFIG_PM
	.suspend	= dovefb_suspend,
	.resume		= dovefb_resume,
#endif
	.driver		= {
		.name	= "dovefb",
		.owner	= THIS_MODULE,
	},
};

extern int __devinit mv_spi_init(void);
static int __devinit dovefb_init(void)
{
	int rc;
	rc = platform_driver_register(&dovefb_driver);

	if (rc)
		return rc;

	return mv_spi_init();
}

late_initcall(dovefb_init);

MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_AUTHOR("Lennert Buytenhek <buytenh@marvell.com>");
MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_DESCRIPTION("Framebuffer driver for Dove");
MODULE_LICENSE("GPL");
