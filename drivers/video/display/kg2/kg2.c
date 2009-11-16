/*
 * kg2.c - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#include <linux/i2c.h>
#include "kg2.h"
#include "scripts/step1_INIT_part1.h"
#include "scripts/step2_INIT_part2.h"
#include "scripts/step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove.h"
#include "scripts/step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG-swap.h"
#include "scripts/step5_Dove_1024x600_1080P.h"
#include "scripts/step6_scaler.h"

#define DBG_I2C_DATA_PER_LINE   16
#define DBG_I2C_LENGTH_PER_DATA 3
#define DBG_I2C_LENGTH_PER_LINE (DBG_I2C_DATA_PER_LINE * DBG_I2C_LENGTH_PER_DATA + 1)
//#define DEBUG		// For detail debug messages, including I2C write operations


static const struct i2c_device_id kg2_i2c_id[] = {
	{"kg2_i2c", 0},
	{}
};

static struct i2c_client * i2c_client_kg2 = NULL;
static struct i2c_driver kg2_driver = {
	.driver = {
		.name  = "kg2_i2c",
		.owner = THIS_MODULE,
	},
	.probe         = kg2_i2c_probe,
	.remove        = kg2_i2c_remove,
	.id_table      = kg2_i2c_id,
};


static int kg2_i2c_probe(struct i2c_client * client,
                         const struct i2c_device_id * id)
{
	dev_info(&client->dev, "probed\n");

	i2c_client_kg2 = client;

	// Run script - step1_INIT_part1
	dev_info(&client->dev, "Run script - INIT_Part1\n");
	if (kg2_run_script(step1_INIT_part1, step1_INIT_part1_Count) < 0)
	{
		return -1;
	}

	// Run script - step2_INIT_part2.h
	dev_info(&client->dev, "Run script - INIT_Part2\n");
	if (kg2_run_script(step2_INIT_part2, step2_INIT_part2_Count) < 0)
	{
		return -1;
	}

	// Run script - step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove.h
	dev_info(&client->dev, "Run script - INIT_IP_Flexiport\n");
	if (kg2_run_script(step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove,
	                   step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove_Count) < 0)
	{
		return -1;
	}

	// Run script - step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG-swap.h
	dev_info(&client->dev, "Run script - INIT_OP_Flexiport\n");
	if (kg2_run_script(step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG_swap,
	                   step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG_swap_Count) < 0)
	{
		return -1;
	}

	// Run script - step5_Dove_1024x600_1080P.h
	dev_info(&client->dev, "Run script - IP_1024x600_OP_1080P\n");
	if (kg2_run_script(step5_Dove_1024x600_1080P,
	                   step5_Dove_1024x600_1080P_Count) < 0)
	{
		return -1;
	}

	// Run script - step6_scaler.h
	dev_info(&client->dev, "Run script - Scaler\n");
	if (kg2_run_script(step6_scaler,
	                   step6_scaler_Count) < 0)
	{
		return -1;
	}

	return 0;
}

static int kg2_i2c_remove(struct i2c_client * client)
{
	i2c_client_kg2 = NULL;
	return 0;
}

/*-----------------------------------------------------------------------------
 * Description : Write data to KG2 via I2C bus
 * Parameters  : baseaddr - Page/Register access (0x20/0x22 or 0x40/0x42)
 *               subaddr  - Page access: 0x00, Register access: register offset
 *               data     - Page access: page number, Register access: data
 *               dataLen  - Length of data
 * Return Type : =0 - Success
 *               <0 - Fail
 *-----------------------------------------------------------------------------
 */

int kg2_i2c_write(unsigned char baseaddr, unsigned char subaddr,
                  const unsigned char * data, unsigned short dataLen)
{
#if defined(DEBUG)
	char output[DBG_I2C_LENGTH_PER_LINE];
	int count = 0;
#endif
	unsigned short i = 0;
	int result = 0;

	if (i2c_client_kg2 == NULL)
	{
		dev_err(&i2c_client_kg2->dev, "i2c_client_kg2 = NULL\n");
		return -1;
	}

	for (i = 0; i < dataLen; i++)
	{
		i2c_client_kg2->addr = baseaddr >> 1;
		result = i2c_smbus_write_byte_data(i2c_client_kg2,
		                                   subaddr + i,
		                                   data[i]);
		if (result < 0)
		{
			dev_err(&i2c_client_kg2->dev,
			        "Failed to write data via i2c. (baseaddr: 0x%02X, subaddr: 0x%02X, data: 0x%02X)\n",
			        baseaddr, subaddr + i, data[i]);

			return -1;
		}

#if defined(DEBUG)
		count += snprintf(output + count,
		                  DBG_I2C_LENGTH_PER_LINE - count,
		                  " %02X",
		                  data[i]);

		if ((i == (dataLen - 1)) || (i % DBG_I2C_DATA_PER_LINE == (DBG_I2C_DATA_PER_LINE - 1)))
		{
			if (i < DBG_I2C_DATA_PER_LINE)
			{
				dev_info(&i2c_client_kg2->dev,
				         "W - %02X %02X%s\n",
				         baseaddr, subaddr, output);
			}
			else
			{
				dev_info(&i2c_client_kg2->dev,
				         "         %s\n",
				         output);
			}

			count = 0;
		}
#endif
	}

	return 0;
}

/*-----------------------------------------------------------------------------
 * Description : Run KG2 script
 * Parameters  : array - The i2c array to be sent, contained in .h script file
 *               count - i2c array element count, contained in .h script file
 * Return Type : =0 - Success
 *               <0 - Fail
 *----------------------------------------------------------------------------- */

int kg2_run_script(const unsigned char * array, int count)
{
	const unsigned char * pArr = array;
	int i;

	for (i = 0; i < count;)
	{
		if (kg2_i2c_write(pArr[1], pArr[2], &pArr[3], pArr[0]) < 0)
		{
			return -1;
		}

		i += pArr[0] + 3;
		pArr += pArr[0] + 3;
	}

	return 0;
}

static int __init dove_kg2_init(void)
{
	return i2c_add_driver(&kg2_driver);
}

static void __exit dove_kg2_exit(void)
{
	i2c_del_driver(&kg2_driver);
}

module_init(dove_kg2_init);
module_exit(dove_kg2_exit);

MODULE_AUTHOR("Marvell");
MODULE_DESCRIPTION("Kyoto G2 driver");
MODULE_DEVICE_TABLE(i2c, kg2_i2c_id);
MODULE_LICENSE("GPL");
