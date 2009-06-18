#ifndef DOVE_VPRO_UIO_DRIVER_H
#define DOVE_VPRO_UIO_DRIVER_H

/****************************************
*	vPro Memory Management Define              *
*****************************************/
#define VPRO_MEM_MMAP_AREA_NUM		3
#define VPRO_DMA_BUFFER_MAP_1		0
#define VPRO_DMA_BUFFER_MAP_2		1
#define VPRO_CONTROL_REGISTER_MAP	2

#define VPRO_DMA_BUFFER_1_SIZE		0x300000 // 3M

/************************
*   vPro Timer Define   *
*************************/
#define VPRO_TIMER_FREQ				HZ

/************************
*   vPro ioctl Define   *
*************************/
#define VPRO_IOCTL_TEST				0

#endif
