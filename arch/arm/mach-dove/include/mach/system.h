/*
 * include/asm-arm/arch-dove/system.h
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/hardware.h>
#include <mach/dove.h>

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	u32 u;

	/*
	 * Enable and issue soft reset
	 */
	u = readl(CPU_RESET_MASK);
	u |= (1 << 2);
	writel(u, CPU_RESET_MASK);

	u = readl(CPU_SOFT_RESET);
	u |= 1;
	writel(u, CPU_SOFT_RESET);

	while (1)
		;
}

#endif
