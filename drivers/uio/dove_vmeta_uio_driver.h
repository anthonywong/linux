#ifndef DOVE_VMETA_UIO_DRIVER_H
#define DOVE_VMETA_UIO_DRIVER_H

/****************************************
*	VMETA Memory Management Define   *
*****************************************/
#ifndef CONFIG_VMETA_NEW
#define VMETA_MEM_MMAP_AREA_NUM		3
#define VMETA_DMA_BUFFER_MAP_1		0
#define VMETA_DMA_BUFFER_MAP_2		1
#define VMETA_CONTROL_REGISTER_MAP	2
#else /* CONFIG_VMETA_NEW */
#define VMETA_MEM_MMAP_AREA_NUM		2
#define VMETA_DMA_BUFFER_MAP		0
#define VMETA_CONTROL_REGISTER_MAP	1
#define CONFIG_MEM_FOR_MULTIPROCESS
#define MEM_SIZE_FOR_MULTIPROCESS	SZ_64K
#define VMETA_VDEC_MEM			2
#endif /* CONFIG_VMETA_NEW */

#ifndef CONFIG_VMETA_NEW
#define VMETA_DMA_BUFFER_1_SIZE		0x300000 // 3M
#endif /* CONFIG_VMETA_NEW */


/************************
*   VMETA ioctl Define   *
*************************/
#define IOP_MAGIC	'v'

struct vmeta_xv_frame {
	unsigned long phy_addr; 	// video frame physical addr
	unsigned long size;		// frame size
};

#define UIO_VMETA_RESERVED0		_IO(IOP_MAGIC, 0)
#define UIO_VMETA_RESERVED1		_IO(IOP_MAGIC, 1)
#define UIO_VMETA_IRQ_ENABLE		_IO(IOP_MAGIC, 2)
#define UIO_VMETA_IRQ_DISABLE		_IO(IOP_MAGIC, 3)
#define UIO_VMETA_XV_IN_QUEUE		_IOW(IOP_MAGIC, 4, struct vmeta_xv_frame)	// used for vmeta decoder to put a video frame in queue
#define UIO_VMETA_XV_DQUEUE		_IOR(IOP_MAGIC, 5, struct vmeta_xv_frame)	// used for vmeta decoder to free a video frame in queue
#define UIO_VMETA_XV_QUERY_VIDEO		_IOR(IOP_MAGIC, 6, struct vmeta_xv_frame)	// used for vo xv interface to query a video frame that from vmeta
#define UIO_VMETA_XV_FREE_VIDEO          _IOW(IOP_MAGIC, 7, struct vmeta_xv_frame)	// used for vo xv interface to free a video frame
#define UIO_VMETA_XV_INIT_QUEUE		_IO(IOP_MAGIC, 8)

#endif
