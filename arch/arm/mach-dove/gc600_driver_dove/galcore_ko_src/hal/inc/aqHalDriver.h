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






#ifndef __aqhaldriver_h_
#define __aqhaldriver_h_

#include "aqTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
******************************* I/O Control Codes ******************************
\******************************************************************************/

#define gcvHAL_CLASS					"galcore"
#define IOCTL_GCHAL_INTERFACE			30000
#define IOCTL_GCHAL_KERNEL_INTERFACE	30001

/******************************************************************************\
********************************* Command Codes ********************************
\******************************************************************************/

typedef enum _gceHAL_COMMAND_CODES
{
	/* Generic query. */
	gcvHAL_QUERY_VIDEO_MEMORY,
	gcvHAL_QUERY_CHIP_IDENTITY,

	/* Contigous memory. */
	gcvHAL_ALLOCATE_NON_PAGED_MEMORY,
	gcvHAL_FREE_NON_PAGED_MEMORY,
	gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY,
	gcvHAL_FREE_CONTIGUOUS_MEMORY,

	/* Video memory allocation. */
	gcvHAL_ALLOCATE_VIDEO_MEMORY,			/* Enforced alignment. */
	gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY,	/* No alignment. */
	gcvHAL_FREE_VIDEO_MEMORY,

	/* Physical-to-logical mapping. */
	gcvHAL_MAP_MEMORY,
	gcvHAL_UNMAP_MEMORY,

	/* Logical-to-physical mapping. */
	gcvHAL_MAP_USER_MEMORY,
	gcvHAL_UNMAP_USER_MEMORY,

	/* Surface lock/unlock. */
	gcvHAL_LOCK_VIDEO_MEMORY,
	gcvHAL_UNLOCK_VIDEO_MEMORY,

	/* Event queue. */
#if USE_EVENT_QUEUE
	gcvHAL_EVENT_COMMIT,
#else
	gcvHAL_EVENT,
#endif

	gcvHAL_USER_SIGNAL,
	gcvHAL_SIGNAL,
	gcvHAL_WRITE_DATA,

	gcvHAL_COMMIT,
	gcvHAL_STALL,

    gcvHAL_READ_REGISTER,
    gcvHAL_WRITE_REGISTER,

	gcvHAL_GET_PROFILE_SETTING,
	gcvHAL_SET_PROFILE_SETTING,

    gcvHAL_READ_ALL_PROFILE_REGISTERS,
	gcvHAL_PROFILE_REGISTERS_2D,

    /* Power management. */
    gcvHAL_SET_POWER_MANAGEMENT_STATE,
    gcvHAL_QUERY_POWER_MANAGEMENT_STATE,

	gcvHAL_GET_BASE_ADDRESS,

	gcvHAL_SET_IDLE,

	/* Queries. */
	gcvHAL_QUERY_KERNEL_SETTINGS,

	/* Reset. */
	gcvHAL_RESET,
}
gceHAL_COMMAND_CODES;

/******************************************************************************\
****************************** Interface Structure *****************************
\******************************************************************************/

#define gcmMAX_PROFILE_FILE_NAME	128

typedef struct _gcsHAL_INTERFACE
{
	/* Command code. */
	gceHAL_COMMAND_CODES		command;

	/* Status value. */
	gceSTATUS					status;

	/* Union of command structures. */
	union _u
	{
		/* gcvHAL_GET_BASE_ADDRESS */
		struct _gcsHAL_GET_BASE_ADDRESS
		{
			/* Physical memory address of internal memory. */
			OUT gctUINT32				baseAddress;
		}
		GetBaseAddress;

		/* gcvHAL_QUERY_VIDEO_MEMORY */
		struct _gcsHAL_QUERY_VIDEO_MEMORY
		{
			/* Physical memory address of internal memory. */
			OUT gctPHYS_ADDR			internalPhysical;

			/* Size in bytes of internal memory.*/
			OUT gctSIZE_T				internalSize;

			/* Physical memory address of external memory. */
			OUT gctPHYS_ADDR			externalPhysical;

			/* Size in bytes of external memory.*/
			OUT gctSIZE_T				externalSize;

			/* Physical memory address of contiguous memory. */
			OUT gctPHYS_ADDR			contiguousPhysical;

			/* Size in bytes of contiguous memory.*/
			OUT gctSIZE_T				contiguousSize;
		}
		QueryVideoMemory;

		/* gcvHAL_QUERY_CHIP_IDENTITY */
		struct _gcsHAL_QUERY_CHIP_IDENTITY
		{

			/* Chip model. */
			OUT gceCHIPMODEL			chipModel;

			/* Revision value.*/
			OUT gctUINT32				chipRevision;

			/* Supported feature fields. */
			OUT gctUINT32				chipFeatures;

			/* Supported minor feature fields. */
			OUT gctUINT32				chipMinorFeatures;

			/* Number of streams supported. */
			OUT gctUINT32				streamCount;

			/* Total number of temporary registers per thread. */
			OUT gctUINT32				registerMax;

			/* Maximum number of threads. */
			OUT gctUINT32				threadCount;

			/* Number of shader cores. */
			OUT gctUINT32				shaderCoreCount;

			/* Size of the vertex cache. */
			OUT gctUINT32				vertexCacheSize;

			/* Number of entries in the vertex output buffer. */
			OUT gctUINT32				vertexOutputBufferSize;
		}
		QueryChipIdentity;

		/* gcvHAL_MAP_MEMORY */
		struct _gcsHAL_MAP_MEMORY
		{
			/* Physical memory address to map. */
			IN gctPHYS_ADDR				physical;

			/* Number of bytes in physical memory to map. */
			IN gctSIZE_T				bytes;

			/* Address of mapped memory. */
			OUT gctPOINTER				logical;
		}
		MapMemory;

		/* gcvHAL_UNMAP_MEMORY */
		struct _gcsHAL_UNMAP_MEMORY
		{
			/* Physical memory address to unmap. */
			IN gctPHYS_ADDR				physical;

			/* Number of bytes in physical memory to unmap. */
			IN gctSIZE_T				bytes;

			/* Address of mapped memory to unmap. */
			IN gctPOINTER				logical;
		}
		UnmapMemory;

		/* gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY */
		struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY
		{
			/* Number of bytes to allocate. */
			IN OUT gctUINT				bytes;

			/* Buffer alignment. */
			IN gctUINT					alignment;

			/* Type of allocation. */
			IN gceSURF_TYPE				type;

			/* Memory pool to allocate from. */
			IN OUT gcePOOL				pool;

			/* Allocated video memory. */
			OUT gcuVIDMEM_NODE_PTR		node;
		}
		AllocateLinearVideoMemory;

		/* gcvHAL_ALLOCATE_VIDEO_MEMORY */
		struct _gcsHAL_ALLOCATE_VIDEO_MEMORY
		{
			/* Width of rectangle to allocate. */
			IN OUT gctUINT				width;

			/* Height of rectangle to allocate. */
			IN OUT gctUINT				height;

			/* Depth of rectangle to allocate. */
			IN gctUINT					depth;

			/* Format rectangle to allocate in gceSURF_FORMAT. */
			IN gceSURF_FORMAT			format;

			/* Type of allocation. */
			IN gceSURF_TYPE				type;

			/* Memory pool to allocate from. */
			IN OUT gcePOOL				pool;

			/* Allocated video memory. */
			OUT gcuVIDMEM_NODE_PTR		node;
		}
		AllocateVideoMemory;

		/* gcvHAL_FREE_VIDEO_MEMORY */
		struct _gcsHAL_FREE_VIDEO_MEMORY
		{
			/* Allocated video memory. */
			IN gcuVIDMEM_NODE_PTR		node;
		}
		FreeVideoMemory;

		/* gcvHAL_LOCK_VIDEO_MEMORY */
		struct _gcsHAL_LOCK_VIDEO_MEMORY
		{
			/* Allocated video memory. */
			IN gcuVIDMEM_NODE_PTR		node;

			/* Hardware specific address. */
			OUT gctUINT32				address;

			/* Mapped logical address. */
			OUT gctPOINTER				memory;
		}
		LockVideoMemory;

		/* gcvHAL_UNLOCK_VIDEO_MEMORY */
		struct _gcsHAL_UNLOCK_VIDEO_MEMORY
		{
			/* Allocated video memory. */
			IN gcuVIDMEM_NODE_PTR		node;

			/* Type of surface. */
			IN gceSURF_TYPE				type;

#if USE_EVENT_QUEUE
			/* Flag to unlock surface asynchroneously. */
			IN OUT gctBOOL				asynchroneous;
#else
			/* Size of local command buffer. */
			IN OUT gctSIZE_T			commandSize;

			/* Local command buffer. */
			OUT gctUINT8				commands[32];
#endif
		}
		UnlockVideoMemory;

		/* gcvHAL_ALLOCATE_NON_PAGED_MEMORY */
		struct _gcsHAL_ALLOCATE_NON_PAGED_MEMORY
		{
			/* Number of bytes to allocate. */
			IN OUT gctSIZE_T			bytes;

			/* Physical address of allocation. */
			OUT gctPHYS_ADDR			physical;

			/* Logical address of allocation. */
			OUT gctPOINTER				logical;
		}
		AllocateNonPagedMemory;

		/* gcvHAL_FREE_NON_PAGED_MEMORY */
		struct _gcsHAL_FREE_NON_PAGED_MEMORY
		{
			/* Number of bytes allocated. */
			IN gctSIZE_T				bytes;

			/* Physical address of allocation. */
			IN gctPHYS_ADDR				physical;

			/* Logical address of allocation. */
			IN gctPOINTER				logical;
		}
		FreeNonPagedMemory;

#if USE_EVENT_QUEUE
		/* gcvHAL_EVENT_COMMIT. */
		struct _gcsHAL_EVENT_COMMIT
		{
			/* Event queue. */
			IN struct _gcsQUEUE *		queue;
		}
		Event;
#else
		/* gcvHAL_EVENT */
		struct _gcsHAL_EVENT
		{
			/* Event type. */
			IN gceEVENT_TYPE			type;

			/* Event data. */
			IN gcuEVENT_DATA			data;

			/* Size of local command buffer. */
			IN OUT gctSIZE_T			commandSize;

			/* Local command buffer. */
			OUT gctUINT8				commands[32];
		}
		Event;
#endif

		/* gcvHAL_COMMIT */
		struct _gcsHAL_COMMIT
		{
			/* Command buffer. */
			IN gcoCMDBUF				commandBuffer;

			/* Context buffer. */
			IN gcoCONTEXT				contextBuffer;
		}
		Commit;

		/* gcvHAL_MAP_USER_MEMORY */
		struct _gcsHAL_MAP_USER_MEMORY
		{
			/* Base address of user memory to map. */
			IN gctPOINTER				memory;

			/* Size of user memory in bytes to map. */
			IN gctSIZE_T				size;

			/* Info record required by gcvHAL_UNMAP_USER_MEMORY. */
			OUT gctPOINTER				info;

			/* Physical address of mapped memory. */
			OUT gctUINT32				address;
		}
		MapUserMemory;

		/* gcvHAL_UNMAP_USER_MEMORY */
		struct _gcsHAL_UNMAP_USER_MEMORY
		{
			/* Base address of user memory to unmap. */
			IN gctPOINTER				memory;

			/* Size of user memory in bytes to unmap. */
			IN gctSIZE_T				size;

			/* Info record returned by gcvHAL_MAP_USER_MEMORY. */
			IN gctPOINTER				info;

			/* Physical address of mapped memory as returned by
			   gcvHAL_MAP_USER_MEMORY. */
			IN gctUINT32				address;
		}
		UnmapUserMemory;

#if !USE_NEW_LINUX_SIGNAL
		/* gcsHAL_USER_SIGNAL  */
		struct _gcsHAL_USER_SIGNAL
		{
			/* Command. */
			gceUSER_SIGNAL_COMMAND_CODES command;

			/* Signal ID. */
			IN OUT gctINT				id;

			/* Reset mode. */
			IN gctBOOL					manualReset;

			/* Wait timedout. */
			IN gctUINT32				wait;

			/* State. */
			IN gctBOOL					state;
		}
		UserSignal;
#endif

		/* gcvHAL_SIGNAL. */
		struct _gcsHAL_SIGNAL
		{
			/* Signal handle to signal. */
			IN gctSIGNAL				signal;

			/* Reserved. */
			IN gctSIGNAL				auxSignal;

			/* Process owning the signal. */
			IN gctHANDLE				process;

            /* Event generated from where of pipeline */
            IN gceKERNEL_WHERE          fromWhere;
		}
		Signal;

		/* gcvHAL_WRITE_DATA. */
		struct _gcsHAL_WRITE_DATA
		{
			/* Address to write data to. */
			IN gctUINT32				address;

			/* Logical address in kernel to write data to. */
			IN  gctPOINTER			kernelAddress;

			/* Data to write. */
			IN gctUINT32				data;
		}
		WriteData;

		/* gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY */
		struct _gcsHAL_ALLOCATE_CONTIGUOUS_MEMORY
		{
			/* Number of bytes to allocate. */
			IN OUT gctSIZE_T			bytes;

			/* Physical address of allocation. */
			OUT gctPHYS_ADDR			physical;

			/* Logical address of allocation. */
			OUT gctPOINTER				logical;
		}
		AllocateContiguousMemory;

		/* gcvHAL_FREE_CONTIGUOUS_MEMORY */
		struct _gcsHAL_FREE_CONTIGUOUS_MEMORY
		{
			/* Number of bytes allocated. */
			IN gctSIZE_T				bytes;

			/* Physical address of allocation. */
			IN gctPHYS_ADDR				physical;

			/* Logical address of allocation. */
			IN gctPOINTER				logical;
		}
		FreeContiguousMemory;

        /* gcvHAL_READ_REGISTER */
	    struct _gcsHAL_READ_REGISTER
	    {
		    /* Logical address of memory to write data to. */
		    IN gctUINT32			address;

		    /* Data read. */
		    OUT gctUINT32			data;
	    }
	    ReadRegisterData;

        /* gcvHAL_WRITE_REGISTER */
	    struct _gcsHAL_WRITE_REGISTER
	    {
		    /* Logical address of memory to write data to. */
		    IN gctUINT32			address;

		    /* Data read. */
		    IN gctUINT32			data;
	    }
	    WriteRegisterData;

		/* gcvHAL_GET_PROFILE_SETTING */
		struct _gcsHAL_GET_PROFILE_SETTING
		{
			/* Enable profiling */
			OUT gctBOOL				enable;

			/* The profile file name */
			OUT gctCHAR				fileName[gcmMAX_PROFILE_FILE_NAME];
		}
		GetProfileSetting;

		/* gcvHAL_SET_PROFILE_SETTING */
		struct _gcsHAL_SET_PROFILE_SETTING
		{
			/* Enable profiling */
			IN gctBOOL				enable;

			/* The profile file name */
			IN gctCHAR				fileName[gcmMAX_PROFILE_FILE_NAME];
		}
		SetProfileSetting;

        /* gcvHAL_READ_ALL_PROFILE_REGISTERS */
        struct _gcsHAL_READ_ALL_PROFILE_REGISTERS
        {
		    /* Data read. */
			OUT struct _gcoHWProfile		hwProfile;
        }
        RegisterProfileData;

        /* gcvHAL_PROFILE_REGISTERS_2D */
        struct _gcsHAL_PROFILE_REGISTERS_2D
        {
		    /* Data read. */
		    OUT gco2D_PROFILE_PTR hwProfile2D;
        }
        RegisterProfileData2D;

        /* Power management. */
        /* gcvHAL_SET_POWER_MANAGEMENT_STATE */
        struct _gcsHAL_SET_POWER_MANAGEMENT
        {
		    /* Data read. */
		    IN gceCHIPPOWERSTATE state;
        }
        SetPowerManagement;

        /* gcvHAL_QUERY_POWER_MANAGEMENT_STATE */
        struct _gcsHAL_QUERY_POWER_MANAGEMENT
        {
		    /* Data read. */
		    OUT gceCHIPPOWERSTATE state;
        }
        QueryPowerManagement;

		/* gcvHAL_QUERY_KERNEL_SETTINGS */
		struct _gcsHAL_QUERY_KERNEL_SETTINGS
		{
			/* Settings.*/
			OUT gcsKERNEL_SETTINGS settings;
		}
		QueryKernelSettings;
    }
	u;
}
gcsHAL_INTERFACE;

#ifdef __cplusplus
}
#endif

#endif /* __aqhaldriver_h_ */
