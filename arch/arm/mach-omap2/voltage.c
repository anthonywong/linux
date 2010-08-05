/*
 * OMAP3/OMAP4 Voltage Management Routines
 *
 * Author: Thara Gopinath	<thara@ti.com>
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 * Lesly A M <x0080970@ti.com>
 *
 * Copyright (C) 2008 Nokia Corporation
 * Kalle Jokiniemi
 *
 * Copyright (C) 2010 Texas Instruments, Inc.
 * Thara Gopinath <thara@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/debugfs.h>

#include <plat/omap-pm.h>
#include <plat/omap34xx.h>
#include <plat/opp.h>
#include <plat/opp_twl_tps.h>
#include <plat/clock.h>
#include <plat/common.h>

#include "prm-regbits-34xx.h"
#include "voltage.h"

#define VP_IDLE_TIMEOUT		200
#define VP_TRANXDONE_TIMEOUT	300

#ifdef CONFIG_PM_DEBUG
struct dentry *voltage_dir;
#endif

/* VP SR debug support */
u32 enable_sr_vp_debug;

/* PRM voltage module */
u32 volt_mod;

/* Voltage processor register offsets */
struct vp_reg_offs {
	u8 vpconfig_reg;
	u8 vstepmin_reg;
	u8 vstepmax_reg;
	u8 vlimitto_reg;
	u8 status_reg;
	u8 voltage_reg;
};

/* Voltage Processor bit field values, shifts and masks */
struct vp_reg_val {
	/* VPx_VPCONFIG */
	u32 vpconfig_erroroffset;
	u16 vpconfig_errorgain;
	u32 vpconfig_errorgain_mask;
	u8 vpconfig_errorgain_shift;
	u32 vpconfig_initvoltage_mask;
	u8 vpconfig_initvoltage_shift;
	u32 vpconfig_timeouten;
	u32 vpconfig_initvdd;
	u32 vpconfig_forceupdate;
	u32 vpconfig_vpenable;
	/* VPx_VSTEPMIN */
	u8 vstepmin_stepmin;
	u16 vstepmin_smpswaittimemin;
	u8 vstepmin_stepmin_shift;
	u8 vstepmin_smpswaittimemin_shift;
	/* VPx_VSTEPMAX */
	u8 vstepmax_stepmax;
	u16 vstepmax_smpswaittimemax;
	u8 vstepmax_stepmax_shift;
	u8 vstepmax_smpswaittimemax_shift;
	/* VPx_VLIMITTO */
	u16 vlimitto_vddmin;
	u16 vlimitto_vddmax;
	u16 vlimitto_timeout;
	u16 vlimitto_vddmin_shift;
	u16 vlimitto_vddmax_shift;
	u16 vlimitto_timeout_shift;
	/* PRM_IRQSTATUS*/
	u32 tranxdone_status;
};

/**
 * omap_vdd_info - Per Voltage Domain info
 *
 * @volt_data		: voltage table having the distinct voltages supported
 *			  by the domain and other associated per voltage data.
 * @vp_offs		: structure containing the offsets for various
 *			  vp registers
 * @vp_reg		: the register values, shifts, masks for various
 *			  vp registers
 * @volt_clk		: the clock associated with the vdd.
 * @opp_type		: the type of OPP associated with this vdd.
 * @volt_data_count	: Number of distinct voltages supported by this vdd.
 * @nominal_volt	: Nominal voltaged for this vdd.
 * cmdval_reg		: Voltage controller cmdval register.
 * @vdd_sr_reg		: The smartreflex register associated with this VDD.
 */
struct omap_vdd_info{
	struct omap_volt_data *volt_data;
	struct vp_reg_offs vp_offs;
	struct vp_reg_val vp_reg;
	struct clk *volt_clk;
	int opp_type;
	int volt_data_count;
	int id;
	unsigned long nominal_volt;
	u8 cmdval_reg;
	u8 vdd_sr_reg;
};
static struct omap_vdd_info *vdd_info;
/*
 * Number of scalable voltage domains.
 */
static int no_scalable_vdd;

/* OMAP3 VP register offsets and other definitions */
struct __init vp_reg_offs omap3_vp_offs[] = {
	/* VP1 */
	{
		.vpconfig_reg = OMAP3_PRM_VP1_CONFIG_OFFSET,
		.vstepmin_reg = OMAP3_PRM_VP1_VSTEPMIN_OFFSET,
		.vstepmax_reg = OMAP3_PRM_VP1_VSTEPMAX_OFFSET,
		.vlimitto_reg = OMAP3_PRM_VP1_VLIMITTO_OFFSET,
		.status_reg = OMAP3_PRM_VP1_STATUS_OFFSET,
		.voltage_reg = OMAP3_PRM_VP1_VOLTAGE_OFFSET,
	},
	/* VP2 */
	{	.vpconfig_reg = OMAP3_PRM_VP2_CONFIG_OFFSET,
		.vstepmin_reg = OMAP3_PRM_VP2_VSTEPMIN_OFFSET,
		.vstepmax_reg = OMAP3_PRM_VP2_VSTEPMAX_OFFSET,
		.vlimitto_reg = OMAP3_PRM_VP2_VLIMITTO_OFFSET,
		.status_reg = OMAP3_PRM_VP2_STATUS_OFFSET,
		.voltage_reg = OMAP3_PRM_VP2_VOLTAGE_OFFSET,
	},
};

#define OMAP3_NO_SCALABLE_VDD ARRAY_SIZE(omap3_vp_offs)
static struct omap_vdd_info omap3_vdd_info[OMAP3_NO_SCALABLE_VDD];

/* TODO: OMAP4 register offsets */

/*
 * Default voltage controller settings.
 */
static struct omap_volt_vc_data vc_config = {
	.clksetup = 0xff,
	.voltsetup_time1 = 0xfff,
	.voltsetup_time2 = 0xfff,
	.voltoffset = 0xff,
	.voltsetup2 = 0xff,
	.vdd0_on = 0x30,        /* 1.2v */
	.vdd0_onlp = 0x20,      /* 1.0v */
	.vdd0_ret = 0x1e,       /* 0.975v */
	.vdd0_off = 0x00,       /* 0.6v */
	.vdd1_on = 0x2c,        /* 1.15v */
	.vdd1_onlp = 0x20,      /* 1.0v */
	.vdd1_ret = 0x1e,       /* .975v */
	.vdd1_off = 0x00,       /* 0.6v */
};

/*
 * Default PMIC Data
 */
static struct omap_volt_pmic_info volt_pmic_info = {
	.slew_rate = 4000,
	.step_size = 12500,
};

/*
 * Structures containing OMAP3430/OMAP3630 voltage supported and various
 * data associated with it per voltage domain basis. Smartreflex Ntarget
 * values are left as 0 as they have to be populated by smartreflex
 * driver after reading the efuse.
 */

/* VDD1 */
static struct omap_volt_data omap34xx_vdd1_volt_data[] = {
	{.volt_nominal = 975000, .sr_errminlimit = 0xF4, .vp_errgain = 0x0C},
	{.volt_nominal = 1075000, .sr_errminlimit = 0xF4, .vp_errgain = 0x0C},
	{.volt_nominal = 1200000, .sr_errminlimit = 0xF9, .vp_errgain = 0x18},
	{.volt_nominal = 1270000, .sr_errminlimit = 0xF9, .vp_errgain = 0x18},
	{.volt_nominal = 1350000, .sr_errminlimit = 0xF9, .vp_errgain = 0x18},
};

static struct omap_volt_data omap36xx_vdd1_volt_data[] = {
	{.volt_nominal = 930000, .sr_errminlimit = 0xF4, .vp_errgain = 0x0C},
	{.volt_nominal = 1100000, .sr_errminlimit = 0xF9, .vp_errgain = 0x16},
	{.volt_nominal = 1260000, .sr_errminlimit = 0xFA, .vp_errgain = 0x23},
	{.volt_nominal = 1350000, .sr_errminlimit = 0xFA, .vp_errgain = 0x27},
};

/* VDD2 */
static struct omap_volt_data omap34xx_vdd2_volt_data[] = {
	{.volt_nominal = 975000, .sr_errminlimit = 0xF4, .vp_errgain = 0x0C},
	{.volt_nominal = 1050000, .sr_errminlimit = 0xF4, .vp_errgain = 0x0C},
	{.volt_nominal = 1150000, .sr_errminlimit = 0xF9, .vp_errgain = 0x18},
};

static struct omap_volt_data omap36xx_vdd2_volt_data[] = {
	{.volt_nominal = 930000, .sr_errminlimit = 0xF4, .vp_errgain = 0x0C},
	{.volt_nominal = 1137500, .sr_errminlimit = 0xF9, .vp_errgain = 0x16},
};


/* By default VPFORCEUPDATE is the chosen method of voltage scaling */
static bool voltscale_vpforceupdate = true;

static inline u32 voltage_read_reg(u8 offset)
{
	return prm_read_mod_reg(volt_mod, offset);
}

static inline void voltage_write_reg(u8 offset, u32 value)
{
	prm_write_mod_reg(value, volt_mod, offset);
}

static int check_voltage_domain(int vdd)
{
	if (cpu_is_omap34xx()) {
		if ((vdd == VDD1) || (vdd == VDD2))
			return 0;
	}
	return -EINVAL;
}

/* Voltage debugfs support */
#ifdef CONFIG_PM_DEBUG
static int vp_debug_get(void *data, u64 *val)
{
	u16 *option = data;

	*val = *option;
	return 0;
}

static int vp_debug_set(void *data, u64 val)
{
	if (enable_sr_vp_debug) {
		u32 *option = data;
		*option = val;
	} else {
		pr_notice("DEBUG option not enabled!\n	\
			echo 1 > pm_debug/enable_sr_vp_debug - to enable\n");
	}
	return 0;
}

static int vp_volt_debug_get(void *data, u64 *val)
{
	struct omap_vdd_info *info = (struct omap_vdd_info *) data;
	u8 vsel;

	vsel = voltage_read_reg(info->vp_offs.voltage_reg);
	pr_notice("curr_vsel = %x\n", vsel);
	*val = vsel * 12500 + 600000;

	return 0;
}

static int nom_volt_debug_get(void *data, u64 *val)
{
	struct omap_vdd_info *info = (struct omap_vdd_info *) data;

	*val = get_curr_voltage(info->id);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vp_debug_fops, vp_debug_get, vp_debug_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(vp_volt_debug_fops, vp_volt_debug_get, NULL, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(nom_volt_debug_fops, nom_volt_debug_get, NULL,
								"%llu\n");
#endif

static void vp_latch_vsel(int vp_id)
{
	u32 vpconfig;
	unsigned long uvdc;
	char vsel;

	uvdc = get_curr_voltage(vp_id);
	if (!uvdc) {
		pr_warning("%s: unable to find current voltage for VDD %d\n",
			__func__, vp_id);
		return;
	}
	vsel = omap_twl_uv_to_vsel(uvdc);
	vpconfig = voltage_read_reg(vdd_info[vp_id].vp_offs.vpconfig_reg);
	vpconfig &= ~(vdd_info[vp_id].vp_reg.vpconfig_initvoltage_mask |
			vdd_info[vp_id].vp_reg.vpconfig_initvdd);
	vpconfig |= vsel << vdd_info[vp_id].vp_reg.vpconfig_initvoltage_shift;

	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg, vpconfig);

	/* Trigger initVDD value copy to voltage processor */
	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg,
			(vpconfig | vdd_info[vp_id].vp_reg.vpconfig_initvdd));

	/* Clear initVDD copy trigger bit */
	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg, vpconfig);
}

/* OMAP3 specific voltage init functions */
/*
 * Intializes the voltage controller registers with the PMIC and board
 * specific parameters and voltage setup times for OMAP3. If the board
 * file does not populate the voltage controller parameters through
 * omap3_pm_init_vc, default values specified in vc_config is used.
 */
static void __init omap3_init_voltagecontroller(void)
{
	voltage_write_reg(OMAP3_PRM_VC_SMPS_SA_OFFSET,
			(OMAP3_SRI2C_SLAVE_ADDR <<
			 OMAP3430_PRM_VC_SMPS_SA_SA1_SHIFT) |
			(OMAP3_SRI2C_SLAVE_ADDR <<
			 OMAP3430_PRM_VC_SMPS_SA_SA0_SHIFT));
	voltage_write_reg(OMAP3_PRM_VC_SMPS_VOL_RA_OFFSET,
			(OMAP3_VDD2_SR_CONTROL_REG << OMAP3430_VOLRA1_SHIFT) |
			(OMAP3_VDD1_SR_CONTROL_REG << OMAP3430_VOLRA0_SHIFT));
	voltage_write_reg(OMAP3_PRM_VC_CMD_VAL_0_OFFSET,
			(vc_config.vdd0_on << OMAP3430_VC_CMD_ON_SHIFT) |
			(vc_config.vdd0_onlp << OMAP3430_VC_CMD_ONLP_SHIFT) |
			(vc_config.vdd0_ret << OMAP3430_VC_CMD_RET_SHIFT) |
			(vc_config.vdd0_off << OMAP3430_VC_CMD_OFF_SHIFT));
	voltage_write_reg(OMAP3_PRM_VC_CMD_VAL_1_OFFSET,
			(vc_config.vdd1_on << OMAP3430_VC_CMD_ON_SHIFT) |
			(vc_config.vdd1_onlp << OMAP3430_VC_CMD_ONLP_SHIFT) |
			(vc_config.vdd1_ret << OMAP3430_VC_CMD_RET_SHIFT) |
			(vc_config.vdd1_off << OMAP3430_VC_CMD_OFF_SHIFT));
	voltage_write_reg(OMAP3_PRM_VC_CH_CONF_OFFSET,
			OMAP3430_CMD1_MASK | OMAP3430_RAV1_MASK);
	voltage_write_reg(OMAP3_PRM_VC_I2C_CFG_OFFSET,
			OMAP3430_MCODE_SHIFT | OMAP3430_HSEN_MASK);
	/* Write setup times */
	voltage_write_reg(OMAP3_PRM_CLKSETUP_OFFSET, vc_config.clksetup);
	voltage_write_reg(OMAP3_PRM_VOLTSETUP1_OFFSET,
			(vc_config.voltsetup_time2 <<
			 OMAP3430_SETUP_TIME2_SHIFT) |
			(vc_config.voltsetup_time1 <<
			 OMAP3430_SETUP_TIME1_SHIFT));
	voltage_write_reg(OMAP3_PRM_VOLTOFFSET_OFFSET, vc_config.voltoffset);
	voltage_write_reg(OMAP3_PRM_VOLTSETUP2_OFFSET, vc_config.voltsetup2);
}

/* Sets up all the VDD related info for OMAP3 */
static void __init omap3_vdd_data_configure(int vdd)
{
	unsigned long curr_volt;
	struct omap_volt_data *volt_data;
	struct clk *sys_ck;
	u32 sys_clk_speed, timeout_val, waittime;

	vdd_info[vdd].vp_offs = omap3_vp_offs[vdd];
	if (vdd == VDD1) {
		if (cpu_is_omap3630()) {
			vdd_info[vdd].vp_reg.vlimitto_vddmin =
					OMAP3630_VP1_VLIMITTO_VDDMIN;
			vdd_info[vdd].vp_reg.vlimitto_vddmax =
					OMAP3630_VP1_VLIMITTO_VDDMAX;
			vdd_info[vdd].volt_data = omap36xx_vdd1_volt_data;
			vdd_info[vdd].volt_data_count =
					ARRAY_SIZE(omap36xx_vdd1_volt_data);
		} else {
			vdd_info[vdd].vp_reg.vlimitto_vddmin =
					OMAP3430_VP1_VLIMITTO_VDDMIN;
			vdd_info[vdd].vp_reg.vlimitto_vddmax =
					OMAP3430_VP1_VLIMITTO_VDDMAX;
			vdd_info[vdd].volt_data = omap34xx_vdd1_volt_data;
			vdd_info[vdd].volt_data_count =
					ARRAY_SIZE(omap34xx_vdd1_volt_data);
		}
		vdd_info[vdd].volt_clk = clk_get(NULL, "dpll1_ck");
		WARN(IS_ERR(vdd_info[vdd].volt_clk),
				"unable to get clock for VDD%d\n", vdd + 1);
		vdd_info[vdd].opp_type = OPP_MPU;
		vdd_info[vdd].vp_reg.tranxdone_status =
				OMAP3430_VP1_TRANXDONE_ST_MASK;
		vdd_info[vdd].cmdval_reg = OMAP3_PRM_VC_CMD_VAL_0_OFFSET;
		vdd_info[vdd].vdd_sr_reg = OMAP3_VDD1_SR_CONTROL_REG;
	} else if (vdd == VDD2) {
		if (cpu_is_omap3630()) {
			vdd_info[vdd].vp_reg.vlimitto_vddmin =
					OMAP3630_VP2_VLIMITTO_VDDMIN;
			vdd_info[vdd].vp_reg.vlimitto_vddmax =
					OMAP3630_VP2_VLIMITTO_VDDMAX;
			vdd_info[vdd].volt_data = omap36xx_vdd2_volt_data;
			vdd_info[vdd].volt_data_count =
					ARRAY_SIZE(omap36xx_vdd2_volt_data);
		} else {
			vdd_info[vdd].vp_reg.vlimitto_vddmin =
					OMAP3430_VP2_VLIMITTO_VDDMIN;
			vdd_info[vdd].vp_reg.vlimitto_vddmax =
					OMAP3430_VP2_VLIMITTO_VDDMAX;
			vdd_info[vdd].volt_data = omap34xx_vdd2_volt_data;
			vdd_info[vdd].volt_data_count =
					ARRAY_SIZE(omap34xx_vdd2_volt_data);
		}
		vdd_info[vdd].volt_clk = clk_get(NULL, "l3_ick");
		WARN(IS_ERR(vdd_info[vdd].volt_clk),
				"unable to get clock for VDD%d\n", vdd + 1);
		vdd_info[vdd].opp_type = OPP_L3;
		vdd_info[vdd].vp_reg.tranxdone_status =
					OMAP3430_VP2_TRANXDONE_ST_MASK;
		vdd_info[vdd].cmdval_reg = OMAP3_PRM_VC_CMD_VAL_1_OFFSET;
		vdd_info[vdd].vdd_sr_reg = OMAP3_VDD2_SR_CONTROL_REG;
	} else {
		pr_warning("%s: Vdd%d does not exisit in OMAP3\n",
			__func__, vdd + 1);
		return;
	}

	curr_volt = get_curr_voltage(vdd);
	if (!curr_volt) {
		pr_warning("%s: unable to find current voltage for VDD%d\n",
			__func__, vdd + 1);
		return;
	}

	volt_data = omap_get_volt_data(vdd, curr_volt);
	if (IS_ERR(volt_data)) {
		pr_warning("%s: Unable to get voltage table for VDD%d at init",
			__func__, vdd + 1);
		return;
	}
	/*
	 * Sys clk rate is require to calculate vp timeout value and
	 * smpswaittimemin and smpswaittimemax.
	 */
	sys_ck = clk_get(NULL, "sys_ck");
	if (IS_ERR(sys_ck)) {
		pr_warning("%s: Could not get the sys clk to calculate"
			"various VP%d params\n", __func__, vdd + 1);
		return;
	}
	sys_clk_speed = clk_get_rate(sys_ck);
	clk_put(sys_ck);
	/* Divide to avoid overflow */
	sys_clk_speed /= 1000;

	/* Nominal/Reset voltage of the VDD */
	vdd_info[vdd].nominal_volt = 1200000;

	/* VPCONFIG bit fields */
	vdd_info[vdd].vp_reg.vpconfig_erroroffset =
				(OMAP3_VP_CONFIG_ERROROFFSET <<
				 OMAP3430_ERROROFFSET_SHIFT);
	vdd_info[vdd].vp_reg.vpconfig_errorgain = volt_data->vp_errgain;
	vdd_info[vdd].vp_reg.vpconfig_errorgain_mask = OMAP3430_ERRORGAIN_MASK;
	vdd_info[vdd].vp_reg.vpconfig_errorgain_shift =
				OMAP3430_ERRORGAIN_SHIFT;
	vdd_info[vdd].vp_reg.vpconfig_initvoltage_shift =
				OMAP3430_INITVOLTAGE_SHIFT;
	vdd_info[vdd].vp_reg.vpconfig_initvoltage_mask =
				OMAP3430_INITVOLTAGE_MASK;
	vdd_info[vdd].vp_reg.vpconfig_timeouten = OMAP3430_TIMEOUTEN_MASK;
	vdd_info[vdd].vp_reg.vpconfig_initvdd = OMAP3430_INITVDD_MASK;
	vdd_info[vdd].vp_reg.vpconfig_forceupdate = OMAP3430_FORCEUPDATE_MASK;
	vdd_info[vdd].vp_reg.vpconfig_vpenable = OMAP3430_VPENABLE_MASK;

	/* VSTEPMIN VSTEPMAX bit fields */
	waittime = ((volt_pmic_info.step_size / volt_pmic_info.slew_rate) *
				sys_clk_speed) / 1000;
	vdd_info[vdd].vp_reg.vstepmin_smpswaittimemin = waittime;
	vdd_info[vdd].vp_reg.vstepmax_smpswaittimemax = waittime;
	vdd_info[vdd].vp_reg.vstepmin_stepmin = OMAP3_VP_VSTEPMIN_VSTEPMIN;
	vdd_info[vdd].vp_reg.vstepmax_stepmax = OMAP3_VP_VSTEPMAX_VSTEPMAX;
	vdd_info[vdd].vp_reg.vstepmin_smpswaittimemin_shift =
				OMAP3430_SMPSWAITTIMEMIN_SHIFT;
	vdd_info[vdd].vp_reg.vstepmax_smpswaittimemax_shift =
				OMAP3430_SMPSWAITTIMEMAX_SHIFT;
	vdd_info[vdd].vp_reg.vstepmin_stepmin_shift = OMAP3430_VSTEPMIN_SHIFT;
	vdd_info[vdd].vp_reg.vstepmax_stepmax_shift = OMAP3430_VSTEPMAX_SHIFT;

	/* VLIMITTO bit fields */
	timeout_val = (sys_clk_speed * OMAP3_VP_VLIMITTO_TIMEOUT_US) / 1000;
	vdd_info[vdd].vp_reg.vlimitto_timeout = timeout_val;
	vdd_info[vdd].vp_reg.vlimitto_vddmin_shift = OMAP3430_VDDMIN_SHIFT;
	vdd_info[vdd].vp_reg.vlimitto_vddmax_shift = OMAP3430_VDDMAX_SHIFT;
	vdd_info[vdd].vp_reg.vlimitto_timeout_shift = OMAP3430_TIMEOUT_SHIFT;
}

/* Generic voltage init functions */
static void __init init_voltageprocesor(int vp_id)
{
	u32 vpconfig;

	vpconfig = vdd_info[vp_id].vp_reg.vpconfig_erroroffset |
			(vdd_info[vp_id].vp_reg.vpconfig_errorgain <<
			vdd_info[vp_id].vp_reg.vpconfig_errorgain_shift) |
			vdd_info[vp_id].vp_reg.vpconfig_timeouten;

	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg, vpconfig);

	voltage_write_reg(vdd_info[vp_id].vp_offs.vstepmin_reg,
			(vdd_info[vp_id].vp_reg.vstepmin_smpswaittimemin <<
			vdd_info[vp_id].vp_reg.vstepmin_smpswaittimemin_shift) |
			(vdd_info[vp_id].vp_reg.vstepmin_stepmin <<
			vdd_info[vp_id].vp_reg.vstepmin_stepmin_shift));

	voltage_write_reg(vdd_info[vp_id].vp_offs.vstepmax_reg,
			(vdd_info[vp_id].vp_reg.vstepmax_smpswaittimemax <<
			vdd_info[vp_id].vp_reg.vstepmax_smpswaittimemax_shift) |
			(vdd_info[vp_id].vp_reg.vstepmax_stepmax <<
			vdd_info[vp_id].vp_reg.vstepmax_stepmax_shift));

	voltage_write_reg(vdd_info[vp_id].vp_offs.vlimitto_reg,
			(vdd_info[vp_id].vp_reg.vlimitto_vddmax <<
			vdd_info[vp_id].vp_reg.vlimitto_vddmax_shift) |
			(vdd_info[vp_id].vp_reg.vlimitto_vddmin <<
			vdd_info[vp_id].vp_reg.vlimitto_vddmin_shift) |
			(vdd_info[vp_id].vp_reg.vlimitto_timeout <<
			vdd_info[vp_id].vp_reg.vlimitto_timeout_shift));

	/* Set the init voltage */
	vp_latch_vsel(vp_id);

	vpconfig = voltage_read_reg(vdd_info[vp_id].vp_offs.vpconfig_reg);
	/* Force update of voltage */
	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg,
			(vpconfig |
			 vdd_info[vp_id].vp_reg.vpconfig_forceupdate));
	/* Clear force bit */
	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg, vpconfig);
}

static void __init vdd_data_configure(int vdd)
{
#ifdef CONFIG_PM_DEBUG
	struct dentry *vdd_debug;
	char name[5];
#endif
	vdd_info[vdd].id = vdd;
	if (cpu_is_omap34xx())
		omap3_vdd_data_configure(vdd);

#ifdef CONFIG_PM_DEBUG
	sprintf(name, "VDD%d", vdd + 1);
	vdd_debug = debugfs_create_dir(name, voltage_dir);
	(void) debugfs_create_file("vp_errorgain", S_IRUGO | S_IWUGO,
				vdd_debug,
				&(vdd_info[vdd].vp_reg.vpconfig_errorgain),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_smpswaittimemin", S_IRUGO | S_IWUGO,
				vdd_debug, &(vdd_info[vdd].vp_reg.
				vstepmin_smpswaittimemin),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_stepmin", S_IRUGO | S_IWUGO, vdd_debug,
				&(vdd_info[vdd].vp_reg.vstepmin_stepmin),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_smpswaittimemax", S_IRUGO | S_IWUGO,
				vdd_debug, &(vdd_info[vdd].vp_reg.
				vstepmax_smpswaittimemax),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_stepmax", S_IRUGO | S_IWUGO, vdd_debug,
				&(vdd_info[vdd].vp_reg.vstepmax_stepmax),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_vddmax", S_IRUGO | S_IWUGO, vdd_debug,
				&(vdd_info[vdd].vp_reg.vlimitto_vddmax),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_vddmin", S_IRUGO | S_IWUGO, vdd_debug,
				&(vdd_info[vdd].vp_reg.vlimitto_vddmin),
				&vp_debug_fops);
	(void) debugfs_create_file("vp_timeout", S_IRUGO | S_IWUGO, vdd_debug,
				&(vdd_info[vdd].vp_reg.vlimitto_timeout),
				&vp_debug_fops);
	(void) debugfs_create_file("curr_vp_volt", S_IRUGO, vdd_debug,
				(void *) &vdd_info[vdd], &vp_volt_debug_fops);
	(void) debugfs_create_file("curr_nominal_volt", S_IRUGO, vdd_debug,
				(void *) &vdd_info[vdd], &nom_volt_debug_fops);
#endif
}
static void __init init_voltagecontroller(void)
{
	if (cpu_is_omap34xx())
		omap3_init_voltagecontroller();
}

/*
 * vc_bypass_scale_voltage - VC bypass method of voltage scaling
 */
static int vc_bypass_scale_voltage(u32 vdd, unsigned long target_volt)
{
	struct omap_volt_data *volt_data;
	u32 vc_bypass_value, vc_cmdval, vc_valid, vc_bypass_val_reg_offs;
	u32 vp_errgain_val, vc_cmd_on_mask;
	u32 loop_cnt = 0, retries_cnt = 0;
	u32 smps_steps = 0, smps_delay = 0;
	u8 vc_data_shift, vc_slaveaddr_shift, vc_regaddr_shift;
	u8 vc_cmd_on_shift;
	u8 target_vsel, current_vsel, sr_i2c_slave_addr;

	if (cpu_is_omap34xx()) {
		vc_cmd_on_shift = OMAP3430_VC_CMD_ON_SHIFT;
		vc_cmd_on_mask = OMAP3430_VC_CMD_ON_MASK;
		vc_data_shift = OMAP3430_DATA_SHIFT;
		vc_slaveaddr_shift = OMAP3430_SLAVEADDR_SHIFT;
		vc_regaddr_shift = OMAP3430_REGADDR_SHIFT;
		vc_valid = OMAP3430_VALID_MASK;
		vc_bypass_val_reg_offs = OMAP3_PRM_VC_BYPASS_VAL_OFFSET;
		sr_i2c_slave_addr = OMAP3_SRI2C_SLAVE_ADDR;
	}

	/* Get volt_data corresponding to target_volt */
	volt_data = omap_get_volt_data(vdd, target_volt);
	if (IS_ERR(volt_data)) {
		/*
		 * If a match is not found but the target voltage is
		 * is the nominal vdd voltage allow scaling
		 */
		if (target_volt != vdd_info[vdd].nominal_volt) {
			pr_warning("%s: Unable to get voltage table for VDD%d"
				"during voltage scaling. Some really Wrong!",
				__func__, vdd + 1);
			return -ENODATA;
		}
		volt_data = NULL;
	}

	target_vsel = omap_twl_uv_to_vsel(target_volt);
	current_vsel = voltage_read_reg(vdd_info[vdd].vp_offs.voltage_reg);
	smps_steps = abs(target_vsel - current_vsel);

	/* Setting the ON voltage to the new target voltage */
	vc_cmdval = voltage_read_reg(vdd_info[vdd].cmdval_reg);
	vc_cmdval &= ~vc_cmd_on_mask;
	vc_cmdval |= (target_vsel << vc_cmd_on_shift);
	voltage_write_reg(vdd_info[vdd].cmdval_reg, vc_cmdval);

	/*
	 * Setting vp errorgain based on the voltage If the debug option is
	 * enabled allow the override of errorgain from user side
	 */
	if (!enable_sr_vp_debug && volt_data) {
		vp_errgain_val = voltage_read_reg(vdd_info[vdd].
				vp_offs.vpconfig_reg);
		vdd_info[vdd].vp_reg.vpconfig_errorgain =
				volt_data->vp_errgain;
		vp_errgain_val &= ~vdd_info[vdd].vp_reg.vpconfig_errorgain_mask;
		vp_errgain_val |= vdd_info[vdd].vp_reg.vpconfig_errorgain <<
				vdd_info[vdd].vp_reg.vpconfig_errorgain_shift;
		voltage_write_reg(vdd_info[vdd].vp_offs.vpconfig_reg,
				vp_errgain_val);
	}

	vc_bypass_value = (target_vsel << vc_data_shift) |
			(vdd_info[vdd].vdd_sr_reg << vc_regaddr_shift) |
			(sr_i2c_slave_addr << vc_slaveaddr_shift);

	voltage_write_reg(vc_bypass_val_reg_offs, vc_bypass_value);

	voltage_write_reg(vc_bypass_val_reg_offs, vc_bypass_value | vc_valid);
	vc_bypass_value = voltage_read_reg(vc_bypass_val_reg_offs);

	while ((vc_bypass_value & vc_valid) != 0x0) {
		loop_cnt++;
		if (retries_cnt > 10) {
			pr_warning("%s: Loop count exceeded in check SR I2C"
				"write during voltgae scaling\n", __func__);
			return -ETIMEDOUT;
		}
		if (loop_cnt > 50) {
			retries_cnt++;
			loop_cnt = 0;
			udelay(10);
		}
		vc_bypass_value = voltage_read_reg(vc_bypass_val_reg_offs);
	}

	/* SMPS slew rate / step size. 2us added as buffer. */
	smps_delay = ((smps_steps * volt_pmic_info.step_size) /
			volt_pmic_info.slew_rate) + 2;
	udelay(smps_delay);
	return 0;
}

/* VP force update method of voltage scaling */
static int vp_forceupdate_scale_voltage(u32 vdd, unsigned long target_volt)
{
	struct omap_volt_data *volt_data;
	u32 vc_cmd_on_mask, vc_cmdval, vpconfig;
	u32 smps_steps = 0, smps_delay = 0;
	int timeout = 0;
	u8 target_vsel, current_vsel;
	u8 vc_cmd_on_shift;
	u8 prm_irqst_reg_offs, ocp_mod;

	if (cpu_is_omap34xx()) {
		vc_cmd_on_shift = OMAP3430_VC_CMD_ON_SHIFT;
		vc_cmd_on_mask = OMAP3430_VC_CMD_ON_MASK;
		prm_irqst_reg_offs = OMAP3_PRM_IRQSTATUS_MPU_OFFSET;
		ocp_mod = OCP_MOD;
	}

	/* Get volt_data corresponding to the target_volt */
	volt_data = omap_get_volt_data(vdd, target_volt);
	if (IS_ERR(volt_data)) {
		/*
		 * If a match is not found but the target voltage is
		 * is the nominal vdd voltage allow scaling
		 */
		if (target_volt != vdd_info[vdd].nominal_volt) {
			pr_warning("%s: Unable to get voltage table for VDD%d"
				"during voltage scaling. Some really Wrong!",
				__func__, vdd + 1);
			return -ENODATA;
		}
		volt_data = NULL;
	}

	target_vsel = omap_twl_uv_to_vsel(target_volt);
	current_vsel = voltage_read_reg(vdd_info[vdd].vp_offs.voltage_reg);
	smps_steps = abs(target_vsel - current_vsel);

	/* Setting the ON voltage to the new target voltage */
	vc_cmdval = voltage_read_reg(vdd_info[vdd].cmdval_reg);
	vc_cmdval &= ~vc_cmd_on_mask;
	vc_cmdval |= (target_vsel << vc_cmd_on_shift);
	voltage_write_reg(vdd_info[vdd].cmdval_reg, vc_cmdval);

	/*
	 * Getting  vp errorgain based on the voltage If the debug option is
	 * enabled allow the override of errorgain from user side.
	 */
	if (!enable_sr_vp_debug && volt_data)
		vdd_info[vdd].vp_reg.vpconfig_errorgain =
					volt_data->vp_errgain;
	/*
	 * Clear all pending TransactionDone interrupt/status. Typical latency
	 * is <3us
	 */
	while (timeout++ < VP_TRANXDONE_TIMEOUT) {
		prm_write_mod_reg(vdd_info[vdd].vp_reg.tranxdone_status,
				ocp_mod, prm_irqst_reg_offs);
		if (!(prm_read_mod_reg(ocp_mod, prm_irqst_reg_offs) &
				vdd_info[vdd].vp_reg.tranxdone_status))
				break;
		udelay(1);
	}

	if (timeout >= VP_TRANXDONE_TIMEOUT) {
		pr_warning("%s: VP%d TRANXDONE timeout exceeded."
			"Voltage change aborted", __func__, vdd + 1);
		return -ETIMEDOUT;
	}
	/* Configure for VP-Force Update */
	vpconfig = voltage_read_reg(vdd_info[vdd].vp_offs.vpconfig_reg);
	vpconfig &= ~(vdd_info[vdd].vp_reg.vpconfig_initvdd |
			vdd_info[vdd].vp_reg.vpconfig_forceupdate |
			vdd_info[vdd].vp_reg.vpconfig_initvoltage_mask |
			vdd_info[vdd].vp_reg.vpconfig_errorgain_mask);
	vpconfig |= ((target_vsel <<
			vdd_info[vdd].vp_reg.vpconfig_initvoltage_shift) |
			(vdd_info[vdd].vp_reg.vpconfig_errorgain <<
			 vdd_info[vdd].vp_reg.vpconfig_errorgain_shift));
	voltage_write_reg(vdd_info[vdd].vp_offs.vpconfig_reg, vpconfig);

	/* Trigger initVDD value copy to voltage processor */
	vpconfig |= vdd_info[vdd].vp_reg.vpconfig_initvdd;
	voltage_write_reg(vdd_info[vdd].vp_offs.vpconfig_reg, vpconfig);

	/* Force update of voltage */
	vpconfig |= vdd_info[vdd].vp_reg.vpconfig_forceupdate;
	voltage_write_reg(vdd_info[vdd].vp_offs.vpconfig_reg, vpconfig);

	timeout = 0;
	/*
	 * Wait for TransactionDone. Typical latency is <200us.
	 * Depends on SMPSWAITTIMEMIN/MAX and voltage change
	 */
	omap_test_timeout((prm_read_mod_reg(ocp_mod, prm_irqst_reg_offs) &
			vdd_info[vdd].vp_reg.tranxdone_status),
			VP_TRANXDONE_TIMEOUT, timeout);

	if (timeout >= VP_TRANXDONE_TIMEOUT)
		pr_err("%s: VP%d TRANXDONE timeout exceeded."
			"TRANXDONE never got set after the voltage update\n",
			__func__, vdd + 1);
	/*
	 * Wait for voltage to settle with SW wait-loop.
	 * SMPS slew rate / step size. 2us added as buffer.
	 */
	smps_delay = ((smps_steps * volt_pmic_info.step_size) /
			volt_pmic_info.slew_rate) + 2;
	udelay(smps_delay);

	/*
	 * Disable TransactionDone interrupt , clear all status, clear
	 * control registers
	 */
	timeout = 0;
	while (timeout++ < VP_TRANXDONE_TIMEOUT) {
		prm_write_mod_reg(vdd_info[vdd].vp_reg.tranxdone_status,
				ocp_mod, prm_irqst_reg_offs);
		if (!(prm_read_mod_reg(ocp_mod, prm_irqst_reg_offs) &
				vdd_info[vdd].vp_reg.tranxdone_status))
				break;
		udelay(1);
	}
	if (timeout >= VP_TRANXDONE_TIMEOUT)
		pr_warning("%s: VP%d TRANXDONE timeout exceeded while trying"
			"to clear the TRANXDONE status\n", __func__, vdd + 1);

	vpconfig = voltage_read_reg(vdd_info[vdd].vp_offs.vpconfig_reg);
	/* Clear initVDD copy trigger bit */
	vpconfig &= ~vdd_info[vdd].vp_reg.vpconfig_initvdd;;
	voltage_write_reg(vdd_info[vdd].vp_offs.vpconfig_reg, vpconfig);
	/* Clear force bit */
	vpconfig &= ~vdd_info[vdd].vp_reg.vpconfig_forceupdate;
	voltage_write_reg(vdd_info[vdd].vp_offs.vpconfig_reg, vpconfig);

	return 0;
}

/* Public functions */
/**
 * get_curr_vdd_voltage : Gets the current non-auto-compensated voltage
 * @vdd	: the VDD for which current voltage info is needed
 *
 * API to get the current non-auto-compensated voltage for a VDD.
 * Returns 0 in case of error else returns the current voltage for the VDD.
 */
unsigned long get_curr_voltage(int vdd)
{
	struct omap_opp *opp;
	unsigned long freq;

	if (check_voltage_domain(vdd)) {
		pr_warning("%s: VDD %d does not exist!\n", __func__, vdd);
		return 0;
	}

	freq = vdd_info[vdd].volt_clk->rate;
	opp = opp_find_freq_ceil(vdd_info[vdd].opp_type, &freq);
	if (IS_ERR(opp)) {
		pr_warning("%s: Unable to find OPP for VDD%d freq%ld\n",
			__func__, vdd + 1, freq);
		return 0;
	}

	/*
	 * Use higher freq voltage even if an exact match is not available
	 * we are probably masking a clock framework bug, so warn
	 */
	if (unlikely(freq != vdd_info[vdd].volt_clk->rate))
		pr_warning("%s: Available freq %ld != dpll freq %ld.\n",
			__func__, freq, vdd_info[vdd].volt_clk->rate);

	return opp_get_voltage(opp);
}

/**
 * omap_voltageprocessor_get_curr_volt	: API to get the current vp voltage.
 * @vp_id: VP id.
 *
 * This API returns the current voltage for the specified voltage processor
 */
unsigned long omap_voltageprocessor_get_curr_volt(int vp_id)
{
	u8 curr_vsel;

	curr_vsel = voltage_read_reg(vdd_info[vp_id].vp_offs.voltage_reg);
	return omap_twl_vsel_to_uv(curr_vsel);
}

/**
 * omap_voltageprocessor_enable : API to enable a particular VP
 * @vp_id : The id of the VP to be enable.
 *
 * This API enables a particular voltage processor. Needed by the smartreflex
 * class drivers.
 */
void omap_voltageprocessor_enable(int vp_id)
{
	u32 vpconfig;

	if (check_voltage_domain(vp_id)) {
		pr_warning("%s: VDD %d does not exist!\n",
			__func__, vp_id + 1);
		return;
	}

	/* If VP is already enabled, do nothing. Return */
	if (voltage_read_reg(vdd_info[vp_id].vp_offs.vpconfig_reg) &
				vdd_info[vp_id].vp_reg.vpconfig_vpenable)
		return;
	/*
	 * This latching is required only if VC bypass method is used for
	 * voltage scaling during dvfs.
	 */
	if (!voltscale_vpforceupdate)
		vp_latch_vsel(vp_id);

	/*
	 * If debug is enabled, it is likely that the following parameters
	 * were set from user space so rewrite them.
	 */
	if (enable_sr_vp_debug) {
		vpconfig = voltage_read_reg(
			vdd_info[vp_id].vp_offs.vpconfig_reg);
		vpconfig |= (vdd_info[vp_id].vp_reg.vpconfig_errorgain <<
			vdd_info[vp_id].vp_reg.vpconfig_errorgain_shift);
		voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg,
			vpconfig);

		voltage_write_reg(vdd_info[vp_id].vp_offs.vstepmin_reg,
			(vdd_info[vp_id].vp_reg.vstepmin_smpswaittimemin <<
			vdd_info[vp_id].vp_reg.vstepmin_smpswaittimemin_shift) |
			(vdd_info[vp_id].vp_reg.vstepmin_stepmin <<
			vdd_info[vp_id].vp_reg.vstepmin_stepmin_shift));

		voltage_write_reg(vdd_info[vp_id].vp_offs.vstepmax_reg,
			(vdd_info[vp_id].vp_reg.vstepmax_smpswaittimemax <<
			vdd_info[vp_id].vp_reg.vstepmax_smpswaittimemax_shift) |
			(vdd_info[vp_id].vp_reg.vstepmax_stepmax <<
			vdd_info[vp_id].vp_reg.vstepmax_stepmax_shift));

		voltage_write_reg(vdd_info[vp_id].vp_offs.vlimitto_reg,
			(vdd_info[vp_id].vp_reg.vlimitto_vddmax <<
			vdd_info[vp_id].vp_reg.vlimitto_vddmax_shift) |
			(vdd_info[vp_id].vp_reg.vlimitto_vddmin <<
			vdd_info[vp_id].vp_reg.vlimitto_vddmin_shift) |
			(vdd_info[vp_id].vp_reg.vlimitto_timeout <<
			vdd_info[vp_id].vp_reg.vlimitto_timeout_shift));
	}

	vpconfig = voltage_read_reg(vdd_info[vp_id].vp_offs.vpconfig_reg);
	/* Enable VP */
	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg,
				vpconfig |
				vdd_info[vp_id].vp_reg.vpconfig_vpenable);
}

/**
 * omap_voltageprocessor_disable : API to disable a particular VP
 * @vp_id : The id of the VP to be disable.
 *
 * This API disables a particular voltage processor. Needed by the smartreflex
 * class drivers.
 */
void omap_voltageprocessor_disable(int vp_id)
{
	u32 vpconfig;
	int timeout;

	if (check_voltage_domain(vp_id)) {
		pr_warning("%s: VDD %d does not exist!\n",
			__func__, vp_id + 1);
		return;
	}

	/* If VP is already disabled, do nothing. Return */
	if (!(voltage_read_reg(vdd_info[vp_id].vp_offs.vpconfig_reg) &
				vdd_info[vp_id].vp_reg.vpconfig_vpenable))
		return;

	/* Disable VP */
	vpconfig = voltage_read_reg(vdd_info[vp_id].vp_offs.vpconfig_reg);
	vpconfig &= ~vdd_info[vp_id].vp_reg.vpconfig_vpenable;
	voltage_write_reg(vdd_info[vp_id].vp_offs.vpconfig_reg, vpconfig);

	/*
	 * Wait for VP idle Typical latency is <2us. Maximum latency is ~100us
	 */
	omap_test_timeout((voltage_read_reg
			(vdd_info[vp_id].vp_offs.status_reg)),
			VP_IDLE_TIMEOUT, timeout);

	if (timeout >= VP_IDLE_TIMEOUT)
		pr_warning("%s: VP%d idle timedout\n", __func__, vp_id + 1);
	return;
}

/**
 * omap_voltage_scale : API to scale voltage of a particular voltage domain.
 * @vdd : the voltage domain whose voltage is to be scaled
 * @target_vsel : The target voltage of the voltage domain
 * @current_vsel : the current voltage of the voltage domain.
 *
 * This API should be called by the kernel to do the voltage scaling
 * for a particular voltage domain during dvfs or any other situation.
 */
int omap_voltage_scale(int vdd, unsigned long target_volt)
{
	int ret = check_voltage_domain(vdd);
	if (ret) {
		pr_warning("%s: VDD %d does not exist!\n", __func__, vdd + 1);
		return ret;
	}

	if (voltscale_vpforceupdate)
		return vp_forceupdate_scale_voltage(vdd, target_volt);
	else
		return vc_bypass_scale_voltage(vdd, target_volt);
}



/**
 * omap_reset_voltage : Resets the voltage of a particular voltage domain
 * to that of the current OPP.
 * @vdd : the voltage domain whose voltage is to be reset.
 *
 * This API finds out the correct voltage the voltage domain is supposed
 * to be at and resets the voltage to that level. Should be used expecially
 * while disabling any voltage compensation modules.
 */
void omap_reset_voltage(int vdd)
{
	unsigned long target_uvdc;

	if (check_voltage_domain(vdd)) {
		pr_warning("%s: VDD %d does not exist!\n", __func__, vdd + 1);
		return;
	}

	target_uvdc = get_curr_voltage(vdd);
	if (!target_uvdc) {
		pr_err("%s: unable to find current voltage for VDD %d\n",
			__func__, vdd + 1);
		return;
	}
	omap_voltage_scale(vdd, target_uvdc);
}

/**
 * omap_change_voltscale_method : API to change the voltage scaling method.
 * @voltscale_method : the method to be used for voltage scaling.
 *
 * This API can be used by the board files to change the method of voltage
 * scaling between vpforceupdate and vcbypass. The parameter values are
 * defined in voltage.h
 */
void omap_change_voltscale_method(int voltscale_method)
{
	switch (voltscale_method) {
	case VOLTSCALE_VPFORCEUPDATE:
		voltscale_vpforceupdate = true;
		return;
	case VOLTSCALE_VCBYPASS:
		voltscale_vpforceupdate = false;
		return;
	default:
		pr_warning("%s: Trying to change the method of voltage scaling"
			"to an unsupported one!\n", __func__);
	}
}

/**
 * omap3_pm_init_vc - polpulates vc_config with values specified in board file
 * @setup_vc - the structure with various vc parameters
 *
 * Updates vc_config with the voltage setup times and other parameters as
 * specified in setup_vc. vc_config is later used in init_voltagecontroller
 * to initialize the voltage controller registers. Board files should call
 * this function with the correct volatge settings corresponding
 * the particular PMIC and chip.
 */
void __init omap_voltage_init_vc(struct omap_volt_vc_data *setup_vc)
{
	if (!setup_vc)
		return;

	vc_config.clksetup = setup_vc->clksetup;
	vc_config.voltsetup_time1 = setup_vc->voltsetup_time1;
	vc_config.voltsetup_time2 = setup_vc->voltsetup_time2;
	vc_config.voltoffset = setup_vc->voltoffset;
	vc_config.voltsetup2 = setup_vc->voltsetup2;
	vc_config.vdd0_on = setup_vc->vdd0_on;
	vc_config.vdd0_onlp = setup_vc->vdd0_onlp;
	vc_config.vdd0_ret = setup_vc->vdd0_ret;
	vc_config.vdd0_off = setup_vc->vdd0_off;
	vc_config.vdd1_on = setup_vc->vdd1_on;
	vc_config.vdd1_onlp = setup_vc->vdd1_onlp;
	vc_config.vdd1_ret = setup_vc->vdd1_ret;
	vc_config.vdd1_off = setup_vc->vdd1_off;
}

/**
 * omap_get_voltage_table : API to get the voltage table associated with a
 *			    particular voltage domain.
 *
 * @vdd : the voltage domain id for which the voltage table is required
 * @volt_data : the voltage table for the particular vdd which is to be
 *		populated by this API
 * This API populates the voltage table associated with a VDD into the
 * passed parameter pointer. Returns the count of distinct voltages
 * supported by this vdd.
 *
 */
int omap_get_voltage_table(int vdd, struct omap_volt_data **volt_data)
{
	if (!vdd_info) {
		pr_err("%s: Voltage driver init not yet happened.Faulting!\n",
			__func__);
		return 0;
	}
	*volt_data = vdd_info[vdd].volt_data;
	return vdd_info[vdd].volt_data_count;
}

/**
 * omap_get_volt_data : API to get the voltage table entry for a particular
 *		     voltage
 * @vdd : the voltage domain id for whose voltage table has to be searched
 * @volt : the voltage to be searched in the voltage table
 *
 * This API searches through the voltage table for the required voltage
 * domain and tries to find a matching entry for the passed voltage volt.
 * If a matching entry is found volt_data is populated with that entry.
 * This API searches only through the non-compensated voltages int the
 * voltage table.
 * Returns pointer to the voltage table entry corresponding to volt on
 * sucess. Returns -ENODATA if no voltage table exisits for the passed voltage
 * domain or if there is no matching entry.
 */
struct omap_volt_data *omap_get_volt_data(int vdd, unsigned long volt)
{
	int i, ret;

	ret = check_voltage_domain(vdd);
	if (ret) {
		pr_warning("%s: VDD %d does not exist!\n", __func__, vdd + 1);
		return ERR_PTR(ret);
	}

	if (!vdd_info[vdd].volt_data) {
		pr_warning("%s: voltage table does not exist for VDD %d\n",
			__func__, vdd + 1);
		return ERR_PTR(-ENODATA);
	}

	for (i = 0; i < vdd_info[vdd].volt_data_count; i++) {
		if (vdd_info[vdd].volt_data[i].volt_nominal == volt)
			return &vdd_info[vdd].volt_data[i];
	}

	pr_notice("%s: Unable to match the current voltage with the voltage"
		"table for VDD %d\n", __func__, vdd + 1);

	return ERR_PTR(-ENODATA);
}

/**
 * omap_voltage_register_pmic : API to register PMIC specific data
 * @pmic_info : the structure containing pmic info
 *
 * This API is to be called by the borad file to specify the pmic specific
 * info as present in omap_volt_pmic_info structure. A default pmic info
 * table is maintained in the driver volt_pmic_info. If the board file do
 * not override the default table using this API, the default values wiil
 * be used in the driver.
 */
void omap_voltage_register_pmic(struct omap_volt_pmic_info *pmic_info)
{
	volt_pmic_info.slew_rate = pmic_info->slew_rate;
	volt_pmic_info.step_size = pmic_info->step_size;
}

/**
 * omap_voltage_init : Volatage init API which does VP and VC init.
 */
static int __init omap_voltage_init(void)
{
	int i;

	if (!cpu_is_omap34xx()) {
		pr_warning("%s: voltage driver support not added\n", __func__);
		return 0;
	}

#ifdef CONFIG_PM_DEBUG
	voltage_dir = debugfs_create_dir("Voltage", pm_dbg_main_dir);
#endif
	if (cpu_is_omap34xx()) {
		volt_mod = OMAP3430_GR_MOD;
		vdd_info = omap3_vdd_info;
		no_scalable_vdd = OMAP3_NO_SCALABLE_VDD;
	}
	init_voltagecontroller();
	for (i = 0; i < no_scalable_vdd; i++) {
		vdd_data_configure(i);
		init_voltageprocesor(i);
	}
	return 0;
}
arch_initcall(omap_voltage_init);
