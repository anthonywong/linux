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

#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysPmuConfig.h"
#include "mvPmuRegs.h"
#include "mvPmu.h"
#include "mvSram.h"
#include "ddr/mvDramIf.h"

/* Constants */
#define PMU_MAX_DESCR_CNT		7		
#define PMU_STANDBY_DESCR_CNT		2
#define PMU_DEEPIDLE_DESCR_CNT		0
#ifndef CONFIG_DOVE_REV_Z0
 #define PMU_SP_TARGET_ID		0xD
 #define PMU_SP_ATTRIBUTE		0x0
#endif

/* DDR parameters pointer */
static MV_VOID * _mvPmuSramDdrParamPtr;		/* External Read-only access */
static MV_VOID * _mvPmuSramDdrParamPtrInt;	/* Internale Read-write access */
static MV_VOID * _mvPmuSramDdrInitPollPtr;	/* External Read-only access */
static MV_VOID * _mvPmuSramDdrInitPollPtrInt;	/* Internale Read-write access */

/* Sram Markers */
static unsigned long mvPmuSramOffs = PMU_SCRATCHPAD_RSRV;
static unsigned long mvPmuSramSize = (PMU_SCRATCHPAD_SIZE - PMU_SCRATCHPAD_RSRV);

/* SRAM functions pointer */
static MV_VOID (*_mvPmuSramDdrReconfigPtr)(MV_U32 cplPtr, MV_U32 cplCnt, MV_U32 dryRun);
static MV_VOID (*_mvPmuSramDeepIdleEnterPtr)(MV_U32 ddrSelfRefresh);
static MV_VOID (*_mvPmuSramDeepIdleExitPtr)(MV_VOID);
static MV_VOID (*_mvPmuSramStandbyEnterPtr)(MV_VOID);
static MV_VOID (*_mvPmuSramStandbyExitPtr)(MV_VOID);
static MV_VOID (*_mvPmuSramCpuDfsPtr)(MV_VOID);

/* Macros */
#define PmuSpVirt2Phys(addr)	(((MV_U32)addr - DOVE_SCRATCHPAD_VIRT_BASE) + DOVE_SCRATCHPAD_PHYS_BASE)
#define dbg_print(...)		

/*******************************************************************************
* mvPmuSramRelocate - Relocate a function into the PMU SRAM
*
* DESCRIPTION:
*   	Relocate a function into the SRAM to be executed from there.
*
* INPUT:
*       start: starting address of the function to relocated
*	size: size of the function to be relocated
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
static MV_VOID * mvPmuSramRelocate(MV_VOID * start, MV_U32 size)
{
	MV_VOID * fncptr;
	MV_U32 * src;
	MV_U32 * dst;
	MV_U32 i;
	MV_U32 orig_size = size;
/*
	if (size & 0x3) {
		dbg_print("Function relocated with non-alligned size\n");
		return NULL;
	}
*/
	/* Round up the size to a complete cache line */
	size += 31;
	size &= ~31;

	if (size > mvPmuSramSize) {
		dbg_print("No more space in SRAM for function relocation\n");
		return NULL;
	}

	mvPmuSramSize -= size;

	src = (MV_U32*)start;
	dst = (MV_U32*)(PMU_SCRATCHPAD_INT_BASE + mvPmuSramOffs);

	if (start)
	{
		for (i=0; i<orig_size; i+=4)
		{
			*dst = *src;
			dst++;
			src++;
		}
	}	

	fncptr = (MV_VOID *)(PMU_SCRATCHPAD_EXT_BASE + mvPmuSramOffs);
	mvPmuSramOffs += size;	
	
	dbg_print("mvPmuSramRelocate: From %08x to %08x (exec %08x), Size = %x\n", (MV_U32)start, (MV_U32)dst, fncptr, size);
	return fncptr;
}

/*******************************************************************************
* mvPmuSramDdrReconfig - Reconfigure the DDR parameters to new frequency
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	while having the DDR in self refresh
*
* INPUT:
*	None
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramDdrReconfig(MV_U32 paramcnt)
{
	if (!_mvPmuSramDdrReconfigPtr)
		panic("Function not yet relocated in SRAM\n");

	/* First DRY run to avoid speculative prefetches*/
	_mvPmuSramDdrReconfigPtr((MV_U32)_mvPmuSramDdrParamPtrInt, paramcnt, 0);

	/* Real run to perform the scalinf */
	_mvPmuSramDdrReconfigPtr((MV_U32)_mvPmuSramDdrParamPtrInt, paramcnt, 1);
	
	return;
}	

/*******************************************************************************
* mvPmuSramCpuDfs - Reconfigure the CPU speed
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	to change the CPU clock withoiut accessing the DDR
*
* INPUT:
*	None
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramCpuDfs(MV_VOID)
{
	if (!_mvPmuSramCpuDfsPtr)
		panic("Function not yet relocated in SRAM\n");

	return _mvPmuSramCpuDfsPtr();
}

/*******************************************************************************
* mvPmuSramDeepIdle - Enter Deep Idle mode
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	to enter deep Idle mode (power down the CPU, VFP and caches)
*
* INPUT:
*	ddrSelfRefresh: Enable/Disable (0x1/0x0) DDR selfrefresh while in Deep
*                       Idle.
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramDeepIdle(MV_U32 ddrSelfRefresh)
{
	if (!_mvPmuSramDeepIdleEnterPtr)
		panic("Function not yet relocated in SRAM\n");

	return _mvPmuSramDeepIdleEnterPtr(ddrSelfRefresh);
}

/*******************************************************************************
* mvPmuSramStandby - Enter Standby mode
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	to enter standby mode (power down the whole SoC). On exiting the Standby
*	mode the CPU returns from this call normally
*
* INPUT:
*	lcdRefresh: LCD refresh mode, enabled/disabled
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramStandby(MV_VOID)
{
	if (!_mvPmuSramStandbyEnterPtr)
		panic("Function not yet relocated in SRAM\n");

	return _mvPmuSramStandbyEnterPtr();
}

/*******************************************************************************
* mvPmuSramInit - Load the PMU Sram with all calls functions needed for PMU
*
* DESCRIPTION:
*   	Initialize the scratch pad SRAM region in the PMU so that all routines
*	needed for deepIdle, standby and DVFS are relocated from the DDR into the
*	SRAM
*
* INPUT:
*       None
* OUTPUT:
*	None
* RETURN:
*    	MV_OK	: All Functions relocated to PMU SRAM successfully
*	MV_FAIL	: At least on function failed relocation
*******************************************************************************/
MV_STATUS mvPmuSramInit (MV_32 ddrTermGpioCtrl)
{
	/* Allocate enough space for the DDR paramters */
	if ((_mvPmuSramDdrParamPtr = mvPmuSramRelocate(NULL,
		(mvDramIfParamCountGet() * sizeof(MV_DDR_MC_PARAMS)))) == NULL)
		return MV_FAIL;

#ifndef CONFIG_DOVE_REV_Z0	
	/* Convert DDR base address from External to Internal Space */
	_mvPmuSramDdrParamPtrInt =  (MV_VOID*)(((MV_U32)_mvPmuSramDdrParamPtr &
					     (PMU_SCRATCHPAD_SIZE-1)) |
					    (PMU_SCRATCHPAD_INT_BASE));
#endif

	/* Allocate enough space for the DDR paramters */
	if ((_mvPmuSramDdrInitPollPtr = mvPmuSramRelocate(NULL, 
					sizeof(MV_DDR_INIT_POLL_AMV))) == NULL)
		return MV_FAIL;

	/* Convert DDR base address from External to Internal Space */
	_mvPmuSramDdrInitPollPtrInt =  (MV_VOID*)(((MV_U32)_mvPmuSramDdrInitPollPtr &
					     (PMU_SCRATCHPAD_SIZE-1)) |
					    (PMU_SCRATCHPAD_INT_BASE));

	/* CODE SECTION STARTS HERE - ALLIGN TO CACHE LINE */
	mvPmuSramOffs += 31;
	mvPmuSramOffs &= ~31;
	
	/* Relocate the DDR reconfiguration function */
	if ((_mvPmuSramDdrReconfigPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramDdrReconfigFunc,
		mvPmuSramDdrReconfigFuncSZ)) == NULL)
		return MV_FAIL;

	/* Relocate the DeepIdle functions */
	if ((_mvPmuSramDeepIdleEnterPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramDeepIdleEnterFunc,
		mvPmuSramDeepIdleEnterFuncSZ)) == NULL)
		return MV_FAIL;
	if ((_mvPmuSramDeepIdleExitPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramDeepIdleExitFunc,
		mvPmuSramDeepIdleExitFuncSZ)) == NULL)
		return MV_FAIL;

	/* Relocate the Standby functions */
	if ((_mvPmuSramStandbyEnterPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramStandbyEnterFunc,
		mvPmuSramStandbyEnterFuncSZ)) == NULL)
		return MV_FAIL;
	if ((_mvPmuSramStandbyExitPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramStandbyExitFunc,
		mvPmuSramStandbyExitFuncSZ)) == NULL)
		return MV_FAIL;

	/* Relocate the CPU DFS function */
	if ((_mvPmuSramCpuDfsPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramCpuDfsFunc,
		mvPmuSramCpuDfsFuncSZ)) == NULL)
		return MV_FAIL;

	/* Save the DDR termination GPIO information */
	if ((ddrTermGpioCtrl >= 0) | (ddrTermGpioCtrl <= 31))
	{	
		MV_MEMIO_LE32_WRITE(PMU_SP_TERM_EN_CTRL_ADDR, 0x1);
		MV_MEMIO_LE32_WRITE(PMU_SP_TERM_GPIO_MASK_ADDR, (0x1 << ddrTermGpioCtrl));
	}
	else
	{
		MV_MEMIO_LE32_WRITE(PMU_SP_TERM_EN_CTRL_ADDR, 0x0);
		MV_MEMIO_LE32_WRITE(PMU_SP_TERM_GPIO_MASK_ADDR, 0x0);
	}

	return MV_OK;
}

/*******************************************************************************
* mvPmuSramDdrTimingPrep - Prepare new DDR timing params
*
* DESCRIPTION:
*   	Request the new timing parameters for the DDR MC according to the new
*	CPU:DDR ratio requested. These parameters are saved on the SRAM to be
*	set later in the DDR DFS sequence.
*
* INPUT:
*       ddrFreq: new target DDR frequency
*	cpuFreq: CPU frequency to calculate the values againt
* OUTPUT:
*	None
* RETURN:
*	status
*******************************************************************************/
MV_STATUS mvPmuSramDdrTimingPrep(MV_U32 ddrFreq, MV_U32 cpuFreq, MV_U32 * cnt)
{
	MV_U32 clear_size = (mvDramIfParamCountGet() * sizeof(MV_DDR_MC_PARAMS));
	MV_U32 i;
	MV_U32 * ptr = (MV_U32*) _mvPmuSramDdrParamPtrInt;

	/* Clear the whole region to zeros first */
	for (i=0; i<(clear_size/4); i++)
		ptr[i] = 0x0;
		
	/* Get the new timing parameters from the DDR HAL */
	return mvDramReconfigParamFill(ddrFreq, cpuFreq, (MV_DDR_MC_PARAMS*)_mvPmuSramDdrParamPtrInt, cnt);
}

/*******************************************************************************
* mvPmuSramDeepIdleResumePrep - Prepare information needed by the BootROM to resume
*       from Deep Idle Mode.
*
* DESCRIPTION:
*	Prepare the necessary register configuration to be executed by the 
*	BootROM before jumping back to the resume code in the SRAM.
*
* INPUT:
*       None
* OUTPUT:
*	None
* RETURN:
*	status
*******************************************************************************/
MV_STATUS mvPmuSramDeepIdleResumePrep(MV_VOID)
{
	MV_U32 reg, i;

	/* set the resume address */
	MV_REG_WRITE(PMU_RESUME_ADDR_REG, PmuSpVirt2Phys(_mvPmuSramDeepIdleExitPtr)); 

	/* Prepare the resume descriptors */
	reg = ((PMU_DEEPIDLE_DESCR_CNT << PMU_RC_DISC_CNT_OFFS) & PMU_RC_DISC_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_CTRL_REG, reg); 

	/* Fill in the used descriptors */
	for (i=0; i<PMU_DEEPIDLE_DESCR_CNT; i++)
	{
		// TBD
	}

	/* Clear out all non used descriptors */
	for (i=PMU_DEEPIDLE_DESCR_CNT; i<PMU_MAX_DESCR_CNT; i++)
	{
		MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(i), 0x0);
		MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(i), 0x0);
	}

	return MV_OK;
}

/*******************************************************************************
* mvPmuSramStandbyResumePrep - Prepare information needed by the BootROM to resume
*       from Standby Mode.
*
* DESCRIPTION:
*	Prepare the necessary register configuration to be executed by the 
*	BootROM before jumping back to the resume code in the SRAM.
*
* INPUT:
*       ddrFreq: DDR frequency to be configured upon resume from Standby
* OUTPUT:
*	None
* RETURN:
*	status
*******************************************************************************/
MV_STATUS mvPmuSramStandbyResumePrep(MV_U32 ddrFreq)
{
	MV_U32 reg, i, cnt;
	MV_U32 clear_size = (mvDramIfParamCountGet() * sizeof(MV_DDR_MC_PARAMS));
	MV_U32 * ptr = (MV_U32*) _mvPmuSramDdrParamPtrInt;
#ifdef CONFIG_DOVE_REV_Z0
	MV_U32 * srcptr = (MV_U32*) (PMU_SCRATCHPAD_EXT_BASE);
	MV_U32 * dstptr = (MV_U32*) (PMU_CESA_SP_BASE);
#endif

	/* Set the resume address */
	MV_REG_WRITE(PMU_RESUME_ADDR_REG, PmuSpVirt2Phys(_mvPmuSramStandbyExitPtr));

	/* Prepare the resume control parameters */
	reg = ((PMU_STANDBY_DESCR_CNT << PMU_RC_DISC_CNT_OFFS) & PMU_RC_DISC_CNT_MASK);	 
#ifndef CONFIG_DOVE_REV_Z0
	reg |= ((PMU_SP_TARGET_ID << PMU_RC_WIN6_TARGET_OFFS) & PMU_RC_WIN6_TARGET_MASK);
	reg |= ((PMU_SP_ATTRIBUTE << PMU_RC_WIN6_ATTR_OFFS) & PMU_RC_WIN6_ATTR_MASK);
	reg |= (DOVE_SCRATCHPAD_PHYS_BASE & PMU_RC_WIN6_BASE_MASK);
#endif
	MV_REG_WRITE(PMU_RESUME_CTRL_REG, reg);

	/* Prepare DDR paramters in the scratch pad for BootROM */
	for (i=0; i<(clear_size/4); i++)	
		ptr[i] = 0x0;
	if (mvDramIfParamFill(ddrFreq, (MV_DDR_MC_PARAMS*)_mvPmuSramDdrParamPtrInt, &cnt) != MV_OK)
		return MV_FAIL;

	/* Discriptor 0: DDR timing parametrs */
	reg = PMU_RD_CTRL_DISC_TYPE_32AV;
	reg |= ((cnt << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(0), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(0), PmuSpVirt2Phys(_mvPmuSramDdrParamPtr));

	/* Prepare the DDR init done polling descriptor */
	if (mvDramInitPollAmvFill((MV_DDR_INIT_POLL_AMV*)_mvPmuSramDdrInitPollPtrInt) != MV_OK)
		return MV_FAIL;

	/* Descriptor 1: DDR Initiaization delay */
	reg = PMU_RD_CTRL_DISC_TYPE_POLL;
	reg |= ((1 << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(1), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(1), PmuSpVirt2Phys(_mvPmuSramDdrInitPollPtr));
	
	/* Clear out all non used descriptors */
	for (i=PMU_STANDBY_DESCR_CNT; i<PMU_MAX_DESCR_CNT; i++)
	{
		MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(i), 0x0);
		MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(i), 0x0);
	}

#ifdef CONFIG_DOVE_REV_Z0
	/*
	 * PMU ScratchPad BUG workaround
	 * Copy all SRAM to PMUSP in 32bit access
	 */	
	for (i=0; i<512; i++)
		dstptr[i] = srcptr[i];
#endif

	return MV_OK;
}

/*******************************************************************************
* mvPmuSramVirt2Phys - Convert virtual address to physical
*
* DESCRIPTION:
*	Convert virtual address to physical
*
* INPUT:
*       addr: virtual address
* OUTPUT:
*	None
* RETURN:
*	physical address
*******************************************************************************/
unsigned long mvPmuSramVirt2Phys(void *addr)
{
	return mvOsIoVirtToPhy(NULL, addr);
}
