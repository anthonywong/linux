#include <linux/cpumask.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/kernel_stat.h>
#include <linux/mc146818rtc.h>
#include <linux/cache.h>
#include <linux/cpu.h>
#include <linux/module.h>

#include <asm/smp.h>
#include <asm/mtrr.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/apic.h>
#include <asm/proto.h>
#include <asm/ipi.h>

#include <xen/evtchn.h>

DECLARE_PER_CPU(int, ipi_to_irq[NR_IPIS]);

static inline void __send_IPI_one(unsigned int cpu, int vector)
{
	int irq = per_cpu(ipi_to_irq, cpu)[vector];
	BUG_ON(irq < 0);
	notify_remote_via_irq(irq);
}

static void __send_IPI_shortcut(unsigned int shortcut, int vector)
{
	unsigned int cpu;

	switch (shortcut) {
	case APIC_DEST_SELF:
		__send_IPI_one(smp_processor_id(), vector);
		break;
	case APIC_DEST_ALLBUT:
		for_each_online_cpu(cpu)
			if (cpu != smp_processor_id())
				__send_IPI_one(cpu, vector);
		break;
	case APIC_DEST_ALLINC:
		for_each_online_cpu(cpu)
			__send_IPI_one(cpu, vector);
		break;
	default:
		printk("XXXXXX __send_IPI_shortcut %08x vector %d\n", shortcut,
		       vector);
		break;
	}
}

void xen_send_IPI_mask_allbutself(const struct cpumask *cpumask, int vector)
{
	unsigned int cpu;
	unsigned long flags;

	local_irq_save(flags);
	WARN_ON(!cpumask_subset(cpumask, cpu_online_mask));
	for_each_cpu_and(cpu, cpumask, cpu_online_mask)
		if (cpu != smp_processor_id())
			__send_IPI_one(cpu, vector);
	local_irq_restore(flags);
}

void xen_send_IPI_mask(const struct cpumask *cpumask, int vector)
{
	unsigned int cpu;
	unsigned long flags;

	local_irq_save(flags);
	WARN_ON(!cpumask_subset(cpumask, cpu_online_mask));
	for_each_cpu_and(cpu, cpumask, cpu_online_mask)
		__send_IPI_one(cpu, vector);
	local_irq_restore(flags);
}

void xen_send_IPI_allbutself(int vector)
{
	__send_IPI_shortcut(APIC_DEST_ALLBUT, vector);
}

void xen_send_IPI_all(int vector)
{
	__send_IPI_shortcut(APIC_DEST_ALLINC, vector);
}

void xen_send_IPI_self(int vector)
{
	__send_IPI_shortcut(APIC_DEST_SELF, vector);
}
