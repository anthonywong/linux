/* drivers/usb/gadget/f_diag.c
 *Diag Function Device - Route ARM9 and ARM11 DIAG messages
 *between HOST and DEVICE.
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <mach/rpc_hsusb.h>
#include <mach/usbdiag.h>


static struct usb_interface_descriptor intf_desc = {
	.bLength            =	sizeof intf_desc,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bNumEndpoints      =	2,
	.bInterfaceClass    =	0xFF,
	.bInterfaceSubClass =	0xFF,
	.bInterfaceProtocol =	0xFF,
};

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength 			=	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType 	=	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes 		=	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize 	=	__constant_cpu_to_le16(512),
	.bInterval 			=	0,
};
static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength          =	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes     =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = __constant_cpu_to_le16(64),
	.bInterval        =	0,
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength          =	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes     =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = __constant_cpu_to_le16(512),
	.bInterval        =	0,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength          =	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes     =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = __constant_cpu_to_le16(64),
	.bInterval        =	0,
};

static struct usb_descriptor_header *fs_diag_desc[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fs_bulk_out_desc,
	NULL,
	};
static struct usb_descriptor_header *hs_diag_desc[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &hs_bulk_in_desc,
	(struct usb_descriptor_header *) &hs_bulk_out_desc,
	NULL,
};

struct diag_context {
	struct usb_composite_dev *cdev;
	struct usb_function function;
	struct usb_ep *out;
	struct usb_ep *in;
	struct usb_endpoint_descriptor  *in_desc;
	struct usb_endpoint_descriptor  *out_desc;
	spinlock_t lock;
	/* linked list of write requets*/
	struct list_head write_req_list;
	struct usb_request *read_req;

	struct diag_operations *operations;
	struct work_struct diag_work;
	unsigned diag_configured;
	unsigned diag_opened;
	unsigned char i_serial_number;
	char *serial_number;
	unsigned short  product_id;
};

static struct diag_context _context;

static void diag_write_complete(struct usb_ep *ep ,
		struct usb_request *req)
{
	struct diag_context *ctxt = &_context;
	struct diag_request *d_req = req->context;
	struct usb_composite_dev *cdev = ctxt->cdev;
	int status = req->status;
	unsigned len = req->length;
	unsigned long flags;

	switch (status) {
	case 0:
		/* normal completion */
		if ((len >= ep->maxpacket) && ((len % ep->maxpacket) == 0)) {
			d_req->actual = req->actual;
			d_req->status = req->status;
			/* Queue zero length packet */
			req->length = 0;
			usb_ep_queue(ep, req, GFP_ATOMIC);
			return;
		}
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		/* connection gone */
		return;
	default:
		/* unexpected failure */
		ERROR(cdev, "diag %s write error %d, %d/%d\n",
			ep->name, status,
			req->actual, req->length);
		return;
	}

	if (len != 0) {
		d_req->actual = req->actual;
		d_req->status = req->status;
	}
	spin_lock_irqsave(&ctxt->lock, flags);
	list_add_tail(&req->list, &ctxt->write_req_list);
	spin_unlock_irqrestore(&ctxt->lock , flags);
	ctxt->operations->diag_char_write_complete(d_req);
}

static void diag_read_complete(struct usb_ep *ep ,
		struct usb_request *req)
{
	struct diag_context *ctxt = &_context;
	struct diag_request *d_req = req->context;
	struct usb_composite_dev *cdev = ctxt->cdev;
	int status = req->status;

	switch (status) {
	case 0:
		/* normal completion */
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		/* connection gone */
		return;
	default:
		/* unexpected failure */
		ERROR(cdev, "diag %s read error %d, %d/%d\n",
			ep->name, status,
			req->actual, req->length);
		return;
	}

	d_req->actual = req->actual;
	d_req->status = req->status;
	ctxt->operations->diag_char_read_complete(d_req);
}

int diag_read(struct diag_request *d_req)
{
	struct diag_context *ctxt = &_context;
	struct usb_composite_dev *cdev = ctxt->cdev;
	struct usb_request *req = ctxt->read_req;
	int ret;

	if (!ctxt->diag_opened || !ctxt->diag_configured)
		return -EIO;

	req->context = d_req;
	req->buf = d_req->buf;
	req->length = d_req->length;
	ret = usb_ep_queue(ctxt->out, req, GFP_ATOMIC);
	if (ret)
		ERROR(cdev, "diag %s enqueue error%d, %d/%d\n",
			ctxt->out->name, ret,
			req->actual, req->length);
	return ret;

}
EXPORT_SYMBOL(diag_read);

int diag_write(struct diag_request *d_req)
{
	struct diag_context *ctxt = &_context;
	struct usb_composite_dev *cdev = ctxt->cdev;
	struct usb_request *req;
	unsigned long flags;
	int ret;

	if (!ctxt->diag_opened || !ctxt->diag_configured)
		return -EIO;

	spin_lock_irqsave(&ctxt->lock , flags);
	if (list_empty(&ctxt->write_req_list)) {
		spin_unlock_irqrestore(&ctxt->lock, flags);
		return -EAGAIN;
	}
	req = list_first_entry(&ctxt->write_req_list, struct usb_request, list);
	list_del(&req->list);
	spin_unlock_irqrestore(&ctxt->lock, flags);

	req->buf = d_req->buf;
	req->length = d_req->length;
	req->context = d_req;

	ret = usb_ep_queue(ctxt->in, req, GFP_ATOMIC);
	if (ret) {
		ERROR(cdev, "diag %s enqueue error%d, %d/%d\n",
			ctxt->in->name, ret,
			req->actual, req->length);
		/* If error add the link to linked list again*/
		spin_lock_irqsave(&ctxt->lock, flags);
		list_add_tail(&req->list, &ctxt->write_req_list);
		spin_unlock_irqrestore(&ctxt->lock, flags);
	}
	return ret;
}
EXPORT_SYMBOL(diag_write);

int diag_open(int num_write_req)
{
	struct diag_context *ctxt = &_context;
	struct usb_request *req;
	int i;

	for (i = 0; i < num_write_req; i++) {
		req = usb_ep_alloc_request(ctxt->in, GFP_KERNEL);
		if (!req)
			goto write_req_fail;
		req->complete = diag_write_complete;
		list_add_tail(&req->list, &ctxt->write_req_list);
	}

	req = usb_ep_alloc_request(ctxt->out, GFP_KERNEL);
	if (!req)
		goto write_req_fail;
	req->complete = diag_read_complete;
	ctxt->read_req = req;

	ctxt->diag_opened = 1;
	return 0;

write_req_fail:
	while (!list_empty(&ctxt->write_req_list)) {
		req = list_first_entry(&ctxt->write_req_list,
				struct usb_request, list);
		list_del(&req->list);
		usb_ep_free_request(ctxt->in, req);
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(diag_open);

void diag_close(void)
{
	struct diag_context *ctxt = &_context;
	struct usb_request *req;

	while (!list_empty(&ctxt->write_req_list)) {
		req = list_first_entry(&ctxt->write_req_list,
				struct usb_request, list);
		list_del(&req->list);
		usb_ep_free_request(ctxt->in, req);
	}

	usb_ep_free_request(ctxt->out, ctxt->read_req);

	ctxt->diag_opened = 0;
}
EXPORT_SYMBOL(diag_close);

int diag_usb_register(struct diag_operations *func)
{
	struct diag_context *ctxt = &_context;

	/* check if all operations present or not and return error */

	ctxt->operations = func;
	if (ctxt->diag_configured == 1)
		if ((ctxt->operations) &&
			(ctxt->operations->diag_connect))
				ctxt->operations->diag_connect();

	return 0;
}
EXPORT_SYMBOL(diag_usb_register);

int diag_usb_unregister(void)
{
	struct diag_context *ctxt = &_context;

	ctxt->operations = NULL;
	return 0;
}
EXPORT_SYMBOL(diag_usb_unregister);

static void diag_function_disable(struct usb_function *f)
{
	struct diag_context  *dev = &_context;

	dev->diag_configured = 0;

	usb_ep_fifo_flush(dev->in);
	usb_ep_disable(dev->in);

	usb_ep_fifo_flush(dev->out);
	usb_ep_disable(dev->out);

	dev->operations->diag_disconnect();
}

static int diag_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct diag_context  *dev = &_context;
	struct usb_composite_dev *cdev = f->config->cdev;

	dev->in_desc = ep_choose(cdev->gadget,
			&hs_bulk_in_desc, &fs_bulk_in_desc);
	dev->out_desc = ep_choose(cdev->gadget,
			&hs_bulk_out_desc, &fs_bulk_in_desc);

	usb_ep_enable(dev->in, dev->in_desc);
	usb_ep_enable(dev->out, dev->out_desc);
	dev->diag_configured = 1;
	schedule_work(&dev->diag_work);

	return 0;
}

static void diag_function_unbind(struct usb_configuration *c,
		struct usb_function *f)
{
	struct diag_context *ctxt = &_context;

	usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->descriptors);
	cancel_work_sync(&ctxt->diag_work);
	ctxt->out->driver_data = NULL; /* release endpoint */
	ctxt->in->driver_data = NULL; /* release endpoint */
}

static int diag_function_bind(struct usb_configuration *c,
		struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct diag_context *ctxt = &_context;
	struct usb_ep      *ep;

	intf_desc.bInterfaceNumber =  usb_interface_id(c, f);

	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_in_desc);
	if (!ep)
		goto fail;
	ep->driver_data = cdev; /* claim endpoint */
	ctxt->in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_out_desc);
	if (!ep)
		goto fail;
	ep->driver_data = cdev;
	ctxt->out = ep; /* claim endpoint */

	/* copy descriptors, and track endpoint copies */
	f->descriptors = usb_copy_descriptors(fs_diag_desc);
	if (!f->descriptors)
		goto fail;

	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_bulk_in_desc.bEndpointAddress =
				fs_bulk_in_desc.bEndpointAddress;
		hs_bulk_out_desc.bEndpointAddress =
				fs_bulk_out_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(hs_diag_desc);
		if (!f->hs_descriptors)
			goto fail;
	}

	ctxt->i_serial_number = cdev->driver->dev->iSerialNumber;
	ctxt->product_id   = cdev->driver->dev->idProduct;
	/*send serial number to A9 sw download, only if serial_number
	* is not null and i_serial_number is non-zero
	*/
	if (ctxt->serial_number && ctxt->i_serial_number) {
		msm_hsusb_is_serial_num_null(0);
		msm_hsusb_send_serial_number(ctxt->serial_number);
	} else
		msm_hsusb_is_serial_num_null(1);
	/* Send product ID to A9 for software download*/
	if (ctxt->product_id)
		msm_hsusb_send_productID(ctxt->product_id);

	return 0;
fail:
	usb_free_descriptors(f->descriptors);
	usb_free_descriptors(f->hs_descriptors);
	if (ctxt->out)
		ctxt->out->driver_data = NULL; /* release endpoint */
	if (ctxt->in)
		ctxt->in->driver_data = NULL; /* release endpoint */

	ERROR(cdev, "diag bind failed\n");
	return -ENODEV;

}
static void usb_config_work_func(struct work_struct *work)
{
	struct diag_context *ctxt = &_context;
	if ((ctxt->operations) &&
		(ctxt->operations->diag_connect))
			ctxt->operations->diag_connect();
}

int diag_function_add(struct usb_configuration *c,
				char *serial_number)
{
	struct diag_context *context = &_context;
	struct usb_composite_dev *cdev = c->cdev;
	int ret;

	DBG(cdev, "diag_function_add\n");

	spin_lock_init(&context->lock);
	INIT_LIST_HEAD(&context->write_req_list);
	context->cdev = cdev;
	context->function.name = "diag";
	context->function.bind = diag_function_bind;
	context->function.unbind = diag_function_unbind;
	context->function.set_alt = diag_function_set_alt;
	context->function.disable = diag_function_disable;
	context->serial_number	= serial_number;
	INIT_WORK(&context->diag_work, usb_config_work_func);

	ret = usb_add_function(c, &context->function);
	if (ret)
		ERROR(cdev, "diag_function_add failed\n");

	return ret;
}
