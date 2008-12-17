/*
 * Elantech Touchpad driver
 *
 * Copyright (C) 2007 Arjan Opmeer <arjan@opmeer.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Trademarks are the property of their respective owners.
 */

#ifndef _ELANTECH_H
#define _ELANTECH_H

#define ELANTECH_COMMAND	0x11	/* Commands start with this value */
#define ELANTECH_FW_VER    0x01     /* Tom Lin */
struct elantech_data {
	unsigned char reg_10;
	unsigned char reg_11;
	unsigned char reg_20;
	unsigned char reg_21;
	unsigned char reg_22;
	unsigned char reg_23;
	unsigned char reg_24;
	unsigned char reg_25;
	unsigned char reg_26;
};


int elantech_detect(struct psmouse *psmouse, int set_properties);
int elantech_init(struct psmouse *psmouse, int set_properties);

#endif
