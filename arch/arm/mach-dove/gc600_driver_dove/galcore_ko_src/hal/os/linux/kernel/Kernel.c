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
**	gcoKERNEL_QueryVideoMemory
**
**	Query the amount of video memory.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**	OUTPUT:
**
**		gcsHAL_INTERFACE * Interface
**			Pointer to an gcsHAL_INTERFACE structure that will be filled in with
**			the memory information.
*/
gceSTATUS gcoKERNEL_QueryVideoMemory(
	IN gcoKERNEL Kernel,
	OUT gcsHAL_INTERFACE * Interface
	)
{
	gcoGALDEVICE device;

	gcmTRACE(gcvLEVEL_VERBOSE, 
			"[ENTER] gcoKERNEL_QueryVideoMemory(%p,%p)\n",
		  	Kernel, 
			Interface);

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(Interface != NULL);

	/* Extract the pointer to the gcoGALDEVICE class. */
	device = (gcoGALDEVICE) Kernel->context;

	/* Get internal memory size and physical address. */
	Interface->u.QueryVideoMemory.internalSize = device->internalSize;
	Interface->u.QueryVideoMemory.internalPhysical = device->internalPhysical;

	/* Get external memory size and physical address. */
	Interface->u.QueryVideoMemory.externalSize = device->externalSize;
	Interface->u.QueryVideoMemory.externalPhysical = device->externalPhysical;

	/* Get contiguous memory size and physical address. */
	Interface->u.QueryVideoMemory.contiguousSize = device->contiguousSize;
	Interface->u.QueryVideoMemory.contiguousPhysical = device->contiguousPhysical;

	gcmTRACE(gcvLEVEL_VERBOSE,
			"[LEAVE] gcoKERNEL_QueryVideoMemory\n");

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoKERNEL_GetVideoMemoryPool
**
**	Get the gcoVIDMEM object belonging to the specified pool.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gcePOOL Pool
**			Pool to query gcoVIDMEM object for.
**
**	OUTPUT:
**
**		gcoVIDMEM * VideoMemory
**			Pointer to a variable that will hold the pointer to the gcoVIDMEM
**			object belonging to the requested pool.
*/
gceSTATUS gcoKERNEL_GetVideoMemoryPool(
	IN gcoKERNEL Kernel,
	IN gcePOOL Pool,
	OUT gcoVIDMEM * VideoMemory
	)
{
	gcoGALDEVICE device;
	gcoVIDMEM videoMemory;

	gcmTRACE(gcvLEVEL_VERBOSE, "[ENTER] gcoKERNEL_GetVideoMemoryPool(%p,%u,%p)\n",
		  	Kernel,	Pool, VideoMemory);

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(VideoMemory != NULL);

    /* Extract the pointer to the gcoGALDEVICE class. */
    device = (gcoGALDEVICE) Kernel->context;

	/* Dispatch on pool. */
	switch (Pool)
	{
	case gcvPOOL_LOCAL_INTERNAL:
		/* Internal memory. */
		videoMemory = device->internalVidMem;
		break;

	case gcvPOOL_LOCAL_EXTERNAL:
		/* External memory. */
		videoMemory = device->externalVidMem;
		break;

	case gcvPOOL_SYSTEM:
		/* System memory. */
		videoMemory = device->contiguousVidMem;
		break;

	default:
		/* Unknown pool. */
		videoMemory = NULL;
	}

	/* Return pointer to the AQVIDMEM object. */
	*VideoMemory = videoMemory;

	gcmTRACE(gcvLEVEL_VERBOSE, "[LEAVE] gcoKERNEL_GetVideoMemoryPool\n");

	/* Return status. */
	return (videoMemory == NULL) ? gcvSTATUS_OUT_OF_MEMORY : gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoKERNEL_MapMemory
**
**	Map video memory into the current process space.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gctPHYS_ADDR Physical
**			Physical address of video memory to map.
**
**		gctSIZE_T Bytes
**			Number of bytes to map.
**
**	OUTPUT:
**
**		gctPOINTER * Logical
**			Pointer to a variable that will hold the base address of the mapped
**			memory region.
*/
gceSTATUS gcoKERNEL_MapMemory(
	IN gcoKERNEL Kernel,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	)
{
  return gcoOS_MapMemory(Kernel->os, Physical, Bytes, Logical);
}

/*******************************************************************************
**
**	gcoKERNEL_UnmapMemory
**
**	Unmap video memory from the current process space.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gctPHYS_ADDR Physical
**			Physical address of video memory to map.
**
**		gctSIZE_T Bytes
**			Number of bytes to map.
**
**		gctPOINTER Logical
**			Base address of the mapped memory region.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoKERNEL_UnmapMemory(
	IN gcoKERNEL Kernel,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	)
{
  return gcoOS_UnmapMemory(Kernel->os, Physical, Bytes, Logical);
}

/*******************************************************************************
**
**	gcoKERNEL_MapVideoMemory
**
**	Get the logical address for a hardware specific memory address for the
**	current process.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to map the memory into the user space.
**
**		gctUINT32 Address
**			Hardware specific memory address.
**
**	OUTPUT:
**
**		gctPOINTER * Logical
**			Pointer to a variable that will hold the logical address of the
**			specified memory address.
*/
gceSTATUS gcoKERNEL_MapVideoMemory(
	IN gcoKERNEL Kernel,
	IN gctBOOL InUserSpace,
	IN gctUINT32 Address,
	OUT gctPOINTER * Logical
	)
{
    gcoGALDEVICE	device;
    PLINUX_MDL  	mdl;
	PLINUX_MDL_MAP	mdlMap;
    gcePOOL 		pool;
    gctUINT32 		offset, base;
    gceSTATUS 		status;
    gctPOINTER 		logical;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
    gcmVERIFY_ARGUMENT(Logical != NULL);

    /* Extract the pointer to the gcoGALDEVICE class. */
    device = (gcoGALDEVICE)Kernel->context;

    /* Split the memory address into a pool type and offset. */
    status = gcoHARDWARE_SplitMemory(Kernel->hardware,
    	    	    	    	     Address,
				     &pool,
				     &offset);

    if (gcmIS_ERROR(status))
    {
    	/* Error. */
    	return status;
    }

    /* Dispatch on pool. */
    switch (pool)
    {
    case gcvPOOL_LOCAL_INTERNAL:
    	/* Internal memory. */
    	logical = device->internalLogical;
    	break;

    case gcvPOOL_LOCAL_EXTERNAL:
    	/* External memory. */
    	logical = device->externalLogical;
    	break;

    case gcvPOOL_SYSTEM:
		/* System memory. */
		if (device->contiguousMapped)
		{
			logical = device->contiguousBase;
		}
		else
		{
			mdl = (PLINUX_MDL) device->contiguousPhysical;

			mdlMap = FindMdlMap(mdl, current->tgid);
			gcmASSERT(mdlMap);

			logical = (gctPOINTER) mdlMap->vmaAddr;
		}

		gcmVERIFY_OK(gcoHARDWARE_SplitMemory(
			Kernel->hardware,
			device->contiguousVidMem->baseAddress,
			&pool, &base
			));

		offset -= base;
		break;

    default:
    	/* Invalid memory pool. */
    	return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Build logical address of specified address. */
    *Logical = (gctPOINTER) ((gctUINT8 *) logical + offset);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoKERNEL_Notify
**
**	This function iscalled by clients to notify the gcoKERNRL object of an event.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gceNOTIFY Notification
**			Notification event.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoKERNEL_Notify(
	IN gcoKERNEL Kernel,
	IN gceNOTIFY Notification,
	IN gctBOOL Data
	)
{
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

	/* Dispatch on notifcation. */
	switch (Notification)
	{
	case gcvNOTIFY_INTERRUPT:
		/* Process the interrupt. */
#if COMMAND_PROCESSOR_VERSION > 1
		status = gcoINTERRUPT_Notify(Kernel->interrupt, Data);
#else
		status = gcoHARDWARE_Interrupt(Kernel->hardware, Data);
#endif
		break;

	default:
		status = gcvSTATUS_OK;
		break;
	}

	/* Success. */
	return status;
}
