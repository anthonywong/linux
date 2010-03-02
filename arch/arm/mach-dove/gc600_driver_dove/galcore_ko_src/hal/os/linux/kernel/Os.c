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

#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#ifdef NO_DMA_COHERENT
#include <linux/dma-mapping.h>
#endif /* NO_DMA_COHERENT */

#define USE_VMALLOC 0
#define USER_SIGNAL_TABLE_LEN_INIT 	64

#define MEMORY_LOCK(os) \
	gcmVERIFY_OK(gcoOS_AcquireMutex( \
								(os), \
								(os)->memoryLock, \
								gcvINFINITE))

#define MEMORY_UNLOCK(os) \
	gcmVERIFY_OK(gcoOS_ReleaseMutex((os), (os)->memoryLock))

#define MEMORY_MAP_LOCK(os) \
	gcmVERIFY_OK(gcoOS_AcquireMutex( \
								(os), \
								(os)->memoryMapLock, \
								gcvINFINITE))

#define MEMORY_MAP_UNLOCK(os) \
	gcmVERIFY_OK(gcoOS_ReleaseMutex((os), (os)->memoryMapLock))

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

struct _gcoOS
{
	/* Object. */
	gcsOBJECT					object;

	/* Heap. */
	gcoHEAP						heap;

	/* Pointer to device */
	gcoGALDEVICE 				device;

	/* Memory management */
	gctPOINTER					memoryLock;
	gctPOINTER					memoryMapLock;

	struct _LINUX_MDL   		*mdlHead;
	struct _LINUX_MDL   		*mdlTail;

	gctUINT32					baseAddress;

	/* Signal management. */
	struct _signal {
		/* Unused signal ID number. */
		gctINT unused;

		/* The pointer to the table. */
		gctPOINTER * table;

		/* Signal table length. */
		gctINT tableLen;

		/* The current unused signal ID. */
		gctINT currentID;

		/* Lock. */
		gctPOINTER lock;
	} signal;
};

typedef struct _gcsSIGNAL
{
	/* Kernel sync primitive. */
	struct completion event;

	/* Manual reset flag. */
	gctBOOL manualReset;

	/* The reference counter. */
	atomic_t ref;

	/* The owner of the signal. */
	gctHANDLE process;
}
gcsSIGNAL;

typedef struct _gcsSIGNAL *	gcsSIGNAL_PTR;

typedef struct _gcsPageInfo * gcsPageInfo_PTR;

typedef struct _gcsPageInfo
{
	struct page **pages;
	gctUINT32_PTR pageTable;
}
gcsPageInfo;

static PLINUX_MDL
_CreateMdl(
	IN gctINT PID
	)
{
	PLINUX_MDL	mdl;

    mdl = (PLINUX_MDL)kmalloc(sizeof(struct _LINUX_MDL), GFP_ATOMIC);
	if (mdl == gcvNULL) return gcvNULL;

	mdl->pid	= PID;
	mdl->maps	= gcvNULL;

	return mdl;
}

static gceSTATUS
_DestroyMdlMap(
	IN PLINUX_MDL Mdl,
	IN PLINUX_MDL_MAP MdlMap
	);

static gceSTATUS
_DestroyMdl(
	IN PLINUX_MDL Mdl
	)
{
	PLINUX_MDL_MAP mdlMap;

	/* Verify the arguments. */
	gcmVERIFY_ARGUMENT(Mdl != gcvNULL);

	mdlMap = Mdl->maps;

	while (mdlMap != gcvNULL)
	{
		gcmVERIFY_OK(_DestroyMdlMap(Mdl, mdlMap));

		mdlMap = Mdl->maps;
	}

	kfree(Mdl);

	return gcvSTATUS_OK;
}

static PLINUX_MDL_MAP
_CreateMdlMap(
	IN PLINUX_MDL Mdl,
	IN gctINT PID
	)
{
	PLINUX_MDL_MAP	mdlMap;

    mdlMap = (PLINUX_MDL_MAP)kmalloc(sizeof(struct _LINUX_MDL_MAP), GFP_ATOMIC);
	if (mdlMap == gcvNULL) return gcvNULL;

	mdlMap->pid		= PID;
	mdlMap->vmaAddr	= gcvNULL;
	mdlMap->vma		= gcvNULL;

	mdlMap->next	= Mdl->maps;
	Mdl->maps		= mdlMap;

	return mdlMap;
}

static gceSTATUS
_DestroyMdlMap(
	IN PLINUX_MDL Mdl,
	IN PLINUX_MDL_MAP MdlMap
	)
{
	PLINUX_MDL_MAP	prevMdlMap;

	/* Verify the arguments. */
	gcmVERIFY_ARGUMENT(MdlMap != gcvNULL);
	gcmASSERT(Mdl->maps != gcvNULL);

	if (Mdl->maps == MdlMap)
	{
		Mdl->maps = MdlMap->next;
	}
	else
	{
		prevMdlMap = Mdl->maps;

		while (prevMdlMap->next != MdlMap)
		{
			prevMdlMap = prevMdlMap->next;

			gcmASSERT(prevMdlMap != gcvNULL);
		}

		prevMdlMap->next = MdlMap->next;
	}

	kfree(MdlMap);

	return gcvSTATUS_OK;
}

extern PLINUX_MDL_MAP
FindMdlMap(
	IN PLINUX_MDL Mdl,
	IN gctINT PID
	)
{
	PLINUX_MDL_MAP	mdlMap;

	mdlMap = Mdl->maps;

	while (mdlMap != gcvNULL)
	{
		if (mdlMap->pid == PID) return mdlMap;

		mdlMap = mdlMap->next;
	}

	return gcvNULL;
}

void
FreeProcessMemoryOnExit(
	IN gcoOS Os,
	IN gcoKERNEL Kernel
	)
{
	PLINUX_MDL      mdl, nextMdl;
	PLINUX_MDL_MAP	mdlMap;

	MEMORY_LOCK(Os);

	mdl = Os->mdlHead;

	while (mdl != gcvNULL)
	{
		if (mdl != Os->mdlTail)
		{
			nextMdl = mdl->next;
		}
		else
		{
			nextMdl = gcvNULL;
		}

		if (mdl->pagedMem)
		{
			mdlMap = mdl->maps;

			if (mdlMap != gcvNULL 
				&& mdlMap->pid == current->tgid 
				&& mdlMap->next == gcvNULL)
			{
				MEMORY_UNLOCK(Os);

				gcmVERIFY_OK(gcoOS_FreePagedMemory(Os, mdl, mdl->numPages * PAGE_SIZE));

				MEMORY_LOCK(Os);

				nextMdl = Os->mdlHead;
			}
		}

		mdl = nextMdl;
    }

	MEMORY_UNLOCK(Os);
}

void
PrintInfoOnExit(
	IN gcoOS Os,
	IN gcoKERNEL Kernel
	)
{
	PLINUX_MDL      mdl, nextMdl;
	PLINUX_MDL_MAP	mdlMap;

#if COMMAND_PROCESSOR_VERSION == 1
#if !USE_EVENT_QUEUE
	{
		gctUINT i, c = 0;

		for (i = 0; i < 32; i++)
		{
			if (Kernel->event->schedule[i].stamp != 0)
			{
				printk("#Unfreed event[%d]: type: %d, stamp: %d\n",
						i, Kernel->event->schedule[i].type,
						(gctUINT32)Kernel->event->schedule[i].stamp);
				c++;

				if (Kernel->event->schedule[i].type == gcvEVENT_FREE_VIDEO_MEMORY)
				{
					gcuVIDMEM_NODE_PTR node =
							Kernel->event->schedule[i].data.FreeVideoMemory.node;

					printk("\tnode: 0x%x, address: 0x%x, bytes: 0x%x\n",
							node,
							node->VidMem.address,
							node->VidMem.bytes);
				}
			}
		}

		printk("#Total unfreed events: %d\n", c);
	}
#endif
#endif

	{
		/* Mark the entire page table as available. */
		gcoMMU mmu;
		gctUINT32 *pageTable;
		int i;

		mmu = Kernel->mmu;
		pageTable = (gctUINT32 *) mmu->pageTableLogical;

		for (i = 0; i < mmu->entryCount; i++)
		{
			if (pageTable[i] != ~0)
			{
				printk("#Occupied mmu entry[%d] -> 0x%x\n", i, pageTable[i]);
				pageTable[i] = ~0;
			}
		}
	}

	MEMORY_LOCK(Os);

	mdl = Os->mdlHead;

	while (mdl != gcvNULL)
	{
		if (mdl != Os->mdlTail)
		{
			nextMdl = mdl->next;
		}
		else
		{
			nextMdl = gcvNULL;
		}

		printk("Unfreed mdl: %p, pid: %d -> pagedMem: %s, addr: %p, dmaHandle: 0x%x, pages: %d\n",
			mdl,
			mdl->pid,
			mdl->pagedMem? "true" : "false",
			mdl->addr,
			mdl->dmaHandle,
			mdl->numPages);

		mdlMap = mdl->maps;

		while (mdlMap != gcvNULL)
		{
			printk("\tmap: %p, pid: %d -> vmaAddr: %p, vma: %p\n",
					mdlMap,
					mdlMap->pid,
					mdlMap->vmaAddr,
					mdlMap->vma);

			mdlMap = mdlMap->next;
		}

		mdl = nextMdl;
	}

	MEMORY_UNLOCK(Os);
}

void
OnProcessExit(
	IN gcoOS Os,
	IN gcoKERNEL Kernel
	)
{
	/* PrintInfoOnExit(Os, Kernel); */

#ifdef ANDROID
	FreeProcessMemoryOnExit(Os, Kernel);
#endif
}

/*******************************************************************************
**
**	gcoOS_Construct
**
**	Construct a new gcoOS object.
**
**	INPUT:
**
**		gctPOINTER Context
**			Pointer to the gcoGALDEVICE class.
**
**	OUTPUT:
**
**		gcoOS * Os
**			Pointer to a variable that will hold the pointer to the gcoOS object.
*/
gceSTATUS gcoOS_Construct(
	IN gctPOINTER Context,
	OUT gcoOS * Os
	)
{
    gcoOS os;
	gceSTATUS status;

	/* Verify the arguments. */
	gcmVERIFY_ARGUMENT(Os != gcvNULL);

	/* Allocate the gcoOS object. */
    os = (gcoOS) kmalloc(gcmSIZEOF(struct _gcoOS), GFP_ATOMIC);

	if (os == gcvNULL)
	{
		/* Out of memory. */
		return gcvSTATUS_OUT_OF_MEMORY;
	}

	/* Zero the memory. */
	memset(os, 0, gcmSIZEOF(struct _gcoOS));

	/* Initialize the gcoOS object. */
	os->object.type = gcvOBJ_OS;

	/* Set device device. */
	os->device = Context;

	/* IMPORTANT! No heap yet. */
	os->heap = gcvNULL;

	/* Initialize the memory lock. */
	gcmONERROR(
		gcoOS_CreateMutex(os, &os->memoryLock));	

	gcmONERROR(
		gcoOS_CreateMutex(os, &os->memoryMapLock));

	/* Create the gcoHEAP object. */
    gcmONERROR(
		gcoHEAP_Construct(os, PAGE_SIZE, 32, &os->heap));

	os->mdlHead = os->mdlTail = gcvNULL;

	/* Find the base address of the physical memory. */
	os->baseAddress = os->device->baseAddress;

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
				  "Physical base address set to 0x%08X.\n",
				  os->baseAddress);

	/* 
	 * Initialize the signal manager.
	 * It creates the signals to be used in
	 * the user space.
	 */

	/* Initialize mutex. */
	gcmONERROR(
		gcoOS_CreateMutex(os, &os->signal.lock));	

	/* Initialize the signal table. */
	os->signal.table = 
		kmalloc(gcmSIZEOF(gctPOINTER) * USER_SIGNAL_TABLE_LEN_INIT, GFP_KERNEL);

	if (os->signal.table == gcvNULL)
	{
		/* Out of memory. */
		status = gcvSTATUS_OUT_OF_MEMORY;
		goto OnError;
	}

	memset(os->signal.table,
		   0,
		   gcmSIZEOF(gctPOINTER) * USER_SIGNAL_TABLE_LEN_INIT);

	/* Set the signal table length. */
	os->signal.tableLen = USER_SIGNAL_TABLE_LEN_INIT;

	/* The table is empty. */
	os->signal.unused = os->signal.tableLen;

	/* Initial signal ID. */
	os->signal.currentID = 0;
	
	/* Return pointer to the gcoOS object. */
	*Os = os;

	/* Success. */
	return gcvSTATUS_OK;

OnError:
	/* Roll back any allocation. */
	if (os->signal.table != gcvNULL)
	{
		kfree(os->signal.table);
	}

	if (os->signal.lock != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoOS_DeleteMutex(os, os->signal.lock));
	}
	
	if (os->heap != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoHEAP_Destroy(os->heap));
	}

	if (os->memoryMapLock != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoOS_DeleteMutex(os, os->memoryMapLock));
	}

	if (os->memoryLock != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoOS_DeleteMutex(os, os->memoryLock));
	}

	kfree(os);

	/* Return the error. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_Destroy
**
**	Destroy an gcoOS object.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object that needs to be destroyed.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_Destroy(
	IN gcoOS Os
	)
{
	gcoHEAP heap;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	/* 
	 * Destroy the signal manager.
	 */

	/* Destroy the mutex. */
	gcmVERIFY_OK(
		gcoOS_DeleteMutex(Os, Os->signal.lock));	

	/* Free the signal table. */
	kfree(Os->signal.table);

	if (Os->heap != NULL)
	{
		/* Mark gcoHEAP as gone. */
		heap     = Os->heap;
		Os->heap = NULL;

		/* Destroy the gcoHEAP object. */
		gcmVERIFY_OK(
			gcoHEAP_Destroy(heap));
	}

	/* Destroy the memory lock. */
	gcmVERIFY_OK(
		gcoOS_DeleteMutex(Os, Os->memoryMapLock));

	gcmVERIFY_OK(
		gcoOS_DeleteMutex(Os, Os->memoryLock));

	/* Mark the gcoOS object as unknown. */
	Os->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoOS object. */
	kfree(Os);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_Allocate
**
**	Allocate memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIZE_T Bytes
**			Number of bytes to allocate.
**
**	OUTPUT:
**
**		gctPOINTER * Memory
**			Pointer to a variable that will hold the allocated memory location.
*/
gceSTATUS
gcoOS_Allocate(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Memory
	)
{
    gctPOINTER memory;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Memory != NULL);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_OS,
				  "Enter gcoOS_Allocate.\n");

    /* Do we have a heap? */
    if (Os->heap != NULL)
    {
        /* Allocate from the heap. */
        return gcoHEAP_Allocate(Os->heap, Bytes, Memory);
    }

    memory = (gctPOINTER) kmalloc(Bytes, GFP_ATOMIC);

    if (memory == NULL)
    {
        /* Out of memory. */
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    /* Return pointer to the memory allocation. */
    *Memory = memory;

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
    			  "gcoOS_Allocate: Memory Allocated->0x%x\n", 
				  (gctUINT32) memory);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_Free
**
**	Free allocated memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Memory
**			Pointer to memory allocation to free.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_Free(
	IN gcoOS Os,
	IN gctPOINTER Memory
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Memory != NULL);

	/* Do we have a heap? */
	if (Os->heap != NULL)
	{
		/* Free from the heap. */
		return gcoHEAP_Free(Os->heap, Memory);
	}

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_Free: Memory Freed->0x%x\n",
				(gctUINT32)Memory);
	
	/* Free the memory from the OS pool. */
	kfree(Memory);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_MapMemory
**
**	Map physical memory into the current process.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Start of physical address memory.
**
**		gctSIZE_T Bytes
**			Number of bytes to map.
**
**	OUTPUT:
**
**		gctPOINTER * Memory
**			Pointer to a variable that will hold the logical address of the
**			mapped memory.
*/
gceSTATUS gcoOS_MapMemory(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	)
{
	PLINUX_MDL_MAP	mdlMap;
    PLINUX_MDL		mdl = (PLINUX_MDL)Physical;

	/* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Physical != 0);
    gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Logical != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"Enter gcoOS_MapMemory\n");
				
	MEMORY_LOCK(Os);
	
	mdlMap = FindMdlMap(mdl, current->tgid);

	if (mdlMap == gcvNULL)
	{
		mdlMap = _CreateMdlMap(mdl, current->tgid);

		if (mdlMap == gcvNULL)
		{
			MEMORY_UNLOCK(Os);
			
			return gcvSTATUS_OUT_OF_MEMORY;
		}
	}

	if (mdlMap->vmaAddr == gcvNULL)
	{
		down_write(&current->mm->mmap_sem);
	    
		mdlMap->vmaAddr = (char *)do_mmap_pgoff(NULL, 
					0L, 
					mdl->numPages * PAGE_SIZE, 
					PROT_READ | PROT_WRITE, 
					MAP_SHARED, 
					0);

		if (mdlMap->vmaAddr == gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
				gcvZONE_OS,
				"gcoOS_MapMemory: do_mmap_pgoff error\n");

			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
				gcvZONE_OS,
				"[gcoOS_MapMemory] mdl->numPages: %d\n",
				"[gcoOS_MapMemory] mdl->vmaAddr: 0x%x\n",
				mdl->numPages,
				mdlMap->vmaAddr
				);

			up_write(&current->mm->mmap_sem);
			
			MEMORY_UNLOCK(Os);
	        
			return gcvSTATUS_OUT_OF_MEMORY;
		}

		mdlMap->vma = find_vma(current->mm, (unsigned long)mdlMap->vmaAddr);

		if (!mdlMap->vma)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
					gcvZONE_OS,
					"gcoOS_MapMemory: find_vma error.\n");
			
			mdlMap->vmaAddr = gcvNULL;

			up_write(&current->mm->mmap_sem);
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}

#ifndef NO_DMA_COHERENT
		if (dma_mmap_coherent(NULL, 
					mdlMap->vma, 
					mdl->addr, 
					mdl->dmaHandle, 
					mdl->numPages * PAGE_SIZE) < 0) 
		{
			up_write(&current->mm->mmap_sem);

			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
					gcvZONE_OS,
					"gcoOS_MapMemory: dma_mmap_coherent error.\n");

			mdlMap->vmaAddr = gcvNULL;
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}
#else
		mdlMap->vma->vm_page_prot = pgprot_noncached(mdlMap->vma->vm_page_prot);
		mdlMap->vma->vm_flags |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED;
		mdlMap->vma->vm_pgoff = 0;
	
		if (remap_pfn_range(mdlMap->vma,
							mdlMap->vma->vm_start,
							mdl->dmaHandle >> PAGE_SHIFT,
                	        mdl->numPages*PAGE_SIZE,
                    	    mdlMap->vma->vm_page_prot) < 0)
		{
    	    up_write(&current->mm->mmap_sem);

			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
					gcvZONE_OS,
					"gcoOS_MapMemory: remap_pfn_range error.\n");

			mdlMap->vmaAddr = gcvNULL;
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}
#endif

		up_write(&current->mm->mmap_sem);
	}
	
	MEMORY_UNLOCK(Os);

    *Logical = mdlMap->vmaAddr;

	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_OS,
    			"gcoOS_MapMemory: User Mapped address for 0x%x is 0x%x pid->%d\n",
           		(gctUINT32)mdl->addr, 
				(gctUINT32)*Logical, 
				mdlMap->pid);
    
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_UnmapMemory
**
**	Unmap physical memory out of the current process.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Start of physical address memory.
**
**		gctSIZE_T Bytes
**			Number of bytes to unmap.
**
**		gctPOINTER Memory
**			Pointer to a previously mapped memory region.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_UnmapMemory(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	)
{
	PLINUX_MDL_MAP			mdlMap;
    PLINUX_MDL				mdl = (PLINUX_MDL)Physical;
    struct task_struct *	task;

    /* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Physical != 0);
	gcmVERIFY_ARGUMENT(Bytes > 0);
	gcmVERIFY_ARGUMENT(Logical != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"in gcoOS_UnmapMemory\n");

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"gcoOS_UnmapMemory Will be unmapping 0x%x mdl->0x%x\n",
				(gctUINT32)Logical,
				(gctUINT32)mdl);
				
	MEMORY_LOCK(Os);

    if (Logical)
    {
		gcmTRACE_ZONE(gcvLEVEL_VERBOSE, 
			gcvZONE_OS,
			"[gcoOS_UnmapMemory] Logical: 0x%x\n",
			Logical
			);

		mdlMap = FindMdlMap(mdl, current->tgid);

		if (mdlMap == gcvNULL || mdlMap->vmaAddr == gcvNULL)
		{
			MEMORY_UNLOCK(Os);
			
			return gcvSTATUS_INVALID_ARGUMENT;
		}

        /* Get the current pointer for the task with stored pid. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
        {
    	    extern rwlock_t tasklist_lock;

        	read_lock(&tasklist_lock);

            task = FIND_TASK_BY_PID(mdlMap->pid);

    	    read_unlock(&tasklist_lock);
        }
#else
		rcu_read_lock();

        task = FIND_TASK_BY_PID(mdlMap->pid);
#endif

        if (task != gcvNULL && task->mm != gcvNULL)
		{
			down_write(&task->mm->mmap_sem);
			do_munmap(task->mm, (unsigned long)Logical, mdl->numPages*PAGE_SIZE);
			up_write(&task->mm->mmap_sem);
        }
        else
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
						gcvZONE_OS,
            			"Can't find the task with pid->%d. No unmapping\n",
						mdlMap->pid);
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
		/* Do Nothing */
#else
		rcu_read_unlock();
#endif
		gcmVERIFY_OK(_DestroyMdlMap(mdl, mdlMap));
    }
	
	MEMORY_UNLOCK(Os);

	/* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_AllocateNonPagedMemory
**
**	Allocate a number of pages from non-paged memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctBOOL InUserSpace
**			gcvTRUE if the pages need to be mapped into user space.
**
**		gctSIZE_T * Bytes
**			Pointer to a variable that holds the number of bytes to allocate.
**
**	OUTPUT:
**
**		gctSIZE_T * Bytes
**			Pointer to a variable that hold the number of bytes allocated.
**
**		gctPHYS_ADDR * Physical
**			Pointer to a variable that will hold the physical address of the
**			allocation.
**
**		gctPOINTER * Logical
**			Pointer to a variable that will hold the logical address of the
**			allocation.
*/
gceSTATUS gcoOS_AllocateNonPagedMemory(
	IN gcoOS Os,
	IN gctBOOL InUserSpace,
	IN OUT gctSIZE_T * Bytes,
	OUT gctPHYS_ADDR * Physical,
	OUT gctPOINTER * Logical
	)
{
    gctSIZE_T		bytes;
    gctINT			numPages;
    PLINUX_MDL		mdl;
	PLINUX_MDL_MAP	mdlMap = 0;
	gctSTRING		addr;

#ifdef NO_DMA_COHERENT
	struct page *	page;
    long			size, order;
	gctPOINTER		vaddr;
#endif

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT((Bytes != NULL) && (*Bytes > 0));
    gcmVERIFY_ARGUMENT(Physical != NULL);
    gcmVERIFY_ARGUMENT(Logical != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"in gcoOS_AllocateNonPagedMemory\n");

    /* Align number of bytes to page size. */
    bytes = gcmALIGN(*Bytes, PAGE_SIZE);

    /* Get total number of pages.. */
    numPages = GetPageCount(bytes, 0);

    /* Allocate mdl+vector structure */
    mdl = _CreateMdl(current->tgid);
	
	if (mdl == gcvNULL)
	{
		return gcvSTATUS_OUT_OF_MEMORY;
	}

	mdl->pagedMem = 0;
    mdl->numPages = numPages;
	
	MEMORY_LOCK(Os);

#ifndef NO_DMA_COHERENT
    addr = dma_alloc_coherent(NULL, 
				mdl->numPages * PAGE_SIZE, 
				&mdl->dmaHandle, 
				GFP_ATOMIC);
#else
	size	= mdl->numPages * PAGE_SIZE;
	order	= get_order(size);
	page	= alloc_pages(GFP_KERNEL | GFP_DMA, order);

	if (page == gcvNULL)
	{
		MEMORY_UNLOCK(Os);

		return gcvSTATUS_OUT_OF_MEMORY;
	}

	vaddr			= (gctPOINTER)page_address(page);
	addr			= ioremap_nocache(virt_to_phys(vaddr), size);
	mdl->dmaHandle	= virt_to_phys(vaddr);
	mdl->kaddr		= vaddr;

#if ENABLE_ARM_L2_CACHE
	dma_cache_maint(vaddr, size, DMA_FROM_DEVICE);
#endif

	while (size > 0) 
	{
		SetPageReserved(virt_to_page(vaddr));

		vaddr	+= PAGE_SIZE;
		size	-= PAGE_SIZE;
	}
#endif

    if (addr == gcvNULL)
	{
		gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
       			"galcore: Can't allocate memorry for size->0x%x\n", 
				(gctUINT32)bytes);

        gcmVERIFY_OK(_DestroyMdl(mdl));
		
		MEMORY_UNLOCK(Os);

        return gcvSTATUS_OUT_OF_MEMORY;
    }
	
	if ((Os->baseAddress & 0x80000000) != (mdl->dmaHandle & 0x80000000))
	{
		mdl->dmaHandle = (mdl->dmaHandle & ~0x80000000)
					   | (Os->baseAddress & 0x80000000);
	}

    mdl->addr = addr;

    /* 
	 * We will not do any mapping from here. 
	 * Mapping will happen from mmap method. 
	 * mdl structure will be used.
	 */

    /* Return allocated memory. */
    *Bytes = bytes;
    *Physical = (gctPHYS_ADDR) mdl;

    if (InUserSpace)
    {
		mdlMap = _CreateMdlMap(mdl, current->tgid);

		if (mdlMap == gcvNULL)
		{
			gcmVERIFY_OK(_DestroyMdl(mdl));
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_MEMORY;
		}

        /* Only after mmap this will be valid. */

        /* We need to map this to user space. */
        down_write(&current->mm->mmap_sem);

        mdlMap->vmaAddr = (gctSTRING)do_mmap_pgoff(gcvNULL, 
				0L, 
				mdl->numPages * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				0);

        if (mdlMap->vmaAddr == gcvNULL)
        {
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"galcore: do_mmap_pgoff error\n");
            
			up_write(&current->mm->mmap_sem);
            
			gcmVERIFY_OK(_DestroyMdlMap(mdl, mdlMap));
			gcmVERIFY_OK(_DestroyMdl(mdl));
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_MEMORY;
        }

        mdlMap->vma = find_vma(current->mm, (unsigned long)mdlMap->vmaAddr);

		if (mdlMap->vma == gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"find_vma error\n");

			up_write(&current->mm->mmap_sem);

			gcmVERIFY_OK(_DestroyMdlMap(mdl, mdlMap));
			gcmVERIFY_OK(_DestroyMdl(mdl));
			
			MEMORY_UNLOCK(Os);

			up_write(&current->mm->mmap_sem);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}

#ifndef NO_DMA_COHERENT
        if (dma_mmap_coherent(NULL, 
				mdlMap->vma, 
				mdl->addr, 
				mdl->dmaHandle, 
				mdl->numPages * PAGE_SIZE) < 0) 
		{
			up_write(&current->mm->mmap_sem);

			gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"dma_mmap_coherent error\n");

			gcmVERIFY_OK(_DestroyMdlMap(mdl, mdlMap));
			gcmVERIFY_OK(_DestroyMdl(mdl));
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}
#else
		mdlMap->vma->vm_page_prot = pgprot_noncached(mdlMap->vma->vm_page_prot);
		mdlMap->vma->vm_flags |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED;
		mdlMap->vma->vm_pgoff = 0;
		
		if (remap_pfn_range(mdlMap->vma,
							mdlMap->vma->vm_start,
							mdl->dmaHandle >> PAGE_SHIFT,
							mdl->numPages * PAGE_SIZE,
							mdlMap->vma->vm_page_prot))
		{
			up_write(&current->mm->mmap_sem);
	
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
					gcvZONE_OS,
					"remap_pfn_range error\n");

			gcmVERIFY_OK(_DestroyMdlMap(mdl, mdlMap));
			gcmVERIFY_OK(_DestroyMdl(mdl));
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}
#endif /* NO_DMA_COHERENT */

        up_write(&current->mm->mmap_sem);

        *Logical = mdlMap->vmaAddr;
    }
    else
    {
        *Logical = (gctPOINTER)mdl->addr;
    }

    /* 
	 * Add this to a global list. 
	 * Will be used by get physical address 
	 * and mapuser pointer functions.
	 */

    if (!Os->mdlHead)
    {
        /* Initialize the queue. */
        Os->mdlHead = Os->mdlTail = mdl;
    }
    else
    {
        /* Add to the tail. */
        mdl->prev = Os->mdlTail;
        Os->mdlTail->next = mdl;
        Os->mdlTail = mdl;
    }
	
	MEMORY_UNLOCK(Os);
	
	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_AllocateNonPagedMemory: "
				"Bytes->0x%x, Mdl->%p, Logical->0x%x dmaHandle->0x%x\n",
           		(gctUINT32)bytes, 
				mdl, 
				(gctUINT32)mdl->addr, 
				mdl->dmaHandle);

	if (InUserSpace)
	{
		gcmTRACE_ZONE(gcvLEVEL_INFO,
				gcvZONE_OS,
				"vmaAddr->0x%x pid->%d\n",
				(gctUINT32)mdlMap->vmaAddr,
 				mdlMap->pid);
	}

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_FreeNonPagedMemory
**
**	Free previously allocated and mapped pages from non-paged memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIZE_T Bytes
**			Number of bytes allocated.
**
**		gctPHYS_ADDR Physical
**			Physical address of the allocated memory.
**
**		gctPOINTER Logical
**			Logical address of the allocated memory.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_FreeNonPagedMemory(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical
	)
{
    PLINUX_MDL				mdl;
	PLINUX_MDL_MAP			mdlMap;
    struct task_struct *	task;

#ifdef NO_DMA_COHERENT
	unsigned				size;
	gctPOINTER				vaddr;
#endif /* NO_DMA_COHERENT */

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Bytes > 0);
	gcmVERIFY_ARGUMENT(Physical != 0);
	gcmVERIFY_ARGUMENT(Logical != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"in gcoOS_FreeNonPagedMemory\n");

	/* Convert physical address into a pointer to a MDL. */
    mdl = (PLINUX_MDL) Physical;

	MEMORY_LOCK(Os);
	
#ifndef NO_DMA_COHERENT
    dma_free_coherent(gcvNULL, 
					mdl->numPages * PAGE_SIZE, 
					mdl->addr, 
					mdl->dmaHandle);
#else
	size	= mdl->numPages * PAGE_SIZE;
	vaddr	= mdl->kaddr;

	while (size > 0)
	{
		ClearPageReserved(virt_to_page(vaddr));

		vaddr	+= PAGE_SIZE;
		size	-= PAGE_SIZE;
	}

	free_pages((unsigned long)mdl->kaddr, get_order(mdl->numPages * PAGE_SIZE));

	iounmap(mdl->addr);
#endif /* NO_DMA_COHERENT */

	mdlMap = mdl->maps;

	while (mdlMap != gcvNULL)
	{
		if (mdlMap->vmaAddr != gcvNULL)
		{
			/* Get the current pointer for the task with stored pid. */
			task = FIND_TASK_BY_PID(mdlMap->pid);

			if (task != gcvNULL && task->mm != gcvNULL)
			{
				down_write(&task->mm->mmap_sem);

				if (do_munmap(task->mm, 
							(unsigned long)mdlMap->vmaAddr, 
							mdl->numPages * PAGE_SIZE) < 0)
				{
					gcmTRACE_ZONE(gcvLEVEL_INFO, 
								gcvZONE_OS,
                				"gcoOS_FreeNonPagedMemory: "
								"Unmap Failed ->Mdl->0x%x Logical->0x%x vmaAddr->0x%x\n",
                        		(gctUINT32)mdl, 
								(gctUINT32)mdl->addr, 
								(gctUINT32)mdlMap->vmaAddr);
				}
	            
				up_write(&task->mm->mmap_sem);
			}

			mdlMap->vmaAddr = gcvNULL;
		}

		mdlMap = mdlMap->next;
	}

    // Remove the node from global list..
    if (mdl == Os->mdlHead)
    {
        if ((Os->mdlHead = mdl->next) == gcvNULL)
        {
            Os->mdlTail = gcvNULL;
        }
    }
    else
    {
        mdl->prev->next = mdl->next;
        if (mdl == Os->mdlTail)
        {
            Os->mdlTail = mdl->prev;
        }
        else
        {
            mdl->next->prev = mdl->prev;
        }
    }
	
	MEMORY_UNLOCK(Os);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_FreeNonPagedMemory: "
				"Mdl->0x%x Logical->0x%x\n",
          		(gctUINT32)mdl, 
				(gctUINT32)mdl->addr);
    
	gcmVERIFY_OK(_DestroyMdl(mdl));

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_ReadRegister
**
**	Read data from a register.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctUINT32 Address
**			Address of register.
**
**	OUTPUT:
**
**		gctUINT32 * Data
**			Pointer to a variable that receives the data read from the register.
*/
gceSTATUS gcoOS_ReadRegister(
	IN gcoOS Os,
	IN gctUINT32 Address,
	OUT gctUINT32 * Data
	)
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Data != NULL);

    *Data = readl((gctUINT8 *)Os->device->registerBase + Address);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_WriteRegister
**
**	Write data to a register.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctUINT32 Address
**			Address of register.
**
**		gctUINT32 Data
**			Data for register.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_WriteRegister(
	IN gcoOS Os,
	IN gctUINT32 Address,
	IN gctUINT32 Data
	)
{
    writel(Data, (gctUINT8 *)Os->device->registerBase + Address);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_GetPageSize
**
**	Get the system's page size.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**	OUTPUT:
**
**		gctSIZE_T * PageSize
**			Pointer to a variable that will receive the system's page size.
*/
gceSTATUS gcoOS_GetPageSize(
	IN gcoOS Os,
	OUT gctSIZE_T * PageSize
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(PageSize != NULL);

	/* Return the page size. */
	*PageSize = (gctSIZE_T) PAGE_SIZE;

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_GetPhysicalAddress
**
**	Get the physical system address of a corresponding virtual address.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Logical
**			Logical address.
**
**	OUTPUT:
**
**		gctUINT32 * Address
**			Poinetr to a variable that receives the	32-bit physical adress.
*/
gceSTATUS gcoOS_GetPhysicalAddress(
	IN gcoOS Os,
	IN gctPOINTER Logical,
	OUT gctUINT32 * Address
	)
{
    PLINUX_MDL		mdl;
	PLINUX_MDL_MAP	mdlMap;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Address != gcvNULL);

    /* 
	 * Try to search the address in our list.
     * This could be an mmaped memory. 
	  * Search in our list.
	  */
	  
	MEMORY_LOCK(Os);

    mdl = Os->mdlHead;

    while (mdl != gcvNULL)
    {
        /* Check for the logical address match. */
        if (mdl->addr
			&& (gctUINT32)Logical >= (gctUINT32)mdl->addr
			&& (gctUINT32)Logical < ((gctUINT32)mdl->addr + mdl->numPages*PAGE_SIZE))
        {
            if (mdl->dmaHandle)
            {
                /* The memory was from coherent area. */
                *Address = (gctUINT32)mdl->dmaHandle 
							+ (gctUINT32)((gctUINT32)Logical - (gctUINT32)mdl->addr);
            }
            else if (mdl->pagedMem)
            {
#if USE_VMALLOC
                *Address = page_to_phys(vmalloc_to_page((gctSTRING)mdl->addr
							+ ((gctUINT32)Logical - (gctUINT32)mdl->addr)));
#else
                *Address = (gctUINT32)virt_to_phys(mdl->addr)
							+ ((gctUINT32)Logical - (gctUINT32)mdl->addr);
#endif
            }
            else
            {
                *Address = (gctUINT32)virt_to_phys(mdl->addr)
							+ ((gctUINT32)Logical - (gctUINT32)mdl->addr);
            }
            break;
        }
        
		mdlMap = FindMdlMap(mdl, current->tgid);

        /* Is the given address within that range. */
        if (mdlMap != gcvNULL
			&& mdlMap->vmaAddr != gcvNULL
			&& Logical >= mdlMap->vmaAddr
			&& Logical < (mdlMap->vmaAddr + mdl->numPages * PAGE_SIZE))
        {
            if (mdl->dmaHandle)
            {
                /* The memory was from coherent area. */
                *Address = (gctUINT32)mdl->dmaHandle
							+ (gctUINT32)((gctUINT32)Logical
							- (gctUINT32)mdlMap->vmaAddr);
            }
            else if (mdl->pagedMem)
            {
#if USE_VMALLOC
                *Address = page_to_phys(vmalloc_to_page((gctSTRING)mdl->addr
							+ ((gctUINT32)Logical - (gctUINT32)mdlMap->vmaAddr)));
#else
                *Address = (gctUINT32)virt_to_phys(mdl->addr)
							+ (gctUINT32)(Logical - mdlMap->vmaAddr);
#endif
            }
            else
            {
                /* Return the kernel virtual pointer based on this. */
                *Address = (gctUINT32)virt_to_phys(mdl->addr) 
							+ (gctUINT32)(Logical - mdlMap->vmaAddr);
            }
            break;
        }

        mdl = mdl->next;
    }

	/* Subtract base address to get a GPU physical address. */
	gcmASSERT(*Address >= Os->baseAddress);
	*Address -= Os->baseAddress;
	
	MEMORY_UNLOCK(Os);

    if (mdl == gcvNULL)
    {
		gcmTRACE_ZONE(gcvLEVEL_INFO, 
					gcvZONE_OS,
        			"gcoOS_GetPhysicalAddress: "
					"Unable to get physical address for 0x%x\n",
					(gctUINT32)Logical);

        return gcvSTATUS_INVALID_ARGUMENT;
    }

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
					gcvZONE_OS,
    				"gcoOS_GetPhysicalAddress: Logical->0x%x Physical->0x%x\n",
           			(gctUINT32)Logical,
					(gctUINT32)*Address);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_MapPhysical
**
**	Map a physical address into kernel space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctUINT32 Physical
**			Physical address of the memory to map.
**
**		gctSIZE_T Bytes
**			Number of bytes to map.
**
**	OUTPUT:
**
**		gctPOINTER * Logical
**			Pointer to a variable that receives the	base address of the mapped
**			memory.
*/
gceSTATUS gcoOS_MapPhysical(
	IN gcoOS Os,
	IN gctUINT32 Physical,
	IN gctUINT32 OriginalLogical,	
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	)
{
	gctPOINTER logical;
    PLINUX_MDL mdl;
	gctUINT32 physical;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

	MEMORY_LOCK(Os);

	/* Compute true physical address (before subtraction of the baseAddress). */
	physical = Physical + Os->baseAddress;
	
    /* Go through our mapping to see if we know this physical address already. */
    mdl = Os->mdlHead;

    while (mdl != gcvNULL)
    {
        if (mdl->dmaHandle != 0)
        {
            if ((physical >= mdl->dmaHandle)
			&&  (physical < mdl->dmaHandle + mdl->numPages * PAGE_SIZE)
			)
            {
                *Logical = mdl->addr + (physical - mdl->dmaHandle);
                break;
            }
        }

        mdl = mdl->next;
    }

    if (mdl == gcvNULL)
    {
        /* Map memory as cached memory. */
        request_mem_region(physical, Bytes, "MapRegion");
        logical = (gctPOINTER) ioremap_nocache(physical, Bytes);

	    if (logical == NULL)
	    {
			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
            			  "gcoOS_MapMemory: Failed to ioremap\n");
						
			MEMORY_UNLOCK(Os);
		    
			/* Out of resources. */
		    return gcvSTATUS_OUT_OF_RESOURCES;
	    }

	    /* Return pointer to mapped memory. */
	    *Logical = logical;
    }
	
	MEMORY_UNLOCK(Os);

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
    			  "gcoOS_MapPhysical: "
				  "Physical->0x%X Bytes->0x%X Logical->0x%X MappingFound->%d\n",
				  (gctUINT32) Physical, 
				  (gctUINT32) Bytes, 
				  (gctUINT32) *Logical, 
				   mdl ? 1 : 0);
    
	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_UnmapPhysical
**
**	Unmap a previously mapped memory region from kernel memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Logical
**			Pointer to the base address of the memory to unmap.
**
**		gctSIZE_T Bytes
**			Number of bytes to unmap.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_UnmapPhysical(
	IN gcoOS Os,
	IN gctPOINTER Logical,
	IN gctSIZE_T Bytes
	)
{
    PLINUX_MDL  mdl;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Logical != NULL);
	gcmVERIFY_ARGUMENT(Bytes > 0);

	MEMORY_LOCK(Os);
	
    mdl = Os->mdlHead;

    while (mdl != gcvNULL)
    {
        if (mdl->addr != gcvNULL)
        {
            if (Logical >= (gctPOINTER)mdl->addr 
					&& Logical < (gctPOINTER)((gctSTRING)mdl->addr + mdl->numPages * PAGE_SIZE))
            {
                break;
            }
        }

        mdl = mdl->next;
    }

    if (mdl == gcvNULL)
    {
	    /* Unmap the memory. */
	    iounmap(Logical);
    }
	
	MEMORY_UNLOCK(Os);
	
	gcmTRACE_ZONE(gcvLEVEL_INFO, 
					gcvZONE_OS,
    				"gcoOS_UnmapPhysical: "
					"Logical->0x%x Bytes->0x%x MappingFound(?)->%d\n",
           			(gctUINT32)Logical, 
					(gctUINT32)Bytes, 
					mdl ? 1 : 0);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_CreateMutex
**
**	Create a new mutex.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**	OUTPUT:
**
**		gctPOINTER * Mutex
**			Pointer to a variable that will hold a pointer to the mutex.
*/
gceSTATUS gcoOS_CreateMutex(
	IN gcoOS Os,
	OUT gctPOINTER * Mutex
	)
{
	/* Validate the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Mutex != NULL);

	/* Allocate a FAST_MUTEX structure. */
	*Mutex = (gctPOINTER)kmalloc(sizeof(struct semaphore), GFP_KERNEL);

	if (*Mutex == gcvNULL)
	{
		return gcvSTATUS_OUT_OF_MEMORY;
	}

    /* Initialize the semaphore.. Come up in unlocked state. */
    init_MUTEX(*Mutex);

	/* Return status. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_DeleteMutex
**
**	Delete a mutex.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Mutex
**			Pointer to the mute to be deleted.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_DeleteMutex(
	IN gcoOS Os,
	IN gctPOINTER Mutex
	)
{
	/* Validate the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Mutex != NULL);

	/* Delete the fast mutex. */
	kfree(Mutex);

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_AcquireMutex
**
**	Acquire a mutex.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Mutex
**			Pointer to the mutex to be acquired.
**
**		gctUINT32 Timeout
**			Timeout value specified in milliseconds.
**			Specify the value of gcvINFINITE to keep the thread suspended
**			until the mutex has been acquired.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_AcquireMutex(
	IN gcoOS Os,
	IN gctPOINTER Mutex,
	IN gctUINT32 Timeout
	)
{
	/* Validate the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Mutex != NULL);

	if (Timeout == gcvINFINITE)
	{
		down((struct semaphore *) Mutex);

		/* Success. */
		return gcvSTATUS_OK;
	}

	while (Timeout-- > 0)
	{
		/* Try to acquire the fast mutex. */
		if (!down_trylock((struct semaphore *) Mutex))
		{
			/* Success. */
			return gcvSTATUS_OK;
		}

		/* Wait for 1 millisecond. */
		gcmVERIFY_OK(gcoOS_Delay(Os, 1));
	}

	/* Timeout. */
	return gcvSTATUS_TIMEOUT;
}

/*******************************************************************************
**
**	gcoOS_ReleaseMutex
**
**	Release an acquired mutex.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Mutex
**			Pointer to the mutex to be released.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_ReleaseMutex(
	IN gcoOS Os,
	IN gctPOINTER Mutex
	)
{
	/* Validate the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Mutex != NULL);

	/* Release the fast mutex. */
	up((struct semaphore *) Mutex);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_AtomicExchange
**
**	Atomically exchange a pair of 32-bit values.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**      IN OUT gctINT32_PTR Target 
**          Pointer to the 32-bit value to exchange.
**
**		IN gctINT32 NewValue
**			Specifies a new value for the 32-bit value pointed to by Target. 
**
**      OUT gctINT32_PTR OldValue
**          The old value of the 32-bit value pointed to by Target.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_AtomicExchange(
	IN gcoOS Os,
    IN OUT gctUINT32_PTR Target,
	IN gctUINT32 NewValue,
    OUT gctUINT32_PTR OldValue
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	/* Exchange the pair of 32-bit values. */
	*OldValue = (gctUINT32) atomic_xchg((atomic_t *) Target, (int) NewValue);

	/* Success. */
	return gcvSTATUS_OK;
}


/*******************************************************************************
**
**	gcoOS_AtomicExchangePtr
**
**	Atomically exchange a pair of pointers.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**      IN OUT gctPOINTER * Target 
**          Pointer to the 32-bit value to exchange.
**
**		IN gctPOINTER NewValue
**			Specifies a new value for the pointer pointed to by Target. 
**
**      OUT gctPOINTER * OldValue
**          The old value of the pointer pointed to by Target.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_AtomicExchangePtr(
	IN gcoOS Os,
    IN OUT gctPOINTER * Target,
	IN gctPOINTER NewValue,
    OUT gctPOINTER * OldValue
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	/* Exchange the pair of pointers. */
	*OldValue = (gctPOINTER) atomic_xchg((atomic_t *) Target, (int) NewValue);

	/* Success. */
	return gcvSTATUS_OK;
}


/*******************************************************************************
**
**	gcoOS_Delay
**
**	Delay execution of the current thread for a number of milliseconds.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctUINT32 Delay
**			Delay to sleep, specified in milliseconds.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_Delay(
	IN gcoOS Os,
	IN gctUINT32 Delay
	)
{
	struct timeval  now = {0, Delay * 1000};
	unsigned long ticks;

	now.tv_usec = Delay;

	/* Convert Delay to jiffies. */
	ticks = timeval_to_jiffies(&now);

	/* schedule timeout. */
	schedule_timeout_interruptible(ticks);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_MemoryBarrier
**
**	Make sure the CPU has executed everything up to this point and the data got
**	written to the specified pointer.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Address
**			Address of memory that needs to be barriered.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_MemoryBarrier(
	IN gcoOS Os,
	IN gctPOINTER Address
	)
{
	/* Verify thearguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	mb();

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_AllocatePagedMemory
**
**	Allocate memory from the paged pool.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIZE_T Bytes
**			Number of bytes to allocate.
**
**	OUTPUT:
**
**		gctPHYS_ADDR * Physical
**			Pointer to a variable that receives	the	physical address of the
**			memory allocation.
*/
gceSTATUS gcoOS_AllocatePagedMemory(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	OUT gctPHYS_ADDR * Physical
	)
{
    gctINT		numPages;
    gctINT		i;
    PLINUX_MDL  mdl;
    gctSTRING	addr;
    gctSIZE_T   bytes;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Physical != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"in gcoOS_AllocatePagedMemory\n");

    bytes = gcmALIGN(Bytes, PAGE_SIZE);

    numPages = GetPageCount(bytes, 0);

	MEMORY_LOCK(Os);
	
#if USE_VMALLOC
    addr = vmalloc(bytes);
#else
    addr = (char *)__get_free_pages(GFP_ATOMIC | GFP_DMA, GetOrder(numPages));
#endif

    if (!addr)
    {
		gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
       			"gcoOS_AllocatePagedMemory: "
				"Can't allocate memorry for size->0x%x\n",
				(gctUINT32)bytes);
				
		MEMORY_UNLOCK(Os);

        return gcvSTATUS_OUT_OF_MEMORY;
    }

    mdl = _CreateMdl(current->tgid);

	if (mdl == gcvNULL)
	{
		MEMORY_UNLOCK(Os);
		
		return gcvSTATUS_OUT_OF_MEMORY;
	}
    
    mdl->addr		= addr;
    mdl->numPages	= numPages;
    mdl->pagedMem	= 1;
    
	for (i = 0; i < mdl->numPages; i++)
    {
#if USE_VMALLOC
        SetPageReserved(vmalloc_to_page((void *)(((unsigned long)addr) + i * PAGE_SIZE)));
#else
        SetPageReserved(virt_to_page((void *)(((unsigned long)addr) + i * PAGE_SIZE)));
#endif
    }

    /* Return physical address. */
    *Physical = (gctPHYS_ADDR) mdl;

    /*
	 * Add this to a global list.
	 * Will be used by get physical address 
	 * and mapuser pointer functions.
	 */
    if (!Os->mdlHead)
    {
        /* Initialize the queue. */
        Os->mdlHead = Os->mdlTail = mdl;
    }
    else
    {
        /* Add to tail. */
        mdl->prev			= Os->mdlTail;
        Os->mdlTail->next	= mdl;
        Os->mdlTail			= mdl;
    }
	
	MEMORY_UNLOCK(Os);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_AllocatePagedMemory: "
				"Bytes->0x%x, Mdl->%p, Logical->%p\n",
           		(gctUINT32)bytes, 
				mdl, 
				mdl->addr);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_FreePagedMemory
**
**	Free memory allocated from the paged pool.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Physical address of the allocation.
**
**		gctSIZE_T Bytes
**			Number of bytes of the allocation.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_FreePagedMemory(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes
	)
{
    PLINUX_MDL  mdl = (PLINUX_MDL)Physical;
    gctSTRING	addr;
    gctINT		i;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Physical != NULL);
    
	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"in gcoOS_FreePagedMemory\n");

    addr = mdl->addr;
	
	MEMORY_LOCK(Os);

    for (i = 0; i < mdl->numPages; i++)
	{
#if USE_VMALLOC
        ClearPageReserved(vmalloc_to_page((gctPOINTER)(((unsigned long)addr) + i * PAGE_SIZE)));
#else
        ClearPageReserved(virt_to_page((gctPOINTER)(((unsigned long)addr) + i * PAGE_SIZE)));
#endif
    }

#if USE_VMALLOC
    vfree(mdl->addr);
#else
    free_pages((unsigned long)mdl->addr, GetOrder(mdl->numPages));
#endif

    /* Remove the node from global list. */
    if (mdl == Os->mdlHead)
    {
        if ((Os->mdlHead = mdl->next) == gcvNULL)
        {
            Os->mdlTail = gcvNULL;
        }
    }
    else
    {
        mdl->prev->next = mdl->next;

        if (mdl == Os->mdlTail)
        {
            Os->mdlTail = mdl->prev;
        }
        else
        {
            mdl->next->prev = mdl->prev;
        }
    }
	
	MEMORY_UNLOCK(Os);

    /* Free the structure... */
    gcmVERIFY_OK(_DestroyMdl(mdl));

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_FreePagedMemory: Bytes->0x%x, Mdl->0x%x\n",
				(gctUINT32)Bytes,
				(gctUINT32)mdl);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_LockPages
**
**	Lock memory allocated from the paged pool.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Physical address of the allocation.
**
**		gctSIZE_T Bytes
**			Number of bytes of the allocation.
**
**	OUTPUT:
**
**		gctPOINTER * Logical
**			Pointer to a variable that receives the	address of the mapped
**			memory.
**
**		gctSIZE_T * PageCount
**			Pointer to a variable that receives the	number of pages required for
**			the page table.
*/
gceSTATUS gcoOS_LockPages(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical,
	OUT gctSIZE_T * PageCount
	)
{
    PLINUX_MDL		mdl;
	PLINUX_MDL_MAP	mdlMap;
    gctSTRING		addr;

#if USE_VMALLOC
    unsigned long	start;
    unsigned long	pfn;
    gctINT			i;
#endif

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Physical != NULL);
    gcmVERIFY_ARGUMENT(Logical != NULL);
    gcmVERIFY_ARGUMENT(PageCount != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO,
				gcvZONE_OS,
				"in gcoOS_LockPages\n");

    mdl = (PLINUX_MDL) Physical;
	
	MEMORY_LOCK(Os);

	mdlMap = FindMdlMap(mdl, current->tgid);

	if (mdlMap == gcvNULL)
	{
		mdlMap = _CreateMdlMap(mdl, current->tgid);

		if (mdlMap == gcvNULL)
		{
			MEMORY_UNLOCK(Os);
		
			return gcvSTATUS_OUT_OF_MEMORY;
		}
	}

	if (mdlMap->vmaAddr == gcvNULL)
	{
		down_write(&current->mm->mmap_sem);

		mdlMap->vmaAddr = (gctSTRING)do_mmap_pgoff(NULL, 
						0L, 
						mdl->numPages * PAGE_SIZE, 
						PROT_READ | PROT_WRITE, 
						MAP_SHARED, 
						0);

		up_write(&current->mm->mmap_sem);

		gcmTRACE_ZONE(gcvLEVEL_INFO, 
						gcvZONE_OS,
						"gcoOS_LockPages: "
						"vmaAddr->0x%x for phys_addr->0x%x\n", 
						(gctUINT32)mdlMap->vmaAddr, 
						(gctUINT32)mdl);

		if (mdlMap->vmaAddr == gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
						gcvZONE_OS,
						"gcoOS_LockPages: do_mmap_pgoff error\n");
						
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_MEMORY;
		}

		mdlMap->vma = find_vma(current->mm, (unsigned long)mdlMap->vmaAddr);

		if (mdlMap->vma == gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
						gcvZONE_OS,
						"find_vma error\n");

			mdlMap->vmaAddr = gcvNULL;
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_RESOURCES;
		}

		mdlMap->vma->vm_flags |= VM_RESERVED;
		/* Make this mapping non-cached. */
		mdlMap->vma->vm_page_prot = pgprot_noncached(mdlMap->vma->vm_page_prot);

		addr = mdl->addr;

		/* Now map all the vmalloc pages to this user address. */
		down_write(&current->mm->mmap_sem);

#if USE_VMALLOC
		start = mdl->vma->vm_start;

		for (i = 0; i < mdl->numPages; i++)
		{
			pfn = vmalloc_to_pfn(addr);

			if (remap_pfn_range(mdlMap->vma, 
								start, 
								pfn, 
								PAGE_SIZE, 
								mdlMap->vma->vm_page_prot) < 0)
			{
				up_write(&current->mm->mmap_sem);

				gcmTRACE_ZONE(gcvLEVEL_INFO, 
							gcvZONE_OS,
            				"gcoOS_LockPages: "
							"gctPHYS_ADDR->0x%x Logical->0x%x Unable to map addr->0x%x to start->0x%x\n",
                   			(gctUINT32)Physical, 
							(gctUINT32)*Logical, 
							(gctUINT32)addr,
							(gctUINT32)start);

				mdlMap->vmaAddr = gcvNULL;
				
				MEMORY_UNLOCK(Os);

				return gcvSTATUS_OUT_OF_MEMORY;
			}

			start += PAGE_SIZE;
			addr += PAGE_SIZE;
		}
#else
		// map kernel memory to user space..
		if (remap_pfn_range(mdlMap->vma,
							mdlMap->vma->vm_start,
							virt_to_phys((gctPOINTER)mdl->addr) >> PAGE_SHIFT,
							mdlMap->vma->vm_end - mdlMap->vma->vm_start,
							mdlMap->vma->vm_page_prot) < 0)
		{
			up_write(&current->mm->mmap_sem);

			gcmTRACE_ZONE(gcvLEVEL_INFO, 
						gcvZONE_OS,
        				"gcoOS_LockPages: unable to mmap ret\n");
	        
			mdlMap->vmaAddr = gcvNULL;
			
			MEMORY_UNLOCK(Os);

			return gcvSTATUS_OUT_OF_MEMORY;
		}
#endif

		up_write(&current->mm->mmap_sem);
	}

    /* Convert pointer to MDL. */
    *Logical = mdlMap->vmaAddr;

    *PageCount = mdl->numPages;
	
	MEMORY_UNLOCK(Os);
	
	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_LockPages: "
				"gctPHYS_ADDR->0x%x Bytes->0x%x Logical->0x%x pid->%d\n",
           		(gctUINT32)Physical, 
				(gctUINT32)Bytes, 
				(gctUINT32)*Logical, 
				mdlMap->pid);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_MapPages
**
**	Map paged memory into a page table.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Physical address of the allocation.
**
**		gctSIZE_T PageCount
**			Number of pages required for the physical address.
**
**		gctPOINTER PageTable
**			Pointer to the page table to fill in.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_MapPages(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T PageCount,
	IN gctPOINTER PageTable
	)
{
    PLINUX_MDL  mdl;
    gctUINT32*	table;
    gctSTRING	addr;
    gctINT		i = 0;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Physical != NULL);
    gcmVERIFY_ARGUMENT(PageCount > 0);
    gcmVERIFY_ARGUMENT(PageTable != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"in gcoOS_MapPages\n");

    /* Convert pointer to MDL. */
    mdl = (PLINUX_MDL)Physical;
	
	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
    			"gcoOS_MapPages: "
				"Physical->0x%x PageCount->0x%x PagedMemory->?%d\n",
           		(gctUINT32)Physical, 
				(gctUINT32)PageCount, 
				mdl->pagedMem);

	MEMORY_LOCK(Os);

    table = (gctUINT32 *)PageTable;

	 /* Get all the physical addresses and store them in the page table. */

	addr = mdl->addr;

    if (mdl->pagedMem)
    {
        /* Try to get the user pages so DMA can happen. */
        while (PageCount-- > 0)
        {
#if USE_VMALLOC
            *table++ = page_to_phys(vmalloc_to_page(addr));
#else
            *table++ = virt_to_phys(addr);
#endif
            addr += PAGE_SIZE;
            i++;
        }
    }
    else
    {
		gcmTRACE_ZONE(gcvLEVEL_INFO, 
					gcvZONE_OS,
        			"We should not get this call for Non Paged Memory!\n");

		while (PageCount-- > 0)
        {
            *table++ = (gctUINT32)virt_to_phys(addr);
            addr += PAGE_SIZE;
        }
    }
	
	MEMORY_UNLOCK(Os);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_UnlockPages
**
**	Unlock memory allocated from the paged pool.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Physical address of the allocation.
**
**		gctSIZE_T Bytes
**			Number of bytes of the allocation.
**
**		gctPOINTER Logical
**			Address of the mapped memory.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_UnlockPages(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	)
{
	PLINUX_MDL_MAP			mdlMap;
    PLINUX_MDL				mdl = (PLINUX_MDL)Physical;
    struct task_struct *	task;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Physical != NULL);
    gcmVERIFY_ARGUMENT(Logical != NULL);
    
	/* Make sure there is already a mapping...*/
    gcmVERIFY_ARGUMENT(mdl->addr != NULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,	
				"in gcoOS_UnlockPages\n");

	MEMORY_LOCK(Os);
	
	mdlMap = mdl->maps;

	while (mdlMap != gcvNULL)
	{
		if (mdlMap->vmaAddr != gcvNULL)
		{
			/* Get the current pointer for the task with stored pid. */
			task = FIND_TASK_BY_PID(mdlMap->pid);

			if (task != gcvNULL && task->mm != gcvNULL)
			{
				down_write(&task->mm->mmap_sem);
				do_munmap(task->mm, (unsigned long)Logical, mdl->numPages * PAGE_SIZE);
				up_write(&task->mm->mmap_sem);
			}
		    
			mdlMap->vmaAddr = gcvNULL;
		}

		mdlMap = mdlMap->next;
	}

	MEMORY_UNLOCK(Os);
	
    /* Success. */
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**	gcoOS_AllocateContiguous
**
**	Allocate memory from the contiguous pool.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
** 		gctBOOL InUserSpace
**			gcvTRUE if the pages need to be mapped into user space.
**
**		gctSIZE_T * Bytes
**			Pointer to the number of bytes to allocate.
**
**	OUTPUT:
**
**		gctSIZE_T * Bytes
**			Pointer to a variable that receives	the	number of bytes allocated.
**
**		gctPHYS_ADDR * Physical
**			Pointer to a variable that receives	the	physical address of the
**			memory allocation.
**
**		gctPOINTER * Logical
**			Pointer to a variable that receives	the	logical address of the
**			memory allocation.
*/
gceSTATUS gcoOS_AllocateContiguous(
	IN gcoOS Os,
	IN gctBOOL InUserSpace,
	IN OUT gctSIZE_T * Bytes,
	OUT gctPHYS_ADDR * Physical,
	OUT gctPOINTER * Logical
	)
{
    /* Same as non-paged memory for now. */
    return gcoOS_AllocateNonPagedMemory(Os,
				InUserSpace,
				Bytes,
				Physical,
				Logical
				);
}

/*******************************************************************************
**
**	gcoOS_FreeContiguous
**
**	Free memory allocated from the contiguous pool.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPHYS_ADDR Physical
**			Physical address of the allocation.
**
**		gctPOINTER Logical
**			Logicval address of the allocation.
**
**		gctSIZE_T Bytes
**			Number of bytes of the allocation.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoOS_FreeContiguous(
	IN gcoOS Os,
	IN gctPHYS_ADDR Physical,
	IN gctPOINTER Logical,
	IN gctSIZE_T Bytes
	)
{
    /* Same of non-paged memory for now. */
    return gcoOS_FreeNonPagedMemory(Os, Bytes, Physical, Logical);
}

/******************************************************************************
**
**	gcoOS_GetKernelLogical
**
**	Return the kernel logical pointer that corresponods to the specified
**	hardware address.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctUINT32 Address
**			Hardware physical address.
**
**	OUTPUT:
**
**		gctPOINTER * KernelPointer
**			Pointer to a variable receiving the pointer in kernel address space.
*/
gceSTATUS
gcoOS_GetKernelLogical(
	IN gcoOS Os,
	IN gctUINT32 Address,
	OUT gctPOINTER * KernelPointer
	)
{
	gceSTATUS status;

	do
	{
		gcoGALDEVICE device;
		gcoKERNEL kernel;
		gcePOOL pool;
		gctUINT32 offset;
		gctPOINTER logical;

		/* Extract the pointer to the gcoGALDEVICE class. */
		device = (gcoGALDEVICE) Os->device;

		/* Kernel shortcut. */
		kernel = device->kernel;

		/* Split the memory address into a pool type and offset. */
		gcmERR_BREAK(gcoHARDWARE_SplitMemory(
			kernel->hardware, Address, &pool, &offset
			));

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
			logical = device->contiguousBase;
			break;

		default:
			/* Invalid memory pool. */
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Build logical address of specified address. */
		* KernelPointer = ((gctUINT8_PTR) logical) + offset;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_MapUserPointer
**
**	Map a pointer from the user process into the kernel address space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Pointer
**			Pointer in user process space that needs to be mapped.
**
**		gctSIZE_T Size
**			Number of bytes that need to be mapped.
**
**	OUTPUT:
**
**		gctPOINTER * KernelPointer
**			Pointer to a variable receiving the mapped pointer in kernel address
**			space.
*/
gceSTATUS
gcoOS_MapUserPointer(
	IN gcoOS Os,
	IN gctPOINTER Pointer,
	IN gctSIZE_T Size,
	OUT gctPOINTER * KernelPointer
	)
{
	*KernelPointer = Pointer;
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_UnmapUserPointer
**
**	Unmap a user process pointer from the kernel address space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Pointer
**			Pointer in user process space that needs to be unmapped.
**
**		gctSIZE_T Size
**			Number of bytes that need to be unmapped.
**
**		gctPOINTER KernelPointer
**			Pointer in kernel address space that needs to be unmapped.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_UnmapUserPointer(
	IN gcoOS Os,
	IN gctPOINTER Pointer,
	IN gctSIZE_T Size,
	IN gctPOINTER KernelPointer
	)
{
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_WriteMemory
**
**	Write data to a memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctPOINTER Address
**			Address of the memory to write to.
**
**		gctUINT32 Data
**			Data for register.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_WriteMemory(
	IN gcoOS Os,
	IN gctPOINTER Address,
	IN gctUINT32 Data
	)
{
	/* Verify the arguments. */
	gcmVERIFY_ARGUMENT(Address != NULL);

	/* Write memory. */
    writel(Data, (gctUINT8 *)Address);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_CreateSignal
**
**	Create a new signal.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctBOOL ManualReset
**			If set to gcvTRUE, gcoOS_Signal with gcvFALSE must be called in
**			order to set the signal to nonsignaled state.
**			If set to gcvFALSE, the signal will automatically be set to
**			nonsignaled state by gcoOS_WaitSignal function.
**
**	OUTPUT:
**
**		gctSIGNAL * Signal
**			Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gcoOS_CreateSignal(
	IN gcoOS Os,
	IN gctBOOL ManualReset,
	OUT gctSIGNAL * Signal
	)
{
	gcsSIGNAL_PTR signal;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Signal != NULL);

	/* Create an event structure. */
	signal = (gcsSIGNAL_PTR)kmalloc(sizeof(gcsSIGNAL), GFP_KERNEL);

	if (signal == gcvNULL)
	{
		return gcvSTATUS_OUT_OF_MEMORY;
	}

	signal->manualReset = ManualReset;

	init_completion(&signal->event);

	atomic_set(&signal->ref, 1);

	*Signal = (gctSIGNAL) signal;

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_DestroySignal
**
**	Destroy a signal.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIGNAL Signal
**			Pointer to the gctSIGNAL.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_DestroySignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal
	)
{
	gcsSIGNAL_PTR signal;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Signal != NULL);

	signal = (gcsSIGNAL_PTR) Signal;

	if (atomic_dec_and_test(&signal->ref))
	{
		 /* Free the sgianl. */
		kfree(Signal);
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_Signal
**
**	Set a state of the specified signal.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIGNAL Signal
**			Pointer to the gctSIGNAL.
**
**		gctBOOL State
**			If gcvTRUE, the signal will be set to signaled state.
**			If gcvFALSE, the signal will be set to nonsignaled state.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_Signal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctBOOL State
	)
{
	gcsSIGNAL_PTR signal;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Signal != gcvNULL);

	signal = (gcsSIGNAL_PTR) Signal;

	/* Set the new state of the event. */
	if (signal->manualReset)
	{
		if (State)
		{
			/* Set the event to a signaled state. */
			complete_all(&signal->event);
		}
		else
		{
			/* Set the event to an unsignaled state. */
			INIT_COMPLETION(signal->event);
		}
	}
	else
	{
		if (State)
		{
			/* Set the event to a signaled state. */
			complete(&signal->event);

		}
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_WaitSignal
**
**	Wait for a signal to become signaled.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIGNAL Signal
**			Pointer to the gctSIGNAL.
**
**		gctUINT32 Wait
**			Number of milliseconds to wait.
**			Pass the value of gcvINFINITE for an infinite wait.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_WaitSignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctUINT32 Wait
	)
{
	gceSTATUS status;
	gcsSIGNAL_PTR signal;
	gctUINT timeout;
	gctUINT rc;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Signal != gcvNULL);

	signal = (gcsSIGNAL_PTR) Signal;

	/* Convert wait to milliseconds. */
	timeout = (Wait == gcvINFINITE) ? MAX_SCHEDULE_TIMEOUT : Wait*HZ/1000;

	/* Linux bug ? */
	if (!signal->manualReset && timeout == 0) timeout = 1;
	
	rc = wait_for_completion_interruptible_timeout(&signal->event, timeout);

	status = ((rc == 0) && !signal->event.done) ? gcvSTATUS_TIMEOUT : gcvSTATUS_OK;

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_MapSignal
**
**	Map a signal in to the current process space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctSIGNAL Signal
**			Pointer to tha gctSIGNAL to map.
**
**		gctHANDLE Process
**			Handle of process owning the signal.
**
**	OUTPUT:
**
**		gctSIGNAL * MappedSignal
**			Pointer to a variable receiving the mapped gctSIGNAL.
*/
gceSTATUS
gcoOS_MapSignal(
	IN gcoOS Os,
	IN gctSIGNAL Signal,
	IN gctHANDLE Process,
	OUT gctSIGNAL * MappedSignal
	)
{
	gctINT signalID;
	gcsSIGNAL_PTR signal;

	gcmVERIFY_ARGUMENT(Signal != gcvNULL);
	gcmVERIFY_ARGUMENT(MappedSignal != gcvNULL);

	signalID = (gctINT) Signal - 1;
		
	gcmVERIFY_OK(gcoOS_AcquireMutex(Os,
		Os->signal.lock,
		gcvINFINITE
		));

	if (signalID >= 0 && signalID < Os->signal.tableLen)
	{
		/* It is a user space signal. */
		signal = Os->signal.table[signalID];
		
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
			Os->signal.lock
			));

		if (signal == gcvNULL)
		{
			return gcvSTATUS_OUT_OF_RESOURCES;
		}
	}
	else
	{
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
			Os->signal.lock
			));

		/* It is a kernel space signal structure. */
		signal = (gcsSIGNAL_PTR) Signal;
	}

	if (atomic_inc_return(&signal->ref) <= 1)
	{
		/* The previous value is 0, it has been deleted. */
		return gcvSTATUS_OUT_OF_RESOURCES;
	}

	/* Success. */
	*MappedSignal = (gctSIGNAL) signal;

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_CreateUserSignal
**
**	Create a new signal to be used in the user space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctBOOL ManualReset
**			If set to gcvTRUE, gcoOS_Signal with gcvFALSE must be called in
**			order to set the signal to nonsignaled state.
**			If set to gcvFALSE, the signal will automatically be set to
**			nonsignaled state by gcoOS_WaitSignal function.
**
**	OUTPUT:
**
**		gctINT * SignalID
**			Pointer to a variable receiving the created signal's ID.
*/
gceSTATUS
gcoOS_CreateUserSignal(
	IN gcoOS Os,
	IN gctBOOL ManualReset,
	OUT gctINT * SignalID
	)
{
	gcsSIGNAL_PTR signal;
	gctINT unused, currentID, tableLen;
	gctPOINTER * table;
	gctINT i;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(SignalID != gcvNULL);
	
	/* Lock the table. */
	gcmVERIFY_OK(gcoOS_AcquireMutex(Os,
		Os->signal.lock,
		gcvINFINITE
		));

	if (Os->signal.unused < 1)
	{
		/* Enlarge the table. */
		table = (gctPOINTER *)kmalloc(
					sizeof(gctPOINTER) * (Os->signal.tableLen + USER_SIGNAL_TABLE_LEN_INIT),
					GFP_KERNEL);

		if (table == gcvNULL)
		{
			/* Out of memory. */
			return gcvSTATUS_OUT_OF_MEMORY;
		}

		memset(table + Os->signal.tableLen, 0, sizeof(gctPOINTER) * USER_SIGNAL_TABLE_LEN_INIT);
		memcpy(table, Os->signal.table, sizeof(gctPOINTER) * Os->signal.tableLen);

		/* Release the old table. */
		kfree(Os->signal.table);
		
		/* Update the table. */
		Os->signal.table = table;
		Os->signal.currentID = Os->signal.tableLen;
		Os->signal.tableLen += USER_SIGNAL_TABLE_LEN_INIT;
		Os->signal.unused += USER_SIGNAL_TABLE_LEN_INIT;
	}

	table = Os->signal.table;
	currentID = Os->signal.currentID;
	tableLen = Os->signal.tableLen;
	unused = Os->signal.unused;

	/* Create a new signal. */
	gcmVERIFY_OK(gcoOS_CreateSignal(Os, 
		ManualReset, 
		(gctSIGNAL *)&signal
		));

	/* Save the process ID. */
	signal->process = (gctHANDLE) current->tgid;
	
	table[currentID] = signal;

	/* Plus 1 to avoid NULL claims. */
	*SignalID = currentID + 1;

	/* Update the currenID. */
	if (--unused > 0)
	{
		for (i = 0; i < tableLen; i++)
		{
			if (++currentID >= tableLen)
			{
				/* Wrap to the begin. */
				currentID = 0;
			}

			if (table[currentID] == gcvNULL)
			{
				break;
			}
		}
	}

	Os->signal.table = table;
	Os->signal.currentID = currentID;
	Os->signal.tableLen = tableLen;
	Os->signal.unused = unused;
	
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
		Os->signal.lock
		));

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_DestroyUserSignal
**
**	Destroy a signal to be used in the user space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctINT SignalID
**			The signal's ID.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_DestroyUserSignal(
	IN gcoOS Os,
	IN gctINT SignalID
	)
{
	gceSTATUS status;
	gcsSIGNAL_PTR signal;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	gcmVERIFY_OK(gcoOS_AcquireMutex(Os,
		Os->signal.lock,
		gcvINFINITE
		));

	if (SignalID < 1 || SignalID > Os->signal.tableLen)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_DestroyUserSignal: invalid signal->%d.\n",
			(gctINT) SignalID 
			);
		
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
			Os->signal.lock
			));

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	SignalID -= 1;

	signal = Os->signal.table[SignalID];

	if (signal == gcvNULL)
	{
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
			Os->signal.lock
			));
		
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_DestroyUserSignal: signal is NULL.\n"
			);

		return gcvSTATUS_OUT_OF_RESOURCES;
	}

	/* Check to see if the process is the owner of the signal. */
	if (signal->process != (gctHANDLE) current->tgid)
	{
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
			Os->signal.lock
			));

		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_DestroyUserSignal: process id doesn't match. ",
			"signal->process: %d, current->tgid: %d\n",
			signal->process,
			current->tgid);

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	status = gcoOS_DestroySignal(Os,
		signal
		);

	/* Update the table. */
	Os->signal.table[SignalID] = gcvNULL;
	if (Os->signal.unused++ == 0)
	{
		Os->signal.currentID = SignalID;
	}

	gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
		Os->signal.lock
		));

	/* Success. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_WaitUserSignal
**
**	Wait for a signal used in the user mode to become signaled.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctINT SignalID
**			Signal ID.
**
**		gctUINT32 Wait
**			Number of milliseconds to wait.
**			Pass the value of gcvINFINITE for an infinite wait.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_WaitUserSignal(
	IN gcoOS Os,
	IN gctINT SignalID,
	IN gctUINT32 Wait
	)
{
	gceSTATUS status;
	gcsSIGNAL_PTR signal;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, 
		gcvZONE_OS,
		"gcoOS_WaitSignal: signal->%d, wait->%d.\n",
		SignalID,
		Wait
		);

	gcmVERIFY_OK(gcoOS_AcquireMutex(Os,
		Os->signal.lock,
		gcvINFINITE
		));

	if (SignalID < 1 || SignalID > Os->signal.tableLen)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_WaitSignal: invalid signal.\n",
			SignalID
			);

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	SignalID -= 1;

	signal = Os->signal.table[SignalID];
	
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
		Os->signal.lock
		));

	if (signal == gcvNULL)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_WaitSignal: signal is NULL.\n"
			);

		return gcvSTATUS_OUT_OF_RESOURCES;
	}
	
	if (signal->process != (gctHANDLE) current->tgid)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_WaitUserSignal: process id doesn't match. "
			"signal->process: %d, current->tgid: %d\n",
			signal->process,
			current->tgid);

		return gcvSTATUS_INVALID_ARGUMENT;
	}


	status = gcoOS_WaitSignal(Os,
		signal,
		Wait
		);

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_SignalUserSignal
**
**	Set a state of the specified signal to be used in the user space.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to an gcoOS object.
**
**		gctINT SignalID
**			SignalID.
**
**		gctBOOL State
**			If gcvTRUE, the signal will be set to signaled state.
**			If gcvFALSE, the signal will be set to nonsignaled state.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_SignalUserSignal(
	IN gcoOS Os,
	IN gctINT SignalID,
	IN gctBOOL State
	)
{
	gceSTATUS status;
	gcsSIGNAL_PTR signal;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	gcmVERIFY_OK(gcoOS_AcquireMutex(Os,
		Os->signal.lock,
		gcvINFINITE
		));

	if (SignalID < 1 || SignalID > Os->signal.tableLen)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_WaitSignal: invalid signal->%d.\n",
			SignalID
			);

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	SignalID -= 1;

	signal = Os->signal.table[SignalID];
	
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
		Os->signal.lock
		));

	if (signal == gcvNULL)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_WaitSignal: signal is NULL.\n"
			);

		return gcvSTATUS_OUT_OF_RESOURCES;
	}
	
	if (signal->process != (gctHANDLE) current->tgid)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"gcoOS_DestroyUserSignal: process id doesn't match. ",
			"signal->process: %d, current->tgid: %d\n",
			signal->process,
			current->tgid);

		return gcvSTATUS_INVALID_ARGUMENT;
	}


	status = gcoOS_Signal(Os,
		signal,
		State
		);

	/* Success. */
	return status;
}

gceSTATUS
gcoOS_CleanProcessSignal(
	gcoOS Os,
	gctHANDLE Process
	)
{
	gctINT signal;

	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);

	gcmVERIFY_OK(gcoOS_AcquireMutex(Os,
		Os->signal.lock,
		gcvINFINITE
		));

	if (Os->signal.unused == Os->signal.tableLen)
	{
		gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
			Os->signal.lock
			));

		return gcvSTATUS_OK;
	}

	for (signal = 0; signal < Os->signal.tableLen; signal++)
	{
		if (Os->signal.table[signal] != gcvNULL &&
			((gcsSIGNAL_PTR)Os->signal.table[signal])->process == Process)
		{
			gcoOS_DestroySignal(Os,	Os->signal.table[signal]);

			/* Update the signal table. */
			Os->signal.table[signal] = gcvNULL;
			if (Os->signal.unused++ == 0)
			{
				Os->signal.currentID = signal;
			}
		}
	}

	gcmVERIFY_OK(gcoOS_ReleaseMutex(Os,
		Os->signal.lock
		));

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoOS_MapUserMemory
**
**	Lock down a user buffer and return an DMA'able address to be used by the
**	hardware to access it.
**
**	INPUT:
**
**		gctPOINTER Memory
**			Pointer to memory to lock down.
**
**		gctSIZE_T Size
**			Size in bytes of the memory to lock down.
**
**	OUTPUT:
**
**		gctPOINTER * Info
**			Pointer to variable receiving the information record required by
**			gcoOS_UnmapUserMemory.
**
**		gctUINT32_PTR Address
**			Pointer to a variable that will receive the address DMA'able by the
**			hardware.
*/
gceSTATUS
gcoOS_MapUserMemory(
	IN gcoOS Os,
	IN gctPOINTER Memory,
	IN gctSIZE_T Size,
	OUT gctPOINTER * Info,
	OUT gctUINT32_PTR Address
	)
{
	gceSTATUS status;
	gctSIZE_T pageCount, i;
	gctUINT32_PTR pageTable;
	gctUINT32 address;
	gctUINT32 start, end, memory;
	gctINT result = 0;

	gcsPageInfo_PTR info = gcvNULL;
	struct page **pages = gcvNULL;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Memory != gcvNULL);
	gcmVERIFY_ARGUMENT(Size > 0);
	gcmVERIFY_ARGUMENT(Info != gcvNULL);
	gcmVERIFY_ARGUMENT(Address != gcvNULL);

	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, 
		gcvZONE_OS,
		"[gcoOS_MapUserMemory] enter.\n"
		);

	do
	{
		memory = (gctUINT32) Memory;
		
		/* Get the number of required pages. */
	 	end = (memory + Size + PAGE_SIZE - 1) >> PAGE_SHIFT;
		start = memory >> PAGE_SHIFT;
		pageCount = end - start;
	
		gcmTRACE_ZONE(gcvLEVEL_INFO, 
			gcvZONE_OS,
			"[gcoOS_MapUserMemory] pageCount: %d.\n",
			pageCount
			);

		/* Invalid argument. */
		if (pageCount == 0)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}
		
		/* Overflow. */
		if ((memory + Size) < memory)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		MEMORY_MAP_LOCK(Os);
	
		/* Allocate the Info struct. */
		info = (gcsPageInfo_PTR)kmalloc(sizeof(gcsPageInfo), GFP_KERNEL);

		if (info == gcvNULL)
		{
			status = gcvSTATUS_OUT_OF_MEMORY;
			break;
		}

		/* Allocate the array of page addresses. */
		pages = (struct page **)kmalloc(pageCount * sizeof(struct page *), GFP_KERNEL);

		if (pages == gcvNULL)
		{
			status = gcvSTATUS_OUT_OF_MEMORY;
			break;
		}
		
		/* Get the user pages. */
		down_read(&current->mm->mmap_sem);
		result = get_user_pages(current,
					current->mm,
					memory & PAGE_MASK,
					pageCount,
					1,
					0,
					pages,
					NULL
					);
		up_read(&current->mm->mmap_sem);

		if (result <=0 || result < pageCount)
		{
			struct vm_area_struct *vma;

			vma = find_vma(current->mm, memory);

			if (vma && (vma->vm_flags & VM_PFNMAP) )
			{
				do
				{
					pte_t		* pte;
					spinlock_t 	* ptl;
					unsigned long pfn;

		    		pgd_t * pgd = pgd_offset(current->mm, memory);
	   				pud_t * pud = pud_alloc(current->mm, pgd, memory);
					if (pud)
					{ 
						pmd_t * pmd = pmd_alloc(current->mm, pud, memory);
						if (pmd)
						{
							pte = pte_offset_map_lock(current->mm, pmd, memory, &ptl);
							if (!pte)
							{
								break;
							}
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}

					pfn 	 = pte_pfn(*pte);
					*Address = ((pfn << PAGE_SHIFT) | (((unsigned long)Memory) & ~PAGE_MASK)) 
								- Os->baseAddress;
					*Info 	 = gcvNULL;


#if MRVL_TAVOR_PV2_PATCH
                    {
                        gctUINT32 patch_address = *Address;
                        *Address = patch_address & 0x7fffffff;
                    }
#endif 

					pte_unmap_unlock(pte, ptl);

					/* Release page info struct. */
					if (info != gcvNULL)
					{
						/* Free the page info struct. */
						kfree(info);
					}

					if (pages != gcvNULL)
					{
						/* Free the page table. */
						kfree(pages);
					}

					MEMORY_MAP_UNLOCK(Os);

                    

					return gcvSTATUS_OK;
				}
				while (gcvFALSE);

				*Address = ~0;
				*Info = gcvNULL;

				status = gcvSTATUS_OUT_OF_RESOURCES;
				break;
			}
			else
			{
				status = gcvSTATUS_OUT_OF_RESOURCES;
				break;
			}
		}
		
		for (i = 0; i < pageCount; i++)
		{
			/* Flush the data cache. */
#ifdef ANDROID			
			dma_sync_single_for_device(
						gcvNULL, 
						page_to_phys(pages[i]), 
						PAGE_SIZE,
						DMA_TO_DEVICE);
#else
			flush_dcache_page(pages[i]);
#endif
		}

		/* Allocate pages inside the page table. */
		gcmERR_BREAK(gcoMMU_AllocatePages(Os->device->kernel->mmu,
										  pageCount,
										  (gctPOINTER *) &pageTable,
										  &address));

		/* Fill the page table. */
		for (i = 0; i < pageCount; i++)
		{
			/* Get the physical address from page struct. */
			pageTable[i] = page_to_phys(pages[i]);

			gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"[gcoOS_MapUserMemory] pages[%d]: 0x%x, pageTable[%d]: 0x%x.\n",
				i, pages[i],
				i, pageTable[i]);
		}

		/* Save pointer to page table. */
		info->pageTable = pageTable;
		info->pages = pages;

		*Info = (gctPOINTER) info;

		gcmTRACE_ZONE(gcvLEVEL_INFO, 
			gcvZONE_OS,
			"[gcoOS_MapUserMemory] info->pages: 0x%x, info->pageTable: 0x%x, info: 0x%x.\n",
			info->pages,
			info->pageTable,
			info
			);

		/* Return address. */
		*Address = address + (memory & ~PAGE_MASK);

		gcmTRACE_ZONE(gcvLEVEL_INFO, 
			gcvZONE_OS,
			"[gcoOS_MapUserMemory] Address: 0x%x.\n",
			*Address
			);

		/* Success. */
		status = gcvSTATUS_OK;
	}
	while (gcvFALSE);

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, 
			gcvZONE_OS,
			"[gcoOS_MapUserMemory] error occured: %d.\n",
			status
			);

		/* Release page array. */
		if (result > 0 && pages != gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
				gcvZONE_OS,
				"[gcoOS_MapUserMemory] error: page table is freed.\n"
				);

			for (i = 0; i < result; i++)
			{
				if (pages[i] == gcvNULL)
				{
					break;
				}
#ifdef ANDROID
				dma_sync_single_for_device(
							gcvNULL, 
							page_to_phys(pages[i]), 
							PAGE_SIZE,
							DMA_FROM_DEVICE);
#endif
				page_cache_release(pages[i]);
			}
		}
		
		if (pages != gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
				gcvZONE_OS,
				"[gcoOS_MapUserMemory] error: pages is freed.\n"
				);

			/* Free the page table. */
			kfree(pages);
			info->pages = gcvNULL;
		}

		/* Release page info struct. */
		if (info != gcvNULL)
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, 
				gcvZONE_OS,
				"[gcoOS_MapUserMemory] error: info is freed.\n"
				);

			/* Free the page info struct. */
			kfree(info);
			*Info = gcvNULL;
		}
	}

	MEMORY_MAP_UNLOCK(Os);
	
	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, 
		gcvZONE_OS,
		"[gcoOS_MapUserMemory] leave.\n"
		);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_UnmapUserMemory
**
**	Unlock a user buffer and that was previously locked down by
**	gcoOS_MapUserMemory.
**
**	INPUT:
**
**		gctPOINTER Memory
**			Pointer to memory to unlock.
**
**		gctSIZE_T Size
**			Size in bytes of the memory to unlock.
**
**		gctPOINTER Info
**			Information record returned by gcoOS_MapUserMemory.
**
**		gctUINT32_PTR Address
**			The address returned by gcoOS_MapUserMemory.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoOS_UnmapUserMemory(
	IN gcoOS Os,
	IN gctPOINTER Memory,
	IN gctSIZE_T Size,
	IN gctPOINTER Info,
	IN gctUINT32 Address
	)
{
	gceSTATUS status;
	gctUINT32 memory, start, end;
	gcsPageInfo_PTR info;
	gctSIZE_T pageCount, i;
	struct page **pages;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Memory != gcvNULL);
	gcmVERIFY_ARGUMENT(Size > 0);
	gcmVERIFY_ARGUMENT(Info != gcvNULL);
	
	gcmTRACE_ZONE(gcvLEVEL_VERBOSE, 
		gcvZONE_OS,
		"[gcoOS_UnmapUserMemory] enter.\n"
		);

	do
	{
		info = (gcsPageInfo_PTR) Info;

		if (info == gcvNULL)
		{
			return gcvSTATUS_OK;
		}

		pages = info->pages;

		gcmTRACE_ZONE(gcvLEVEL_INFO, 
			gcvZONE_OS,
			"[gcoOS_UnmapUserMemory] info: 0x%x, pages: 0x%x.\n",
			info,
			pages
			);

		/* Invalid page array. */
		if (pages == gcvNULL)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}
		
		memory = (gctUINT32) Memory;
		end = (memory + Size + PAGE_SIZE - 1) >> PAGE_SHIFT;
		start = memory >> PAGE_SHIFT;
		pageCount = end - start;

		/* Overflow. */
		if ((memory + Size) < memory)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Invalid argument. */
		if (pageCount == 0)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}
		
		gcmTRACE_ZONE(gcvLEVEL_INFO, 
			gcvZONE_OS,
			"[gcoOS_UnmapUserMemory] memory: 0x%x, pageCount: %d, pageTable: 0x%x.\n",
			memory,
			pageCount,
			info->pageTable
			);

		MEMORY_MAP_LOCK(Os);

		/* Free the pages from the MMU. */
		gcmERR_BREAK(gcoMMU_FreePages(Os->device->kernel->mmu,
									  info->pageTable,
									  pageCount
									  ));
		
		/* Release the page cache. */
		for (i = 0; i < pageCount; i++)
		{
			gcmTRACE_ZONE(gcvLEVEL_INFO, 
				gcvZONE_OS,
				"[gcoOS_UnmapUserMemory] pages[%d]: 0x%x.\n",
				i,
				pages[i]
				);

			if (!PageReserved(pages[i]))
			{
				SetPageDirty(pages[i]);
			}

#ifdef ANDROID
			dma_sync_single_for_device(
						gcvNULL, 
						page_to_phys(pages[i]), 
						PAGE_SIZE,
						DMA_FROM_DEVICE);
#endif
			page_cache_release(pages[i]);
		}
				
		/* Success. */
		status = gcvSTATUS_OK;
	}
	while (gcvFALSE);

	if (info != gcvNULL)
	{
		/* Free the page array. */
		if (info->pages != gcvNULL)
		{
			kfree(info->pages);
		}

		kfree(info);
	}

	MEMORY_MAP_UNLOCK(Os);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoOS_GetBaseAddress
**
**	Get the base address for the physical memory.
**
**	INPUT:
**
**		gcoOS Os
**			Pointer to the gcoOS object.
**
**	OUTPUT:
**
**		gctUINT32_PTR BaseAddress
**			Pointer to a variable that will receive the base address.
*/
gceSTATUS
gcoOS_GetBaseAddress(
	IN gcoOS Os,
	OUT gctUINT32_PTR BaseAddress
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(BaseAddress != gcvNULL);

	/* Return base address. */
	*BaseAddress = Os->baseAddress;

	/* Success. */
	return gcvSTATUS_OK;
}

#if USE_EVENT_QUEUE
gceSTATUS
gcoOS_SuspendInterrupt(
	IN gcoOS Os
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	
	disable_irq(Os->device->irqLine);
	
	return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_ResumeInterrupt(
	IN gcoOS Os
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	
	enable_irq(Os->device->irqLine);
	
	return gcvSTATUS_OK;
}
#endif

gceSTATUS
gcoOS_NotifyIdle(
	IN gcoOS Os,
	IN gctBOOL Idle
	)
{
	/* TODO */
	return gcvSTATUS_OK;
}

