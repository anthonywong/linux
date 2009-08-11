#ifndef __ASM_ARCH_DOVE_NAND_H
#define __ASM_ARCH_DOVE_NAND_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

struct dove_nand_platform_data {
	unsigned int		tclk;		/* Clock supplied to NFC */
	unsigned int		nfc_width;	/* Width of NFC 16/8 bits */
	unsigned int		num_devs;	/* Number of NAND devices 
						   (2 for ganged mode).   */
	unsigned int		num_cs;		/* Number of NAND devices 
						   chip-selects.	  */
	unsigned int		use_dma;	/* Enable/Disable DMA 1/0 */
	unsigned int 		use_ecc;	/* Enable/Disable ECC 1/0 */
	unsigned int		use_bch;	/* Enable/Disable BCH 1/0 (if ECC enabled) */
	struct mtd_partition *	parts;
	unsigned int		nr_parts;	
};
#endif /* __ASM_ARCH_DOVE_NAND_H */
