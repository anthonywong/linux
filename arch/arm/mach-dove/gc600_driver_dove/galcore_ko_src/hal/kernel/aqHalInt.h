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






#ifndef __aqhalint_h_
#define __aqhalint_h_

#include "aqHal.h"
#include "aqHardware.h"
#include "aqHalDriver.h"

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

/* gcoKERNEL object. */
struct _gcoKERNEL
{
	/* Object. */
	gcsOBJECT					object;

	/* Pointer to gcoOS object. */
	gcoOS						os;

	/* Pointer to gcoHARDWARE object. */
	gcoHARDWARE					hardware;

	/* Pointer to gcoCOMMAND object. */
	gcoCOMMAND					command;

	/* Pointer to gcoEVENT object. */
	gcoEVENT					event;

	/* Pointer to context. */
	gctPOINTER					context;

	/* Pointer to gcoMMU object. */
	gcoMMU						mmu;

	/* Require to notify idle status */
	gctBOOL						notifyIdle;

    /* Pointer to reversion string */
    gctCONST_STRING             version;

        
};

/* gcoCOMMAND object. */
struct _gcoCOMMAND
{
	/* Object. */
	gcsOBJECT					object;

	/* Pointer to required object. */
	gcoKERNEL					kernel;
	gcoOS						os;

	/* Number of bytes per page. */
	gctSIZE_T					pageSize;

	/* Current pipe select. */
	gctUINT32					pipeSelect;

	/* Command queue running flag. */
	gctBOOL						running;

	/* Idle flag and commit stamp. */
	gctBOOL						idle;
	gctUINT64					commitStamp;

	/* Command queue mutex. */
	gctPOINTER					mutexQueue;

	/* Context switching mutex. */
	gctPOINTER					mutexContext;

	/* Current command queue. */
	gctPHYS_ADDR				physical;
	gctPOINTER					logical;
	gctUINT32					offset;

	/* The command queue is new. */
	gctBOOL						newQueue;

	/* Context counter used for unique ID. */
	gctUINT64					contextCounter;

	/* Current context ID. */
	gctUINT64					currentContext;

	/* Pointer to last WAIT command. */
	gctPOINTER					wait;
	gctSIZE_T					waitSize;

	/* Command buffer alignment. */
	gctSIZE_T					alignment;
	gctSIZE_T					reservedHead;
	gctSIZE_T					reservedTail;
};

typedef struct _gcsEVENT *		gcsEVENT_PTR;

typedef struct _gcsEVENT
{
	/* Pointer to next event in queue. */
	gcsEVENT_PTR				next;

	/* Event information. */
	gcsHAL_INTERFACE			event;
}
gcsEVENT;

typedef struct _gcsEVENT_QUEUE
{
	/* Time stamp. */
	gctUINT64					stamp;

	/* Pointer to head of event queue. */
	gcsEVENT_PTR				head;

	/* Pointer to tail of event queue. */
	gcsEVENT_PTR				tail;
}
gcsEVENT_QUEUE;

/* gcoEVENT object. */
struct _gcoEVENT
{
	/* The object. */
	gcsOBJECT					object;

	/* Pointer to required objects. */
	gcoOS						os;
	gcoKERNEL					kernel;

	/* Time stamp. */
	gctUINT64					stamp;
	gctUINT64					lastCommitStamp;

#if USE_EVENT_QUEUE
	/* Queue mutex. */
	gctPOINTER					mutexQueue;

	/* Array of event queues. */
	gcsEVENT_QUEUE				queues[32];

	/* Event chain to be handled. */
	gcsEVENT_PTR				head;
	gcsEVENT_PTR				tail;
#else

	/* Array of events. */
	struct _gcsEVENT_SCHEDULE
	{
		gctUINT64				stamp;
		gceEVENT_TYPE			type;
		gcuEVENT_DATA			data;
	}							schedule[32];

	/* Last used event. */
	gctUINT8					lastID;
#endif
};

/* gcuVIDMEM_NODE structure. */
typedef union _gcuVIDMEM_NODE
{
	/* Allocated from gcoVIDMEM. */
	struct _gcsVIDMEM_NODE_VIDMEM
	{
		/* Owner of this node. */
		gcoVIDMEM				memory;

		/* Dual-linked list of nodes. */
		gcuVIDMEM_NODE_PTR		next;
		gcuVIDMEM_NODE_PTR		prev;

		/* Dual linked list of free nodes. */
		gcuVIDMEM_NODE_PTR		nextFree;
		gcuVIDMEM_NODE_PTR		prevFree;

		/* Information for this node. */
		gctUINT32				address;
		gctSIZE_T				bytes;
		gctUINT32				alignment;

		/* Locked counter. */
		gctINT32				locked;
	}
	VidMem;

	/* Allocated from gcoOS. */
	struct _AQVIDMEM_NODE_VIRTUAL
	{
		/* Pointer to gcoKERNEL object. */
		gcoKERNEL				kernel;

		/* Information for this node. */
		gctPHYS_ADDR			physical;
		gctSIZE_T				bytes;
		gctPOINTER				logical;

		/* Page table information. */
		gctSIZE_T				pageCount;
		gctPOINTER				pageTable;
		gctUINT32				address;

		/* Mutex. */
		gctPOINTER				mutex;

		/* Locked counter. */
		gctINT32				locked;

		/* Pending flag. */
		gctBOOL					pending;
	}
	Virtual;
}
gcuVIDMEM_NODE;

/* gcoVIDMEM object. */
struct _gcoVIDMEM
{
	/* Object. */
	gcsOBJECT					object;

	/* Pointer to gcoOS object. */
	gcoOS						os;

	/* Information for this video memory heap. */
	gctUINT32					baseAddress;
	gctSIZE_T					bytes;
	gctSIZE_T					freeBytes;

	/* Mapping for each type of surface. */
	gctINT						mapping[gcvSURF_NUM_TYPES];

	/* Sentinel nodes for up to 8 banks. */
	gcuVIDMEM_NODE				sentinel[8];

	/* Allocation threshold. */
	gctSIZE_T					threshold;

    /* The heap mutex. */
    gctPOINTER					mutex;
};

/* gcoMMU object. */
struct _gcoMMU
{
	/* The object. */
	gcsOBJECT					object;

	/* Pointer to gcoOS object. */
	gcoOS						os;

	/* Pointer to gcoHARDWARE object. */
	gcoHARDWARE					hardware;

	/* The page table mutex. */
	gctPOINTER					mutex;

	/* Page table information. */
	gctSIZE_T					pageTableSize;
	gctPHYS_ADDR				pageTablePhysical;
	gctPOINTER					pageTableLogical;

	/* Allocation index. */
	gctUINT32					entryCount;
	gctUINT32					entry;
};

#endif /* __aqhalint_h_ */
