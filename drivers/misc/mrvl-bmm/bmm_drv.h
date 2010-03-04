/*
 *  bmm_drv.h
 *
 *  Buffer Management Module
 *
 *  User/Driver level BMM Defines/Globals/Functions
 *
 *  Li Li (lea.li@marvell.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.

 *(C) Copyright 2007 Marvell International Ltd.
 * All Rights Reserved
 */

#ifndef _BMM_DRV_H
#define _BMM_DRV_H

#include <linux/dma-mapping.h>

#define BMM_MINOR		94

/* assemble an ioctl command */
#define BMM_IOCTL(cmd, arg)	(((cmd) << 16) | (arg))

/* disassemble an ioctl command */
#define BMM_IOCTL_CMD(cmd)	((cmd) >> 16)
#define BMM_IOCTL_ARG(cmd)	((cmd) & 0xffff)

/* ioctl commands */
#define BMM_MALLOC		(0)
#define BMM_FREE		(1)
#define BMM_GET_VIRT_ADDR	(2)
#define BMM_GET_PHYS_ADDR	(3)
#define BMM_GET_MEM_ATTR	(4)
#define BMM_SET_MEM_ATTR	(5)
#define BMM_GET_MEM_SIZE	(6)
#define BMM_GET_TOTAL_SPACE	(7)
#define BMM_GET_FREE_SPACE	(8)
#define BMM_FLUSH_CACHE		(9)
#define BMM_DMA_MEMCPY		(10)
#define BMM_DMA_SYNC		(11)
#define BMM_CONSISTENT_SYNC	(12)
#define BMM_DUMP		(13)

/* ioctl arguments: memory attributes */
#define BMM_ATTR_DEFAULT	(0)		/* cacheable bufferable */
#define BMM_ATTR_NONBUFFERABLE	(1 << 0)	/* non-bufferable */
#define BMM_ATTR_NONCACHEABLE	(1 << 1)	/* non-cacheable */
/* Note: extra attributes below are not supported yet! */
#define BMM_ATTR_HUGE_PAGE	(1 << 2)	/* 64KB page size */
#define BMM_ATTR_WRITETHROUGH	(1 << 3)	/* implies L1 Cacheable */
#define BMM_ATTR_L2_CACHEABLE	(1 << 4)	/* implies L1 Cacheable */

/* ioctl arguments: cache flush direction */
#define BMM_DMA_BIDIRECTIONAL	DMA_BIDIRECTIONAL	/* 0 */
#define BMM_DMA_TO_DEVICE	DMA_TO_DEVICE		/* 1 */
#define BMM_DMA_FROM_DEVICE	DMA_FROM_DEVICE		/* 2 */
#define BMM_DMA_NONE		DMA_NONE		/* 3 */

#ifdef CONFIG_DOVE_VPU_GPU_USE_BMM
extern unsigned int dove_vmeta_get_memory_start(void);
extern int dove_vmeta_get_memory_size(void);
extern unsigned int dove_gpu_get_memory_start(void);
extern int dove_gpu_get_memory_size(void);
#endif

#endif

