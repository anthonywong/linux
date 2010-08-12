/****************************************************************************
*
*    Copyright (C) 2002 - 2008 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public Lisence as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public Lisence for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/






#ifndef __DEVICE_H
#define __DEVICE_H

/******************************************************************************\
******************************* gcoGALDEVICE Structure *******************************
\******************************************************************************/

typedef struct _gcoGALDEVICE
{
	/* Objects. */
	gcoOS				os;
	gcoKERNEL			kernel;

	/* Attributes. */
	gctSIZE_T			internalSize;
	gctPHYS_ADDR		internalPhysical;
	gctPOINTER			internalLogical;
	gcoVIDMEM			internalVidMem;
	gctSIZE_T			externalSize;
	gctPHYS_ADDR		externalPhysical;
	gctPOINTER			externalLogical;
	gcoVIDMEM			externalVidMem;
	gcoVIDMEM			contiguousVidMem;
	gctPOINTER			contiguousBase;
	gctPHYS_ADDR		contiguousPhysical;
	gctSIZE_T			contiguousSize;
	gctBOOL				contiguousMapped;
	gctPOINTER			contiguousMappedUser;
	gctSIZE_T			systemMemorySize;
	gctUINT32			systemMemoryBaseAddress;
	gctPOINTER			registerBase;
	gctSIZE_T			registerSize;
	gctUINT32			baseAddress;

	/* IRQ management. */
	gctINT				irqLine;
	gctBOOL				isrInitialized;
	gctBOOL				dataReady;

	/* Thread management. */
	struct task_struct	*threadCtxt;
	struct semaphore	sema;
	gctBOOL				threadInitialized;
	gctBOOL				killThread;

	/* Signal management. */
	gctINT				signal;

#ifdef CONFIG_PXA_DVFM    
    /* dvfm device index */
    gctINT              dvfm_dev_index;

    /* dvfm notifier */
    struct notifier_block *dvfm_notifier;

    /* GC register state can't be retained 
       after existing form D2 on PV2 board.
       So we must reset GC on this case.
    */
    gctBOOL             needResetAfterD2;
    gctBOOL             needD2DebugInfo;
#endif

    gctBOOL             enableDVFM;
    gctBOOL             enableLowPowerMode;

}
* gcoGALDEVICE;

#ifdef ANDROID
typedef struct MEMORY_RECORD
{
	gcuVIDMEM_NODE_PTR		node;

	struct MEMORY_RECORD *	prev;
	struct MEMORY_RECORD *	next;
}
MEMORY_RECORD, * MEMORY_RECORD_PTR;
#endif

typedef struct _gcsHAL_PRIVATE_DATA
{
    gcoGALDEVICE		device;
    gctPOINTER			mappedMemory;
	gctPOINTER			contiguousLogical;
#ifdef ANDROID
	MEMORY_RECORD		memoryRecordList;
#endif
}
gcsHAL_PRIVATE_DATA, * gcsHAL_PRIVATE_DATA_PTR;

gceSTATUS gcoGALDEVICE_Setup_ISR(
	IN gcoGALDEVICE Device
	);

gceSTATUS gcoGALDEVICE_Release_ISR(
	IN gcoGALDEVICE Device
	);

gceSTATUS gcoGALDEVICE_Start_Thread(
	IN gcoGALDEVICE Device
	);

gceSTATUS gcoGALDEVICE_Stop_Thread(
	gcoGALDEVICE Device
	);

gceSTATUS gcoGALDEVICE_Start(
	IN gcoGALDEVICE Device
	);

gceSTATUS gcoGALDEVICE_Stop(
	gcoGALDEVICE Device
	);

gceSTATUS gcoGALDEVICE_Construct(
	IN gctINT IrqLine,
	IN gctUINT32 RegisterMemBase,
	IN gctSIZE_T RegisterMemSize,
	IN gctUINT32 ContiguousBase,
	IN gctSIZE_T ContiguousSize,
	IN gctSIZE_T BankSize,
	IN gctINT FastClear,
	IN gctINT Compression,
	IN gctUINT32 BaseAddress,
	IN gctINT Signal,
	OUT gcoGALDEVICE *Device
	);

gceSTATUS gcoGALDEVICE_Destroy(
	IN gcoGALDEVICE Device
	);

#endif // __DEVICE_H
