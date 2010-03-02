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
**	_NewQueue
**
**	Allocate a new command queue.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object.
**
**	OUTPUT:
**
**		gcoCOMMAND Command
**			gcoCOMMAND object has been updated with a new command queue.
*/
static gceSTATUS _NewQueue(
    IN OUT gcoCOMMAND Command
    )
{
    gceSTATUS status;
    gctPHYS_ADDR physical;
    gctPOINTER logical;
    gctSIZE_T bytes;

    /* Allocate new command queue. */
    status = gcoOS_AllocateNonPagedMemory(Command->os, 
										 gcvFALSE, 
										 &Command->pageSize,
                                         &physical, 
										 &logical);

    if (status < 0)
    {
        /* Error. */
        return status;
    }

    bytes = Command->pageSize;

    /* Schedule old command queue to be deleted. */
    status = gcoEVENT_FreeNonPagedMemory(Command->kernel->event,
    									logical,
    									&bytes,
                                        Command->pageSize,
                                        Command->physical,
                                        Command->logical,
                                        gcvKERNEL_COMMAND,
                                        gcvTRUE);

    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        gcmVERIFY_OK(gcoOS_FreeNonPagedMemory(Command->os, Command->pageSize,
                                          physical, logical));

        /* Error. */
        return status;
    }

    /* Update gcoCOMMAND object with new command queue. */
    Command->newQueue = gcvTRUE;
    Command->physical = physical;
    Command->logical = logical;
    Command->offset = bytes;

    /* Success. */
    return gcvSTATUS_OK;
}

/******************************************************************************\
****************************** gcoCOMMAND API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**	gcoCOMMAND_Construct
**
**	Construct a new gcoCOMMAND object.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**	OUTPUT:
**
**		gcoCOMMAND * Command
**			Pointer to a variable that will hold the pointer to the gcoCOMMAND
**			object.
*/
gceSTATUS gcoCOMMAND_Construct(
	IN gcoKERNEL Kernel,
	OUT gcoCOMMAND * Command
	)
{
    gcoOS os;
	gcoCOMMAND command;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(Command != gcvNULL);

    /* Extract the gcoOS object. */
    os = Kernel->os;

	/* Allocate the gcoCOMMAND structure. */
	status = gcoOS_Allocate(os, sizeof(struct _gcoCOMMAND), (gctPOINTER *) &command);

	if (status != gcvSTATUS_OK)
	{
		/* Error. */
		return status;
	}

	/* Initialize the gcoCOMMAND object.*/
	command->object.type = gcvOBJ_COMMAND;
	command->kernel = Kernel;
	command->os = os;

    /* Get the command buffer requirements. */
    status = gcoHARDWARE_QueryCommandBuffer(Kernel->hardware,
                                           &command->alignment,
                                           &command->reservedHead,
                                           &command->reservedTail);

    if (status < 0)
    {
        /* Roll-back. */
        gcmVERIFY_OK(gcoOS_Free(os, command));

        /* Error. */
        return status;
    }

    /* No contexts available yet. */
    command->contextCounter = command->currentContext = 0;

    /* Create the command queue mutex. */
    status = gcoOS_CreateMutex(os, &command->mutexQueue);

    if (status < 0)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcoOS_Free(os, command));

        /* Error. */
        return status;
    }

	/* Create the context switching mutex. */
	status = gcoOS_CreateMutex(os, &command->mutexContext);

	if (status < 0)
	{
	    /* Roll back. */
        gcmVERIFY_OK(gcoOS_DeleteMutex(os, command->mutexQueue));
	    gcmVERIFY_OK(gcoOS_Free(os, command));

	    /* Error. */
	    return status;
	}

	/* Get the page size from teh OS. */
	status = gcoOS_GetPageSize(os, &command->pageSize);

	if (status != gcvSTATUS_OK)
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoOS_Free(os, command));

		/* Error. */
		return status;
	}

	/* Set hardware to pipe 0. */
	command->pipeSelect = 0;

    /* No command queues created yet. */
	command->newQueue = gcvFALSE;
    command->logical = gcvNULL;

	/* Command is not yet running. */
	command->running = gcvFALSE;

	/* Command queue is idle. */
	command->idle = gcvTRUE;

	/* Commit stamp is zero. */
	command->commitStamp = 0;

    /* Return pointer to the gcoCOMMAND object. */
	*Command = command;

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoCOMMAND_Destroy
**
**	Destroy an gcoCOMMAND object.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object to destroy.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoCOMMAND_Destroy(
	IN gcoCOMMAND Command
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

	/* Stop the command queue. */
	gcmVERIFY_OK(gcoCOMMAND_Stop(Command));

	if (Command->logical != gcvNULL)
	{
		/* Free the command page. */
		gcmVERIFY_OK(gcoOS_FreeNonPagedMemory(Command->os, Command->pageSize,
										  Command->physical, Command->logical));
	}

    /* Delete the context switching mutex. */
    gcmVERIFY_OK(gcoOS_DeleteMutex(Command->os, Command->mutexContext));

    /* Delete the command queue mutex. */
    gcmVERIFY_OK(gcoOS_DeleteMutex(Command->os, Command->mutexQueue));

	/* Mark object as unknown. */
	Command->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoCOMMAND object. */
	gcmVERIFY_OK(gcoOS_Free(Command->os, Command));

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoCOMMAND_Start
**
**	Start up the command queue.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object to start.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoCOMMAND_Start(
	IN gcoCOMMAND Command
	)
{
    gcoHARDWARE hardware;
	gceSTATUS status;
    gctSIZE_T bytes;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

	if (Command->running)
	{
		/* Command queue already running. */
		return gcvSTATUS_OK;
	}

	/* Extract the gcoHARDWARE object. */
	hardware = Command->kernel->hardware;
	gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

	if (Command->logical == gcvNULL)
	{
		/* Allocate command page. */
		status = gcoOS_AllocateNonPagedMemory(Command->os, gcvFALSE,
		                                     &Command->pageSize,
		                                     &Command->physical,
		                                     &Command->logical);

		if (status != gcvSTATUS_OK)
		{
			/* Error. */
			return status;
		}
	}

	/* Start at beginning of page. */
	Command->offset = 0;

	/* Append WAIT/LINK. */
    bytes = Command->pageSize;
	gcmERR_RETURN(
		gcoHARDWARE_WaitLink(hardware, 
							 Command->logical, 
							 0, 
							 &bytes,
							 &Command->wait, 
							 &Command->waitSize));

    /* Adjust offset. */
    Command->offset = bytes;

	/* Enable command processor. */
	gcmERR_RETURN(
		gcoHARDWARE_Execute(hardware,
							Command->logical, 
							bytes));

	/* Command queue is running. */
	Command->running = gcvTRUE;

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoCOMMAND_Stop
**
**	Stop the command queue.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object to stop.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoCOMMAND_Stop(
	IN gcoCOMMAND Command
	)
{
    gcoHARDWARE hardware;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

	if (!Command->running)
	{
		/* Command queue is not running. */
		return gcvSTATUS_OK;
	}

    /* Extract the gcoHARDWARE object. */
    hardware = Command->kernel->hardware;
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Replace last WAIT with END. */
    gcmERR_RETURN(
		gcoHARDWARE_End(hardware,
						Command->wait,
						&Command->waitSize));

	/* Command queue is no longer running. */
	Command->running = gcvFALSE;

	/* Success. */
	return gcvSTATUS_OK;
}

typedef struct _gcsMAPPED * gcsMAPPED_PTR;
struct _gcsMAPPED
{
	gcsMAPPED_PTR next;
	gctPOINTER pointer;
	gctPOINTER kernelPointer;
	gctSIZE_T bytes;
};

static gceSTATUS
_AddMap(
	IN gcoOS Os,
	IN gctPOINTER Source,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Destination,
	IN OUT gcsMAPPED_PTR * Stack
	)
{
	gcsMAPPED_PTR map = gcvNULL;
	gceSTATUS status;

	/* Don't try to map NULL pointers. */
	if (Source == gcvNULL)
	{
		*Destination = gcvNULL;
		return gcvSTATUS_OK;
	}

	do
	{
		/* Allocate the gcsMAPPED structure. */
		gcmERR_BREAK(gcoOS_Allocate(Os, sizeof(*map), (gctPOINTER *) &map));

		/* Map the user pointer into kernel addressing space. */
		gcmERR_BREAK(gcoOS_MapUserPointer(Os, Source, Bytes, Destination));

		/* Save mapping. */
		map->pointer       = Source;
		map->kernelPointer = *Destination;
		map->bytes         = Bytes;

		/* Push structure on top of the stack. */
		map->next = *Stack;
		*Stack    = map;

		/* Success. */
		status = gcvSTATUS_OK;
	}
	while (gcvFALSE);

	if (gcmIS_ERROR(status) && (map != gcvNULL))
	{
		/* Roll back on error. */
		gcmVERIFY_OK(gcoOS_Free(Os, map));
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoCOMMAND_Commit
**
**	Commit a command buffer to the command queue.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object.
**
**		gcoCMDBUF CommandBuffer
**			Pointer to an gcoCMDBUF object.
**
**		gcoCONTEXT Context
**			Pointer to an gcoCONTEXT object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoCOMMAND_Commit(
	IN gcoCOMMAND Command,
	IN gcoCMDBUF CommandBuffer,
	IN gcoCONTEXT Context
	)
{
	gcoCMDBUF commandBuffer;
	gcoCONTEXT context;
	gcoHARDWARE hardware;
	gcoEVENT event;
	gceSTATUS status;
	gctPOINTER initialLink, link;
	gctSIZE_T bytes, initialSize, lastRun;
	gcoCMDBUF buffer;
	gctPOINTER wait;
	gctSIZE_T waitSize;
	gctUINT32 offset;
	gctPOINTER fetchAddress;
	gctSIZE_T fetchSize;
	gctPOINTER logical;
	gcsMAPPED_PTR stack = gcvNULL;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);
	gcmVERIFY_OK(_AddMap(Command->os,
						 CommandBuffer,
						 sizeof(struct _gcoCMDBUF),
						 (gctPOINTER *) &commandBuffer,
						 &stack));
	gcmVERIFY_OBJECT(commandBuffer, gcvOBJ_COMMANDBUFFER);
	gcmVERIFY_OK(_AddMap(Command->os,
						 Context,
						 sizeof(struct _gcoCONTEXT),
						 (gctPOINTER *) &context,
						 &stack));
	gcmVERIFY_OBJECT(context, gcvOBJ_CONTEXT);

	/* Extract the gcoHARDWARE and gcoEVENT objects. */
	hardware = Command->kernel->hardware;
	gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
	event = Command->kernel->event;
	gcmVERIFY_OBJECT(event, gcvOBJ_EVENT);

	/* Acquire the context switching mutex. */
	status = gcoOS_AcquireMutex(Command->os,
								Command->mutexContext,
								gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}

	do
	{
		/* Reserved slot in the context or command buffer. */
		gcmERR_BREAK(gcoHARDWARE_PipeSelect(hardware, gcvNULL, 0, &bytes));

		/* Test if we need to switch to this context. */
		if ((context->id != 0)
		&&  (context->id != Command->currentContext)
		)
		{
			gcmVERIFY_OK(_AddMap(Command->os, 
								 context->logical,
								 bytes,
								 &logical,
								 &stack));

			if (context->pipe2DIndex != 0)
			{
				if (context->initialPipe == Command->pipeSelect)
				{
					gctUINT32 reserved = bytes;
					gctUINT8_PTR nop   = (gctUINT8_PTR) logical;

					/* Already in the correct pipe, fill context buffer with NOP. */
					while (reserved > 0)
					{
						bytes = reserved;
						gcmERR_BREAK(gcoHARDWARE_Nop(hardware,
													 nop,
													 &bytes));

						reserved -= bytes;
						nop      += bytes;
					}
				}
				else
				{
					/* Switch to the correct pipe. */
					gcmERR_BREAK(gcoHARDWARE_PipeSelect(hardware,
														logical,
														context->initialPipe,
														&bytes));
				}
			}

			/* Save initial link pointer. */
			initialLink = logical;
			initialSize = context->bufferSize;

			/* Save pointer to next link. */
			gcmVERIFY_OK(_AddMap(Command->os,
								 context->link,
								 8,
								 &link,
								 &stack));

			/* Start parsing CommandBuffer. */
			buffer = commandBuffer;

			/* Mark context buffer as used. */
			if (context->inUse != gcvNULL)
			{
				*context->inUse = gcvTRUE;
			}
		}

		else
		{
			/* Test if this is a new context. */
			if (context->id == 0)
			{
				/* Generate unique ID for the context buffer. */
				context->id = ++ Command->contextCounter;

				if (context->id == 0)
				{
					/* Context counter overflow (wow!) */
					status = gcvSTATUS_TOO_COMPLEX;
					break;
				}
			}

			gcmVERIFY_OK(_AddMap(Command->os,
								 commandBuffer->logical,
								 bytes,
								 &logical,
								 &stack));

			if (context->entryPipe == Command->pipeSelect)
			{
				gctUINT32 reserved = bytes;
				gctUINT8_PTR nop   = (gctUINT8_PTR) logical;

				/* Already in the correct pipe, fill context buffer with NOP. */
				while (reserved > 0)
				{
					bytes = reserved;
					gcmERR_BREAK(gcoHARDWARE_Nop(hardware,
												 nop,
												 &bytes));

					reserved -= bytes;
					nop      += bytes;
				}
			}
			else
			{
				/* Switch to the correct pipe. */
				gcmVERIFY_OK(gcoHARDWARE_PipeSelect(hardware,
													logical,
													context->entryPipe,
													&bytes));
			}

			/* Save initial link pointer. */
			initialLink = logical;
			initialSize = commandBuffer->offset + Command->reservedTail;

			/* Save pointer to next link. */
			link = (gctUINT8 *) logical + commandBuffer->offset;

			/* Start parsing next CommandBuffer. */
			gcmVERIFY_OK(_AddMap(Command->os,
								 commandBuffer->sibling,
								 sizeof(struct _gcoCMDBUF),
								 (gctPOINTER *) &buffer,
								 &stack));
		}

		/* Loop through all remaining command buffers. */
		while (buffer != gcvNULL)
		{
			/* Reserved slot in the command buffer. */
			bytes = Command->reservedHead;

			/* First slot becomes a NOP. */
			gcmVERIFY_OK(_AddMap(Command->os,
								 buffer->logical,
								 bytes,
								 &logical,
								 &stack));

			{
				gctUINT32 reserved = bytes;
				gctUINT8_PTR nop   = (gctUINT8_PTR) logical;

				/* Already in the correct pipe, fill context buffer with NOP. */
				while (reserved > 0)
				{
					bytes = reserved;
					gcmERR_BREAK(gcoHARDWARE_Nop(hardware,
												 nop,
												 &bytes));

					reserved -= bytes;
					nop      += bytes;
				}
			}

			/* Generate the LINK to this command buffer. */
			gcmERR_BREAK(
				gcoHARDWARE_Link(hardware,
								 link,
								 logical,
								 buffer->offset + Command->reservedTail,
								 &bytes));

			/* Save pointer to next link. */
			link = (gctUINT8 *) logical + buffer->offset;

			/* Parse next buffer. */
			gcmVERIFY_OK(_AddMap(Command->os,
								 buffer->sibling,
								 sizeof(struct _gcoCMDBUF),
								 (gctPOINTER *) &buffer,
								 &stack));
		}

		/* Compute number of bytes required for WAIT/LINK. */
		gcmERR_BREAK(
			gcoHARDWARE_WaitLink(hardware,
								 gcvNULL,
								 Command->offset,
								 &bytes, 
								 gcvNULL, 
								 gcvNULL));

		lastRun = bytes;

		/* Grab the command queue mutex. */
		gcmERR_BREAK(gcoOS_AcquireMutex(Command->os,
										Command->mutexQueue,
										gcvINFINITE));

		if (Command->kernel->notifyIdle)
		{
			/* Increase the commit stamp */
			Command->commitStamp++;

			/* Set busy if idle */
			if (Command->idle)
			{
				Command->idle = gcvFALSE;

				gcmVERIFY_OK(gcoOS_NotifyIdle(Command->os, gcvFALSE));
			}
		}

		do
		{
			/* Compute number of bytes left in current command queue. */
			bytes = Command->pageSize - Command->offset;

			if (bytes < lastRun)
			{
				/* Create a new command queue. */
				gcmERR_BREAK(_NewQueue(Command));

				/* Adjust run size with any extra commands inserted. */
				lastRun += Command->offset;
			}

			/* Get current offset. */
			offset = Command->offset;

			/* Append WAIT/LINK in command queue. */
			bytes  = Command->pageSize - offset;

			gcmERR_BREAK(
				gcoHARDWARE_WaitLink(hardware,
									 (gctUINT8 *) Command->logical + offset,
									 offset,
									 &bytes,
									 &wait,
									 &waitSize));

			/* Adjust offset. */
			offset += bytes;

			if (Command->newQueue)
			{
				/* Compute fetch location and size for a new command queue. */
				fetchAddress = Command->logical;
				fetchSize = offset;
			}
			else
			{
				/* Compute fetch location and size for an existing command
				   queue. */
				fetchAddress = (gctUINT8 *) Command->logical + Command->offset;
				fetchSize = offset - Command->offset;
			}

			bytes = 8;

			/* Link in WAIT/LINK. */
			gcmVERIFY_OK(gcoHARDWARE_Link(hardware,
										  link,
										  fetchAddress,
										  fetchSize,
										  &bytes));

			/* Execute the entire sequence. */
			gcmERR_BREAK(gcoHARDWARE_Link(hardware,
										  Command->wait,
										  initialLink,
										  initialSize,
										  &Command->waitSize));

			/* Update command queue offset. */
			Command->offset   = offset;
			Command->newQueue = gcvFALSE;

			/* Update address of last WAIT. */
			Command->wait     = wait;
			Command->waitSize = waitSize;

			/* Update context and pipe select. */
			Command->currentContext = context->id;
			Command->pipeSelect     = context->currentPipe;

			/* Update queue tail pointer. */
			gcmERR_BREAK(gcoHARWDARE_UpdateQueueTail(hardware,
													 Command->logical,
													 Command->offset));
		}
		while (gcvFALSE);

		/* Release the command queue mutex. */
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Command->os, Command->mutexQueue));
	}
	while (gcvFALSE);

	/* Release the context switching mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Command->os, Command->mutexContext));

	/* Unmap all mapped pointers. */
	while (stack != gcvNULL)
	{
		gcsMAPPED_PTR map = stack;
		stack = map->next;

		gcmVERIFY_OK(gcoOS_UnmapUserPointer(Command->os,
											map->pointer,
											map->bytes,
											map->kernelPointer));

		gcmVERIFY_OK(gcoOS_Free(Command->os, map));
	}

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoCOMMAND_Reserve
**
**	Reserve space in the command queue.  Also acquire the command queue mutex.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object.
**
**		gctSIZE_T RequestedBytes
**			Number of bytes previously reserved.
**
**	OUTPUT:
**
**		gctPOINTER * Buffer
**          Pointer to a variable that will receive the address of the reserved
**          space.
**
**      gctSIZE_T * BufferSize
**          Pointer to a variable that will receive the number of bytes
**          available in the command queue.
*/
gceSTATUS gcoCOMMAND_Reserve(
    IN gcoCOMMAND Command,
    IN gctSIZE_T RequestedBytes,
    OUT gctPOINTER * Buffer,
    OUT gctSIZE_T * BufferSize
    )
{
    gceSTATUS status;
    gctSIZE_T requiredBytes, bytes;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

    /* Grab the conmmand queue mutex. */
    status = gcoOS_AcquireMutex(
		Command->os,
		Command->mutexQueue,
		gcvINFINITE
		);

    if (status < 0)
    {
        /* Error. */
        return status;
    }

    do
    {
        /* Compute number of bytes required for WAIT/LINK. */
        gcmERR_BREAK(
			gcoHARDWARE_WaitLink(Command->kernel->hardware,
								 gcvNULL,
								 Command->offset + gcmALIGN(RequestedBytes,
															Command->alignment),
                                 &requiredBytes,
								 gcvNULL,
								 gcvNULL));

        /* Compute total number of bytes required. */
        requiredBytes += gcmALIGN(RequestedBytes, Command->alignment);

        /* Compute number of bytes available in command queue. */
        bytes = Command->pageSize - Command->offset;

        if (bytes < requiredBytes)
        {
            /* Create a new command queue. */
            status = _NewQueue(Command);

            if (status < 0)
            {
                /* Error. */
                break;
            }

            /* Recompute number of bytes available in command queue. */
            bytes = Command->pageSize - Command->offset;

            if (bytes < requiredBytes)
            {
                /* Rare case, not enough room in command queue. */
                status = gcvSTATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        /* Return pointer to empty slot command queue. */
        *Buffer = (gctUINT8 *) Command->logical + Command->offset;

        /* Return number of bytes left in command queue. */
        *BufferSize = bytes;

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (status < 0)
    {
        /* Release command queue mutex on error. */
        gcmVERIFY_OK(gcoOS_ReleaseMutex(Command->os, Command->mutexQueue));
    }

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**	gcoCOMMAND_Release
**
**	Release a previously reserved command queue.  The command FIFO mutex will be
**  released.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoCOMMAND_Release(
    IN gcoCOMMAND Command
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

    /* Release the command queue mutex. */
    gcmVERIFY_OK(gcoOS_ReleaseMutex(Command->os, Command->mutexQueue));

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoCOMMAND_Execute
**
**	Execute a previously reserved command queue by appending a WAIT/LINK command
**  sequence after it and modifying the last WAIT into a LINK command.  The
**  command FIFO mutex will be released whether this function succeeds or not.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object.
**
**		gctSIZE_T RequestedBytes
**			Number of bytes previously reserved.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoCOMMAND_Execute(
    IN gcoCOMMAND Command,
    IN gctSIZE_T RequestedBytes
    )
{
    gctUINT32 offset;
    gctPOINTER address;
    gctSIZE_T bytes;
    gceSTATUS status;
    gctPOINTER wait;
    gctSIZE_T waitBytes;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

    do
    {
		if (Command->kernel->notifyIdle)
		{
			/* Increase the commit stamp */
			Command->commitStamp++;

			/* Set busy if idle */
			if (Command->idle)
			{
				Command->idle = gcvFALSE;

				gcmVERIFY_OK(gcoOS_NotifyIdle(Command->os, gcvFALSE));
			}
		}

        /* Compute offset for WAIT/LINK. */
        offset = Command->offset + RequestedBytes;

        /* Compute number of byts left in command queue. */
        bytes = Command->pageSize - offset;

        /* Append WAIT/LINK in command queue. */
        gcmERR_BREAK(
			gcoHARDWARE_WaitLink(Command->kernel->hardware,
								 (gctUINT8 *) Command->logical + offset,
								 offset,
								 &bytes,
								 &wait, 
								 &waitBytes));

        if (Command->newQueue)
        {
            /* For a new command queue, point to the start of the command
            ** queue and include both the commands inserted at the head of it
            ** and the WAIT/LINK. */
            address = Command->logical;
            bytes  += offset;
        }
        else
        {
            /* For an existing command queue, point to the current offset and
            ** include the WAIT/LINK. */
            address = (gctUINT8 *) Command->logical + Command->offset;
            bytes  += RequestedBytes;
        }

        /* Convert th last WAIT into a LINK. */
        gcmERR_BREAK(gcoHARDWARE_Link(
			Command->kernel->hardware, Command->wait,
			address, bytes, &Command->waitSize
			));

        /* Update the pointer to the last WAIT. */
        Command->wait = wait;
        Command->waitSize = waitBytes;

        /* Update the command queue. */
        Command->offset += bytes;
        Command->newQueue = gcvFALSE;

        /* Update queue tail pointer. */
        gcmERR_BREAK(gcoHARWDARE_UpdateQueueTail(
			Command->kernel->hardware,
			Command->logical,
			Command->offset
			));
    }
    while (gcvFALSE);

    /* Release the command queue mutex. */
    gcmVERIFY_OK(gcoOS_ReleaseMutex(Command->os, Command->mutexQueue));

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**	gcoCOMMAND_Stall
**
**	The calling thread will be suspended until the command queue has been
**  completed.
**
**	INPUT:
**
**		gcoCOMMAND Command
**			Pointer to an gcoCOMMAND object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoCOMMAND_Stall(
    IN gcoCOMMAND Command
    )
{
    gcoOS os;
    gcoHARDWARE hardware;
    gcoEVENT event;
    gceSTATUS status;
    volatile gctBOOL * busy;
    gctSIZE_T requested, bytes;
    gctUINT8 * buffer;
    gctSIZE_T bufferSize;
    gctBOOL acquired = gcvFALSE;
#ifdef _DEBUG
	gctUINT32 counter = 0;
#endif

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Command, gcvOBJ_COMMAND);

    /* Extract the gcoOS object pointer. */
    os = Command->os;
    gcmVERIFY_OBJECT(os, gcvOBJ_OS);

    /* Extract the gcoHARDWARE object pointer. */
    hardware = Command->kernel->hardware;
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Extract the gcoEVENT object pointer. */
    event = Command->kernel->event;
    gcmVERIFY_OBJECT(event, gcvOBJ_EVENT);

    /* Allocate the busy variable. */
    status = gcoOS_Allocate(os, sizeof(gctBOOL), (gctPOINTER *) &busy);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    /* Mark the hardware as busy. */
    *busy = gcvTRUE;

    do
    {
        /* Get the size of the EVENT command. */
        gcmERR_BREAK(gcoHARDWARE_Event(hardware,
        						   gcvNULL,
        						   0,
        						   gcvKERNEL_PIXEL,
        						   &requested));

        /* Reserve space in the command queue. */
        gcmERR_BREAK(gcoCOMMAND_Reserve(Command, 
								    requested, 
									(gctPOINTER *) &buffer, 
									&bufferSize));

        /* Command queue has been acquired. */
        acquired = gcvTRUE;

        /* Append the EVENT command to write data into the boolean. */
        bytes = bufferSize;
        gcmERR_BREAK(gcoEVENT_WriteData(event,
        						    buffer,
        						    &bytes,
        						    (gctPOINTER) busy,
        						    gcvFALSE,
                                    gcvKERNEL_PIXEL,
                                    gcvTRUE));

        /* Append the reserved commands to the command queue. */
        status = gcoCOMMAND_Execute(Command, requested);

        /* Execute releases the command queue. */
        acquired = gcvFALSE;

        if (gcmIS_ERROR(status) || (status == gcvSTATUS_CHIP_NOT_READY))
        {
            /* Error. */
            break;
        }

        /* Loop while the hardware is busy. */
        while (*busy)
        {
            /* Wait a little bit as not to consume the entire CPU resource. */
            gcmVERIFY_OK(gcoOS_Delay(os, 1));

#ifdef _DEBUG
			if ((++counter % 100) == 0)
			{
				gctUINT32 idle;

				/* Read idle register. */
				gcmVERIFY_OK(gcoHARDWARE_GetIdle(Command->kernel->hardware,
										     &idle));

				gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_COMMAND,
						   "gcoCOMMAND_Stall: Idle register = 0x%08X",
						   idle);
			}
#endif
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (acquired)
    {
        /* Release the command queue. */
        gcmVERIFY_OK(gcoCOMMAND_Release(Command));
    }

    /* Free the busy variable. */
    gcmVERIFY_OK(gcoOS_Free(os, (gctPOINTER) busy));

    /* Return status. */
    return status;
}
