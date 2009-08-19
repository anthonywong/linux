/*
 * linux/include/video/dovefb_display.h -- Marvell display configuration driver
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by Green Wan <gwan@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */
#ifndef _DOVEDISPLAY_H_
#define	_DOVEDISPLAY_H_

#include <linux/fb.h>
#include <video/dovedcon.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>

struct dcon_config {
	unsigned char dcon_disable;
	unsigned char lcd_disable;
	unsigned char vga_disable;
	unsigned char mode_porta;
	unsigned char mode_portb;
};

#define DISPLAY_NORMAL		0
#define DISPLAY_CLONE		1
#define DISPLAY_DUALVIEW	2
#define DISPLAY_EXTENDED	3

struct display_settings {
	unsigned char display_mode;	/* 0: normal mode.
					 * 1: clone mode.
					 * 2: dualview mode.
					 * 3: extended mode.
					 */

	unsigned extend_ratio;		/*
					 * 1~4, default is 2.
					 * 1st portion width
					 * = src_width * (extend_ratio/4).
					 */
	struct fb_info *lcd0_gfx;
	struct fb_info *lcd0_vid;
	struct fb_info *lcd1_gfx;
	struct fb_info *lcd1_vid;
};



#ifdef __KERNEL__
#endif

#endif
