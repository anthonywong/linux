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

#define MV_ASMLANGUAGE

#include "mvCommon.h"
#include "mvOs_asm.h"
#include "mvSysPmuConfig.h"
#include "pmu/mvPmuRegs.h"



#ifndef ENTRY
#define ENTRY(name) \
	.globl name; \
	.align 4,0x90; \
	name:
#endif

	.text

/***************************************************************
 * mvPmuSramDdrReconfigFunc: Change the DDR frequency
 * Inputs:
 *    r0: address of first Addr/Value couple
 *    r1: count of Addr/Value couples 
 * Outputs:
 *    None
 **************************************************************/
	.align 5
ENTRY(mvPmuSramDdrReconfigFunc)
	stmfd	sp!, {r4 - r10, lr}	@ save registers on stack

	mov	r9, r0
	mov	r10, r1

	/*
	 * Block all data requests to the MC from the SB
	 */
	ldr	r2, ddfs_mc_ctrl_reg
	mov	r3, #0x1
	str	r3, [r2]


	/* 
	 * Disable All SB access ro DDR
	 */
/*
	ldr	r3, ddfs_arbit_val
	ldr	r2, ddfs_arbit_low_reg	
	ldr	r7, [r2]			@ r7 saves Arbitor HI
	str	r3, [r2]
	ldr	r2, ddfs_arbit_hi_reg
	ldr	r8, [r2]			@ r8 saves Arbitor LOW
	str	r3, [r2]	
*/
	/*
	 * Set DDR in Self refresh mode and poll status bit
	 */
	ldr	r2, ddfs_ctrl_reg
	mov	r3, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r4, [r2]
	orr	r4, r4, r3
	str	r4, [r2]
ddr_fs_sr_loop:
	ldr	r4, [r2]
	tst	r4, #PMU_CTRL_MC_SR_ACK_MASK
	beq	ddr_fs_sr_loop

	/*
	 * Enter WFI
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4


	/*
	 * Exit DDR self refresh mode if active
	 */
	ldr	r0, ddfs_ctrl_reg
	mov	r1, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r2, [r0]
	tst 	r2, #PMU_CTRL_MC_SR_REQ_MASK
	beq	skip_ddrfs_sr_exit
	bic	r2, r2, r1
	str	r2, [r0]
ddrfs_sr_exit_loop:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	bne	ddrfs_sr_exit_loop
skip_ddrfs_sr_exit:

	/*
	 * Loop and execute MC init addr/value couples
	 */
	mov	r0, r9
	mov	r1, r10
	ldr	r4, ddfs_nb_offset
	ldr	r5, ddfs_nb_mask
mc_init_loop:
	cmp	r1, #0
	beq	mc_init_done
	ldr 	r2, [r0]			@ Address
	add	r0, #4
	ldr	r3, [r0]			@ Value
	add	r0, #4
	bic	r2, r2, r5			
	orr	r2, r2, r4			@ NB Offset
	str	r3, [r2]			@ [Address]=Value
	sub	r1, r1, #1
	b	mc_init_loop
mc_init_done:

/*************************************************************/
/*   FOR DEBUG ONLY - BLINK LED ON MPP7 AS A LIFE INDICATION */
/*************************************************************/
/*
	mov	r4, #8
	ldr	r0, led_addr
	ldr	r1, led_val_on
	ldr	r2, led_val_off

loop_blink:
	str	r1, [r0]
	ldr	r3, =0x40000
loop_delay:
	sub	r3, r3, #1
	cmp	r3, #0
	bne	loop_delay

	str	r2, [r0]
	ldr	r3, =0x40000
loop_delay2:
	sub	r3, r3, #1
	cmp	r3, #0
	bne	loop_delay2

	sub	r4, r4, #1
	cmp	r4, #0
	bne	loop_blink
*/
/********************************************************************/

	/*
	 * UN-Block data requests to the MC from the SB
	 */
	ldr	r2, ddfs_mc_ctrl_reg
	mov	r3, #0x0
	str	r3, [r2]


	/* 
	 * Restore SB access ro DDR
	 */
/*
	ldr	r2, ddfs_arbit_low_reg	
	str	r7, [r2]			@ restore Arbitor HI
	ldr	r2, ddfs_arbit_hi_reg
	str	r8, [r2]			@ restore Arbitor LOW
*/
	ldmfd	sp!, {r4 - r10, pc}	@ restore regs and return

ddfs_nb_offset:
	.word (DOVE_NB_REGS_VIRT_BASE)
ddfs_nb_mask:
	.word (0xFFF00000)
ddfs_mc_ctrl_reg:
	.word (DOVE_NB_REGS_VIRT_BASE + 0x7E0)
ddfs_ctrl_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_CTRL_REG)
/*
ddfs_arbit_low_reg:
	.word(DOVE_SB_REGS_VIRT_BASE + 0xD02A8)
ddfs_arbit_hi_reg:
	.word(DOVE_SB_REGS_VIRT_BASE + 0xD02AC)
ddfs_arbit_val:
	.word(0xFFFFFFFF)
*/
/*********************************************************************/
/*
led_addr:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_SIG_SLCT_CTRL_0_REG)
led_val_on:
	.word (0x1048B500)
led_val_off:
	.word (0x2048B500)
*/
/*********************************************************************/
ENTRY(mvPmuSramDdrReconfigFuncSZ)
	.word	. - mvPmuSramDdrReconfigFunc



/***************************************************************
 * mvPmuSramDeepIdleEnterFunc: Enter Deep Idle mode
 * Inputs:
 *    r0: 0x0: enter ddr SR, else enter
 * Outputs:
 *    None
 **************************************************************/
	.align 5
ENTRY(mvPmuSramDeepIdleEnterFunc)
	stmfd	sp!, {r4-r12, lr}		@ save general regs
	mrc 	p15, 0, r2, c1, c0, 0		@ save CP15 - CR
	mrc 	p15, 0, r3, c3, c0, 0		@ save CP15 - DACR
	mrc 	p15, 0, r4, c13, c0, 0		@ save CP15 - FCSE
	mrc 	p15, 0, r5, c2, c0, 0		@ save CP15 - TTBR
	mrc     p15, 0, r6, c13, c0, 1          @ save CP15 - context ID
	mrc	p15, 1, r7, c15, c1, 0		@ save CP15 extra features
	mov	r8, r13				@ save resume SP
	stmfd	sp!, {r2-r8}

	/*
	 * Save the physical address of the resume SP
	 */
	mov	r4, r0				@ suspend mode needed later
	mov	r5, r2				@ CR needed later for L2
	mov	r0, sp
	ldr	r1, die_vtp
	adr	lr, sb_pc_v2p_res
	mov	pc, r1
sb_pc_v2p_res:
	adr	r1, dix_saved_sp

#ifndef CONFIG_DOVE_REV_Z0
	/*
	 * Write SP to scratch NVRAM
	 * First Convert external to internal addrress space
	 */
	ldr	r3, die_nvram_sp_mask
	and	r1, r1, r3
	ldr	r3, die_nvram_int_base
	orr	r1, r1, r3
#endif
	str	r0, [r1]

	/*
	 * Flush all L1 DCache
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c14, 0		@ D cache clean+invalidate
	mcr     p15, 0, r0, c7, c5, 6           @ flush BTAC/BTB
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
	tst	r5, #0x04000000			@ check if L2 is enabled
	beq	die_skip_l2_clean
	mcr 	p15, 1, r0, c7, c11, 0		@ clean all L2 cache
die_skip_l2_clean:
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer

	/*
	 * Set DDR in Self refresh mode and poll status bit
	 */
	cmp	r4, #0
	bne	die_leave_ddr_active
	ldr	r0, die_ctrl_reg
	mov	r1, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]
die_sr_loop:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	beq	die_sr_loop

	/*
	 * Disable DDR Terminations
	 */
	ldr	r0, die_term_ctrl_addr
	ldr	r0, [r0]
	cmp	r0, #1
	bne	die_leave_ddr_active
	ldr	r2, die_term_mask_addr
	ldr	r2, [r2]
	ldr	r0, die_gpio_out_val_reg
	ldr	r1, [r0]
	bic	r1, r1, r2
	str	r1, [r0]
	
die_leave_ddr_active:
	/*
	 * Initiate Deep Idle flow
	 */
	ldr	r0, die_ctrl_reg
	mov	r1, #PMU_CTRL_DEEPIDLE_MODE_EN_MASK
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]

	/*
	 * Relocate the Internal Registers Window to h/w default
	 */
	ldr	r1, die_irb_reg
	mov	r0, #0xD0000000
	str	r0, [r1]	

	/*
	 * Enter Deep Idle by entering WFI
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4	

die_vtp:
	.word mvPmuSramVirt2Phys
die_ctrl_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_CTRL_REG)
die_irb_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + 0x20080)
die_gpio_out_val_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + 0xD0400)
die_term_ctrl_addr:
	.word (PMU_SP_TERM_EN_CTRL_ADDR)
die_term_mask_addr:
	.word (PMU_SP_TERM_GPIO_MASK_ADDR)
#ifndef CONFIG_DOVE_REV_Z0
die_nvram_int_base:
	.word (PMU_SCRATCHPAD_INT_BASE)
die_nvram_sp_mask:
	.word (0x7FF)
#endif

ENTRY(mvPmuSramDeepIdleEnterFuncSZ)
	.word	. - mvPmuSramDeepIdleEnterFunc



/***************************************************************
 * mvPmuSramDeepIdleExitFunc: Rollback from Deep Idle mode
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
	.align 5
ENTRY(mvPmuSramDeepIdleExitFunc)
	/*
	 * Move back Internal regs to Linux base
	 */
	ldr	r1, dix_irbd_reg
	mov	r0, #DOVE_SB_REGS_PHYS_BASE
	str	r0, [r1]

	mov	r1, #0
	mcr	p15, 0, r1, c5, c7, 0		@ Invalidate both caches, BTC, BTAC
	mcr	p15, 0, r1, c8, c7, 0		@ TLB invalidate	
	mcr	p15, 1, r1, c7, c7, 0		@ Invalidate entire L2
	mcr	p15, 0, r1, c7, c5, 4		@ Prefetch Flush - ISB

	/*
	 * Exit DDR self refresh mode if active
	 */
	ldr	r0, dix_ctrl_reg
	mov	r1, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r2, [r0]
	tst 	r2, #PMU_CTRL_MC_SR_REQ_MASK
	beq	dix_ddr_active
	bic	r2, r2, r1
	str	r2, [r0]
dix_sr_exit_loop:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	bne	dix_sr_exit_loop		

	/*
	 * Re-enable DDR terminations
	 */
	ldr	r0, dix_term_ctrl_addr
	ldr	r0, [r0]
	cmp	r0, #1
	bne	dix_ddr_active
	ldr	r2, dix_term_mask_addr
	ldr	r2, [r2]
	ldr	r0, dix_gpio_out_val_reg
	ldr	r1, [r0]
	orr	r1, r1, r2
	str	r1, [r0]

	mov	r1, #0x40
dix_term_enable_delay:
	sub	r1, r1, #1
	cmp	r1, #0
	bne	dix_term_enable_delay	

dix_ddr_active:	
	ldr	r0, dix_saved_sp		@ stack phys addr
	ldmfd	r0, {r2-r8}
	mcr	p15, 1, r7, c15, c1, 0		@ restore CP15 extra features		
	mcr     p15, 0, r6, c13, c0, 1          @ restore CP15 - context ID
	mcr 	p15, 0, r5, c2, c0, 0		@ restore CP15 - TTBR
	mcr 	p15, 0, r4, c13, c0, 0		@ restore CP15 - FCSE
	mcr 	p15, 0, r3, c3, c0, 0		@ restore CP15 - DACR
	adr	r3, dix_resume_after_mmu	@ physical resume address
	ldr	r4, dix_virt_mask
	and	r3, r3, r4
	ldr	r4, dix_virt_offs
	orr	r3, r3, r4			@ virtual resume address

	/*
	* DDR Sync workaround
	*/
	ldr     r1, dix_ddr_sync_reg
	ldr     r0, dix_ddr_sync_val
	str     r0, [r1]
	ldr	r0, [r1]

	/*
	 * temporarily map PMU SRAM into the page table so that PA=VA
	 * otherwise prefetch abort occurs after MMU is turned on
	 */
	ldr	r1, dix_virt_offs
	mov	r1, r1, lsr #20
	ldr	r0, [r5, r1, lsl #2]		@ use PMU mapping mode
	bic	r0, r0, #0xFF000000		@ Clear bits 24-31
	bic	r0, r0, #0x00F00000		@ Clear bits 20-23
	ldr	r1, dix_phys_offs
	mov	r1, r1, lsr #20
	orr	r7, r0, r1, lsl #20		@ temporay mapping
	ldr	r4, [r5, r1, lsl #2]		@ save original mapping
	str     r7, [r5, r1, lsl #2]		@ set temporary mapping

	/*
	 * Map the page table to its physical euivalent (PA=VA)
	 * Needed to restore the original PTA in virtual mode
	 */
	mov	r6, r5, lsr #20
	orr	r7, r0, r6, lsl #20		@ temporay mapping
	ldr	r9, [r5, r6, lsl #2]		@ save original mapping
	str     r7, [r5, r6, lsl #2]		@ set temporary mapping	
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr	p15, 0, r0, c7, c10, 5		@ data memory barrier

	.align 5
	mcr 	p15, 0, r2, c1, c0, 0		@ restore CP15 - CR
	mov 	r0, #0
	mcr	p15, 0, r0, c7, c5, 4		@ Prefetch Flush - ISB
	mov	pc, r3

dix_saved_sp:
	.word	0				@ physical SP saved here

	.text
dix_resume_after_mmu:
	str	r4, [r5, r1, lsl #2]		@ restore original PMU mapping
	str	r9, [r5, r6, lsl #2]		@ restore original PT mapping
	/*
	 * No need for cach flushes since the 2 PT entries are strongly ordered
	 */
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr     p15, 0, r0, c7, c10, 5          @ data memory barrier
	mcr	p15, 0, r0, c8, c7, 0		@ TLB invalidate
	mov	r13, r8				@ restore virtual SP
	ldmfd	sp!, {r4-r12, pc}		@ restore SVC registers

dix_irbd_reg:
	.word (0xD0020080)
dix_ctrl_reg:
	.word (DOVE_SB_REGS_PHYS_BASE + PMU_CTRL_REG)
dix_virt_mask:
	.word (0xFFFFF)
dix_virt_offs:
	.word (DOVE_SCRATCHPAD_VIRT_BASE)
dix_phys_offs:
	.word (DOVE_SCRATCHPAD_PHYS_BASE)
dix_gpio_out_val_reg:
	.word (DOVE_SB_REGS_PHYS_BASE + 0xD0400)
dix_term_ctrl_addr:
	.word (PMU_SP_TERM_EN_CTRL_ADDR_PHYS)
dix_term_mask_addr:
	.word (PMU_SP_TERM_GPIO_MASK_ADDR_PHYS)
dix_ddr_sync_reg:
	.word (0xF1800240)
dix_ddr_sync_val:
	.word (0x80000000)

ENTRY(mvPmuSramDeepIdleExitFuncSZ)
	.word	. - mvPmuSramDeepIdleExitFunc


/***************************************************************
 * mvPmuSramStandbyEnterFunc: Enter Standby mode
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
	.align 5
ENTRY(mvPmuSramStandbyEnterFunc)

	stmfd	sp!, {r4-r12, lr}		@ save general regs
	mrc 	p15, 0, r2, c1, c0, 0		@ save CP15 - CR
	mrc 	p15, 0, r3, c3, c0, 0		@ save CP15 - DACR
	mrc 	p15, 0, r4, c13, c0, 0		@ save CP15 - FCSE
	mrc 	p15, 0, r5, c2, c0, 0		@ save CP15 - TTBR
	mrc     p15, 0, r6, c13, c0, 1          @ save CP15 - context ID
	mrc	p15, 1, r7, c15, c1, 0		@ save extra features
	mov	r8, r13				@ save resume SP
	stmfd	sp!, {r2-r8}

	/*
	 * Save the physical address of the resume SP
	 */
	mov	r4, r0				@ suspend mode needed later
	mov	r5, r2				@ CR needed later for L2
	mov	r0, sp
	ldr	r1, sbe_vtp
	adr	lr, sbe_pc_v2p_res
	mov	pc, r1
sbe_pc_v2p_res:
	adr	r1, sbx_saved_sp

	/*
	 * Write SP to scratch NVRAM
	 * Convert to internal addrress space
	 */
	ldr	r2, sbe_nvram_sp_mask
	and	r1, r1, r2
	ldr	r2, sbe_nvram_int_base
	orr	r1, r1, r2
	str	r0, [r1]

	/*
	 * Flush all L1 Cache
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c14, 0		@ D cache clean+invalidate
	mcr     p15, 0, r0, c7, c5, 6           @ flush BTAC/BTB
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
	tst	r5, #0x04000000
	beq	sbe_skip_l2_clean
	mcr 	p15, 1, r0, c7, c11, 0		@ clean all L2 cache
sbe_skip_l2_clean:
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr	p15, 0, r0, c7, c10, 5		@ data memory barrier

	/*
	 * Initiate standby flow
	 */
	ldr	r0, sbe_ctrl_reg
	mov	r1, #(PMU_CTRL_STANDBY_MODE_EN_MASK | PMU_CTRL_DDR_SLF_RFRSH_EN_MASK)
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]
#if 0
	/*
	 * Block all data requests to the MC from the SB
	 * Step 4: 0xB00007E0 = 0x1
	 */
	ldr	r0, sbe_mc_ctrl_reg
	mov	r1, #0x1
	str	r1, [r0]	
#endif
	/*
	 * Enter standby by entering WFI
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4	

#ifdef CONFIG_DOVE_REV_Z0
sbe_nvram_int_base:
	.word (PMU_CESA_SP_BASE)
#else
sbe_nvram_int_base:
	.word (PMU_SCRATCHPAD_INT_BASE)
#endif
sbe_nvram_sp_mask:
	.word (0x7FF)
sbe_vtp:
	.word mvPmuSramVirt2Phys
sbe_ctrl_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_CTRL_REG)
sbe_mc_ctrl_reg:
	.word (DOVE_NB_REGS_VIRT_BASE + 0x7E0)

ENTRY(mvPmuSramStandbyEnterFuncSZ)
	.word	. - mvPmuSramStandbyEnterFunc



/***************************************************************
 * mvPmuSramStandbyExitFunc: Rollback from Standby mode
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
	.align 5
ENTRY(mvPmuSramStandbyExitFunc)
	mov	r1, #0
	mcr	p15, 0, r1, c5, c7, 0		@ Invalidate both caches, BTC, BTAC
	mcr	p15, 0, r1, c8, c7, 0		@ TLB invalidate	
	mcr	p15, 1, r1, c7, c7, 0		@ Invalidate entire L2
	mcr	p15, 0, r1, c7, c5, 4		@ Prefetch Flush - ISB

	ldr	r0, sbx_saved_sp		@ stack phys addr
	ldmfd	r0, {r2-r8}

	mcr	p15, 1, r7, c15, c1, 0		@ restore CP15 extra features		
	mcr     p15, 0, r6, c13, c0, 1          @ restore CP15 - context ID
	mcr 	p15, 0, r5, c2, c0, 0		@ restore CP15 - TTBR
	mcr 	p15, 0, r4, c13, c0, 0		@ restore CP15 - FCSE
	mcr 	p15, 0, r3, c3, c0, 0		@ restore CP15 - DACR
	adr	r3, sbx_resume_after_mmu	@ physical resume address
	ldr	r4, sbx_virt_mask
	and	r3, r3, r4
	ldr	r4, sbx_virt_offs
	orr	r3, r3, r4			@ virtual resume address

	/*
	 * temporarily map PMU SRAM into the page table so that PA=VA
	 * otherwise prefetch abort occurs after MMU is turned on
	 */
	ldr	r1, sbx_virt_offs
	mov	r1, r1, lsr #20
	ldr	r0, [r5, r1, lsl #2]		@ use PMU mapping mode
	bic	r0, r0, #0xFF000000		@ Clear bits 24-31
	bic	r0, r0, #0x00F00000		@ Clear bits 20-23
	ldr	r1, sbx_phys_offs
	mov	r1, r1, lsr #20
	orr	r7, r0, r1, lsl #20		@ temporay mapping
	ldr	r4, [r5, r1, lsl #2]		@ save original mapping
	str     r7, [r5, r1, lsl #2]		@ set temporary mapping

	/*
	 * Map the page table to its physical euivalent (PA=VA)
	 * Needed to restore the original PTA in virtual mode
	 */
	mov	r6, r5, lsr #20
	orr	r7, r0, r6, lsl #20		@ temporay mapping
	ldr	r9, [r5, r6, lsl #2]		@ save original mapping
	str     r7, [r5, r6, lsl #2]		@ set temporary mapping	

	.align 5
	mcr 	p15, 0, r2, c1, c0, 0		@ restore CP15 - CR
	mov 	r0, #0
	mcr	p15, 0, r0, c7, c5, 4		@ Prefetch Flush - ISB
	mov	pc, r3

sbx_saved_sp:
	.word	0				@ physical SP saved here

	.text
sbx_resume_after_mmu:
	str	r4, [r5, r1, lsl #2]		@ restore original PMU mapping
	str	r9, [r5, r6, lsl #2]		@ restore original PT mapping
	mcr	p15, 0, r0, c8, c7, 0		@ TLB invalidate
	mov	r13, r8				@ restore virtual SP
	ldmfd	sp!, {r4-r12, pc}		@ restore SVC registers

sbx_virt_mask:
	.word (0xFFFFF)
sbx_virt_offs:
	.word (DOVE_SCRATCHPAD_VIRT_BASE)
sbx_phys_offs:
	.word (DOVE_SCRATCHPAD_PHYS_BASE)

ENTRY(mvPmuSramStandbyExitFuncSZ)
	.word	. - mvPmuSramStandbyExitFunc


/***************************************************************
 * mvPmuSramCpuDfsFunc: Change the CPU frequency
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
	.align 5
ENTRY(mvPmuSramCpuDfsFunc)
	stmfd	sp!, {r0 - r10, lr}	@ save registers on stack

	/*
	 * Force DDR in SR Mode
	 */
	ldr	r0, dfs_ctrl_reg
	mov	r1, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]
dfs_sr_enter_loop:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	beq	dfs_sr_enter_loop

	/*
	 * Enter WFI
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4

	/*
	 * Exit DDR SR Mode
	 */
	ldr	r0, dfs_ctrl_reg
	mov	r1, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r2, [r0]
	bic	r2, r2, r1
	str	r2, [r0]
dfs_sr_exit_loop:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	bne	dfs_sr_exit_loop

	ldmfd	sp!, {r0 - r10, pc}	@ restore regs and return

dfs_ctrl_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_CTRL_REG)

ENTRY(mvPmuSramCpuDfsFuncSZ)
	.word	. - mvPmuSramCpuDfsFunc
