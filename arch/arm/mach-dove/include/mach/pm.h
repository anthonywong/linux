/*
 * include/asm-arm/arch-dove/pm.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_PM_H
#define __ASM_ARCH_PM_H

#include <asm/errno.h>
#include <mach/irqs.h>

#define PMU_INTERRUPT_CAUSE	(DOVE_PMU_VIRT_BASE + 0x50)
#define PMU_INTERRUPT_MASK	(DOVE_PMU_VIRT_BASE + 0x54)

static inline int pmu_to_irq(int pin)
{
	if (pin < NR_PMU_IRQS)
		return pin + IRQ_DOVE_PMU_START;

	return -EINVAL;
}

static inline int irq_to_pmu(int irq)
{
	if (IRQ_DOVE_PMU_START < irq && irq < NR_IRQS)
		return irq - IRQ_DOVE_PMU_START;

	return -EINVAL;
}

#endif
