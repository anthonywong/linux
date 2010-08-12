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






#ifndef __aqhal_h_
#define __aqhal_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "aqTypes.h"
#include "aqHalEnum.h"
#include "gcHAL_Base.h"
#include "gcProfiler.h"

/******************************************************************************\
********************************* Inline Macro *********************************
\******************************************************************************/

#if defined(LINUX)
#	define gcmINLINE inline
#else
#	define gcmINLINE __inline
#endif

/******************************************************************************\
******************************* Alignment Macros *******************************
\******************************************************************************/

#define gcmALIGN(n, align) \
( \
	((n) + ((align) - 1)) & ~((align) - 1) \
)

/******************************************************************************\
***************************** Element Count Macro *****************************
\******************************************************************************/

#define gcmSIZEOF(a) \
( \
	(gctSIZE_T) (sizeof(a)) \
)

#define gcmCOUNTOF(a) \
( \
	sizeof(a) / sizeof(a[0]) \
)

/******************************************************************************\
******************************** gcsOBJECT Object *******************************
\******************************************************************************/

/* Macro to combine four characters into a Charcater Code. */
#define gcmCC(c1, c2, c3, c4) \
( \
	(char)(c1) \
	| \
	((char)(c2) << 8) \
	| \
	((char)(c3) << 16) \
	| \
	((char)(c4) << 24) \
)

/* Type of objects. */
typedef enum _gceOBJECT_TYPE
{
	gcvOBJ_UNKNOWN 				= 0,
	gcvOBJ_2D					= gcmCC('2','D',' ',' '),
	gcvOBJ_3D					= gcmCC('3','D',' ',' '),
	gcvOBJ_ATTRIBUTE			= gcmCC('A','T','T','R'),
	gcvOBJ_BRUSHCACHE			= gcmCC('B','R','U','$'),
	gcvOBJ_BRUSHNODE			= gcmCC('B','R','U','n'),
	gcvOBJ_BRUSH				= gcmCC('B','R','U','o'),
	gcvOBJ_BUFFER				= gcmCC('B','U','F','R'),
	gcvOBJ_COMMAND				= gcmCC('C','M','D',' '),
	gcvOBJ_COMMANDBUFFER		= gcmCC('C','M','D','B'),
	gcvOBJ_CONTEXT				= gcmCC('C','T','X','T'),
	gcvOBJ_DUMP					= gcmCC('D','U','M','P'),
	gcvOBJ_EVENT				= gcmCC('E','V','N','T'),
	gcvOBJ_FUNCTION				= gcmCC('F','U','N','C'),
	gcvOBJ_HAL					= gcmCC('H','A','L',' '),
	gcvOBJ_HARDWARE				= gcmCC('H','A','R','D'),
	gcvOBJ_HEAP					= gcmCC('H','E','A','P'),
	gcvOBJ_INDEX				= gcmCC('I','N','D','X'),
	gcvOBJ_INTERRUPT			= gcmCC('I','N','T','R'),
	gcvOBJ_KERNEL				= gcmCC('K','E','R','N'),
	gcvOBJ_MMU					= gcmCC('M','M','U',' '),
	gcvOBJ_OS					= gcmCC('O','S',' ',' '),
	gcvOBJ_OUTPUT				= gcmCC('O','U','T','P'),
	gcvOBJ_PAINT				= gcmCC('P','N','T',' '),
	gcvOBJ_PATH					= gcmCC('P','A','T','H'),
	gcvOBJ_QUEUE				= gcmCC('Q','U','E',' '),
	gcvOBJ_SAMPLER				= gcmCC('S','A','M','P'),
	gcvOBJ_SHADER				= gcmCC('S','H','D','R'),
	gcvOBJ_STREAM				= gcmCC('S','T','R','M'),
	gcvOBJ_SURF					= gcmCC('S','U','R','F'),
	gcvOBJ_TEXTURE				= gcmCC('T','X','T','R'),
	gcvOBJ_UNIFORM				= gcmCC('U','N','I','F'),
	gcvOBJ_VARIABLE				= gcmCC('V','A','R','I'),
	gcvOBJ_VERTEX				= gcmCC('V','R','T','X'),
	gcvOBJ_VIDMEM				= gcmCC('V','M','E','M'),
	gcvOBJ_VG					= gcmCC('V','G',' ',' '),
}
gceOBJECT_TYPE;

/* gcsOBJECT object defintinon. */
typedef struct _gcsOBJECT
{
	/* Type of an object. */
	gceOBJECT_TYPE				type;
}
gcsOBJECT;

/* Kernel settings. */
typedef struct _gcsKERNEL_SETTINGS
{
	/* Used RealTime signal between kernel and user. */
	gctINT signal;
}
gcsKERNEL_SETTINGS;

/*******************************************************************************
**
**	gcmVERIFY_OBJECT
**
**		Assert if an object is invalid or is not of the specified type.  If the
**		object is invalid or not of the specified type, AQSTATUS_INVALD_OBJECT
**		will be returned from the current function.  In retail mode this macro
**		does nothing.
**
**	ARGUMENTS:
**
**		obj		Object to test.
**		t		Expected type of the object.
*/
#ifdef _DEBUG
#	define gcmVERIFY_OBJECT(obj, t) \
		do \
		{ \
			if ( (obj == gcvNULL) || (((gcsOBJECT*)(obj))->type != t) ) \
			{ \
				gcmASSERT( \
					(obj != gcvNULL) && (((gcsOBJECT*)(obj))->type == t) \
					); \
				return gcvSTATUS_INVALID_OBJECT; \
			} \
		} \
		while (gcvFALSE)
#else
#	define gcmVERIFY_OBJECT(obj, t)
#endif

/******************************************************************************\
********************************** gcoOS Object *********************************
\******************************************************************************/

/* Query the video memory. */
gceSTATUS
gcoOS_QueryVideoMemory(
	IN gcoOS Os,
	OUT gctPHYS_ADDR * InternalAddress,
	OUT gctSIZE_T * InternalSize,
	OUT gctPHYS_ADDR * ExternalAddress,
	OUT gctSIZE_T * ExternalSize,
	OUT gctPHYS_ADDR * ContiguousAddress,
	OUT gctSIZE_T * ContiguousSize
	);

/* Allocate memory from the heap. */
gceSTATUS
gcoOS_Allocate(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Memory
	);

/* Free allocated memory. */
gceSTATUS
gcoOS_Free(
	IN gcoOS Os,
	IN gctPOINTER Memory
	);

/* Allocate paged memory. */
gceSTATUS
gcoOS_AllocatePagedMemory(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	OUT gctPHYS_ADDR * Physical
	);

/* Lock pages. */
gceSTATUS
gcoOS_LockPages(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical,
	OUT gctSIZE_T * PageCount
	);

/* Map pages. */
gceSTATUS
gcoOS_MapPages(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T PageCount,
	IN gctPOINTER PageTable
	);

/* Unlock pages. */
gceSTATUS
gcoOS_UnlockPages(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	);

/* Free paged memory. */
gceSTATUS
gcoOS_FreePagedMemory(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes
	);

/* Allocate non-paged memory. */
gceSTATUS
gcoOS_AllocateNonPagedMemory(
	IN gcoOS Os,
	IN gctBOOL InUserSpace,
	IN OUT gctSIZE_T * Bytes,
	OUT gctPHYS_ADDR * Physical,
	OUT gctPOINTER * Logical
	);

/* Free non-paged memory. */
gceSTATUS
gcoOS_FreeNonPagedMemory(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical
	);

/* Allocate contiguous memory. */
gceSTATUS
gcoOS_AllocateContiguous(
	IN gcoOS Os,
	IN gctBOOL InUserSpace,
	IN OUT gctSIZE_T * Bytes,
	OUT gctPHYS_ADDR * Physical,
	OUT gctPOINTER * Logical
	);

/* Free contiguous memory. */
gceSTATUS
gcoOS_FreeContiguous(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical,
	IN gctSIZE_T Bytes
	);

/* Get the number fo bytes per page. */
gceSTATUS
gcoOS_GetPageSize(
	IN gcoOS Os,
	OUT gctSIZE_T * PageSize
	);

/* Get the physical address of a corresponding logical address. */
gceSTATUS
gcoOS_GetPhysicalAddress(
	IN gcoOS Os,
	IN gctPOINTER Logical,
	OUT gctUINT32 * Address
	);

/* Map physical memory. */
gceSTATUS
gcoOS_MapPhysical(
	IN gcoOS Os,
	IN gctUINT32 Physical,
	IN gctUINT32 OriginalLogical,	
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	);

/* Unmap previously mapped physical memory. */
gceSTATUS
gcoOS_UnmapPhysical(
	IN gcoOS Os,
	IN gctPOINTER Logical,
	IN gctSIZE_T Bytes
	);

/* Read data from a hardware register. */
gceSTATUS
gcoOS_ReadRegister(
	IN gcoOS Os,
	IN gctUINT32 Address,
	OUT gctUINT32 * Data
	);

/* Write data to a hardware register. */
gceSTATUS
gcoOS_WriteRegister(
	IN gcoOS Os,
	IN gctUINT32 Address,
	IN gctUINT32 Data
	);

/* Write data to a 32-bit memory location. */
gceSTATUS
gcoOS_WriteMemory(
	IN gcoOS Os,
	IN gctPOINTER Address,
	IN gctUINT32 Data
	);

/* Map physical memory into the process space. */
gceSTATUS
gcoOS_MapMemory(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	);

/* Unmap physical memory from the process space. */
gceSTATUS
gcoOS_UnmapMemory(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	);

/* Create a new mutex. */
gceSTATUS
gcoOS_CreateMutex(
	IN gcoOS Os,
	OUT gctPOINTER * Mutex
	);

/* Delete a mutex. */
gceSTATUS
gcoOS_DeleteMutex(
	IN gcoOS Os,
	IN gctPOINTER Mutex
	);

/* Acquire a mutex. */
gceSTATUS
gcoOS_AcquireMutex(
	IN gcoOS Os,
	IN gctPOINTER Mutex,
	IN gctUINT32 Timeout
	);

/* Release a mutex. */
gceSTATUS
gcoOS_ReleaseMutex(
	IN gcoOS Os,
	IN gctPOINTER Mutex
	);

/* Atomically exchange a pair of 32-bit values. */
gceSTATUS
gcoOS_AtomicExchange(
	IN gcoOS Os,
    IN OUT gctUINT32_PTR Target,
	IN gctUINT32 NewValue,
    OUT gctUINT32_PTR OldValue
	);

/* Atomically exchange a pair of pointers. */
gceSTATUS
gcoOS_AtomicExchangePtr(
	IN gcoOS Os,
    IN OUT gctPOINTER * Target,
	IN gctPOINTER NewValue,
    OUT gctPOINTER * OldValue
	);

/* Delay a number of microseconds. */
gceSTATUS
gcoOS_Delay(
	IN gcoOS Os,
	IN gctUINT32 Delay
	);

/* Memory barrier. */
gceSTATUS
gcoOS_MemoryBarrier(
	IN gcoOS Os,
	IN gctPOINTER Address
	);

/* Map user pointer. */
gceSTATUS
gcoOS_MapUserPointer(
	IN gcoOS Os,
	IN gctPOINTER Pointer,
	IN gctSIZE_T Size,
	OUT gctPOINTER * KernelPointer
	);

/* Map user pointer. */
gceSTATUS
gcoOS_UnmapUserPointer(
	IN gcoOS Os,
	IN gctPOINTER Pointer,
	IN gctSIZE_T Size,
	IN gctPOINTER KernelPointer
	);

#if USE_EVENT_QUEUE
gceSTATUS
gcoOS_SuspendInterrupt(
	IN gcoOS Os
	);

gceSTATUS
gcoOS_ResumeInterrupt(
	IN gcoOS Os
	);
#endif

/* Notify the idle status. */
gceSTATUS
gcoOS_NotifyIdle(
	IN gcoOS Os,
	IN gctBOOL Idle
	);

gceSTATUS
gcoOS_SetConstraint(
	IN gcoOS Os,
	IN gctBOOL enableDVFM,
	IN gctBOOL enableLPM
	);

gceSTATUS
gcoOS_UnSetConstraint(
	IN gcoOS Os,
	IN gctBOOL enableDVFM,
	IN gctBOOL enableLPM
	);

/******************************************************************************\
********************************** Signal Object *********************************
\******************************************************************************/

/* User signal command codes. */
typedef enum _gceUSER_SIGNAL_COMMAND_CODES
{
	gcvUSER_SIGNAL_CREATE,
	gcvUSER_SIGNAL_DESTROY,
	gcvUSER_SIGNAL_SIGNAL,
	gcvUSER_SIGNAL_WAIT,
}
gceUSER_SIGNAL_COMMAND_CODES;

/* Create a signal. */
gceSTATUS
gcoOS_CreateSignal(
	IN gcoOS Os,
	IN gctBOOL ManualReset,
	OUT gctSIGNAL * Signal
	);

/* Destroy a signal. */
gceSTATUS
gcoOS_DestroySignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal
	);

/* Signal a signal. */
gceSTATUS
gcoOS_Signal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctBOOL State
	);

/* Wait for a signal. */
gceSTATUS
gcoOS_WaitSignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctUINT32 Wait
	);

/* Map a user signal to the kernel space. */
gceSTATUS
gcoOS_MapSignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctHANDLE Process,
	OUT gctSIGNAL * MappedSignal
	);

/* Map user memory. */
gceSTATUS
gcoOS_MapUserMemory(
	IN gcoOS Os,
	IN gctPOINTER Memory,
	IN gctSIZE_T Size,
	OUT gctPOINTER * Info,
	OUT gctUINT32_PTR Address
	);

/* Unmap user memory. */
gceSTATUS
gcoOS_UnmapUserMemory(
	IN gcoOS Os,
	IN gctPOINTER Memory,
	IN gctSIZE_T Size,
	IN gctPOINTER Info,
	IN gctUINT32 Address
	);

#ifdef ANDROID_VERSION_ECLAIR
gceSTATUS
gcoOS_FlushCache(
    int fd,
    int offset,
    int size 
    );
#endif

#if !USE_NEW_LINUX_SIGNAL
/* Create signal to be used in the user space. */
gceSTATUS
gcoOS_CreateUserSignal(
	IN gcoOS Os,
	IN gctBOOL ManualReset,
	OUT gctINT * SignalID
	);

/* Destroy signal used in the user space. */
gceSTATUS
gcoOS_DestroyUserSignal(
	IN gcoOS Os,
	IN gctINT SignalID
	);

/* Wait for signal used in the user space. */
gceSTATUS
gcoOS_WaitUserSignal(
	IN gcoOS Os,
	IN gctINT SignalID,
	IN gctUINT32 Wait
	);

/* Signal a signal used in the user space. */
gceSTATUS
gcoOS_SignalUserSignal(
	IN gcoOS Os,
	IN gctINT SignalID,
	IN gctBOOL State
	);
#endif /* USE_NEW_LINUX_SIGNAL */

/* Set a signal owned by a process. */
gceSTATUS
gcoOS_UserSignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctHANDLE Process
	);

/******************************************************************************\
********************************* gcoHEAP Object ********************************
\******************************************************************************/

typedef struct _gcoHEAP *		gcoHEAP;

/* Construct a new gcoHEAP object. */
gceSTATUS
gcoHEAP_Construct(
	IN gcoOS Os,
	IN gctSIZE_T AllocationSize,
	IN gctSIZE_T WasteThreshold,
	OUT gcoHEAP * Heap
	);

/* Destroy an gcoHEAP object. */
gceSTATUS
gcoHEAP_Destroy(
	IN gcoHEAP Heap
	);

/* Allocate memory. */
gceSTATUS
gcoHEAP_Allocate(
	IN gcoHEAP Heap,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Node
	);

/* Free memory. */
gceSTATUS
gcoHEAP_Free(
	IN gcoHEAP Heap,
	IN gctPOINTER Node
	);

/* Compact the heap. */
gceSTATUS
gcoHEAP_Compact(
	IN gcoHEAP Heap
	);

/* Profile the heap. */
void
gcoHEAP_ProfileStart(
	IN gcoHEAP Heap
	);

void
gcoHEAP_ProfileEnd(
	IN gcoHEAP Heap,
	IN gctCONST_STRING Title
	);

/******************************************************************************\
******************************** gcoVIDMEM Object *******************************
\******************************************************************************/

typedef struct _gcoVIDMEM *			gcoVIDMEM;
typedef union  _gcuVIDMEM_NODE *	gcuVIDMEM_NODE_PTR;
typedef struct _gcoHARDWARE *		gcoHARDWARE;
typedef struct _gcoKERNEL *			gcoKERNEL;

/* Construct a new gcoVIDMEM object. */
gceSTATUS
gcoVIDMEM_Construct(
	IN gcoOS Os,
	IN gctUINT32 BaseAddress,
	IN gctSIZE_T Bytes,
	IN gctSIZE_T Threshold,
	IN gctSIZE_T Banking,
	OUT gcoVIDMEM * Memory
	);

/* Destroy an AQVDIMEM object. */
gceSTATUS
gcoVIDMEM_Destroy(
	IN gcoVIDMEM Memory
	);

/* Allocate rectangular memory. */
gceSTATUS
gcoVIDMEM_Allocate(
	IN gcoVIDMEM Memory,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctUINT Depth,
	IN gctUINT BytesPerPixel,
	IN gctUINT32 Alignment,
	IN gceSURF_TYPE Type,
	OUT gcuVIDMEM_NODE_PTR * Node
	);

/* Allocate linear memory. */
gceSTATUS
gcoVIDMEM_AllocateLinear(
	IN gcoVIDMEM Memory,
	IN gctSIZE_T Bytes,
	IN gctUINT32 Alignment,
	IN gceSURF_TYPE Type,
	OUT gcuVIDMEM_NODE_PTR * Node
	);

/* Free memory. */
gceSTATUS
gcoVIDMEM_Free(
	IN gcuVIDMEM_NODE_PTR Node
	);

/* Lock memory. */
gceSTATUS
gcoVIDMEM_Lock(
	IN gcuVIDMEM_NODE_PTR Node,
	OUT gctUINT32 * Address
	);

/* Unlock memory. */
gceSTATUS
gcoVIDMEM_Unlock(
	IN gcuVIDMEM_NODE_PTR Node,
	IN gceSURF_TYPE Type,
#if USE_EVENT_QUEUE
	IN OUT gctBOOL * Asynchroneous
#else
	IN OUT gctSIZE_T * CommandSize,
	OUT gctPOINTER	Commands
#endif
	);

/* Construct a gcuVIDMEM_NODE union for virtual memory. */
gceSTATUS
gcoVIDMEM_ConstructVirtual(
	IN gcoKERNEL Kernel,
	IN gctSIZE_T Bytes,
	OUT gcuVIDMEM_NODE_PTR * Node
	);

/* Destroy a gcuVIDMEM_NODE union for virtual memory. */
gceSTATUS
gcoVIDMEM_DestroyVirtual(
	IN gcuVIDMEM_NODE_PTR Node
	);

/******************************************************************************\
******************************** gcoKERNEL Object *******************************
\******************************************************************************/

struct _gcsHAL_INTERFACE;

/* Notifications. */
typedef enum _gceNOTIFY
{
	gcvNOTIFY_INTERRUPT,
	gcvNOTIFY_COMMAND_QUEUE,
}
gceNOTIFY;

/* Event locations. */
typedef enum _gceKERNEL_WHERE
{
	gcvKERNEL_COMMAND,
	gcvKERNEL_VERTEX,
	gcvKERNEL_TRIANGLE,
	gcvKERNEL_TEXTURE,
	gcvKERNEL_PIXEL,
}
gceKERNEL_WHERE;

/* Flush flags. */
typedef enum _gceKERNEL_FLUSH
{
	gcvFLUSH_COLOR				= 0x01,
	gcvFLUSH_DEPTH				= 0x02,
	gcvFLUSH_TEXTURE			= 0x04,
	gcvFLUSH_2D					= 0x08,
}
gceKERNEL_FLUSH;

/* Construct a new gcoKERNEL object. */
gceSTATUS
gcoKERNEL_Construct(
	IN gcoOS Os,
	IN gctPOINTER Context,
	OUT gcoKERNEL * Kernel
	);

/* Destroy an gcoKERNEL object. */
gceSTATUS
gcoKERNEL_Destroy(
	IN gcoKERNEL Kernel
	);

/* Dispatch a user-level command. */
gceSTATUS
gcoKERNEL_Dispatch(
	IN gcoKERNEL Kernel,
	IN gctBOOL FromUser,
	IN OUT struct _gcsHAL_INTERFACE * Interface
	);

/* Query the video memory. */
gceSTATUS
gcoKERNEL_QueryVideoMemory(
	IN gcoKERNEL Kernel,
	OUT struct _gcsHAL_INTERFACE * Interface
	);

/* Lookup the gcoVIDMEM object for a pool. */
gceSTATUS
gcoKERNEL_GetVideoMemoryPool(
	IN gcoKERNEL Kernel,
	IN gcePOOL Pool,
	OUT gcoVIDMEM * VideoMemory
	);

/* Map video memory. */
gceSTATUS
gcoKERNEL_MapVideoMemory(
	IN gcoKERNEL Kernel,
	IN gctBOOL InUserSpace,
	IN gctUINT32 Address,
	OUT gctPOINTER * Logical
	);

/* Map memory. */
gceSTATUS
gcoKERNEL_MapMemory(
	IN gcoKERNEL Kernel,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	);

/* Unmap memory. */
gceSTATUS
gcoKERNEL_UnmapMemory(
	IN gcoKERNEL Kernel,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	);

/* Notification of events. */
gceSTATUS
gcoKERNEL_Notify(
	IN gcoKERNEL Kernel,
	IN gceNOTIFY Notifcation,
	IN gctBOOL Data
	);

gceSTATUS
gcoKERNEL_QuerySettings(
	IN gcoKERNEL Kernel,
	OUT gcsKERNEL_SETTINGS * Settings
	);

/******************************************************************************\
******************************* gcoHARDWARE Object ******************************
\******************************************************************************/

/* Construct a new gcoHARDWARE object. */
gceSTATUS
gcoHARDWARE_Construct(
	IN gcoOS Os,
	OUT gcoHARDWARE * Hardware
	);

/* Destroy an gcoHARDWARE object. */
gceSTATUS
gcoHARDWARE_Destroy(
	IN gcoHARDWARE Hardware
	);

/* Query system memory requirements. */
gceSTATUS
gcoHARDWARE_QuerySystemMemory(
	IN gcoHARDWARE Hardware,
	OUT gctSIZE_T * SystemSize,
	OUT gctUINT32 * SystemBaseAddress
	);

/* Build virtual address. */
gceSTATUS
gcoHARDWARE_BuildVirtualAddress(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Index,
	IN gctUINT32 Offset,
	OUT gctUINT32 * Address
	);

/* Query command buffer requirements. */
gceSTATUS
gcoHARDWARE_QueryCommandBuffer(
	IN gcoHARDWARE Hardware,
	OUT gctSIZE_T * Alignment,
	OUT gctSIZE_T * ReservedHead,
	OUT gctSIZE_T * ReservedTail
	);

/* Add a WAIT/LINK pair in the command queue. */
gceSTATUS
gcoHARDWARE_WaitLink(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctUINT32 Offset,
	IN OUT gctSIZE_T * Bytes,
	OUT gctPOINTER * Wait,
	OUT gctSIZE_T * WaitBytes
	);

/* Kickstart the command processor. */
gceSTATUS
gcoHARDWARE_Execute(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctSIZE_T Bytes
	);

/* Add an END command in the command queue. */
gceSTATUS
gcoHARDWARE_End(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN OUT gctSIZE_T * Bytes
	);

/* Add a NOP command in the command queue. */
gceSTATUS
gcoHARDWARE_Nop(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN OUT gctSIZE_T * Bytes
	);

/* Add a WAIT command in the command queue. */
gceSTATUS
gcoHARDWARE_Wait(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctUINT32 Count,
	IN OUT gctSIZE_T * Bytes
	);

/* Add a PIPESELECT command in the command queue. */
gceSTATUS
gcoHARDWARE_PipeSelect(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctUINT32 Pipe,
	IN OUT gctSIZE_T * Bytes
	);

/* Add a LINK command in the command queue. */
gceSTATUS
gcoHARDWARE_Link(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctPOINTER FetchAddress,
	IN gctSIZE_T FetchSize,
	IN OUT gctSIZE_T * Bytes
	);

/* Add an EVENT command in the command queue. */
gceSTATUS
gcoHARDWARE_Event(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctUINT8 Event,
	IN gceKERNEL_WHERE FromWhere,
	IN OUT gctSIZE_T * Bytes
	);

/* Query the available memory. */
gceSTATUS
gcoHARDWARE_QueryMemory(
	IN gcoHARDWARE Hardware,
	OUT gctSIZE_T * InternalSize,
	OUT gctUINT32 * InternalBaseAddress,
	OUT gctUINT32 * InternalAlignment,
	OUT gctSIZE_T * ExternalSize,
	OUT gctUINT32 * ExternalBaseAddress,
	OUT gctUINT32 * ExternalAlignment,
	OUT gctUINT32 * HorizontalTileSize,
	OUT gctUINT32 * VerticalTileSize
	);

/* Query the identity of the hardware. */
gceSTATUS
gcoHARDWARE_QueryChipIdentity(
    IN gcoHARDWARE Hardware,
	OUT gceCHIPMODEL* ChipModel,
	OUT gctUINT32* ChipRevision,
	OUT gctUINT32* ChipFeatures,
	OUT gctUINT32* ChipMinorFeatures
	);

/* Query the specifications sof the hardware. */
gceSTATUS
gcoHARDWARE_QueryChipSpecs(
	IN gcoHARDWARE Hardware,
	OUT gctUINT32_PTR StreamCount,
	OUT gctUINT32_PTR RegisterMax,
	OUT gctUINT32_PTR ThreadCount,
	OUT gctUINT32_PTR ShaderCoreCount,
	OUT gctUINT32_PTR VertexCacheSize,
	OUT gctUINT32_PTR VertexOutputBufferSize
	);

/* Convert an API format. */
gceSTATUS
gcoHARDWARE_ConvertFormat(
	IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT Format,
	OUT gctUINT32 * BitsPerPixel,
	OUT gctUINT32 * BytesPerTile
	);

/* Split a harwdare specific address into API stuff. */
gceSTATUS
gcoHARDWARE_SplitMemory(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	OUT gcePOOL * Pool,
	OUT gctUINT32 * Offset
	);

/* Align size to tile boundary. */
gceSTATUS
gcoHARDWARE_AlignToTile(
	IN gcoHARDWARE Hardware,
	IN gceSURF_TYPE Type,
	IN OUT gctUINT32_PTR Width,
	IN OUT gctUINT32_PTR Height,
	OUT gctBOOL_PTR SuperTiled
	);

/* Update command queue tail pointer. */
gceSTATUS
gcoHARWDARE_UpdateQueueTail(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	IN gctUINT32 Offset
	);

/* Convert logical address to hardware specific address. */
gceSTATUS
gcoHARDWARE_ConvertLogical(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical,
	OUT gctUINT32 * Address
	);

/* Interrupt manager. */
gceSTATUS
gcoHARDWARE_Interrupt(
	IN gcoHARDWARE Hardware,
	IN gctBOOL InterruptValid
	);

/* Program MMU. */
gceSTATUS
gcoHARDWARE_SetMMU(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical
	);

/* Flush the MMU. */
gceSTATUS
gcoHARDWARE_FlushMMU(
	IN gcoHARDWARE Hardware
	);

/* Get idle register. */
gceSTATUS
gcoHARDWARE_GetIdle(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Wait,
	OUT gctUINT32 * Data
	);

/* Flush the caches. */
gceSTATUS
gcoHARDWARE_Flush(
	IN gcoHARDWARE Hardware,
	IN gceKERNEL_FLUSH Flush,
    IN gctPOINTER Logical,
    IN OUT gctSIZE_T * Bytes
	);

/* Enable/disable fast clear. */
gceSTATUS
gcoHARDWARE_SetFastClear(
    IN gcoHARDWARE Hardware,
    IN gctINT Enable,
	IN gctINT Compression
    );

gceSTATUS
gcoHARDWARE_ReadInterrupt(
	IN gcoHARDWARE Hardware,
	OUT gctUINT32_PTR IDs
	);

/* Power management. */
gceSTATUS
gcoHARDWARE_SetPowerManagementState(
    IN gcoHARDWARE Hardware,
    IN gceCHIPPOWERSTATE State
    );

gceSTATUS
gcoHARDWARE_QueryPowerManagementState(
    IN gcoHARDWARE Hardware,
    OUT gceCHIPPOWERSTATE* State
    );

/* Profile 2D Engine. */
gceSTATUS
gcoHARDWARE_ProfileEngine2D(
	IN gcoHARDWARE Hardware,
	OUT gco2D_PROFILE_PTR Profile
	);

gceSTATUS
gcoHARDWARE_InitializeHardware(
    IN gcoHARDWARE Hardware
	);

gceSTATUS
gcoHARDWARE_Reset(
	IN gcoHARDWARE Hardware
	);

/******************************************************************************\
***************************** gcoINTERRUPT Object ******************************
\******************************************************************************/

typedef struct _gcoINTERRUPT *	gcoINTERRUPT;

typedef gceSTATUS (* gctINTERRUPT_HANDLER)(
	IN gcoKERNEL Kernel
	);

gceSTATUS
gcoINTERRUPT_Construct(
	IN gcoKERNEL Kernel,
	OUT gcoINTERRUPT * Interrupt
	);

gceSTATUS
gcoINTERRUPT_Destroy(
	IN gcoINTERRUPT Interrupt
	);

gceSTATUS
gcoINTERRUPT_SetHandler(
	IN gcoINTERRUPT Interrupt,
	IN OUT gctINT32_PTR Id,
	IN gctINTERRUPT_HANDLER Handler
	);

gceSTATUS
gcoINTERRUPT_Notify(
	IN gcoINTERRUPT Interrupt,
	IN gctBOOL Valid
	);

/******************************************************************************\
******************************** gcoEVENT Object ********************************
\******************************************************************************/

typedef struct _gcoEVENT *		gcoEVENT;

#if !USE_EVENT_QUEUE
/* Event types. */
typedef enum _gceEVENT_TYPE
{
	gcvEVENT_SIGNAL,
	gcvEVENT_UNLOCK,
	gcvEVENT_UNMAP_MEMORY,
	gcvEVENT_UNMAP_USER_MEMORY,
	gcvEVENT_FREE_NON_PAGED_MEMORY,
	gcvEVENT_FREE_CONTIGUOUS_MEMORY,
	gcvEVENT_FREE_VIDEO_MEMORY,
	gcvEVENT_WRITE_DATA,
	gcvEVENT_SET_IDLE,
}
gceEVENT_TYPE;

/* Event data. */
typedef union _gcuEVENT_DATA
{
	/* gcvEVENT_FREE_NON_PAGED_MEMORY */
	struct _gcsEVENT_FREE_NON_PAGED_MEMORY
	{
		/* Number of allocated bytes. */
		IN gctSIZE_T			bytes;

		/* Physical address of allocation. */
		IN gctPHYS_ADDR			physical;

		/* Logical address of allocation. */
		IN gctPOINTER			logical;
	}
	FreeNonPagedMemory;

	/* gcvEVENT_FREE_VIDEO_MEMORY */
	struct _gcsEVENT_FREE_VIDEO_MEMORY
	{
		/* Allocated memory. */
		IN gcuVIDMEM_NODE_PTR	node;
	}
	FreeVideoMemory;

	/* gcvEVENT_WRITE_DATA */
	struct _gcsEVENT_WRITE_DATA
	{
		/* Logical address of memory to write data to. */
		IN gctPOINTER			address;

		/* Logical address in kernel to write data to. */
		IN gctPOINTER			kernelAddress;

		/* Data to write. */
		IN gctUINT32			data;
	}
	WriteData;

	/* gcvEVENT_UNLOCK */
	struct _gcsEVENT_UNLOCK
	{
		/* Video memory node to unlock. */
		IN gcuVIDMEM_NODE_PTR	node;

		/* Type of surface to unlock. */
		IN gceSURF_TYPE			type;
	}
	Unlock;

	/* gcvEVENT_SIGNAL */
	struct _gcsEVENT_SIGNAL
	{
		/* Signal handle to signal. */
		IN gctSIGNAL			signal;

		/* Aux signal handle to signal. */
		IN gctSIGNAL			auxSignal;

		/* Process owning the signal. */
		IN gctHANDLE			process;

        /* Event generated from where of pipeline */
        IN gceKERNEL_WHERE      fromWhere;
	}
	Signal;

	/* gcvEVENT_FREE_CONTIGUOUS_MEMORY */
	struct _gcsEVENT_FREE_CONTIGUOUS_MEMORY
	{
		/* Number of allocated bytes. */
		IN gctSIZE_T			bytes;

		/* Physical address of allocation. */
		IN gctPHYS_ADDR			physical;

		/* Logical address of allocation. */
		IN gctPOINTER			logical;
	}
	FreeContiguousMemory;

	/* gcvEVENT_UNMAP_USER_MEMORY */
	struct _gcsEVENT_UNMAP_USER_MEMORY
	{
		/* Private data for memory mapping. */
		IN gctPOINTER			info;

		/* Memory size. */
		IN gctSIZE_T			size;

		/* Physical address. */
		IN gctUINT32			address;

		/* Virtual address. */
		IN gctPOINTER			memory;
	}
	UnmapUserMemory;

	/* gcvEVENT_UNMAP_MEMORY */
	struct _gcsEVENT_UNMAP_MEMORY
	{
		/* Memory size. */
		IN gctSIZE_T			bytes;

		/* Physical address. */
		IN gctPHYS_ADDR			physical;

		/* Virtual address. */
		IN gctPOINTER			logical;
	}
	UnmapMemory;
}
gcuEVENT_DATA;
#endif

/* Construct a new gcoEVENT object. */
gceSTATUS
gcoEVENT_Construct(
	IN gcoKERNEL Kernel,
	OUT gcoEVENT * Event
	);

/* Destroy an gcoEVENT object. */
gceSTATUS
gcoEVENT_Destroy(
	IN gcoEVENT Event
	);

#if !USE_EVENT_QUEUE
/* Schedule an event. */
gceSTATUS
gcoEVENT_Schedule(
	IN gcoEVENT Event,
	IN gceEVENT_TYPE Type,
	IN gcuEVENT_DATA * Data,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize
	);
#endif

/* Schedule a FreeNonPagedMemory event. */
gceSTATUS
gcoEVENT_FreeNonPagedMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gctSIZE_T Bytes,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	);

/* Schedule a FreeContiguousMemory event. */
gceSTATUS
gcoEVENT_FreeContiguousMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gctSIZE_T Bytes,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	);

/* Schedule a FreeVideoMemory event. */
gceSTATUS
gcoEVENT_FreeVideoMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gcuVIDMEM_NODE_PTR VideoMemory,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	);

/* Schedule a WriteData event. */
gceSTATUS
gcoEVENT_WriteData(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gctPOINTER Logical,
	IN gctUINT32 Data,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	);

/* Schedule an Unlock event. */
gceSTATUS
gcoEVENT_Unlock(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gcuVIDMEM_NODE_PTR Node,
	IN gceSURF_TYPE Type,
	IN gctBOOL Wait
	);

#if USE_EVENT_QUEUE
struct _gcsQUEUE;

/* Commit an event queue. */
gceSTATUS
gcoEVENT_Commit(
	IN gcoEVENT Event,
	IN struct _gcsQUEUE * Queue
	);

/* Append an event to the event queue. */
gceSTATUS
gcoEVENT_Append(
	IN gcoEVENT Event,
	IN gctUINT8 Id,
	IN gcsHAL_INTERFACE_PTR Interface
	);

/* Remove all commands from a queue. */
gceSTATUS
gcoEVENT_Clear(
	IN gcoEVENT Event,
	IN gctUINT8 Id
	);
#else
/* Schedule a Signal event. */
gceSTATUS
gcoEVENT_Signal(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gctSIGNAL Signal,
	IN gctSIGNAL AuxSignal,
	IN gctHANDLE Process,
	IN gctBOOL Wait
	);

/* Schedule an event to unmap a buffer mapped through its physical address. */
gceSTATUS
gcoEVENT_UnmapMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gctSIZE_T Size,
	IN gctPHYS_ADDR Address,
	IN gctPOINTER Memory,
	IN gctBOOL Wait
	);

/* Schedule a user memory unmapping event. */
gceSTATUS
gcoEVENT_UnmapUserMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gctPOINTER Memory,
	IN gctSIZE_T Size,
	IN gctPOINTER Info,
	IN gctUINT32 Address,
	IN gctBOOL Wait
	);
#endif

/* Event callback routine. */
gceSTATUS
gcoEVENT_Notify(
	IN gcoEVENT Event,
	IN gctUINT32 IDs
	);

#if USE_EVENT_QUEUE
/* Event callback routine. */
gceSTATUS
gcoEVENT_Interrupt(
	IN gcoEVENT Event,
	IN gctUINT32 IDs
	);
#endif

/******************************************************************************\
******************************* gcoCOMMAND Object *******************************
\******************************************************************************/

typedef struct _gcoCOMMAND *		gcoCOMMAND;

/* Construct a new gcoCOMMAND object. */
gceSTATUS
gcoCOMMAND_Construct(
	IN gcoKERNEL Kernel,
	OUT gcoCOMMAND * Command
	);

/* Destroy an gcoCOMMAND object. */
gceSTATUS
gcoCOMMAND_Destroy(
	IN gcoCOMMAND Command
	);

/* Start the command queue. */
gceSTATUS
gcoCOMMAND_Start(
	IN gcoCOMMAND Command
	);

/* Stop the command queue. */
gceSTATUS
gcoCOMMAND_Stop(
	IN gcoCOMMAND Command
	);

/* Commit a buffer to the command queue. */
gceSTATUS
gcoCOMMAND_Commit(
	IN gcoCOMMAND Command,
	IN gcoCMDBUF CommandBuffer,
	IN gcoCONTEXT Context
	);

/* Reserve space in the command buffer. */
gceSTATUS
gcoCOMMAND_Reserve(
	IN gcoCOMMAND Command,
	IN gctSIZE_T RequestedBytes,
	OUT gctPOINTER * Buffer,
	OUT gctSIZE_T * BufferSize
	);

/* Release reserved space in the command buffer. */
gceSTATUS
gcoCOMMAND_Release(
	IN gcoCOMMAND Command
	);

/* Execute reserved space in the command buffer. */
gceSTATUS
gcoCOMMAND_Execute(
	IN gcoCOMMAND Command,
	IN gctSIZE_T RequstedBytes
	);

/* Stall the command queue. */
gceSTATUS
gcoCOMMAND_Stall(
	IN gcoCOMMAND Command
	);

/******************************************************************************\
********************************* gcoMMU Object *********************************
\******************************************************************************/

typedef struct _gcoMMU *			gcoMMU;

/* Construct a new gcoMMU object. */
gceSTATUS
gcoMMU_Construct(
	IN gcoKERNEL Kernel,
	IN gctSIZE_T MmuSize,
	OUT gcoMMU * Mmu
	);

/* Destroy an gcoMMU object. */
gceSTATUS
gcoMMU_Destroy(
	IN gcoMMU Mmu
	);

/* Allocate pages inside the MMU. */
gceSTATUS
gcoMMU_AllocatePages(
	IN gcoMMU Mmu,
	IN gctSIZE_T PageCount,
	OUT gctPOINTER * PageTable,
	OUT gctUINT32 * Address
	);

/* Remove a page table from	the MMU. */
gceSTATUS
gcoMMU_FreePages(
	IN gcoMMU Mmu,
	IN gctPOINTER PageTable,
	IN gctSIZE_T PageCount
	);

/******************************************************************************\
********************************* gcsPROFILER structure ***********************
\******************************************************************************/

typedef struct _gcProfile gcsPROFILER;

gceSTATUS
gcoHARDWARE_QueryProfileRegisters(
	IN gcoHARDWARE Hardware,
    OUT gctINT32_PTR HWProfile
	);

/* Memory profile information. */
struct _gcMemProfile
{
    /* Memory Usage */
    gctUINT32       videoMemUsed;
    gctUINT32       systemMemUsed;
    gctUINT32       commitBufferSize;
    gctUINT32       contextBufferCopyBytes;

};

/* Shader profile information. */
struct _gcSHADER_PROFILER
{
    gctUINT32       shaderLength;
    gctUINT32       shaderALUCycles;
    gctUINT32       shaderTexLoadCycles;
    gctUINT32       shaderTempRegCount;
    gctUINT32       shaderSamplerRegCount;
    gctUINT32       shaderInputRegCount;
    gctUINT32       shaderOutputRegCount;
};

/* Initialize the gcProfiler. */
gceSTATUS
gcsPROFILER_Initialize(
    IN gcoHAL Hal,
    void* Fptr
    );

/* Destroy the gcProfiler. */
gceSTATUS
gcsPROFILER_Destroy(
    IN gcoHAL Hal
    );

/* Call to signal end of frame. */
gceSTATUS
gcsPROFILER_EndFrame(
    IN gcoHAL Hal
    );

/* Profile input vertex shader. */
void gcsPROFILER_ShaderVS(
    IN gcoHAL Hal,
    IN void* Vs
    );

/* Profile input fragment shader. */
void gcsPROFILER_ShaderFS(
    IN gcoHAL Hal,
    IN void* Fs
    );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __aqhal_h_ */
