/*
 * chrome.c -- USB chrome composite driver
 *
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 by David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/device.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"

/*-------------------------------------------------------------------------*/

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "composite.c"
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"

#include "f_diag.c"

/*-------------------------------------------------------------------------*/

#define DRIVER_VENDOR_NUM	0x05C6
#define DRIVER_PRODUCT_NUM	0x900E

/*-------------------------------------------------------------------------*/

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.idVendor =		__constant_cpu_to_le16(DRIVER_VENDOR_NUM),
	.idProduct =		__constant_cpu_to_le16(DRIVER_PRODUCT_NUM),
	.bNumConfigurations =	1,
};

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = "Qualcomm Incorporated",
	[STRING_PRODUCT_IDX].s = "Qualcomm HSUSB Device",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
	{  }			/* end of list */
};


static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

char *serial_number = "0123456789ABCDEF";
/*-------------------------------------------------------------------------*/

static int __init chrome_bind_config(struct usb_configuration *c)
{
	return diag_function_add(c, serial_number);
}

static struct usb_configuration chrome_config_driver = {
	.label		= "chrome",
	.bind		= chrome_bind_config,
	.bConfigurationValue = 1,
	.bmAttributes	= (USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER |
				USB_CONFIG_ATT_WAKEUP),
	.bMaxPower	= 0x32, /* 100 mA */
};

static int __init chrome_bind(struct usb_composite_dev *cdev)
{
	int			gcnum;
	struct usb_gadget	*gadget = cdev->gadget;
	int			status;

	status = usb_string_id(cdev);
	if (status < 0)
		return status;
	strings_dev[STRING_MANUFACTURER_IDX].id = status;
	device_desc.iManufacturer = status;

	status = usb_string_id(cdev);
	if (status < 0)
		return status;
	strings_dev[STRING_PRODUCT_IDX].id = status;
	device_desc.iProduct = status;

	/* config description */
	status = usb_string_id(cdev);
	if (status < 0)
		return status;
	strings_dev[STRING_SERIAL_IDX].id = status;
	device_desc.iSerialNumber = status;

	/* set up other descriptors */
	/* REVISIT */
	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 | gcnum);
	else {
		/* this is so simple (for now, no altsettings) that it
		 * SHOULD NOT have problems with bulk-capable hardware.
		 * so warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("chrome_bind: controller '%s' not recognized\n",
			gadget->name);
		device_desc.bcdDevice =
			__constant_cpu_to_le16(0x0200 | 0x0099);
	}

	/* register our configuration */
	status = usb_add_config(cdev, &chrome_config_driver);

	return status;
}

static struct usb_composite_driver chrome_driver = {
	.name		= "g_chrome",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= chrome_bind,
};

static int __init init(void)
{
	return usb_composite_register(&chrome_driver);
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&chrome_driver);
}
module_exit(cleanup);
