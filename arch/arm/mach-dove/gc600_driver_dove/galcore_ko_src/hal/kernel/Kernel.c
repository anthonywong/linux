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






#include "PreComp.h"

/******************************************************************************\
******************************* gcoKERNEL API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**	gcoKERNEL_Construct
**
**	Construct a new gcoKERNEL object.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		IN gctPOINTER Context
**			Pointer to a driver defined context.
**
**	OUTPUT:
**
**		gcoKERNEL * Kernel
**			Pointer to a variable that will hold the pointer to the gcoKERNEL
**			object.
*/
gceSTATUS gcoKERNEL_Construct(
	IN gcoOS Os,
	IN gctPOINTER Context,
	OUT gcoKERNEL * Kernel
	)
{
	gcoKERNEL kernel;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Kernel != gcvNULL);

	/* Allocate the gcoKERNEL object. */
	gcmERR_RETURN(
		gcoOS_Allocate(Os, sizeof(struct _gcoKERNEL), (gctPOINTER *) &kernel));

	/* Zero the object pointers. */
	kernel->hardware = gcvNULL;
	kernel->command  = gcvNULL;
	kernel->event    = gcvNULL;
	kernel->mmu      = gcvNULL;

	/* Initialize the gcoKERNEL object. */
	kernel->object.type = gcvOBJ_KERNEL;
	kernel->os          = Os;

	/* Save context. */
	kernel->context = Context;

	/* Construct the gcoHARDWARE object. */
	gcmONERROR(
		gcoHARDWARE_Construct(Os, &kernel->hardware));

	/* Set pointer to gcoKERNEL object in gcoHARDWARE object. */
	kernel->hardware->kernel = kernel;

	/* Initialize the hardware. */
	gcmONERROR(
		gcoHARDWARE_InitializeHardware(kernel->hardware));

	/* Construct the gcoCOMMAND object. */
	gcmONERROR(
		gcoCOMMAND_Construct(kernel, &kernel->command));

	/* Construct the gcoEVENT object. */
	gcmONERROR(
		gcoEVENT_Construct(kernel, &kernel->event));

	/* Construct the gcoMMU object. */
	gcmONERROR(
		gcoMMU_Construct(kernel, 32 << 10, &kernel->mmu));

	/* Return pointer to the gcoKERNEL object. */
	kernel->notifyIdle = gcvFALSE;

	/* Return pointer to the gcoKERNEL object. */
	*Kernel = kernel;

	/* Success. */
	return gcvSTATUS_OK;

OnError:
	if (kernel->event != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoEVENT_Destroy(kernel->event));
	}

	if (kernel->command != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoCOMMAND_Destroy(kernel->command));
	}

	if (kernel->hardware != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoHARDWARE_Destroy(kernel->hardware));
	}

    kernel->version = _GAL_VERSION_STRING_;

	gcmVERIFY_OK(
		gcoOS_Free(Os, kernel));

	/* Return the error. */
	return status;
}

/*******************************************************************************
**
**	gcoKERNEL_Destroy
**
**	Destroy an gcoKERNEL object.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object to destroy.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoKERNEL_Destroy(
	IN gcoKERNEL Kernel
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

	/* Destroy the gcoMMU object. */
	gcmVERIFY_OK(gcoMMU_Destroy(Kernel->mmu));

	/* Destroy the gcoEVENT object. */
	gcmVERIFY_OK(gcoEVENT_Destroy(Kernel->event));

	/* Destroy the AQCOMMNAND object. */
	gcmVERIFY_OK(gcoCOMMAND_Destroy(Kernel->command));

	/* Destroy the gcoHARDWARE object. */
	gcmVERIFY_OK(gcoHARDWARE_Destroy(Kernel->hardware));

	/* Mark the gcoKERNEL object as unknown. */
	Kernel->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoKERNEL object. */
	gcmVERIFY_OK(gcoOS_Free(Kernel->os, Kernel));

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	_AllocateMemory
**
**	Private function to walk all required memory pools to allocate the requested
**	amount of video memory.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gcsHAL_INTERFACE * Interface
**			Pointer to a gcsHAL_INTERFACE structure that defines the command to
**			be dispatched.
**
**	OUTPUT:
**
**		gcsHAL_INTERFACE * Interface
**			Pointer to a gcsHAL_INTERFACE structure that receives any data to be
**			returned.
*/
static gceSTATUS
_AllocateMemory(
	IN gcoKERNEL Kernel,
	IN OUT gcePOOL * Pool,
	IN gctSIZE_T Bytes,
	IN gctSIZE_T Alignment,
	IN gceSURF_TYPE Type,
	OUT gcuVIDMEM_NODE_PTR * Node
	)
{
	gcePOOL pool;
	gceSTATUS status;
	gcoVIDMEM videoMemory;

	/* Get initial pool. */
	switch (pool = *Pool)
	{
	case gcvPOOL_DEFAULT:
	case gcvPOOL_LOCAL:
		pool = gcvPOOL_LOCAL_INTERNAL;
		break;

	case gcvPOOL_UNIFIED:
		pool = gcvPOOL_SYSTEM;
		break;
	
	default: 
		break;	
	}

	do
	{
		/* Verify the number of bytes to allocate. */
		if (Bytes == 0)
		{
			status = gcvSTATUS_INVALID_ARGUMENT;
			break;
		}

		if (pool == gcvPOOL_VIRTUAL)
		{
			/* Create a gcuVIDMEM_NODE for virtual memory. */
			gcmERR_BREAK(gcoVIDMEM_ConstructVirtual(Kernel, Bytes, Node));

			/* Success. */
			break;
		}

		else
		{
			/* Get pointer to gcoVIDMEM object for pool. */
			status = gcoKERNEL_GetVideoMemoryPool(Kernel, pool, &videoMemory);

			if (status == gcvSTATUS_OK)
			{
				/* Allocate memory. */
				status = gcoVIDMEM_AllocateLinear(videoMemory,
												  Bytes,
												  Alignment,
												  Type,
												  Node);

				if (status == gcvSTATUS_OK)
				{
					/* Memory allocated. */
					break;
				}
			}
		}

		if (pool == gcvPOOL_LOCAL_INTERNAL)
		{
			/* Advance to external memory. */
			pool = gcvPOOL_LOCAL_EXTERNAL;
		}
		else if (pool == gcvPOOL_LOCAL_EXTERNAL)
		{
			/* Advance to contiguous system memory. */
			pool = gcvPOOL_SYSTEM;
		}
		else if (pool == gcvPOOL_SYSTEM)
		{
			/* Advance to virtual memory. */
			pool = gcvPOOL_VIRTUAL;
		}
		else
		{
			/* Out of pools. */
			break;
		}
	}
	/* Loop only for multiple selection pools. */
	while ((*Pool == gcvPOOL_DEFAULT)
	||     (*Pool == gcvPOOL_LOCAL)
	||     (*Pool == gcvPOOL_UNIFIED)
	);

	if (gcmIS_SUCCESS(status))
	{
		/* Return pool used for allocation. */
		*Pool = pool;
	}

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoKERNEL_Dispatch
**
**	Dispatch a command received from the user HAL layer.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gctBOOL FromUser
**			whether the call is from the user space.
**
**		gcsHAL_INTERFACE * Interface
**			Pointer to a gcsHAL_INTERFACE structure that defines the command to
**			be dispatched.
**
**	OUTPUT:
**
**		gcsHAL_INTERFACE * Interface
**			Pointer to a gcsHAL_INTERFACE structure that receives any data to be
**			returned.
*/
gceSTATUS gcoKERNEL_Dispatch(
	IN gcoKERNEL Kernel,
	IN gctBOOL FromUser,
	IN OUT gcsHAL_INTERFACE * Interface
	)
{
	gceSTATUS status;
	gctUINT32 bitsPerPixel;
	gctSIZE_T bytes;
	gcsHAL_INTERFACE * kernelInterface;
	gcuVIDMEM_NODE_PTR node;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(Interface != gcvNULL);

	if (FromUser)
	{
		/* Map interface structure into kernel address space. */
		status = gcoOS_MapUserPointer(Kernel->os,
									  Interface,
									  sizeof(gcsHAL_INTERFACE),
									  (gctPOINTER *)&kernelInterface);

		if (gcmIS_ERROR(status))
		{
			return status;
		}
	}
	else
	{
		kernelInterface = Interface;
	}

	/* Dispatch on command. */
	switch (Interface->command)
	{
	case gcvHAL_GET_BASE_ADDRESS:
		/* Get base address. */
		status = gcoOS_GetBaseAddress(Kernel->os,
									  &kernelInterface->u.GetBaseAddress.baseAddress);
		break;

	case gcvHAL_QUERY_VIDEO_MEMORY:
		/* Query video memory size. */
		status = gcoKERNEL_QueryVideoMemory(Kernel, kernelInterface);
		break;

	case gcvHAL_QUERY_CHIP_IDENTITY:
		/* Query chip identity. */
		status = gcoHARDWARE_QueryChipIdentity(
			Kernel->hardware,
			&kernelInterface->u.QueryChipIdentity.chipModel,
			&kernelInterface->u.QueryChipIdentity.chipRevision,
			&kernelInterface->u.QueryChipIdentity.chipFeatures,
			&kernelInterface->u.QueryChipIdentity.chipMinorFeatures
			);

		if (gcmIS_SUCCESS(status))
		{
			/* Query chip specifications. */
			status = gcoHARDWARE_QueryChipSpecs(
				Kernel->hardware,
				&kernelInterface->u.QueryChipIdentity.streamCount,
				&kernelInterface->u.QueryChipIdentity.registerMax,
				&kernelInterface->u.QueryChipIdentity.threadCount,
				&kernelInterface->u.QueryChipIdentity.shaderCoreCount,
				&kernelInterface->u.QueryChipIdentity.vertexCacheSize,
				&kernelInterface->u.QueryChipIdentity.vertexOutputBufferSize);
		}
		break;

	case gcvHAL_MAP_MEMORY:
		/* Map memory. */
		status = gcoKERNEL_MapMemory(Kernel,
									 kernelInterface->u.MapMemory.physical,
									 kernelInterface->u.MapMemory.bytes,
									 &kernelInterface->u.MapMemory.logical);
		break;

	case gcvHAL_UNMAP_MEMORY:
		/* Unmap memory. */
		status = gcoKERNEL_UnmapMemory(Kernel,
									   kernelInterface->u.MapMemory.physical,
									   kernelInterface->u.MapMemory.bytes,
									   kernelInterface->u.MapMemory.logical);
		break;

	case gcvHAL_ALLOCATE_NON_PAGED_MEMORY:
		/* Allocate non-paged memory. */
		status = gcoOS_AllocateContiguous(
			Kernel->os,
			FromUser,
			&kernelInterface->u.AllocateNonPagedMemory.bytes,
			&kernelInterface->u.AllocateNonPagedMemory.physical,
			&kernelInterface->u.AllocateNonPagedMemory.logical);
		break;

	case gcvHAL_FREE_NON_PAGED_MEMORY:
		/* Free non-paged memory. */
		status = gcoOS_FreeNonPagedMemory(
			Kernel->os,
			kernelInterface->u.AllocateNonPagedMemory.bytes,
			kernelInterface->u.AllocateNonPagedMemory.physical,
			kernelInterface->u.AllocateNonPagedMemory.logical);
		break;

	case gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY:
		/* Allocate contiguous memory. */
		status = gcoOS_AllocateContiguous(
			Kernel->os,
			FromUser,
			&kernelInterface->u.AllocateNonPagedMemory.bytes,
			&kernelInterface->u.AllocateNonPagedMemory.physical,
			&kernelInterface->u.AllocateNonPagedMemory.logical);
		break;

	case gcvHAL_FREE_CONTIGUOUS_MEMORY:
		/* Free contiguous memory. */
		status = gcoOS_FreeContiguous(
			Kernel->os,
			kernelInterface->u.AllocateNonPagedMemory.physical,
			kernelInterface->u.AllocateNonPagedMemory.logical,
			kernelInterface->u.AllocateNonPagedMemory.bytes);
		break;

	case gcvHAL_ALLOCATE_VIDEO_MEMORY:
		/* Align width and height to tiles. */
		status = gcoHARDWARE_AlignToTile(
			Kernel->hardware,
			kernelInterface->u.AllocateVideoMemory.type,
			&kernelInterface->u.AllocateVideoMemory.width,
			&kernelInterface->u.AllocateVideoMemory.height,
			gcvNULL);

		if (gcmIS_ERROR(status))
		{
			/* Error. */
			break;
		}

		/* Convert format into bytes per pixel and bytes per tile. */
		status = gcoHARDWARE_ConvertFormat(
			Kernel->hardware,
			kernelInterface->u.AllocateVideoMemory.format,
			&bitsPerPixel,
			gcvNULL);

		/* Compute number of bytes for the allocation. */
		bytes = kernelInterface->u.AllocateVideoMemory.width * bitsPerPixel *
				kernelInterface->u.AllocateVideoMemory.height *
				kernelInterface->u.AllocateVideoMemory.depth / 8;

		/* Allocate memory. */
		status = _AllocateMemory(Kernel,
								 &kernelInterface->u.AllocateVideoMemory.pool,
								 bytes,
								 64,
								 kernelInterface->u.AllocateVideoMemory.type,
								 &kernelInterface->u.AllocateVideoMemory.node);
		break;

	case gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY:
		/* Allocate memory. */
		status = _AllocateMemory(
			Kernel,
			&kernelInterface->u.AllocateLinearVideoMemory.pool,
			kernelInterface->u.AllocateLinearVideoMemory.bytes,
			kernelInterface->u.AllocateLinearVideoMemory.alignment,
			kernelInterface->u.AllocateLinearVideoMemory.type,
			&kernelInterface->u.AllocateLinearVideoMemory.node);
		break;

	case gcvHAL_FREE_VIDEO_MEMORY:
		/* Free video memory. */
		status = gcoVIDMEM_Free(Interface->u.FreeVideoMemory.node);
		break;

	case gcvHAL_LOCK_VIDEO_MEMORY:
		/* Lock video memory. */
		status = gcoVIDMEM_Lock(kernelInterface->u.LockVideoMemory.node,
							    &kernelInterface->u.LockVideoMemory.address);

		if (gcmIS_SUCCESS(status))
		{
			node = kernelInterface->u.LockVideoMemory.node;

			if (node->VidMem.memory->object.type == gcvOBJ_VIDMEM)
			{
				/* Map video memory address into user space. */
				status = gcoKERNEL_MapVideoMemory(
					Kernel,
					FromUser,
					kernelInterface->u.LockVideoMemory.address,
					&kernelInterface->u.LockVideoMemory.memory);

				if (gcmIS_ERROR(status))
				{
					/* Roll back. */
#if USE_EVENT_QUEUE
					gcmVERIFY_OK(gcoVIDMEM_Unlock(
						kernelInterface->u.LockVideoMemory.node,
						gcvSURF_TYPE_UNKNOWN,
						gcvNULL));
#else
					gcmVERIFY_OK(gcoVIDMEM_Unlock(
						kernelInterface->u.LockVideoMemory.node,
						gcvSURF_TYPE_UNKNOWN,
						gcvNULL,
						gcvNULL));
#endif
				}
			}
			else
			{
				/* Copy logical memory for virtual memory. */
				kernelInterface->u.LockVideoMemory.memory =
					node->Virtual.logical;

				/* Success. */
				status = gcvSTATUS_OK;
			}
		}

		break;

	case gcvHAL_UNLOCK_VIDEO_MEMORY:
		/* Unlock video memory. */
		node = kernelInterface->u.UnlockVideoMemory.node;

		status = gcoVIDMEM_Unlock(
			node,
			kernelInterface->u.UnlockVideoMemory.type,
#if USE_EVENT_QUEUE
			&kernelInterface->u.UnlockVideoMemory.asynchroneous);
#else
			&kernelInterface->u.UnlockVideoMemory.commandSize,
			kernelInterface->u.UnlockVideoMemory.commands);
#endif
		break;

#if USE_EVENT_QUEUE
	case gcvHAL_EVENT_COMMIT:
		/* Commit an event queue. */
		gcmERR_BREAK(gcoEVENT_Commit(Kernel->event,
						             kernelInterface->u.Event.queue));
        break;
#else
	case gcvHAL_EVENT:
        /* Schedule an event. */
        status = gcoEVENT_Schedule(Kernel->event,
        						   kernelInterface->u.Event.type,
                                   &kernelInterface->u.Event.data,
                                   kernelInterface->u.Event.commands,
                                   (kernelInterface->u.Event.commandSize == 0)
	                                  ? gcvNULL
	                                  : &kernelInterface->u.Event.commandSize);
        break;
#endif

    case gcvHAL_COMMIT:
        /* Commit a command and context buffer. */
        gcmERR_BREAK(gcoCOMMAND_Commit(Kernel->command,
						               kernelInterface->u.Commit.commandBuffer,
							           kernelInterface->u.Commit.contextBuffer));
        break;

    case gcvHAL_STALL:
        /* Stall the command queue. */
        status = gcoCOMMAND_Stall(Kernel->command);
        break;

	case gcvHAL_MAP_USER_MEMORY:
		/* Map user memory to DMA. */
		status = gcoOS_MapUserMemory(Kernel->os,
									 kernelInterface->u.MapUserMemory.memory,
									 kernelInterface->u.MapUserMemory.size,
									 &kernelInterface->u.MapUserMemory.info,
									 &kernelInterface->u.MapUserMemory.address);
		break;

	case gcvHAL_UNMAP_USER_MEMORY:
		/* Unmap user memory. */
		status = gcoOS_UnmapUserMemory(
			Kernel->os,
			kernelInterface->u.UnmapUserMemory.memory,
			kernelInterface->u.UnmapUserMemory.size,
			kernelInterface->u.UnmapUserMemory.info,
			kernelInterface->u.UnmapUserMemory.address);
		break;

	case gcvHAL_USER_SIGNAL:
		/* Dispatch depends on the user signal subcommands. */
		switch(kernelInterface->u.UserSignal.command) 
		{
		case gcvUSER_SIGNAL_CREATE:
			/* Create a signal used in the user space. */
			status = gcoOS_CreateUserSignal(Kernel->os,
							kernelInterface->u.UserSignal.manualReset,
							&kernelInterface->u.UserSignal.id);
			break;

		case gcvUSER_SIGNAL_DESTROY:
			/* Destroy the signal. */
			status = gcoOS_DestroyUserSignal(Kernel->os,
							kernelInterface->u.UserSignal.id);
			break;

		case gcvUSER_SIGNAL_SIGNAL:
			/* Signal the signal. */
			status = gcoOS_SignalUserSignal(Kernel->os,
							kernelInterface->u.UserSignal.id,
							kernelInterface->u.UserSignal.state);
			break;

		case gcvUSER_SIGNAL_WAIT:
			/* Wait on the signal. */
			status = gcoOS_WaitUserSignal(Kernel->os,
							kernelInterface->u.UserSignal.id,
							kernelInterface->u.UserSignal.wait
							);
			break;

		default:
			/* Invalid user signal command. */
			status = gcvSTATUS_INVALID_ARGUMENT;

			break;
		}
        break;

    case gcvHAL_SET_POWER_MANAGEMENT_STATE:
		/* Set the power management state. */
		status = gcoHARDWARE_SetPowerManagementState(
			Kernel->hardware,
			kernelInterface->u.SetPowerManagement.state);
		break;

    case gcvHAL_QUERY_POWER_MANAGEMENT_STATE:
		/* Query the power management state. */
		status = gcoHARDWARE_QueryPowerManagementState(
			Kernel->hardware,
			&kernelInterface->u.QueryPowerManagement.state);
        break;

    case gcvHAL_READ_REGISTER:
		/* Read a register. */
        status = gcoOS_ReadRegister(
			Kernel->os,
			kernelInterface->u.ReadRegisterData.address,
			&kernelInterface->u.ReadRegisterData.data);
        break;

    case gcvHAL_WRITE_REGISTER:
		/* Write a register. */
        status = gcoOS_WriteRegister(
			Kernel->os,
			kernelInterface->u.WriteRegisterData.address,
			kernelInterface->u.WriteRegisterData.data);
		break;

    case gcvHAL_READ_ALL_PROFILE_REGISTERS:
#if VIVANTE_PROFILER
		/* Read all 3D profile registers. */
        status = gcoHARDWARE_QueryProfileRegisters(
			Kernel->hardware,
			kernelInterface->u.RegisterProfileData.hwProfile);
#else
        status = gcvSTATUS_OK;
#endif
        break;

    case gcvHAL_PROFILE_REGISTERS_2D:
		/* Read all 2D profile registers. */
        status = gcoHARDWARE_ProfileEngine2D(
			Kernel->hardware,
			kernelInterface->u.RegisterProfileData2D.hwProfile2D);
        break;

	default:
		/* Invalid command. */
		status = gcvSTATUS_INVALID_ARGUMENT;
	}

	/* Save status. */
	kernelInterface->status = status;

	if (FromUser)
	{
		/* Unmap interface from kernel address space. */
		gcmVERIFY_OK(gcoOS_UnmapUserPointer(Kernel->os, 
											Interface, 
											sizeof(gcsHAL_INTERFACE), 
											kernelInterface));
	}

	/* Return the status. */
	return status;
}
