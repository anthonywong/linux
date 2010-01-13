/*
 * kg2.c - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#include <linux/i2c.h>
#include <video/kg2.h>
#include "autocalib.h"
#include "kg2_i2c.h"
#include "kg2_regs.h"
#include "scripts/step1_INIT_part1.h"
#include "scripts/step2_INIT_part2.h"
#include "scripts/step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove.h"
#include "scripts/step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG-swap.h"
#include "scripts/step5_Dove_1024x600_1080P.h"
#include "scripts/step6_scaler.h"


#define I2C_PAGE_WRITE		0x20	// 0x40 while DEV_ADDR_SEL = 1
#define I2C_PAGE_READ		0x21	// 0x41 while DEV_ADDR_SEL = 1
#define I2C_REG_WRITE		0x22	// 0x42 while DEV_ADDR_SEL = 1
#define I2C_REG_READ		0x23	// 0x43 while DEV_ADDR_SEL = 1


// Glocal variable definitions
bool				bI2CBusy = false;				// Used to detect I2C bus collision
unsigned char			regCurrentPage = 0xFF;
AVC_CMD_TIMING_PARAM		avcInputTiming = {0};
extern struct i2c_client *	i2c_client_kg2;


static int kg2_i2c_probe(struct i2c_client * client, const struct i2c_device_id * id);
static int kg2_i2c_remove(struct i2c_client * client);
static int kg2_i2c_suspend(struct i2c_client * client, pm_message_t msg);
static int kg2_i2c_resume(struct i2c_client * client);

static const struct i2c_device_id kg2_i2c_id[] = {
	{"kg2_i2c", 0},
	{}
};

static struct i2c_driver kg2_driver = {
	.driver = {
		.name  = "kg2_i2c",
		.owner = THIS_MODULE,
	},
	.probe         = kg2_i2c_probe,
	.remove        = kg2_i2c_remove,
	.suspend       = kg2_i2c_suspend,
	.resume        = kg2_i2c_resume,
	.id_table      = kg2_i2c_id,
};


static int kg2_i2c_probe(struct i2c_client * client,
                         const struct i2c_device_id * id)
{
	dev_info(&client->dev, "probed\n");

	i2c_client_kg2 = client;
	return kg2_initialize();
}

static int kg2_i2c_remove(struct i2c_client * client)
{
	i2c_client_kg2 = NULL;
	return 0;
}

static int kg2_i2c_suspend(struct i2c_client * client, pm_message_t msg)
{
	return 0;
}

static int kg2_i2c_resume(struct i2c_client * client)
{
	if (kg2_initialize() != 0)
	{
		return -1;
	}

	if (kg2_set_input_timing(NULL) != 0)
	{
		return -1;
	}

	return 0;
}

int kg2_initialize(void)
{
	unsigned char ana_stat0, page, reg;

	dev_info(&i2c_client_kg2->dev, "Initialize KG2\n");

	bI2CBusy = true;

	// Run script - step1_INIT_part1
	dev_info(&i2c_client_kg2->dev, "Run script    - INIT_Part1\n");
	if (kg2_run_script(step1_INIT_part1, step1_INIT_part1_Count) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Wait until SSPLL is locked (ANA_STAT0 - bit 6, 0xF41)
	dev_info(&i2c_client_kg2->dev, "Check status  - SSPLL");
	page = 0x0F;
	reg = 0x41;

	if (kg2_i2c_write(I2C_PAGE_WRITE, 0x00, &page, sizeof(page)) == 0)
	{
		do
		{
			if (kg2_i2c_read(I2C_REG_READ, reg, &ana_stat0, sizeof(ana_stat0)) < 0)
			{
				goto ABORT_SSPLL_CHECK;
			}
		}
		while ((ana_stat0 & 0x40) == 0);
	}

ABORT_SSPLL_CHECK:

	// Run script - step2_INIT_part2.h
	dev_info(&i2c_client_kg2->dev, "Run script    - INIT_Part2\n");
	if (kg2_run_script(step2_INIT_part2, step2_INIT_part2_Count) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Run script - step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove.h
	dev_info(&i2c_client_kg2->dev, "Run script    - INIT_IP_Flexiport\n");
	if (kg2_run_script(step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove,
	                   step3_INIT_IP_Flexiport_RGB24_or_YCbCCr24_for_Dove_Count) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Run script - step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG-swap.h
	dev_info(&i2c_client_kg2->dev, "Run script    - INIT_OP_Flexiport\n");
	if (kg2_run_script(step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG_swap,
	                   step4_INIT_OP_Flexiport_RGB24_or_YCbCr24_for_Dove_RG_swap_Count) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Autocalibrate DDR2's drive strength
	dev_info(&i2c_client_kg2->dev, "Autocalibrate - SDRAM\n");
	if (StartSdramAutoCalibration() != 1)
	{
		dev_err(&i2c_client_kg2->dev, "Failed to do autocalibration.\n");
	}

	// Autocalibrate DAPLL
	dev_info(&i2c_client_kg2->dev, "Autocalibrate - DAPLL\n");
	if (StartPllAutoCalibration(AVC_DAPLL) != 1)
	{
		dev_err(&i2c_client_kg2->dev, "Failed to do autocalibration.\n");
	}

	// Run script - step5_Dove_1024x600_1080P.h
	dev_info(&i2c_client_kg2->dev, "Run script    - IP_1024x600_OP_1080P\n");
	if (kg2_run_script(step5_Dove_1024x600_1080P,
	                   step5_Dove_1024x600_1080P_Count) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Autocalibrate SAPLL2
	dev_info(&i2c_client_kg2->dev, "Autocalibrate - SAPLL2\n");
	if (StartPllAutoCalibration(AVC_SAPLL2) != 1)
	{
		dev_err(&i2c_client_kg2->dev, "Failed to do autocalibration.\n");
	}

	// Run script - step6_scaler.h
	dev_info(&i2c_client_kg2->dev, "Run script    - Scaler\n");
	if (kg2_run_script(step6_scaler,
	                   step6_scaler_Count) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	bI2CBusy = false;
	return 0;
}

int kg2_set_input_timing(AVC_CMD_TIMING_PARAM * timing)
{
	unsigned char			len, page, reg, * data;
	unsigned short			dht;
	HWI_FLEXIPORT_PIN_CONTROL_BITS	regFlexiPort = {{0x00}, 0x00, 0x00};
	HWI_FE_CHANNEL_REG		regFrontEnd;

	if (i2c_client_kg2 == NULL) {
		pr_info("No KG2 device found\n");
		return;
	}

	dev_info(&i2c_client_kg2->dev, "Set KG2 input timing\n");

	// Log if KG2 is accessed by two or more drivers simultaneously
	if (bI2CBusy == true)
	{
		dev_err(&i2c_client_kg2->dev, "KG2 is still under access\n");
		return -1;
	}

	bI2CBusy = true;

	if (timing != NULL)
	{
		avcInputTiming = *timing;
	}

	dev_info(&i2c_client_kg2->dev, "Ht:%5d, Ha:%5d, Hfp:%5d, Hsw:%5d, Hsp: %d\n",
	         avcInputTiming.HTotal, avcInputTiming.HActive, avcInputTiming.HFrontPorch, avcInputTiming.HSyncWidth, avcInputTiming.HPolarity);
	dev_info(&i2c_client_kg2->dev, "Vt:%5d, Va:%5d, Vfp:%5d, Vsw:%5d, Vsp: %d\n",
	         avcInputTiming.VTotal, avcInputTiming.VActive, avcInputTiming.VFrontPorch, avcInputTiming.VSyncWidth, avcInputTiming.VPolarity);

	// Calculate the values of the active window coordinates
	regFlexiPort.MiscInputControl.PolarityInversionforHSync	= avcInputTiming.HPolarity;
	regFlexiPort.MiscInputControl.PolarityInversionforVSync	= avcInputTiming.VPolarity;
	regFrontEnd.FeDcStrX.Value				= avcInputTiming.HTotal - avcInputTiming.HActive - avcInputTiming.HFrontPorch;
	regFrontEnd.FeDcStrY.Value				= avcInputTiming.VTotal - avcInputTiming.VActive - avcInputTiming.VFrontPorch;
	regFrontEnd.FeDcEndX.Value				= regFrontEnd.FeDcStrX.Value + avcInputTiming.HActive;
	regFrontEnd.FeDcEndY.Value				= regFrontEnd.FeDcStrY.Value + avcInputTiming.VActive;
	regFrontEnd.FeDcFrst					= (avcInputTiming.VTotal - avcInputTiming.VActive) / 2;
	regFrontEnd.FeDcLrst.Value				= (avcInputTiming.HTotal - avcInputTiming.HActive - avcInputTiming.HSyncWidth) / 4;
	dht							= avcInputTiming.HSyncWidth + avcInputTiming.HFrontPorch;
	regFrontEnd.FeDeltaHtot					= (dht > 0xFF) ? (0xFF) : dht;

	dev_dbg(&i2c_client_kg2->dev, "PolarityInversionforHSync: %d\n", regFlexiPort.MiscInputControl.PolarityInversionforHSync);
	dev_dbg(&i2c_client_kg2->dev, "PolarityInversionforVSync: %d\n", regFlexiPort.MiscInputControl.PolarityInversionforVSync);
	dev_dbg(&i2c_client_kg2->dev, "FeDcStrX: %d\n", regFrontEnd.FeDcStrX.Value);
	dev_dbg(&i2c_client_kg2->dev, "FeDcStrY: %d\n", regFrontEnd.FeDcStrY.Value);
	dev_dbg(&i2c_client_kg2->dev, "FeDcEndX: %d\n", regFrontEnd.FeDcEndX.Value);
	dev_dbg(&i2c_client_kg2->dev, "FeDcEndY: %d\n", regFrontEnd.FeDcEndY.Value);
	dev_dbg(&i2c_client_kg2->dev, "FeDcFrst: %d\n", regFrontEnd.FeDcFrst);
	dev_dbg(&i2c_client_kg2->dev, "FeDcLrst: %d\n", regFrontEnd.FeDcLrst.Value);
	dev_dbg(&i2c_client_kg2->dev, "FeDeltaHtot: %d\n", regFrontEnd.FeDeltaHtot);

	// Change KG2 register page if needed
	page = (unsigned char) (REG_PINH >> 8);

	if (regCurrentPage != page)
	{
		if (kg2_i2c_write(I2C_PAGE_WRITE, 0x00, &page, 1) < 0)
		{
			bI2CBusy = false;
			return -1;
		}

		regCurrentPage = page;
	}

	// Write to KG2 registers (0x0D7)
	reg  = (unsigned char) (REG_PINH & 0xFF);
	data = (unsigned char *) &regFlexiPort.MiscInputControl;
	len  = (unsigned short) ((void *) &regFlexiPort.CCIR656Control - (void *) &regFlexiPort.MiscInputControl);

	if (kg2_i2c_write(I2C_REG_WRITE, reg, data, len) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Change KG2 register page if needed
	page = (unsigned char) (REG_FE >> 8);

	if (regCurrentPage != page)
	{
		if (kg2_i2c_write(I2C_PAGE_WRITE, 0x00, &page, 1) < 0)
		{
			bI2CBusy = false;
			return -1;
		}

		regCurrentPage = page;
	}

	// Write to KG2 registers (0x100 ~ 0x107)
	reg  = (unsigned char) (REG_FE & 0xFF);
	data = (unsigned char *) &regFrontEnd.FeDcStrX;
	len  = (unsigned short) ((void *) &regFrontEnd.FeDcVSamp - (void *) &regFrontEnd.FeDcStrX);

	if (kg2_i2c_write(I2C_REG_WRITE, reg, data, len) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Write to KG2 registers (0x157 ~ 0x159)
	reg  = (unsigned char) ((REG_FE & 0xFF) + ((void *) &regFrontEnd.FeDcFrst - (void *) &regFrontEnd));
	data = (unsigned char *) &regFrontEnd.FeDcFrst;
	len  = (unsigned short) ((void *) &regFrontEnd.FeDcTgLoadFlg - (void *) &regFrontEnd.FeDcFrst);

	if (kg2_i2c_write(I2C_REG_WRITE, reg, data, len) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	// Write to KG2 registers (0x15E)
	reg  = (unsigned char) ((REG_FE & 0xFF) + ((void *) &regFrontEnd.FeDeltaHtot - (void *) &regFrontEnd));
	data = (unsigned char *) &regFrontEnd.FeDeltaHtot;
	len  = (unsigned short) ((void *) &regFrontEnd.Unused2 - (void *) &regFrontEnd.FeDeltaHtot);

	if (kg2_i2c_write(I2C_REG_WRITE, reg, data, len) < 0)
	{
		bI2CBusy = false;
		return -1;
	}

	bI2CBusy = false;
	return 0;
}

/*-----------------------------------------------------------------------------
 * Description : Run KG2 script
 * Parameters  : array - The i2c array to be sent, contained in .h script file
 *               count - i2c array element count, contained in .h script file
 * Return Type : =0 - Success
 *               <0 - Fail
 *-----------------------------------------------------------------------------
 */

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
