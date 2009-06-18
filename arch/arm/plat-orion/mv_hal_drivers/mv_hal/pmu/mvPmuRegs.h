/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __INCMVpmuRegsh
#define __INCMVpmuRegsh

#include "ctrlEnv/mvCtrlEnvSpec.h"

/****************/
/* PM Registers */
/****************/

/* CPU Subsystem DFS Control Register */
#define PMU_CPU_DFS_CTRL_REG		(PMU_REG_BASE + 0x0)
#define PMU_DFS_CTRL_DFS_EN_OFFS	0
#define PMU_DFS_CTRL_DFS_EN_MASK	(0x1 << PMU_DFS_CTRL_DFS_EN_OFFS)
#define PMU_DFS_CTRL_SLOW_SPD_OFFS	1
#define PMU_DFS_CTRL_SLOW_SPD_MASK	(0x1 << PMU_DFS_CTRL_SLOW_SPD_OFFS)
#define PMU_DFS_CTRL_DDR_RATIO_OFFS	3
#define PMU_DFS_CTRL_DDR_RATIO_MASK	(0x1F << PMU_DFS_CTRL_DDR_RATIO_OFFS)
#define PMU_DFS_CTRL_L2_RATIO_OFFS	8
#define PMU_DFS_CTRL_L2_RATIO_MASK	(0x1F << PMU_DFS_CTRL_L2_RATIO_OFFS)
#define PMU_DFS_CTRL_CHNG_PLL_EN_OFFS	16
#define PMU_DFS_CTRL_CHNG_PLL_EN_MASK	(0x1 << PMU_DFS_CTRL_CHNG_PLL_EN_OFF)

/* CPU Subsystem DFS Status Register */
#define PMU_DFS_STATUS_REG		(PMU_REG_BASE + 0x4)
#define PMU_DFS_STAT_SLOW_OFFS		1
#define PMU_DFS_STAT_SLOW_MASK		(0x1 << PMU_DFS_STAT_SLOW_OFFS)
#define PMU_DFS_STAT_DDR_RATIO_OFFS	3
#define PMU_DFS_STAT_DDR_RATIO_MASK	(0x1F << PMU_DFS_STAT_DDR_RATIO_OFFS)
#define PMU_DFS_STAT_L2_RATIO_OFFS	8
#define PMU_DFS_STAT_L2_RATIO_MASK	(0x1F << PMU_DFS_STAT_L2_RATIO_OFFS)

/* CPU DVS Control Register */
#define PMU_CPU_DVS_CTRL_REG		(PMU_REG_BASE + 0x10)
#define PMU_DVS_CTRL_PMIC_SADDR_OFFS	0
#define PMU_DVS_CTRL_PMIC_SADDR_MASK	(0xF << PMU_DVS_CTRL_PMIC_SADDR_OFFS)
#define PMU_DVS_CTRL_PMIC_RADDR_OFFS	4
#define PMU_DVS_CTRL_PMIC_RADDR_MASK	(0xF << PMU_DVS_CTRL_PMIC_RADDR_OFFS)
#define PMU_DVS_CTRL_PMIC_VSET_OFFS	8
#define PMU_DVS_CTRL_PMIC_VSET_MASK	(0xF << PMU_DVS_CTRL_PMIC_VSET_OFFS)
#define PMU_DVS_CTRL_PMIC_PSET_OFFS	12
#define PMU_DVS_CTRL_PMIC_PSET_MASK	(0xF << PMU_DVS_CTRL_PMIC_PSET_OFFS)
#define PMU_DVS_CTRL_DVS_EN_OFFS	16
#define PMU_DVS_CTRL_DVS_EN_MASK	(0x1 << PMU_DVS_CTRL_DVS_EN_OFFS)

/* CPU DVS Status Register */
#define PMU_DVS_STAT_Reg		(PMU_REG_BASE + 0x14)
#define PMU_DVS_STAT_PMIC_SADDR_OFFS	0
#define PMU_DVS_STAT_PMIC_SADDR_MASK	(0xF << PMU_DVS_STAT_PMIC_SADDR_OFFS)
#define PMU_DVS_STAT_PMIC_RADDR_OFFS	4
#define PMU_DVS_STAT_PMIC_RADDR_MASK	(0xF << PMU_DVS_STAT_PMIC_RADDR_OFFS)
#define PMU_DVS_STAT_PMIC_VSET_OFFS	8
#define PMU_DVS_STAT_PMIC_VSET_MASK	(0xF << PMU_DVS_STAT_PMIC_VSET_OFFS)
#define PMU_DVS_STAT_PMIC_PSET_OFFS	12
#define PMU_DVS_STAT_PMIC_PSET_MASK	(0xF << PMU_DVS_STAT_PMIC_PSET_OFFS)

/* CPU DVS Delay Register */
#define PMU_DVS_DELAY_REG		(PMU_REG_BASE + 0x18)

/* Thermal Manager Control and Status Register */
#define PMU_THERMAL_MNGR_REG		(PMU_REG_BASE + 0x1C)
#define PMU_TM_DISABLE_OFFS		0
#define PMU_TM_DISABLE_MASK		(0x1 << PMU_TM_DISABLE_OFFS)
#define PMU_TM_CURR_TEMP_OFFS		1
#define PMU_TM_CURR_TEMP_MASK		(0x1FF << PMU_TM_CURR_TEMP_OFFS)
#define PMU_TM_OVRHEAT_THRSH_OFFS	10
#define PMU_TM_OVRHEAT_THRSH_MASK	(0x1FF << PMU_TM_OVRHEAT_THRSH_OFFS)
#define PMU_TM_COOL_THRSH_OFFS		19
#define PMU_TM_COOL_THRSH_MASK		(0x1FF << PMU_TM_COOL_THRSH_OFFS)

/* Thermal Manager Overheat Delay Register */
#define PMU_TM_OVRHEAT_DLY_REG		(PMU_REG_BASE + 0x20)

/* Thermal Manager Cooling Delay Register */
#define PMU_TM_COOLING_DLY_REG		(PMU_REG_BASE + 0x24)

/* Software Reset Control Register */
#define PMU_SW_RST_CTRL_REG		(PMU_REG_BASE + 0x30)
#define PMU_SW_RST_USB0_MASK		(0x1 << 0)
#define PMU_SW_RST_GIGA_MASK		(0x1 << 2)
#define PMU_SW_RST_SATA_MASK		(0x1 << 3)
#define PMU_SW_RST_PEX0_MASK		(0x1 << 4)
#define PMU_SW_RST_PEX1_MASK		(0x1 << 5)
#define PMU_SW_RST_SDIO0_MASK		(0x1 << 8)
#define PMU_SW_RST_SDIO1_MASK		(0x1 << 9)
#define PMU_SW_RST_NAND_MASK		(0x1 << 10)
#define PMU_SW_RST_CAMERA_MASK		(0x1 << 11)
#define PMU_SW_RST_AUDIO0_MASK		(0x1 << 12)
#define PMU_SW_RST_AUDIO1_MASK		(0x1 << 13)
#define PMU_SW_RST_XOR_MASK		(0x1 << 14)
#define PMU_SW_RST_CESA_MASK		(0x1 << 15)
#define PMU_SW_RST_VIDEO_MASK		(0x1 << 16)
#define PMU_SW_RST_LCD_MASK		(0x1 << 17)
#define PMU_SW_RST_GPU_MASK		(0x1 << 18)
#define PMU_SW_RST_AC97_MASK		(0x1 << 21)
#define PMU_SW_RST_PDMA_MASK		(0x1 << 22)

/* Clock Gating Control Register */
#define PMU_CLK_GATING_CTRL_REG		(PMU_REG_BASE + 0x38)

/* CPU PLL Control 0 Register */
#define PMU_PLL_CTRL_0_REG		(PMU_REG_BASE + 0x3c)
#define PMU_PLL_N_DIVIDER_OFFS		0
#define PMU_PLL_N_DIVIDER_MASK		(0x1FF << PMU_PLL_N_DIVIDER_OFFS)
#define PMU_PLL_M_DIVIDER_OFFS		9
#define PMU_PLL_M_DIVIDER_MASK		(0x1FF << PMU_PLL_M_DIVIDER_OFFS)
#define PMU_PLL_K_DIVIDER_OFFS		18
#define PMU_PLL_K_DIVIDER_MASK		(0x7 << PMU_PLL_K_DIVIDER_OFFS)

/* CPU Clock Divider Control 0 Register */
#define PMU_CLK_DIVIDER_0_REG		(PMU_REG_BASE + 0x44)
#define PMU_CLK_DIV_PPRATIO_OFFS	0
#define PMU_CLK_DIV_PPRATIO_MASK	(0x1F << PMU_CLK_DIV_PPRATIO_OFFS)
#define PMU_CLK_DIV_BPRATIO_OFFS	8
#define PMU_CLK_DIV_BPRATIO_MASK	(0x1F << PMU_CLK_DIV_BPRATIO_OFFS)
#define PMU_CLK_DIV_XPRATIO_OFFS	16
#define PMU_CLK_DIV_XPRATIO_MASK	(0x1F << PMU_CLK_DIV_XPRATIO_OFFS)
#define PMU_CLK_DIV_DPRATIO_OFFS	24
#define PMU_CLK_DIV_DPRATIO_MASK	(0x1F << PMU_CLK_DIV_DPRATIO_OFFS)

/* CPU Clock Divider Control 1 Register */
#define PMU_CLK_DIVIDER_1_REG		(PMU_REG_BASE + 0x48)
#define PMU_CLK1_DIV_D2PRATIO_OFFS	0
#define PMU_CLK1_DIV_D2PRATIO_MASK	(0x1F << PMU_CLK1_DIV_D2PRATIO_OFFS)
#define PMU_CLK1_DIV_D2PRATIO_1TO1	(0x1 << PMU_CLK1_DIV_D2PRATIO_OFFS)
#define PMU_CLK1_DIV_D2PRATIO_1TO2	(0x2 << PMU_CLK1_DIV_D2PRATIO_OFFS)
#define PMU_CLK1_DIV_D2CLK_PLL_OFFS	8
#define PMU_CLK1_DIV_D2CLK_PLL_DDR	(0x0 << PMU_CLK1_DIV_D2CLK_PLL_OFFS)
#define PMU_CLK1_DIV_D2CLK_PLL_CPU	(0x1 << PMU_CLK1_DIV_D2CLK_PLL_OFFS)

/* PMU Interrupts Cause + MASK Registers */
#define PMU_INT_CAUSE_REG		(PMU_REG_BASE + 0x50)
#define PMU_INT_MASK_REG		(PMU_REG_BASE + 0x54)
#define PMU_INT_DFS_DONE_MASK		(0x1 << 0)
#define PMU_INT_DVS_DONE_MASK		(0x1 << 1)
#define PMU_INT_OVRHEAT_MASK		(0x1 << 3)
#define PMU_INT_COOLING_MASK		(0x1 << 4)
#define PMU_INT_RTC_ALARM_MASK		(0x1 << 5)
#define PMU_INT_BAT_FAULT_MASK		(0x1 << 6)
#define PMU_INT_EXT0_WKUP_MASK		(0x1 << 7)
#define PMU_INT_EXT1_WKUP_MASK		(0x1 << 8)
#define PMU_INT_EXT2_WKUP_MASK		(0x1 << 9)

/* IO Power Down Control Register */
#define PMU_IO_PWRDWN_CTRL_REG		(PMU_REG_BASE + 0x58)
#define PMU_IO_PWRDWN_AUDIO_MASK	(0x1 << 0)
#define PMU_IO_PWRDWN_CAMERA_MASK	(0x1 << 1)
#define PMU_IO_PWRDWN_GBE_MASK		(0x1 << 2)
#define PMU_IO_PWRDWN_LCD_MASK		(0x1 << 3)
#define PMU_IO_PWRDWN_NAND_MASK		(0x1 << 4)
#define PMU_IO_PWRDWN_SDIO_MASK		(0x1 << 5)
#define PMU_IO_PWRDWN_SPI_MASK		(0x1 << 6)
#define PMU_IO_PWRDWN_PEX0_MASK		(0x1 << 16)
#define PMU_IO_PWRDWN_PEX1_MASK		(0x1 << 17)

/* Theraml Diode Control 0 Register */
#define PMU_TEMP_DIOD_CTRL0_REG		(PMU_REG_BASE + 0x5C)
#define PMU_TDC0_PWR_DWN_MASK		(0x1 << 0)
#define PMU_TDC0_SW_RST_MASK		(0x1 << 1)
#define PMU_TDC0_TC_TRIP_OFFS		2
#define PMU_TDC0_TC_TRIP_MASK		(0x7 << PMU_TDC0_TC_TRIP_OFFS)
#define PMU_TDC0_SEL_VCAL_OFFS		5
#define PMU_TDC0_SEL_VCAL_MASK		(0x3 << PMU_TDC0_SEL_VCAL_OFFS)
#define PMU_TDC0_VBE_BYPS_MASK		(0x1 << 7)
#define PMU_TDC0_SELF_RFRNC_MASK	(0x1 << 8)
#define PMU_TDC0_A_TEST_OFFS		9
#define PMU_TDC0_A_TEST_MASK		(0x3 << PMU_TDC0_A_TEST_OFFS)
#define PMU_TDC0_REF_CAL_CNT_OFFS	11
#define PMU_TDC0_REF_CAL_CNT_MASK	(0x1FF << PMU_TDC0_REF_CAL_CNT_OFFS)
#define PMU_TDC0_CAL_CAP_SRC_OFFS	20
#define PMU_TDC0_CAL_CAP_SRC_MASK	(0x7 << PMU_TDC0_CAL_CAP_SRC_OFFS)
#define PMU_TDC0_RSRV_MASK		(0x3 << 23)
#define PMU_TDC0_AVG_NUM_OFFS		25
#define PMU_TDC0_AVG_NUM_MASK		(0x7 << PMU_TDC0_AVG_NUM_OFFS)
#define PMU_TDC0_DBL_SLOP_MASK		(0x1 << 28)
#define PMU_TDC0_OTF_CAL_MASK		(0x1 << 29)
#define PMU_TDC0_SLEEP_EN_MASK		(0x1 << 30)

/* Thermal Diode Control 1 Register */
#define PMU_TEMP_DIOD_CTRL1_REG		(PMU_REG_BASE + 0x60)
#define PMU_TDC1_CAL_STRT_VAL_OFFS	0
#define PMU_TDC1_CAL_STRT_VAL_MASK	(0xFF << PMU_TDC1_CAL_STRT_VAL_OFFS)
#define PMU_TDC1_STRT_CAL_MASK		(0x1 << 8)
#define PMU_TDC1_CAL_STAT_MASK		(0x1 << 9)
#define PMU_TDC1_TEMP_VLID_MASK		(0x1 << 10)

/* PMU General Control Register */
#define PMU_GNRL_CTRL_REG		(PMU_REG_BASE + 0x70)
#define PMU_GCTRL_CLK_CHNG_MASK		(0x1 << 0)
#define PMU_GCTRL_CLK_SRC_MASK		(0x1 << 1)

/* PMU DFS Procs Program Counter 0 Register */
#define PMU_DFS_PROC_PC_0_REG		(PMU_REG_BASE + 0x100)
#define PMU_DFS_PRE_PC_OFFS		0
#define PMU_DFS_PRE_PC_MASK		(0x7FF << PMU_DFS_PRE_PC_OFFS)
#define PMU_DFS_PRE_PC_VAL		(16 << PMU_DFS_PRE_PC_OFFS)
#define PMU_DFS_CPU_PC_OFFS		16
#define PMU_DFS_CPU_PC_MASK		(0x7FF << PMU_DFS_CPU_PC_OFFS)
#define PMU_DFS_CPU_PC_VAL		(35 << PMU_DFS_CPU_PC_OFFS)

/* PMU DFS Procs Program Counter 1 Register */
#define PMU_DFS_PROC_PC_1_REG		(PMU_REG_BASE + 0x104)
#define PMU_DFS_DDR_PC_OFFS		0
#define PMU_DFS_DDR_PC_MASK		(0x7FF << PMU_DFS_DDR_PC_OFFS)
#define PMU_DFS_DDR_PC_VAL		(50 << PMU_DFS_DDR_PC_OFFS)
#define PMU_DFS_L2_PC_OFFS		16
#define PMU_DFS_L2_PC_MASK		(0x7FF << PMU_DFS_L2_PC_OFFS)
#define PMU_DFS_L2_PC_VAL		(70 << PMU_DFS_L2_PC_OFFS)

/* PMU DFS Procs Program Counter 2 Register */
#define PMU_DFS_PROC_PC_2_REG		(PMU_REG_BASE + 0x108)
#define PMU_DFS_PLL_PC_OFFS		0
#define PMU_DFS_PLL_PC_MASK		(0x7FF << PMU_DFS_PLL_PC_OFFS)
#define PMU_DFS_PLL_PC_VAL		(80 << PMU_DFS_PLL_PC_OFFS)
#define PMU_DFS_POST_PC_OFFS		16
#define PMU_DFS_POST_PC_MASK		(0x7FF << PMU_DFS_POST_PC_OFFS)
#define PMU_DFS_POST_PC_VAL		(120 << PMU_DFS_POST_PC_OFFS)

/* PMU DVS Procs Program Counter Register */
#define PMU_DVS_PROC_PC_REG		(PMU_REG_BASE + 0x10C)
#define PMU_DVS_PRE_PC_OFFS		0
#define PMU_DVS_PRE_PC_MASK		(0x7FF << PMU_DVS_PRE_PC_OFFS)
#define PMU_DVS_PRE_PC_VAL		(150 << PMU_DVS_PRE_PC_OFFS)
#define PMU_DVS_POST_PC_OFFS		16
#define PMU_DVS_POST_PC_MASK		(0x7FF << PMU_DVS_POST_PC_OFFS)
#define PMU_DVS_POST_PC_VAL		(170 << PMU_DVS_POST_PC_OFFS)

/* PMU Stdby and Sleep Procs Program Counter Register */
#define PMU_STBY_SLEEP_PROC_PC_REG	(PMU_REG_BASE + 0x110)
#define PMU_STBY_PC_OFFS		0
#define PMU_STBY_PC_MASK		(0x7FF << PMU_STBY_PC_OFFS)
#define PMU_STBY_PC_VAL			(200 << PMU_STBY_PC_OFFS)
#define PMU_SLP_PRE_PC_OFFS		16
#define PMU_SLP_PRE_PC_MASK		(0x7FF << PMU_SLP_PRE_PC_OFFS)
#define PMU_SLP_PRE_PC_VAL		(350 << PMU_SLP_PRE_PC_OFFS)

/* PMU Sleep Procs Program Counter Register */
#define PMU_SLP_PROC_PC_REG		(PMU_REG_BASE + 0x114)
#define PMU_SLP_DDR_PC_OFFS		0
#define PMU_SLP_DDR_PC_MASK		(0x7FF << PMU_SLP_DDR_PC_OFFS)
#define PMU_SLP_DDR_PC_VAL		(450 << PMU_SLP_DDR_PC_OFFS)
#define PMU_SLP_POST_PC_OFFS		16
#define PMU_SLP_POST_PC_MASK		(0x7FF << PMU_SLP_POST_PC_OFFS)
#define PMU_SLP_POST_PC_VAL		(452 << PMU_SLP_POST_PC_OFFS)

/* PMU Thermal Procs Program Counter Register */
#define PMU_THERM_PROC_PC_REG		(PMU_REG_BASE + 0x118)
#define PMU_OVERHEAT_PC_OFFS		0
#define PMU_OVERHEAT_PC_MASK		(0x7FF << PMU_OVERHEAT_PC_OFFS)
#define PMU_COOLING_PC_OFFS		16
#define PMU_COOLING_MASK		(0x7FF << PMU_COOLING_PC_OFFS)

/* PMU Program Register */
#define PMU_PROGRAM_REG(n)		(PMU_REG_BASE + 0x500 + (n & 0x1FC))
#define PMU_PROGRAM_OFFS(n)		((n & 0x3) << 3)
#define PMU_PROGRAM_MASK(n)		(0xFF << PMU_PROGRAM_OFFS(n))

/* PMU Control Register */
#define	PMU_CTRL_REG			(PMU_REG_BASE + 0x8000)
#define PMU_CTRL_DEEP_IDLE_EN_OFFS	0
#define PMU_CTRL_DEEP_IDLE_EN_MASK	(0x1 << PMU_CTRL_DEEP_IDLE_EN_OFFS)
#define PMU_CTRL_LCD_RFRSH_EN_OFFS	2
#define PMU_CTRL_LCD_RFRSH_EN_MASK	(0x1 << PMU_CTRL_LCD_RFRSH_EN_OFFS)
#define PMU_CTRL_STBY_MODE_EN_OFFS	3
#define PMU_CTRL_STBY_MODE_EN_MASK	(0x1 << PMU_CTRL_STBY_MODE_EN_OFFS)
#define PMU_CTRL_SLEEP_MODE_EN_OFFS	4
#define PMU_CTRL_SLEEP_MODE_EN_MASK	(0x1 << PMU_CTRL_SLEEP_MODE_EN_OFFS)
#define PMU_CTRL_DDR_SLF_RFRSH_EN_OFFS	5
#define PMU_CTRL_DDR_SLF_RFRSH_EN_MASK	(0x1 << PMU_CTRL_DDR_SLF_RFRSH_EN_OFFS)
#define PMU_CTRL_MC_HALT_REQ_OFFS	16
#define PMU_CTRL_MC_HALT_REQ_MASK	(0x1 << PMU_CTRL_MC_HALT_REQ_OFFS)
#define PMU_CTRL_MC_HALT_ACK_OFFS	17
#define PMU_CTRL_MC_HALT_ACK_MASK	(0x1 << PMU_CTRL_MC_HALT_ACK_OFFS)
#define PMU_CTRL_MC_SLEEP_REQ_OFFS	18
#define PMU_CTRL_MC_SLEEP_REQ_MASK	(0x1 << PMU_CTRL_MC_SLEEP_REQ_OFFS)
#define PMU_CTRL_MC_SLEEP_ACK_OFFS	19
#define PMU_CTRL_MC_SLEEP_ACK_MASK	(0x1 << PMU_CTRL_MC_SLEEP_ACK_OFFS)
#define PMU_CTRL_MASK_IRQ_OFFS		24
#define PMU_CTRL_MASK_IRQ_MASK		(0x1 << PMU_CTRL_MASK_IRQ_OFFS)
#define PMU_CTRL_MASK_FIQ_OFFS		28
#define PMU_CTRL_MASK_FIQ_MASK		(0x1 << PMU_CTRL_MASK_FIQ_OFFS)

/* PMU Status Register */
#define PMU_STATUS_REG			(PMU_REG_BASE + 0x8004)
#define PMU_STATUS_DEEP_IDLE_OFFS	0
#define PMU_STATUS_DEEP_IDLE_MASK	(0x1 << PMU_STATUS_DEEP_IDLE_OFFS)
#define PMU_STATUS_LCD_RFRSH_OFFS	2
#define PMU_STATUS_LCD_RFRSH_MASK	(0x1 << PMU_STATUS_LCD_RFRSH_OFFS)
#define PMU_STATUS_STBY_OFFS		3
#define PMU_STATUS_STBY_MASK		(0x1 << PMU_STATUS_STBY_OFFS)
#define PMU_STATUS_SLEEP_OFFS		4
#define PMU_STATUS_SLEEP_MASK		(0x1 << PMU_STATUS_SLEEP_OFFS)

/* Sleep Mode Wake-up Events Control Register */
#define PMU_SLP_WKUP_EVENT_CTRL_REG	(PMU_REG_BASE + 0x8008)
#define PMU_SLP_WKUP_CTRL_RTC_OFFS	0
#define PMU_SLP_WKUP_CTRL_RTC_MASK	(0x1 << PMU_SLP_WKUP_CTRL_RTC_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT0_OFFS	1
#define PMU_SLP_WKUP_CTRL_EXT0_MASK	(0x3 << PMU_SLP_WKUP_CTRL_EXT0_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT0_DIS	(0x0 << PMU_SLP_WKUP_CTRL_EXT0_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT0_RISE	(0x1 << PMU_SLP_WKUP_CTRL_EXT0_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT0_FALL	(0x2 << PMU_SLP_WKUP_CTRL_EXT0_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT1_OFFS	3
#define PMU_SLP_WKUP_CTRL_EXT1_MASK	(0x3 << PMU_SLP_WKUP_CTRL_EXT1_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT1_DIS	(0x0 << PMU_SLP_WKUP_CTRL_EXT1_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT1_RISE	(0x1 << PMU_SLP_WKUP_CTRL_EXT1_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT1_FALL	(0x2 << PMU_SLP_WKUP_CTRL_EXT1_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT2_OFFS	5
#define PMU_SLP_WKUP_CTRL_EXT2_MASK	(0x3 << PMU_SLP_WKUP_CTRL_EXT2_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT2_DIS	(0x0 << PMU_SLP_WKUP_CTRL_EXT2_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT2_RISE	(0x1 << PMU_SLP_WKUP_CTRL_EXT2_OFFS)
#define PMU_SLP_WKUP_CTRL_EXT2_FALL	(0x2 << PMU_SLP_WKUP_CTRL_EXT2_OFFS)
#define PMU_SLP_WKUP_CTRL_BAT_FALT_OFFS	7
#define PMU_SLP_WKUP_CTRL_BAT_FALT_MASK	(0x1 << PMU_SLP_WKUP_CTRL_BAT_FALT_OFFS)
#define PMU_SLP_WKUP_CTRL_ALL_EV_MASK	(0xFF << 0)

/* Sleep Mode Wake-up Events Status Register */
#define PMU_SLP_WKUP_EVENT_STAT_REG	(PMU_REG_BASE + 0x800C)
#define PMU_SLP_WKUP_STAT_RTC_OFFS	0
#define PMU_SLP_WKUP_STAT_RTC_MASK	(0x1 << PMU_SLP_WKUP_STAT_RTC_OFFS)
#define PMU_SLP_WKUP_STAT_EXT0_OFFS	1
#define PMU_SLP_WKUP_STAT_EXT0_MASK	(0x1 << PMU_SLP_WKUP_STAT_EXT0_OFFS)
#define PMU_SLP_WKUP_STAT_EXT1_OFFS	3
#define PMU_SLP_WKUP_STAT_EXT1_MASK	(0x1 << PMU_SLP_WKUP_STAT_EXT1_OFFS)
#define PMU_SLP_WKUP_STAT_EXT2_OFFS	5
#define PMU_SLP_WKUP_STAT_EXT2_MASK	(0x1 << PMU_SLP_WKUP_STAT_EXT2_OFFS)
#define PMU_SLP_WKUP_STAT_BAT_FALT_OFFS	7
#define PMU_SLP_WKUP_STAT_BAT_FALT_MASK	(0x1 << PMU_SLP_WKUP_STAT_BAT_FALT_OFFS)
#define PMU_SLP_WKUP_STAT_ALL_EV_MASK	(0xF << 0)

/* PMU Power Supply Control Register */
#define PMU_PWR_SUPLY_CTRL_REG		(PMU_REG_BASE + 0x8010)
#define PMU_PWR_GOOD_PIN_EN_OFFS	0
#define PMU_PWR_GOOD_PIN_EN_MASK	(0x1 << PMU_PWR_GOOD_PIN_EN_OFFS)
#define PMU_PWR_DWN_PIN_PLRTY_OFFS	1
#define PMU_PWR_DWN_PIN_PLRTY_MASK	(0x1 << PMU_PWR_DWN_PIN_PLRTY_OFFS)
#define PMU_PWR_GPU_PWR_DWN_OFFS	2
#define PMU_PWR_GPU_PWR_DWN_MASK	(0x1 << PMU_PWR_GPU_PWR_DWN_OFFS)
#define PMU_PWR_VPU_PWR_DWN_OFFS	3
#define PMU_PWR_VPU_PWR_DWN_MASK	(0x1 << PMU_PWR_VPU_PWR_DWN_OFFS)
#define PMU_PWR_CPU_PWR_DWN_OFFS	4
#define PMU_PWR_CPU_PWR_DWN_MASK	(0x1 << PMU_PWR_CPU_PWR_DWN_OFFS)
#define PMU_PWR_SLEEP_PWR_DWN_OFFS	5
#define PMU_PWR_SLEEP_PWR_DWN_MASK	(0x1 << PMU_PWR_SLEEP_PWR_DWN_OFFS)
#define PMU_PWR_SLEEP_PWRGOOD_EN_OFFS	6
#define PMU_PWR_SLEEP_PWRGOOD_EN_MASK	(0x1 << PMU_PWR_SLEEP_PWRGOOD_EN_OFFS)
#define PMU_PWR_POWER_GATE_OFFS		8
#define PMU_PWR_POWER_GATE_MASK		(0xFF << PMU_PWR_POWER_GATE_OFFS)

/* CPU Power Delay Register */
#define PMU_CPU_PWR_DELAY_REG		(PMU_REG_BASE + 0x8014)

/* Sleep Power Delay Register */
#define PMU_SLEEP_PWR_DELAY_REG		(PMU_REG_BASE + 0x8018)

/* Battery Management Control Register */
#define PMU_BAT_MNGMT_CTRL_REG		(PMU_REG_BASE + 0x801C)
#define PMU_BAT_FLT_DISABLE_OFFS	0
#define PMU_BAT_FLT_DISABLE_MASK	(0x1 << PMU_BAT_FLT_DISABLE_OFFS)
#define PMU_BAT_FLT_SLP_EXIT_DIS_OFFS	1
#define PMU_BAT_FLT_SLP_EXIT_DIS_MASK	(0x1 << PMU_BAT_FLT_SLP_EXIT_DIS_OFFS)

/* PMU Wait Timer Register */
#define PMU_WAIT_TIMER_REG		(PMU_REG_BASE + 0x8024)

/* PMU Wait For Register */
#define PMU_WAIT_FOR_REG		(PMU_REG_BASE + 0x8028)
#define PMU_W4_CPU_IDLE_MASK		(0x1 << 0)
#define PMU_W4_CPU_PLL_LOCK_MASK	(0x1 << 1)
#define PMU_W4_CORE_PLL_LOCK_MASK	(0x1 << 2)
#define PMU_W4_CPU_PWR_GOOD_MASK	(0x1 << 3)
#define PMU_W4_IRQ_MASK			(0x1 << 4)
#define PMU_W4_FIQ_MASK			(0x1 << 5)
#define PMU_W4_BAT_FLT_MASK		(0x1 << 6)
#define PMU_W4_SB_UPSTRM_REQ_MASK	(0x1 << 7)
#define PMU_W4_LCD_0_INT_MASK		(0x1 << 8)
#define PMU_W4_LCD_1_INT_MASK		(0x1 << 9)
#define PMU_W4_SLP_PWR_GOOD_MASK	(0x1 << 10)

/* PMU Signal selector Control 0 Register */
#define PMU_SIG_SLCT_CTRL_0_REG		(PMU_REG_BASE + 0x802C)
#define PMU_SIG_0_SLCT_CTRL_OFFS	0
#define PMU_SIG_0_SLCT_CTRL_MASK	(0xF << PMU_SIG_0_SLCT_CTRL_OFFS)
#define PMU_SIG_1_SLCT_CTRL_OFFS	4
#define PMU_SIG_1_SLCT_CTRL_MASK	(0xF << PMU_SIG_1_SLCT_CTRL_OFFS)
#define PMU_SIG_2_SLCT_CTRL_OFFS	8
#define PMU_SIG_2_SLCT_CTRL_MASK	(0xF << PMU_SIG_2_SLCT_CTRL_OFFS)
#define PMU_SIG_3_SLCT_CTRL_OFFS	12
#define PMU_SIG_3_SLCT_CTRL_MASK	(0xF << PMU_SIG_3_SLCT_CTRL_OFFS)
#define PMU_SIG_4_SLCT_CTRL_OFFS	16
#define PMU_SIG_4_SLCT_CTRL_MASK	(0xF << PMU_SIG_4_SLCT_CTRL_OFFS)
#define PMU_SIG_5_SLCT_CTRL_OFFS	20
#define PMU_SIG_5_SLCT_CTRL_MASK	(0xF << PMU_SIG_5_SLCT_CTRL_OFFS)
#define PMU_SIG_6_SLCT_CTRL_OFFS	24
#define PMU_SIG_6_SLCT_CTRL_MASK	(0xF << PMU_SIG_6_SLCT_CTRL_OFFS)
#define PMU_SIG_7_SLCT_CTRL_OFFS	28
#define PMU_SIG_7_SLCT_CTRL_MASK	(0xF << PMU_SIG_7_SLCT_CTRL_OFFS)

/* PMU Signal selector Control 1 Register */
#define PMU_SIG_SLCT_CTRL_1_REG		(PMU_REG_BASE + 0x8030)
#define PMU_SIG_8_SLCT_CTRL_OFFS	0
#define PMU_SIG_8_SLCT_CTRL_MASK	(0xF << PMU_SIG_8_SLCT_CTRL_OFFS)
#define PMU_SIG_9_SLCT_CTRL_OFFS	4
#define PMU_SIG_9_SLCT_CTRL_MASK	(0xF << PMU_SIG_9_SLCT_CTRL_OFFS)
#define PMU_SIG_10_SLCT_CTRL_OFFS	8
#define PMU_SIG_10_SLCT_CTRL_MASK	(0xF << PMU_SIG_10_SLCT_CTRL_OFFS)
#define PMU_SIG_11_SLCT_CTRL_OFFS	12
#define PMU_SIG_11_SLCT_CTRL_MASK	(0xF << PMU_SIG_11_SLCT_CTRL_OFFS)
#define PMU_SIG_12_SLCT_CTRL_OFFS	16
#define PMU_SIG_12_SLCT_CTRL_MASK	(0xF << PMU_SIG_12_SLCT_CTRL_OFFS)
#define PMU_SIG_13_SLCT_CTRL_OFFS	20
#define PMU_SIG_13_SLCT_CTRL_MASK	(0xF << PMU_SIG_13_SLCT_CTRL_OFFS)
#define PMU_SIG_14_SLCT_CTRL_OFFS	24
#define PMU_SIG_14_SLCT_CTRL_MASK	(0xF << PMU_SIG_14_SLCT_CTRL_OFFS)
#define PMU_SIG_15_SLCT_CTRL_OFFS	28
#define PMU_SIG_15_SLCT_CTRL_MASK	(0xF << PMU_SIG_15_SLCT_CTRL_OFFS)

/* PMU Internal Control Register */
#define PMU_INTERNAL_CTRL_REG		(PMU_REG_BASE + 0x8050)
#define PMU_INTCTRL_PMU_EN_MASK		(0x1 << 0)
#define PMU_INTCTRL_PMU_CLK_SLCT_MASK	(0x1 << 4)
#define PMU_INTCTRL_SLP_PRE_EN_MASK	(0x1 << 8)
#define PMU_INTCTRL_SLP_DDR_EN_MASK	(0x1 << 9)
#define PMU_INTCTRL_DFS_PRE_EN_MASK	(0x1 << 16)
#define PMU_INTCTRL_DFS_CPU_EN_MASK	(0x1 << 17)
#define PMU_INTCTRL_DFS_DDR_EN_MASK	(0x1 << 18)
#define PMU_INTCTRL_DFS_L2_EN_MASK	(0x1 << 19)
#define PMU_INTCTRL_DFS_PLL_EN_MASK	(0x1 << 20)
#define PMU_INTCTRL_DVS_PRE_EN_MASK	(0x1 << 21)
#define PMU_INTCTRL_CPU_RESET_MASK	(0x1 << 22)
#define PMU_INTCTRL_SLP_RESET_OUT_MASK	(0x1 << 23)

/* PMU PC Force Control Register */
#define PMU_PC_FORCE_CTRL_REG		(PMU_REG_BASE + 0x8054)
#define PMU_PC_FORCE_CMD_OFFS		0
#define PMU_PC_FORCE_CMD_MASK		(0x1 << PMU_PC_FORCE_CMD_OFFS)
#define PMU_PC_FORCE_VAL_OFFS		1
#define PMU_PC_FORCE_VAL_MASK		(0x7F << PMU_PC_FORCE_VAL_OFFS)

/* PMU ISO Control Register */
#define PMU_ISO_CTRL_REG		(PMU_REG_BASE + 0x8058)
#define PMU_ISO_VIDEO_MASK		(0x1 << 0)
#define PMU_ISO_GPU_MASK		(0x1 << 1)
#define PMU_ISO_CPU_MASK		(0x1 << 2)
#define PMU_ISO_CORE_MASK		(0x1 << 3)

/* PMU RAM Control Register */
#define PMU_RAM_CTRL_REG		(PMU_REG_BASE + 0x8060)
#define PMU_RAM_EMA_A_OFFS		0
#define PMU_RAM_EMA_A_MASK		(0x7 << PMU_RAM_EMA_A_OFFS)
#define PMU_RAM_EMA_B_OFFS		3
#define PMU_RAM_EMA_B_MASK		(0x7 << PMU_RAM_EMA_B_OFFS)
#define PMU_RAM_PWR_OFFS		8
#define PMU_RAM_PWR_MASK		(0x1 << PMU_RAM_PWR_OFFS)

/* PMU Resume Descriptors Configuration Register */
#define PMU_RESUME_DESC_CFG_REG		(PMU_REG_BASE + 0x8100)
#define PMU_RD_CFG_DISC_CNT_OFFS	0
#define PMU_RD_CFG_DISC_CNT_MASK	(0x7 << PMU_RD_CFG_DISC_CNT_OFFS)

/* PMU Resume Address Register */
#define PMU_RESUME_ADDR_REG		(PMU_REG_BASE + 0x8104)

/* PMU Resume Descriptor Control Register */
#define PMU_RESUME_DESC_CTRL_REG(n)	(PMU_REG_BASE + 0x8108 + (n*8))
#define PMU_RD_CTRL_DISC_TYPE_OFFS	0
#define PMU_RD_CTRL_DISC_TYPE_MASK	(0xF << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_NONE	(0x0 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_32AV	(0x1 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_16AV	(0x2 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_8AV	(0x3 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_32AMV	(0x4 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_32BC	(0x5 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_32BS	(0x6 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_POLL	(0x7 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_DISC_TYPE_DELY	(0x8 << PMU_RD_CTRL_DISC_TYPE_OFFS)
#define PMU_RD_CTRL_CFG_CNT_OFFS	4
#define PMU_RD_CTRL_CFG_CNT_MASK	(0x7FF << PMU_RD_CTRL_CFG_CNT_OFFS)
#define PMU_RD_CTRL_CFG_DLY_OFFS	15
#define PMU_RD_CTRL_CFG_DLY_MASK	(0x1FFFF << PMU_RD_CTRL_CFG_DLY_OFFS)

/* PMU Resume Descriptor Address Register */
#define PMU_RESUME_DESC_ADDR_REG(n)	(PMU_REG_BASE + 0x810C + (n*8))

/* PMU SRAM - scratch pad */
#define PMU_PMUSP_OFFS			(PMU_REG_BASE + 0xC000)
#define PMU_SCRATCHPAD_OFFS		(0x10000)
#define PMU_SCRATCHPAD_SIZE		0x800

#endif /* __INCMVpmuRegsh */
