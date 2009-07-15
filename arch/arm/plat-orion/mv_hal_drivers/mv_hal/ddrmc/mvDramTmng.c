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


/* includes */
#include "ddrmc/mvDramIf.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "pmu/mvPmuRegs.h"
#include <asm/setup.h>

#if defined (CONFIG_PM) && defined (CONFIG_ARCH_DOVE)

/* DDR speed to mask map table */
MV_U32 ddr_freq_mask[][2] = {{100, MV_DDR_100},
			     {133, MV_DDR_133},
			     {167, MV_DDR_167},
			     {200, MV_DDR_200},
			     {233, MV_DDR_233},
			     {250, MV_DDR_250},
			     {267, MV_DDR_267},
			     {333, MV_DDR_333},
			     {400, MV_DDR_400},
			     {500, MV_DDR_500},
			     {533, MV_DDR_533},
			     };
#define MV_DRAM_FREQ_MASK_CNT	(sizeof(ddr_freq_mask)/sizeof(ddr_freq_mask[0]))

/* Mandatory address decoding configurations */
MV_DDR_MC_PARAMS dove_windows[]={{0xD0800010, 0xF1800000},	/* Set DDR register space */
                                 {0xD00D025C, 0x000F1890},	/* Set NB register space */
				 {0xD0020080, 0xF1000000},	/* Set SB register space */
				 };
#define MV_DRAM_ADDR_DEC_CNT	(sizeof(dove_windows)/sizeof(dove_windows[0]))

/* Mandatory DDR reconfig configurations */
MV_DDR_MC_PARAMS ddr_reconfig[]={{0x00120, 0x03000100},	/* Load Mode Register */
			         {0x00120, 0x03000200},	/* load Extended Mode Register */
				 };
#define MV_DRAM_RECONFIG_CNT	(sizeof(ddr_reconfig)/sizeof(ddr_reconfig[0]))

extern MV_DRAM_INIT 	mv_dram_init_info;
extern u32		mv_dram_init_valid;

/*******************************************************************************
* mvDramIfParamCountGet - Get the number of Addr/Value configuration needed 
*			  to init the DDR
*
* DESCRIPTION:
*       Get the number of Addr/Value configuration needed to init the DDR
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Number of Address Value couples
*
*******************************************************************************/
MV_U32 mvDramIfParamCountGet(MV_VOID)
{
	MV_U32 cnt=0;
	MV_U32 hdr;

	if (!mv_dram_init_valid)
	{
		mvOsPrintf("Warning: DRAM Initialization Parameters Not Found (Check Tags)!");	
		return 0;
	}

	/* scan all available frequencies and decide MAX parameters count */
	for (hdr = 0; hdr < MV_DRAM_HEADERS_CNT; hdr++) {
		if (mv_dram_init_info.dram_init_ctrl[hdr].freq_mask != 0) {
			if (mv_dram_init_info.dram_init_ctrl[hdr].size > cnt) {				
				cnt = mv_dram_init_info.dram_init_ctrl[hdr].size;
			}
		}
	}

	/* Add the FIXED count used for address decoding or reconfig; the bigger */
	if (MV_DRAM_ADDR_DEC_CNT > MV_DRAM_RECONFIG_CNT)
		cnt += MV_DRAM_ADDR_DEC_CNT;
	else
		cnt += MV_DRAM_RECONFIG_CNT;

	return cnt;
}

/*******************************************************************************
* mvDramIfParamFill - Fill in the Address/Value couples
*
* DESCRIPTION:
*       This function fills in the addr/val couples needed to init the DDR
*       controller based on the requesed frequency
*
* INPUT:
*       ddrFreq - Target frequency
*
* OUTPUT:
*	params - pointer to the first addr/value element.
*	paramcnt - Number of paramters filled in the addr/value array.
*
* RETURN:
*	STATUS
*
*******************************************************************************/
MV_STATUS mvDramIfParamFill(MV_U32 ddrFreq, MV_DDR_MC_PARAMS * params, MV_U32 * paramcnt)
{
	MV_U32	reg_index, i, mask;

	/* Check that the Uboot passed valid parameters in the TAG */
	if (!mv_dram_init_valid) {
		*paramcnt = 0;
		mvOsPrintf("Warning: DRAM Initialization Parameters Not Found (Check Tags)!");
		return MV_FAIL;
	}
	
	/* Lookup the appropriate frequency mask */
	for (i=0; i<MV_DRAM_FREQ_MASK_CNT; i++) {
		if (ddr_freq_mask[i][0] == ddrFreq) {
			mask = ddr_freq_mask[i][1];
			break;
		}
	}

	/* Verify that the mask was found in the lookup table */
	if (i == MV_DRAM_FREQ_MASK_CNT) {
		*paramcnt = 0;
		return MV_FAIL;
	}

	/* Lookup the configurations entry in the table */
	for (i=0; i<MV_DRAM_HEADERS_CNT; i++) {
		if (mv_dram_init_info.dram_init_ctrl[i].freq_mask & mask) {
			reg_index = mv_dram_init_info.dram_init_ctrl[i].start_index;
			*paramcnt = mv_dram_init_info.dram_init_ctrl[i].size;
			break;
		}
	}

	/* Check if frequency is not available OR zero configurations */
	if ((i == MV_DRAM_HEADERS_CNT) || (*paramcnt == 0))
		return MV_FAIL;

	/* First copy the address decoding PREFIX */
	for (i=0; i<MV_DRAM_ADDR_DEC_CNT; i++) {
		params->addr = dove_windows[i].addr;
		params->val = dove_windows[i].val;
		params++;
	}

	/* Copy the parameters in 32bit access */
	for (i=0; i<*paramcnt; i++) {
		params->addr = mv_dram_init_info.reg_init[reg_index].reg_addr;
		params->val = mv_dram_init_info.reg_init[reg_index].reg_value;
		reg_index++;
		params++;
	}

	/* Add the count of the Address decoding registers */
	*paramcnt += MV_DRAM_ADDR_DEC_CNT;

	return MV_OK;
}


/*******************************************************************************
* mvDramReconfigParamFill - Fill in the Address/Value couples
*
* DESCRIPTION:
*       This function fills in the addr/val couples needed to init the DDR
*       controller based on the requesed frequency
*
* INPUT:
*       ddrFreq - Target frequency
*	cpuFreq - cpu frequency to calculate Timing against
*
* OUTPUT:
*	params - pointer to the first addr/value element.
*	paramcnt - Number of paramters filled in the addr/value array.
*
* RETURN:
*	STATUS
*
*******************************************************************************/
MV_STATUS mvDramReconfigParamFill(MV_U32 ddrFreq, MV_U32 cpuFreq, MV_DDR_MC_PARAMS * params, MV_U32 * paramcnt)
{
	MV_U32	reg_index, i, mask;

	/* Check that the Uboot passed valid parameters in the TAG */
	if (!mv_dram_init_valid) {
		*paramcnt = 0;
		mvOsPrintf("Warning: DRAM Initialization Parameters Not Found (Check Tags)!");
		return MV_FAIL;
	}
	
	/* Lookup the appropriate frequency mask */
	for (i=0; i<MV_DRAM_FREQ_MASK_CNT; i++) {
		if (ddr_freq_mask[i][0] == ddrFreq) {
			mask = ddr_freq_mask[i][1];
			break;
		}
	}

	/* Verify that the mask was found in the lookup table */
	if (i == MV_DRAM_FREQ_MASK_CNT) {
		*paramcnt = 0;
		return MV_FAIL;
	}

	/* Lookup the configurations entry in the table */
	for (i=0; i<MV_DRAM_HEADERS_CNT; i++) {
		if (mv_dram_init_info.dram_init_ctrl[i].freq_mask & mask) {
			reg_index = mv_dram_init_info.dram_init_ctrl[i].start_index;
			*paramcnt = mv_dram_init_info.dram_init_ctrl[i].size;
			break;
		}
	}

	/* Check if frequency is not available OR zero configurations */
	if ((i == MV_DRAM_HEADERS_CNT) || (*paramcnt == 0))
		return MV_FAIL;

	/* Drop the last line with the DDR init trigger - replaced with LMR and LEMR */
	(*paramcnt)--;

	/* Firt copy the parameters in 32bit access */
	for (i=0; i<*paramcnt; i++) {
		params->addr = (mv_dram_init_info.reg_init[reg_index].reg_addr & 0xFFFFF); /* offset only */
		params->val = mv_dram_init_info.reg_init[reg_index].reg_value;
		reg_index++;
		params++;
	}

	/* Finally add the DRAM reinit couples */
	for (i=0; i<MV_DRAM_RECONFIG_CNT; i++) {
		params->addr = ddr_reconfig[i].addr;
		params->val = ddr_reconfig[i].val;
		params++;
	}

	/* Add the count of LMR and LEMR registers count */
	*paramcnt += MV_DRAM_RECONFIG_CNT;

	return MV_OK;	
}


/*******************************************************************************
* mvDramInitPollAmvFill - Fill in the Address/Value couples
*
* DESCRIPTION:
*       This function fills in the addr/val couples needed to init the DDR
*       controller based on the requesed frequency
*
* INPUT:
*       None.
*
* OUTPUT:
*	amv - address/mask/value for the DDR init done register.
*		amv->addr: Physical adddress of the init done register
*		amv->mask: Bit mask to poll for init done
*		amv->val: Value expected after the mask.
*
* RETURN:
*	STATUS
*
*******************************************************************************/
MV_STATUS mvDramInitPollAmvFill(MV_DDR_INIT_POLL_AMV * amv)
{
	amv->addr = (DOVE_SB_REGS_PHYS_BASE|SDRAM_STATUS_REG)/*mvOsIoVirtToPhy(NULL, (void*)(INTER_REGS_BASE|SDRAM_STATUS_REG))*/;
	amv->mask = SDRAM_STATUS_INIT_DONE_MASK;
	amv->val = SDRAM_STATUS_INIT_DONE;

	return MV_OK;
}

#endif /* #if defined (CONFIG_PM) && defined (CONFIG_ARCH_DOVE) */
