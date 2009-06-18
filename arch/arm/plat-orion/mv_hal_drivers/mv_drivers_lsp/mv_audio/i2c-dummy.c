/*
 *
 * This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include "mvOs.h"
#include "mvCommon.h"
#include "twsi/mvTwsi.h"

struct i2c_adapter      *i2s_i2c_adapter = NULL;
struct i2c_client *i2s_i2c_client = NULL;

static const struct i2c_device_id i2s_i2c_id[] = {
     { "i2s_i2c", 0 },
     { }
};

MV_STATUS mvTwsiWrapper (struct i2c_client *i2c_client, int read, 
			 MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *pBlock, MV_U32 blockSize)
{
	int ret;
	
	if(!i2c_client) {
		pr_err("in %s: i2c_client not initiaized\n", __func__);
		return MV_ERROR;
	}

	if(MV_TRUE == pTwsiSlave->validOffset){
		pr_debug("%s:  block. %s offset 0x%x data[0] 0x%x blockSize %d\n",
			 __func__, read ? "read" : "write", pTwsiSlave->offset, 
		       pBlock[0], blockSize);
		if(read){
			ret = i2c_smbus_read_i2c_block_data(i2s_i2c_client,
							    pTwsiSlave->offset,
							    blockSize,
							    pBlock);
			if (ret < blockSize) {
				pr_err( "%s error %d\n", __func__, ret);
				return MV_ERROR;
			}
		}else{
			ret = i2c_smbus_write_i2c_block_data(i2s_i2c_client,
							     pTwsiSlave->offset,
							     blockSize,
							     pBlock);
			if (ret) {
				pr_err( "%s block error %d\n", __func__, ret);
				return MV_ERROR;
			}
		}

	}else{
		struct i2c_msg		msg;
		
		memset(&msg, 0, sizeof(msg));
		msg.addr = pTwsiSlave->slaveAddr.address;
		if(read)
			msg.flags = I2C_M_RD;

		if(pTwsiSlave->slaveAddr.type == ADDR10_BIT) 
		{
			msg.flags |= I2C_M_TEN;
		}
		msg.len = blockSize;
		msg.buf = pBlock;

		ret = i2c_transfer(i2s_i2c_adapter, &msg, 1);
		if (ret != 1) {
			pr_err( "%s error %d\n", __func__, ret);
			return MV_ERROR;
			
		}
	}

	return MV_OK;
}

MV_STATUS mvTwsiRead (MV_U8 chanNum, MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *pBlock, MV_U32 blockSize)
{
	return mvTwsiWrapper(i2s_i2c_client, 1, pTwsiSlave, pBlock, blockSize);
}
	
MV_STATUS mvTwsiWrite(MV_U8 chanNum, MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *pBlock, MV_U32 blockSize)
{
	return mvTwsiWrapper(i2s_i2c_client, 0, pTwsiSlave, pBlock, blockSize);
}

static int __devinit i2s_i2c_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
     i2s_i2c_adapter = to_i2c_adapter(client->dev.parent);
     i2s_i2c_client = client;

     if (!i2c_check_functionality(i2s_i2c_adapter,
				  I2C_FUNC_SMBUS_EMUL))
	  return -EIO;
     printk("I2S I2C driver loaded successfully\n");
     return 0;
}
static int __devexit i2s_i2c_remove(struct i2c_client *client)
{
     return 0;
}


static struct i2c_driver i2s_i2c_driver = {
     .driver = {
	  .name   = "i2s_i2c",
	  .owner  = THIS_MODULE,
     },
     .probe          = i2s_i2c_probe,
     .remove         = __devexit_p(i2s_i2c_remove),
     .id_table       = i2s_i2c_id,
};

static int __init i2s_i2c_init(void)
{
     return i2c_add_driver(&i2s_i2c_driver);
}
module_init(i2s_i2c_init);

static void __exit i2s_i2c_exit(void)
{
     i2c_del_driver(&i2s_i2c_driver);
}
module_exit(i2s_i2c_exit);

MODULE_AUTHOR("Saeed Bishara <saeed@marvell.com>");
MODULE_DESCRIPTION("I2C wrapper client driver for I2C HAL function Driver");
MODULE_LICENSE("GPL");
