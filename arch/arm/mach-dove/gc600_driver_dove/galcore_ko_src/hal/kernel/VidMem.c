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
******************************* Private Functions ******************************
\******************************************************************************/

/*******************************************************************************
**
**	_Split
**
**	Split a node on the required byte boundary.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gcuVIDMEM_NODE_PTR Node
**			Pointer to the node to split.
**
**		gctSIZE_T Bytes
**			Number of bytes to keep in the node.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		gctBOOL
**			gcvTRUE if the node was split successfully, or gcvFALSE if there is an
**			error.
**
*/
static gctBOOL
_Split(
	IN gcoOS Os,
	IN gcuVIDMEM_NODE_PTR Node,
	IN gctSIZE_T Bytes
	)
{
	gcuVIDMEM_NODE_PTR node;
	gceSTATUS status;

	/* Make sure the byte boundary makes sense. */
	if ((Bytes <= 0) || (Bytes > Node->VidMem.bytes))
	{
		return gcvFALSE;
	}

	/* Allocate a new gcuVIDMEM_NODE object. */
	status = gcoOS_Allocate(Os,
							gcmSIZEOF(gcuVIDMEM_NODE),
							(gctPOINTER *) &node);

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return gcvFALSE;
	}

	/* Initialize gcuVIDMEM_NODE structure. */
	node->VidMem.address   = Node->VidMem.address + Bytes;
	node->VidMem.bytes     = Node->VidMem.bytes   - Bytes;
	node->VidMem.alignment = 0;
	node->VidMem.locked    = 0;

	/* Insert node behind specified node. */
	node->VidMem.next = Node->VidMem.next;
	node->VidMem.prev = Node;
	Node->VidMem.next = node->VidMem.next->VidMem.prev = node;

	/* Insert free node behind specified node. */
	node->VidMem.nextFree = Node->VidMem.nextFree;
	node->VidMem.prevFree = Node;
	Node->VidMem.nextFree = node->VidMem.nextFree->VidMem.prevFree = node;

	/* Adjust size of specified node. */
	Node->VidMem.bytes = Bytes;

	/* Success. */
	return gcvTRUE;
}

/*******************************************************************************
**
**	_Merge
**
**	Merge two adjacent nodes together.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gcuVIDMEM_NODE_PTR Node
**			Pointer to the first of the two nodes to merge.
**
**	OUTPUT:
**
**		Nothing.
**
*/
static gceSTATUS
_Merge(
	IN gcoOS Os,
	IN gcuVIDMEM_NODE_PTR Node
	)
{
	gcuVIDMEM_NODE_PTR node;

	/* Save pointer to next node. */
	node = Node->VidMem.next;

	/* This is a good time to make sure the heap is not corrupted. */
	if (Node->VidMem.address + Node->VidMem.bytes != node->VidMem.address)
	{
		/* Corrupted heap. */
		gcmASSERT(
			Node->VidMem.address + Node->VidMem.bytes == node->VidMem.address);
		return gcvSTATUS_HEAP_CORRUPTED;
	}

	/* Adjust byte count. */
	Node->VidMem.bytes += node->VidMem.bytes;

	/* Unlink next node from linked list. */
	Node->VidMem.next     = node->VidMem.next;
	Node->VidMem.nextFree = node->VidMem.nextFree;

	Node->VidMem.next->VidMem.prev         =
	Node->VidMem.nextFree->VidMem.prevFree = Node;

	/* Free next node. */
	return gcoOS_Free(Os, node);
}

/******************************************************************************\
******************************* gcoVIDMEM API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**	gcoVIDMEM_ConstructVirtual
**
**	Construct a new gcuVIDMEM_NODE union for virtual memory.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gctSIZE_T Bytes
**			Number of byte to allocate.
**
**	OUTPUT:
**
**		gcuVIDMEM_NODE_PTR * Node
**			Pointer to a variable that receives the gcuVIDMEM_NODE union pointer.
*/
gceSTATUS
gcoVIDMEM_ConstructVirtual(
	IN gcoKERNEL Kernel,
	IN gctSIZE_T Bytes,
	OUT gcuVIDMEM_NODE_PTR * Node
	)
{
	gcoOS os;
	gceSTATUS status;
	gcuVIDMEM_NODE_PTR node;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(Bytes > 0);
	gcmVERIFY_ARGUMENT(Node != gcvNULL);

	/* Extract the gcoOS object pointer. */
	os = Kernel->os;
	gcmVERIFY_OBJECT(os, gcvOBJ_OS);

	/* Allocate an gcuVIDMEM_NODE union. */
	status = gcoOS_Allocate(os,
							gcmSIZEOF(gcuVIDMEM_NODE),
							(gctPOINTER *) &node);

	if (gcmIS_ERROR(status))
	{
		/* Error. */
		return status;
	}

	/* Initialize gcuVIDMEM_NODE union for virtual memory. */
	node->Virtual.kernel    = Kernel;
	node->Virtual.locked    = 0;
	node->Virtual.logical   = gcvNULL;
	node->Virtual.pageTable = gcvNULL;
	node->Virtual.pending   = gcvFALSE;

	/* Create the mutex. */
	status = gcoOS_CreateMutex(os, &node->Virtual.mutex);

	if (gcmIS_ERROR(status))
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoOS_Free(os, node));

		/* Error. */
		return status;
	}

	/* Allocate the virtual memory. */
	status = gcoOS_AllocatePagedMemory(os,
									  node->Virtual.bytes = Bytes,
									  &node->Virtual.physical);

	if (gcmIS_ERROR(status))
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoOS_DeleteMutex(os, node->Virtual.mutex));
		gcmVERIFY_OK(gcoOS_Free(os, node));

		/* Error. */
		return status;
	}

	/* Return pointer to the gcuVIDMEM_NODE union. */
	*Node = node;

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
				  "Created virtual node %p for %u bytes @ %p",
				  node,
				  Bytes,
				  node->Virtual.physical);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoVIDMEM_DestroyVirtual
**
**	Destroy an gcuVIDMEM_NODE union for virtual memory.
**
**	INPUT:
**
**		gcuVIDMEM_NODE_PTR Node
**			Pointer to a gcuVIDMEM_NODE union.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoVIDMEM_DestroyVirtual(
	IN gcuVIDMEM_NODE_PTR Node
	)
{
	gcoOS os;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Node->Virtual.kernel, gcvOBJ_KERNEL);

	/* Extact the gcoOS object pointer. */
	os = Node->Virtual.kernel->os;
	gcmVERIFY_OBJECT(os, gcvOBJ_OS);

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
				  "Destroying virtual node: %p",
				  Node);

	/* Delete the mutex. */
	gcmVERIFY_OK(gcoOS_DeleteMutex(os, Node->Virtual.mutex));

	if (Node->Virtual.pageTable != gcvNULL)
	{
		/* Free the pages. */
		gcmVERIFY_OK(gcoMMU_FreePages(Node->Virtual.kernel->mmu,
									  Node->Virtual.pageTable,
									  Node->Virtual.pageCount));

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
					  "Unmapped virtual memory 0x%08X",
					  Node->Virtual.address);
	}

	/* Delete the gcuVIDMEM_NODE union. */
	gcmVERIFY_OK(gcoOS_Free(os, Node));

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoVIDMEM_Construct
**
**	Construct a new gcoVIDMEM object.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctUINT32 BaseAddress
**			Base address for te video memory heap.
**
**		gctSIZE_T Bytes
**			Number of bytes ni the video memory heap.
**
**		gctSIZE_T Threshold
**			Minimum number of bytes beyond am allocation before the node is
**			split.  Can be used as a minimum alignment requirement.
**
**		gctSIZE_T BankSize
**			Number of bytes per physical memory bank.  Used by bank
**			optimization.
**
**	OUTPUT:
**
**		gcoVIDMEM * Memory
**			Pointer to a variable that will hold the pointer to the gcoVIDMEM
**			object.
*/
gceSTATUS
gcoVIDMEM_Construct(
	IN gcoOS Os,
	IN gctUINT32 BaseAddress,
	IN gctSIZE_T Bytes,
	IN gctSIZE_T Threshold,
	IN gctSIZE_T BankSize,
	OUT gcoVIDMEM * Memory
	)
{
	gcoVIDMEM memory = gcvNULL;
	gceSTATUS status;
	gcuVIDMEM_NODE_PTR node;
	gctINT i, banks = 0;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Bytes > 0);
	gcmVERIFY_ARGUMENT(Memory != gcvNULL);

	do
	{
		/* Allocate the gcoVIDMEM object. */
		gcmERR_BREAK(gcoOS_Allocate(Os,
									gcmSIZEOF(struct _gcoVIDMEM),
									(gctPOINTER *) &memory));

		/* Initialize the gcoVIDMEM object. */
		memory->object.type = gcvOBJ_VIDMEM;
		memory->os          = Os;

#if MRVL_TAVOR_PV2_PATCH
        BaseAddress = BaseAddress & 0x7fffffff;
#endif 

		/* Set video memory heap information. */
		memory->baseAddress = BaseAddress;
		memory->bytes       = memory->freeBytes = Bytes;
		memory->threshold   = Threshold;
		memory->mutex       = gcvNULL;

		/* Walk all possible banks. */
		for (i = 0; i < gcmCOUNTOF(memory->sentinel); ++i)
		{
			gctSIZE_T bytes;

			if (BankSize == 0)
			{
				/* Use all bytes for the first bank. */
				bytes = Bytes;
			}
			else
			{
				/* Compute number of bytes for this bank. */
				bytes = gcmALIGN(BaseAddress + 1, BankSize) - BaseAddress;

				if (bytes > Bytes)
				{
					/* Make sure we don't exceed the total number of bytes. */
					bytes = Bytes;
				}
			}

			if (bytes == 0)
			{
				/* Mark heap is not used. */
				memory->sentinel[i].VidMem.next     =
				memory->sentinel[i].VidMem.prev     =
				memory->sentinel[i].VidMem.nextFree =
				memory->sentinel[i].VidMem.prevFree = gcvNULL;
				continue;
			}

			/* Allocate one gcuVIDMEM_NODE union. */
			gcmERR_BREAK(gcoOS_Allocate(Os,
										gcmSIZEOF(gcuVIDMEM_NODE),
										(gctPOINTER *) &node));

			/* Initialize gcuVIDMEM_NODE union. */
			node->VidMem.memory    = memory;

			node->VidMem.next      =
			node->VidMem.prev      =
			node->VidMem.nextFree  =
			node->VidMem.prevFree  = &memory->sentinel[i];

			node->VidMem.address   = BaseAddress;
			node->VidMem.bytes     = bytes;
			node->VidMem.alignment = 0;

			node->VidMem.locked    = 0;

			/* Initialize the linked list of nodes. */
			memory->sentinel[i].VidMem.next     =
			memory->sentinel[i].VidMem.prev     =
			memory->sentinel[i].VidMem.nextFree =
			memory->sentinel[i].VidMem.prevFree = node;

			/* Mark sentinel. */
			memory->sentinel[i].VidMem.bytes = 0;

			/* Adjust address for next bank. */
			BaseAddress += bytes;
			Bytes       -= bytes;
			banks       ++;
		}

		/* Break on error.  */
		gcmERR_BREAK(status);

		/* Assign all the bank mappings. */
		memory->mapping[gcvSURF_RENDER_TARGET] = banks - 1;
		memory->mapping[gcvSURF_BITMAP] = banks - 1;
		if (banks > 1) --banks;
		memory->mapping[gcvSURF_DEPTH] = banks - 1;
		memory->mapping[gcvSURF_HIERARCHICAL_DEPTH] = banks - 1;
		if (banks > 1) --banks;
		memory->mapping[gcvSURF_TEXTURE] = banks - 1;
		if (banks > 1) --banks;
		memory->mapping[gcvSURF_VERTEX] = banks - 1;
		if (banks > 1) --banks;
		memory->mapping[gcvSURF_INDEX] = banks - 1;
		if (banks > 1) --banks;
		memory->mapping[gcvSURF_TILE_STATUS] = banks - 1;
		if (banks > 1) --banks;
		memory->mapping[gcvSURF_TYPE_UNKNOWN] = 0;

		gcmTRACE_ZONE(gcvZONE_DRIVER, gcvLEVEL_INFO,
		    	      "[GALCORE] INDEX:         bank %d\n",
			      memory->mapping[gcvSURF_INDEX]);
		gcmTRACE_ZONE(gcvZONE_DRIVER, gcvLEVEL_INFO,
		    	      "[GALCORE] VERTEX:        bank %d\n",
			      memory->mapping[gcvSURF_VERTEX]);
		gcmTRACE_ZONE(gcvZONE_DRIVER, gcvLEVEL_INFO,
		    	      "[GALCORE] TEXTURE:       bank %d\n",
			      memory->mapping[gcvSURF_TEXTURE]);
		gcmTRACE_ZONE(gcvZONE_DRIVER, gcvLEVEL_INFO,
		    	      "[GALCORE] RENDER_TARGET: bank %d\n",
			      memory->mapping[gcvSURF_RENDER_TARGET]);
		gcmTRACE_ZONE(gcvZONE_DRIVER, gcvLEVEL_INFO,
		    	      "[GALCORE] DEPTH:         bank %d\n",
			      memory->mapping[gcvSURF_DEPTH]);
		gcmTRACE_ZONE(gcvZONE_DRIVER, gcvLEVEL_INFO,
		    	      "[GALCORE] TILE_STATUS:   bank %d\n",
			      memory->mapping[gcvSURF_TILE_STATUS]);

	    /* Allocate the mutex. */
		gcmERR_BREAK(gcoOS_CreateMutex(Os, &memory->mutex));

		/* Return pointer to the gcoVIDMEM object. */
		*Memory = memory;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Error - roll back everything we just did. */
	if (memory != gcvNULL)
	{
		if (memory->mutex != gcvNULL)
		{
			/* Delete the mutex. */
			gcmVERIFY_OK(gcoOS_DeleteMutex(Os, memory->mutex));
		}

		for (i = 0; i < banks; ++i)
		{
			/* Free the heap. */
			gcmASSERT(memory->sentinel[i].VidMem.next != gcvNULL);
			gcmVERIFY_OK(gcoOS_Free(Os, memory->sentinel[i].VidMem.next));
		}

		/* Free the object. */
		gcoOS_Free(Os, memory);
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoVIDMEM_Destroy
**
**	Destroy an gcoVIDMEM object.
**
**	INPUT:
**
**		gcoVIDMEM Memory
**			Pointer to an gcoVIDMEM object to destroy.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoVIDMEM_Destroy(
	IN gcoVIDMEM Memory
	)
{
	gcuVIDMEM_NODE_PTR node, next;
	gceSTATUS status;
	gctINT i;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Memory, gcvOBJ_VIDMEM);

	/* Walk all sentinels. */
	for (i = 0; i < gcmCOUNTOF(Memory->sentinel); ++i)
	{
		/* Bail out of the heap is not used. */
		if (Memory->sentinel[i].VidMem.next == gcvNULL)
		{
			break;
		}

		/* Walk all the nodes until we reach the sentinel. */
		for (node = Memory->sentinel[i].VidMem.next;
			 node->VidMem.bytes != 0;
			 node = next)
		{
			/* Save pointer to the next node. */
			next = node->VidMem.next;

			/* Free the node. */
			gcmVERIFY_OK(gcoOS_Free(Memory->os, node));
		}
	}

    /* Free the mutex. */
    gcmVERIFY_OK(gcoOS_DeleteMutex(Memory->os, Memory->mutex));

	/* Mark the object as unknown. */
	Memory->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoVIDMEM object. */
	status = gcoOS_Free(Memory->os, Memory);

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoVIDMEM_Allocate
**
**	Allocate rectangular memory from the gcoVIDMEM object.
**
**	INPUT:
**
**		gcoVIDMEM Memory
**			Pointer to an gcoVIDMEM object.
**
**		gctUINT Width
**			Width of rectangle to allocate.  Make sure the width is properly
**			aligned.
**
**		gctUINT Height
**			Height of rectangle to allocate.  Make sure the height is properly
**			aligned.
**
**		gctUINT Depth
**			Depth of rectangle to allocate.  This equals to the number of
**			rectangles to allocate contiguously (i.e., for cubic maps and volume
**			textures).
**
**		gctUINT BytesPerPixel
**			Number of bytes per pixel.
**
**		gctUINT32 Alignment
**			Byte alignment for allocation.
**
**		gceSURF_TYPE Type
**			Type of surface to allocate (use by bank optimization).
**
**	OUTPUT:
**
**		gcuVIDMEM_NODE_PTR * Node
**			Pointer to a variable that will hold the allocated memory node.
*/
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
	)
{
	gctSIZE_T bytes;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Memory, gcvOBJ_VIDMEM);
	gcmVERIFY_ARGUMENT(Width > 0);
	gcmVERIFY_ARGUMENT(Height > 0);
	gcmVERIFY_ARGUMENT(Depth > 0);
	gcmVERIFY_ARGUMENT(BytesPerPixel > 0);
	gcmVERIFY_ARGUMENT(Node != gcvNULL);

	/* Compute linear size. */
	bytes = Width * Height * Depth * BytesPerPixel;

	/* Allocate through linear function. */
	return gcoVIDMEM_AllocateLinear(Memory, bytes, Alignment, Type, Node);
}

static gcuVIDMEM_NODE_PTR
_FindNode(
	IN gcoVIDMEM Memory,
	IN gctINT Bank,
	IN gctSIZE_T Bytes,
	IN OUT gctUINT32_PTR Alignment
	)
{
	gcuVIDMEM_NODE_PTR node;
	gctUINT32 alignment;

	/* Walk all free nodes until we have one that is big enough or we have
	   reached the sentinel. */
	for (node = Memory->sentinel[Bank].VidMem.nextFree;
		 node->VidMem.bytes != 0;
		 node = node->VidMem.nextFree)
	{
		/* Compute number of bytes to skip for alignment. */
		alignment = (*Alignment == 0)
				  ? 0
				  : (*Alignment - (node->VidMem.address % *Alignment));

		if (alignment == *Alignment)
		{
			/* Node is already aligned. */
			alignment = 0;
		}

		if (node->VidMem.bytes >= Bytes + alignment)
		{
			/* This node is big enough. */
			*Alignment = alignment;
			return node;
		}
	}

	/* Not enough memory. */
	return gcvNULL;
}

/*******************************************************************************
**
**	gcoVIDMEM_AllocateLinear
**
**	Allocate linear memory from the gcoVIDMEM object.
**
**	INPUT:
**
**		gcoVIDMEM Memory
**			Pointer to an gcoVIDMEM object.
**
**		gctSIZE_T Bytes
**			Number of bytes to allocate.
**
**		gctUINT32 Alignment
**			Byte alignment for allocation.
**
**		gceSURF_TYPE Type
**			Type of surface to allocate (use by bank optimization).
**
**	OUTPUT:
**
**		gcuVIDMEM_NODE_PTR * Node
**			Pointer to a variable that will hold the allocated memory node.
*/
gceSTATUS
gcoVIDMEM_AllocateLinear(
	IN gcoVIDMEM Memory,
	IN gctSIZE_T Bytes,
	IN gctUINT32 Alignment,
	IN gceSURF_TYPE Type,
	OUT gcuVIDMEM_NODE_PTR * Node
	)
{
    gceSTATUS status;
	gcuVIDMEM_NODE_PTR node;
	gctUINT32 alignment;
	gctINT bank, i;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Memory, gcvOBJ_VIDMEM);
	gcmVERIFY_ARGUMENT(Bytes > 0);
	gcmVERIFY_ARGUMENT(Node != gcvNULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
				  "gcoVIDMEM_AllocateLinear: Allocating %u bytes",
				  Bytes);

    /* Acquire the mutex. */
    status = gcoOS_AcquireMutex(Memory->os, Memory->mutex, gcvINFINITE);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    do
    {
    	if (Bytes > Memory->freeBytes)
	    {
		    /* Not enough memory. */
		    status = gcvSTATUS_OUT_OF_MEMORY;
            break;
    	}

		/* Find the default bank for this surface type. */
		gcmASSERT((gctINT) Type < gcmCOUNTOF(Memory->mapping));
		bank      = Memory->mapping[Type];
		alignment = Alignment;

		/* Find a free node in the default bank. */
		node = _FindNode(Memory, bank, Bytes, &alignment);

		/* Out of memory? */
		if (node == gcvNULL)
		{
			/* Walk all lower banks. */
			for (i = bank - 1; i >= 0; --i)
			{
				/* Find a free node inside the current bank. */
				node = _FindNode(Memory, i, Bytes, &alignment);
				if (node != gcvNULL)
				{
					break;
				}
			}
		}

		if (node == gcvNULL)
		{
			/* Walk all upper banks. */
			for (i = bank + 1; i < gcmCOUNTOF(Memory->sentinel); ++i)
			{
				if (Memory->sentinel[i].VidMem.nextFree == gcvNULL)
				{
					/* Abort when we reach unused banks. */
					break;
				}

				/* Find a free node inside the current bank. */
				node = _FindNode(Memory, i, Bytes, &alignment);
				if (node != gcvNULL)
				{
					break;
				}
			}
		}

		if (node == gcvNULL)
		{
			/* Out of memory. */
			status = gcvSTATUS_OUT_OF_MEMORY;
			break;
		}

	    /* Do we have an alignment? */
	    if (alignment > 0)
	    {
		    /* Split the node so it is aligned. */
		    if (_Split(Memory->os, node, alignment))
		    {
			    /* Successful split, move to aligned node. */
			    node = node->VidMem.next;

				/* Remove alignment. */
				alignment = 0;
		    }
	    }

	    /* Do we have enough memory after the allocation to split it? */
	    if (node->VidMem.bytes - Bytes > Memory->threshold)
	    {
		    /* Adjust the node size. */
		    _Split(Memory->os, node, Bytes);
	    }

	    /* Remove the node from the free list. */
	    node->VidMem.prevFree->VidMem.nextFree = node->VidMem.nextFree;
	    node->VidMem.nextFree->VidMem.prevFree = node->VidMem.prevFree;
	    node->VidMem.nextFree                  =
		node->VidMem.prevFree                  = gcvNULL;

	    /* Fill in the information. */
	    node->VidMem.alignment = alignment;
	    node->VidMem.memory    = Memory;

	    /* Adjust the number of free bytes. */
	    Memory->freeBytes -= node->VidMem.bytes;

	    /* Return the pointer to the node. */
    	*Node = node;

    	/* Success. */
	    status = gcvSTATUS_OK;

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
					  "gcoVIDMEM_AllocateLinear: Allocated %u bytes @ %p [0x%08X]",
					  node->VidMem.bytes,
					  node,
					  node->VidMem.address);
	}
    while (gcvFALSE);

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_VIDMEM,
					  "gcoVIDMEM_AllocateLinear: "
					  "Heap %p has not enough bytse (%u) for %u bytes",
					  Memory,
					  Memory->freeBytes,
					  Bytes);
	}

    /* Release the mutex. */
    gcmVERIFY_OK(gcoOS_ReleaseMutex(Memory->os, Memory->mutex));

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**	gcoVIDMEM_Free
**
**	Free an allocated video memory node.
**
**	INPUT:
**
**		gcuVIDMEM_NODE_PTR Node
**			Pointer to a gcuVIDMEM_NODE object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoVIDMEM_Free(
	IN gcuVIDMEM_NODE_PTR Node
	)
{
	gcoVIDMEM memory;
	gcuVIDMEM_NODE_PTR node;
	gceSTATUS status = gcvSTATUS_OK;

	/* Verify the arguments. */
	if ((Node == gcvNULL)
	||  (Node->VidMem.memory == gcvNULL)
	)
	{
		/* Invalid object. */
		return gcvSTATUS_INVALID_OBJECT;
	}

	/**************************** Video Memory ********************************/

	if (Node->VidMem.memory->object.type == gcvOBJ_VIDMEM)
	{
		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
					  "gcoVIDMEM_Free: Freeing node %p",
					  Node);

		if (Node->VidMem.locked > 0)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_VIDMEM,
						  "gcoVIDMEM_Free: Node %p is locked (%d)",
						  Node,
						  Node->VidMem.locked);

			/* Node is locked. */
			return gcvSTATUS_MEMORY_LOCKED;
		}

		/* Extract pointer to gcoVIDMEM object owning the node. */
		memory = Node->VidMem.memory;

		/* Acquire the mutex. */
		status = gcoOS_AcquireMutex(memory->os, memory->mutex, gcvINFINITE);

		if (gcmIS_ERROR(status))
		{
		    /* Error. */
		    return status;
		}

		do
		{
		    /* Update the number of free bytes. */
		    memory->freeBytes += Node->VidMem.bytes;

		    /* Find the next free node. */
		    for (node = Node->VidMem.next;
				 node->VidMem.nextFree == gcvNULL;
		    	 node = node->VidMem.next) ;

		    /* Insert this node in the free list. */
		    Node->VidMem.nextFree = node;
		    Node->VidMem.prevFree = node->VidMem.prevFree;

		    Node->VidMem.prevFree->VidMem.nextFree =
		    node->VidMem.prevFree                  = Node;

		    /* Is the next node a free node and not the sentinel? */
		    if ((Node->VidMem.next == Node->VidMem.nextFree)
			&&  (Node->VidMem.next->VidMem.bytes != 0)
			)
		    {
		        /* Merge this node with the next node. */
		        status = _Merge(memory->os, node = Node);
		        gcmASSERT(node->VidMem.nextFree != node);
		        gcmASSERT(node->VidMem.prevFree != node);

		        if (status != gcvSTATUS_OK)
		        {
			        /* Error. */
			        break;
		        }
		    }

		    /* Is the previous node a free node and not the sentinel? */
		    if ((Node->VidMem.prev == Node->VidMem.prevFree)
			&&  (Node->VidMem.prev->VidMem.bytes != 0)
			)
		    {
		        /* Merge this node with the previous node. */
		        status = _Merge(memory->os, node = Node->VidMem.prev);
		        gcmASSERT(node->VidMem.nextFree != node);
		        gcmASSERT(node->VidMem.prevFree != node);

		        if (status != gcvSTATUS_OK)
		        {
			        /* Error. */
			        break;
		        }
		    }

		    /* Success. */
		    status = gcvSTATUS_OK;
		}
		while (gcvFALSE);

		/* Release the mutex. */
		gcmVERIFY_OK(gcoOS_ReleaseMutex(memory->os, memory->mutex));
	}

	/*************************** Virtual Memory *******************************/

	else
	{
		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
					  "gcoVIDMEM_Free: Freeing virtual node %p",
					  Node);

		/* Verify the gcoKERNEL object pointer. */
		gcmVERIFY_OBJECT(Node->Virtual.kernel, gcvOBJ_KERNEL);

		if (!Node->Virtual.pending && Node->Virtual.locked > 0)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_VIDMEM,
						  "gcoVIDMEM_Free: Virtual node %p is locked (%d)",
						  Node,
						  Node->Virtual.locked);

			/* Node is locked. */
			return gcvSTATUS_MEMORY_LOCKED;
		}

		if (Node->Virtual.pending)
		{
			gctBOOL acquired;

			gcmASSERT(Node->Virtual.locked == 1);

			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
						  "gcoVIDMEM_Free: Scheduling node %p to be freed later",
						  Node);

			/* Schedule the node to be freed. */
			do
			{
				gctSIZE_T bufferSize, requested;
				gctPOINTER buffer;

				/* Command queue not yet acquired. */
				acquired = gcvFALSE;

				/* Get size of event.  */
				gcmERR_BREAK(gcoHARDWARE_Event(Node->Virtual.kernel->hardware,
											   gcvNULL,
											   0,
											   gcvKERNEL_PIXEL,
											   &requested));

				/* Reserve space in the command queue. */
				gcmERR_BREAK(gcoCOMMAND_Reserve(Node->Virtual.kernel->command,
												requested,
												&buffer,
												&bufferSize));

				/* Command queue is acquired. */
				acquired = gcvTRUE;

				/* Schedule the video memory to be freed again. */
				gcmERR_BREAK(
					gcoEVENT_FreeVideoMemory(Node->Virtual.kernel->event,
											 buffer,
											 &bufferSize,
											 Node,
											 gcvKERNEL_PIXEL,
											 gcvTRUE));

				/* Execute the command queue. */
				gcmERR_BREAK(gcoCOMMAND_Execute(Node->Virtual.kernel->command,
												requested));
			}
			while (gcvFALSE);

			if (gcmIS_ERROR(status) && acquired)
			{
				/* Release the command queue if acquired and there was an     *\
				\* error.                                                     */
				gcmVERIFY_OK(gcoCOMMAND_Release(Node->Virtual.kernel->command));
			}
		}
		else
		{
			/* Free the virtual memory. */
			gcmVERIFY_OK(gcoOS_FreePagedMemory(Node->Virtual.kernel->os,
											   Node->Virtual.physical,
											   Node->Virtual.bytes));

			/* Destroy the gcuVIDMEM_NODE union. */
			gcmVERIFY_OK(gcoVIDMEM_DestroyVirtual(Node));
		}
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoVIDMEM_Lock
**
**	Lock a video memory node and return it's hardware specific address.
**
**	INPUT:
**
**		gcuVIDMEM_NODE_PTR Node
**			Pointer to a gcuVIDMEM_NODE union.
**
**	OUTPUT:
**
**		gctUINT32 * Address
**			Pointer to avraibale that will hold the hardware spefific address.
*/
gceSTATUS
gcoVIDMEM_Lock(
	IN gcuVIDMEM_NODE_PTR Node,
	OUT gctUINT32 * Address
	)
{
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_ARGUMENT(Address != gcvNULL);

	if ((Node == gcvNULL)
	||  (Node->VidMem.memory == gcvNULL)
	)
	{
		/* Invalid object. */
		return gcvSTATUS_INVALID_OBJECT;
	}

	/**************************** Video Memory ********************************/

	if (Node->VidMem.memory->object.type == gcvOBJ_VIDMEM)
	{
		/* Increment the lock count. */
		Node->VidMem.locked ++;

		/* Return the address of the node. */
		*Address = Node->VidMem.address + Node->VidMem.alignment;

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
					  "gcoVIDMEM_Lock: Locked node %p (%d) @ 0x%08X",
					  Node,
					  Node->VidMem.locked,
					  *Address);
	}

	/*************************** Virtual Memory *******************************/

	else
	{
		gcoOS os;

		/* Verify the gcoKERNEL object pointer. */
		gcmVERIFY_OBJECT(Node->Virtual.kernel, gcvOBJ_KERNEL);

		/* Extract the gcoOS object pointer. */
		os = Node->Virtual.kernel->os;
		gcmVERIFY_OBJECT(os, gcvOBJ_OS);

		/* Grab the mutex. */
		status = gcoOS_AcquireMutex(os, Node->Virtual.mutex, gcvINFINITE);

		if (gcmIS_ERROR(status))
		{
			/* Error. */
			return status;
		}

		do
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
						  "gcoVIDMEM_Lock: Locking virtual node %p (%d) @ 0x%08X",
						  Node,
						  Node->Virtual.locked);

			/* Increment the lock count. */
			if (Node->Virtual.locked ++ == 0)
			{
				/* Is this node pending for a final unlock? */
				if (Node->Virtual.pending)
				{
					/* Make sure we have a page table. */
					gcmASSERT(Node->Virtual.pageTable != gcvNULL);

					/* Remove pending unlock. */
					Node->Virtual.pending = gcvFALSE;

					/* Success. */
					status = gcvSTATUS_OK;
					break;
				}

				/* First lock - create a page table. */
				gcmASSERT(Node->Virtual.pageTable == gcvNULL);

				/* Make sure we mark our node as not flushed. */
				Node->Virtual.pending = gcvFALSE;

				/* Lock the allocated pages. */
				status = gcoOS_LockPages(os,
									     Node->Virtual.physical,
									     Node->Virtual.bytes,
									     &Node->Virtual.logical,
									     &Node->Virtual.pageCount);

				if (gcmIS_ERROR(status))
				{
					/* Error. */
					break;
				}

				/* Allocate pages inside the MMU. */
				status = gcoMMU_AllocatePages(Node->Virtual.kernel->mmu,
											  Node->Virtual.pageCount,
											  &Node->Virtual.pageTable,
											  &Node->Virtual.address);

				if (gcmIS_ERROR(status))
				{
					/* Roll back. */
					gcmVERIFY_OK(gcoOS_UnlockPages(os,
												   Node->Virtual.physical,
												   Node->Virtual.bytes,
												   Node->Virtual.logical));

					/* Error. */
					break;
				}

				/* Map the pages. */
				status = gcoOS_MapPages(os,
										Node->Virtual.physical,
									    Node->Virtual.pageCount,
									    Node->Virtual.pageTable);

				if (gcmIS_ERROR(status))
				{
					/* Roll back. */
					gcmVERIFY_OK(gcoMMU_FreePages(Node->Virtual.kernel->mmu,
												  Node->Virtual.pageTable,
												  Node->Virtual.pageCount));

					Node->Virtual.pageTable = gcvNULL;

					gcmVERIFY_OK(gcoOS_UnlockPages(os,
												   Node->Virtual.physical,
												   Node->Virtual.bytes,
												   Node->Virtual.logical));

					/* Error. */
					break;
				}

				gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
							  "gcoVIDMEM_Lock: Mapped virtual node %p to 0x%08X",
							  Node,
							  Node->Virtual.address);
			}

			/* Return hardware address. */
			*Address = Node->Virtual.address;
		}
		while (gcvFALSE);

		/* Release the mutex. */
		gcmVERIFY_OK(gcoOS_ReleaseMutex(os, Node->Virtual.mutex));

		if (gcmIS_ERROR(status))
		{
			/* Error. */
			return status;
		}
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoVIDMEM_Unlock
**
**	Unlock a video memory node.
**
**	INPUT:
**
**		gcuVIDMEM_NODE_PTR Node
**			Pointer to a locked gcuVIDMEM_NODE union.
**
**		gceSURF_TYPE Type
**			Type of surface to unlock.
**
**		gctSIZE_T * CommandSize
**			Pointer to a variable specifiying the number of bytes in the command
**			buffer specified by 'Commands'.  If gcvNULL, there is no command buffer
**			and the video memory shoud be unlocked synchronously.
**
**		gctBOOL * Asynchroneous
**			Pointer to a variable specifiying whether the surface should be
**			unlocked asynchroneously or not.
**
**	OUTPUT:
**
**		gctSIZE_T * CommandSize
**			Pointer to a variable receiving the number of byets used in the
**			command buffer specified by 'Commands'.  If gcvNULL, there is no
**			command buffer.
**
**		gctPOINTER Commands
**			Pointer to a command buffer that will be filled with any
**			asynchronous commands required to unlock the video memory.
**
**		gctBOOL * Asynchroneous
**			Pointer to a variable receiving the number of bytes used in the
**			command buffer specified by 'Commands'.  If gcvNULL, there is no
**			command buffer.
*/
gceSTATUS
gcoVIDMEM_Unlock(
	IN gcuVIDMEM_NODE_PTR Node,
	IN gceSURF_TYPE Type,
#if USE_EVENT_QUEUE
	IN OUT gctBOOL * Asynchroneous
#else
	IN OUT gctSIZE_T * CommandSize,
	OUT gctPOINTER Commands
#endif
	)
{
	gceSTATUS status;
	gcoKERNEL kernel;
	gcoHARDWARE hardware;
	gctPOINTER buffer;
	gctSIZE_T requested, bufferSize, size;
	gcoCOMMAND command;
	gceKERNEL_FLUSH flush;

	/* Verify the arguments. */
	if ( (Node == gcvNULL) || (Node->VidMem.memory == gcvNULL) )
	{
		/* Invalid object. */
		return gcvSTATUS_INVALID_OBJECT;
	}

	/**************************** Video Memory ********************************/

	if (Node->VidMem.memory->object.type == gcvOBJ_VIDMEM)
	{
		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
					  "gcoVIDMEM_Unlock: Unlocking node %p (%d)",
					  Node,
					  Node->VidMem.locked);

		if (Node->VidMem.locked <= 0)
		{
			/* The surface was not locked. */
			return gcvSTATUS_MEMORY_UNLOCKED;
		}

		/* Decrement the lock count. */
		Node->VidMem.locked --;

#if USE_EVENT_QUEUE
		if (Asynchroneous != gcvNULL)
		{
			/* No need for any events. */
			*Asynchroneous = gcvFALSE;
		}
#else
		if (CommandSize != gcvNULL)
		{
			/* No need for any commands. */
			*CommandSize = 0;
		}
#endif
	}

	/*************************** Virtual Memory *******************************/

	else
	{
		/* Verify the gcoKERNEL object pointer. */
		kernel = Node->Virtual.kernel;
		gcmVERIFY_OBJECT(kernel, gcvOBJ_KERNEL);

		/* Verify the gcoHARDWARE object pointer. */
		hardware = Node->Virtual.kernel->hardware;
		gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

		/* Verify the gcoCOMMAND object pointer. */
		command = Node->Virtual.kernel->command;
		gcmVERIFY_OBJECT(command, gcvOBJ_COMMAND);

#if USE_EVENT_QUEUE
		if (Asynchroneous == gcvNULL)
#else
		if (CommandSize == gcvNULL)
#endif
		{
			gcoOS os;

			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
						   "gcoVIDMEM_Unlock: Unlocking virtual node %p (%d)",
						   Node,
						   Node->Virtual.locked);

			/* Get the gcoOS object pointer. */
			os = Node->Virtual.kernel->os;
			gcmVERIFY_OBJECT(os, gcvOBJ_OS);

			/* Grab the mutex. */
			status = gcoOS_AcquireMutex(os, Node->Virtual.mutex, gcvINFINITE);

			if (gcmIS_ERROR(status))
			{
				/* Error. */
				return status;
			}

			do
			{
				/* If we need to unlock a node from virtual memory we have to be
				   very carefull.  If the node is still inside the caches we
				   might get a bus error later if the cache line needs to be
				   replaced.  So - we have to flush the caches before we do
				   anything.  We also need to stall to make sure the flush has
				   happened.  However - when we get to this point we are inside
				   the interrupt handler and we cannot just gcoCOMMAND_Wait
				   because it will wait forever.  So - what we do here is we
				   verify the type of the surface, flush the appropriate cache,
				   mark the node as flushed, and issue another unlock to unmap
				   the MMU. */
				if ((Node->Virtual.locked == 1) && !Node->Virtual.pending)
				{
					if (Type == gcvSURF_BITMAP)
					{
						/* Flush 2D cache. */
						flush = gcvFLUSH_2D;
					}
					else if (Type == gcvSURF_RENDER_TARGET)
					{
						/* Flush color cache. */
						flush = gcvFLUSH_COLOR;
					}
					else if (Type == gcvSURF_DEPTH)
					{
						/* Flush depth cache. */
						flush = gcvFLUSH_DEPTH;
					}
					else
					{
						/* No flush required. */
						flush = 0;
					}

					gcmERR_BREAK(gcoHARDWARE_Flush(hardware,
												   flush,
												   gcvNULL,
												   &requested));

					if (requested != 0)
					{
						gcmERR_BREAK(gcoHARDWARE_Event(hardware,
													   gcvNULL,
													   0,
													   gcvKERNEL_PIXEL,
													   &bufferSize));

						requested += bufferSize;

						gcmERR_BREAK(gcoCOMMAND_Reserve(command,
														requested,
														&buffer,
														&bufferSize));

						size = bufferSize;

						gcmERR_BREAK(gcoHARDWARE_Flush(hardware,
													   flush,
													   buffer,
													   &size));

						bufferSize -= size;

						gcmERR_BREAK(
							gcoEVENT_Unlock(Node->Virtual.kernel->event,
											(gctUINT8_PTR) buffer + size,
											&bufferSize,
											gcvKERNEL_PIXEL,
											Node,
											Type,
											gcvTRUE));

						/* Mark node as pending. */
						Node->Virtual.pending = gcvTRUE;

						gcmERR_BREAK(gcoCOMMAND_Execute(command, requested));

						break;
					}
				}

				/* Decrement lock count. */
				-- Node->Virtual.locked;

				/* See if we can unlock the resources. */
				if (Node->Virtual.locked == 0)
				{
					/* Unlock the pages. */
					gcmERR_BREAK(gcoOS_UnlockPages(os,
												   Node->Virtual.physical,
												   Node->Virtual.bytes,
												   Node->Virtual.logical));

					/* Free the page table. */
					if (Node->Virtual.pageTable != gcvNULL)
					{
						gcmERR_BREAK(gcoMMU_FreePages(Node->Virtual.kernel->mmu,
													  Node->Virtual.pageTable,
													  Node->Virtual.pageCount));

						/* Mark page table as freed. */
 						Node->Virtual.pageTable = gcvNULL;
					}

					/* Mark node as unlocked. */
					Node->Virtual.pending = gcvFALSE;

					gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
								  "gcoVIDMEM_Unlock: "
								  "Unmapped virtual node %p from 0x%08X",
								  Node,
								  Node->Virtual.address);
				}
			}
			while (gcvFALSE);

			/* Release the mutex. */
			gcmVERIFY_OK(gcoOS_ReleaseMutex(os, Node->Virtual.mutex));

			if (gcmIS_ERROR(status))
			{
				/* Error. */
				return status;
			}
		}

		else
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VIDMEM,
						  "gcoVIDMEM_Unlock: Scheduled unlock for virtual node %p",
						  Node);

#if USE_EVENT_QUEUE
			/* Schedule the surface to be unlocked. */
			*Asynchroneous = gcvTRUE;
#else
			/* Verify the arguments. */
			gcmVERIFY_ARGUMENT(Commands != gcvNULL);

			/* Schedule the surface to be unlocked. */
			status = gcoEVENT_Unlock(Node->Virtual.kernel->event,
									 Commands,
									 CommandSize,
									 gcvKERNEL_PIXEL,
									 Node,
									 Type,
									 gcvFALSE);

			if (gcmIS_ERROR(status))
			{
				/* Error. */
				return status;
			}
#endif
		}
	}

	/* Success. */
	return gcvSTATUS_OK;
}
