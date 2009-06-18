#ifndef __ARCH_DOVE_MPP_H
#define __ARCH_DOVE_MPP_H

enum dove_mpp_type {
	/*
	 * This MPP is unused.
	 */
	MPP_UNUSED,

	/*
	 * This MPP pin is used as a generic GPIO pin.
	 */
	MPP_GPIO,

        /*
         * This MPP is used as a SATA activity LED.
         */
        MPP_SATA_LED,
        /*
         * This MPP is used as a functional pad.
         */
        MPP_FUNCTIONAL,

	MPP_SPI,

};

struct dove_mpp_mode {
	int			mpp;
	enum dove_mpp_type	type;
};

void dove_mpp_conf(struct dove_mpp_mode *mode);


#endif
