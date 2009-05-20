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

struct vpro_xv_frame {
	unsigned long phy_addr; 	// video frame physical addr
	unsigned long size;		// frame size
};

#define UIO_VPRO_RESERVED0		_IO(IOP_MAGIC, 0)
#define UIO_VPRO_RESERVED1		_IO(IOP_MAGIC, 1)
#define UIO_VPRO_IRQ_ENABLE		_IO(IOP_MAGIC, 2)
#define UIO_VPRO_IRQ_DISABLE		_IO(IOP_MAGIC, 3)
#define UIO_VPRO_XV_IN_QUEUE		_IOW(IOP_MAGIC, 4, struct vpro_xv_frame)	// used for vpro decoder to put a video frame in queue
#define UIO_VPRO_XV_DQUEUE		_IOR(IOP_MAGIC, 5, struct vpro_xv_frame)	// used for vpro decoder to free a video frame in queue
#define UIO_VPRO_XV_QUERY_VIDEO		_IOR(IOP_MAGIC, 6, struct vpro_xv_frame)	// used for vo xv interface to query a video frame that from vpro
#define UIO_VPRO_XV_FREE_VIDEO          _IOW(IOP_MAGIC, 7, struct vpro_xv_frame)	// used for vo xv interface to free a video frame
#define UIO_VPRO_XV_INIT_QUEUE		_IO(IOP_MAGIC, 8)

#endif
