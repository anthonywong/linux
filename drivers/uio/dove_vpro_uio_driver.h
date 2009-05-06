#ifndef DOVE_VPRO_UIO_DRIVER_H
#define DOVE_VPRO_UIO_DRIVER_H

/****************************************
*	VPRO Memory Management Define   *
*****************************************/
#ifndef CONFIG_VPRO_NEW
#define VPRO_MEM_MMAP_AREA_NUM		3
#define VPRO_DMA_BUFFER_MAP_1		0
#define VPRO_DMA_BUFFER_MAP_2		1
#define VPRO_CONTROL_REGISTER_MAP	2
#else /* CONFIG_VPRO_NEW */
#define VPRO_MEM_MMAP_AREA_NUM		2
#define VPRO_DMA_BUFFER_MAP		0
#define VPRO_CONTROL_REGISTER_MAP	1
#endif /* CONFIG_VPRO_NEW */

#ifndef CONFIG_VPRO_NEW
#define VPRO_DMA_BUFFER_1_SIZE		0x300000 // 3M
#endif /* CONFIG_VPRO_NEW */


/************************
*   VPRO ioctl Define   *
*************************/
#define IOP_MAGIC	'v'

#define UIO_VPRO_IRQ_ENABLE		_IO(IOP_MAGIC, 0)
#define UIO_VPRO_IRQ_DISABLE		_IO(IOP_MAGIC, 1)

#endif
