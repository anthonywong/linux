/*
    i2c-sch.c - Part of lm_sensors, Linux kernel modules for hardware
              monitoring
    Based on piix4.c
    Copyright (c) 1998 - 2002 Frodo Looijaard <frodol@dds.nl> and
    Philip Edelbrock <phil@netroedge.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
   Supports:
	Intel POULSBO

   Note: we assume there can only be one device, with one SMBus interface.
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/apm_bios.h>
#include <asm/io.h>


struct sd {
	const unsigned short mfr;
	const unsigned short dev;
	const unsigned char fn;
	const char *name;
};
/* POULSBO SMBus address offsets */
#define SMBHSTCNT	(0 + poulsbo_smba)
#define SMBHSTSTS	(1 + poulsbo_smba)
#define SMBHSTADD	(4 + poulsbo_smba) /* TSA */
#define SMBHSTCMD	(5 + poulsbo_smba)
#define SMBHSTDAT0	(6 + poulsbo_smba)
#define SMBHSTDAT1	(7 + poulsbo_smba)
#define SMBBLKDAT	(0x20 + poulsbo_smba)


/* count for request_region */
#define SMBIOSIZE	8

/* PCI Address Constants */
#define SMBBA_SCH	0x040

/* Other settings */
#define MAX_TIMEOUT	500
#define  ENABLE_INT9	0

/* POULSBO constants */
#define POULSBO_QUICK		0x00
#define POULSBO_BYTE		0x01
#define POULSBO_BYTE_DATA		0x02
#define POULSBO_WORD_DATA		0x03
#define POULSBO_BLOCK_DATA	0x05

/* insmod parameters */

/* If force is set to anything different from 0, we forcibly enable the
   POULSBO. DANGEROUS! */
static int force;
module_param (force, int, 0);
MODULE_PARM_DESC(force, "Forcibly enable the POULSBO. DANGEROUS!");


static int poulsbo_transaction(void);

static unsigned short poulsbo_smba;
static struct pci_driver poulsbo_driver;
static struct i2c_adapter poulsbo_adapter;


static int __devinit poulsbo_setup(struct pci_dev *POULSBO_dev,
				const struct pci_device_id *id)
{
	unsigned short smbase;
	if(POULSBO_dev->device != PCI_DEVICE_ID_INTEL_POULSBO_LPC) {
		/* match up the function */
		if (PCI_FUNC(POULSBO_dev->devfn) != id->driver_data)
			return -ENODEV;
		dev_info(&POULSBO_dev->dev, "Found %s device\n", pci_name(POULSBO_dev));
	} else {
		dev_info(&POULSBO_dev->dev, "Found POULSBO SMBUS %s device\n", pci_name(POULSBO_dev));
		/* find SMBUS base address */
		pci_read_config_word(POULSBO_dev, 0x40, &smbase);
		dev_info(&POULSBO_dev->dev, "POULSBO SM base = 0x%04x\n", smbase);
	}


	/* Determine the address of the SMBus areas */
		if(POULSBO_dev->device == PCI_DEVICE_ID_INTEL_POULSBO_LPC)
			pci_read_config_word(POULSBO_dev, SMBBA_SCH, &poulsbo_smba);
		else
		        poulsbo_smba=0;

		poulsbo_smba &= 0xfff0;
		if(poulsbo_smba == 0) {
			dev_err(&POULSBO_dev->dev, "SMB base address "
				"uninitialized - upgrade BIOS or use "
				"force_addr=0xaddr\n");
			return -ENODEV;
		}

	if (!request_region(poulsbo_smba, SMBIOSIZE, poulsbo_driver.name)) {
		dev_err(&POULSBO_dev->dev, "SMB region 0x%x already in use!\n",
			poulsbo_smba);
		return -ENODEV;
	}

	dev_dbg(&POULSBO_dev->dev, "SMBA = 0x%X\n", poulsbo_smba);

	return 0;
}

/* Another internally used function */
static int poulsbo_transaction(void)
{
	int temp;
	int result = 0;
	int timeout = 0;

	dev_dbg(&poulsbo_adapter.dev, "Transaction (pre): CNT=%02x, CMD=%02x, "
		"ADD=%02x, DAT0=%02x, DAT1=%02x\n", inb_p(SMBHSTCNT),
		inb_p(SMBHSTCMD), inb_p(SMBHSTADD), inb_p(SMBHSTDAT0),
		inb_p(SMBHSTDAT1));

	/* Make sure the SMBus host is ready to start transmitting */
	if ((temp = inb_p(SMBHSTSTS)) != 0x00) {
	  if(temp == 1) {
		dev_dbg(&poulsbo_adapter.dev, "Completion (%02x). "
			"clear...\n", temp);
		outb_p(temp, SMBHSTSTS);

	  } else  if(temp & 0xe) {
		dev_dbg(&poulsbo_adapter.dev, "SMBus error (%02x). "
			"Resetting...\n", temp);
		outb_p(temp, SMBHSTSTS);
	  }
	  if ((temp = inb_p(SMBHSTSTS)) != 0x00) {
	    dev_err(&poulsbo_adapter.dev, "Failed! (%02x)\n", temp);
	    return -1;
	  } else {
	    dev_dbg(&poulsbo_adapter.dev, "Successfull!\n");
	  }
	}

	/* start the transaction by setting bit 4 */
	outb_p(inb(SMBHSTCNT) | 0x10, SMBHSTCNT);

	/* We will always wait for a fraction of a second! (See POULSBO docs errata) */
	do {
		msleep(1);
		temp = inb_p(SMBHSTSTS);
	} while ((temp & 0x08) && (timeout++ < MAX_TIMEOUT));

	/* If the SMBus is still busy, we give up */
	if (timeout >= MAX_TIMEOUT) {
		dev_err(&poulsbo_adapter.dev, "SMBus Timeout!\n");
		result = -1;
	}

	if (temp & 0x10) {
		result = -1;
		dev_err(&poulsbo_adapter.dev, "Error: Failed bus transaction\n");
	}

	if (temp & 0x08) {
		result = -1;
		dev_dbg(&poulsbo_adapter.dev, "Bus collision! SMBus may be "
			"locked until next hard reset. (sorry!)\n");
		/* Clock stops and slave is stuck in mid-transmission */
	}

	if (temp & 0x04) {
		result = -1;
		dev_dbg(&poulsbo_adapter.dev, "Error: no response!\n");
	}
	if ((temp = inb_p(SMBHSTSTS)) != 0x00) {
	  if( temp == 0x1) {
	    dev_dbg(&poulsbo_adapter.dev, "post complete!\n");
	    outb_p(temp, SMBHSTSTS);
	}
	  else if(temp & 0xe) {
	    dev_dbg(&poulsbo_adapter.dev, "Error: bus, etc!\n");
	    outb_p(inb(SMBHSTSTS), SMBHSTSTS);
	  }
	}
	  msleep(1);
	if ((temp = inb_p(SMBHSTSTS)) & 0xe) {
	  /* BSY, device or bus error */
		dev_err(&poulsbo_adapter.dev, "Failed reset at end of "
			"transaction (%02x), Bus error\n", temp);
	}
	dev_dbg(&poulsbo_adapter.dev, "Transaction (post): CNT=%02x, CMD=%02x, "
		"ADD=%02x, DAT0=%02x, DAT1=%02x\n", inb_p(SMBHSTCNT),
		inb_p(SMBHSTCMD), inb_p(SMBHSTADD), inb_p(SMBHSTDAT0),
		inb_p(SMBHSTDAT1));
	return result;
}

/* Return -1 on error. */
static s32 poulsbo_access(struct i2c_adapter * adap, u16 addr,
		 unsigned short flags, char read_write,
		 u8 command, int size, union i2c_smbus_data * data)
{
	int i, len;
	dev_dbg(&poulsbo_adapter.dev,"access size: %d %s\n", size, (read_write)?"READ":"WRITE");
	switch (size) {
	case I2C_SMBUS_PROC_CALL:
		dev_err(&adap->dev, "I2C_SMBUS_PROC_CALL not supported!\n");
		return -1;
	case I2C_SMBUS_QUICK:
		outb_p(((addr & 0x7f) << 1) | (read_write & 0x01),
		       SMBHSTADD);
		size = POULSBO_QUICK;
		break;
	case I2C_SMBUS_BYTE:
		outb_p(((addr & 0x7f) << 1) | (read_write & 0x01),
		       SMBHSTADD);
		if (read_write == I2C_SMBUS_WRITE)
			outb_p(command, SMBHSTCMD);
		size = POULSBO_BYTE;
		break;
	case I2C_SMBUS_BYTE_DATA:
		outb_p(((addr & 0x7f) << 1) | (read_write & 0x01),
		       SMBHSTADD);
		outb_p(command, SMBHSTCMD);
		if (read_write == I2C_SMBUS_WRITE)
			outb_p(data->byte, SMBHSTDAT0);
		size = POULSBO_BYTE_DATA;
		break;
	case I2C_SMBUS_WORD_DATA:
		outb_p(((addr & 0x7f) << 1) | (read_write & 0x01),
		       SMBHSTADD);
		outb_p(command, SMBHSTCMD);
		if (read_write == I2C_SMBUS_WRITE) {
			outb_p(data->word & 0xff, SMBHSTDAT0);
			outb_p((data->word & 0xff00) >> 8, SMBHSTDAT1);
		}
		size = POULSBO_WORD_DATA;
		break;
	case I2C_SMBUS_BLOCK_DATA:
		outb_p(((addr & 0x7f) << 1) | (read_write & 0x01),
		       SMBHSTADD);
		outb_p(command, SMBHSTCMD);
		if (read_write == I2C_SMBUS_WRITE) {
			len = data->block[0];
			if (len < 0)
				len = 0;
			if (len > 32)
				len = 32;
			outb_p(len, SMBHSTDAT0);
			i = inb_p(SMBHSTCNT);	/* Reset SMBBLKDAT */
			for (i = 1; i <= len; i++)
				outb_p(data->block[i], SMBBLKDAT);
		}
		size = POULSBO_BLOCK_DATA;
		break;
	}
	dev_dbg(&poulsbo_adapter.dev,"write size %d to 0x%04x\n", size, SMBHSTCNT);
	outb_p((size & 0x7), SMBHSTCNT);

	if (poulsbo_transaction())	/* Error in transaction */
		return -1;

	if ((read_write == I2C_SMBUS_WRITE) || (size == POULSBO_QUICK))
		return 0;


	switch (size) {
	case POULSBO_BYTE:	/* Where is the result put? I assume here it is in
				   SMBHSTDAT0 but it might just as well be in the
				   SMBHSTCMD. No clue in the docs */

		data->byte = inb_p(SMBHSTDAT0);
		break;
	case POULSBO_BYTE_DATA:
		data->byte = inb_p(SMBHSTDAT0);
		break;
	case POULSBO_WORD_DATA:
		data->word = inb_p(SMBHSTDAT0) + (inb_p(SMBHSTDAT1) << 8);
		break;
	case POULSBO_BLOCK_DATA:
		data->block[0] = inb_p(SMBHSTDAT0);
		i = inb_p(SMBHSTCNT);	/* Reset SMBBLKDAT */
		for (i = 1; i <= data->block[0]; i++)
			data->block[i] = inb_p(SMBBLKDAT);
		break;
	}
	return 0;
}

static u32 poulsbo_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE |
	    I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA |
	    I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm smbus_algorithm = {
	.smbus_xfer	= poulsbo_access,
	.functionality	= poulsbo_func,
};

static struct i2c_adapter poulsbo_adapter = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON,
	.algo		= &smbus_algorithm,
};

static struct pci_device_id poulsbo_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_POULSBO_LPC),
	  .driver_data = 0xf8 },
	{ 0, }
};

MODULE_DEVICE_TABLE (pci, poulsbo_ids);

static int __devinit poulsbo_probe(struct pci_dev *dev,
				const struct pci_device_id *id)
{
	int retval;
	retval = poulsbo_setup(dev, id);
	if (retval)
		return retval;

	/* set up the driverfs linkage to our parent device */
	poulsbo_adapter.dev.parent = &dev->dev;

	snprintf(poulsbo_adapter.name, I2C_NAME_SIZE,
		"SMBus POULSBO adapter at %04x", poulsbo_smba);

	if ((retval = i2c_add_adapter(&poulsbo_adapter))) {
		dev_err(&dev->dev, "Couldn't register adapter!\n");
		release_region(poulsbo_smba, SMBIOSIZE);
		poulsbo_smba = 0;
	}

	return retval;
}

static void __devexit poulsbo_remove(struct pci_dev *dev)
{
	if (poulsbo_smba) {
		i2c_del_adapter(&poulsbo_adapter);
		release_region(poulsbo_smba, SMBIOSIZE);
		poulsbo_smba = 0;
	}
}

static struct pci_driver poulsbo_driver = {
	.name		= "poulsbo_smbus",
	.id_table	= poulsbo_ids,
	.probe		= poulsbo_probe,
	.remove		= __devexit_p(poulsbo_remove),
};

static int __init i2c_poulsbo_init(void)
{
	return pci_register_driver(&poulsbo_driver);
}

static void __exit i2c_poulsbo_exit(void)
{
	pci_unregister_driver(&poulsbo_driver);
}

MODULE_AUTHOR("Jacob Pan <jacob.jun.pan@intel.com> ");
MODULE_DESCRIPTION("POULSBO SMBus driver");
MODULE_LICENSE("GPL");

module_init(i2c_poulsbo_init);
module_exit(i2c_poulsbo_exit);
