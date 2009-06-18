/*
 * include/asm-arm/arch-mv78xx0/memory.h
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define PHYS_OFFSET		UL(0x00000000)

#define __virt_to_bus(x)	__virt_to_phys(x)
#define __bus_to_virt(x)	__phys_to_virt(x)


/* Override the ARM default */
#ifdef CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE

#if (CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE == 0)
#undef CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE
#define CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE 2
#endif

#define CONSISTENT_DMA_SIZE \
	(((CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE + 1) & ~1) * 1024 * 1024)

#endif


#endif
