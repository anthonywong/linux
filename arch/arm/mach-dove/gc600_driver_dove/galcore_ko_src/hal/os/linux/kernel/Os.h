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




#ifndef _OS_H
#define _OS_H

typedef struct _LINUX_MDL_MAP
{
	gctINT					pid;
	gctPOINTER				vmaAddr;
	struct vm_area_struct *	vma;
	struct _LINUX_MDL_MAP *	next;
}
LINUX_MDL_MAP, *PLINUX_MDL_MAP;

typedef struct _LINUX_MDL
{
	gctINT					pid;
	char *					addr;

#ifdef NO_DMA_COHERENT
	gctPOINTER				kaddr;
#endif /* NO_DMA_COHERENT */

	gctINT					numPages;
	gctINT					pagedMem;
	dma_addr_t				dmaHandle;
	PLINUX_MDL_MAP			maps;
	struct _LINUX_MDL *		prev;
	struct _LINUX_MDL *		next;
}
LINUX_MDL, *PLINUX_MDL;

extern PLINUX_MDL_MAP
FindMdlMap(
	IN PLINUX_MDL Mdl,
	IN gctINT PID
	);

typedef struct _DRIVER_ARGS
{
	gctPOINTER 				InputBuffer;
	gctUINT32  				InputBufferSize;
	gctPOINTER 				OutputBuffer;
	gctUINT32				OutputBufferSize;
}
DRIVER_ARGS;

/* Cleanup the signal table. */
gceSTATUS
gcoOS_CleanProcessSignal(
	gcoOS Os,
	gctHANDLE Process
	);

#ifdef ANDROID
MEMORY_RECORD_PTR
CreateMemoryRecord(
	gcoOS Os,
	MEMORY_RECORD_PTR List,
	gcuVIDMEM_NODE_PTR Node
	);

void
DestoryMemoryRecord(
	gcoOS Os,
	MEMORY_RECORD_PTR Mr
	);

MEMORY_RECORD_PTR
FindMemoryRecord(
	gcoOS Os,
	MEMORY_RECORD_PTR List,
	gcuVIDMEM_NODE_PTR Node
	);

void
FreeAllMemoryRecord(
	gcoOS Os,
	MEMORY_RECORD_PTR List
	);
#endif

#endif /* _OS_H */
