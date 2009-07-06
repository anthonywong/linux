/*
 * rt5610.c  --  RT5610 ALSA Soc Audio driver
 *
 * Copyright 2008 Realtek Microelectronics
 *
 * Author: flove <flove@realtek.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include "rt5610.h"

#define RT5610_VERSION "0.01"

struct rt5610_priv {
	u32 pll_in; /* PLL input frequency */
	u32 pll_out; /* PLL output frequency */
};

static unsigned int ac97_read(struct snd_soc_codec *codec,
	unsigned int reg);
static int ac97_write(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int val);

static int ac97_write_mask(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int val, unsigned int mask);
/*
 * RT5610 register cache
 * Reg 0x3c bit 15 is used by touch driver.
 */
static const u16 rt5610_reg[] = {
	0x59b4, 0x8080, 0x8080, 0x0000, // 6
	0xc880, 0xe808, 0xe808, 0x0808, // e
	0xe0e0, 0xf58b, 0x7f7f, 0x0000, // 16
	0xe800, 0x0000, 0x0000, 0x0000, // 1e
	0x0000, 0x0000, 0x0000, 0xef00, // 26
	0x0000, 0x0000, 0xbb80, 0x0000, // 2e
	0x0000, 0xbb80, 0x0000, 0x0000, // 36
	0x0000, 0x0000, 0x0000, 0x0000, // 3e
	0x0428, 0x0000, 0x0000, 0x0000, // 46
	0x0000, 0x0000, 0x2e3e, 0x2e3e, // 4e
	0x0000, 0x0000, 0x003a, 0x0000, // 56
	0x0cff, 0x0000, 0x0000, 0x0000, // 5e
	0x0000, 0x0000, 0x2130, 0x0010, // 66
	0x0053, 0x0000, 0x0000, 0x0000, // 6e
	0x0000, 0x0000, 0x008c, 0x3f00, // 76
	0x0000, 0x0000, 0x10ec, 0x1003, // 7e
	0x0000, 0x0000, 0x0000 // virtual hp & mic mixers
};

/* virtual HP mixers regs */
#define HPL_MIXER	0x80
#define HPR_MIXER	0x82
#define MICB_MUX	0x82


static u16 Set_Codec_Reg_Init[][2]={
	
	{RT5610_SPK_OUT_VOL		,0x8080},//default speaker volume to 0db 
	{RT5610_HP_OUT_VOL		,0x8888},//default HP volume to 0db
	{RT5610_ADC_REC_MIXER	,0x3F3F},//default Record is Mic1
	{RT5610_STEREO_DAC_VOL	,0x0808},//default stereo DAC volume to 0db
	{RT5610_MIC_CTRL		,0x0500},//set boost to +20DB
	{RT5610_INDEX_ADDRESS 	,0x0054},//AD_DA_Mixer_internal Register5
	{RT5610_INDEX_DATA		,0xE184},//To reduce power consumption for DAC reference
	{RT5610_TONE_CTRL		,0x0001},//Enable varible sample rate	
	{RT5610_OUTPUT_MIXER_CTRL,0x93C0},//default output mixer control,CLASS AB
};


static const char *rt5610_spkl_pga[] = {"Vmid","HPL mixer","SPK mixer","Mono Mixer"};
static const char *rt5610_spkr_pga[] = {"Vmid","HPR mixer","SPK mixer","Mono Mixer"};
static const char *rt5610_hpl_pga[]  = {"Vmid","HPL mixer"};
static const char *rt5610_hpr_pga[]  = {"Vmid","HPR mixer"};
static const char *rt5610_mono_pga[] = {"Vmid","HP mixer","SPK mixer","Mono Mixer"};
static const char *rt5610_amp_type_select[] = {"Class AB","Class D"};
static const char *rt5610_mic_boost_select[] = {"Bypass","20db","30db","40db"};


static const struct soc_enum rt5610_enum[] = {
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 14, 4, rt5610_spkl_pga), /* spk left input sel 0 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 11, 4, rt5610_spkr_pga), /* spk right input sel 1 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 9, 2, rt5610_hpl_pga), /* hp left input sel 2 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 8, 2, rt5610_hpr_pga), /* hp right input sel 3 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 6, 4, rt5610_mono_pga), /* mono input sel 4 */
SOC_ENUM_SINGLE(RT5610_MIC_CTRL, 10,4, rt5610_mic_boost_select), /*Mic1 boost sel 5 */
SOC_ENUM_SINGLE(RT5610_MIC_CTRL, 8,4, rt5610_mic_boost_select), /*Mic2 boost sel 6 */
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 12,2,rt5610_amp_type_select), /*Speaker AMP sel 7 */
};

static const struct snd_kcontrol_new rt5610_snd_ac97_controls[] = {
SOC_DOUBLE("Speaker Playback Volume", 	RT5610_SPK_OUT_VOL, 8, 0, 31, 1),	
SOC_DOUBLE("Speaker Playback Switch", 	RT5610_SPK_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE("Headphone Playback Volume", RT5610_HP_OUT_VOL, 8, 0, 31, 1),
SOC_DOUBLE("Headphone Playback Switch", RT5610_HP_OUT_VOL,15, 7, 1, 1),
SOC_SINGLE("Mono Playback Volume", 		RT5610_PHONEIN_MONO_OUT_VOL, 0, 31, 1),
SOC_SINGLE("Mono Playback Switch", 		RT5610_PHONEIN_MONO_OUT_VOL, 7, 1, 1),
SOC_DOUBLE("PCM Playback Volume", 		RT5610_STEREO_DAC_VOL, 8, 0, 31, 1),
SOC_DOUBLE("PCM Playback Switch", 		RT5610_STEREO_DAC_VOL,15, 7, 1, 1),
SOC_DOUBLE("Line In Volume", 			RT5610_LINE_IN_VOL, 8, 0, 31, 1),
SOC_SINGLE("Mic 1 Volume", 				RT5610_MIC_VOL, 8, 31, 1),
SOC_SINGLE("Mic 2 Volume", 				RT5610_MIC_VOL, 0, 31, 1),
SOC_ENUM("Mic 1 Boost", 				rt5610_enum[5]),
SOC_ENUM("Mic 2 Boost", 				rt5610_enum[6]),
SOC_ENUM("Speaker AMP", 				rt5610_enum[7]),
SOC_SINGLE("Phone In Volume", 			RT5610_PHONEIN_MONO_OUT_VOL, 8, 31, 1),
SOC_DOUBLE("Capture Volume", 			RT5610_ADC_REC_GAIN, 7, 0, 31, 0),
	};


/* add non dapm controls */
static int rt5610_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(rt5610_snd_ac97_controls); i++) {
		err = snd_ctl_add(codec->card,
				snd_soc_cnew(&rt5610_snd_ac97_controls[i],
				codec, NULL));
		if (err < 0)
			return err;
	}

	return 0;
}

/* We have to create a fake left and right HP mixers because
 * the codec only has a single control that is shared by both channels.
 * This makes it impossible to determine the audio path using the current
 * register map, thus we add a new (virtual) register to help determine the
 * audio route within the device.
 */
static int mixer_event (struct snd_soc_dapm_widget *w, int event)
{
	u16 l, r, lineIn,mic1,mic2, phone, pcm,voice;

	l = ac97_read(w->codec, HPL_MIXER);
	r = ac97_read(w->codec, HPR_MIXER);
	lineIn = ac97_read(w->codec, RT5610_LINE_IN_VOL);
	mic1 = ac97_read(w->codec, RT5610_MIC_ROUTING_CTRL);
	mic2 = ac97_read(w->codec, RT5610_MIC_ROUTING_CTRL);
	phone = ac97_read(w->codec,RT5610_PHONEIN_MONO_OUT_VOL);
	pcm = ac97_read(w->codec, RT5610_STEREO_DAC_VOL);
	voice = ac97_read(w->codec, RT5610_VOICE_DAC_OUT_VOL);

	if (event & SND_SOC_DAPM_PRE_REG)
		return 0;
	if (l & 0x1 || r & 0x1)
		ac97_write(w->codec, RT5610_VOICE_DAC_OUT_VOL, voice & 0x7fff);
	else
		ac97_write(w->codec, RT5610_VOICE_DAC_OUT_VOL, voice | 0x8000);

	if (l & 0x2 || r & 0x2)
		ac97_write(w->codec, RT5610_STEREO_DAC_VOL, pcm & 0x7fff);
	else
		ac97_write(w->codec, RT5610_STEREO_DAC_VOL, pcm | 0x8000);

	if (l & 0x4 || r & 0x4)
		ac97_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic2 & 0xf7ff);
	else
		ac97_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic2 | 0x0800);

	if (l & 0x8 || r & 0x8)
		ac97_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic1 & 0x7fff);
	else
		ac97_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic1 | 0x8000);

	if (l & 0x10 || r & 0x10)
		ac97_write(w->codec, RT5610_PHONEIN_MONO_OUT_VOL, phone & 0x7fff);
	else
		ac97_write(w->codec, RT5610_PHONEIN_MONO_OUT_VOL, phone | 0x8000);

	if (l & 0x20 || r & 0x20)
		ac97_write(w->codec, RT5610_LINE_IN_VOL, lineIn & 0x7fff);
	else
		ac97_write(w->codec, RT5610_LINE_IN_VOL, lineIn | 0x8000);

	return 0;
}

/* Left Headphone Mixers */
static const struct snd_kcontrol_new rt5610_hpl_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", HPL_MIXER, 5, 1, 0),
SOC_DAPM_SINGLE("PhoneIn Playback Switch", HPL_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPL_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPL_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("PCM Playback Switch", HPL_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("Voice Playback Switch", HPL_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("RecordL Playback Switch", RT5610_ADC_REC_GAIN, 15, 1,1),
};


/* Right Headphone Mixers */
static const struct snd_kcontrol_new rt5610_hpr_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", HPR_MIXER, 5, 1, 0),
SOC_DAPM_SINGLE("PhoneIn Playback Switch", HPR_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPR_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPR_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("PCM Playback Switch", HPR_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("Voice Playback Switch", HPR_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("RecordR Playback Switch", RT5610_ADC_REC_GAIN, 14, 1,1),
};


static const struct snd_kcontrol_new rt5610_captureL_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5610_ADC_REC_MIXER, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5610_ADC_REC_MIXER, 13, 1, 1),
SOC_DAPM_SINGLE("LineInL Capture Switch",RT5610_ADC_REC_MIXER,12, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", RT5610_ADC_REC_MIXER, 11, 1, 1),
SOC_DAPM_SINGLE("HPMixerL Capture Switch", RT5610_ADC_REC_MIXER,10, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",RT5610_ADC_REC_MIXER,9, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",RT5610_ADC_REC_MIXER,8, 1, 1),
};


static const struct snd_kcontrol_new rt5610_captureR_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5610_ADC_REC_MIXER, 6, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5610_ADC_REC_MIXER, 5, 1, 1),
SOC_DAPM_SINGLE("LineInR Capture Switch",RT5610_ADC_REC_MIXER,4, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", RT5610_ADC_REC_MIXER, 3, 1, 1),
SOC_DAPM_SINGLE("HPMixer Capture Switch", RT5610_ADC_REC_MIXER,2, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",RT5610_ADC_REC_MIXER,1, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",RT5610_ADC_REC_MIXER,0, 1, 1),
};


/* Speaker Mixer */
static const struct snd_kcontrol_new rt5610_speaker_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", RT5610_LINE_IN_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("PhoneIn Playback Switch", RT5610_PHONEIN_MONO_OUT_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5610_MIC_ROUTING_CTRL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5610_MIC_ROUTING_CTRL, 6, 1, 1),
SOC_DAPM_SINGLE("PCM Playback Switch", RT5610_STEREO_DAC_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("Voice Playback Switch", RT5610_VOICE_DAC_OUT_VOL, 14, 1, 1),
};


/* Mono Mixer */
static const struct snd_kcontrol_new rt5610_mono_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", RT5610_LINE_IN_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5610_MIC_ROUTING_CTRL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5610_MIC_ROUTING_CTRL, 5, 1, 1),
SOC_DAPM_SINGLE("PCM Playback Switch", RT5610_STEREO_DAC_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("RecL Playback Switch", RT5610_ADC_REC_GAIN, 13, 1, 1),
SOC_DAPM_SINGLE("RecR Playback Switch", RT5610_ADC_REC_GAIN, 12, 1, 1),
SOC_DAPM_SINGLE("Voice Playback Switch", RT5610_VOICE_DAC_OUT_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("RecordL Playback Switch", RT5610_ADC_REC_GAIN, 13, 1,1),
SOC_DAPM_SINGLE("RecordR Playback Switch", RT5610_ADC_REC_GAIN, 12, 1,1),
};

/* mono output mux */
static const struct snd_kcontrol_new rt5610_mono_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[4]);


/* speaker left output mux */
static const struct snd_kcontrol_new rt5610_hp_spkl_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[0]);


/* speaker right output mux */
static const struct snd_kcontrol_new rt5610_hp_spkr_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[1]);


/* headphone left output mux */
static const struct snd_kcontrol_new rt5610_hpl_out_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[2]);


/* headphone right output mux */
static const struct snd_kcontrol_new rt5610_hpr_out_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[3]);


static const struct snd_soc_dapm_widget rt5610_dapm_widgets[] = {
SND_SOC_DAPM_MUX("Mono Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_mono_mux_controls),
SND_SOC_DAPM_MUX("Left Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hp_spkl_mux_controls),
SND_SOC_DAPM_MUX("Right Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hp_spkr_mux_controls),
SND_SOC_DAPM_MUX("Left Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hpl_out_mux_controls),
SND_SOC_DAPM_MUX("Right Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hpr_out_mux_controls),
SND_SOC_DAPM_MIXER_E("Left HP Mixer",RT5610_PWR_MANAG_ADD2, 5, 0,
	&rt5610_hpl_mixer_controls[0], ARRAY_SIZE(rt5610_hpl_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER_E("Right HP Mixer",RT5610_PWR_MANAG_ADD2, 4, 0,
	&rt5610_hpr_mixer_controls[0], ARRAY_SIZE(rt5610_hpr_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER("Mono Mixer", RT5610_PWR_MANAG_ADD2, 2, 0,
	&rt5610_mono_mixer_controls[0], ARRAY_SIZE(rt5610_mono_mixer_controls)),
SND_SOC_DAPM_MIXER("Speaker Mixer", RT5610_PWR_MANAG_ADD2,3,0,
	&rt5610_speaker_mixer_controls[0],
	ARRAY_SIZE(rt5610_speaker_mixer_controls)),	
SND_SOC_DAPM_MIXER("Left Record Mixer", RT5610_PWR_MANAG_ADD2,1,0,
	&rt5610_captureL_mixer_controls[0],
	ARRAY_SIZE(rt5610_captureL_mixer_controls)),	
SND_SOC_DAPM_MIXER("Right Record Mixer", RT5610_PWR_MANAG_ADD2,0,0,
	&rt5610_captureR_mixer_controls[0],
	ARRAY_SIZE(rt5610_captureR_mixer_controls)),				
SND_SOC_DAPM_DAC("Left DAC", "Left HiFi Playback", RT5610_PWR_MANAG_ADD2,9, 0),
SND_SOC_DAPM_DAC("Right DAC", "Right HiFi Playback", RT5610_PWR_MANAG_ADD2, 8, 0),	
SND_SOC_DAPM_MIXER("AC97 Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("HP Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_ADC("Left ADC", "Left HiFi Capture", RT5610_PWR_MANAG_ADD2, 7, 0),
SND_SOC_DAPM_ADC("Right ADC", "Right HiFi Capture", RT5610_PWR_MANAG_ADD2, 6, 0),
SND_SOC_DAPM_PGA("Left Headphone", RT5610_PWR_MANAG_ADD3, 11, 1, NULL, 0),
SND_SOC_DAPM_PGA("Right Headphone", RT5610_PWR_MANAG_ADD3, 10, 1, NULL, 0),
SND_SOC_DAPM_PGA("Left Speaker", RT5610_PWR_MANAG_ADD3, 8, 1, NULL, 0),
SND_SOC_DAPM_PGA("Right Speaker", RT5610_PWR_MANAG_ADD3, 7, 1, NULL, 0),
SND_SOC_DAPM_PGA("Mono Out", RT5610_PWR_MANAG_ADD3, 13, 1, NULL, 0),
SND_SOC_DAPM_PGA("Left Line In", RT5610_PWR_MANAG_ADD3, 6, 1, NULL, 0),
SND_SOC_DAPM_PGA("Right Line In", RT5610_PWR_MANAG_ADD3, 5, 1, NULL, 0),
SND_SOC_DAPM_PGA("Phone In PGA", RT5610_PWR_MANAG_ADD3, 4, 1, NULL, 0),
SND_SOC_DAPM_PGA("Phone In Mixer", RT5610_PWR_MANAG_ADD3, 5, 1, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 PGA", RT5610_PWR_MANAG_ADD3, 3, 1, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 PGA", RT5610_PWR_MANAG_ADD3, 2, 1, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 Pre Amp", RT5610_PWR_MANAG_ADD3, 1, 1, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 Pre Amp", RT5610_PWR_MANAG_ADD3, 0, 1, NULL, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias1", RT5610_PWR_MANAG_ADD1, 3, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias2", RT5610_PWR_MANAG_ADD1, 2, 0),
SND_SOC_DAPM_OUTPUT("MONO"),
SND_SOC_DAPM_OUTPUT("HPL"),
SND_SOC_DAPM_OUTPUT("HPR"),
SND_SOC_DAPM_OUTPUT("SPKL"),
SND_SOC_DAPM_OUTPUT("SPKR"),
SND_SOC_DAPM_INPUT("LINEL"),
SND_SOC_DAPM_INPUT("LINER"),
SND_SOC_DAPM_INPUT("PHONEIN"),
SND_SOC_DAPM_INPUT("MIC1"),
SND_SOC_DAPM_INPUT("MIC2"),
SND_SOC_DAPM_INPUT("PCMIN"),
SND_SOC_DAPM_VMID("VMID"),
};


static const struct snd_soc_dapm_route intercon[] = {
	/* left HP mixer */
	{"Left HP Mixer", "LineIn Playback Switch", "LINEL"},
	{"Left HP Mixer", "PhoneIn Playback Switch","PHONEIN"},
	{"Left HP Mixer", "Mic1 Playback Switch","MIC1"},
	{"Left HP Mixer", "Mic2 Playback Switch","MIC2"},
	{"Left HP Mixer", "PCM Playback Switch","Left DAC"},
	{"Left HP Mixer", "Voice Playback Switch","Voice DAC"},
	{"Left HP Mixer", "RecordL Playback Switch","Left Record Mixer"},
	
	/* right HP mixer */
	{"Right HP Mixer", "LineIn Playback Switch", "LINER"},
	{"Right HP Mixer", "PhoneIn Playback Switch","PHONEIN"},
	{"Right HP Mixer", "Mic1 Playback Switch","MIC1"},
	{"Right HP Mixer", "Mic2 Playback Switch","MIC2"},
	{"Right HP Mixer", "PCM Playback Switch","Right DAC"},
	{"Right HP Mixer", "Voice Playback Switch","Voice DAC"},
	{"Right HP Mixer", "RecordR Playback Switch","Right Record Mixer"},
	
	/* virtual mixer - mixes left & right channels for spk and mono */
	{"AC97 Mixer", NULL, "Left DAC"},
	{"AC97 Mixer", NULL, "Right DAC"},
	{"Line Mixer", NULL, "Right Line In"},
	{"Line Mixer", NULL, "Left Line In"},
	{"HP Mixer", NULL, "Left HP Mixer"},
	{"HP Mixer", NULL, "Right HP Mixer"},
	
	/* speaker mixer */
	{"Speaker Mixer", "LineIn Playback Switch","Line Mixer"},
	{"Speaker Mixer", "PhoneIn Playback Switch","PHONEIN"},
	{"Speaker Mixer", "Mic1 Playback Switch","MIC1"},
	{"Speaker Mixer", "Mic2 Playback Switch","MIC2"},
	{"Speaker Mixer", "PCM Playback Switch","AC97 Mixer"},
	{"Speaker Mixer", "Voice Playback Switch","Voice DAC"},

	/* mono mixer */
	{"Mono Mixer", "LineIn Playback Switch","Line Mixer"},
	{"Mono Mixer", "PhoneIn Playback Switch","PHONEIN"},
	{"Mono Mixer", "Mic1 Playback Switch","MIC1"},
	{"Mono Mixer", "Mic2 Playback Switch","MIC2"},
	{"Mono Mixer", "PCM Playback Switch","AC97 Mixer"},
	{"Mono Mixer", "Voice Playback Switch","Voice DAC"},
	{"Mono Mixer", "RecordL Playback Switch","Left Record Mixer"},
	{"Mono Mixer", "RecordR Playback Switch","Right Record Mixer"},
	
	/*Left record mixer */
	{"Left Record Mixer", "Mic1 Capture Switch","Mic 1 Pre Amp"},
	{"Left Record Mixer", "Mic2 Capture Switch","Mic 2 Pre Amp"},
	{"Left Record Mixer", "LineInL Capture Switch","LINEL"},
	{"Left Record Mixer", "Phone Capture Switch","PHONEIN"},
	{"Left Record Mixer", "HPMixerL Capture Switch","Left HP Mixer"},
	{"Left Record Mixer", "SPKMixer Capture Switch","Speaker Mixer"},
	{"Left Record Mixer", "MonoMixer Capture Switch","Mono Mixer"},
	
	/*Right record mixer */
	{"Right Record Mixer", "Mic1 Capture Switch","Mic 1 Pre Amp"},
	{"Right Record Mixer", "Mic2 Capture Switch","Mic 2 Pre Amp"},
	{"Right Record Mixer", "LineInR Capture Switch","LINER"},
	{"Right Record Mixer", "Phone Capture Switch","PHONEIN"},
	{"Right Record Mixer", "HPMixerR Capture Switch","Right HP Mixer"},
	{"Right Record Mixer", "SPKMixer Capture Switch","Speaker Mixer"},
	{"Right Record Mixer", "MonoMixer Capture Switch","Mono Mixer"},	

	/* headphone left mux */
	{"Left Headphone Out Mux", "HPL mixer", "Left HP Mixer"},

	/* headphone right mux */
	{"Right Headphone Out Mux", "HPR mixer", "Right HP Mixer"},

	/* speaker left mux */
	{"Left Speaker Out Mux", "HPL mixer", "Left HP Mixer"},
	{"Left Speaker Out Mux", "SPK mixer", "Speaker Mixer"},
	{"Left Speaker Out Mux", "Mono Mixer", "Mono Mixer"},

	/* speaker right mux */
	{"Right Speaker Out Mux", "HPR mixer", "Right HP Mixer"},
	{"Right Speaker Out Mux", "SPK mixer", "Speaker Mixer"},
	{"Right Speaker Out Mux", "Mono Mixer", "Mono Mixer"},

	/* mono mux */
	{"Mono Out Mux", "HP mixer", "HP Mixer"},
	{"Mono Out Mux", "SPK mixer", "Speaker Mixer"},
	{"Mono Out Mux", "Mono Mixer", "Mono Mixer"},
	
	/* output pga */
	{"HPL", NULL, "Left Headphone"},
	{"Left Headphone", NULL, "Left Headphone Out Mux"},
	{"HPR", NULL, "Right Headphone"},
	{"Right Headphone", NULL, "Right Headphone Out Mux"},
	{"SPKL", NULL, "Left Speaker"},
	{"Left Speaker", NULL, "Left Speaker Out Mux"},
	{"SPKR", NULL, "Right Speaker"},
	{"Right Speaker", NULL, "Right Speaker Out Mux"},
	{"MONO", NULL, "Mono Out"},
	{"Mono Out", NULL, "Mono Out Mux"},

	/* input pga */
	{"Left Line In", NULL, "LINEL"},
	{"Right Line In", NULL, "LINER"},
	{"Phone In PGA", NULL, "PHONEIN"},
	{"Phone In Mixer", NULL, "PHONEIN"},
	{"Mic 1 Pre Amp", NULL, "MIC1"},
	{"Mic 2 Pre Amp", NULL, "MIC2"},	
	{"Mic 1 PGA", NULL, "Mic 1 Pre Amp"},
	{"Mic 2 PGA", NULL, "Mic 2 Pre Amp"},

	/* left ADC */
	{"Left ADC", NULL, "Left Record Mixer"},

	/* right ADC */
	{"Right ADC", NULL, "Right Record Mixer"},
	{NULL, NULL, NULL},	
};



static int rt5610_add_widgets(struct snd_soc_codec *codec)
{
	int i;

	snd_soc_dapm_new_controls(codec, rt5610_dapm_widgets, ARRAY_SIZE(rt5610_dapm_widgets));

	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(codec);

	return 0;
}

static unsigned int ac97_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	if(reg<=0x7E)
	{
		return soc_ac97_ops.read(codec->ac97, reg);
	}
	else 
	{
		reg = reg >> 1;

		if (reg > (ARRAY_SIZE(rt5610_reg)))
			return -EIO;

		return cache[reg];
	}
}

static int ac97_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int val)
{
	u16 *cache = codec->reg_cache;
	if (reg < 0x7c)
		soc_ac97_ops.write(codec->ac97, reg, val);
	reg = reg >> 1;
	if (reg <= (ARRAY_SIZE(rt5610_reg)))
		cache[reg] = val;

	return 0;
}

static int ac97_write_mask(struct snd_soc_codec *codec,unsigned int reg,unsigned int val,unsigned int mask)
{
	unsigned int CodecData;

	if(!mask)
		return 0; 

	if(mask!=0xffff)
	 {
		CodecData=ac97_read(codec,reg);

		CodecData&=~mask;
		CodecData|=(val&mask);
		ac97_write(codec,reg,CodecData);
	 }		
	else
	{
		ac97_write(codec,reg,val);
	}
	return 0;
	
}

static int rt5610_ChangeCodecPowerStatus(struct snd_soc_codec *codec,int power_state)
{
	unsigned short int PowerDownState=0;

	switch(power_state)
	{
		case POWER_STATE_D0:			//FULL ON-----power on all power

			ac97_write(codec,RT5610_PD_CTRL_STAT,PowerDownState);
			ac97_write(codec,RT5610_PWR_MANAG_ADD1,~PowerDownState);
			ac97_write(codec,RT5610_PWR_MANAG_ADD2,~PowerDownState);
			ac97_write(codec,RT5610_PWR_MANAG_ADD3,~PowerDownState);

		break;	

		case POWER_STATE_D1:		//LOW ON-----
			
																																
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_DAC_REF|PWR_MIC_BIAS1|PWR_HI_R_LOAD_HP
											   		,PWR_DAC_REF|PWR_MIC_BIAS1|PWR_HI_R_LOAD_HP);
									   
											
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_SPK_MIXER | PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN | 
												PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB
											   ,PWR_SPK_MIXER | PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN | 
												PWR_R_DAC_CLK | PWR_L_DAC_CLK | PWR_CLASS_AB);

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD3,PWR_MIC1_BOOST  | PWR_SPK_R_OUT| PWR_SPK_L_OUT |PWR_HP_R_OUT | PWR_HP_L_OUT |
											    		PWR_SPK_RN_OUT|PWR_SPK_LN_OUT 
											   			,PWR_MIC1_BOOST | PWR_SPK_R_OUT| PWR_SPK_L_OUT |PWR_HP_R_OUT | PWR_HP_L_OUT	|
											    		PWR_SPK_RN_OUT|PWR_SPK_LN_OUT);								
		break;

		case POWER_STATE_D1_PLAYBACK:	//Low on of Playback

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_DAC_REF | PWR_HI_R_LOAD_HP,PWR_DAC_REF | PWR_HI_R_LOAD_HP);
											
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_SPK_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB
											   ,PWR_SPK_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB);

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD3,PWR_SPK_L_OUT|PWR_SPK_R_OUT | PWR_HP_R_OUT | PWR_HP_L_OUT|PWR_SPK_RN_OUT|PWR_SPK_LN_OUT
											   		   ,PWR_SPK_L_OUT|PWR_SPK_R_OUT | PWR_HP_R_OUT | PWR_HP_L_OUT|PWR_SPK_RN_OUT|PWR_SPK_LN_OUT);	

		break;

		case POWER_STATE_D1_RECORD:	//Low on of Record
			
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_MIC_BIAS1,PWR_MIC_BIAS1);
											//
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN												
											   ,PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN);

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD3,PWR_MIC1_BOOST ,PWR_MIC1_BOOST);	
		
		break;

		case POWER_STATE_D2:		//STANDBY----
											//																								
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,0,PWR_DAC_REF | PWR_HI_R_LOAD_HP);
											//
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,0,PWR_SPK_MIXER | PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN | 
												PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB);

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD3,0,PWR_MIC1_BOOST | PWR_MIC1_VOL_CTRL | PWR_SPK_R_OUT | PWR_SPK_L_OUT |PWR_HP_R_OUT | PWR_HP_L_OUT |
											    PWR_SPK_RN_OUT | PWR_SPK_LN_OUT);	
				
		break;

		case POWER_STATE_D2_PLAYBACK:	//STANDBY of playback

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,0,PWR_DAC_REF | PWR_HI_R_LOAD_HP);
											//
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,0,PWR_SPK_MIXER | PWR_R_HP_MIXER | PWR_L_HP_MIXER | PWR_R_DAC_CLK | PWR_L_DAC_CLK  | PWR_CLASS_AB);

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD3,0,PWR_SPK_R_OUT |PWR_SPK_L_OUT | PWR_HP_R_OUT | PWR_HP_L_OUT | PWR_SPK_RN_OUT| PWR_SPK_LN_OUT);

		break;

		case POWER_STATE_D2_RECORD:		//STANDBY of record

//			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,0,PWR_MIC_BIAS1);
											//
			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,0,PWR_R_ADC_REC_MIXER | PWR_L_ADC_REC_MIXER | PWR_R_ADC_CLK_GAIN | PWR_L_ADC_CLK_GAIN);

			ac97_write_mask(codec,RT5610_PWR_MANAG_ADD3,0,PWR_MIC1_BOOST | PWR_MIC1_VOL_CTRL);	

		break;		

		case POWER_STATE_D3:		//SLEEP
		case POWER_STATE_D4:		//OFF----power off all power,include PR0,PR1,PR3,PR4,PR5,PR6,EAPD,and addition power managment
				ac97_write(codec,RT5610_PWR_MANAG_ADD3,PowerDownState);
				ac97_write(codec,RT5610_PWR_MANAG_ADD1,PowerDownState);
				ac97_write(codec,RT5610_PWR_MANAG_ADD2,PowerDownState);
			

				PowerDownState=RT_PWR_PR0 | RT_PWR_PR1 | RT_PWR_PR2 | RT_PWR_PR3 /*| RT_PWR_PR4*/ | RT_PWR_PR5 | RT_PWR_PR6 | RT_PWR_PR7; 		
				ac97_write(codec,RT5610_PD_CTRL_STAT,PowerDownState);		
				
		break;	

		default:

		break;
	}

	return 0;	
}

static int rt5610_AudioOutEnable(struct snd_soc_codec *codec,unsigned short int WavOutPath,int Mute)
{
	int RetVal=0;	

	if(Mute)
	{
		switch(WavOutPath)
		{
			case RT_WAVOUT_ALL_ON:

				RetVal=ac97_write_mask(codec,RT5610_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel
				RetVal=ac97_write_mask(codec,RT5610_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
				RetVal=ac97_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,RT_R_MUTE,RT_R_MUTE);				//Mute Mono channel
				RetVal=ac97_write_mask(codec,RT5610_STEREO_DAC_VOL,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER
															  ,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
		
			break;
		
			case RT_WAVOUT_HP:

				RetVal=ac97_write_mask(codec,RT5610_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
					
			break;

			case RT_WAVOUT_SPK:
				
				RetVal=ac97_write_mask(codec,RT5610_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel			

			break;
			

			case RT_WAVOUT_MONO:

				RetVal=ac97_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,RT_R_MUTE,RT_R_MUTE);	//Mute MonoOut channel		

			break;

			case RT_WAVOUT_DAC:

				RetVal=ac97_write_mask(codec,RT5610_STEREO_DAC_VOL,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER
															  	,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer

			break;
			default:

				return 0;

		}
	}
	else
	{
		switch(WavOutPath)
		{

			case RT_WAVOUT_ALL_ON:

				RetVal=ac97_write_mask(codec,RT5610_SPK_OUT_VOL	,0,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel
				RetVal=ac97_write_mask(codec,RT5610_HP_OUT_VOL 		,0,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
				RetVal=ac97_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//Mute Mono channel
				RetVal=ac97_write_mask(codec,RT5610_STEREO_DAC_VOL	,0,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
		
			break;
		
			case RT_WAVOUT_HP:

				RetVal=ac97_write_mask(codec,RT5610_HP_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//unMute headphone right/left channel
					
			break;

			case RT_WAVOUT_SPK:
				
				RetVal=ac97_write_mask(codec,RT5610_SPK_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//unMute Speaker right/left channel			

			break;			

			case RT_WAVOUT_MONO:

				RetVal=ac97_write_mask(codec,RT5610_PHONEIN_MONO_OUT_VOL,0,RT_R_MUTE);	//unMute MonoOut channel		

			break;
			case RT_WAVOUT_DAC:

				RetVal=ac97_write_mask(codec,RT5610_STEREO_DAC_VOL,0,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//unMute DAC to HP,Speaker,Mono Mixer

			break;
			default:
				return 0;
		}

	}
	
	return RetVal;
}



/* PLL divisors */
struct _pll_div {
	u32 pll_in;
	u32 pll_out;
	u16 regvalue;
};

static const struct _pll_div rt5610_pll_div[] = {		
		
	{  2048000,  24576000,	0x2ea0},
	{  3686400,  24576000,	0xee27},	
	{ 12000000,  24576000,	0x2915},   
	{ 13000000,  24576000,	0x772e},
	{ 13100000,	 24576000,	0x0d20},	
	
};


static int rt5610_set_pll(struct snd_soc_codec *codec,
	int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	int i;
		
	if (!freq_in || !freq_out) 
		return 0;		
	
	for (i = 0; i < ARRAY_SIZE(rt5610_pll_div); i++) {
					
			if (rt5610_pll_div[i].pll_in == freq_in && rt5610_pll_div[i].pll_out == freq_out)
			 {			 	
			 	ac97_write(codec,RT5610_PLL_CTRL,rt5610_pll_div[i].regvalue);//set PLL parameter			 							 				 	
				break;
			}
		}

	//codec clock source from PLL output		
	ac97_write_mask(codec,RT5610_GEN_CTRL_REG1,GP_CLK_FROM_PLL,GP_CLK_FROM_PLL);	
	//enable PLL power	
	ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_PLL,PWR_PLL);						
	//Power off ACLink	
	ac97_write_mask(codec,RT5610_PD_CTRL_STAT,RT_PWR_PR4,RT_PWR_PR4);	 		
	//need ac97 controller to do warm reset	
	soc_ac97_ops.warm_reset(codec->ac97);
		
	/* wait 10ms AC97 link frames for the link to stabilise */
	schedule_timeout_interruptible(msecs_to_jiffies(10));	

	return 0;	
}	

static int rt5610_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	rt5610_set_pll(codec,pll_id,freq_in,freq_out);

	return 0;
}

static int rt5610_init(struct snd_soc_codec *codec)
{
	int i;
	printk("rt5610_init\n");
	ac97_write(codec,RT5610_PD_CTRL_STAT,0);	
	//power on main bias of codec
	ac97_write_mask(codec,RT5610_PWR_MANAG_ADD1,PWR_MAIN_BIAS|PWR_MIC_BIAS1,PWR_MAIN_BIAS|PWR_MIC_BIAS1);	
	//power on vref of codec
	ac97_write_mask(codec,RT5610_PWR_MANAG_ADD2,PWR_MIXER_VREF,PWR_MIXER_VREF);	
	
//	rt5610_set_pll(codec,0,13000000,24576000);//input 13Mhz,output 24.576Mhz
			
	for(i=0;i<ARRAY_SIZE(Set_Codec_Reg_Init);i++)
	{
		ac97_write(codec,Set_Codec_Reg_Init[i][0],Set_Codec_Reg_Init[i][1]);
		
	}

	return 0;
}

static int ac97_hifi_trigger(struct snd_pcm_substream *substream,int cmd,
			     struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret = 0;
	
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		
		if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
		{
			rt5610_ChangeCodecPowerStatus(codec,POWER_STATE_D1_PLAYBACK);
						
			rt5610_AudioOutEnable(codec,RT_WAVOUT_SPK,0);
			
			rt5610_AudioOutEnable(codec,RT_WAVOUT_HP,0);
		}
		else
		{
			rt5610_ChangeCodecPowerStatus(codec,POWER_STATE_D1_RECORD);			
		}		
		
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:

		break;

	case SNDRV_PCM_TRIGGER_RESUME:

		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:

		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

//if steram will close,it will call ac97_hifi_shutdown
static void ac97_hifi_shutdown(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;



}

static int ac97_hifi_prepare(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int reg;
	u16 vra;

	vra = ac97_read(codec, RT5610_TONE_CTRL);
	ac97_write(codec, RT5610_TONE_CTRL, vra | 0x1);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		reg = RT5610_STEREO_DAC_RATE;
	else
		reg = RT5610_STEREO_ADC_RATE;

	return ac97_write(codec, reg, runtime->rate);
}


#define RT5610_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)

#define RT5610_PCM_FORMATS SNDRV_PCM_FMTBIT_S16_LE

static struct snd_soc_dai_ops rt5610_dai_ops = {
		.prepare = ac97_hifi_prepare,
		.trigger = ac97_hifi_trigger,
//		.shutdown= ac97_hifi_shutdown,
		.set_pll = rt5610_set_dai_pll,
};


struct snd_soc_dai rt5610_dai = {
	.name = "AC97 HiFi",
	.playback = {
		.stream_name = "HiFi Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = RT5610_RATES,
		.formats = RT5610_PCM_FORMATS,},
	.capture = {
		.stream_name = "HiFi Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = RT5610_RATES,
		.formats = RT5610_PCM_FORMATS,},

	.ops = &rt5610_dai_ops,
};
EXPORT_SYMBOL_GPL(rt5610_dai);


extern void pxa3xx_enable_ac97_pins(void);
int rt5610_reset(struct snd_soc_codec *codec, int try_warm)
{

	soc_ac97_ops.reset(codec->ac97);

	return 0;
}
	
EXPORT_SYMBOL_GPL(rt5610_reset);


static int rt5610_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		/* vref/mid, osc on, dac unmute */
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		/* everything off except vref/vmid, */
		break;
	case SND_SOC_BIAS_OFF:
		/* everything off, dac mute, inactive */
		break;
	}
	codec->bias_level = level;
	return 0;
}


static int rt5610_soc_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	rt5610_set_bias_level(codec,SND_SOC_BIAS_OFF);

	return 0;
}

static int rt5610_soc_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	int i, ret;
	u16 *cache = codec->reg_cache;

	if ((ret = rt5610_reset(codec, 1)) < 0){
		printk(KERN_ERR "could not reset AC97 codec\n");
		return ret;
	}

	rt5610_set_bias_level(codec,SND_SOC_BIAS_STANDBY);

	/* do we need to re-start the PLL ? */
	if (rt5610->pll_out)
		rt5610_set_pll(codec, 0, rt5610->pll_in, rt5610->pll_out);

	/* only synchronise the codec if warm reset failed */
	if (ret == 0) {
		for (i = 2; i < ARRAY_SIZE(rt5610_reg) << 1; i+=2) {
			if (i > 0x66)
				continue;
			soc_ac97_ops.write(codec->ac97, i, cache[i>>1]);
		}
	}

	if (codec->suspend_bias_level == SND_SOC_BIAS_ON)
		rt5610_set_bias_level(codec,SND_SOC_BIAS_ON);

	return ret;
}

static int rt5610_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;

	printk(KERN_INFO "RT5610/RT5611 SoC Audio Codec %s\n", RT5610_VERSION);

	socdev->card->codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return -ENOMEM;
	codec = socdev->card->codec;
	mutex_init(&codec->mutex);

	codec->reg_cache =
			kzalloc(sizeof(u16) * ARRAY_SIZE(rt5610_reg), GFP_KERNEL);
	if (codec->reg_cache == NULL){
		ret = -ENOMEM;
		goto cache_err;
	}
	memcpy(codec->reg_cache, rt5610_reg,
		sizeof(u16) * ARRAY_SIZE(rt5610_reg));
	codec->reg_cache_size = sizeof(u16) * ARRAY_SIZE(rt5610_reg);
	codec->reg_cache_step = 2;

	codec->private_data = kzalloc(sizeof(struct rt5610_priv), GFP_KERNEL);
	if (codec->private_data == NULL) {
		ret = -ENOMEM;
		goto priv_err;
	}

	codec->dev = &pdev->dev;
	codec->name = "RT5610";
	codec->owner = THIS_MODULE;
	codec->dai = &rt5610_dai;
	codec->num_dai = 1;
	codec->write = ac97_write;
	codec->read = ac97_read;
	codec->set_bias_level = rt5610_set_bias_level;

	rt5610_dai.dev = codec->dev;

	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0);
	if (ret < 0)
		goto codec_err;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0)
		goto pcm_err;

	ret = rt5610_reset(codec, 1);
	if (ret < 0) {
		printk(KERN_ERR "AC97 link error\n");
		goto reset_err;
	}

	rt5610_set_bias_level(codec,SND_SOC_BIAS_ON);


	rt5610_add_controls(codec);	
//	rt5610_add_widgets(codec);	


	rt5610_init(codec);

	
	ret = snd_soc_init_card(socdev);
	if (ret < 0)
		goto reset_err;
	
	return 0;

reset_err:
	snd_soc_free_pcms(socdev);

pcm_err:
	snd_soc_free_ac97_codec(codec);

codec_err:
	kfree(codec->private_data);

priv_err:
	kfree(codec->reg_cache);

cache_err:
	kfree(socdev->card->codec);
	socdev->card->codec = NULL;

	return ret;
}

static int rt5610_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec == NULL)
		return 0;

	snd_soc_dapm_free(socdev);
	snd_soc_free_pcms(socdev);
	snd_soc_free_ac97_codec(codec);
	kfree(codec->private_data);
	kfree(codec->reg_cache);
	kfree(codec->dai);
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_rt5610 = {
	.probe = 	rt5610_soc_probe,
	.remove = 	rt5610_soc_remove,
/*	.suspend =	rt5610_soc_suspend,
	.resume = 	rt5610_soc_resume,*/
};

EXPORT_SYMBOL_GPL(soc_codec_dev_rt5610);

static int __init rt5610_modinit(void)
{
	return snd_soc_register_dai(&rt5610_dai);
}
module_init(rt5610_modinit);

static void __exit rt5610_modexit(void)
{
	snd_soc_unregister_dai(&rt5610_dai);
}
module_exit(rt5610_modexit);

MODULE_DESCRIPTION("ASoC RT5610/RT5611 driver");
MODULE_AUTHOR("flove");
MODULE_LICENSE("GPL");
