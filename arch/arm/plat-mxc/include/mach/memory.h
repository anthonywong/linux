/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_MEMORY_H__
#define __ASM_ARCH_MXC_MEMORY_H__

#if defined CONFIG_ARCH_MX1
#define PHYS_OFFSET		UL(0x08000000)

#elif defined CONFIG_ARCH_MX2

#ifdef CONFIG_MACH_MX21
#define PHYS_OFFSET		UL(0xC0000000)
#endif /* CONFIG_MACH_MX21 */

#ifdef CONFIG_MACH_MX27
#define PHYS_OFFSET		UL(0xA0000000)
#endif /* CONFIG_MACH_MX27 */

#elif defined CONFIG_ARCH_MX3
#define PHYS_OFFSET		UL(0x80000000)
#endif /* CONFIG_ARCH_MX3 */

#include <asm/page.h>
#include <asm/sizes.h>

/* Start of physical RAM */
#if defined(CONFIG_MACH_MX35EVB) || defined(CONFIG_ARCH_MX51)
#define PHYS_OFFSET             UL(0x90000000)
#endif

#ifdef CONFIG_MACH_MX27ADS
#define PHYS_OFFSET             UL(0xA0000000)
#endif /* CONFIG_MACH_MX27ADS */

#ifdef CONFIG_MACH_MX37_3DS
#define PHYS_OFFSET             UL(0x40000000)
#endif /* CONFIG_MACH_MX37_3DS */

#ifndef PHYS_OFFSET
#define PHYS_OFFSET	        UL(0x80000000)
#endif /* PHYS_OFFSET */

/* Size of contiguous memory for DMA and other h/w blocks */
#ifdef CONFIG_ARCH_MX51
#define CONSISTENT_DMA_SIZE	(64 * SZ_1M)
#else
#define CONSISTENT_DMA_SIZE	(32 * SZ_1M)
#endif /* CONFIG_ARCH_MX51 */

#ifndef __ASSEMBLY__

#ifdef CONFIG_DMA_ZONE_SIZE
#define MXC_DMA_ZONE_SIZE	((CONFIG_DMA_ZONE_SIZE * SZ_1M) >> PAGE_SHIFT)
#else
#define MXC_DMA_ZONE_SIZE	((12 * SZ_1M) >> PAGE_SHIFT)
#endif /* CONFIG_DMA_ZONE_SIZE */

static inline void __arch_adjust_zones(int node, unsigned long *zone_size,
				       unsigned long *zhole_size)
{
	if (node != 0)
		return;
	/* Create separate zone to reserve memory for DMA */
	zone_size[1] = zone_size[0] - MXC_DMA_ZONE_SIZE;
	zone_size[0] = MXC_DMA_ZONE_SIZE;
	zhole_size[1] = zhole_size[0];
	zhole_size[0] = 0;
}

#define arch_adjust_zones(node, size, holes) \
	__arch_adjust_zones(node, size, holes)

#endif /* __ASSEMBLY__ */

/*
 * Virtual view <-> DMA view memory address translations
 * This macro is used to translate the virtual address to an address
 * suitable to be passed to set_dma_addr()
 */
#define __virt_to_bus(a)	__virt_to_phys(a)

/*
 * Used to convert an address for DMA operations to an address that the
 * kernel can use.
 */
#define __bus_to_virt(a)	__phys_to_virt(a)

#if defined(CONFIG_MX1_VIDEO)
/*
 * Increase size of DMA-consistent memory region.
 * This is required for i.MX camera driver to capture at least four VGA frames.
 */
#define CONSISTENT_DMA_SIZE SZ_4M
#endif /* CONFIG_MX1_VIDEO */

#if defined(CONFIG_MX3_VIDEO)
/*
 * Increase size of DMA-consistent memory region.
 * This is required for mx3 camera driver to capture at least two QXGA frames.
 */
#define CONSISTENT_DMA_SIZE SZ_8M
#endif /* CONFIG_MX3_VIDEO */

#endif /* __ASM_ARCH_MXC_MEMORY_H__ */
