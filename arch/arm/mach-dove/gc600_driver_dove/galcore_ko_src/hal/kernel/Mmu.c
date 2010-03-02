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

/*******************************************************************************
**
**	gcoMMU_Construct
**
**	Construct a new gcoMMU object.
**
**	INPUT:
**
**		gcoKERNEL Kernel
**			Pointer to an gcoKERNEL object.
**
**		gctSIZE_T MmuSize
**			Number of bytes for the page table.
**
**	OUTPUT:
**
**		gcoMMU * Mmu
**			Pointer to a variable that receives the gcoMMU object pointer.
*/
gceSTATUS gcoMMU_Construct(
	IN gcoKERNEL Kernel,
	IN gctSIZE_T MmuSize,
	OUT gcoMMU * Mmu
	)
{
	gcoOS os;
	gcoHARDWARE hardware;
	gceSTATUS status;
	gcoMMU mmu;
	gctUINT32 * pageTable;
	gctUINT32 i;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmVERIFY_ARGUMENT(MmuSize > 0);
	gcmVERIFY_ARGUMENT(Mmu != gcvNULL);

	/* Extract the gcoOS object pointer. */
	os = Kernel->os;
	gcmVERIFY_OBJECT(os, gcvOBJ_OS);

	/* Extract the gcoHARDWARE object pointer. */
	hardware = Kernel->hardware;
	gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

	/* Allocate memory for the gcoMMU object. */
	status = gcoOS_Allocate(os, sizeof(struct _gcoMMU), (gctPOINTER *) &mmu);

	if (status < 0)
	{
		/* Error. */
		gcmFATAL("gcoMMU_Construct: Could not allocate gcoMMU object");
		return status;
	}

	/* Initialize the gcoMMU object. */
	mmu->object.type = gcvOBJ_MMU;
	mmu->os = os;
	mmu->hardware = hardware;

	/* Create the mutex. */
	status = gcoOS_CreateMutex(os, &mmu->mutex);

	if (status < 0)
	{
		/* Roll back. */
		mmu->object.type = gcvOBJ_UNKNOWN;
		gcmVERIFY_OK(gcoOS_Free(os, mmu));

		/* Error. */
		return status;
	}

	/* Allocate the page table. */
	mmu->pageTableSize = MmuSize;
	status = gcoOS_AllocateContiguous(os,
									  gcvFALSE,
									  &mmu->pageTableSize,
									  &mmu->pageTablePhysical,
									  &mmu->pageTableLogical);

	if (status < 0)
	{
		/* Roll back. */
		gcmVERIFY_OK(gcoOS_DeleteMutex(os, mmu->mutex));

		mmu->object.type = gcvOBJ_UNKNOWN;
		gcmVERIFY_OK(gcoOS_Free(os, mmu));

		/* Error. */
		gcmFATAL("gcoMMU_Construct: Could not allocate page table");
		return status;
	}

	/* Compute number of entries in page table. */
	mmu->entryCount = mmu->pageTableSize / sizeof(gctUINT32);
	mmu->entry = 0;

	/* Mark the entire page table as available. */
	pageTable = (gctUINT32 *) mmu->pageTableLogical;
	for (i = 0; i < mmu->entryCount; i++)
	{
		pageTable[i] = ~0;
	}

	/* Set page table address. */
	status = gcoHARDWARE_SetMMU(hardware, mmu->pageTableLogical);

	if (status < 0)
	{
		/* Free the page table. */
		gcmVERIFY_OK(gcoOS_FreeContiguous(mmu->os,
									  mmu->pageTablePhysical,
									  mmu->pageTableLogical,
									  mmu->pageTableSize));

		/* Roll back. */
		gcmVERIFY_OK(gcoOS_DeleteMutex(os, mmu->mutex));

		mmu->object.type = gcvOBJ_UNKNOWN;
		gcmVERIFY_OK(gcoOS_Free(os, mmu));

		/* Error. */
		gcmFATAL("gcoMMU_Construct: Could not program page table");
		return status;
	}

	/* Return the gcoMMU object pointer. */
	*Mmu = mmu;

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_MMU,
			   "gcoMMU_Construct: %u entries at %p(0x%08X)",
			   mmu->entryCount,
			   mmu->pageTableLogical,
			   mmu->pageTablePhysical);

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoMMU_Destroy
**
**	Destroy a nAQMMU object.
**
**	INPUT:
**
**		gcoMMU Mmu
**			Pointer to an gcoMMU object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoMMU_Destroy(
	IN gcoMMU Mmu
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Mmu, gcvOBJ_MMU);

	/* Free the page table. */
	gcmVERIFY_OK(gcoOS_FreeContiguous(Mmu->os,
								  Mmu->pageTablePhysical,
								  Mmu->pageTableLogical,
								  Mmu->pageTableSize));

	/* Roll back. */
	gcmVERIFY_OK(gcoOS_DeleteMutex(Mmu->os, Mmu->mutex));

	/* Mark the gcoMMU object as unknown. */
	Mmu->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoMMU object. */
	gcmVERIFY_OK(gcoOS_Free(Mmu->os, Mmu));

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoMMU_AllocatePages
**
**	Allocate pages inside the page table.
**
**	INPUT:
**
**		gcoMMU Mmu
**			Pointer to an gcoMMU object.
**
**		gctSIZE_T PageCount
**			Number of pages to allocate.
**
**	OUTPUT:
**
**		gctPOINTER * PageTable
**			Pointer to a variable that receives the	base address of the page
**			table.
**
**		gctUINT32 * Address
**			Pointer to a variable that receives the hardware specific address.
*/
gceSTATUS gcoMMU_AllocatePages(
	IN gcoMMU Mmu,
	IN gctSIZE_T PageCount,
	OUT gctPOINTER * PageTable,
	OUT gctUINT32 * Address
	)
{
	gceSTATUS status;
	gctUINT32 tail, index, i;
	gctUINT32 * table;
	gctBOOL allocated = gcvFALSE;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Mmu, gcvOBJ_MMU);
	gcmVERIFY_ARGUMENT(PageCount > 0);
	gcmVERIFY_ARGUMENT(PageTable != gcvNULL);
	gcmVERIFY_ARGUMENT(Address != gcvNULL);

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_MMU,
			   "gcoMMU_AllocatePages: %u pages",
			   PageCount);

	if (PageCount > Mmu->entryCount)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_MMU,
				   "gcoMMU_AllocatePages: Page table too small for %u pages",
				   PageCount);

		/* Not enough pages avaiable. */
		return gcvSTATUS_OUT_OF_RESOURCES;
	}

	/* Grab the mutex. */
	status = gcoOS_AcquireMutex(Mmu->os, Mmu->mutex, gcvINFINITE);

	if (status < 0)
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_MMU,
				   "gcoMMU_AllocatePages: Could not acquire mutex");

		/* Error. */
		return status;
	}

	/* Compute the tail for this allocation. */
	tail = Mmu->entryCount - PageCount;

	/* Walk all entries until we find enough slots. */
	for (index = Mmu->entry; index <= tail;)
	{
		/* Access page table. */
		table =	(gctUINT32 *) Mmu->pageTableLogical + index;

		/* See if all slots are available. */
		for (i = 0; i < PageCount; i++, table++)
		{
			if (*table != ~0)
			{
				/* Start from next slot. */
				index += i + 1;
				break;
			}
		}

		if (i == PageCount)
		{
			/* Bail out if we have enough page entries. */
			allocated = gcvTRUE;
			break;
		}
	}

	if (!allocated)
	{
		/* Flush the MMU. */
		status = gcoHARDWARE_FlushMMU(Mmu->hardware);

		if (status >= 0)
		{
			/* Walk all entries until we find enough slots. */
			for (index = 0; index <= tail;)
			{
				/* Access page table. */
				table =	(gctUINT32 *) Mmu->pageTableLogical + index;

				/* See if all slots are available. */
				for (i = 0; i < PageCount; i++, table++)
				{
					if (*table != ~0)
					{
						/* Start from next slot. */
						index += i + 1;
						break;
					}
				}

				if (i == PageCount)
				{
					/* Bail out if we have enough page entries. */
					allocated = gcvTRUE;
					break;
				}
			}
		}
	}

	if (!allocated && (status >= 0))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_MMU,
				   "gcoMMU_AllocatePages: Not enough free pages for %u pages",
				   PageCount);

		/* Not enough empty slots available. */
		status = gcvSTATUS_OUT_OF_RESOURCES;
	}

	if (status >= 0)
	{
		/* Build virtual address. */
		status = gcoHARDWARE_BuildVirtualAddress(Mmu->hardware,
												 index,
												 0,
												 Address);

		if (status >= 0)
		{
			/* Update current entry into page table. */
			Mmu->entry = index + PageCount;

			/* Return pointer to page table. */
			*PageTable = (gctUINT32 *)	Mmu->pageTableLogical + index;

			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_MMU,
			   "gcoMMU_AllocatePages: "
			   "Allocated %u pages at index %u (0x%08X) @ %p",
			   PageCount,
			   index,
			   *Address,
			   *PageTable);
			}
	}

	/* Release the mutex. */
	gcmVERIFY_OK(gcoOS_ReleaseMutex(Mmu->os, Mmu->mutex));

	/* Return status. */
	return status;
}

/*******************************************************************************
**
**	gcoMMU_FreePages
**
**	Free pages inside the page table.
**
**	INPUT:
**
**		gcoMMU Mmu
**			Pointer to an gcoMMU object.
**
**		gctPOINTER PageTable
**			Base address of the page table to free.
**
**		gctSIZE_T PageCount
**			Number of pages to free.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoMMU_FreePages(
	IN gcoMMU Mmu,
	IN gctPOINTER PageTable,
	IN gctSIZE_T PageCount
	)
{
	gctUINT32 * table;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Mmu, gcvOBJ_MMU);
	gcmVERIFY_ARGUMENT(PageTable != gcvNULL);
	gcmVERIFY_ARGUMENT(PageCount > 0);

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_MMU,
			   "gcoMMU_FreePages: Freeing %u pages at index %u @ %p",
			   PageCount,
			   ((gctUINT32 *) PageTable - (gctUINT32 *) Mmu->pageTableLogical),
			   PageTable);

	/* Convert pointer. */
	table = (gctUINT32 *) PageTable;

	/* Mark the page table entries as available. */
	while (PageCount-- > 0)
	{
		*table++ = ~0;
	}

	/* Success. */
	return gcvSTATUS_OK;
}
