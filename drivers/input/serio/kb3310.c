/*
 * linux/drivers/input/serio/kb3310.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>

#include <linux/i2c/kb3310.h>


/* #define KBC_DEBUG */

#define KB3310_MAX_DATA		2

#define KB3310_RESET_CMD	0x99

/*
 * KB3310 data read byte 0 fields
 */
#define KB3310_READ_TOGGLE	0x01
#define KB3310_READ_DATA	0x02

struct kb3310_data {
	struct mutex		lock;
	struct i2c_client	*client;
	struct work_struct	work;
	unsigned int		pm_suspend;
	unsigned int		enabled_ifaces;
	struct serio		*keybd_io;
	struct serio		*mouse_io;
	int			ref_count;
	unsigned int		num_of_retries;
	unsigned int		retry_delay;
	unsigned char		gpio_devid;
	unsigned char		keybd_devid;
	unsigned char		mouse_devid;
	unsigned char		smbus_devid;
	int			toggle;
};

static struct workqueue_struct	*kb3310_wq;

/*
 * To write, we just access the chip's address in write mode, and dump the
 * command and data out on the bus.  The data are taken as sequential u8s,
 * to a maximum of KB3310_MAX_DATA.
 */
static int ene_i2c_write(struct kb3310_data *kb, u8 cmd, u8 *buf, int len)
{
	int i, ret = 0;
	u8 data[KB3310_MAX_DATA+1];

	if (unlikely(len > KB3310_MAX_DATA)) {
		dev_err(&kb->client->dev, "tried to send %d bytes\n", len);
		return 0;
	}

	data[0] = cmd;
	for (i = 0; i < len; i++)
		data[i+1] = buf[i];
	for ( ; i < KB3310_MAX_DATA; i++)
		data[i+1] = 0;

	mutex_lock(&kb->lock);

	/*
	 * If the EC is busy (pulling battery status on another i2c)
	 * while we send the data, we can get a timeout,
	 * so try a few times with a delay.
	 */
	for (i = kb->num_of_retries; i > 0; i--) {
		ret = i2c_master_send(kb->client, data, KB3310_MAX_DATA+1);
		if (unlikely(ret == -ETIMEDOUT)) {
			mdelay(kb->retry_delay);
			continue;
		}
		if (ret > 0)
			break;
	}

	mutex_unlock(&kb->lock);

	if (unlikely(ret != (KB3310_MAX_DATA+1)))
		dev_err(&kb->client->dev, "sent %d bytes of %d total\n",
			(KB3310_MAX_DATA+1), ret);

	return len;
}

/*
 * To read, we first send the command byte to the chip and end the transaction,
 * then access the chip in read mode, at which point it will send the data.
 */
static int ene_i2c_read(struct kb3310_data *kb, u8 cmd, u8 *buf, int len)
{
	int i, ret = 0;

	mutex_lock(&kb->lock);

	/*
	 * If the EC is busy (pulling battery status on another i2c)
	 * while we send the data, we can get a timeout,
	 * so try a few times with a delay.
	 */
	for (i = kb->num_of_retries; i > 0; i--) {
		ret = i2c_master_send(kb->client, &cmd, 1);
		if (unlikely(ret == -ETIMEDOUT)) {
			mdelay(kb->retry_delay);
			continue;
		}
		if (ret > 0)
			break;
	}

	if (unlikely(ret != 1))
		dev_err(&kb->client->dev, "sending read cmd 0x%02x failed\n",
			cmd);

	ret = i2c_master_recv(kb->client, buf, len);

	mutex_unlock(&kb->lock);

	if (unlikely(ret != len))
		dev_err(&kb->client->dev, "wanted %d bytes, got %d\n",
			len, ret);

#ifdef KBC_DEBUG
	printk("R:%02x %02x\n", buf[0], buf[1]);
#endif

	return ret;
}

static int kb3310_write(struct serio *io, unsigned char val)
{
	struct kb3310_data *kb = (struct kb3310_data *)io->port_data;
	u8 cmd, buf[KB3310_MAX_DATA];
	int ret;

	if (io == kb->keybd_io)
		cmd = kb->keybd_devid;
	else if (io == kb->mouse_io)
		cmd = kb->mouse_devid;
	else {
		dev_err(&kb->client->dev,
			"write reguest from unregistered interface\n");
		return 0;
	}

	buf[0] = val;
	buf[1] = 0;

#ifdef KBC_DEBUG
	printk("W:%02x %02x %02x\n", cmd, buf[0], buf[1]);
#endif

	ret = ene_i2c_write(kb, cmd, buf, KB3310_MAX_DATA);

	return (ret == KB3310_MAX_DATA) ? 0 : SERIO_TIMEOUT;
}

/*
 * Bottom half: handle the interrupt by posting key events, or dealing with
 * errors appropriately.
 */
static void kb3310_work(struct work_struct *work)
{
	struct kb3310_data *kb = container_of(work, struct kb3310_data, work);
	u8 cmd = 0;
	u8 buf[KB3310_MAX_DATA];
	int ret, toggle;

	ret = ene_i2c_read(kb, cmd, buf, KB3310_MAX_DATA);

	toggle = (int)(buf[0] & KB3310_READ_TOGGLE);

	if (unlikely((kb->toggle ^ toggle) == 0)) {
		dev_warn(&kb->client->dev, "invalid toggle bit\n");
	} else {
		if ((kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD) &&
			((buf[0] & kb->keybd_devid) == kb->keybd_devid)) {
			dev_dbg(&kb->client->dev, "keybd: %s %x\n",
				(buf[0] & KB3310_READ_DATA) ? "data" : "rsp",
				(unsigned int)buf[1]);
			serio_interrupt(kb->keybd_io, buf[1], 0);
		}

		if ((kb->enabled_ifaces & KB3310_USE_PS2_MOUSE) &&
			((buf[0] & kb->mouse_devid) == kb->mouse_devid)) {
			dev_dbg(&kb->client->dev, "mouse: %s %x\n",
				(buf[0] & KB3310_READ_DATA) ? "data" : "rsp",
				(unsigned int)buf[1]);
			serio_interrupt(kb->mouse_io, buf[1], 0);
		}

		kb->toggle = toggle;
	}

	enable_irq(kb->client->irq);
}

/*
 * We cannot use I2C in interrupt context, so we just schedule work.
 */
static irqreturn_t kb3310_irq(int irq, void *data)
{
	struct kb3310_data *kb = (struct kb3310_data *)data;

	disable_irq_nosync(irq);
	queue_work(kb3310_wq, &kb->work);

#ifdef KBC_DEBUG
	printk("int\n");
#endif

	return IRQ_HANDLED;
}

/*
 * Read the chip ID.
 */
static int kb3310_read_id(struct kb3310_data *kb, u8 *buf)
{
	return 0;
}

static int kb3310_open(struct serio *io)
{
	struct kb3310_data *kb = (struct kb3310_data *)io->port_data;
	int ret;

#ifdef KBC_DEBUG
	printk("kb3310_open: io=%08x, ref=%d\n", (u32)io, kb->ref_count);
#endif
	if ((kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD) ||
		(kb->enabled_ifaces & KB3310_USE_PS2_MOUSE)) {
		if (! kb->ref_count) {
			INIT_WORK(&kb->work, kb3310_work);

			ret = request_irq(kb->client->irq, kb3310_irq, 0,
					"kb3310", (void *)kb);
			if (ret) {
				dev_err(&kb->client->dev,
					"could not get keyboard/mouse IRQ %d\n",
					kb->client->irq);
				return ret;
			}
#if 0
			/* Configure for rising edge trigger */
			set_irq_chip(kb->client->irq, &orion_gpio_irq_chip);
			set_irq_handler(kb->client->irq, handle_edge_irq);
			set_irq_type(kb->client->irq, IRQ_TYPE_EDGE_RISING);
#else
			/* Configure for high level trigger */
			set_irq_type(kb->client->irq, IRQ_TYPE_LEVEL_HIGH);
#endif
		}

		mutex_lock(&kb->lock);
		kb->ref_count++;
		mutex_unlock(&kb->lock);
	}

	return 0;
}

static void kb3310_close(struct serio *io)
{
	struct kb3310_data *kb = (struct kb3310_data *)io->port_data;

#ifdef KBC_DEBUG
	printk("kb3310_close: io=%08x, ref=%d\n", (u32)io, kb->ref_count);
#endif

	if ((kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD) ||
		(kb->enabled_ifaces & KB3310_USE_PS2_MOUSE)) {
		mutex_lock(&kb->lock);
		kb->ref_count--;
		mutex_unlock(&kb->lock);

		if (! kb->ref_count) {
			free_irq(kb->client->irq, (void *)kb);
			cancel_work_sync(&kb->work);
			flush_workqueue(kb3310_wq);
		}
	}
}

static int __devinit kb3310_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct kb3310_data *kb;
	struct kb3310_platform_data *pdata;
	struct serio *keybd_io = NULL, *mouse_io = NULL;
	u8 cmd = 0, buf[KB3310_MAX_DATA];
	u16 data = 0, tmp;
	int i, ret;

	kb = kzalloc(sizeof(struct kb3310_data), GFP_KERNEL);
	if (!kb) {
		ret = -ENOMEM;
		goto release1;
	}

	mutex_init(&kb->lock);

	pdata			= client->dev.platform_data;
	kb->enabled_ifaces	= pdata->enabled_ifaces;
	kb->num_of_retries	= 5;
	kb->retry_delay		= 1;		/* 1 ms */
	kb->gpio_devid		= 0x20;
	kb->keybd_devid		= 0x30;
	kb->mouse_devid		= 0x40;
	kb->smbus_devid		= 0x50;

	i2c_set_clientdata(client, kb);
	kb->client = client;

	if (kb3310_read_id(kb, buf) != 0) {
		dev_err(&client->dev, "device not found\n");
		ret = -ENODEV;
		goto release2;
	}

	buf[0] = buf[1] = 0;

	/*
	 * Drain old data out of device and we do not care whether the data
	 * came from keyboard or mouse.
	 */
	ret = ene_i2c_read(kb, cmd, buf, KB3310_MAX_DATA);
	data = (u16) buf[1];
	data <<= 8;
	data |= (u16) buf[0];

	dev_dbg(&client->dev, "drain data : 0x%04x\n", (unsigned int)data);

	for (i = 32; i > 0; i--) {
		ret = ene_i2c_read(kb, cmd, buf, KB3310_MAX_DATA);
		tmp = (u16) buf[1];
		tmp <<= 8;
		tmp |= (u16) buf[0];

		/* no new data */
		if (tmp == data)
			break;
		else {
			data = tmp;
			dev_dbg(&client->dev, "drain data : 0x%04x\n",
				(unsigned int)data);
		}
	}

	kb->toggle = (int)(data & KB3310_READ_TOGGLE);

	buf[0] = KB3310_RESET_CMD;
	buf[1] = 0;

	if (kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD) {
		keybd_io = kzalloc(sizeof(struct serio), GFP_KERNEL);
		if (!keybd_io) {
			ret = -ENOMEM;
			goto release2;
		}

		cmd = kb->keybd_devid;
		ret = ene_i2c_write(kb, cmd, buf, KB3310_MAX_DATA);
#ifdef KBC_DEBUG
		printk("W:%02x %02x %02x\n", cmd, buf[0], buf[1]);
#endif

		keybd_io->id.type	= SERIO_8042_XL;
		keybd_io->write		= kb3310_write;
		keybd_io->open		= kb3310_open;
		keybd_io->close		= kb3310_close;
		snprintf(keybd_io->name, sizeof(keybd_io->name),
			"%s ps/2 keyboard", client->driver->driver.name);
		snprintf(keybd_io->phys, sizeof(keybd_io->phys),
			"i2c/addr-%x/dev-%x/irq-%x", client->addr,
			kb->keybd_devid, client->irq);
		keybd_io->port_data	= kb;
		keybd_io->dev.parent	= &client->dev;
		kb->keybd_io		= keybd_io;

		serio_register_port(kb->keybd_io);
	}

	if (kb->enabled_ifaces & KB3310_USE_PS2_MOUSE) {
		mouse_io = kzalloc(sizeof(struct serio), GFP_KERNEL);
		if (!mouse_io) {
			ret = -ENOMEM;
			goto release3;
		}

		cmd = kb->mouse_devid;
		ret = ene_i2c_write(kb, cmd, buf, KB3310_MAX_DATA);
#ifdef KBC_DEBUG
		printk("W:%02x %02x %02x\n", cmd, buf[0], buf[1]);
#endif

		mouse_io->id.type	= SERIO_8042;
		mouse_io->write		= kb3310_write;
		mouse_io->open		= kb3310_open;
		mouse_io->close		= kb3310_close;
		snprintf(mouse_io->name, sizeof(mouse_io->name),
			"%s ps/2 mouse", client->driver->driver.name);
		snprintf(mouse_io->phys, sizeof(mouse_io->phys),
			"i2c/addr-%x/dev-%x/irq-%x", client->addr,
			kb->mouse_devid, client->irq);
		mouse_io->port_data	= kb;
		mouse_io->dev.parent	= &client->dev;
		kb->mouse_io		= mouse_io;

		serio_register_port(kb->mouse_io);
	}

	dev_info(&client->dev, "KB3310 PS2 Keyboard/Mouse driver loaded!\n");

	return 0;

 release3:
	kfree(keybd_io);

 release2:
	kfree(kb);

 release1:
	return ret;
}

static int __devexit kb3310_remove(struct i2c_client *client)
{
	struct kb3310_data *kb = i2c_get_clientdata(client);

	if (kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD)
		serio_unregister_port(kb->keybd_io);
	if (kb->enabled_ifaces & KB3310_USE_PS2_MOUSE)
		serio_unregister_port(kb->mouse_io);
	i2c_set_clientdata(client, NULL);
	kfree(kb);

	return 0;
}

#ifdef CONFIG_PM
/*
 * We don't need to explicitly suspend the chip, as it already switches off
 * when there's no activity.
 */
static int kb3310_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct kb3310_data *kb = i2c_get_clientdata(client);

	if ((kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD) ||
		(kb->enabled_ifaces & KB3310_USE_PS2_MOUSE)) {
		set_irq_wake(kb->client->irq, 0);
		disable_irq(kb->client->irq);
	}

	mutex_lock(&kb->lock);
	kb->pm_suspend = 1;
	mutex_unlock(&kb->lock);

	return 0;
}

static int kb3310_resume(struct i2c_client *client)
{
	struct kb3310_data *kb = i2c_get_clientdata(client);

	mutex_lock(&kb->lock);
	kb->pm_suspend = 0;
	mutex_unlock(&kb->lock);

	if ((kb->enabled_ifaces & KB3310_USE_PS2_KEYBOARD) ||
		(kb->enabled_ifaces & KB3310_USE_PS2_MOUSE)) {
		enable_irq(kb->client->irq);
		set_irq_wake(kb->client->irq, 1);
	}

	return 0;
}
#else
#define kb3310_suspend	NULL
#define kb3310_resume	NULL
#endif

static const struct i2c_device_id kb3310_id[] = {
	{ "kb3310", 0 },
	{ }
};

static struct i2c_driver kb3310_i2c_driver = {
	.driver = {
		.name	 = "kb3310",
		.owner  = THIS_MODULE,
 	},
	.probe		= kb3310_probe,
	.remove		= __devexit_p(kb3310_remove),
	.suspend	= kb3310_suspend,
	.resume		= kb3310_resume,
	.id_table	= kb3310_id,
};
MODULE_DEVICE_TABLE(i2c, kb3310_id);

static int __init kb3310_init(void)
{
	kb3310_wq = create_workqueue("kb3310");
	if (!kb3310_wq) {
		printk(KERN_ERR "Creation of kb3310_wq failed\n");
		return -EFAULT;
	}
	return i2c_add_driver(&kb3310_i2c_driver);
}

static void __exit kb3310_exit(void)
{
	i2c_del_driver(&kb3310_i2c_driver);
	destroy_workqueue(kb3310_wq);
}

module_init(kb3310_init);
module_exit(kb3310_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("XXX <xxx@marvell.com>");
MODULE_DESCRIPTION("ENE KB3310 PS/2 keyboard/mouse driver");
