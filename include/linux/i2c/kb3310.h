/*
 * linux/i2c/kb3310.h
 *
 * Configuration for ENE KB3310 keyboard/mouse driver.
 */

#ifndef __LINUX_KB3310_H
#define __LINUX_KB3310_H

#define KB3310_USE_PS2_KEYBOARD		0x01
#define KB3310_USE_PS2_MOUSE		0x02
#define KB3310_USE_BATTERY_INTERFACE	0x04

struct kb3310_platform_data {
	unsigned int enabled_ifaces;	/* Enabled interfaces using bits OR'ed 
					 * as defined above
					 */
};

#endif /* __LINUX_KB3310_H */
