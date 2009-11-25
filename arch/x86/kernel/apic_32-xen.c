/*
 *	Local APIC handling, local APIC timers
 *
 *	(c) 1999, 2000 Ingo Molnar <mingo@redhat.com>
 *
 *	Fixes
 *	Maciej W. Rozycki	:	Bits for genuine 82489DX APICs;
 *					thanks to Eric Gilmore
 *					and Rolf G. Tews
 *					for testing these extensively.
 *	Maciej W. Rozycki	:	Various updates and fixes.
 *	Mikael Pettersson	:	Power Management for UP-APIC.
 *	Pavel Machek and
 *	Mikael Pettersson	:	PM converted to driver model.
 */

#include <linux/init.h>

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/interrupt.h>
#include <linux/mc146818rtc.h>
#include <linux/kernel_stat.h>
#include <linux/sysdev.h>
#include <linux/cpu.h>
#include <linux/clockchips.h>
#include <linux/acpi_pmtmr.h>
#include <linux/module.h>

#include <asm/atomic.h>
#include <asm/smp.h>
#include <asm/mtrr.h>
#include <asm/mpspec.h>
#include <asm/desc.h>
#include <asm/arch_hooks.h>
#include <asm/hpet.h>
#include <asm/i8253.h>
#include <asm/nmi.h>

#include <mach_apic.h>
#include <mach_apicdef.h>
#include <mach_ipi.h>

#include "io_ports.h"

#ifndef CONFIG_XEN
/*
 * cpu_mask that denotes the CPUs that needs timer interrupt coming in as
 * IPIs in place of local APIC timers
 */
static cpumask_t timer_bcast_ipi;
#endif

/*
 * Knob to control our willingness to enable the local APIC.
 */

/*
 * Debug level, exported for io_apic.c
 */
int apic_verbosity;

#ifndef CONFIG_XEN
static int modern_apic(void)
{
	/* AMD systems use old APIC versions, so check the CPU */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD &&
	    boot_cpu_data.x86 >= 0xf)
		return 1;
	return lapic_get_version() >= 0x14;
}
#endif /* !CONFIG_XEN */

int get_physical_broadcast(void)
{
        return 0xff;
}

int setup_profiling_timer(unsigned int multiplier)
{
	return -EINVAL;
}

/*
 * This initializes the IO-APIC and APIC hardware if this is
 * a UP kernel.
 */
int __init APIC_init_uniprocessor (void)
{
#ifdef CONFIG_X86_IO_APIC
	if (smp_found_config)
		if (!skip_ioapic_setup && nr_ioapics)
			setup_IO_APIC();
#endif

	return 0;
}
