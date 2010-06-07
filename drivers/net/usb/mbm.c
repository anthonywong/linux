/* -*- linux-c -*-
 * Copyright (C) 2008 Carl Nordbeck <Carl.Nordbeck@ericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/ctype.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/usb.h>
#include <linux/usb/cdc.h>
#include "usbnet.h"

#define DRIVER_VERSION "0.03"

/* Bogus speed for bugy HSPA modems */
#define TX_LINK_SPEED 0x001E8480	/* 2.0 Mbps */
#define RX_LINK_SPEED 0x006DDD00	/* 7.2 Mbps */
#define FIX_SPEED 0x00989680	/* 10.0 Mbps */

struct mbm_data {
	unsigned int rx_speed;
	unsigned int tx_speed;
	unsigned int connect;
};

static const u8 mbm_guid[16] = {
	0xa3, 0x17, 0xa8, 0x8b, 0x04, 0x5e, 0x4f, 0x01,
	0xa6, 0x07, 0xc0, 0xff, 0xcb, 0x7e, 0x39, 0x2a,
};
static void dumpspeed(struct usbnet *dev, __le32 *speeds)
{
	struct mbm_data *data = (void *)&dev->data;

	data->rx_speed = __le32_to_cpu(speeds[0]);
	data->tx_speed = __le32_to_cpu(speeds[1]);

	if (data->rx_speed == FIX_SPEED && data->tx_speed == FIX_SPEED) {
/* Bogus speed for buggy HSPA modems */
		dev_info(&dev->udev->dev,
			 "link speeds: %u kbps RX, %u kbps TX\n",
			 RX_LINK_SPEED / 1000, TX_LINK_SPEED / 1000);

		data->rx_speed = RX_LINK_SPEED;
		data->tx_speed = TX_LINK_SPEED;
	} else
		dev_info(&dev->udev->dev,
			 "link speeds: %u kbps RX, %u kbps TX\n",
			 __le32_to_cpu(speeds[0]) / 1000,
			 __le32_to_cpu(speeds[1]) / 1000);
}

static void mbm_status(struct usbnet *dev, struct urb *urb)
{
	struct mbm_data *data = (void *)&dev->data;
	struct usb_cdc_notification *event;

	if (urb->actual_length < sizeof(*event))
		return;

	/* SPEED_CHANGE can get split into two 8-byte packets */
	if (test_and_clear_bit(EVENT_STS_SPLIT, &dev->flags)) {
		dumpspeed(dev, (__le32 *) urb->transfer_buffer);
		return;
	}

	event = urb->transfer_buffer;
	switch (event->bNotificationType) {
	case USB_CDC_NOTIFY_NETWORK_CONNECTION:
		data->connect = event->wValue;
		if (netif_msg_timer(dev))
			dev_dbg(&dev->udev->dev, "CDC: carrier %s\n",
				data->connect ? "on" : "off");
		if (event->wValue)
			netif_carrier_on(dev->net);
		else
			netif_carrier_off(dev->net);
		break;
	case USB_CDC_NOTIFY_SPEED_CHANGE:	/* tx/rx rates */
		if (netif_msg_timer(dev))
			dev_dbg(&dev->udev->dev, "CDC: speed change (len %d)\n",
				urb->actual_length);
		if (urb->actual_length != (sizeof(*event) + 8))
			set_bit(EVENT_STS_SPLIT, &dev->flags);
		else
			dumpspeed(dev, (__le32 *) &event[1]);
		break;
	default:
		dev_err(&dev->udev->dev, "CDC: unexpected notification %02x!\n",
			event->bNotificationType);
		break;
	}
}

static u8 nibble(unsigned char c)
{
	if (likely(isdigit(c)))
		return c - '0';
	c = toupper(c);
	if (likely(isxdigit(c)))
		return 10 + c - 'A';
	return 0;
}

static inline int
get_ethernet_addr(struct usbnet *dev, struct usb_cdc_ether_desc *e)
{
	int tmp, i;
	unsigned char buf[13];

	tmp = usb_string(dev->udev, e->iMACAddress, buf, sizeof(buf));
	if (tmp != 12) {
		dev_dbg(&dev->udev->dev,
			"bad MAC string %d fetch, %d\n", e->iMACAddress, tmp);
		if (tmp >= 0)
			tmp = -EINVAL;
		return tmp;
	}
	for (i = tmp = 0; i < 6; i++, tmp += 2)
		dev->net->dev_addr[i] =
		    (nibble(buf[tmp]) << 4) + nibble(buf[tmp + 1]);
	return 0;
}

static void mbm_get_drvinfo(struct net_device *net,
			     struct ethtool_drvinfo *info)
{
	struct usbnet *dev = netdev_priv(net);

	strncpy(info->driver, dev->driver_name, sizeof(info->driver));
	strncpy(info->version, DRIVER_VERSION, sizeof(info->version));
	strncpy(info->fw_version, dev->driver_info->description,
		sizeof(info->fw_version));
	usb_make_path(dev->udev, info->bus_info, sizeof(info->bus_info));
}

static struct ethtool_ops mbm_ethtool_ops = {
	.get_drvinfo = mbm_get_drvinfo,
	.get_link = usbnet_get_link,
	.get_msglevel = usbnet_get_msglevel,
	.set_msglevel = usbnet_set_msglevel,
	.get_settings = usbnet_get_settings,
	.set_settings = usbnet_set_settings,
	.nway_reset = usbnet_nway_reset,
};

static int mbm_check_connect(struct usbnet *dev)
{
	struct mbm_data *data = (void *)&dev->data;

	return !data->connect;
}

static int mbm_bind(struct usbnet *dev, struct usb_interface *intf)
{
	struct cdc_state *info = (void *)&dev->data;
	struct usb_driver *driver = driver_of(intf);
	struct usb_interface_descriptor *d = NULL;
	struct usb_cdc_mdlm_desc *desc = NULL;
	struct usb_cdc_mdlm_detail_desc *detail = NULL;
	struct mbm_data *data = NULL;

	u8 *buf = intf->cur_altsetting->extra;
	int len = intf->cur_altsetting->extralen;
	int status;

	memset(info, 0, sizeof(*info));
	info->control = intf;
	while (len > 3) {
		if (buf[1] != USB_DT_CS_INTERFACE)
			goto next_desc;

		switch (buf[2]) {
		case USB_CDC_MDLM_TYPE:
			if (info->header)
				goto bad_desc;

			desc = (void *)buf;

			if (desc->bLength != sizeof(*desc))
				goto bad_desc;

			if (memcmp(&desc->bGUID, mbm_guid, 16))
				goto bad_desc;
			break;
		case USB_CDC_MDLM_DETAIL_TYPE:
			if (detail)
				goto bad_desc;

			detail = (void *)buf;

			if (detail->bGuidDescriptorType == 0) {
			  if (detail->bLength < (sizeof(*detail) + 1))
					goto bad_desc;
			}
			break;
		case USB_CDC_UNION_TYPE:
			if (info->u)
				goto bad_desc;

			info->u = (void *)buf;

			if (info->u->bLength != sizeof(*info->u))
				goto bad_desc;

			info->control = usb_ifnum_to_if(dev->udev,
							info->u->
							bMasterInterface0);
			info->data =
			    usb_ifnum_to_if(dev->udev,
					    info->u->bSlaveInterface0);
			if (!info->control || !info->data) {
				dev_dbg(&intf->dev,
					"master #%u/%p slave #%u/%p\n",
					info->u->bMasterInterface0,
					info->control,
					info->u->bSlaveInterface0, info->data);
				goto bad_desc;
			}

			/* a data interface altsetting does the real i/o */
			d = &info->data->cur_altsetting->desc;
			if (d->bInterfaceClass != USB_CLASS_CDC_DATA)
				goto bad_desc;
			break;
		case USB_CDC_ETHERNET_TYPE:
			if (info->ether)
				goto bad_desc;

			info->ether = (void *)buf;
			if (info->ether->bLength != sizeof(*info->ether))
				goto bad_desc;
			dev->hard_mtu =
			    le16_to_cpu(info->ether->wMaxSegmentSize);
			break;
		}
next_desc:
		len -= buf[0];	/* bLength */
		buf += buf[0];
	}

	if (!desc || !detail) {
		dev_dbg(&intf->dev, "missing cdc mdlm %s%sdescriptor\n",
			desc ? "" : "func ", detail ? "" : "detail ");
		goto bad_desc;
	}

	if (!info->u || (!info->ether)) {
		dev_dbg(&intf->dev, "missing cdc %s%s%sdescriptor\n",
			info->header ? "" : "header ",
			info->u ? "" : "union ", info->ether ? "" : "ether ");
		goto bad_desc;
	}

	status = usb_driver_claim_interface(driver, info->data, dev);
	if (status < 0) {
		dev_dbg(&intf->dev, "Failed claimin interface\n");
		return status;
	}
	status = usbnet_get_endpoints(dev, info->data);
	if (status < 0) {
		dev_dbg(&intf->dev, "Failed get endpoints\n");
		usb_set_intfdata(info->data, NULL);
		usb_driver_release_interface(driver, info->data);
		return status;
	}

	dev->status = NULL;
	if (info->control->cur_altsetting->desc.bNumEndpoints == 1) {
		struct usb_endpoint_descriptor *desc;

		dev->status = &info->control->cur_altsetting->endpoint[0];
		desc = &dev->status->desc;
		if (!usb_endpoint_is_int_in(desc)
		    || (le16_to_cpu(desc->wMaxPacketSize)
			< sizeof(struct usb_cdc_notification))
		    || !desc->bInterval) {
			dev_dbg(&intf->dev, "bad notification endpoint\n");
			dev->status = NULL;
		}
	}
	usb_set_intfdata(intf, data);
	dev->net->ethtool_ops = &mbm_ethtool_ops;

	status = get_ethernet_addr(dev, info->ether);
	if (status < 0) {
		usb_set_intfdata(info->data, NULL);
		usb_driver_release_interface(driver_of(intf), info->data);
		return status;
	}

	return 0;

bad_desc:
	dev_info(&dev->udev->dev, "unsupported MDLM descriptors\n");
	return -ENODEV;
}

static const struct driver_info mbm_info = {
	.description = "Mobile Broadband Network Device",
	.flags = FLAG_MBN,
	.check_connect = mbm_check_connect,
	.bind = mbm_bind,
	.unbind = usbnet_cdc_unbind,
	.status = mbm_status,
};

static const struct usb_device_id products[] = {
	{
	 USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_MDLM,
			    USB_CDC_PROTO_NONE),
	 .driver_info = (unsigned long)&mbm_info,
	 },

	{}, /* END */
};

MODULE_DEVICE_TABLE(usb, products);

int mbm_suspend(struct usb_interface *intf, pm_message_t message)
{
	dev_dbg(&intf->dev, "mbm%d_suspend\n", intf->minor);
	return usbnet_suspend(intf, message);
}

int mbm_resume(struct usb_interface *intf)
{
	dev_dbg(&intf->dev, "mbm%d_resume\n", intf->minor);
	return usbnet_resume(intf);
}

static struct usb_driver usbmbm_driver = {
	.name = "mbm",
	.id_table = products,
	.probe = usbnet_probe,
	.disconnect = usbnet_disconnect,
	.suspend = mbm_suspend,
	.resume = mbm_resume,
	.supports_autosuspend = 1,
};

static int __init usbmbm_init(void)
{
	return usb_register(&usbmbm_driver);
}

module_init(usbmbm_init);

static void __exit usbmbm_exit(void)
{
	usb_deregister(&usbmbm_driver);
}

module_exit(usbmbm_exit);

MODULE_AUTHOR("Carl Nordbeck");
MODULE_DESCRIPTION("Ericsson Mobile Broadband");
MODULE_LICENSE("GPL");
