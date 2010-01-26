#ifndef _ASM_X86_SPINLOCK_TYPES_H
#define _ASM_X86_SPINLOCK_TYPES_H

#ifndef __LINUX_SPINLOCK_TYPES_H
# error "please don't include this file directly"
#endif

typedef union {
	unsigned int slock;
	struct {
/*
 * Xen versions prior to 3.2.x have a race condition with HYPERVISOR_poll().
 */
#if CONFIG_XEN_COMPAT >= 0x030200
/*
 * On Xen we support a single level of interrupt re-enabling per lock. Hence
 * we can have twice as many outstanding tickets. Thus the cut-off for using
 * byte register pairs must be at half the number of CPUs.
 */
#if 2 * CONFIG_NR_CPUS < 256
# define TICKET_SHIFT 8
		u8 cur, seq;
#else
# define TICKET_SHIFT 16
		u16 cur, seq;
#endif
#endif
	};
} raw_spinlock_t;

#define __RAW_SPIN_LOCK_UNLOCKED	{ 0 }

typedef struct {
	unsigned int lock;
} raw_rwlock_t;

#define __RAW_RW_LOCK_UNLOCKED		{ RW_LOCK_BIAS }

#endif /* _ASM_X86_SPINLOCK_TYPES_H */
