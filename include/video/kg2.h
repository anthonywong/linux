/*
 * kg2.h - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#ifndef __KG2_H__
#define __KG2_H__

// Copied from "88DE2750/sdk/2750_core/private/88DE2750/avc/cmdapp/inc/avc27xx_cmd.h" +

//AVC_CMD_SIGNAL_POLARITY is used in the AVC_CMD_TIMING_PARAM structure
typedef enum tagAVC_CMD_SIGNAL_POLARITY
{
	AVC_CMD_POLARITY_NO_INVERT = 0,
	AVC_CMD_POLARITY_INVERT

}AVC_CMD_SIGNAL_POLARITY;

//AVC_CMD_TIMING_PARAM to be used when the SUBCMD is AVC_SUBCMD_INPUT_RES_MANUAL_SEL only (Common for Input/Output)
typedef struct tagAVC_CMD_PARAM_TIMING
{
	unsigned short			HTotal;
	unsigned short			HActive;
	unsigned short			HFrontPorch;
	unsigned char			HSyncWidth;
	AVC_CMD_SIGNAL_POLARITY		HPolarity;

	unsigned short			VTotal;
	unsigned short			VActive;
	unsigned short			VFrontPorch;
	unsigned char			VSyncWidth;
	AVC_CMD_SIGNAL_POLARITY		VPolarity;

//	AVC_CMD_ASPECT_RATIO		AspRatio;		// Not used yet
//	unsigned char			IsProgressive;		// Not used yet

//	float				RefRate;		// Not used yet

}AVC_CMD_TIMING_PARAM, *PAVC_CMD_TIMING_PARAM;

// Copied from "88DE2750/sdk/2750_core/private/88DE2750/avc/cmdapp/inc/avc27xx_cmd.h" -

int kg2_run_script(const unsigned char * array, int count);
int kg2_i2c_write(unsigned char baseaddr, unsigned char subaddr, const unsigned char * data, unsigned short dataLen);
int kg2_initialize(void);
int kg2_set_input_timing(AVC_CMD_TIMING_PARAM * timing);

#endif
