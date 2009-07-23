/*
 *  Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_IO_H__
#define __ASM_ARCH_MXC_IO_H__

/* Allow IO space to be anywhere in the memory */
#define IO_SPACE_LIMIT 0xffffffff

#ifdef CONFIG_ARCH_MX3
#define __arch_ioremap __mx3_ioremap
#define __arch_iounmap __iounmap

static inline void __iomem *
__mx3_ioremap(unsigned long phys_addr, size_t size, unsigned int mtype)
{
	if (mtype == MT_DEVICE) {
		/* Access all peripherals below 0x80000000 as nonshared device
		 * but leave l2cc alone.
		 */
		if ((phys_addr < 0x80000000) && ((phys_addr < 0x30000000) ||
			(phys_addr >= 0x30000000 + SZ_1M)))
			mtype = MT_DEVICE_NONSHARED;
	}

	return __arm_ioremap(phys_addr, size, mtype);
}
#endif

extern void __iomem *__mxc_ioremap(unsigned long cookie, size_t size,
				unsigned int mtype);
extern void __mxc_iounmap(void __iomem *addr);

#define __arch_ioremap(a, s, f) __mxc_ioremap(a, s, f)
#define __arch_iounmap(a)        __mxc_iounmap(a)

/* io address mapping macro */
#define __io(a)		__typesafe_io(a)

#define __mem_pci(a)	(a)

/*!
 * This function is called to read a CPLD register over CSPI.
 *
 * @param        offset    number of the cpld register to be read
 *
 * @return       Returns 0 on success -1 on failure.
 */
unsigned int spi_cpld_read(unsigned int offset);

/*!
 * This function is called to write to a CPLD register over CSPI.
 *
 * @param        offset    number of the cpld register to be written
 * @param        reg_val   value to be written
 *
 * @return       Returns 0 on success -1 on failure.
 */
unsigned int spi_cpld_write(unsigned int offset, unsigned int reg_val);
#endif
