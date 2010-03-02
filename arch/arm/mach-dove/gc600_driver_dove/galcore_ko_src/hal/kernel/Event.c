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
#include "Context.h"

/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

/*******************************************************************************
**
**	_GetEvent
**
**	Get an empty event ID.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**		gctBOOL Wait
**			Wait for events in case they are exhausted.
**
**		gctBOOL ReduceCount
**          Reduce the number of available events for user command buffers.
**
**	OUTPUT:
**
**		gctUINT8 * EventID
**			Pointer to a variable that receives an empty event ID.
*/
static gceSTATUS
_GetEvent(
    IN gcoEVENT Event,
    IN gctBOOL Wait,
#if USE_EVENT_QUEUE
    IN gctCONST_STRING Title,
#else
    IN gctBOOL ReduceCount,
#endif
    OUT gctUINT8 * EventID
    )
{
    gctINT i;
#if USE_EVENT_QUEUE
	gceSTATUS status;
#else
    gctUINT8 id;
    gctINT count = ReduceCount ? 4 : 0;
#endif

#if USE_EVENT_QUEUE
	/* Grab the queue mutex. */
	status = gcoOS_AcquireMutex(Event->os,
								Event->mutexQueue,
								gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		return status;
	}

	/* Suspend interrupt */
	gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));
#endif

    do
    {
	/* Walk through all events. */
#if USE_EVENT_QUEUE
	for (i = 0; i < gcmCOUNTOF(Event->queues); ++i)
#else
	for (id = Event->lastID, i = 0; i < 32; id = (id + 1) & 31, i++)
#endif
	{
#if USE_EVENT_QUEUE
		if (Event->queues[i].head == gcvNULL)
		{
			*EventID = (gctUINT8) i;

			/* Save time stamp of event. */
			Event->queues[i].stamp = ++(Event->stamp);

			/* Resume interrupt */
			gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

			/* Release the queue mutex. */
			gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->mutexQueue));

			return gcvSTATUS_OK;
		}
	}
	
	/* Resume interrupt */
	gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

	/* Release the queue mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->mutexQueue));

	if (Wait)
	{
	    gcmVERIFY_OK(gcoOS_Delay(Event->os, 1));
	}

	/* Grab the queue mutex. */
	status = gcoOS_AcquireMutex(Event->os,
								Event->mutexQueue,
								gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		return status;
	}

	/* Suspend interrupt */
	gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));
#else
	    /* Check if event is available. */
	    if (Event->schedule[id].stamp == 0)
	    {
		/* Check if we need to keep a number of free events. */
		if (count-- == 0)
		{
		    /* Mark event as used. */
		    Event->schedule[id].stamp = ++(Event->stamp);

		    /* Return event ID. */
		    *EventID = Event->lastID = id;

		    /* Success. */
		    return gcvSTATUS_OK;
	    	}
	    }
	}

	if (ReduceCount)
	{
	    /* Bail out if no free slots. */
	    break;
	}

	if (Wait)
	{
	    gcmVERIFY_OK(gcoOS_Delay(Event->os, 1));
	}
#endif
    }
    while (Wait);

#if USE_EVENT_QUEUE
	/* Resume interrupt */
	gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

	/* Release the queue mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->mutexQueue));
#endif

    /* Out of resources. */
    return gcvSTATUS_OUT_OF_RESOURCES;
}

/******************************************************************************\
******************************* gcoEVENT API Code *******************************
\******************************************************************************/

/*******************************************************************************
**
**	gcoEVENT_Construct
**
**	Construct a new gcoEVENT object.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**	OUTPUT:
**
**		gcoEVENT * Event
**			Pointer to a variable that receives the gcoEVENT object pointer.
*/
gceSTATUS
gcoEVENT_Construct(
	IN gcoKERNEL Kernel,
	OUT gcoEVENT * Event
	)
{
	gcoOS os;
	gceSTATUS status;
	gcoEVENT event;
	int i;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(Event != gcvNULL);

	do
	{
		/* Extract the pointer to the gcoOS object. */
		os = Kernel->os;
		gcmVERIFY_OBJECT(os, gcvOBJ_OS);

		/* Allocate the gcoEVENT object. */
		gcmERR_BREAK(gcoOS_Allocate(os,
									gcmSIZEOF(struct _gcoEVENT),
									(gctPOINTER *) &event));

		/* Initialize the gcoEVENT object. */
		event->object.type = gcvOBJ_EVENT;
		event->kernel      = Kernel;
		event->os          = os;

#if USE_EVENT_QUEUE
		/* Create the queue mutex. */
		status = gcoOS_CreateMutex(os, &event->mutexQueue);

		if (gcmIS_ERROR(status))
		{
			/* Roll back. */
			gcmVERIFY_OK(gcoOS_Free(os, event));

			break;
		}

		/* Zero out the entire event queue. */
		for (i = 0; i < gcmCOUNTOF(event->queues); ++i)
		{
			event->queues[i].head = gcvNULL;
		}

		/* Zero out the time stamp. */
		event->stamp = 0;

		/* No events to handle. */
		event->head  = gcvNULL;
#else
		/* Zero out the entire schedule. */
		for (i = 0; i < gcmCOUNTOF(event->schedule); i++)
		{
			event->schedule[i].stamp = 0;
		}

		/* Start at first event ID. */
		event->stamp  = 0;
		event->lastID = 0;
#endif

		/* Return pointer to the gcoEVENT object. */
		*Event = event;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_Destroy
**
**	Destroy an gcoEVENT object.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoEVENT_Destroy(
	IN gcoEVENT Event
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);

#if USE_EVENT_QUEUE
    /* Delete the queue mutex. */
    gcmVERIFY_OK(gcoOS_DeleteMutex(Event->os, Event->mutexQueue));
#endif

	/* Mark the gcoEVENT object as unknown. */
	Event->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoEVENT object. */
	gcmVERIFY_OK(gcoOS_Free(Event->os, Event));

	/* Success. */
	return gcvSTATUS_OK;
}

#if !USE_EVENT_QUEUE
/*******************************************************************************
**
**	gcoEVENT_Schedule
**
**	Schedule an event.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gceEVENT_TYPE Type
**          Type of event to schedule.
**
**      gcuEVENT_DATA * Data
**          Pointer to event data to schedule.
**
**		gctPOINTER CommandBuffer,
**			Pointer to user-level command buffer.
**
**		gctSIZE_T * CommandBufferSize
**			Pointer to the number of bytes in the user-level command buffer or
**			gcvNULL to use kernel queue for event.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandBuffer
**			Pointer to a variable receiving the number of bytes copied into the
**			command buffer.
*/
gceSTATUS 
gcoEVENT_Schedule(
	IN gcoEVENT Event,
	IN gceEVENT_TYPE Type,
	IN gcuEVENT_DATA * Data,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandBufferSize
	)
{
	gcoOS os;
	gcoHARDWARE hardware;
	gcoCOMMAND command;
	gceSTATUS status;
	gctPOINTER buffer;
	gctSIZE_T required, bytes;
	gctPOINTER address;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(Data != gcvNULL);

	/* Extract the required objects. */
	os = Event->os;
	hardware = Event->kernel->hardware;
	gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
	command = Event->kernel->command;
	gcmVERIFY_OBJECT(command, gcvOBJ_COMMAND);

	if (CommandBufferSize == gcvNULL)
	{
		/* Compute size of EVENT. */
		status = gcoHARDWARE_Event(hardware,
    							  gcvNULL,
    							  0,
    							  gcvKERNEL_COMMAND,
    							  &required);

		if (gcmIS_ERROR(status))
		{
			/* Error. */
			return status;
		}

		/* Reserve space in the command buffer. */
		status = gcoCOMMAND_Reserve(command, required, &buffer, &bytes);

		if (gcmIS_ERROR(status))
		{
			/* Error. */
			return status;
		}
	}
	else
	{
		/* Use user-level command buffer. */
		buffer = CommandBuffer;
		bytes =	*CommandBufferSize;
	}

	/* Dispatch on event type. */
	switch (Type)
	{
	case gcvEVENT_SIGNAL:
		/* Signal. */
		status = gcoEVENT_Signal(Event,
								buffer,
								&bytes,
								Data->Signal.fromWhere,
								Data->Signal.signal,
								Data->Signal.auxSignal,
								Data->Signal.process,
								(CommandBufferSize == gcvNULL));
		break;
	
	case gcvEVENT_UNLOCK:
		/* Unlock. */
		status = gcoEVENT_Unlock(Event,
								 buffer,
								 &bytes,
								 gcvKERNEL_PIXEL,
								 Data->Unlock.node,
								 Data->Unlock.type,
								 (CommandBufferSize == gcvNULL));
		break;

	case gcvEVENT_UNMAP_MEMORY:
		/* Unmap memory. */
		status = gcoEVENT_UnmapMemory(Event,
									  buffer,
									  &bytes,
									  gcvKERNEL_PIXEL,
									  Data->UnmapMemory.bytes,
									  Data->UnmapMemory.physical,
									  Data->UnmapMemory.logical,
									  (CommandBufferSize == gcvNULL));
		break;

	case gcvEVENT_UNMAP_USER_MEMORY:
		/* Unmap user memory. */
		status = gcoEVENT_UnmapUserMemory(Event,
										  buffer,
										  &bytes,
										  gcvKERNEL_PIXEL,
										  Data->UnmapUserMemory.memory,
										  Data->UnmapUserMemory.size,
										  Data->UnmapUserMemory.info,
										  Data->UnmapUserMemory.address,
										  (CommandBufferSize == gcvNULL));
		break;

	case gcvEVENT_FREE_NON_PAGED_MEMORY:
		/* Free non-paged memory. */
		status = gcoEVENT_FreeNonPagedMemory(Event,
    										buffer,
    										&bytes,
											Data->FreeNonPagedMemory.bytes,
											Data->FreeNonPagedMemory.physical,
											Data->FreeNonPagedMemory.logical,
											gcvKERNEL_PIXEL,
											(CommandBufferSize == gcvNULL));
		break;

	case gcvEVENT_FREE_CONTIGUOUS_MEMORY:
		/* Free contiguous memory. */
		status = gcoEVENT_FreeContiguousMemory(Event,
    										   buffer,
    										   &bytes,
											   Data->FreeContiguousMemory.bytes,
											   Data->FreeContiguousMemory.physical,
											   Data->FreeContiguousMemory.logical,
											   gcvKERNEL_PIXEL,
											   (CommandBufferSize == gcvNULL));
		break;

	case gcvEVENT_FREE_VIDEO_MEMORY:
		/* Free video memory. */
		status = gcoEVENT_FreeVideoMemory(Event,
    									 buffer,
    									 &bytes,
										 Data->FreeVideoMemory.node,
										 gcvKERNEL_PIXEL,
										 (CommandBufferSize == gcvNULL));
		break;

	case gcvEVENT_WRITE_DATA:
		/* Map user pointer to kernel space. */
		status = gcoOS_MapUserPointer(Event->os,
									  Data->WriteData.address,
									  sizeof(gctUINT32),
									  &address);

		if (gcmIS_SUCCESS(status))
		{
			/* Write data. */
			status = gcoEVENT_WriteData(Event,
									    buffer,
    								    &bytes,
									    address,
									    Data->WriteData.data,
									    gcvKERNEL_COMMAND,
									    (CommandBufferSize == gcvNULL));

			if (gcmIS_SUCCESS(status))
			{
				/* Unmap the write back address. */
				gcmVERIFY_OK(gcoOS_UnmapUserPointer(Event->os,
													Data->WriteData.address,
													sizeof(gctUINT32),
													address));
			}
		}
		break;

	default:
		if (CommandBufferSize == gcvNULL)
		{
			/* Release command buffer. */
			gcmVERIFY_OK(gcoCOMMAND_Release(command));
		}

		/* Invalid argument. */
		return gcvSTATUS_INVALID_ARGUMENT;
	}

	if (CommandBufferSize == gcvNULL)
	{
		/* Execute EVENT command. */
		status = gcoCOMMAND_Execute(command, required);
	}
	else
	{
		/* Return number of bytes copied to user-level command buffer. */
		*CommandBufferSize = bytes;
	}

	/* Return the status. */
	return status;
}
#endif

/*******************************************************************************
**
**	gcoEVENT_FreeNonPagedMemory
**
**	Schedule an event to free non-paged memory.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gctSIZE_T Bytes
**          Number of bytes of non-paged memory to free.
**
**      gctPHYS_ADDR Physical
**          Physical address of non-paged memory to free.
**
**      gctPOINTER Logical
**          Logical address of non-paged memory to free.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_FreeNonPagedMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gctSIZE_T Bytes,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	)
{
	gctUINT8 id;
	gceSTATUS status;
#if USE_EVENT_QUEUE
	gcsHAL_INTERFACE interface;
#endif

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	do
	{
		/* Get an available event ID. */
#if USE_EVENT_QUEUE
		gcmERR_BREAK(_GetEvent(Event, Wait, "FreeNonPagedMemory", &id));
#else
		gcmERR_BREAK(_GetEvent(Event, Wait, !Wait, &id));
#endif

#if USE_EVENT_QUEUE
		/* Create an event. */
		interface.command = gcvHAL_FREE_NON_PAGED_MEMORY;
		interface.u.FreeNonPagedMemory.bytes    = Bytes;
		interface.u.FreeNonPagedMemory.physical = Physical;
		interface.u.FreeNonPagedMemory.logical  = Logical;

		/* Append it to the queue. */
		gcmERR_BREAK(gcoEVENT_Append(Event, id, &interface));
#else
		/* Mark the event as non-paged memory free. */
		Event->schedule[id].type = gcvEVENT_FREE_NON_PAGED_MEMORY;
		Event->schedule[id].data.FreeNonPagedMemory.bytes = Bytes;
		Event->schedule[id].data.FreeNonPagedMemory.physical = Physical;
		Event->schedule[id].data.FreeNonPagedMemory.logical = Logical;
#endif

		/* Program the hardware event. */
		status = gcoHARDWARE_Event(Event->kernel->hardware,
								   CommandBuffer,
								   id,
								   FromWhere,
								   CommandSize);

		if (gcmIS_ERROR(status))
		{
#if USE_EVENT_QUEUE
			/* Roll back the event append. */
			gcmVERIFY_OK(gcoEVENT_Clear(Event, id));
#else
			/* Error, mark event as available. */
			Event->schedule[id].stamp = 0;
#endif
		}
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_FreeContigiuousMemory
**
**	Schedule an event to free contiguous memory.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gctSIZE_T Bytes
**          Number of bytes of contiguous memory to free.
**
**      gctPHYS_ADDR Physical
**          Physical address of contiguous memory to free.
**
**      gctPOINTER Logical
**          Logical address of contiguous memory to free.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_FreeContiguousMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gctSIZE_T Bytes,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	)
{
	gctUINT8 id;
	gceSTATUS status;
#if USE_EVENT_QUEUE
	gcsHAL_INTERFACE interface;
#endif

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	do
	{
		/* Get an available event ID. */
#if USE_EVENT_QUEUE
		gcmERR_BREAK(_GetEvent(Event, Wait, "FreeContiguousMemory", &id));
#else
		gcmERR_BREAK(_GetEvent(Event, Wait, !Wait, &id));
#endif

#if USE_EVENT_QUEUE
		/* Create an event. */
		interface.command = gcvHAL_FREE_CONTIGUOUS_MEMORY;
		interface.u.FreeContiguousMemory.bytes    = Bytes;
		interface.u.FreeContiguousMemory.physical = Physical;
		interface.u.FreeContiguousMemory.logical  = Logical;

		/* Append it to the queue. */
		gcmERR_BREAK(gcoEVENT_Append(Event, id, &interface));
#else
		/* Mark the event as non-paged memory free. */
		Event->schedule[id].type = gcvEVENT_FREE_CONTIGUOUS_MEMORY;
		Event->schedule[id].data.FreeContiguousMemory.bytes    = Bytes;
		Event->schedule[id].data.FreeContiguousMemory.physical = Physical;
		Event->schedule[id].data.FreeContiguousMemory.logical  = Logical;
#endif

		/* Program the hardware event. */
		status = gcoHARDWARE_Event(Event->kernel->hardware,
								   CommandBuffer,
								   id,
								   FromWhere,
								   CommandSize);

		if (gcmIS_ERROR(status))
		{
#if USE_EVENT_QUEUE
			/* Roll back the event append. */
			gcmVERIFY_OK(gcoEVENT_Clear(Event, id));
#else
			/* Error, mark event as available. */
			Event->schedule[id].stamp = 0;
#endif
		}
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_FreeVideoMemory
**
**	Schedule an event to free video memory.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gcuVIDMEM_NODE_PTR VideoMemory
**          Pointer to a gcuVIDMEM_NODE object to free.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_FreeVideoMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gcuVIDMEM_NODE_PTR VideoMemory,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	)
{
	gctUINT8 id;
	gceSTATUS status;
#if USE_EVENT_QUEUE
	gcsHAL_INTERFACE interface;
#endif

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	/* Get an available event ID. */
#if USE_EVENT_QUEUE
	status = _GetEvent(Event, Wait, "FreeVideoMemory", &id);
#else
	status = _GetEvent(Event, Wait, !Wait, &id);
#endif

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}

#if USE_EVENT_QUEUE
	/* Create an event. */
	interface.command = gcvHAL_FREE_VIDEO_MEMORY;
	interface.u.FreeVideoMemory.node = VideoMemory;

		/* Append it to the queue. */
	status = gcoEVENT_Append(Event, id, &interface);
	
	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}
#else
	/* Mark the event as video memory free. */
	Event->schedule[id].type = gcvEVENT_FREE_VIDEO_MEMORY;
	Event->schedule[id].data.FreeVideoMemory.node = VideoMemory;
#endif

	/* Program the hardware event. */
	status = gcoHARDWARE_Event(Event->kernel->hardware,
							   CommandBuffer,
							   id,
							   FromWhere,
							   CommandSize);

	if (gcmIS_ERROR(status))
	{
#if USE_EVENT_QUEUE
			/* Roll back the event append. */
			gcmVERIFY_OK(gcoEVENT_Clear(Event, id));
#else
		/* Error, mark event as available. */
		Event->schedule[id].stamp = 0;
#endif
	}

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_WriteData
**
**	Schedule an event to write data.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gctPOINTER Address
**          Pointer to the memory location to write data to.
**
**      gctUINT32 Data
**          Data to write.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_WriteData(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gctPOINTER Address,
	IN gctUINT32 Data,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	)
{
	gcoOS os;
	gctUINT8 id;
	gceSTATUS status;
	gctUINT32 address;
#if USE_EVENT_QUEUE
	gcsHAL_INTERFACE interface;
#endif

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	do
	{
		/* Extract the gcoOS object pointer. */
		os = Event->os;
		gcmVERIFY_OBJECT(os, gcvOBJ_OS);

		/* Convert address into physical address. */
		gcmERR_BREAK(gcoOS_GetPhysicalAddress(os, Address, &address));

		/* Get an available event ID. */
#if USE_EVENT_QUEUE
		gcmERR_BREAK(_GetEvent(Event, Wait, "WriteData", &id));
#else
		gcmERR_BREAK(_GetEvent(Event, Wait, !Wait, &id));
#endif

		/* Mark the event as a data write. */
#if USE_EVENT_QUEUE
		interface.command = gcvHAL_WRITE_DATA;
		interface.u.WriteData.address = address;
		interface.u.WriteData.data    = Data;
	       interface.u.WriteData.kernelAddress = Address;
		
		/* Append it to the queue. */
		gcmERR_BREAK(gcoEVENT_Append(Event, id, &interface));
#else
		Event->schedule[id].type = gcvEVENT_WRITE_DATA;
		Event->schedule[id].data.WriteData.address = gcmINT2PTR(address);
		Event->schedule[id].data.WriteData.data = Data;
		Event->schedule[id].data.WriteData.kernelAddress = Address;     
#endif

		/* Program the hardware event. */
		status = gcoHARDWARE_Event(Event->kernel->hardware,
								   CommandBuffer,
								   id,
								   FromWhere,
								   CommandSize);

		if (gcmIS_ERROR(status))
		{
#if USE_EVENT_QUEUE
			/* Roll back the event append. */
			gcmVERIFY_OK(gcoEVENT_Clear(Event, id));
#else
			/* Error, mark event as available. */
			Event->schedule[id].stamp = 0;
#endif
		}
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_SetIdle
**
**	Schedule an event to set idle status.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_SetIdle(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gctBOOL Wait
	)
{
	gcoOS os;
	gctUINT8 id;
	gceSTATUS status;
#if USE_EVENT_QUEUE
	gcsHAL_INTERFACE interface;
#endif

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	do
	{
		/* Extract the gcoOS object pointer. */
		os = Event->os;
		gcmVERIFY_OBJECT(os, gcvOBJ_OS);

		/* Get an available event ID. */
#if USE_EVENT_QUEUE
		gcmERR_BREAK(_GetEvent(Event, Wait, "SetIdle", &id));
#else
		gcmERR_BREAK(_GetEvent(Event, Wait, !Wait, &id));
#endif

		/* Mark the event as a data write. */
#if USE_EVENT_QUEUE
		interface.command = gcvHAL_SET_IDLE;

		/* Append it to the queue. */
		gcmERR_BREAK(gcoEVENT_Append(Event, id, &interface));
#else
		Event->schedule[id].type = gcvEVENT_SET_IDLE;
#endif

		/* Program the hardware event. */
		status = gcoHARDWARE_Event(Event->kernel->hardware,
								   CommandBuffer,
								   id,
								   FromWhere,
								   CommandSize);

		if (gcmIS_ERROR(status))
		{
#if USE_EVENT_QUEUE
			/* Roll back the event append. */
			gcmVERIFY_OK(gcoEVENT_Clear(Event, id));
#else
			/* Error, mark event as available. */
			Event->schedule[id].stamp = 0;
#endif
		}
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_Unlock
**
**	Schedule an event to unlock virtual memory.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**      gcuVIDMEM_NODE_PTR Node
**          Pointer to a gcuVIDMEM_NODE union that specifies the virtual memory
**			to unlock.
**
**		gceSURF_TYPE Type
**			Type of surface to unlock.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_Unlock(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gcuVIDMEM_NODE_PTR Node,
	IN gceSURF_TYPE Type,
	IN gctBOOL Wait
	)
{
	gctUINT8 id;
	gceSTATUS status;
#if USE_EVENT_QUEUE
	gcsHAL_INTERFACE interface;
#endif

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	do
	{
#if USE_EVENT_QUEUE
		/* Get an available event ID. */
		gcmERR_BREAK(_GetEvent(Event, Wait, "Unlock", &id));
#else
		gcmERR_BREAK(_GetEvent(Event, Wait, !Wait, &id));
#endif

#if USE_EVENT_QUEUE
		/* Mark the event as an unlock. */
		interface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
		interface.u.UnlockVideoMemory.node = Node;
		interface.u.UnlockVideoMemory.type = Type;

		/* Append it to the queue. */
		gcmERR_BREAK(gcoEVENT_Append(Event, id, &interface));
#else
		Event->schedule[id].type = gcvEVENT_UNLOCK;
		Event->schedule[id].data.Unlock.node = Node;
		Event->schedule[id].data.Unlock.type = Type;
#endif

		/* Program the hardware event. */
		status = gcoHARDWARE_Event(Event->kernel->hardware,
								   CommandBuffer,
								   id,
								   FromWhere,
								   CommandSize);

		if (gcmIS_ERROR(status))
		{
#if USE_EVENT_QUEUE
			/* Roll back the event append. */
			gcmVERIFY_OK(gcoEVENT_Clear(Event, id));
#else
			/* Error, mark event as available. */
			Event->schedule[id].stamp = 0;
#endif
		}
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

#if !USE_EVENT_QUEUE
/*******************************************************************************
**
**	gcoEVENT_Signal
**
**	Schedule an event to signal a signal.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**      gctSIGNAL Signal
**          Handle of the signal to signal.
**
**      gctHANDLE Process
**          Handle of the process owning the signal.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_Signal(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gctSIGNAL Signal,
	IN gctSIGNAL AuxSignal,
	IN gctHANDLE Process,
	IN gctBOOL Wait
	)
{
	gctUINT8 id;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	/* Get an available event ID. */
	status = _GetEvent(Event, Wait, !Wait, &id);

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}

	/* Mark the event as a signal. */
	Event->schedule[id].type = gcvEVENT_SIGNAL;
	Event->schedule[id].data.Signal.signal = Signal;
	Event->schedule[id].data.Signal.auxSignal = AuxSignal;
	Event->schedule[id].data.Signal.process = Process;

	/* Program the hardware event. */
	status = gcoHARDWARE_Event(Event->kernel->hardware,
							   CommandBuffer,
							   id,
							   FromWhere,
							   CommandSize);

	if (gcmIS_ERROR(status))
	{
		/* Error, mark event as available. */
		Event->schedule[id].stamp = 0;
	}


	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_UnmapMemory
**
**	Schedule an event to unmap a buffer mapped through its physical address.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**      gctSIZE_T Size
**			Memory size to be unmapped.
**
**      gctUINT32 Address
**			Physical address to be unmapped.
**
**      gctPOINTER Memory
**			Virtual address to be unmapped.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
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
	)
{
	gctUINT8 id;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	/* Get an available event ID. */
	status = _GetEvent(Event, Wait, !Wait, &id);

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}

	/* Mark the event as a signal. */
	Event->schedule[id].type = gcvEVENT_UNMAP_MEMORY;
	Event->schedule[id].data.UnmapMemory.bytes    = Size;
	Event->schedule[id].data.UnmapMemory.physical = Address;
	Event->schedule[id].data.UnmapMemory.logical  = Memory;

	/* Program the hardware event. */
	status = gcoHARDWARE_Event(Event->kernel->hardware,
							   CommandBuffer,
							   id,
							   FromWhere,
							   CommandSize);

	if (gcmIS_ERROR(status))
	{
		/* Error, mark event as available. */
		Event->schedule[id].stamp = 0;
	}

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_UnmapUserMemory
**
**	Schedule an event to unmap the user memory.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctPOINTER CommandBuffer
**          Pointer to the command buffer to append the event.
**
**      gctSIZE_T * CommandSize
**          Pointer to the number of bytes available in the command buffer.
**
**      gceKERNEL_WHERE FromWhere
**          Place in the pipe where the event needs to be generated.
**
**      gctPOINTER Memory
**			Virtual address to be unmapped.
**
**      gctSIZE_T Size
**			Memory size to be unmapped.
**
**      gctPOINTER Info
**			Private data for user memory mapping.
**
**      gctUINT32 Address
**			Physical address to be unmapped.
**
**		gctBOOL Wait
**			Wait flag.  Should be set to gcvFALSE for any event that needs to be
**			copied into a command buffer since we might exhaust the number of
**			events.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**          Number of bytes used for the event.
*/
gceSTATUS
gcoEVENT_UnmapUserMemory(
	IN gcoEVENT Event,
	IN gctPOINTER CommandBuffer,
	IN OUT gctSIZE_T * CommandSize,
	IN gceKERNEL_WHERE FromWhere,
	IN gctPOINTER Memory,
	IN gctSIZE_T Size,
	IN gctPOINTER Info,
	IN gctUINT32 Address,
	IN gctBOOL Wait
	)
{
	gctUINT8 id;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(CommandSize != gcvNULL);

	/* Get an available event ID. */
	status = _GetEvent(Event, Wait, !Wait, &id);

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}

	/* Mark the event as a signal. */
	Event->schedule[id].type = gcvEVENT_UNMAP_USER_MEMORY;
	Event->schedule[id].data.UnmapUserMemory.memory = Memory;
	Event->schedule[id].data.UnmapUserMemory.size = Size;
	Event->schedule[id].data.UnmapUserMemory.info = Info;
	Event->schedule[id].data.UnmapUserMemory.address = Address;

	/* Program the hardware event. */
	status = gcoHARDWARE_Event(Event->kernel->hardware,
							   CommandBuffer,
							   id,
							   FromWhere,
							   CommandSize);

	if (gcmIS_ERROR(status))
	{
		/* Error, mark event as available. */
		Event->schedule[id].stamp = 0;
	}

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoEVENT_Notify
**
**	Callback function to notify the gcoEVENT object an event has occured.
**
**	INPUT:
**
**		gcoEVENT Event
**			Pointer to an gcoEVENT object.
**
**      gctUINT32 IDs
**          Bit mask of event IDs that occured.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoEVENT_Notify(
	IN gcoEVENT Event,
	IN gctUINT32 IDs
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctPOINTER logical;
	gctSIGNAL signal;
	gctINT id;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);

	while (IDs != 0)
	{
		gctINT current;
		struct _gcsEVENT_SCHEDULE * event = gcvNULL;

		for (id = 0; id < 32; ++id)
		{
			if (IDs & (1 << id))
			{
				if (Event->schedule[id].stamp == 0)
				{
					gcmTRACE_ZONE(gcvZONE_EVENT, gcvLEVEL_ERROR,
								  "gcoEVENT_Notify: Triggered ID=%d is not set!",
								  id);

					IDs &= ~(1 << id);
				}
				else if ((event == gcvNULL)
				||       (Event->schedule[id].stamp < event->stamp)
				)
				{
					event   = &Event->schedule[id];
					current = id;
				}
			}
		}

		if (event == gcvNULL)
		{
			continue;
		}

		/* Dispatch on event type. */
		switch (event->type)
		{
		case gcvEVENT_SIGNAL:
			if (event->data.Signal.auxSignal != gcvNULL)
			{
				/* Map the aux signal into kernel space. */
				gcmVERIFY_OK(gcoOS_MapSignal(Event->os,
											 event->data.Signal.auxSignal,
											 event->data.Signal.process,
											 &signal));

				if (signal == gcvNULL)
				{
					/* Signal. */
					gcmVERIFY_OK(gcoOS_Signal(Event->os,
											  event->data.Signal.auxSignal, 
											  gcvTRUE));
				}
				else
				{
					/* Signal. */
					gcmVERIFY_OK(gcoOS_Signal(Event->os, signal, gcvTRUE));

					/* Destroy the mapped signal. */
					gcmVERIFY_OK(gcoOS_DestroySignal(Event->os, signal));
				}
			}

			/* Map the signal into kernel space. */
			gcmVERIFY_OK(gcoOS_MapSignal(Event->os,
										 event->data.Signal.signal,
										 event->data.Signal.process,
										 &signal));

			if (signal == gcvNULL)
			{
				/* Signal. */
				gcmVERIFY_OK(gcoOS_Signal(Event->os,
										  event->data.Signal.signal, 
										  gcvTRUE));
			}
			else
			{
				/* Signal. */
				gcmVERIFY_OK(gcoOS_Signal(Event->os, signal, gcvTRUE));

				/* Destroy the mapped signal. */
				gcmVERIFY_OK(gcoOS_DestroySignal(Event->os, signal));
			}

			/* Success. */
			status = gcvSTATUS_OK;
			break;

		case gcvEVENT_UNLOCK:
			/* Unlock. */
			gcmVERIFY_OK(gcoVIDMEM_Unlock(event->data.Unlock.node, 
										  event->data.Unlock.type, 
										  gcvNULL, 
										  gcvNULL));

			/* Success. */
			status = gcvSTATUS_OK;
			break;

		case gcvEVENT_UNMAP_MEMORY:
			/* Unmap user memory mapped through physical address. */
			status = gcoOS_UnmapMemory(Event->os,
									   event->data.UnmapMemory.physical,
									   event->data.UnmapMemory.bytes,
									   event->data.UnmapMemory.logical);
			break;

		case gcvEVENT_UNMAP_USER_MEMORY:
			/* Unmap user memory mapped through logical address. */
			status = gcoOS_UnmapUserMemory(Event->os,
										   event->data.UnmapUserMemory.memory,
										   event->data.UnmapUserMemory.size,
										   event->data.UnmapUserMemory.info,
										   event->data.UnmapUserMemory.address);
			break;
			
		case gcvEVENT_FREE_NON_PAGED_MEMORY:
			/* Free non-paged memory. */
			status = gcoOS_FreeNonPagedMemory(
						 Event->os,
						 event->data.FreeNonPagedMemory.bytes,
						 event->data.FreeNonPagedMemory.physical,
						 event->data.FreeNonPagedMemory.logical);
			break;

		case gcvEVENT_FREE_CONTIGUOUS_MEMORY:
			/* Free contiguous memory. */
			status = gcoOS_FreeContiguous(
						 Event->os,
						 event->data.FreeNonPagedMemory.physical,
						 event->data.FreeNonPagedMemory.logical,
						 event->data.FreeNonPagedMemory.bytes);
			break;

		case gcvEVENT_FREE_VIDEO_MEMORY:
			/* Free video memory. */
			status = gcoVIDMEM_Free(event->data.FreeVideoMemory.node);
			break;

		case gcvEVENT_WRITE_DATA:
			/* Convert physical into logical address. */
			gcmVERIFY_OK(
				gcoOS_MapPhysical(Event->os,
								  gcmPTR2INT(event->data.WriteData.address),
								  gcmPTR2INT(event->data.WriteData.kernelAddress),
								  sizeof(gctUINT32),
								  &logical));

			/* Write data. */
			gcmVERIFY_OK(gcoOS_WriteMemory(Event->os,
										   logical, 
										   event->data.WriteData.data));

			/* Unmap the physical memory. */
			gcmVERIFY_OK(gcoOS_UnmapPhysical(Event->os,
	    									 logical,
											 sizeof(gctUINT32)));

			/* Success. */
			status = gcvSTATUS_OK;
			break;

		default:
			/* Invalid argument. */
			gcmFATAL("gcoEVENT_Notify: Unknown event type: %d", event->type);
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Mark event is available. */
		IDs &= ~(1 << current);
		event->stamp = 0;
	}

	/* Return the status. */
	return status;
}
#else
gceSTATUS
gcoEVENT_Append(
	IN gcoEVENT Event,
	IN gctUINT8 Id,
	IN gcsHAL_INTERFACE_PTR Interface
	)
{
	gceSTATUS status;
	gcsEVENT_PTR event = gcvNULL;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(Id < gcmCOUNTOF(Event->queues));

	do
	{
		/* Allocate a new event record. */
		gcmERR_BREAK(gcoOS_Allocate(Event->os,
									gcmSIZEOF(gcsEVENT),
									(gctPOINTER *) &event));

		/* Grab the queue mutex. */
		gcmERR_BREAK(gcoOS_AcquireMutex(Event->os,
										Event->mutexQueue,
										gcvINFINITE));

		/* Suspend interrupt */
		gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));

		/* Initiailze the event record. */
		event->next  = gcvNULL;
		event->event = *Interface;
		
		if (Event->queues[Id].head == gcvNULL)
		{
			/* Mark haead of new queue. */
			Event->queues[Id].head = event;
		}
		else
		{
			/* Link in event record at tail of queue. */
			Event->queues[Id].tail->next = event;
		}

		/* Mark new tail of queue. */
		Event->queues[Id].tail = event;

		/* Resume interrupt */
		gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

		/* Release the queue mutex. */
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->mutexQueue));

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	if (event != gcvNULL)
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoOS_Free(Event->os, event));
	}

	/* Return the status. */
	return status;
}

gceSTATUS
gcoEVENT_Clear(
	IN gcoEVENT Event,
	IN gctUINT8 Id
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	gcmVERIFY_ARGUMENT(Id < gcmCOUNTOF(Event->queues));

	/* Grab the queue mutex. */
	status = gcoOS_AcquireMutex(Event->os,
								Event->mutexQueue,
								gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		return status;
	}

	/* Suspend interrupt */
	gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));

	/* Look while we have event records in the queue. */
	while (Event->queues[Id].head != gcvNULL)
	{
		/* Unlink the event record from the queue. */
		gcsEVENT_PTR event = Event->queues[Id].head;
		Event->queues[Id].head = event->next;

		/* Free the event record. */
		gcmERR_BREAK(gcoOS_Free(Event->os, event));
	}

	if (gcmIS_SUCCESS(status))
	{
		/* Mark queue as empty. */
		Event->queues[Id].tail = gcvNULL;
	}

	/* Resume interrupt */
	gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

	/* Release the queue mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->mutexQueue));

	/* Return the status. */
	return status;
}

gceSTATUS
gcoEVENT_TryToSetIdle(
	IN gcoEVENT Event
	);

gceSTATUS
gcoEVENT_Commit(
	IN gcoEVENT Event,
	IN gcsQUEUE_PTR Queue
	)
{
	gctUINT8 id = ~0;
	gceSTATUS status;
	gctSIZE_T bytes;
	gctPOINTER buffer = gcvNULL;
    gctBOOL newCommand;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);

	/* Don't do antyhing if there is nothing to commit. */
	if (Queue == gcvNULL)
	{
		/* Try to set idle */
		gcmVERIFY_OK(gcoEVENT_TryToSetIdle(Event));

		return gcvSTATUS_OK;
	}

	do
	{
		/* Get event ID. */
		gcmERR_BREAK(_GetEvent(Event, gcvTRUE, "Commit", &id));

		/* Loop while there are records in the queue. */
        newCommand = gcvFALSE;
        
		while (Queue != gcvNULL && !newCommand)
		{
			gcsQUEUE_PTR record, next;

			/* Map record into kernel memory. */
			gcmERR_BREAK(gcoOS_MapUserPointer(Event->os,
											  Queue,
											  gcmSIZEOF(gcsQUEUE),
											  (gctPOINTER *) &record));

			/* Append event record to event queue. */
			gcmERR_BREAK(gcoEVENT_Append(Event, id, &record->interface));

            if (record->next != gcvNULL)
            {
                switch (record->interface.command)
                {
                case gcvHAL_UNLOCK_VIDEO_MEMORY:
                    if (record->interface.u.UnlockVideoMemory.node->VidMem.memory->object.type != gcvOBJ_VIDMEM)
                    {
                        newCommand = gcvTRUE;
                    }
                    break;
                    
                case gcvHAL_FREE_VIDEO_MEMORY:
                    if (record->interface.u.FreeVideoMemory.node->VidMem.memory->object.type != gcvOBJ_VIDMEM)
                    {
                        newCommand = gcvTRUE;
                    }
                    break;
				
				default:
					break;
                }
            }

			/* Next record in the queue. */
			next = record->next;

			/* Unmap record from kernel memory. */
			gcmERR_BREAK(gcoOS_UnmapUserPointer(Event->os,
												Queue,
												gcmSIZEOF(gcsQUEUE),
												(gctPOINTER *) record));
			Queue = next;
		}
        
        if (gcmIS_ERROR(status)) break;

		/* Get the size of the hardware event. */
		gcmERR_BREAK(gcoHARDWARE_Event(Event->kernel->hardware,
									   gcvNULL,
									   id,
									   gcvKERNEL_PIXEL,
									   &bytes));

		/* Reserve space in the command queue. */
		gcmERR_BREAK(gcoCOMMAND_Reserve(Event->kernel->command,
										bytes,
										&buffer,
										&bytes));

		/* Set the hardware event in the command queue. */
		gcmERR_BREAK(gcoHARDWARE_Event(Event->kernel->hardware,
									   buffer,
									   id,
									   gcvKERNEL_PIXEL,
									   &bytes));

		/* Execute the hardware event. */
		gcmERR_BREAK(gcoCOMMAND_Execute(Event->kernel->command, bytes));
	}
	while (Queue != gcvNULL);

	if (gcmIS_ERROR(status) && (id != 0xFF))
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoEVENT_Clear(Event, id));

		if (buffer != gcvNULL)
		{
			/* Release the command buffer. */
			gcmVERIFY_OK(gcoCOMMAND_Release(Event->kernel->command));
		}
	}
	
	/* Return the status. */
	return status;
}

gceSTATUS
gcoEVENT_Interrupt(
	IN gcoEVENT Event,
	IN gctUINT32 Data
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	
	/* Loop while there are interrupts to handle. */
	while (Data != 0)
	{
		gctINT i;
		gctUINT32 mask, savedMask = 0;
		gcsEVENT_QUEUE * queue = gcvNULL;

		/* Find the interrupt with the smallest time stamp. */
		for (i = 0, mask = 0x1; i < 32; ++i, mask <<= 1)
		{
			if ((Data & ~(mask - 1)) == 0)
			{
				break;
			}

			if ((Data & mask)
			&&  (  (queue == gcvNULL)
				|| (Event->queues[i].stamp < queue->stamp)
				)
			)
			{
				queue     = &Event->queues[i];
				savedMask = mask;
			}
		}

		/* Make sure we have a valid interrupt. */
		gcmASSERT((queue != gcvNULL) && (queue->head != gcvNULL));
		
		if (queue->head != gcvNULL)
		{
			if (Event->head == gcvNULL)
			{
				/* Chain empty - set it to the interrupted event chain. */
				Event->head = queue->head;
				Event->tail = queue->tail;
			}
			else
			{
				/* Chain used - append interrupted chain. */
				gcmASSERT(Event->tail != gcvNULL);
				Event->tail->next = queue->head;
				Event->tail = queue->tail;
			}
		}

		/* Remove event from interrupts. */
		queue->head = gcvNULL;
		Data       &= ~savedMask;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

gceSTATUS
gcoEVENT_Notify(
	IN gcoEVENT Event,
	IN gctUINT32 IDs
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);
	
	/* Suspend interrupt */
	gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));

	/* Loop while there are events to be handled. */
	while (Event->head != gcvNULL)
	{
		gcsEVENT_PTR event;
		gctPOINTER logical;
		gctSIGNAL signal;

		/* Unlink head from chain. */
		event = Event->head;
		Event->head = Event->head->next;
		if (Event->head == gcvNULL) Event->tail = gcvNULL;

		/* Resume interrupt */
		gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

		/* Dispatch on event type. */
		switch (event->event.command)
		{
		case gcvHAL_FREE_NON_PAGED_MEMORY:
			/* Free non-paged memory. */
			gcmERR_BREAK(gcoOS_FreeNonPagedMemory(
				Event->os,
				event->event.u.FreeNonPagedMemory.bytes,
				event->event.u.FreeNonPagedMemory.physical,
				event->event.u.FreeNonPagedMemory.logical));
			break;
			
		case gcvHAL_FREE_CONTIGUOUS_MEMORY:
			/* Unmap the user memory. */
			gcmERR_BREAK(gcoOS_FreeContiguous(
				Event->os,
				event->event.u.FreeContiguousMemory.physical,
				event->event.u.FreeContiguousMemory.logical,
				event->event.u.FreeContiguousMemory.bytes));
			break;
			
		case gcvHAL_FREE_VIDEO_MEMORY:
			/* Free video memory. */
			gcmERR_BREAK(gcoVIDMEM_Free(event->event.u.FreeVideoMemory.node));
			break;

		case gcvHAL_WRITE_DATA:
			/* Convert physical into logical address. */
			gcmERR_BREAK(gcoOS_MapPhysical(Event->os,
										   gcmPTR2INT(event->event.u.WriteData.address),
										   gcmPTR2INT(event->event.u.WriteData.kernelAddress),
										   gcmSIZEOF(gctUINT32),
										   &logical));

			/* Write data. */
			gcmERR_BREAK(gcoOS_WriteMemory(Event->os,
										   logical, 
										   event->event.u.WriteData.data));

			/* Unmap the physical memory. */
			gcmERR_BREAK(gcoOS_UnmapPhysical(Event->os,
    										 logical,
											 gcmSIZEOF(gctUINT32)));
			break;

		case gcvHAL_UNLOCK_VIDEO_MEMORY:
			/* Unlock. */
			gcmERR_BREAK(gcoVIDMEM_Unlock(event->event.u.UnlockVideoMemory.node,
										  event->event.u.UnlockVideoMemory.type,
										  gcvNULL));
			break;
		
		case gcvHAL_SIGNAL:
			if (event->event.u.Signal.auxSignal != gcvNULL)
			{
				/* Map the aux signal into kernel space. */
				gcmERR_BREAK(gcoOS_MapSignal(Event->os,
											 event->event.u.Signal.auxSignal,
											 event->event.u.Signal.process,
											 &signal));

				if (signal == gcvNULL)
				{
					/* Signal. */
					gcmERR_BREAK(gcoOS_Signal(Event->os,
											  event->event.u.Signal.auxSignal, 
											  gcvTRUE));
				}
				else
				{
					/* Signal. */
					gcmERR_BREAK(gcoOS_Signal(Event->os, signal, gcvTRUE));

					/* Destroy the mapped signal. */
					gcmERR_BREAK(gcoOS_DestroySignal(Event->os, signal));
				}
			}

			/* Map the signal into kernel space. */
			gcmERR_BREAK(gcoOS_MapSignal(Event->os,
										 event->event.u.Signal.signal,
										 event->event.u.Signal.process,
										 &signal));

			if (signal == gcvNULL)
			{
				/* Signal. */
				gcmERR_BREAK(gcoOS_Signal(Event->os,
										  event->event.u.Signal.signal, 
										  gcvTRUE));
			}
			else
			{
				/* Signal. */
				gcmERR_BREAK(gcoOS_Signal(Event->os, signal, gcvTRUE));

				/* Destroy the mapped signal. */
				gcmERR_BREAK(gcoOS_DestroySignal(Event->os, signal));
			}
			break;
			
		case gcvHAL_UNMAP_USER_MEMORY:
			/* Unmap the user memory. */
			status = gcoOS_UnmapUserMemory(Event->os,
										   event->event.u.UnmapUserMemory.memory,
										   event->event.u.UnmapUserMemory.size,
										   event->event.u.UnmapUserMemory.info,
										   event->event.u.UnmapUserMemory.address);
			break;
		
		case gcvHAL_SET_IDLE:
			/* Grab the conmmand queue mutex. */
			gcmVERIFY_OK(gcoOS_AcquireMutex(Event->os,
											Event->kernel->command->mutexQueue,
											gcvINFINITE));

			/* Set idle if no new commitments */
			if (Event->lastCommitStamp == Event->kernel->command->commitStamp)
			{
				Event->kernel->command->idle = gcvTRUE;

				gcmVERIFY_OK(gcoOS_NotifyIdle(Event->os, gcvTRUE));
			}
					
			/* Release the command queue mutex. */
			gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->kernel->command->mutexQueue));

			break;

		default:
			/* Invalid argument. */
			gcmFATAL("gcoEVENT_Notify: Unknown event type: %d", event->event.command);
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Make sure there are no errors generated. */
		gcmASSERT(gcmNO_ERROR(status));

		/* Free the event. */
		gcmVERIFY_OK(gcoOS_Free(Event->os, event));

		/* Suspend interrupt */
		gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));
	}

	/* Resume interrupt */
	gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

	/* Try to set idle */
	gcmVERIFY_OK(gcoEVENT_TryToSetIdle(Event));

	/* Success. */
	return gcvSTATUS_OK;
}

gceSTATUS
gcoEVENT_TryToSetIdle(
	IN gcoEVENT Event
	)
{
	gctINT i;
	gctBOOL setIdle;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Event, gcvOBJ_EVENT);

	if (!Event->kernel->notifyIdle) return gcvSTATUS_OK;

	/* Initialize the flag */
	setIdle = gcvFALSE;
	
	/* Grab the queue mutex. */
	gcmVERIFY_OK(gcoOS_AcquireMutex(Event->os,
									Event->mutexQueue,
									gcvINFINITE));

	/* Suspend interrupt */
	gcmVERIFY_OK(gcoOS_SuspendInterrupt(Event->os));
	
	if (Event->lastCommitStamp != Event->kernel->command->commitStamp)
	{
		setIdle = gcvTRUE;

		/* Check if no pending events */
		for (i = 0; i < gcmCOUNTOF(Event->queues); ++i)
		{
			if (Event->queues[i].head != gcvNULL)
			{
				setIdle = gcvFALSE;
				break;
			}
		}
	}
	
	/* Resume interrupt */
	gcmVERIFY_OK(gcoOS_ResumeInterrupt(Event->os));

	/* Release the queue mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Event->os, Event->mutexQueue));

	/* Issue an event to set idle if necessary */
	if (setIdle)
	{
		gctSIZE_T requested, bytes;
		gctUINT8 * buffer;
		gctSIZE_T bufferSize;

		/* Get the size of the EVENT command. */
		gcmVERIFY_OK(gcoHARDWARE_Event(Event->kernel->hardware,
					   gcvNULL,
					   0,
					   gcvKERNEL_PIXEL,
					   &requested));

		/* Reserve space in the command queue. */
		gcmVERIFY_OK(gcoCOMMAND_Reserve(Event->kernel->command, 
						requested, 
						(gctPOINTER *) &buffer, 
						&bufferSize));

		/* Append the EVENT command to write data into the boolean. */
		bytes = bufferSize;
		gcmVERIFY_OK(gcoEVENT_SetIdle(Event,
									buffer,
									&bytes,
									gcvKERNEL_PIXEL,
									gcvTRUE));
		
		Event->lastCommitStamp = Event->kernel->command->commitStamp + 1;
	
		/* Append the reserved commands to the command queue. */
		gcmVERIFY_OK(gcoCOMMAND_Execute(Event->kernel->command, requested));
	}

	/* Success. */
	return gcvSTATUS_OK;
}
#endif /* USE_EVENT_QUEUE */
