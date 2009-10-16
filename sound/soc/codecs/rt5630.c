#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/jiffies.h>
#include <asm/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>

#include "rt5630.h"

#define RT5630_VERSION "0.03"
#define USE_DAPM_CTRL 0

/*
#define RT5630_DEBUG(format, args...) \
	printk(KERN_DEBUG "%s(%d): "format"\n", __FUNCTION__, __LINE__, ##args)
*/

#define RT5630_DEBUG(format, args...)


struct rt5630_priv {
	unsigned int stereo_sysclk;
	unsigned int voice_sysclk;
};

struct rt5630_init_reg {
	char name[26];
	u16 reg_value;
	u8 reg_index;
};

static struct rt5630_init_reg rt5630_init_list[] = {
	{"HP Output Volume", 0x9090, RT5630_HP_OUT_VOL},
	{"SPK Output Volume", 0x8080, RT5630_SPK_OUT_VOL},
	{"DAC Mic Route", 0xee03, RT5630_DAC_AND_MIC_CTRL},
	{"Output Mixer Control", 0x0748, RT5630_OUTPUT_MIXER_CTRL},
	{"Mic Control", 0x0500, RT5630_MIC_CTRL},
	{"Voice DAC Volume", 0x6020, RT5630_VOICE_DAC_OUT_VOL},
	{"ADC Rec Mixer", 0x5f5f, RT5630_ADC_REC_MIXER},
//	{"ADC Rec Mixer", 0x3f3f, RT5630_ADC_REC_MIXER},
	{"General Control", 0x0c0a, RT5630_GEN_CTRL_REG1},
	{"Power Management Addition 3", 0x000f, RT5630_PWR_MANAG_ADD3},
	{"Power Management Addition 1", 0x000e, RT5630_PWR_MANAG_ADD1},
	{"PCM Capture Volume",0xcfcf, RT5630_ADC_REC_GAIN},
	{"MIC Volume Control", 0x889F, RT5630_MIC_VOL},



};

#define RT5630_INIT_REG_NUM ARRAY_SIZE(rt5630_init_list)

/*
 *	bit[0]  for linein playback switch
 *	bit[1] phone
 *	bit[2] mic1
 *	bit[3] mic2
 *	bit[4] vopcm
 *	
 */
 #define HPL_MIXER 0x80
#define HPR_MIXER 0x82
static unsigned int reg80 = 0, reg82 = 0;

/*
 *	bit[0][1] use for aec control
 *	bit[4] for SPKL pga
 *	bit[5] for SPKR pga
 *	bit[6] for hpl pga
 *	bit[7] for hpr pga
 *	bit[8-9] for misc dsp func
 *	bit[10-11] for ve mode
 */
 #define VIRTUAL_REG_FOR_MISC_FUNC 0x84
static unsigned int reg84 = 0;


static const u16 rt5630_reg[] = {
	0x59b4, 0x8080, 0x8080, 0x8080,		/*reg00-reg06*/
	0xc800, 0xe808, 0x1010, 0x0808,		/*reg08-reg0e*/
	0xe0ef, 0xcbcb, 0x7f7f, 0x0000,		/*reg10-reg16*/
	0xe010, 0x0000, 0x8008, 0x2007,		/*reg18-reg1e*/
	0x0000, 0x0000, 0x00c0, 0xef00,		/*reg20-reg26*/
	0x0000, 0x0000, 0x0000, 0x0000,		/*reg28-reg2e*/
	0x0000, 0x0000, 0x0000, 0x0000,		/*reg30-reg36*/
	0x0000, 0x0000, 0x0000, 0x0000, 		/*reg38-reg3e*/
	0x0c0a, 0x0000, 0x0000, 0x0000,		/*reg40-reg46*/
	0x0029, 0x0000, 0xbe3e, 0x3e3e,		/*reg48-reg4e*/
	0x0000, 0x0000, 0x803a, 0x0000,		/*reg50-reg56*/
	0x0000, 0x0009, 0x0000, 0x3000,		/*reg58-reg5e*/
	0x3075, 0x1010, 0x3110, 0x0000,		/*reg60-reg66*/
	0x0553, 0x0000, 0x0000, 0x0000,		/*reg68-reg6e*/
	0x0000, 0x0000, 0x0000, 0x0000,		/*reg70-reg76*/
	0x0000, 0x0000, 0x0000, 0x0000,               /*reg76-reg7e*/
};


Voice_DSP_Reg VODSP_AEC_Init_Value[]=
{
	{0x232C, 0x0025},
	{0x230B, 0x0001},
	{0x2308, 0x007F},
	{0x23F8, 0x4003},
	{0x2301, 0x0002},
	{0x2328, 0x0001},
	{0x2304, 0x00FA},
	{0x2305, 0x0100},
	{0x2306, 0x4000},
	{0x230D, 0x0400},
	{0x230E, 0x0100},
	{0x2312, 0x00B1},
	{0x2314, 0xC000},
	{0x2316, 0x0041},
	{0x2317, 0x2000},
	{0x2318, 0x0C00},
	{0x231D, 0x00A0},
	{0x231F, 0x5800},
	{0x2330, 0x0008},
	{0x2335, 0x0005},
	{0x2336, 0x0001},
	{0x2337, 0x5800},
	{0x233A, 0x0300},
	{0x233B, 0x0030},
	{0x2341, 0x0008},
	{0x2343, 0x0800},		
	{0x23A7, 0x0200},
	{0x22CE, 0x0400},
	{0x22D3, 0x0C00},
	{0x22D4, 0x1800},
	{0x230C, 0x0000},	//to enable VODSP AEC function
};


#define SET_VODSP_REG_AEC_INIT_NUM	ARRAY_SIZE(VODSP_AEC_Init_Value)

Voice_DSP_Reg VODSP_VE_Init_Value[]=
{
	{0x232C, 0x0025},
	{0x230B, 0x0003},
	{0x2308, 0x007F},
	{0x23F8, 0x4003},
	{0x23FF, 0x8001},
	{0x2301, 0x0002},
	{0x2304, 0x00FA},
	{0x2305, 0x0100},
	{0x2306, 0x6000},
	{0x230D, 0x0200},
	{0x230E, 0x0100},
	{0x2311, 0x0012},
	{0x2312, 0x00B1},
	{0x2314, 0xD000},
	{0x2316, 0x0075},
	{0x2317, 0x1200},
	{0x2318, 0x0C00},
	{0x2352, 0x2000},
	{0x2353, 0x6000},
	{0x231D, 0x00A0},
	{0x231F, 0x5800},
	{0x2330, 0x0008},
	{0x233A, 0x0300},
	{0x233B, 0x0030},
	{0x2341, 0x0008},	
	{0x23A7, 0x0200},
	{0x22D3, 0x0C00},
	{0x22D4, 0x1800},
	{0x230C, 0x0000},	//to enable VODSP VE function
};

#define SET_VODSP_VE_REG_INIT_NUM	(sizeof(VODSP_VE_Init_Value)/sizeof(Voice_DSP_Reg))


struct Voice_DSP_Function
{
	int feature;
	Voice_DSP_Reg *reg_setting;
	int regsize;
};

static struct Voice_DSP_Function voice_dsp_function_table[] =
{
		{VODSP_AEC_FUNC, VODSP_AEC_Init_Value, SET_VODSP_REG_AEC_INIT_NUM},
		{VODSP_VE_FUNC, VODSP_VE_Init_Value, SET_VODSP_VE_REG_INIT_NUM},
		{VODSP_ALL_FUNC, NULL, 0},
};

/*here we default set dsp init data to be aec function*/
static Voice_DSP_Reg *dsp_reg_for_misc_func = VODSP_AEC_Init_Value;           
static int dsp_reg_num = SET_VODSP_REG_AEC_INIT_NUM;


static struct snd_soc_device *rt5630_socdev;

static inline unsigned int rt5630_read_reg_cache(struct snd_soc_codec *codec, 
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;

	if (reg > 0x7e)
		return -1;
	return cache[reg / 2];
}


static unsigned int rt5630_read_hw_reg(struct snd_soc_codec *codec, unsigned int reg) 
{
	u8 data[2] = {0};
	unsigned int value = 0x0;
	
	data[0] = reg;
	if (codec->hw_write(codec->control_data, data, 1) == 1)
	{
		codec->hw_read(codec->control_data, data, 2);
		value = (data[0] << 8) | data[1];
//		RT5630_DEBUG("%s read reg%x = %x\n", reg, value);
		return value;
	}
	else
	{
		RT5630_DEBUG("%s failed\n");
		return -EIO;
	}
}


static unsigned int rt5630_read(struct snd_soc_codec *codec, unsigned int reg)
{
	if ((reg == 0x80)
		|| (reg == 0x82)
		|| (reg == 0x84))
		return (reg == 0x80) ? reg80 : ((reg == 0x82) ? reg82 : reg84);



	return rt5630_read_reg_cache(codec, reg);
}


static inline void rt5630_write_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg > 0x7E)
		return;
	cache[reg / 2] = value;
}

static int rt5630_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[3];
	unsigned int *regvalue = NULL;

	data[0] = reg;
	data[1] = (value & 0xff00) >> 8;
	data[2] = value & 0x00ff;

	
	if ((reg == 0x80)
		|| (reg == 0x82)
		|| (reg == 0x84))
	{		
		regvalue = ((reg == 0x80) ? &reg80 : ((reg == 0x82) ? &reg82 : &reg84));
		*regvalue = value;
		RT5630_DEBUG("rt5630_write ok, reg = %x, value = %x\n", reg, value);
		return 0;
	}
	rt5630_write_reg_cache(codec, reg, value);
	if (codec->hw_write(codec->control_data, data, 3) == 3)
	{
		RT5630_DEBUG("rt5630_write ok, reg = %x, value = %x\n", reg, value);
		return 0;
	}
	else 
	{
		RT5630_DEBUG("rt5630_write fail\n");
		return -EIO;
	}
}


#define rt5630_write_mask(c, reg, value, mask) snd_soc_update_bits(c, reg, mask, value)

#define rt5630_reset(c) rt5630_write(c, RT5630_RESET, 0)

static int rt5630_reg_init(struct snd_soc_codec *codec)
{
	int i;

	for (i = 0; i < RT5630_INIT_REG_NUM; i++)
		rt5630_write(codec, rt5630_init_list[i].reg_index, rt5630_init_list[i].reg_value);

	return 0;
}


/*read/write dsp reg*/
static int rt5630_wait_vodsp_i2c_done(struct snd_soc_codec *codec)
{
	unsigned int checkcount = 0, vodsp_data;

	vodsp_data = rt5630_read(codec, RT5630_VODSP_REG_CMD);
	while(vodsp_data & VODSP_BUSY)
	{
		if(checkcount > 10)
			return -EBUSY;
		vodsp_data = rt5630_read(codec, RT5630_VODSP_REG_CMD);
		checkcount ++;		
	}
	return 0;
}


static int rt5630_write_vodsp_reg(struct snd_soc_codec *codec, unsigned int vodspreg, unsigned int value)
{
	int ret = 0;

	if(ret = rt5630_wait_vodsp_i2c_done(codec))
		return ret;

	rt5630_write(codec, RT5630_VODSP_REG_ADDR, vodspreg);
	rt5630_write(codec, RT5630_VODSP_REG_DATA, value);
	rt5630_write(codec, RT5630_VODSP_REG_CMD, VODSP_WRITE_ENABLE | VODSP_CMD_MW);

	return ret;
	
}

static int rt5630_read_vodsp_reg(struct snd_soc_codec *codec, unsigned int vodspreg, unsigned int value)
{
	int ret = 0;
	unsigned int nDataH, nDataL;

	if(ret = rt5630_wait_vodsp_i2c_done(codec))
		return ret;
	
	rt5630_write(codec, RT5630_VODSP_REG_ADDR, vodspreg);
	rt5630_write(codec, RT5630_VODSP_REG_CMD, VODSP_READ_ENABLE | VODSP_CMD_MR);

	if (ret = rt5630_wait_vodsp_i2c_done(codec))
		return ret;
	rt5630_write(codec, RT5630_VODSP_REG_ADDR, 0x26);
	rt5630_write(codec, RT5630_VODSP_REG_CMD, VODSP_READ_ENABLE | VODSP_CMD_RR);

	if(ret = rt5630_wait_vodsp_i2c_done(codec))
		return ret;
	nDataH = rt5630_read(codec, RT5630_VODSP_REG_DATA);
	rt5630_write(codec, RT5630_VODSP_REG_ADDR, 0x25);
	rt5630_write(codec, RT5630_VODSP_REG_CMD, VODSP_READ_ENABLE | VODSP_CMD_RR);

	if(ret = rt5630_wait_vodsp_i2c_done(codec))
		return ret;
	nDataL = rt5630_read(codec, RT5630_VODSP_REG_DATA);
	return ((nDataH & 0xff) << 8) |(nDataL & 0xff);	
}




static const char *rt5630_vodsp_aec_path_sel[] = {"pcm in pcm out", "analog in analog out", 
					"dac in adc out", "disable"};    								/*0*/
static const char *rt5630_spk_out_sel[] = {"Class AB", "Class D"}; 					/*1*/
static const char *rt5630_spk_l_source_sel[] = {"LPRN", "LPRP", "LPLN", "MM"};		/*2*/	
static const char *rt5630_spkmux_source_sel[] = {"VMID", "HP Mixer", 
							"SPK Mixer", "Mono Mixer"};               					/*3*/
static const char *rt5630_hplmux_source_sel[] = {"VMID","HPL Mixer"};				/*4*/
static const char *rt5630_hprmux_source_sel[] = {"VMID","HPR Mixer"};				/*5*/
static const char *rt5630_auxmux_source_sel[] = {"VMID", "HP Mixer", 
							"SPK Mixer", "Mono Mixer"};							/*6*/
static const char *rt5630_spkamp_ratio_sel[] = {"2.25 Vdd", "2.00 Vdd",
					"1.75 Vdd", "1.50 Vdd", "1.25 Vdd", "1.00 Vdd"};				/*7*/
static const char *rt5630_mic1_boost_sel[] = {"Bypass", "+20db", "+30db", "+40db"};	/*8*/
static const char *rt5630_mic2_boost_sel[] = {"Bypass", "+20db", "+30db", "+40db"};	/*9*/
static const char *rt5630_dmic_boost_sel[] = {"Bypass", "+6db", "+12db", "+18db", 
					"+24db", "+30db", "+36db", "+42db"};						/*10*/
static const char *rt5630_dsp_misc_func_sel[] = {"AEC Func", "VE Func", "DSP Func 3", "DSP Func 4"};  /*11*/
static const char *rt5630_ve_path_sel[] = {"VE Output to DAC", "VE Output to VoDAC", "disable"};          /*12*/  


static const struct soc_enum rt5630_enum[] = {
SOC_ENUM_SINGLE(VIRTUAL_REG_FOR_MISC_FUNC, 0, 4, rt5630_vodsp_aec_path_sel),		/*0*/
SOC_ENUM_SINGLE(RT5630_OUTPUT_MIXER_CTRL, 13, 2, rt5630_spk_out_sel),		/*1*/
SOC_ENUM_SINGLE(RT5630_OUTPUT_MIXER_CTRL, 14, 4, rt5630_spk_l_source_sel),	/*2*/
SOC_ENUM_SINGLE(RT5630_OUTPUT_MIXER_CTRL, 10, 4, rt5630_spkmux_source_sel),/*3*/
SOC_ENUM_SINGLE(RT5630_OUTPUT_MIXER_CTRL, 9, 2, rt5630_hplmux_source_sel),	/*4*/
SOC_ENUM_SINGLE(RT5630_OUTPUT_MIXER_CTRL, 8, 2, rt5630_hprmux_source_sel),/*5*/
SOC_ENUM_SINGLE(RT5630_OUTPUT_MIXER_CTRL, 6, 4, rt5630_auxmux_source_sel),/*6*/
SOC_ENUM_SINGLE(RT5630_GEN_CTRL_REG1, 1, 6, rt5630_spkamp_ratio_sel),		/*7*/
SOC_ENUM_SINGLE(RT5630_MIC_CTRL, 10, 4,  rt5630_mic1_boost_sel),			/*8*/
SOC_ENUM_SINGLE(RT5630_MIC_CTRL, 8, 4, rt5630_mic2_boost_sel),				/*9*/
SOC_ENUM_SINGLE(RT5630_DMIC_CTRL, 0, 8, rt5630_dmic_boost_sel),				/*10*/
SOC_ENUM_SINGLE(VIRTUAL_REG_FOR_MISC_FUNC, 8, 4, rt5630_dsp_misc_func_sel),/*11*/
SOC_ENUM_SINGLE(VIRTUAL_REG_FOR_MISC_FUNC, 10, 3, rt5630_ve_path_sel),        /*12*/
};

static int init_vodsp(struct snd_soc_codec *codec)
{
	int i;
	int ret = 0;
	
	/*enable LDO power and set output voltage to 1.2V*/
	rt5630_write_mask(codec, RT5630_LDO_CTRL,LDO_ENABLE|LDO_OUT_VOL_CTRL_1_20V,LDO_ENABLE|LDO_OUT_VOL_CTRL_MASK);
	mdelay(20);
	/*enable power of VODSP I2C interface*/ 
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP);
	mdelay(1);
	rt5630_write_mask(codec, RT5630_VODSP_CTL,0,VODSP_NO_RST_MODE_ENA);	/*Reset VODSP*/
	mdelay(1);
	rt5630_write_mask(codec, RT5630_VODSP_CTL,VODSP_NO_RST_MODE_ENA,VODSP_NO_RST_MODE_ENA);	/*set VODSP to non-reset status*/		
	mdelay(20);

	/*initize AEC paramter*/
	for(i = 0; i < dsp_reg_num; i++)
	{
		ret = rt5630_write_vodsp_reg(codec, dsp_reg_for_misc_func[i].VoiceDSPIndex,dsp_reg_for_misc_func[i].VoiceDSPValue);

		if(ret)
			return -EIO;
	}		
	mdelay(100);

	//set VODSP to pown down mode	
	rt5630_write_mask(codec, RT5630_VODSP_CTL,0,VODSP_NO_PD_MODE_ENA);	
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3,0,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP);            //disable txdc/txdp/rxdp

	return 0;
}

static int set_vodsp_aec_path(struct snd_soc_codec *codec, unsigned int mode)
{
		switch(mode)
		{
			/*Far End signal is from Voice interface and Near End signal is from MIC1/MIC2)*/
			case PCM_IN_PCM_OUT:

				 /*	1.Far End setting(Far end device PCM out-->VODAC PCM IN--->VODSP_RXDP )*/
				 
				/*****************************************************************************
				  *	a.Enable RxDP power and select RxDP source from "Voice to Stereo Digital path"		
				  *	b.Voice PCM out from VoDSP TxDP(VODSP TXDP--->VODAC PCM out-->Far End devie PCM out)
				  ******************************************************************************/
				rt5630_write_mask(codec, RT5630_VODSP_PDM_CTL,VODSP_RXDP_PWR|VODSP_RXDP_S_SEL_VOICE|VOICE_PCM_S_SEL_AEC_TXDP
															,VODSP_RXDP_PWR|VODSP_RXDP_S_SEL_MASK|VOICE_PCM_S_SEL_MASK);
				 /*	2.Near end setting*/
				/***********************************************************************************	 
				  *	a.ADCR function select PDM Slave interface(Mic-->ADCR-->PDM interface)
				  *	b.Voice DAC Source Select VODSP_TxDC
				  ************************************************************************************/
				rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL,ADCR_FUNC_SEL_PDM|VODAC_SOUR_SEL_VODSP_TXDC
																,ADCR_FUNC_SEL_MASK|VODAC_SOUR_SEL_MASK);

				/*3.setting VODSP LRCK to 8k*/
				rt5630_write_mask(codec, RT5630_VODSP_CTL,VODSP_LRCK_SEL_8K,VODSP_LRCK_SEL_MASK);						

				break;
			
			/*Far End signal is from Analog input and Near End signal is from MIC1/MIC2)*/
			case ANALOG_IN_ANALOG_OUT:	
				/*	1.Far End setting(Far end device-->Analog in-->ADC_L-->VODSP_RXDP)   */
				/************************************************************************	
				  *	a.Enable RxDP power and select RxDP source from ADC_L 
				  ************************************************************************/
				rt5630_write_mask(codec, RT5630_VODSP_PDM_CTL,VODSP_RXDP_PWR|VODSP_RXDP_S_SEL_ADCL,
															 VODSP_RXDP_PWR|VODSP_RXDP_S_SEL_MASK);

				/*2.Near end setting*/
					/*************************************************************************
					  *a.VoDSP TxDP--->VODAC--->analog out-->to Far end analog input
					  *b.ADCR function select PDM Slave interface(Mic-->ADCR-->PDM interface)
					  *************************************************************************/
				rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL,ADCR_FUNC_SEL_PDM|VODAC_SOUR_SEL_VODSP_TXDP
																	,ADCR_FUNC_SEL_MASK|VODAC_SOUR_SEL_MASK);
				/*3.setting VODSP LRCK to 16k*/
				rt5630_write_mask(codec, RT5630_VODSP_CTL,VODSP_LRCK_SEL_16K,VODSP_LRCK_SEL_MASK);	
			
				break;

			/*Far End signal is from Playback and Near End signal is from MIC1/MIC2)*/
			case DAC_IN_ADC_OUT:	
				/***********************************************************************
				  *	1.Far End setting(Playback-->SRC1-->VODSP_RXDP)
				  *		a.enable SRC1 and VoDSP_RXDP source select SRC1
				  *	2.Near End setting(VoDSP_TXDP-->SRC2-->Stereo Record)
				  *		a.enable SRC2 and select Record source from SRC2
				  **********************************************************************/
				rt5630_write_mask(codec, RT5630_VODSP_PDM_CTL,VODSP_SRC1_PWR|VODSP_SRC2_PWR|VODSP_RXDP_PWR|VODSP_RXDP_S_SEL_SRC1|REC_S_SEL_SRC2,
															 VODSP_SRC1_PWR|VODSP_SRC2_PWR|VODSP_RXDP_PWR|VODSP_RXDP_S_SEL_MASK|REC_S_SEL_MASK);					

				break;

			case VODSP_AEC_DISABLE:
			default:
			
				/*set stereo DAC&Voice DAC&Stereo ADC function select to default*/ 
				rt5630_write(codec, RT5630_DAC_ADC_VODAC_FUN_SEL,0);

				/*set VODSP&PDM Control to default*/ 
				rt5630_write(codec, RT5630_VODSP_PDM_CTL,0);	
			
				break;
		}		

	return 0;
}



int enable_vodsp_aec_func(struct snd_soc_codec *codec, unsigned int VodspAEC_En, unsigned int AEC_mode)
{
	int  ret = 0;

	
	if (VodspAEC_En != 0)
	{	
		//select input/output of VODSP AEC
		set_vodsp_aec_path(codec, AEC_mode);		
		//enable power of VODSP I2C interface & VODSP interface
		rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP);
		//enable power of VODSP I2S interface 
		rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD1,PWR_I2S_INTERFACE,PWR_I2S_INTERFACE);	
		//set VODSP to active			
		rt5630_write_mask(codec, RT5630_VODSP_CTL,VODSP_NO_PD_MODE_ENA,VODSP_NO_PD_MODE_ENA);	
		mdelay(50);	
	}
	else
	{
		//set VODSP AEC to power down mode			
		rt5630_write_mask(codec, RT5630_VODSP_CTL,0,VODSP_NO_PD_MODE_ENA);
		//disable power of VODSP I2C interface & VODSP interface
		rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3,0,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP);
		//disable VODSP AEC path
		set_vodsp_aec_path(codec, VODSP_AEC_DISABLE);				
	}

	return ret;
}

EXPORT_SYMBOL_GPL(enable_vodsp_aec_func);
static void rt5630_aec_config(struct snd_soc_codec *codec, unsigned int mode)
{
	static int aec_init_flag = 0;
	unsigned int aec_enable = 1;

	if (mode == VODSP_AEC_DISABLE)
		aec_enable = 0;
	
	if (!aec_init_flag && aec_enable)
	{
		init_vodsp(codec);
		aec_init_flag = 1;
	}
	
	enable_vodsp_aec_func(codec, aec_enable, mode);	
}


static int rt5630_get_aec_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int mode = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC) & 0x03;  

	ucontrol->value.integer.value[0] = mode;    
	return 0;
}


static int rt5630_set_aec_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int mode = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC);

	if ( (mode & 0x03) == ucontrol->value.integer.value[0])
		return 0;

	mode &= 0xfffc;
	mode |= ucontrol->value.integer.value[0];
	rt5630_write(codec, VIRTUAL_REG_FOR_MISC_FUNC, mode);
	rt5630_aec_config(codec, ucontrol->value.integer.value[0]);
	return 1;
}


static int set_vodsp_ve_path(struct snd_soc_codec *codec, int VE_mode)
{
	switch (VE_mode)
	{
		case TXDP_TO_DAC:
			rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL, 0x2000, 0x3000);                      /*DAC source from TXDP*/
			break;
		case TXDP_TO_VODAC:
			rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL, 0x0200, 0x0700);                     /*vodac source from TXDP*/
			break;
		case VE_DISABLE:
			rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL, 0x0000, 0x3000);
			rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL, 0x0000, 0x0700);
			break;
		default:
			return -EINVAL;		
	}
	return 0;
}


int enable_vodsp_ve_func(struct snd_soc_codec *codec, int vodsp_ve_en, int VE_mode)
{
	int ret = 0;

	if(vodsp_ve_en != 0)
	{	
		rt5630_write_mask(codec, RT5630_DMIC_CTRL, 0x8000, 0x8000);         /*enable dmic*/
		set_vodsp_ve_path(codec, VE_mode);
		//enable power of VODSP I2C interface & VODSP interface
		rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP);
		//enable power of VODSP I2S interface 
		rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD1,PWR_I2S_INTERFACE,PWR_I2S_INTERFACE);	
		//set VODSP to active			
		rt5630_write_mask(codec, RT5630_VODSP_CTL,VODSP_NO_PD_MODE_ENA,VODSP_NO_PD_MODE_ENA);	
		msleep(50);	

	}
	else
	{
		//set VODSP to power down mode			
		rt5630_write_mask(codec, RT5630_DMIC_CTRL, 0x0000, 0x8000);        /*disable dmic*/
		rt5630_write_mask(codec, RT5630_VODSP_CTL,0,VODSP_NO_PD_MODE_ENA);
		//disable power of VODSP I2C interface & VODSP interface
		rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3,0,PWR_VODSP_INTERFACE|PWR_I2C_FOR_VODSP);
		set_vodsp_ve_path(codec, VE_DISABLE);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(enable_vodsp_ve_func);


static void rt5630_ve_config(struct snd_soc_codec *codec, unsigned int mode)
{
	static int ve_init_flag = 0;
	unsigned int ve_enable = 1;

	if (mode == VE_DISABLE)
		ve_enable = 0;
	
	if (!ve_init_flag && ve_enable)
	{
		init_vodsp(codec);
		ve_init_flag = 1;
	}

	enable_vodsp_ve_func(codec, ve_enable, mode);	
}

static int rt5630_get_ve_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int mode = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC) & (0x03 << 10);  

	ucontrol->value.integer.value[0] = (mode >> 10);    
	return 0;
}


static int rt5630_set_ve_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int mode = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC);

	if (((mode >> 10) & 0x03) == ucontrol->value.integer.value[0])
		return 0;

	mode &= 0xf3ff;
	mode |= (ucontrol->value.integer.value[0] << 10);
	rt5630_write(codec, VIRTUAL_REG_FOR_MISC_FUNC, mode);
	rt5630_ve_config(codec, ucontrol->value.integer.value[0]);	
	return 0;
}



static int rt5630_get_vodsp_func(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int mode = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC) & (0x03 << 8);  

	ucontrol->value.integer.value[0] = (mode >> 8);    
	return 0;
}

static int rt5630_set_vodsp_init_reg(struct snd_soc_codec *codec, int mode)
{
	struct Voice_DSP_Function *p;

	for (p = voice_dsp_function_table; p != NULL; p++)
	{
		if (p->feature == mode)
			break;
	}

	if ((p->feature == VODSP_ALL_FUNC) ||(p->reg_setting == NULL))
		return 0;

	dsp_reg_for_misc_func = p->reg_setting;
	dsp_reg_num = p->regsize;
	return 0;
}


/*template driver when you want to use aec func*/
/*here we assume that your mclk is 13Mhz, and pcm in pcm out*/

int rt5630_set_vodsp_func_template(struct snd_soc_codec *codec, int enable)
{
	if (enable) {
		rt5630_codec_set_pll1(codec, RT5630_PLL1_FROM_MCLK, 13000000,24576000);
		rt5630_write_mask(codec, 0x40, 0x8000, 0x8000);     // sysclk from pll1
		rt5630_write_mask(codec, 0x1e, 0x0000, 0x8000);     //vodsp clk from sysclk    

		enable_vodsp_aec_func(codec, 1, PCM_IN_PCM_OUT);
	}
	else {
		enable_vodsp_aec_func(codec, 0, PCM_IN_PCM_OUT);
	}
}

static const struct snd_kcontrol_new rt5630_snd_controls[] = {
SOC_ENUM("SPK Amp Type", rt5630_enum[1]),
SOC_ENUM("Left SPK Source", rt5630_enum[2]),
SOC_ENUM("SPK Amp Ratio", rt5630_enum[7]),
SOC_ENUM("Mic1 Boost", rt5630_enum[8]),
SOC_ENUM("Mic2 Boost", rt5630_enum[9]),
SOC_ENUM("Dmic Boost", rt5630_enum[10]),
SOC_DOUBLE("LineIn Playback Volume", RT5630_LINE_IN_VOL, 8, 0, 31, 1),
SOC_SINGLE("Phone Playback Volume", RT5630_PHONEIN_VOL, 8, 31, 1),
SOC_SINGLE("Mic1 Playback Volume", RT5630_MIC_VOL, 8, 31, 1),
SOC_SINGLE("Mic2 Playback Volume", RT5630_MIC_VOL, 0, 31, 1),
SOC_DOUBLE("PCM Capture Volume", RT5630_ADC_REC_GAIN, 8, 0, 31, 1),
SOC_DOUBLE("SPKOUT Playback Volume", RT5630_SPK_OUT_VOL, 8, 0, 31, 1),
SOC_DOUBLE("SPKOUT Playback Switch", RT5630_SPK_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE("HPOUT Playback Volume", RT5630_HP_OUT_VOL, 8, 0, 31, 1),
SOC_DOUBLE("HPOUT Playback Switch", RT5630_HP_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE("AUXOUT Playback Volume", RT5630_AUX_OUT_VOL, 8, 0, 31, 1),
SOC_DOUBLE("AUXOUT Playback Switch", RT5630_AUX_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE("ADC Record Gain", RT5630_ADC_REC_GAIN, 8, 0, 31, 0),
};

static int rt5630_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(rt5630_snd_controls); i++){
		err = snd_ctl_add(codec->card, 
				snd_soc_cnew(&rt5630_snd_controls[i],
						codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}

static void hp_depop_mode2(struct snd_soc_codec *codec)
{
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD1, PWR_MAIN_BIAS, PWR_MAIN_BIAS);
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD2, PWR_MIXER_VREF, PWR_MIXER_VREF);
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD1, PWR_SOFTGEN_EN, PWR_SOFTGEN_EN);
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD3, PWR_HP_R_OUT_VOL|PWR_HP_L_OUT_VOL,
		PWR_HP_R_OUT_VOL|PWR_HP_L_OUT_VOL);
	rt5630_write_mask(codec, RT5630_MISC_CTRL, HP_DEPOP_MODE2_EN, HP_DEPOP_MODE2_EN);
	schedule_timeout_uninterruptible(msecs_to_jiffies(300));
	rt5630_write_mask(codec, RT5630_MISC_CTRL, 0, HP_DEPOP_MODE2_EN);
	rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD1, PWR_HP_OUT_AMP,
		PWR_HP_OUT_AMP);
}
#if USE_DAPM_CTRL
/*
 * _DAPM_ Controls
 */
 /*Left ADC Rec mixer*/
static const struct snd_kcontrol_new rt5630_left_adc_rec_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5630_ADC_REC_MIXER, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5630_ADC_REC_MIXER, 13, 1, 1),
SOC_DAPM_SINGLE("LineIn Capture Switch", RT5630_ADC_REC_MIXER, 12, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", RT5630_ADC_REC_MIXER, 11, 1, 1),
SOC_DAPM_SINGLE("HP Mixer Capture Switch", RT5630_ADC_REC_MIXER, 10, 1, 1),
SOC_DAPM_SINGLE("MoNo Mixer Capture Switch", RT5630_ADC_REC_MIXER, 8, 1, 1),
SOC_DAPM_SINGLE("SPK Mixer Capture Switch", RT5630_ADC_REC_MIXER, 9, 1, 1),

};

/*Left ADC Rec mixer*/
static const struct snd_kcontrol_new rt5630_right_adc_rec_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5630_ADC_REC_MIXER, 6, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5630_ADC_REC_MIXER, 5, 1, 1),
SOC_DAPM_SINGLE("LineIn Capture Switch", RT5630_ADC_REC_MIXER, 4, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", RT5630_ADC_REC_MIXER, 3, 1, 1),
SOC_DAPM_SINGLE("HP Mixer Capture Switch", RT5630_ADC_REC_MIXER, 2, 1, 1),
SOC_DAPM_SINGLE("MoNo Mixer Capture Switch", RT5630_ADC_REC_MIXER, 0, 1, 1),
SOC_DAPM_SINGLE("SPK Mixer Capture Switch", RT5630_ADC_REC_MIXER, 1, 1, 1),
};

static const struct snd_kcontrol_new rt5630_left_hp_mixer_controls[] = {
SOC_DAPM_SINGLE("ADC Playback Switch", RT5630_ADC_REC_GAIN, 15, 1, 1),
SOC_DAPM_SINGLE("LineIn Playback Switch", HPL_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("Phone Playback Switch", HPL_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPL_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPL_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("HIFI DAC Playback Switch", RT5630_DAC_AND_MIC_CTRL, 3, 1, 1),
SOC_DAPM_SINGLE("Voice DAC Playback Switch", HPL_MIXER, 4, 1, 0),
};

static const struct snd_kcontrol_new rt5630_right_hp_mixer_controls[] = {
SOC_DAPM_SINGLE("ADC Playback Switch", RT5630_ADC_REC_GAIN, 7, 1, 1),
SOC_DAPM_SINGLE("LineIn Playback Switch", HPR_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("Phone Playback Switch", HPR_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPR_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPR_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("HIFI DAC Playback Switch", RT5630_DAC_AND_MIC_CTRL, 2, 1, 1),
SOC_DAPM_SINGLE("Voice DAC Playback Switch", HPR_MIXER, 4, 1, 0),
};

static const struct snd_kcontrol_new rt5630_mono_mixer_controls[] = {
SOC_DAPM_SINGLE("ADCL Playback Switch", RT5630_ADC_REC_GAIN, 14, 1, 1),
SOC_DAPM_SINGLE("ADCR Playback Switch", RT5630_ADC_REC_GAIN, 6, 1, 1),
SOC_DAPM_SINGLE("Line Mixer Playback Switch", RT5630_LINE_IN_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5630_DAC_AND_MIC_CTRL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5630_DAC_AND_MIC_CTRL, 9, 1, 1),
SOC_DAPM_SINGLE("DAC Mixer Playback Switch", RT5630_DAC_AND_MIC_CTRL, 0, 1, 1),
SOC_DAPM_SINGLE("Voice DAC Playback Switch", RT5630_VOICE_DAC_OUT_VOL, 13, 1, 1),
};

static const struct snd_kcontrol_new rt5630_spk_mixer_controls[] = {
SOC_DAPM_SINGLE("Line Mixer Playback Switch", RT5630_LINE_IN_VOL, 14, 1, 1),	
SOC_DAPM_SINGLE("Phone Playback Switch", RT5630_PHONEIN_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5630_DAC_AND_MIC_CTRL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5630_DAC_AND_MIC_CTRL, 10, 1, 1),
SOC_DAPM_SINGLE("DAC Mixer Playback Switch", RT5630_DAC_AND_MIC_CTRL, 1, 1, 1),
SOC_DAPM_SINGLE("Voice DAC Playback Switch", RT5630_VOICE_DAC_OUT_VOL, 14, 1, 1),
};

static int mixer_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = w->codec;
	unsigned int l, r;

	RT5630_DEBUG("enter %s\n");

	l= rt5630_read(codec, HPL_MIXER);
	r = rt5630_read(codec, HPR_MIXER);
	
	if ((l & 0x1) || (r & 0x1))
		rt5630_write_mask(codec, 0x0a, 0x0000, 0x8000);
	else
		rt5630_write_mask(codec, 0x0a, 0x8000, 0x8000);

	if ((l & 0x2) || (r & 0x2))
		rt5630_write_mask(codec, 0x08, 0x0000, 0x8000);
	else
		rt5630_write_mask(codec, 0x08, 0x8000, 0x8000);

	if ((l & 0x4) || (r & 0x4))
		rt5630_write_mask(codec, 0x10, 0x0000, 0x8000);
	else
		rt5630_write_mask(codec, 0x10, 0x8000, 0x8000);

	if ((l & 0x8) || (r & 0x8))
		rt5630_write_mask(codec, 0x10, 0x0000, 0x0800);
	else
		rt5630_write_mask(codec, 0x10, 0x0800, 0x0800);

	if ((l & 0x10) || (r & 0x10))
		rt5630_write_mask(codec, 0x18, 0x0000, 0x8000);
	else
		rt5630_write_mask(codec, 0x18, 0x8000, 0x8000);

	return 0;
}


/*
 *	bit[0][1] use for aec control
 *	bit[2][3] for ADCR func
 *	bit[4] for SPKL pga
 *	bit[5] for SPKR pga
 *	bit[6] for hpl pga
 *	bit[7] for hpr pga
 */
static int spk_pga_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
 {
	struct snd_soc_codec *codec = w->codec;
	int reg;
	
	RT5630_DEBUG("enter %s\n");
	reg = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC) & (0x3 << 4);
	if ((reg >> 4) != 0x3 && reg != 0)
		return 0;

	
	
	switch (event)
	{
		case SND_SOC_DAPM_POST_PMU:
			RT5630_DEBUG("after virtual spk power up!\n");
			rt5630_write_mask(codec, 0x3e, 0x3000, 0x3000);
			rt5630_write_mask(codec, 0x02, 0x0000, 0x8080);
			rt5630_write_mask(codec, 0x3a, 0x0400, 0x0400);                  //power on spk amp
			break;
		case SND_SOC_DAPM_POST_PMD:
			RT5630_DEBUG("aftet virtual spk power down!\n");
			rt5630_write_mask(codec, 0x3a, 0x0000, 0x0400);//power off spk amp
			rt5630_write_mask(codec, 0x02, 0x8080, 0x8080);
			rt5630_write_mask(codec, 0x3e, 0x0000, 0x3000);                 
			break;
		default:
			return 0;
	}
	return 0;
}





static int hp_pga_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = w->codec;
	int reg;

	RT5630_DEBUG("enter %s\n");
	reg = rt5630_read(codec, VIRTUAL_REG_FOR_MISC_FUNC) & (0x3 << 6);
	if ((reg >> 6) != 0x3 && reg != 0)
		return 0;
	
	switch (event)
	{
		case SND_SOC_DAPM_POST_PMD:
			RT5630_DEBUG("aftet virtual hp power down!\n");
			rt5630_write_mask(codec, 0x04, 0x8080, 0x8080);
			rt5630_write_mask(codec, 0x3e, 0x0000, 0x0600);
			rt5630_write_mask(codec, 0x3a, 0x0000, 0x0030);
			break;
		case SND_SOC_DAPM_POST_PMU:	
			RT5630_DEBUG("after virtual hp power up!\n");
			hp_depop_mode2(codec);
			rt5630_write_mask(codec ,0x04, 0x0000, 0x8080);
			break;
		default:
			return 0;
	}	
	return 0;
}



static int aux_pga_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	return 0;
}

/*SPKOUT Mux*/
static const struct snd_kcontrol_new rt5630_spkout_mux_out_controls = 
SOC_DAPM_ENUM("Route", rt5630_enum[3]);

/*HPLOUT MUX*/
static const struct snd_kcontrol_new rt5630_hplout_mux_out_controls = 
SOC_DAPM_ENUM("Route", rt5630_enum[4]);

/*HPROUT MUX*/
static const struct snd_kcontrol_new rt5630_hprout_mux_out_controls = 
SOC_DAPM_ENUM("Route", rt5630_enum[5]);
/*AUXOUT MUX*/
static const struct snd_kcontrol_new rt5630_auxout_mux_out_controls = 
SOC_DAPM_ENUM("Route", rt5630_enum[6]);

static const struct snd_soc_dapm_widget rt5630_dapm_widgets[] = {
SND_SOC_DAPM_INPUT("Left LineIn"),
SND_SOC_DAPM_INPUT("Right LineIn"),
SND_SOC_DAPM_INPUT("Phone"),
SND_SOC_DAPM_INPUT("Mic1"),
SND_SOC_DAPM_INPUT("Mic2"),

SND_SOC_DAPM_PGA("Mic1 Boost", RT5630_PWR_MANAG_ADD3, 1, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic2 Boost", RT5630_PWR_MANAG_ADD3, 0, 0, NULL, 0),

SND_SOC_DAPM_DAC("Left DAC", "Left HiFi Playback DAC", RT5630_PWR_MANAG_ADD2, 9, 0),
SND_SOC_DAPM_DAC("Right DAC", "Right HiFi Playback DAC", RT5630_PWR_MANAG_ADD2, 8, 0),
SND_SOC_DAPM_DAC("Voice DAC", "Voice Playback DAC", RT5630_PWR_MANAG_ADD2, 10, 0),

SND_SOC_DAPM_PGA("Left LineIn PGA", RT5630_PWR_MANAG_ADD3, 7, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right LineIn PGA", RT5630_PWR_MANAG_ADD3, 6, 0, NULL, 0),
SND_SOC_DAPM_PGA("Phone PGA", RT5630_PWR_MANAG_ADD3, 5, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic1 PGA", RT5630_PWR_MANAG_ADD3, 3, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic2 PGA", RT5630_PWR_MANAG_ADD3, 2, 0, NULL, 0),

SND_SOC_DAPM_MIXER("Left Rec Mixer", RT5630_PWR_MANAG_ADD2, 1, 0,
	&rt5630_left_adc_rec_mixer_controls[0], ARRAY_SIZE(rt5630_left_adc_rec_mixer_controls)),
SND_SOC_DAPM_MIXER("Right Rec Mixer", RT5630_PWR_MANAG_ADD2, 0, 0,
	&rt5630_right_adc_rec_mixer_controls[0], ARRAY_SIZE(rt5630_right_adc_rec_mixer_controls)),
SND_SOC_DAPM_MIXER_E("Left HP Mixer", RT5630_PWR_MANAG_ADD2, 5, 0,
	&rt5630_left_hp_mixer_controls[0], ARRAY_SIZE(rt5630_left_hp_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER_E("Right HP Mixer", RT5630_PWR_MANAG_ADD2, 4, 0,
	&rt5630_right_hp_mixer_controls[0], ARRAY_SIZE(rt5630_right_hp_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER("MoNo Mixer", RT5630_PWR_MANAG_ADD2, 2, 0, 
	&rt5630_mono_mixer_controls[0], ARRAY_SIZE(rt5630_mono_mixer_controls)),
SND_SOC_DAPM_MIXER("SPK Mixer", RT5630_PWR_MANAG_ADD2, 3, 0,
	&rt5630_spk_mixer_controls[0], ARRAY_SIZE(rt5630_spk_mixer_controls)),
	
/*hpl mixer --> hp mixer-->spkout mux, hpr mixer-->hp mixer -->spkout mux
   hpl mixer-->hp mixer-->Auxout Mux, hpr muxer-->hp mixer-->auxout mux*/	
SND_SOC_DAPM_MIXER("HP Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("DAC Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("Line Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),

SND_SOC_DAPM_MUX("SPKOUT Mux", SND_SOC_NOPM, 0, 0, &rt5630_spkout_mux_out_controls),
SND_SOC_DAPM_MUX("HPLOUT Mux", SND_SOC_NOPM, 0, 0, &rt5630_hplout_mux_out_controls),
SND_SOC_DAPM_MUX("HPROUT Mux", SND_SOC_NOPM, 0, 0, &rt5630_hprout_mux_out_controls),
SND_SOC_DAPM_MUX("AUXOUT Mux", SND_SOC_NOPM, 0, 0, &rt5630_auxout_mux_out_controls),

SND_SOC_DAPM_PGA_E("SPKL Out PGA", VIRTUAL_REG_FOR_MISC_FUNC, 4, 0, NULL, 0,
				spk_pga_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
SND_SOC_DAPM_PGA_E("SPKR Out PGA", VIRTUAL_REG_FOR_MISC_FUNC, 5, 0, NULL, 0,
				spk_pga_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
SND_SOC_DAPM_PGA_E("HPL Out PGA",VIRTUAL_REG_FOR_MISC_FUNC, 6, 0, NULL, 0, 
				hp_pga_event, SND_SOC_DAPM_POST_PMD | SND_SOC_DAPM_POST_PMU),
SND_SOC_DAPM_PGA_E("HPR Out PGA",VIRTUAL_REG_FOR_MISC_FUNC, 7, 0, NULL, 0, 
				hp_pga_event, SND_SOC_DAPM_POST_PMD | SND_SOC_DAPM_POST_PMU),
SND_SOC_DAPM_PGA_E("AUX Out PGA",RT5630_PWR_MANAG_ADD3, 14, 0, NULL, 0, 
				aux_pga_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
				

SND_SOC_DAPM_ADC("Left ADC", "Left ADC HiFi Capture", RT5630_PWR_MANAG_ADD2, 7, 0),
SND_SOC_DAPM_ADC("Right ADC", "Right ADC HiFi Capture", RT5630_PWR_MANAG_ADD2, 6, 0),
SND_SOC_DAPM_OUTPUT("SPKL"),
SND_SOC_DAPM_OUTPUT("SPKR"),
SND_SOC_DAPM_OUTPUT("HPL"),
SND_SOC_DAPM_OUTPUT("HPR"),
SND_SOC_DAPM_OUTPUT("AUX"),
SND_SOC_DAPM_MICBIAS("Mic1 Bias", RT5630_PWR_MANAG_ADD1, 3, 0),
SND_SOC_DAPM_MICBIAS("Mic2 Bias", RT5630_PWR_MANAG_ADD1, 2, 0),
};

static const struct snd_soc_dapm_route audio_map[] = {
		/*Input PGA*/

		{"Left LineIn PGA", NULL, "Left LineIn"},
		{"Right LineIn PGA", NULL, "Right LineIn"},
		{"Phone PGA", NULL, "Phone"},
		{"Mic1 Boost", NULL, "Mic1"},
		{"Mic2 Boost", NULL, "Mic2"},
		{"Mic1 PGA", NULL, "Mic1 Boost"},
		{"Mic2 PGA", NULL, "Mic2 Boost"},

		/*Left ADC mixer*/
		{"Left Rec Mixer", "LineIn Capture Switch", "Left LineIn"},
		{"Left Rec Mixer", "Phone Capture Switch", "Phone"},
		{"Left Rec Mixer", "Mic1 Capture Switch", "Mic1"},
		{"Left Rec Mixer", "Mic2 Capture Switch", "Mic2"},
		{"Left Rec Mixer", "HP Mixer Capture Switch", "Left HP Mixer"},
		{"Left Rec Mixer", "SPK Mixer Capture Switch", "SPK Mixer"},
		{"Left Rec Mixer", "MoNo Mixer Capture Switch", "MoNo Mixer"},

		/*Right ADC Mixer*/
		{"Right Rec Mixer", "LineIn Capture Switch", "Right LineIn"},
		{"Right Rec Mixer", "Phone Capture Switch", "Phone"},
		{"Right Rec Mixer", "Mic1 Capture Switch", "Mic1"},
		{"Right Rec Mixer", "Mic2 Capture Switch", "Mic2"},
		{"Right Rec Mixer", "HP Mixer Capture Switch", "Right HP Mixer"},
		{"Right Rec Mixer", "SPK Mixer Capture Switch", "SPK Mixer"},
		{"Right Rec Mixer", "MoNo Mixer Capture Switch", "MoNo Mixer"},
		
		/*HPL mixer*/
		{"Left HP Mixer", "ADC Playback Switch", "Left Rec Mixer"},
		{"Left HP Mixer", "LineIn Playback Switch", "Left LineIn PGA"},
		{"Left HP Mixer", "Phone Playback Switch", "Phone PGA"},
		{"Left HP Mixer", "Mic1 Playback Switch", "Mic1 PGA"},
		{"Left HP Mixer", "Mic2 Playback Switch", "Mic2 PGA"},
		{"Left HP Mixer", "HIFI DAC Playback Switch", "Left DAC"},
		{"Left HP Mixer", "Voice DAC Playback Switch", "Voice DAC"},
		
		/*HPR mixer*/
		{"Right HP Mixer", "ADC Playback Switch", "Right Rec Mixer"},
		{"Right HP Mixer", "LineIn Playback Switch", "Right LineIn PGA"},	
		{"Right HP Mixer", "HIFI DAC Playback Switch", "Right DAC"},
		{"Right HP Mixer", "Phone Playback Switch", "Phone PGA"},
		{"Right HP Mixer", "Mic1 Playback Switch", "Mic1 PGA"},
		{"Right HP Mixer", "Mic2 Playback Switch", "Mic2 PGA"},
		{"Right HP Mixer", "Voice DAC Playback Switch", "Voice DAC"},

		/*DAC Mixer*/
		{"DAC Mixer", NULL, "Left DAC"},
		{"DAC Mixer", NULL, "Right DAC"},

		/*line mixer*/
		{"Line Mixer", NULL, "Left LineIn PGA"},
		{"Line Mixer", NULL, "Right LineIn PGA"},

		/*spk mixer*/
		{"SPK Mixer", "Line Mixer Playback Switch", "Line Mixer"},
		{"SPK Mixer", "Phone Playback Switch", "Phone PGA"},
		{"SPK Mixer", "Mic1 Playback Switch", "Mic1 PGA"},
		{"SPK Mixer", "Mic2 Playback Switch", "Mic2 PGA"},
		{"SPK Mixer", "DAC Mixer Playback Switch", "DAC Mixer"},
		{"SPK Mixer", "Voice DAC Playback Switch", "Voice DAC"},

		/*mono mixer*/
		{"MoNo Mixer", "Line Mixer Playback Switch", "Line Mixer"},
		{"MoNo Mixer", "ADCL Playback Switch","Left Rec Mixer"},
		{"MoNo Mixer", "ADCR Playback Switch","Right Rec Mixer"},
		{"MoNo Mixer", "Mic1 Playback Switch", "Mic1 PGA"},
		{"MoNo Mixer", "Mic2 Playback Switch", "Mic2 PGA"},
		{"MoNo Mixer", "DAC Mixer Playback Switch", "DAC Mixer"},
		{"MoNo Mixer", "Voice DAC Playback Switch", "Voice DAC"},
		
		/*hp mixer*/
		{"HP Mixer", NULL, "Left HP Mixer"},
		{"HP Mixer", NULL, "Right HP Mixer"},

		/*spkout mux*/
		{"SPKOUT Mux", "HP Mixer", "HP Mixer"},
		{"SPKOUT Mux", "SPK Mixer", "SPK Mixer"},
		{"SPKOUT Mux", "Mono Mixer", "MoNo Mixer"},
		
		/*hpl out mux*/
		{"HPLOUT Mux", "HPL Mixer", "Left HP Mixer"},
		
		/*hpr out mux*/
		{"HPROUT Mux", "HPR Mixer", "Right HP Mixer"},

		/*aux out mux*/
		{"AUXOUT Mux", "HP Mixer", "HP Mixer"},
		{"AUXOUT Mux", "SPK Mixer", "SPK Mixer"},
		{"SPKOUT Mux", "Mono Mixer", "MoNo Mixer"},

		/*spkl out pga*/
		{"SPKL Out PGA", NULL, "SPKOUT Mux"},
		
		
		/*spkr out pga*/
		{"SPKR Out PGA", NULL, "SPKOUT Mux"},
		
		/*hpl out pga*/
		{"HPL Out PGA", NULL, "HPLOUT Mux"},

		/*hpr out pga*/
		{"HPR Out PGA", NULL, "HPROUT Mux"},

		/*aux out pga*/
		{"AUX Out PGA", NULL, "AUXOUT Mux"}, 
		
		/*left adc*/
		{"Left ADC", NULL, "Left Rec Mixer"},
		
		/*right adc*/
		{"Right ADC", NULL, "Right Rec Mixer"},
		
		/*output*/
		{"SPKL", NULL, "SPKL Out PGA"},
		{"SPKR", NULL, "SPKR Out PGA"},
		{"HPL", NULL, "HPL Out PGA"},
		{"HPR", NULL, "HPR Out PGA"},
		{"AUX", NULL, "AUX Out PGA"},
};


static int rt5630_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, rt5630_dapm_widgets, 
				ARRAY_SIZE(rt5630_dapm_widgets));
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));
	snd_soc_dapm_new_widgets(codec);
}

#endif
#if !USE_DAPM_CTRL

static enum OUTPUT_DEVICE_MASK
{
	SPK_OUTPUT_MASK = 1,
	HP_OUTPUT_MASK,
};



static int rt5630_set_path_from_dac_to_output(struct snd_soc_codec *codec, int enable, int sink)
{
	switch (sink)
	{
		case SPK_OUTPUT_MASK:
			if (enable)
			{	
				rt5630_write_mask(codec, 0x10, 0x000c, 0x000c);          /*unmute dac-->hpmixer*/
				rt5630_write_mask(codec, 0x1c, 0x0400, 0x0400);  	 /*choose mux to hpmixer-->spk  */		
				rt5630_write_mask(codec, 0x3c, 0x0300, 0x0300);  	/*power up dac lr */
				rt5630_write_mask(codec, 0x3c, 0x0030, 0x0030);  	/*power up hp mixer lr*/
				rt5630_write_mask(codec, 0x3c, 0x3000, 0x3000);      /*power up spk lr*/
				
			}
			else
				rt5630_write_mask(codec, 0x02, 0x8080, 0x8080);      /*mute spk*/			
			break;
		case HP_OUTPUT_MASK:
			if (enable)
			{
				rt5630_write_mask(codec, 0x10, 0x000c, 0x000c);          /*unmute dac-->hpmixer*/
				rt5630_write_mask(codec, 0x1c, 0x0300, 0x0300);          /*hpmixer to hp out*/
				rt5630_write_mask(codec, 0x3c, 0x0300, 0x0300);  	/*power up dac lr */
				rt5630_write_mask(codec, 0x3c, 0x0030, 0x0030);  	/*power up hp mixer lr*/
				hp_depop_mode2(codec);
			}
			else
			{
				rt5630_write_mask(codec, 0x3a, 0x0000, 0x0300);                     /*power off hp amp*/
				rt5630_write_mask(codec, 0x04, 0x8080, 0x8080);
			}
			break;
		default:
		return 0;
	}
	return 0;
}


#endif

#if !USE_DAPM_CTRL
static int rt5630_pcm_hw_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int stream = substream->stream;

	switch (stream)
	{
		case SNDRV_PCM_STREAM_PLAYBACK:
			RT5630_DEBUG("enter %s\n");
			rt5630_write_mask(codec, 0x3c, 0x0300, 0x0300);        /*power daclr*/
			rt5630_write_mask(codec, 0x3c, 0x0030, 0x0030);       /*power hp mixerlr*/
			hp_depop_mode2(codec);
			rt5630_write_mask(codec, 0x3e, 0x3000, 0x3000);       /*power spklr volume*/
			rt5630_write_mask(codec, 0x04, 0x0000, 0x8080);        /*unmute hp*/
			rt5630_write_mask(codec, 0x02, 0x0000, 0x8080);        /*unmute spk*/
			rt5630_write_mask(codec, 0x3a, 0x0400, 0x0400);        /*power on classabd amp*/		
			break;
		case SNDRV_PCM_STREAM_CAPTURE:
			rt5630_write_mask(codec, 0x3e, 0x000F, 0x000F);        /*power mic1,mic2 boost*/
			rt5630_write_mask(codec, 0x3a, 0x000e, 0x000e);        /*mic bias*/
//			rt5630_write_mask(codec, 0x3e, 0x0002, 0x0002);        /*power mic1 boost*/
//			rt5630_write_mask(codec, 0x3a, 0x0008, 0x0008);        /*mic bias*/
			rt5630_write_mask(codec, 0x3c, 0x0003, 0x0003);        /*power adc rec mixer lr*/
			rt5630_write_mask(codec, 0x3c, 0x00c0, 0x00c0);         /*power adc lr*/
			break;
		default:
			return 0;
	}
	return 0;	
}

static int rt5630_vopcm_hw_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int stream = substream->stream;

	switch (stream)
	{
		case SNDRV_PCM_STREAM_PLAYBACK:
			rt5630_write_mask(codec, 0x3c, 0x0430, 0x0430);
			hp_depop_mode2(codec);
			rt5630_write_mask(codec, 0x3e, 0x3000, 0x3000); 
			rt5630_write_mask(codec, 0x04, 0x0000, 0x8080); 
			rt5630_write_mask(codec, 0x02, 0x0000, 0x8080);
			rt5630_write_mask(codec, 0x3a, 0x0400, 0x0400);  
			break;
		case SNDRV_PCM_STREAM_CAPTURE: 
			rt5630_write_mask(codec, 0x3e, 0x000f, 0x000f);   
			rt5630_write_mask(codec, 0x3a, 0x000e, 0x000e);        /*mic bias*/
//			rt5630_write_mask(codec, 0x3e, 0x0002, 0x0002);   
//			rt5630_write_mask(codec, 0x3a, 0x0008, 0x0008);        /*mic bias*/
			rt5630_write_mask(codec, 0x3c, 0x0041, 0x0041);  		
	}
	return 0;
}
#endif
struct _pll_div{
	u32 pll_in;
	u32 pll_out;
	u16 regvalue;
};


/**************************************************************
  *	note:
  *	our codec support you to select different source as pll input, but if you 
  *	use both of the I2S audio interface and pcm interface instantially. 
  *	The two DAI must have the same pll setting params, so you have to offer
  *	the same pll input, and set our codec's sysclk the same one, we suggest 
  *	24576000.
  **************************************************************/
static const struct _pll_div codec_master_pll1_div[] = {
		
	{  2048000,  24576000,	0x2ea0},
	{  3686400,  24576000,	0xee27},	
	{ 12000000,  24576000,	0x2915},   
	{ 13000000,  24576000,	0x772e},
	{ 13100000,	 24576000,	0x0d20},	
};

static const struct _pll_div codec_bclk_pll1_div[] = {

	{  1536000,	 24576000,	0x3ea0},	
	{  3072000,	 24576000,	0x1ea0},
};

static const struct _pll_div codec_vbclk_pll1_div[] = {

	{  1536000,	 24576000,	0x3ea0},	
	{  3072000,	 24576000,	0x1ea0},
};


struct _coeff_div_stereo {
	unsigned int mclk;
	unsigned int rate;
	unsigned int reg60;
	unsigned int reg62;
};

struct _coeff_div_voice {
	unsigned int mclk;
	unsigned int rate;
	unsigned int reg64;
};

static const struct _coeff_div_stereo coeff_div_stereo[] = {
		/*bclk is config to 32fs, if codec is choose to be slave mode , input bclk should be 32*fs */
		{24576000, 48000, 0x3174, 0x1010},                 
		{12288000, 48000, 0x1174, 0x0000},
		{18432000, 48000, 0x2174, 0x1111},
		{36864000, 48000, 0x2274, 0x2020},
		{49152000, 48000, 0xf074, 0x3030},
		{0, 0, 0, 0},
};

static const struct _coeff_div_voice coeff_div_voice[] = {
		/*bclk is config to 32fs, if codec is choose to be slave mode , input bclk should be 32*fs */
		{24576000, 16000, 0x2622}, 
		{24576000, 8000, 0x2824},
		{0, 0, 0},
};

static int get_coeff(unsigned int mclk, unsigned int rate, int mode)
{
	int i;

	RT5630_DEBUG("get_coeff mclk = %d, rate = %d\n", mclk, rate);
	if (!mode){
		for (i = 0; i < ARRAY_SIZE(coeff_div_stereo); i++) {
			if ((coeff_div_stereo[i].rate == rate) && (coeff_div_stereo[i].mclk == mclk))
				return i;
		}
	}
	else {
		for (i = 0; i< ARRAY_SIZE(coeff_div_voice); i++) {
			if ((coeff_div_voice[i].rate == rate) && (coeff_div_voice[i].mclk == mclk))
				return i;
		}
	}

	return -EINVAL;
	RT5630_DEBUG("can't find a matched mclk and rate in %s\n", 
				(mode ? "coeff_div_voice[]" : "coeff_div_audio[]"));
}

int rt5630_codec_set_pll1(struct snd_soc_codec *codec, int pll_id, 
		unsigned int freq_in, unsigned  int freq_out)
{
	int i;
	int ret = -EINVAL;

	RT5630_DEBUG("enter %s\n");

	if (pll_id < RT5630_PLL1_FROM_MCLK || pll_id > RT5630_PLL1_FROM_VBCLK)
		return -EINVAL;

	if (!freq_in || !freq_out)
		return 0;

	if (RT5630_PLL1_FROM_MCLK == pll_id)
	{
		for (i = 0; i < ARRAY_SIZE(codec_master_pll1_div); i ++)
		{
			if ((freq_in == codec_master_pll1_div[i].pll_in) && (freq_out == codec_master_pll1_div[i].pll_out))
			{
				rt5630_write(codec, RT5630_GEN_CTRL_REG2, 0x0000);                    			 /*PLL source from MCLK*/
				rt5630_write(codec, RT5630_PLL_CTRL, codec_master_pll1_div[i].regvalue);   /*set pll code*/
				rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD2, 0x8000, 0x8000);			/*enable pll1 power*/
				ret = 0;
			}
		}
	}
	else if (RT5630_PLL1_FROM_BCLK == pll_id)
	{
		for (i = 0; i < ARRAY_SIZE(codec_bclk_pll1_div); i ++)
		{
			if ((freq_in == codec_bclk_pll1_div[i].pll_in) && (freq_out == codec_bclk_pll1_div[i].pll_out))
			{
				rt5630_write(codec, RT5630_GEN_CTRL_REG2, 0x2000);    					/*PLL source from BCLK*/
				rt5630_write(codec, RT5630_PLL_CTRL, codec_bclk_pll1_div[i].regvalue);   /*set pll1 code*/
				rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD2, 0x8000, 0x8000);       	 /*enable pll1 power*/
				ret = 0;
			}
		}
	}
	else if (RT5630_PLL1_FROM_VBCLK == pll_id)
	{
		for (i = 0; i < ARRAY_SIZE(codec_vbclk_pll1_div); i ++)
		{
			if ((freq_in == codec_vbclk_pll1_div[i].pll_in) && (freq_out == codec_vbclk_pll1_div[i].pll_out))
			{
				rt5630_write(codec, RT5630_GEN_CTRL_REG2, 0x2000);    					/*PLL source from BCLK*/
				rt5630_write(codec, RT5630_PLL_CTRL, codec_vbclk_pll1_div[i].regvalue);   /*set pll1 code*/
				rt5630_write_mask(codec, RT5630_PWR_MANAG_ADD2, 0x8000, 0x8000);       	 /*enable pll1 power*/
				ret = 0;
			}
		}
	}
	
	rt5630_write_mask(codec, RT5630_GEN_CTRL_REG1, 0x8000, 0x8000);
	return ret;
}

EXPORT_SYMBOL_GPL(rt5630_codec_set_pll1);

static int rt5630_codec_set_pll2(struct snd_soc_codec *codec, int times)
{
	int iface = 0;
	
	rt5630_write_mask(codec, 0x3c, 0x4000, 0x4000);             //power on pll2

	switch (times)
	{
		case 8:
			break;	
		case 16:
			iface |= 0x0001;
		break;
		default:
			return -EINVAL;
	}
	iface |= 0x8000;
	rt5630_write(codec, 0x46, iface);

	return 0;
	
}

static int rt5630_codec_set_dai_pll(struct snd_soc_dai *codec_dai, 
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int times;

	times = freq_out / freq_in;

	if (pll_id < RT5630_PLL2_FROM_VBCLK)
		return rt5630_codec_set_pll1(codec, pll_id, freq_in, freq_out);
	else
		return rt5630_codec_set_pll2(codec, times);
}


static int rt5630_hifi_codec_set_dai_sysclk(struct snd_soc_dai *codec_dai, 
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5630_priv * rt5630 = codec->private_data;
	RT5630_DEBUG("enter %s\n");
	
	if ((freq >= (256 * 8000)) && (freq <= (512 * 48000))) {
		rt5630->stereo_sysclk = freq;
		return 0;
	}
	
	RT5630_DEBUG("unsupported sysclk freq %u for audio i2s\n", freq);
	return -EINVAL;
}

static int rt5630_voice_codec_set_dai_sysclk(struct snd_soc_dai *codec_dai, 
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5630_priv * rt5630 = codec->private_data;
	

	if ((freq >= (256 * 8000)) && (freq <= (512 * 48000))) {
		rt5630->voice_sysclk = freq;
		return 0;
	}			

	RT5630_DEBUG("unsupported sysclk freq %u for voice pcm\n", freq);
	return -EINVAL;
}


static int rt5630_hifi_pcm_hw_params(struct snd_pcm_substream *substream, 
			struct snd_pcm_hw_params *params,
			struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
//	struct rt5630_priv *rt5630 = codec->private_data;
	struct snd_soc_dapm_widget *w;
	int stream = substream->stream;
	unsigned int iface = rt5630_read(codec, RT5630_MAIN_SDP_CTRL) & 0xfff3;
	int rate = params_rate(params);
//	int coeff = get_coeff(rt5630->stereo_sysclk, rate, 0);
	
	RT5630_DEBUG("enter %s\n");

	if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		list_for_each_entry(w, &codec->dapm_widgets, list)
		{
			if (!w->sname)
				continue;
			if (!strcmp(w->name, "Right ADC"))
				strcpy(w->sname, "Right ADC HiFi Capture");
		}
	}
	
	switch (params_format(params))
	{
		case SNDRV_PCM_FORMAT_S16_LE:
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0004;
		case SNDRV_PCM_FORMAT_S24_LE:
			iface |= 0x0008;
		case SNDRV_PCM_FORMAT_S8:
			iface |= 0x000c;
	}
	rt5630_write(codec, RT5630_MAIN_SDP_CTRL, iface);
	rt5630_write_mask(codec, 0x3a, 0x0801, 0x0801);   /*power i2s and dac ref*/
//	if (coeff >= 0) {
//		rt5630_write(codec, RT5630_STEREO_DAC_CLK_CTRL1, coeff_div_stereo[coeff].reg60);
//		rt5630_write(codec, RT5630_STEREO_DAC_CLK_CTRL2, coeff_div_stereo[coeff].reg62);
		rt5630_write(codec, RT5630_STEREO_DAC_CLK_CTRL1, coeff_div_stereo[0].reg60);
		rt5630_write(codec, RT5630_STEREO_DAC_CLK_CTRL2, coeff_div_stereo[0].reg62);

//	}
//	else
//		return coeff;
	
	return 0;
}

static int rt5630_voice_pcm_hw_params(struct snd_pcm_substream *substream, 
			struct snd_pcm_hw_params *params,
			struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
//	struct rt5630_priv *rt5630 = codec->private_data;
	unsigned int iface = rt5630_read(codec, RT5630_EXTEND_SDP_CTRL) & 0xfff3;
	struct snd_soc_dapm_widget *w;
	int stream = substream->stream;
	int rate = params_rate(params);
//	int coeff = get_coeff(rt5630->voice_sysclk, rate, 1);

	RT5630_DEBUG("enter %s\n");

	if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		list_for_each_entry(w, &codec->dapm_widgets, list)
		{
			if (!w->sname)
				continue;
			if (!strcmp(w->name, "Right ADC"))
				strcpy(w->sname, "Right ADC Voice Capture");
		}
		rt5630_write_mask(codec, 0x2e, 0x0010, 0x0030);                           /*set adcr to be voice adc*/
	}
	switch (params_format(params))
	{
		case SNDRV_PCM_FORMAT_S16_LE:
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0004;
		case SNDRV_PCM_FORMAT_S24_LE:
			iface |= 0x0008;
		case SNDRV_PCM_FORMAT_S8:
			iface |= 0x000c;
	}
	rt5630_write_mask(codec, 0x3a, 0x0801, 0x0801);   /*power i2s and dac ref*/
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		rt5630_write_mask(codec, RT5630_DAC_ADC_VODAC_FUN_SEL, 0x0001, 0x0030);     /*set adcr to be stereo adcr func*/

//	if (coeff >= 0)
//		rt5630_write(codec, RT5630_VOICE_DAC_PCMCLK_CTRL1, coeff_div_voice[coeff].reg64);
		rt5630_write(codec, RT5630_VOICE_DAC_PCMCLK_CTRL1, coeff_div_voice[0].reg64);
//	else
//		return coeff;
	return 0;
}


static int rt5630_hifi_codec_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5630_priv *rt5630 = codec->private_data;
	u16 iface = 0;

	RT5630_DEBUG("enter %s\n");

	/*set master/slave interface*/
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
	{
		case SND_SOC_DAIFMT_CBM_CFM:
			iface = 0x0000;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:
			iface = 0x8000;
			break;
		default:
			return -EINVAL;
	}

	/*interface format*/
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK)
	{
		case SND_SOC_DAIFMT_I2S:
			iface |= 0x0000;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			iface |= 0x0001;
			break;
		case SND_SOC_DAIFMT_DSP_A:
			iface |= 0x0002;
			break;
		case SND_SOC_DAIFMT_DSP_B:
			iface |= 0x0003;
			break;
		default:
			return -EINVAL;			
	}

	/*clock inversion*/
	switch (fmt & SND_SOC_DAIFMT_INV_MASK)
	{
		case SND_SOC_DAIFMT_NB_NF:
			iface |= 0x0000;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			iface |= 0x0080;
			break;
		default:
			return -EINVAL;
	}

	rt5630_write(codec, RT5630_MAIN_SDP_CTRL, iface);
	return 0;
}

static int rt5630_voice_codec_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5630_priv *rt5630 = codec->private_data;
	int iface;

	RT5630_DEBUG("enter %s\n");
	/*set slave/master mode*/
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
	{
		case SND_SOC_DAIFMT_CBM_CFM:
			iface = 0x0000;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:
			iface = 0x4000;
			break;
		default:
			return -EINVAL;			
	}

	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK)
	{
		case SND_SOC_DAIFMT_I2S:
			iface |= 0x0000;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			iface |= 0x0001;
			break;
		case SND_SOC_DAIFMT_DSP_A:
			iface |= 0x0002;
			break;
		case SND_SOC_DAIFMT_DSP_B:
			iface |= 0x0003;
			break;
		default:
			return -EINVAL;		
	}

	/*clock inversion*/
	switch (fmt & SND_SOC_DAIFMT_INV_MASK)
	{
		case SND_SOC_DAIFMT_NB_NF:
			iface |= 0x0000;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			iface |= 0x0080;
			break;
		default:
			return -EINVAL;			
	}

	iface |= 0x8000;      /*enable vopcm*/
	rt5630_write(codec, RT5630_EXTEND_SDP_CTRL, iface);	
	
	return 0;
}


static int rt5630_hifi_codec_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	
	if (mute)
		rt5630_write_mask(codec, RT5630_STEREO_DAC_VOL, 0x8080, 0x8080);
	else
		rt5630_write_mask(codec, RT5630_STEREO_DAC_VOL, 0x0000, 0x8080);
	
	return 0;
}

static int rt5630_voice_codec_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	if (mute)
		rt5630_write_mask(codec, RT5630_VOICE_DAC_OUT_VOL, 0x1000, 0x1000);
	else 
		rt5630_write_mask(codec, RT5630_VOICE_DAC_OUT_VOL, 0x0000, 0x1000);
	return 0;
}


#if USE_DAPM_CTRL
static int rt5630_set_bias_level(struct snd_soc_codec *codec, 
			enum snd_soc_bias_level level)
{
	switch(level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_OFF:		
		rt5630_write_mask(codec, 0x3a, 0x0000, 0x0800);    /*enable main i2s*/
		rt5630_write_mask(codec, 0x3a, 0x0000, 0x0001);	/*enable dac ref*/
		rt5630_write_mask(codec, 0x3c, 0x0000, 0xc000);  /*off pll*/
		break;
	}
	codec->bias_level = level;
	return 0;
}
#else
static int rt5630_set_bias_level(struct snd_soc_codec *codec, 
			enum snd_soc_bias_level level)
{
#if 0
	switch(level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_OFF:
			rt5630_write_mask(codec, 0x3a, 0x0000, 0x0300);
			rt5630_write_mask(codec, 0x02, 0x8080, 0x8080);
			rt5630_write_mask(codec, 0x04, 0x8080, 0x8080);
			rt5630_write(codec, 0x3a, 0x0002);
			rt5630_write(codec, 0x3c, 0x2000);
			rt5630_write(codec, 0x3e, 0x8000);
		break;
	
	}
#endif
	codec->bias_level = level;
	return 0;
}
#endif


static int rt5630_voice_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	rt5630_write_mask(codec, 0x2e, 0x0000, 0x0030);
#if !USE_DAPM_CTRL
	rt5630_write_mask(codec, 0x3a, 0x0000, 0x0300);
	rt5630_write_mask(codec, 0x02, 0x8080, 0x8080);
	rt5630_write_mask(codec, 0x04, 0x8080, 0x8080);
	rt5630_write(codec, 0x3a, 0x0002);
	rt5630_write(codec, 0x3c, 0x2000);
	rt5630_write(codec, 0x3e, 0x8000);
#endif
	return 0;
}


static int rt5630_hifi_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	rt5630_write_mask(codec, 0x2e, 0x0000, 0x0030);
#if !USE_DAPM_CTRL
	rt5630_write_mask(codec, 0x3a, 0x0000, 0x0300);
	rt5630_write_mask(codec, 0x02, 0x8080, 0x8080);
	rt5630_write_mask(codec, 0x04, 0x8080, 0x8080);
	rt5630_write(codec, 0x3a, 0x0002);
	rt5630_write(codec, 0x3c, 0x2000);
	rt5630_write(codec, 0x3e, 0x8000);
#endif
	return 0;
}


#define RT5630_STEREO_RATES (SNDRV_PCM_RATE_48000)
#define RT5626_VOICE_RATES (SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_8000)

#define RT5630_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE |\
			SNDRV_PCM_FMTBIT_S8)


struct snd_soc_dai_ops rt5630_hifi_ops = {
		.hw_params = rt5630_hifi_pcm_hw_params,	
		.set_fmt = rt5630_hifi_codec_set_dai_fmt,
		.set_sysclk = rt5630_hifi_codec_set_dai_sysclk,
		.set_pll = rt5630_codec_set_dai_pll,
		#if !USE_DAPM_CTRL
		.prepare = rt5630_pcm_hw_prepare,
		#endif
		.shutdown = rt5630_hifi_shutdown,
};

struct snd_soc_dai_ops rt5630_voice_ops = {
		.hw_params = rt5630_voice_pcm_hw_params,
		.set_fmt = rt5630_voice_codec_set_dai_fmt,
		.set_sysclk = rt5630_voice_codec_set_dai_sysclk,
		.set_pll = rt5630_codec_set_dai_pll,
		#if !USE_DAPM_CTRL
		.prepare = rt5630_vopcm_hw_prepare,
		#endif
		.shutdown = rt5630_voice_shutdown,
};

struct snd_soc_dai rt5630_dai[] = {
	/*hifi codec dai*/
	{
		.name = "RT5630 HiFi",
		.id = 1,
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5630_STEREO_RATES,
			.formats = RT5630_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5630_STEREO_RATES,
			.formats = RT5630_FORMATS,
		},
		.ops = &rt5630_hifi_ops,
	},

	/*voice codec dai*/
	{
		.name = "RT5630 Voice",
		.id = 1,
		.playback = {
			.stream_name = "Voice Playback",
			.channels_min = 1,
			.channels_max =1,
			.rates = RT5626_VOICE_RATES,
			.formats = RT5630_FORMATS,
		},
		.capture = {
			.stream_name = "Voice Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = RT5626_VOICE_RATES,
			.formats = RT5630_FORMATS,
		},
		.ops = &rt5630_voice_ops,
	},
};

EXPORT_SYMBOL_GPL(rt5630_dai);


static void rt5630_work(struct work_struct *work)
{
	struct snd_soc_codec *codec =
		 container_of(work, struct snd_soc_codec, delayed_work.work);
	rt5630_set_bias_level(codec, codec->bias_level);
}



static int rt5630_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->card->codec;
	int reg, ret = 0;

	codec->name = "RT5630";
	codec->owner = THIS_MODULE;
	codec->read = rt5630_read;
	codec->write = rt5630_write;
	codec->set_bias_level = rt5630_set_bias_level;
	codec->dai= rt5630_dai;
	codec->num_dai = 2;
	codec->reg_cache_size = ARRAY_SIZE(rt5630_reg);
	codec->reg_cache = kmemdup(rt5630_reg, sizeof(rt5630_reg), GFP_KERNEL);
	if (codec->reg_cache == NULL)
		return -ENOMEM;

	rt5630_reset(codec);

	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0 )
	{
		RT5630_DEBUG("rt5630:  failed to create pcms\n");
		goto pcm_err;
	}
	
	rt5630_write(codec, RT5630_PD_CTRL_STAT, 0);
	rt5630_write(codec, RT5630_PWR_MANAG_ADD1, PWR_MAIN_BIAS);
	rt5630_write(codec, RT5630_PWR_MANAG_ADD2, PWR_MIXER_VREF);
	rt5630_reg_init(codec);
	init_vodsp(codec);
	rt5630_set_bias_level(codec, SND_SOC_BIAS_PREPARE);
	codec->bias_level = SND_SOC_BIAS_STANDBY;
	schedule_delayed_work(&codec->delayed_work, msecs_to_jiffies(1000));
	
	//rt5630_hifi_codec_mute(codec->dai, 1);
	//rt5630_voice_codec_mute(codec->dai, 1);
	
	rt5630_add_controls(codec);
#if USE_DAPM_CTRL
	rt5630_add_widgets(codec);
#endif
	ret = snd_soc_init_card(socdev);
	if (ret < 0)
	{
		RT5630_DEBUG("rt5630: failed to register card\n");
		goto card_err;
	}
	RT5630_DEBUG("rt5630: initial ok\n");
	return ret;

	card_err:
		snd_soc_free_pcms(socdev);
		snd_soc_dapm_free(socdev);
	
	pcm_err:
		kfree(codec->reg_cache);
		return ret;
	
	
}


static int rt5630_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct snd_soc_device *socdev = rt5630_socdev;
        struct snd_soc_codec *codec = socdev->card->codec;
	int ret;

	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = rt5630_init(socdev);
	if (ret < 0)
		pr_err("failed to initialise rt5630\n");

	return ret;
}

static int rt5630_i2c_remove(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	kfree(codec->reg_cache);
	return 0;
}

static const struct i2c_device_id rt5630_i2c_id[] = {
//		{"rt5630", 0},
		{"i2s_i2c",0},
		{}
};
MODULE_DEVICE_TABLE(i2c, rt5630_i2c_id);
static struct i2c_driver rt5630_i2c_driver = {
	.driver = {
		.name = "RT5630 I2C Codec",
		.owner = THIS_MODULE,
	},
	.probe =    rt5630_i2c_probe,
	.remove =   rt5630_i2c_remove,
	.id_table = rt5630_i2c_id,
};


static int rt5630_add_i2c_device(struct platform_device *pdev,
				 const struct rt5630_setup_data *setup)
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;

	ret = i2c_add_driver(&rt5630_i2c_driver);
	if (ret != 0) {
		dev_err(&pdev->dev, "can't add i2c driver\n");
		return ret;
	}
#if 0
	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = setup->i2c_address;
//	strlcpy(info.type, "rt5630", I2C_NAME_SIZE);
	strlcpy(info.type, "i2s_i2c", I2C_NAME_SIZE);


	adapter = i2c_get_adapter(setup->i2c_bus);
	if (!adapter) {
		dev_err(&pdev->dev, "can't get i2c adapter %d\n",
			setup->i2c_bus);
		goto err_driver;
	}

	client = i2c_new_device(adapter, &info);
	i2c_put_adapter(adapter);
	if (!client) {
		dev_err(&pdev->dev, "can't add i2c device at 0x%x\n",
			(unsigned int)info.addr);
		goto err_driver;
	}
#endif
	return 0;

err_driver:
	i2c_del_driver(&rt5630_i2c_driver);
	return -ENODEV;
}


static int rt5630_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct rt5630_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec;
	struct rt5630_priv *rt5630;
	int ret;

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	rt5630 = kzalloc(sizeof(struct rt5630_priv), GFP_KERNEL);
	if (rt5630 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

	codec->private_data = rt5630;
	socdev->card->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	rt5630_socdev = socdev;
	INIT_DELAYED_WORK(&codec->delayed_work, rt5630_work);

	ret = -ENODEV;
	if (setup->i2c_address) {
		codec->hw_write = (hw_write_t)i2c_master_send;
		codec->hw_read = (hw_read_t)i2c_master_recv;
		ret = rt5630_add_i2c_device(pdev, setup);
	}

	if (ret != 0) {
		kfree(codec->private_data);
		kfree(codec);
	}
	return ret;
}

static int run_delayed_work(struct delayed_work *dwork)
{
	int ret;

	/* cancel any work waiting to be queued. */
	ret = cancel_delayed_work(dwork);

	/* if there was any work waiting then we run it now and
	 * wait for it's completion */
	if (ret) {
		schedule_delayed_work(dwork, 0);
		flush_scheduled_work();
	}
	return ret;
}


static int rt5630_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
        struct snd_soc_codec *codec = socdev->card->codec;

	if (codec->control_data)
		rt5630_set_bias_level(codec, SND_SOC_BIAS_OFF);
	run_delayed_work(&codec->delayed_work);
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
	i2c_unregister_device(codec->control_data);
	i2c_del_driver(&rt5630_i2c_driver);
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

static int rt5630_suspend_reg[] = {0x3a, 0x3c, 0x3e};
static int rt5630_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
        struct snd_soc_codec *codec = socdev->card->codec;
	int i;
	u8 data[3];
	
	rt5630_set_bias_level(codec, SND_SOC_BIAS_OFF);
	for (i = 0; i < ARRAY_SIZE(rt5630_suspend_reg); i ++) {
		data[0] = rt5630_suspend_reg[i];
		data[1] = 0x00;
		data[2] = 0x00;
		codec->hw_write(codec->control_data, data, 3);
	}
	return 0;
}

static int rt5630_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
        struct snd_soc_codec *codec = socdev->card->codec;
	int i;
	u8 data[3];
	u16 *cache = codec->reg_cache;

	rt5630_write_mask(codec, 0x3a, 0x0002, 0x0002); 
	rt5630_write_mask(codec, 0x3c, 0x2000, 0x2000);  
	schedule_timeout_uninterruptible(msecs_to_jiffies(110));
	/* Sync reg_cache with the hardware */
	for (i = 0; i < ARRAY_SIZE(rt5630_reg); i++) {
		if (i == RT5630_RESET)
			continue;
		data[0] = i << 1;
		data[1] = (0xff00 & cache[i]) >> 8;
		data[2] = (0x00ff & cache[i]);
		codec->hw_write(codec->control_data, data, 3);
	}

	rt5630_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/* charge rt5630 caps */
	if (codec->suspend_bias_level == SND_SOC_BIAS_ON) {
		rt5630_set_bias_level(codec, SND_SOC_BIAS_PREPARE);
		codec->bias_level = SND_SOC_BIAS_ON;
		schedule_delayed_work(&codec->delayed_work,
					msecs_to_jiffies(1000));
	}

	return 0;
}


struct snd_soc_codec_device soc_codec_dev_rt5630 = {
	.probe = 	rt5630_probe,
	.remove = 	rt5630_remove,
	.suspend = 	rt5630_suspend,
	.resume =	rt5630_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_rt5630);

static int __init rt5630_modinit(void)
{
	return snd_soc_register_dais(rt5630_dai, ARRAY_SIZE(rt5630_dai));
}

static void __exit rt5630_exit(void)
{
	snd_soc_unregister_dais(rt5630_dai, ARRAY_SIZE(rt5630_dai));
}

module_init(rt5630_modinit);
module_exit(rt5630_exit);
MODULE_LICENSE("GPL");

