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

#ifdef _DEBUG
#	define VALIDATE_HEAP
#endif

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

typedef struct _gcsHEAPARENA *	gcsHEAPARENA_PTR;
typedef union _gcsHEAPNODE *	gcsHEAPNODE_PTR;

union _gcsHEAPNODE
{
	/* Pointer to next free node. */
	gcsHEAPNODE_PTR				nextFree;

	/* Owner of a used node. */
	gcsHEAPARENA_PTR			owner;
};

struct _gcsHEAPARENA
{
	/* Linked list pointers. */
	gcsHEAPARENA_PTR			next;
	gcsHEAPARENA_PTR			prev;

	/* Physical allocation. */
	gctPHYS_ADDR				physical;
	gctSIZE_T					allocationSize;

	/* Allocation range. */
	gctSIZE_T					minAllocationSize;
	gctSIZE_T					maxAllocationSize;

	/* Allocation data. */
	gctSIZE_T					nodeSize;
	gcsHEAPNODE_PTR				firstNode;
	gcsHEAPNODE_PTR				lastNode;

	/* Pointer to the first free node. */
	gcsHEAPNODE_PTR				freeNode;
};

struct _gcoHEAP
{
	/* Object. */
	gcsOBJECT					object;

	/* Pointer to a gcoOS object. */
	gcoOS						os;

	/* Locking mutex. */
	gctPOINTER					mutex;

	/* Allocation parameters. */
	gctSIZE_T					allocationSize;
	gctSIZE_T					wasteThreshold;

	/* Total number of bytes allocated. */
	gctSIZE_T					allocated;

	/* Sentinel of linked list of arenas. */
	struct _gcsHEAPARENA		sentinel;
};

static gceSTATUS
_ValidateHeap(
	IN gcoHEAP Heap
	)
{
	gcsHEAPARENA_PTR arena;
	gcsHEAPNODE_PTR node, freeNode;
	gctSIZE_T allocated = 0;

	/* Walk all arenas. */
	for (arena = Heap->sentinel.next;
		 arena != &Heap->sentinel;
		 arena = arena->next)
	{
		gctSIZE_T bytes;

		allocated += arena->allocationSize;

		if (arena->freeNode != gcvNULL)
		{
			if ((arena->freeNode <  arena->firstNode)
			||  (arena->freeNode >= arena->lastNode)
			)
			{
				gcmFATAL("Arena %p corrupted free list",
						 arena);

				return gcvSTATUS_HEAP_CORRUPTED;
			}

			for (node = arena->freeNode; node != gcvNULL; node = node->nextFree)
			{
				if (node->nextFree != gcvNULL)
				{
					if ((node->nextFree <  arena->firstNode)
					||  (node->nextFree >= arena->lastNode)
					)
					{
						gcmFATAL("Arena %p corrupted free list at %p",
								 arena,
								 node);

						return gcvSTATUS_HEAP_CORRUPTED;
					}
				}
			}
		}

		bytes = arena->nodeSize;

		for (node = arena->firstNode;
			 node < arena->lastNode;
			 node = (gcsHEAPNODE_PTR) ((gctUINT8 *) node + bytes))
		{
			for (freeNode = arena->freeNode;
				 freeNode != gcvNULL;
				 freeNode = freeNode->nextFree)
			{
				if (freeNode == node)
				{
					break;
				}
			}

			if (node->owner == arena)
			{
				if (freeNode != gcvNULL)
				{
					gcmFATAL("Arena %p expected used node at %p, but it is "
							 "part of the free list %p",
							 arena,
							 node,
							 freeNode);

					return gcvSTATUS_HEAP_CORRUPTED;
				}
			}
			else
			{
				if (freeNode == gcvNULL)
				{
					gcmFATAL("Arena %p expected free node at %p",
							 arena,
							 node);

					return gcvSTATUS_HEAP_CORRUPTED;
				}
			}
		}
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************\
******************************** gcoHEAP API Code *******************************
\******************************************************************************/

/*******************************************************************************
**
**	gcoHEAP_Construct
**
**	Construct a new gcoHEAP object.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to a gcoOS object.
**
**		gctSIZE_T AllocationSize
**			Minimum size per arena.
**
**		gctSIZE_T WasteThreshold
**			Number of bytes per allocation that can be wasted before moving to a
**			next arena.
**
**	OUTPUT:
**
**		gcoHEAP * Heap
**			Pointer to a variable that will hold the pointer to the gcoHEAP
**			object.
*/
gceSTATUS
gcoHEAP_Construct(
	IN gcoOS Os,
	IN gctSIZE_T AllocationSize,
	IN gctSIZE_T WasteThreshold,
	OUT gcoHEAP * Heap
	)
{
	gceSTATUS status;
	gcoHEAP heap = gcvNULL;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Heap != gcvNULL);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_HEAP,
				  "gcoHEAP_Construct: AllocationSize=%u WasteThreshold=%u",
				  AllocationSize, WasteThreshold);

	do
	{
		/* Allocate the gcoHEAP object. */
		gcmERR_BREAK(gcoOS_Allocate(Os,
									gcmSIZEOF(struct _gcoHEAP),
									(gctPOINTER *) &heap));

		/* Initialize the gcoHEAP object. */
		heap->object.type = gcvOBJ_HEAP;
		heap->os = Os;

		/* Create the mutex. */
		gcmERR_BREAK(gcoOS_CreateMutex(Os, &heap->mutex));

		/* Set heap parameters. */
		heap->allocationSize = AllocationSize;
		heap->wasteThreshold = WasteThreshold;
		heap->allocated = 0;

		/* Initialize the linked list of arenas. */
		heap->sentinel.next = heap->sentinel.prev = &heap->sentinel;

		/* Initialize the sentinel. */
		heap->sentinel.minAllocationSize = heap->sentinel.maxAllocationSize = 0;
		heap->sentinel.firstNode = heap->sentinel.lastNode = gcvNULL;

		/* Return the pointer to the gcoHEAP object. */
		*Heap = heap;

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
					  "gcoHEAP_Construct: Heap=%p",
					  heap);

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
				  "gcoHEAP_Construct: ERROR: Got status %d",
				  status);

	if (heap != gcvNULL)
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoOS_Free(Os, heap));
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoHEAP_Destroy
**
**	Destroy a gcoHEAP object.
**
**	INPUT:
**
**		gcoHEAP Heap
**			Pointer to a gcoHEAP object to destroy.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoHEAP_Destroy(
	IN gcoHEAP Heap
	)
{
	gcsHEAPARENA_PTR arena, next;
	gceSTATUS status = gcvSTATUS_OK;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_HEAP,
				  "gcoHEAP_Destroy: Heap=%p",
				  Heap);

	do
	{
		/* Walk all arenas until we reach the sentinel. */
		for (arena = Heap->sentinel.next;
			 arena->maxAllocationSize > 0;
			 arena = next)
		{
			/* Save pointer to next arena. */
			next = arena->next;

			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
						  "gcoHEAP_Destroy: Freeing arena %p",
						  arena);

			/* Free the arena. */
			gcmERR_BREAK(gcoOS_FreeNonPagedMemory(Heap->os,
												  arena->allocationSize,
												  arena->physical, 
												  arena));
		}

		if (gcmIS_ERROR(status))
		{
			break;
		}

		/* Free the mutex. */
		gcmERR_BREAK(gcoOS_DeleteMutex(Heap->os, Heap->mutex));

		/* Mark object as unknown. */
		Heap->object.type = gcvOBJ_UNKNOWN;

		/* Free the gcoHEAP object. */
		gcmERR_BREAK(gcoOS_Free(Heap->os, Heap));

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
				  "gcoHEAP_Destroy: ERROR: Got status %d",
				  status);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoHEAP_Allocate
**
**	Allocate data from the heap.
**
**	INPUT:
**
**		gcoHEAP Heap
**			Pointer to a gcoHEAP object.
**
**		IN gctSIZE_T Bytes
**			Number of byte to allocate.
**
**	OUTPUT:
**
**		gctPOINTER * Node
**			Pointer to a variable that will hold the address of the allocated
**			memory.
*/
gceSTATUS
gcoHEAP_Allocate(
	IN gcoHEAP Heap,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Node
	)
{
	gctSIZE_T bytes;
	gcsHEAPARENA_PTR arena;
	gcsHEAPNODE_PTR node;
	gctSIZE_T allocationSize, nodeCount;
	gceSTATUS status;
	gctPHYS_ADDR physical;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);
	gcmVERIFY_ARGUMENT(Bytes > 0);
	gcmVERIFY_ARGUMENT(Node != gcvNULL);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_HEAP,
				  "gcoHEAP_Allocate: %u bytes",
				  Bytes);

	/* Acquire te mutex to the heap. */
	status = gcoOS_AcquireMutex(Heap->os, Heap->mutex, gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
					  "gcoHEAP_Allocate: ERROR: Got status %d",
					  status);

		return status;
	}

	do
	{
#ifdef VALIDATE_HEAP
		/* Validate the heap. */
		gcmERR_BREAK(_ValidateHeap(Heap));
#endif

		/* Align byte count to 4-byte integrals. */
		bytes = gcmALIGN(Bytes, 4);

		/* Find an arena that fits the requested allocation. */
		for (arena = Heap->sentinel.next;
			 arena->firstNode != gcvNULL;
			 arena = arena->next)
		{
			/* Does the current arena fits the allocation? */
			if ((Bytes >= arena->minAllocationSize)
			&&  (Bytes <= arena->maxAllocationSize)
			/* And is there any space available?. */
			&&  (arena->freeNode != gcvNULL)
			)
			{
				goto MoveToFront;
			}
		}

		/* Start with the default allocation size.*/
		allocationSize = Heap->allocationSize;

		do
		{
			/* Compute the number of nodes that fit in the arena. */
			nodeCount = (allocationSize - gcmSIZEOF(struct _gcsHEAPARENA))
					  / (bytes + gcmSIZEOF(union _gcsHEAPNODE));

			if (nodeCount == 0)
			{
				/* If the arena is too small, add another chunk of memory. */
				allocationSize += Heap->allocationSize;
			}
		}
		while (nodeCount == 0);

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
					  "gcoHEAP_Allocate: Creating a new %u-byte arena",
					  allocationSize);

		/* Allocate a new arena. */
		gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(Heap->os, 
												  gcvFALSE, 
												  &allocationSize,
												  &physical, 
												  (gctPOINTER *) &arena));

		/* Compute the number of nodes that fit in the allocated arena. */
		nodeCount = (allocationSize - gcmSIZEOF(struct _gcsHEAPARENA))
				  / (bytes + gcmSIZEOF(union _gcsHEAPNODE));

		/* Adjust heap allocation size. */
		Heap->allocated += allocationSize;

		if (nodeCount == 1)
		{
			/* Insert arena at the tail of the linked list. */
			arena->next = &Heap->sentinel;
			arena->prev = Heap->sentinel.prev;
		}
		else
		{
			/* Insert arena at the head of the linked list. */
			arena->next = Heap->sentinel.next;
			arena->prev = &Heap->sentinel;
		}

		arena->prev->next = arena->next->prev = arena;

		/* Initialize arena. */
		arena->allocationSize = allocationSize;
		arena->physical = physical;

		/* Compute range of allocation requests for this arena. */
		arena->minAllocationSize = (bytes > Heap->wasteThreshold)
			? bytes - Heap->wasteThreshold
			: 0;
		arena->maxAllocationSize = bytes;

		/* Initialize node pointers. */
		arena->nodeSize = sizeof(union _gcsHEAPNODE) + bytes;
		arena->firstNode = (gcsHEAPNODE_PTR) (arena + 1);
		arena->lastNode = (gcsHEAPNODE_PTR)
			((gctUINT8*) arena->firstNode + nodeCount * arena->nodeSize);

		/* Add all nodes to the free list. */
		arena->freeNode = arena->firstNode;
		for (node = arena->firstNode; node != gcvNULL; node = node->nextFree)
		{
			/* Save pointer to next free node. */
			node->nextFree = (gcsHEAPNODE_PTR) ((gctUINT8*) node + arena->nodeSize);

			if (node->nextFree == arena->lastNode)
			{
				/* At end of list. */
				node->nextFree = gcvNULL;
			}
		}

		goto UseNode;

MoveToFront:
		/* Move arena to head of linked list. */
		if (arena->prev->firstNode != gcvNULL)
		{
			/* Unlink the arena from the linked list. */
			arena->prev->next = arena->next;
			arena->next->prev = arena->prev;

			/* Insert the arena at the head of the linked list. */
			arena->next = Heap->sentinel.next;
			arena->prev = &Heap->sentinel;

			arena->prev->next = arena->next->prev = arena;
		}

UseNode:
		/* Use first node in the free list and remove it from the free list. */
		node = arena->freeNode;
		arena->freeNode = node->nextFree;

		/* Save owner of node. */
		node->owner = arena;

		/* Return pointer to memory. */
		*Node = (gctPOINTER) (node + 1);

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
					  "gcoHEAP_Allocate: Using arena %p, node %p -> address %p",
					  arena,
					  node,
					  *Node);

#ifdef VALIDATE_HEAP
		/* Validate the heap. */
		gcmERR_BREAK(_ValidateHeap(Heap));
#endif
	}
	while (gcvFALSE);

	/* Release the heap mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Heap->os, Heap->mutex));

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
					  "gcoHEAP_Allocate: ERROR: Got error %d",
					  status);
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoHEAP_Free
**
**	Free allocated memory from the heap.
**
**	INPUT:
**
**		gcoHEAP Heap
**			Pointer to a gcoHEAP object.
**
**		IN gctPOINTER Node
**			Number of byte to allocate.
**
**	OUTPUT:
**
**		gctPOINTER * Node
**			Pointer to a variable that will hold the address of the allocated
**			memory.
*/
gceSTATUS
gcoHEAP_Free(
	IN gcoHEAP Heap,
	IN gctPOINTER Node
	)
{
	gcsHEAPNODE_PTR node;
	gcsHEAPARENA_PTR arena;
	gceSTATUS status;

	/* Verify arguments. */
	gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);
	gcmVERIFY_ARGUMENT(Node != gcvNULL);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_HEAP,
				  "gcoHEAP_Free: Node=%p",
				  Node);

	/* Acquire te mutex to the heap. */
	status = gcoOS_AcquireMutex(Heap->os, Heap->mutex, gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
					  "gcoHEAP_Free: ERROR: Got status %d",
					  status);

		return status;
	}

	do
	{
#ifdef VALIDATE_HEAP
		/* Validate the heap. */
		gcmERR_BREAK(_ValidateHeap(Heap));
#endif

		/* Get pointer to gcsHEAPNODE_PTR. */
		node = (gcsHEAPNODE_PTR) Node - 1;

		/* Get pointer to gcsHEAPARENA_PTR. */
		arena = node->owner;

		/* Verify the node is still vald. */
		if ((node < arena->firstNode) || (node >= arena->lastNode))
		{
			gcmASSERT((node >= arena->firstNode) && (node < arena->lastNode));

			/* Corrupted heap. */
			status = gcvSTATUS_HEAP_CORRUPTED;
		}
		else
		{
			/* Insert node at head of free list. */
			node->nextFree = arena->freeNode;
			arena->freeNode = node;

			/* Success. */
			status = gcvSTATUS_OK;
		}
	}
	while (gcvFALSE);

	/* Release the heap mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Heap->os, Heap->mutex));

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
					  "gcoHEAP_Free: ERROR: Got error %d",
					  status);
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoHEAP_Compact
**
**	Compact the entire heap by freeing up any empty arenas.
**
**	INPUT:
**
**		gcoHEAP Heap
**			Pointer to a gcoHEAP object to compact.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoHEAP_Compact(
	IN gcoHEAP Heap
	)
{
	gcsHEAPARENA_PTR arena, next;
	gctBOOL freeArena;
	gcsHEAPNODE_PTR node;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_HEAP,
				  "gcoHEAP_Compact");

	/* Acquire te mutex to the heap. */
	status = gcoOS_AcquireMutex(Heap->os, Heap->mutex, gcvINFINITE);

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
					  "gcoHEAP_Free: ERROR: Got status %d",
					  status);

		return status;
	}

	do
	{
		/* Validate the heap. */
		gcmERR_BREAK(_ValidateHeap(Heap));

		/* Walk all arenas until we reached the sentinel. */
		for (arena = Heap->sentinel.next;
			 arena->firstNode != gcvNULL;
			 arena = next)
		{
			/* Save pointer to next arena. */
			next = arena->next;

			/* Assume arena is completely free. */
			freeArena = gcvTRUE;

			/* Walk all nodes in arena. */
			for (node = arena->firstNode;
				 node < arena->lastNode;
				 node = (gcsHEAPNODE_PTR) ((gctUINT8 *) node + arena->nodeSize))
			{
				/* If the node points to the arena, it is in use. */
				if (node->owner == arena)
				{
					/* The arena has used nodes. */
					freeArena = gcvFALSE;
					break;
				}
			}

			/* Destroy arena if completely free. */
			if (freeArena)
			{
				/* Remove arena from linked list.*/
				arena->prev->next = arena->next;
				arena->next->prev = arena->prev;

				/* Adjust allocated heap memory. */
				Heap->allocated -= arena->allocationSize;

				/* Free arena. */
				gcmERR_BREAK(gcoOS_FreeNonPagedMemory(Heap->os,
													  arena->allocationSize,
													  arena->physical,
													  arena));
			}
		}
	}
	while (gcvFALSE);

	/* Release the heap mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Heap->os, Heap->mutex));

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
					  "gcoHEAP_Compact: ERROR: Got error %d",
					  status);
	}

	/* Return the status. */
	return status;
}
