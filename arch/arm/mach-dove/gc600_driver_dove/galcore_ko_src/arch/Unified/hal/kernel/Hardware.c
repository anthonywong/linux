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
#include "aqHalUser.h"
#ifdef LINUX
#include <linux/init.h>
#include <linux/module.h>
#endif

/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

static gceSTATUS
_IdentifyHardware(
	IN gcoOS Os,
	OUT gceCHIPMODEL * ChipModel,
	OUT gctUINT32_PTR ChipRevision,
	OUT gctUINT32_PTR ChipFeatures,
	OUT gctUINT32_PTR ChipMinorFeatures
	)
{
	gceSTATUS status;
	gctUINT32 chipIdentity;

	do
	{
		/* Read chip identity register. */
		gcmERR_BREAK(gcoOS_ReadRegister(Os, 0x00018, &chipIdentity));

		/* Special case for older graphic cores. */
		if (((((gctUINT32) (chipIdentity)) >> (0 ? 31:24) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1)))))) == (0x01 & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))))
		{
			*ChipModel    = gcv500;
			*ChipRevision = ( ((((gctUINT32) (chipIdentity)) >> (0 ? 15:12)) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1)))))) );
		}

		else
		{
			/* Read chip identity register. */
			gcmERR_BREAK(gcoOS_ReadRegister(Os,
											0x00020,
											(gctUINT32 *) ChipModel));

			/* !!!! HACK ALERT !!!! */
			/* Because people change device IDs without letting software know
			** about it - here is the hack to make it all look the same.  Only
			** for GC400 family.  Next time - TELL ME!!! */
			if ((*ChipModel & 0xFF00) == 0x0400)
			{
				*ChipModel &= 0x0400;
			}

			/* Read CHIP_REV register. */
			gcmERR_BREAK(gcoOS_ReadRegister(Os,
											0x00024,
											ChipRevision));

			if ((*ChipModel == gcv300)
			&&  (*ChipRevision == 0x2201)
			)
			{
				gctUINT32 date, time;

				/* Read date and time registers. */
				gcmERR_BREAK(
					gcoOS_ReadRegister(Os, 0x00028, &date));

				gcmERR_BREAK(
					gcoOS_ReadRegister(Os, 0x0002C, &time));

				if ((date == 0x20080814) && (time == 0x12051100))
				{
					/* This IP has an ECO; put the correct revision in it. */
					*ChipRevision = 0x1051;
				}
			}
		}

		/* Read chip feature register. */
		gcmERR_BREAK(
			gcoOS_ReadRegister(Os, 0x0001C, ChipFeatures));

		/* Disable fast clear on GC700. */
		if (*ChipModel == gcv700)
		{
			*ChipFeatures = ((((gctUINT32) (*ChipFeatures)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
		}

		if (((*ChipModel == gcv500) && (*ChipRevision < 2))
		||  ((*ChipModel == gcv300) && (*ChipRevision < 0x2000))
		)
		{
			/* GC500 rev 1.x and GC300 rev < 2.0 doesn't have this register. */
			*ChipMinorFeatures = 0;
		}
		else
		{
			/* Read chip minor feature register. */
			gcmERR_BREAK(
				gcoOS_ReadRegister(Os,
								   0x00034,
								   ChipMinorFeatures));
		}


#if MRVL_TAVOR_PV2_DISABLE_YFLIP
		if( (*ChipModel == 0x530) && (*ChipRevision == 4) )
		{
			*ChipMinorFeatures = *ChipMinorFeatures & ~0x00000001;				
		}

		if( (*ChipModel == 0x800) && (*ChipRevision == 0x4301) )
		{
			*ChipMinorFeatures = *ChipMinorFeatures & ~0x00000001;
		}
#endif 


#if MRVL_TAVOR_PV2_DISABLE_FASTCLEAR
		if( (*ChipModel == 0x530) && (*ChipRevision == 4) )
		{
			*ChipFeatures = *ChipFeatures &  ~0x00000001;	

			*ChipModel  = 0x500;		
		}	


		if( (*ChipModel == 0x800) && (*ChipRevision == 0x4301) )
		{
			*ChipFeatures = *ChipFeatures &  ~0x00000001;	
		}	
#endif 


		gcmTRACE(gcvLEVEL_VERBOSE,
				 "ChipModel=0x%08X ChipRevision=0x%08X ChipFeatures=0x%08X "
				 "ChipMinorFeatures=0x%08X\n",
				 *ChipModel,
				 *ChipRevision,
				 *ChipFeatures,
				 *ChipMinorFeatures);

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

static gceSTATUS
_GetChipSpecs(
	IN gcoHARDWARE Hardware
	)
{
	gctUINT32 streamCount            = 0;
	gctUINT32 registerMax            = 0;
	gctUINT32 threadCount            = 0;
	gctUINT32 shaderCoreCount        = 0;
	gctUINT32 vertexCacheSize        = 0;
	gctUINT32 vertexOutputBufferSize = 0;

	if (((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 21:21) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))))
	{
		gctUINT32 specs;
		gceSTATUS status;

		/* Read gcChipSpecs register. */
		gcmERR_RETURN(
			gcoOS_ReadRegister(Hardware->os, 0x00048, &specs));

		/* Handy macro to improve reading. */
#define gcmSPEC_FIELD(field) \
		( ((((gctUINT32) (specs)) >> (0 ? GC_CHIP_SPECS_field)) & ((gctUINT32) ((((1 ? GC_CHIP_SPECS_field) - (0 ? GC_CHIP_SPECS_field) + 1) == 32) ? ~0 : (~(~0 << ((1 ? GC_CHIP_SPECS_field) - (0 ? GC_CHIP_SPECS_field) + 1)))))) )

		/* Extract the fields. */
		streamCount            = ( ((((gctUINT32) (specs)) >> (0 ? 3:0)) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1)))))) );
		registerMax            = ( ((((gctUINT32) (specs)) >> (0 ? 7:4)) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1)))))) );
		threadCount            = ( ((((gctUINT32) (specs)) >> (0 ? 11:8)) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1)))))) );
		shaderCoreCount        = ( ((((gctUINT32) (specs)) >> (0 ? 24:20)) & ((gctUINT32) ((((1 ? 24:20) - (0 ? 24:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:20) - (0 ? 24:20) + 1)))))) );
		vertexCacheSize        = ( ((((gctUINT32) (specs)) >> (0 ? 16:12)) & ((gctUINT32) ((((1 ? 16:12) - (0 ? 16:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:12) - (0 ? 16:12) + 1)))))) );
		vertexOutputBufferSize = ( ((((gctUINT32) (specs)) >> (0 ? 31:28)) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1)))))) );
	}

	/* Get the stream count. */
	Hardware->streamCount = (streamCount != 0)
						  ? streamCount
						  : (Hardware->chipModel >= gcv1000) ? 4 : 1;
	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
				  "Specs: streamCount=%u%s\n",
				  Hardware->streamCount,
				  (streamCount == 0) ? " (default)" : "");

	/* Get the vertex output buffer size. */
	Hardware->vertexOutputBufferSize = (vertexOutputBufferSize != 0)
									 ? 1 << vertexOutputBufferSize
									 : (Hardware->chipModel == gcv400)
									   ? (Hardware->chipRevision < 0x4000) ? 512
									   : (Hardware->chipRevision < 0x4200) ? 256
									   : 128
								     : (Hardware->chipModel == gcv530)
								       ? (Hardware->chipRevision < 0x4200) ? 512
									   : 128
								     : 512;
	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
				  "Specs: vertexOutputBufferSize=%u%s\n",
				  Hardware->vertexOutputBufferSize,
				  (vertexOutputBufferSize == 0) ? " (default)" : "");

	/* Get the maximum number of threads. */
	Hardware->threadCount = (threadCount != 0)
						  ? 1 << threadCount
						  : (Hardware->chipModel == gcv400) ? 64
						  : (Hardware->chipModel == gcv500) ? 128
						  : (Hardware->chipModel == gcv530) ? 128
						  : 256;
	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
				  "Specs: threadCount=%u%s\n",
				  Hardware->threadCount,
				  (threadCount == 0) ? " (default)" : "");

	/* Get the number of shader cores. */
	Hardware->shaderCoreCount = (shaderCoreCount != 0)
							  ? shaderCoreCount
							  : (Hardware->chipModel >= gcv1000) ? 2
							  : 1;
	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
				  "Specs: shaderCoreCount=%u%s\n",
				  Hardware->shaderCoreCount,
				  (shaderCoreCount == 0) ? " (default)" : "");

	/* Get the vertex cache size. */
	Hardware->vertexCacheSize = (vertexCacheSize != 0)
							  ? vertexCacheSize
							  : 8;
	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
				  "Specs: vertexCacheSize=%u%s\n",
				  Hardware->vertexCacheSize,
				  (vertexCacheSize == 0) ? " (default)" : "");

	/* Get the maximum number of temporary registers. */
	Hardware->registerMax = (registerMax != 0)
						  ? 1 << registerMax
						  : (Hardware->chipModel == gcv400) ? 32
						  : 64;
	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
				  "Specs: registerMax=%u%s\n",
				  Hardware->registerMax,
				  (registerMax == 0) ? " (default)" : "");

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************\
****************************** gcoHARDWARE API code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_Construct
**
**  Construct a new gcoHARDWARE object.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an initialized gcoOS object.
**
**  OUTPUT:
**
**      gcoHARDWARE * Hardware
**          Pointer to a variable that will hold the pointer to the gcoHARDWARE
**          object.
*/
gceSTATUS
gcoHARDWARE_Construct(
	IN gcoOS Os,
	OUT gcoHARDWARE * Hardware
	)
{
	gcoHARDWARE hardware = gcvNULL;
	gceSTATUS status;
	gceCHIPMODEL chipModel;
	gctUINT32 chipRevision;
	gctUINT32 chipFeatures;
	gctUINT32 chipMinorFeatures;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
	gcmVERIFY_ARGUMENT(Hardware != gcvNULL);

	/* Identify the hardware. */
	gcmONERROR(
		_IdentifyHardware(Os,
						  &chipModel,
						  &chipRevision,
						  &chipFeatures,
						  &chipMinorFeatures));

	/* Allocate the gcoHARDWARE object. */
	gcmONERROR(
		gcoOS_Allocate(Os,
					   gcmSIZEOF(struct _gcoHARDWARE),
					   (gctPOINTER *) &hardware));

	/* Initialize the gcoHARDWARE object. */
	hardware->object.type = gcvOBJ_HARDWARE;
	hardware->os          = Os;

	/* Reset last read interrupt status register. */
#if !USE_EVENT_QUEUE
	hardware->data = 0;
#endif

	/* Set chip identity. */
	hardware->chipModel         = chipModel;
	hardware->chipRevision      = chipRevision;
	hardware->chipFeatures      = chipFeatures;
	hardware->chipMinorFeatures = chipMinorFeatures;
	hardware->powerBaseAddress  = (  (chipModel == gcv300)
								  && (chipRevision < 0x2000)
								  ) ? 0x100 : 0x00;

	/* Get chip specs. */
	gcmONERROR(
		_GetChipSpecs(hardware));

	/* Return pointer to the gcoHARDWARE object. */
	*Hardware = hardware;

	/* Set power state to ON. */
	hardware->chipPowerState = gcvPOWER_ON;

	/* Success. */
	return gcvSTATUS_OK;

OnError:
	/* Destroy gcoHARDWARE structure if it was allocated. */
	if (hardware != gcvNULL)
	{
		gcmVERIFY_OK(
			gcoOS_Free(Os, hardware));
	}

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Destroy
**
**  Destroy an gcoHARDWARE object.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Destroy(
    IN gcoHARDWARE Hardware
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Mark the object as unknown. */
	Hardware->object.type = gcvOBJ_UNKNOWN;

    /* Free the object. */
	return gcoOS_Free(Hardware->os, Hardware);
}

/*******************************************************************************
**
**  gcoHARDWARE_InitializeHardware
**
**  Initialize the hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS
gcoHARDWARE_InitializeHardware(
    IN gcoHARDWARE Hardware
	)
{
	gceSTATUS status;
	gctUINT32 baseAddress;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	/* Initialize the fast clear. */
	gcmONERROR(
		gcoHARDWARE_SetFastClear(Hardware, -1));

	gcmONERROR(
		gcoOS_GetBaseAddress(Hardware->os, &baseAddress));

	/* Program the base addesses. */
	gcmONERROR(
		gcoOS_WriteRegister(Hardware->os,
							0x0041C,
							baseAddress));

	gcmONERROR(
		gcoOS_WriteRegister(Hardware->os,
							0x00418,
							baseAddress));

	gcmONERROR(
		gcoOS_WriteRegister(Hardware->os,
							0x00420,
							baseAddress));

	gcmONERROR(
		gcoOS_WriteRegister(Hardware->os,
							0x00428,
							baseAddress));

	gcmONERROR(
		gcoOS_WriteRegister(Hardware->os,
							0x00424,
							baseAddress));
#if 0
	/* Enable clock gating. */
	gcmONERROR(
		gcoOS_WriteRegister(Hardware->os,
							Hardware->powerBaseAddress +
							0x00100,
							((((gctUINT32) (0x00140020)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))));
#endif
	/* Success. */
    return gcvSTATUS_OK;

OnError:
	/* Return the error. */
	return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryMemory
**
**  Query the amount of memory available on the hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctSIZE_T * InternalSize
**          Pointer to a variable that will hold the size of the internal video
**          memory in bytes.  If 'InternalSize' is gcvNULL, no information of the
**          internal memory will be returned.
**
**      gctUINT32 * InternalBaseAddress
**          Pointer to a variable that will hold the hardware's base address for
**          the internal video memory.  This pointer cannot be gcvNULL if
**          'InternalSize' is also non-gcvNULL.
**
**      gctUINT32 * InternalAlignment
**          Pointer to a variable that will hold the hardware's base address for
**          the internal video memory.  This pointer cannot be gcvNULL if
**          'InternalSize' is also non-gcvNULL.
**
**      gctSIZE_T * ExternalSize
**          Pointer to a variable that will hold the size of the external video
**          memory in bytes.  If 'ExternalSize' is gcvNULL, no information of the
**          external memory will be returned.
**
**      gctUINT32 * ExternalBaseAddress
**          Pointer to a variable that will hold the hardware's base address for
**          the external video memory.  This pointer cannot be gcvNULL if
**          'ExternalSize' is also non-gcvNULL.
**
**      gctUINT32 * ExternalAlignment
**          Pointer to a variable that will hold the hardware's base address for
**          the external video memory.  This pointer cannot be gcvNULL if
**          'ExternalSize' is also non-gcvNULL.
**
**      gctUINT32 * HorizontalTileSize
**          Number of horizontal pixels per tile.  If 'HorizontalTileSize' is
**          gcvNULL, no horizontal pixel per tile will be returned.
**
**      gctUINT32 * VerticalTileSize
**          Number of vertical pixels per tile.  If 'VerticalTileSize' is
**          gcvNULL, no vertical pixel per tile will be returned.
*/
gceSTATUS
gcoHARDWARE_QueryMemory(
    IN gcoHARDWARE Hardware,
    OUT gctSIZE_T * InternalSize,
    OUT gctUINT32 * InternalBaseAddress,
    OUT gctUINT32 * InternalAlignment,
    OUT gctSIZE_T * ExternalSize,
    OUT gctUINT32 * ExternalBaseAddress,
    OUT gctUINT32 * ExternalAlignment,
    OUT gctUINT32 * HorizontalTileSize,
    OUT gctUINT32 * VerticalTileSize
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (InternalSize != gcvNULL)
    {
        /* No internal memory. */
        *InternalSize = 0;
    }

    if (ExternalSize != gcvNULL)
    {
        /* No external memory. */
        *ExternalSize = 0;
    }

    if (HorizontalTileSize != gcvNULL)
    {
		/* 4x4 tiles. */
        *HorizontalTileSize = 4;
    }

    if (VerticalTileSize != gcvNULL)
    {
		/* 4x4 tiles. */
        *VerticalTileSize = 4;
    }

    /* Success. */
    return gcvSTATUS_OK;
}


gceSTATUS
gcoHARDWARE_QueryChipIdentity(
    IN gcoHARDWARE Hardware,
    OUT gceCHIPMODEL * ChipModel,
    OUT gctUINT32 * ChipRevision,
	OUT gctUINT32* ChipFeatures,
	OUT gctUINT32* ChipMinorFeatures
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Return chip model. */
    if (ChipModel != gcvNULL)
    {
    	*ChipModel = Hardware->chipModel;
    }

    /* Return revision number. */
    if (ChipRevision != gcvNULL)
    {
		*ChipRevision = Hardware->chipRevision;
    }

    /* Return feature set. */
    if (ChipFeatures != gcvNULL)
    {
    	gctUINT32 features = Hardware->chipFeatures;

		if (( ((((gctUINT32) (features)) >> (0 ? 0:0)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) ))
		{
			features = ((((gctUINT32) (features)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (Hardware->allowFastClear) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
   		}

		/* Mark 2D pipe as available for GC500.0 through GC500.2 and GC300,
		** since they did not have this bit. */
		if ((  (Hardware->chipModel == gcv500)
			&& (Hardware->chipRevision <= 2)
			)
		|| (Hardware->chipModel == gcv300)
		)
		{
			features = ((((gctUINT32) (features)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)));
		}

   		*ChipFeatures = features;
    }

    /* Return minor feature set. */
    if (ChipMinorFeatures != gcvNULL)
    {
		*ChipMinorFeatures = Hardware->chipMinorFeatures;
	}

    /* Success. */
    return gcvSTATUS_OK;
}

gceSTATUS
gcoHARDWARE_QueryChipSpecs(
	IN gcoHARDWARE Hardware,
	OUT gctUINT32_PTR StreamCount,
	OUT gctUINT32_PTR RegisterMax,
	OUT gctUINT32_PTR ThreadCount,
	OUT gctUINT32_PTR ShaderCoreCount,
	OUT gctUINT32_PTR VertexCacheSize,
	OUT gctUINT32_PTR VertexOutputBufferSize
	)
{
	/* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	/* Return the number of streams. */
	if (StreamCount != gcvNULL)
	{
		*StreamCount = Hardware->streamCount;
	}

	/* Return the number of temporary registers. */
	if (RegisterMax != gcvNULL)
	{
		*RegisterMax = Hardware->registerMax;
	}

	/* Return the maximum number of thrteads. */
	if (ThreadCount != gcvNULL)
	{
		*ThreadCount = Hardware->threadCount;
	}

	/* Return the number of shader cores. */
	if (ShaderCoreCount != gcvNULL)
	{
		*ShaderCoreCount = Hardware->shaderCoreCount;
	}

	/* Return the number of entries in the vertex cache. */
	if (VertexCacheSize != gcvNULL)
	{
		*VertexCacheSize = Hardware->vertexCacheSize;
	}

	/* Return the number of entries in the vertex output buffer. */
	if (VertexOutputBufferSize != gcvNULL)
	{
		*VertexOutputBufferSize = Hardware->vertexOutputBufferSize;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_ConvertFormat
**
**  Convert an API format to hardware parameters.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**      gceSURF_FORMAT Format
**          API format to convert.
**
**  OUTPUT:
**
**      gctUINT32 * BitsPerPixel
**          Pointer to a variable that will hold the number of bits per pixel.
**
**      gctUINT32 * BytesPerTile
**          Pointer to a variable that will hold the number of bytes per tile.
*/
gceSTATUS
gcoHARDWARE_ConvertFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT Format,
    OUT gctUINT32 * BitsPerPixel,
    OUT gctUINT32 * BytesPerTile
    )
{
	gctUINT32 bitsPerPixel;
	gctUINT32 bytesPerTile;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	/* Dispatch on format. */
	switch (Format)
	{
	case gcvSURF_INDEX8:
	case gcvSURF_A8:
	case gcvSURF_L8:
		/* 8-bpp format. */
		bitsPerPixel  = 8;
		bytesPerTile  = (8 * 4 * 4) / 8;
		break;

	case gcvSURF_YV12:
	case gcvSURF_I420:
	case gcvSURF_NV12:
	case gcvSURF_NV21:
		/* 12-bpp planar YUV formats. */
		bitsPerPixel  = 12;
		bytesPerTile  = (12 * 4 * 4) / 8;
		break;

	case gcvSURF_A8L8:
	case gcvSURF_X4R4G4B4:
	case gcvSURF_A4R4G4B4:
	case gcvSURF_X1R5G5B5:
	case gcvSURF_A1R5G5B5:
	case gcvSURF_R5G5B5X1:
	case gcvSURF_R4G4B4X4:
	case gcvSURF_X4B4G4R4:
	case gcvSURF_X1B5G5R5:
	case gcvSURF_B4G4R4X4:
	case gcvSURF_R5G6B5:
	case gcvSURF_B5G5R5X1:
	case gcvSURF_YUY2:
	case gcvSURF_UYVY:
	case gcvSURF_YVYU:
	case gcvSURF_VYUY:
	case gcvSURF_NV16:
	case gcvSURF_NV61:
	case gcvSURF_D16:
		/* 16-bpp format. */
		bitsPerPixel  = 16;
		bytesPerTile  = (16 * 4 * 4) / 8;
		break;

	case gcvSURF_X8R8G8B8:
	case gcvSURF_A8R8G8B8:
	case gcvSURF_X8B8G8R8:
	case gcvSURF_A8B8G8R8:
	case gcvSURF_R8G8B8X8:
	case gcvSURF_D32:
		/* 32-bpp format. */
		bitsPerPixel  = 32;
		bytesPerTile  = (32 * 4 * 4) / 8;
		break;

	case gcvSURF_D24S8:
	case gcvSURF_D24X8:
		/* 24-bpp format. */
		bitsPerPixel  = 32;
		bytesPerTile  = (32 * 4 * 4) / 8;
		break;

	case gcvSURF_DXT1:
	case gcvSURF_ETC1:
		bitsPerPixel  = 4;
		bytesPerTile  = (4 * 4 * 4) / 8;
		break;

	case gcvSURF_DXT2:
	case gcvSURF_DXT3:
	case gcvSURF_DXT4:
	case gcvSURF_DXT5:
		bitsPerPixel  = 8;
		bytesPerTile  = (8 * 4 * 4) / 8;
		break;

	default:
		/* Invalid format. */
		return gcvSTATUS_INVALID_ARGUMENT;
	}

	/* Set the result. */
	if (BitsPerPixel != gcvNULL)
	{
		* BitsPerPixel = bitsPerPixel;
	}

	if (BytesPerTile != gcvNULL)
	{
		* BytesPerTile = bytesPerTile;
	}

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_SplitMemory
**
**  Split a hardware specific memory address into a pool and offset.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**      gctUINT32 Address
**          Address in hardware specific format.
**
**  OUTPUT:
**
**      gcePOOL * Pool
**          Pointer to a variable that will hold the pool type for the address.
**
**      gctUINT32 * Offset
**          Pointer to a variable that will hold the offset for the address.
*/
gceSTATUS
gcoHARDWARE_SplitMemory(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    OUT gcePOOL * Pool,
    OUT gctUINT32 * Offset
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Pool != gcvNULL);
    gcmVERIFY_ARGUMENT(Offset != gcvNULL);

    /* Dispatch on memory type. */
    switch (( ((((gctUINT32) (Address)) >> (0 ? 31:31)) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) ))
    {
    case 0x0:
        /* System memory. */
        *Pool = gcvPOOL_SYSTEM;
        break;

    case 0x1:
        /* Virtual memory. */
        *Pool = gcvPOOL_VIRTUAL;
        break;

    default:
        /* Invalid memory type. */
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Return offset of address. */
    *Offset = ( ((((gctUINT32) (Address)) >> (0 ? 30:0)) & ((gctUINT32) ((((1 ? 30:0) - (0 ? 30:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:0) - (0 ? 30:0) + 1)))))) );

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_Execute
**
**  Kickstart the hardware's command processor with an initialized command
**  buffer.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Logical address of command buffer.
**
**      gctSIZE_T Bytes
**          Number of bytes for the prefetch unit (until after the first LINK).
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Execute(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;
    gctUINT32 address = 0, control;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

	do
	{
	    /* Convert logical into hardware specific address. */
		gcmERR_BREAK(gcoHARDWARE_ConvertLogical(Hardware, Logical, &address));

	   	/* Enable all events. */
   		gcmERR_BREAK(gcoOS_WriteRegister(Hardware->os,
										 0x00014,
										 ~0));

	    /* Write address register. */
		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
					  "Writing 0x%08X to AQ_CMD_BUFFER_ADDR\n", address);

		gcmERR_BREAK(gcoOS_WriteRegister(Hardware->os,
										 0x00654,
										 address));

        /* Build control register. */
        control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) ((Bytes+7)>>3) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        /* Write control register. */
		gcmERR_BREAK(gcoOS_WriteRegister(Hardware->os,
										 0x00658,
										 control));

		/* Success. */
		return gcvSTATUS_OK;
    }
	while (gcvFALSE);

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_WaitLink
**
**  Append a WAIT/LINK command sequence at the specified location in the command
**  queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          WAIT/LINK command sequence at or gcvNULL just to query the size of the
**          WAIT/LINK command sequence.
**
**		gctUINT32 Offset
**			Offset into command buffer required for alignment.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the WAIT/LINK command
**          sequence.  If 'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          by the WAIT/LINK command sequence.  If 'Bytes' is gcvNULL, nothing will
**          be returned.
**
**      gctPOINTER * Wait
**          Pointer to a variable that will receive the pointer to the WAIT
**          command.  If 'Wait' is gcvNULL nothing will be returned.
**
**      gctSIZE_T * WaitSize
**          Pointer to a variable that will receive the number of bytes used by
**          the WAIT command.  If 'LinkSize' is gcvNULL nothing will be returned.
*/
gceSTATUS
gcoHARDWARE_WaitLink(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
	IN gctUINT32 Offset,
    IN OUT gctSIZE_T * Bytes,
    OUT gctPOINTER * Wait,
    OUT gctSIZE_T * WaitSize
    )
{
    gceSTATUS status;
    gctUINT32 address;
	gctUINT32_PTR logical = (gctUINT32_PTR) Logical;
	gctSIZE_T bytes;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

	/* Compute number of bytes required. */
	bytes = gcmALIGN(Offset + 16, 64) - Offset;

	do
	{
	    if (Logical != gcvNULL)
		{
			/* Convert logical into hardware specific address. */
			gcmERR_BREAK(
				gcoHARDWARE_ConvertLogical(Hardware,
										   Logical,
										   &address));
                                                    

	        if (*Bytes < bytes)
		    {
			    /* Command queue too small. */
				status = gcvSTATUS_BUFFER_TOO_SMALL;
				break;
		    }

	        /* Append WAIT(200). */
		    logical[0]
			    = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (200) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

	        /* Append LINK(2, address). */
			gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
						  "Adding wait/link 0x%08X\n", address);
		    logical[2]
				= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x08 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (bytes>>3) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
	        logical[3] = address;

			if (Wait != gcvNULL)
			{
				/* Return pointer to WAIT command. */
				*Wait = Logical;
			}

			if (WaitSize != gcvNULL)
			{
				/* Return number of bytes used by the WAIT command. */
				*WaitSize = 8;
			}
		}

		if (Bytes != gcvNULL)
		{
			/* Return number of bytes required by the WAIT/LINK command
			** sequence. */
			*Bytes = bytes;
		}

	    /* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_End
**
**  Append an END command at the specified location in the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          END command at or gcvNULL just to query the size of the END command.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the END command.  If
**          'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the END command.  If 'Bytes' is gcvNULL, nothing will be returned.
*/
gceSTATUS
gcoHARDWARE_End(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN OUT gctSIZE_T * Bytes
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    if (Logical != gcvNULL)
    {
        if (*Bytes < 8)
        {
            /* Command queue too small. */
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* Append END. */
        *(gctUINT32 *) Logical =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
    }

    if (Bytes != gcvNULL)
    {
        /* Return number of bytes required by the END command. */
        *Bytes = 8;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_Nop
**
**  Append a NOP command at the specified location in the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          NOP command at or gcvNULL just to query the size of the NOP command.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the NOP command.  If
**          'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the NOP command.  If 'Bytes' is gcvNULL, nothing will be returned.
*/
gceSTATUS
gcoHARDWARE_Nop(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN OUT gctSIZE_T * Bytes
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    if (Logical != gcvNULL)
    {
        if (*Bytes < 8)
        {
            /* Command queue too small. */
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* Append NOP. */
        ((gctUINT32_PTR) Logical)[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
    }

    if (Bytes != gcvNULL)
    {
        /* Return number of bytes required by the NOP command. */
        *Bytes = 8;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_Wait
**
**  Append a WAIT command at the specified location in the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          WAIT command at or gcvNULL just to query the size of the WAIT command.
**
**		gctUINT32 Count
**			Number of cycles to wait.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the WAIT command.  If
**          'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the NOP command.  If 'Bytes' is gcvNULL, nothing will be returned.
*/
gceSTATUS
gcoHARDWARE_Wait(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
	IN gctUINT32 Count,
    IN OUT gctSIZE_T * Bytes
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    if (Logical != gcvNULL)
    {
        if (*Bytes < 8)
        {
            /* Command queue too small. */
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* Append WAIT. */
        *(gctUINT32 *) Logical = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
							   | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Count) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    if (Bytes != gcvNULL)
    {
        /* Return number of bytes required by the WAIT command. */
        *Bytes = 8;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_Event
**
**  Append an EVENT command at the specified location in the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          the EVENT command at or gcvNULL just to query the size of the EVENT
**          command.
**
**      gctUINT8 Event
**          Event ID to program.
**
**      gceKERNEL_WHERE FromWhere
**          Location of the pipe to send the event.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the EVENT command.  If
**          'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the EVENT command.  If 'Bytes' is gcvNULL, nothing will be
**          returned.
*/
gceSTATUS
gcoHARDWARE_Event(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT8 Event,
    IN gceKERNEL_WHERE FromWhere,
    IN OUT gctSIZE_T * Bytes
    )
{
    gctUINT32 destination;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));
    gcmVERIFY_ARGUMENT(Event < 32);

    if (Logical != gcvNULL)
    {
        if (*Bytes < 8)
        {
            /* Command queue too small. */
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

		switch (FromWhere)
		{
		case gcvKERNEL_COMMAND:
			/* From command processor. */
			destination = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1&((gctUINT32)((((1?5:5)-(0?5:5)+1)==32)?~0:(~(~0<<((1?5:5)-(0?5:5)+1)))))))<<(0?5:5)));			break;

		case gcvKERNEL_PIXEL:
			/* From pixel engine. */
			destination = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1&((gctUINT32)((((1?6:6)-(0?6:6)+1)==32)?~0:(~(~0<<((1?6:6)-(0?6:6)+1)))))))<<(0?6:6)));			break;

		default:
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Append EVENT(Event, destiantion). */
        ((gctUINT32 *) Logical)[0] =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))|            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E01) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))|            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
        ((gctUINT32 *) Logical)[1] = ((((gctUINT32) (destination)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (Event) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));
    }

    if (Bytes != gcvNULL)
    {
        /* Return number of bytes required by the EVENT command. */
        *Bytes = 8;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_PipeSelect
**
**  Append a PIPESELECT command at the specified location in the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          the PIPESELECT command at or gcvNULL just to query the size of the
**          PIPESELECT command.
**
**      gctUINT32 Pipe
**          Pipe value to select.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the PIPESELECT command.
**          If 'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the PIPESELECT command.  If 'Bytes' is gcvNULL, nothing will be
**          returned.
*/
gceSTATUS
gcoHARDWARE_PipeSelect(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT32 Pipe,
    IN OUT gctSIZE_T * Bytes
    )
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
	gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

	/* Append a PipeSelect. */
	if (Logical != gcvNULL)
	{
		gctUINT32 flush, stall;

	    if (*Bytes < 32)
	    {
	        /* Command queue too small. */
	        return gcvSTATUS_BUFFER_TOO_SMALL;
	    }

		flush = (Pipe == 0x1)
			  ? ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1&((gctUINT32)((((1?1:1)-(0?1:1)+1)==32)?~0:(~(~0<<((1?1:1)-(0?1:1)+1)))))))<<(0?1:1)))			  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1&((gctUINT32)((((1?0:0)-(0?0:0)+1)==32)?~0:(~(~0<<((1?0:0)-(0?0:0)+1)))))))<<(0?0:0)))			  : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1&((gctUINT32)((((1?3:3)-(0?3:3)+1)==32)?~0:(~(~0<<((1?3:3)-(0?3:3)+1)))))))<<(0?3:3)));
		stall = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
			  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

		/* LoadState(AQFlush, 1), flush. */
		((gctUINT32_PTR) Logical)[0]
			= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));		((gctUINT32_PTR) Logical)[1] = flush;

		/* LoadState(AQSempahore, 1), stall. */
		((gctUINT32_PTR) Logical)[2]
			= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));		((gctUINT32_PTR) Logical)[3] = stall;

		/* Stall, stall. */
		((gctUINT32_PTR) Logical)[4] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
		((gctUINT32_PTR) Logical)[5] = stall;

		/* LoadState(AQPipeSelect, 1), pipe. */
		((gctUINT32_PTR) Logical)[6]
			= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E00) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));		((gctUINT32_PTR) Logical)[7] = Pipe;
	}

	if (Bytes != gcvNULL)
	{
		/* Return number of bytes required by the PIPESELECT command. */
		*Bytes = 32;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_Link
**
**  Append a LINK command at the specified location in the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command queue to append
**          the LINK command at or gcvNULL just to query the size of the LINK
**          command.
**
**      gctPOINTER FetchAddress
**          Logical address of destination of LINK.
**
**      gctSIZE_T FetchSize
**          Number of bytes in destination of LINK.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the LINK command.  If
**          'Logical' is gcvNULL, this argument will be ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the LINK command.  If 'Bytes' is gcvNULL, nothing will be returned.
*/
gceSTATUS
gcoHARDWARE_Link(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctPOINTER FetchAddress,
    IN gctSIZE_T FetchSize,
    IN OUT gctSIZE_T * Bytes
    )
{
    gceSTATUS status;
	gctSIZE_T bytes;
	gctUINT32 address;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    if (Logical != gcvNULL)
    {
        if (*Bytes < 8)
        {
            /* Command queue too small. */
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* Convert logical address to hardware address. */
		gcmERR_RETURN(
			gcoHARDWARE_ConvertLogical(Hardware,
									   FetchAddress,
									   &address));

		((gctUINT32_PTR) Logical)[1] = address;

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
					  "Adding link 0x%08X\n",
					  address);

        /* Make sure the address got written before the LINK command. */
        gcmVERIFY_OK(
			gcoOS_MemoryBarrier(Hardware->os,
								&((gctUINT32 *) Logical)[1]));

		/* Compute number of 64-byte aligned bytes to fetch. */
		bytes = gcmALIGN(address + FetchSize, 64) - address;

        /* Append LINK(bytes / 8), FetchAddress. */
        *(gctUINT32 *) Logical = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x08 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
							   | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (bytes>>3) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    if (Bytes != gcvNULL)
    {
        /* Return number of bytes required by the LINK command. */
        *Bytes = 8;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_AlignToTile
**
**  Align the specified width and height to tile boundaries.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**		gceSURF_TYPE Type
**			Type of alignment.
**
**      gctUINT32 * Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32 * Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32 * Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32 * Height
**          Pointer to a variable that will receive the aligned height.
**
**		gctBOOL_PTR SuperTiled
**			Pointer to a variable that receives the super-tiling flag for the
**			surface.
*/
gceSTATUS
gcoHARDWARE_AlignToTile(
    IN gcoHARDWARE Hardware,
	IN gceSURF_TYPE Type,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
	OUT gctBOOL_PTR SuperTiled
    )
{
	gctBOOL superTiled = gcvFALSE;
	gctUINT32 xAlignment, yAlignment;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);


	/* Compute alignment factors. */
	xAlignment = superTiled ? 64
			   : (Type == gcvSURF_TEXTURE) ? 4
			   : 16;
	yAlignment = superTiled ? 64 : 4;

    if (Width != gcvNULL)
    {
		/* Align the width. */
		*Width = gcmALIGN(*Width, xAlignment);
    }

    if (Height != gcvNULL)
    {
		/* Align the height. */
		*Height = gcmALIGN(*Height, yAlignment);
    }

	if (SuperTiled != gcvNULL)
	{
		/* Copy the super tiling. */
		*SuperTiled = superTiled;
	}

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARWDARE_UpdateQueueTail
**
**  Update the tail of the command queue.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Logical address of the start of the command queue.
**
**      gctUINT32 Offset
**          Offset into the command queue of the tail (last command).
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARWDARE_UpdateQueueTail(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset
    )
{
    /* Verify the hardware. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Notify gcoKERNEL object of change. */
    return gcoKERNEL_Notify(Hardware->kernel,
							gcvNOTIFY_COMMAND_QUEUE,
							gcvFALSE);
}

/*******************************************************************************
**
**  gcoHARDWARE_ConvertLogical
**
**  Convert a logical system address into a hardware specific address.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER Logical
**          Logical address to convert.
**
**      gctUINT32* Address
**          Return hardware specific address.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ConvertLogical(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    OUT gctUINT32 * Address
    )
{
    gctUINT32 address;
    gceSTATUS status;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);
    gcmVERIFY_ARGUMENT(Address != gcvNULL);

	do
	{
	    /* Convert logical address into a physical address. */
		gcmERR_BREAK(gcoOS_GetPhysicalAddress(Hardware->os,
											  Logical,
											  &address));
                                              
                                              
#if MRVL_TAVOR_PV2_PATCH                                              
        address = address & 0x7fffffff;
        /* In fact, below code will clear the highest bit too. */
#endif

	    /* Return hardware specific address. */
		*Address = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)))
			     | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:0) - (0 ? 30:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:0) - (0 ? 30:0) + 1))))))) << (0 ? 30:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ? 30:0) - (0 ? 30:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:0) - (0 ? 30:0) + 1))))))) << (0 ? 30:0)));

	    /* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Interrupt
**
**  Process an interrupt.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**		gctBOOL InterruptValid
**			If gcvTRUE, this function will read the interrupt acknowledge register,
**			stores the data, and return whether or not the interrupt is ours or
**			not.  If gcvFALSE, this functions will read the interrupt acknowledge
**			register and combine it with any stored value to handle the event
**			notifications.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Interrupt(
    IN gcoHARDWARE Hardware,
	IN gctBOOL InterruptValid
    )
{
    gcoEVENT event;
	gctUINT32 data;
    gceSTATUS status;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Extract gcoEVENT object. */
    event = Hardware->kernel->event;
    gcmVERIFY_OBJECT(event, gcvOBJ_EVENT);

#if USE_EVENT_QUEUE
	if (InterruptValid)
	{
		/* Read AQIntrAcknowledge register. */
		gcmVERIFY_OK(gcoOS_ReadRegister(Hardware->os,
										0x00010,
									    &data));

		if (data == 0)
		{
			/* Not our interrupt. */
			status = gcvSTATUS_NOT_OUR_INTERRUPT;
		}
		else
		{
			/* Inform gcoEVENT of the interrupt. */
			status = gcoEVENT_Interrupt(event, data);
		}
	}
	else
	{
		/* Handle events. */
		status = gcoEVENT_Notify(event, 0);
	}

	/* Return the status. */
	return status;
#else
	/* Read AQIntrAcknowledge register. */
	gcmVERIFY_OK(gcoOS_ReadRegister(Hardware->os,
									0x00010,
								    &data));

	if (InterruptValid)
	{
		/* Store data. */
		Hardware->data |= data;

		/* Return interrupt status. */
		return (data == 0) ? gcvSTATUS_NOT_OUR_INTERRUPT : gcvSTATUS_OK;
	}

	{
		gctUINT32 storedData;

		/* Combine with stored interrupt acknowledge register. */
		gcmVERIFY_OK(gcoOS_AtomicExchange(Hardware->os,
										  &Hardware->data,
										  0,
										  &storedData));
		data |= storedData;
	}

	/* Check all bits. */
	status = gcoEVENT_Notify(event, data);

    /* Success. */
    return gcvSTATUS_OK;
#endif
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryCommandBuffer
**
**	Query the command buffer alignment and number of reserved bytes.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**	OUTPUT:
**
**		gctSIZE_T * Alignment
**			Pointer to a variable receiving the alignment for each command.
**
**		gctSIZE_T * ReservedHead
**			Pointer to a variable receiving the number of reserved bytes at the
**          head of each command buffer.
**
**		gctSIZE_T * ReservedTail
**			Pointer to a variable receiving the number of bytes reserved at the
**          tail of each command buffer.
*/
gceSTATUS gcoHARDWARE_QueryCommandBuffer(
    IN gcoHARDWARE Hardware,
    OUT gctSIZE_T * Alignment,
    OUT gctSIZE_T * ReservedHead,
    OUT gctSIZE_T * ReservedTail
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Alignment != gcvNULL)
    {
        /* Align every 8 bytes. */
        *Alignment = 8;
    }

    if (ReservedHead != gcvNULL)
    {
        /* Reserve space for SelectPipe(). */
        *ReservedHead = 32;
    }

    if (ReservedTail != gcvNULL)
    {
        /* Reserve space for Link(). */
        *ReservedTail = 8;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_QuerySystemMemory
**
**	Query the command buffer alignment and number of reserved bytes.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**	OUTPUT:
**
**		gctSIZE_T * SystemSize
**			Pointer to a variable that receives the maximum size of the system
**          memory.
**
**		gctUINT32 * SystemBaseAddress
**			Poinetr to a variable that receives the base address for system
**			memory.
*/
gceSTATUS gcoHARDWARE_QuerySystemMemory(
    IN gcoHARDWARE Hardware,
    OUT gctSIZE_T * SystemSize,
	OUT gctUINT32 * SystemBaseAddress
    )
{
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (SystemSize != gcvNULL)
    {
        /* Maximum system memory can be 2GB. */
        *SystemSize = 1 << 31;
    }

	if (SystemBaseAddress != gcvNULL)
	{
		/* Set system memory base address. */
		*SystemBaseAddress = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
	}

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_SetMMU
**
**	Set the page table base address.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**		gctPOINTER Logical
**			Logical address of the page table.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoHARDWARE_SetMMU(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER Logical
	)
{
	gceSTATUS status;
	gctUINT32 address = 0;
	gctUINT32 baseAddress;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
	gcmVERIFY_ARGUMENT(Logical != gcvNULL);

	do
	{
		/* Convert the logical address into an hardware address. */
		gcmERR_BREAK(
			gcoHARDWARE_ConvertLogical(Hardware, Logical, &address));

		/* Also get the base address - we need a real physical address. */
		gcmERR_BREAK(
			gcoOS_GetBaseAddress(Hardware->os, &baseAddress));

		gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
					  "Setting page table to 0x%08X\n",
					  address + baseAddress);

		/* Write the AQMemoryFePageTable register. */
		gcmERR_BREAK(
			gcoOS_WriteRegister(Hardware->os,
								0x00400,
								address + baseAddress));

		/* Write the AQMemoryRaPageTable register. */
		gcmERR_BREAK(
			gcoOS_WriteRegister(Hardware->os,
								0x00410,
								address + baseAddress));

		/* Write the AQMemoryTxPageTable register. */
		gcmERR_BREAK(
			gcoOS_WriteRegister(Hardware->os,
								0x00404,
								address + baseAddress));

		/* Write the AQMemoryPePageTable register. */
		gcmERR_BREAK(
			gcoOS_WriteRegister(Hardware->os,
								0x00408,
								address + baseAddress));

		/* Write the AQMemoryPezPageTable register. */
		gcmERR_BREAK(
			gcoOS_WriteRegister(Hardware->os,
								0x0040C,
								address + baseAddress));
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_FlushMMU
**
**	Flush the page table.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoHARDWARE_FlushMMU(
	IN gcoHARDWARE Hardware
	)
{
	gceSTATUS status;
	gctUINT32 flush;
	gctPOINTER buffer;
	gctSIZE_T bufferSize;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	do
	{
		/* Flush the memory controller. */
		flush = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1&((gctUINT32)((((1?0:0)-(0?0:0)+1)==32)?~0:(~(~0<<((1?0:0)-(0?0:0)+1)))))))<<(0?0:0)))			  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1&((gctUINT32)((((1?1:1)-(0?1:1)+1)==32)?~0:(~(~0<<((1?1:1)-(0?1:1)+1)))))))<<(0?1:1)))			  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))) | (((gctUINT32) (0x1&((gctUINT32)((((1?2:2)-(0?2:2)+1)==32)?~0:(~(~0<<((1?2:2)-(0?2:2)+1)))))))<<(0?2:2)))			  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1&((gctUINT32)((((1?3:3)-(0?3:3)+1)==32)?~0:(~(~0<<((1?3:3)-(0?3:3)+1)))))))<<(0?3:3)))			  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x1&((gctUINT32)((((1?4:4)-(0?4:4)+1)==32)?~0:(~(~0<<((1?4:4)-(0?4:4)+1)))))))<<(0?4:4)));
		gcmERR_BREAK(gcoCOMMAND_Reserve(Hardware->kernel->command,
										8,
										&buffer,
										&bufferSize));

		((gctUINT32 *) buffer)[0] =
			((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))|			((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E04) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))|			((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
		((gctUINT32 *) buffer)[1] = flush;

		gcmERR_BREAK(gcoCOMMAND_Execute(Hardware->kernel->command,
										8));
	}
	while(gcvFALSE);

	/* Return the status. */
	return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_BuildVirtualAddress
**
**	Build a virtual address.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**		gctUINT32 Index
**			Index into page table.
**
**		gctUINT32 Offset
**			Offset into page.
**
**	OUTPUT:
**
**		gctUINT32 * Address
**			Pointer to a variable receiving te hardware address.
*/
gceSTATUS gcoHARDWARE_BuildVirtualAddress(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Index,
	IN gctUINT32 Offset,
	OUT gctUINT32 * Address
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
	gcmVERIFY_ARGUMENT(Address != gcvNULL);

	/* Build virtual address. */
	*Address = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)))|
			   ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:0) - (0 ? 30:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:0) - (0 ? 30:0) + 1))))))) << (0 ? 30:0))) | (((gctUINT32) ((gctUINT32) (Offset|(Index<<12)) & ((gctUINT32) ((((1 ? 30:0) - (0 ? 30:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:0) - (0 ? 30:0) + 1))))))) << (0 ? 30:0)));

	/* Success. */
	return gcvSTATUS_OK;
}

gceSTATUS
gcoHARDWARE_GetIdle(
	IN gcoHARDWARE Hardware,
	OUT gctUINT32 * Data
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
	gcmVERIFY_ARGUMENT(Data != gcvNULL);

	/* Read register and return. */
	return gcoOS_ReadRegister(Hardware->os, 0x00004, Data);
}

/* Flush the caches. */
gceSTATUS
gcoHARDWARE_Flush(
	IN gcoHARDWARE Hardware,
	IN gceKERNEL_FLUSH Flush,
    IN gctPOINTER Logical,
    IN OUT gctSIZE_T * Bytes
	)
{
	gctUINT32 pipe;
	gctUINT32 flush = 0;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	/* Get current pipe. */
	pipe = Hardware->kernel->command->pipeSelect;

	/* Flush 3D color cache. */
	if ((Flush & gcvFLUSH_COLOR) && (pipe == 0x0))
	{
		flush |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1&((gctUINT32)((((1?1:1)-(0?1:1)+1)==32)?~0:(~(~0<<((1?1:1)-(0?1:1)+1)))))))<<(0?1:1)));	}

	/* Flush 3D depth cache. */
	if ((Flush & gcvFLUSH_DEPTH) && (pipe == 0x0))
	{
		flush |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1&((gctUINT32)((((1?0:0)-(0?0:0)+1)==32)?~0:(~(~0<<((1?0:0)-(0?0:0)+1)))))))<<(0?0:0)));	}

	/* Flush 3D texture cache. */
	if ((Flush & gcvFLUSH_TEXTURE) && (pipe == 0x0))
	{
		flush |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))) | (((gctUINT32) (0x1&((gctUINT32)((((1?2:2)-(0?2:2)+1)==32)?~0:(~(~0<<((1?2:2)-(0?2:2)+1)))))))<<(0?2:2)));	}

	/* Flush 2D cache. */
	if ((Flush & gcvFLUSH_2D) && (pipe == 0x1))
	{
		flush |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1&((gctUINT32)((((1?3:3)-(0?3:3)+1)==32)?~0:(~(~0<<((1?3:3)-(0?3:3)+1)))))))<<(0?3:3)));	}

	/* See if there is a valid flush. */
	if (flush == 0)
	{
		if (Bytes != gcvNULL)
		{
			/* No bytes required. */
			*Bytes = 0;
		}

		/* Success. */
		return gcvSTATUS_OK;
	}

	/* Copy to command queue. */
	if (Logical != gcvNULL)
	{
        if (*Bytes < 8)
        {
            /* Command queue too small. */
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* Append LOAD_STATE to AQFlush. */
        ((gctUINT32 *) Logical)[0]
			= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))			| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
        ((gctUINT32 *) Logical)[1] = flush;
	}

	if (Bytes != gcvNULL)
	{
		/* 8 bytes required. */
		*Bytes = 8;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

gceSTATUS
gcoHARDWARE_SetFastClear(
    IN gcoHARDWARE Hardware,
    IN gctINT Enable
    )
{
    gctUINT32 debug;
    gceSTATUS status;

    if (!( ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 0:0)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) ))
    {
    	return gcvSTATUS_OK;
    }

    do
    {
    	if (Enable == -1)
    	{
    	    Enable = (Hardware->chipModel != gcv500)
				   | (Hardware->chipRevision >= 3);
    	}

#if MRVL_TAVOR_PV2_DISABLE_FASTCLEAR
	if( (Hardware->chipModel== 0x530) && (Hardware->chipRevision == 4) )
	{
		Enable = 0;
	}

	if( (Hardware->chipModel== 0x800) && (Hardware->chipRevision == 0x4301) )
	{
		Enable = 0;
	}    
#endif 

        gcmERR_BREAK(gcoOS_ReadRegister(Hardware->os,
    	    	    	    	    	0x00414,
										&debug));

    	debug = ((((gctUINT32) (debug)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))) | (((gctUINT32) ((gctUINT32) (Enable==0) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)));

    	debug = ((((gctUINT32) (debug)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (Enable==0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));

    	gcmERR_BREAK(
			gcoOS_WriteRegister(Hardware->os,
				    	    	0x00414,
								debug));

        Hardware->allowFastClear = Enable;

    	status = gcvFALSE;
    }
    while (gcvFALSE);

    return status;
}

#define gcmREAD_REGISTER(addr, data) \
	gcoOS_ReadRegister(Hardware->os, (addr), (data))

#define gcmWRITE_REGISTER(addr, _register) \
	gcoOS_WriteRegister(Hardware->os, (addr), (_register))

/*******************************************************************************
**
**	gcoHARDWARE_SetPowerManagementState
**
**	Set GPU to a specified power state.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**		gceCHIPPOWERSTATE State
**			Power State.
**
*/
gceSTATUS
gcoHARDWARE_SetPowerManagementState(
    IN gcoHARDWARE Hardware,
    IN gceCHIPPOWERSTATE State
    )
{
	gctUINT32 control;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Redundant State change */
    if (Hardware->chipPowerState == State)
	{
        return gcvSTATUS_OK;
	}

    if (Hardware->chipPowerState == gcvPOWER_ON)
    {
		gceSTATUS status;

		/* Acquire the context switching mutex. */
		gcmERR_RETURN(
			gcoOS_AcquireMutex(Hardware->kernel->command->os,
							   Hardware->kernel->command->mutexContext,
							   gcvINFINITE));

        /* Wait to finish all commands. */
        gcmVERIFY_OK(
			gcoCOMMAND_Stall(Hardware->kernel->command));
    }

    else if (Hardware->chipPowerState == gcvPOWER_OFF)
    {
        /* Reset context ID to force context flush. */
		gcmASSERT(Hardware->kernel != gcvNULL);
		gcmASSERT(Hardware->kernel->command != gcvNULL);
        Hardware->kernel->command->currentContext = 0;
    }

    switch (State)
    {
    case gcvPOWER_ON:
        /* Enable all clocks, disable memory clock gating, frequency scaling to
		** full speed. */
		control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:2) - (0 ? 8:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:2) - (0 ? 8:2) + 1))))))) << (0 ? 8:2))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 8:2) - (0 ? 8:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:2) - (0 ? 8:2) + 1))))))) << (0 ? 8:2)));

		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (control)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))));
		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (control)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))));

        /* Sleep for 10ms, to make sure everything is powered on */
        gcoOS_Delay(Hardware->os, 10);
        break;

    case gcvPOWER_SUSPEND:
    case gcvPOWER_OFF:
        /* Disable all clocks. */
        control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));

		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								control));
        break;

    case gcvPOWER_IDLE:
        /* Enable all clocks, enable memory clock gating, frequency scaling to
		** 1/64th. */
		control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:2) - (0 ? 8:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:2) - (0 ? 8:2) + 1))))))) << (0 ? 8:2))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 8:2) - (0 ? 8:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:2) - (0 ? 8:2) + 1))))))) << (0 ? 8:2)));

		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (control)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))));
		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (control)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))));
        break;
    }

	if (Hardware->chipPowerState == gcvPOWER_ON)
	{
		/* Release the context switching mutex. */
		gcmVERIFY_OK(
			gcoOS_ReleaseMutex(Hardware->kernel->command->os,
							   Hardware->kernel->command->mutexContext));
	}

	else if (Hardware->chipPowerState == gcvPOWER_OFF)
	{
		gcmVERIFY_OK(
			gcoHARDWARE_InitializeHardware(Hardware));
	}

	/* Save new power state. */
    Hardware->chipPowerState = State;

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_NotifyPower
**
**	Notify the transition of the power state.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**		gctBOOL PowerOn
**			Power on or off.
**
*/
gceSTATUS
gcoHARDWARE_NotifyPower(
    IN gcoHARDWARE Hardware,
	IN gctBOOL PowerOn
	)
{
	gceSTATUS status;
	/* gctUINT32 control; */

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	if (PowerOn)
	{
		/* Check the current power state */
		if (Hardware->chipPowerState != gcvPOWER_OFF)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}
#if 0
		/* Enable all clocks, disable memory clock gating, frequency scaling to
		** full speed. */
		control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)))

				| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:2) - (0 ? 8:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:2) - (0 ? 8:2) + 1))))))) << (0 ? 8:2))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 8:2) - (0 ? 8:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:2) - (0 ? 8:2) + 1))))))) << (0 ? 8:2)));

		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (control)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))));
		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (control)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))));

		/* Sleep for 10ms, to make sure everything is powered on */
		gcmVERIFY_OK(
			gcoOS_Delay(Hardware->os, 10));
#endif
		/* Reset context ID to force context flush */
		Hardware->kernel->command->currentContext = 0;

		/* Set power state to enabled. */
		Hardware->chipPowerState = gcvPOWER_ON;

		/* Initialize the hardware. */
		gcmVERIFY_OK(
			gcoHARDWARE_InitializeHardware(Hardware));

		/* Reset MMU. */
		gcmVERIFY_OK(
			gcoHARDWARE_SetMMU(Hardware,
							   Hardware->kernel->mmu->pageTableLogical));

		/* Restart the command queue. */
		gcmVERIFY_OK(
			gcoCOMMAND_Start(Hardware->kernel->command));

		/* Release the context switching mutex. */
		gcmVERIFY_OK(
			gcoOS_ReleaseMutex(Hardware->kernel->command->os,
							   Hardware->kernel->command->mutexContext));
	}
	else
	{
		/* Check the current power state */
		if (Hardware->chipPowerState != gcvPOWER_ON)
		{
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Acquire the context switching mutex. */
		gcmERR_RETURN(
			gcoOS_AcquireMutex(Hardware->kernel->command->os,
							   Hardware->kernel->command->mutexContext,
							   gcvINFINITE));

		/* Wait to finish all commands. */
		gcmVERIFY_OK(
			gcoCOMMAND_Stall(Hardware->kernel->command));

		/* Stop the command queue. */
		gcmVERIFY_OK(
			gcoCOMMAND_Stop(Hardware->kernel->command));

		/* Disable 2D/3D clock */
		gcmVERIFY_OK(
			gcoOS_WriteRegister(Hardware->os,
								0x00000,
								((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))

								| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))));

		/* Set power state to OFF. */
		Hardware->chipPowerState = gcvPOWER_OFF;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryPowerManagementState
**
**	Get GPU power state.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**		gceCHIPPOWERSTATE* State
**			Power State.
**
*/
gceSTATUS
gcoHARDWARE_QueryPowerManagementState(
    IN gcoHARDWARE Hardware,
    OUT gceCHIPPOWERSTATE* State
    )
{
    *State = Hardware->chipPowerState;

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_ProfileEngine2D
**
**	Read the profile registers available in the 2D engine and sets them in the profile.
**	The function will also reset the pixelsRendered counter every time.
**
**
**	INPUT:
**
**		gcoHARDWARE Hardware
**			Pointer to an gcoHARDWARE object.
**
**		OPTIONAL gco2D_PROFILE_PTR Profile
**			Pointer to a gco2D_Profile structure.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gcoHARDWARE_ProfileEngine2D(
	IN gcoHARDWARE Hardware,
	OPTIONAL gco2D_PROFILE_PTR Profile
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	if (Profile != gcvNULL)
	{
		/* Read the cycle count. */
		gcmREAD_REGISTER(0x00438, &Profile->cycleCount);

		/* Read the pixels rendered. */
		gcmREAD_REGISTER(0x00454, &Profile->pixelsRendered);
	}

	/* Reset pixelsRendered counter. */
	gcmWRITE_REGISTER(0x00470, 0xF0000);

	/* Select pixelsRendered counter to be read. */
	gcmWRITE_REGISTER(0x00470, 0xB0000);

	/* Return status. */
	return gcvSTATUS_OK;
}

#if VIVANTE_PROFILER

gceSTATUS
gcoHARDWARE_QueryProfileRegisters(
	IN gcoHARDWARE Hardware,
    OUT gctINT32_PTR HWProfile
	)
{
    gctUINT32 Signature;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    gcoHWProfile hwProfile = (gcoHWProfile)HWProfile;

    /* Read the counters */
    gcmREAD_REGISTER(0x00040, &hwProfile->gpuTotalRead64BytesPerFrame);
    gcmREAD_REGISTER(0x00044, &hwProfile->gpuTotalWrite64BytesPerFrame);

    gcmREAD_REGISTER(0x00438, &hwProfile->gpuCyclesCounter);

    /* Reset counters and stop counting */
    gcmWRITE_REGISTER(0x0003C, 0x00000001);
    /* start counting again. */
    gcmWRITE_REGISTER(0x0003C, 0x00000000);

    /* Counters not described in AQMemory.r have been disabled for now. */

    /* FE */
    /*gcmWRITE_REGISTER(0x00470, 0x00000000);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_0);
    gcmWRITE_REGISTER(0x00470, 0x00000001);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_1);
    gcmWRITE_REGISTER(0x00470, 0x00000002);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_2);
    gcmWRITE_REGISTER(0x00470, 0x00000003);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_3);
    gcmWRITE_REGISTER(0x00470, 0x00000004);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_4);
    gcmWRITE_REGISTER(0x00470, 0x00000005);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_5);
    gcmWRITE_REGISTER(0x00470, 0x00000006);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_6);
    gcmWRITE_REGISTER(0x00470, 0x00000007);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_7);
    gcmWRITE_REGISTER(0x00470, 0x00000008);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_8);
    gcmWRITE_REGISTER(0x00470, 0x00000009);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_9);
    gcmWRITE_REGISTER(0x00470, 0x0000000A);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_10);
    gcmWRITE_REGISTER(0x00470, 0x0000000B);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_11);
    gcmWRITE_REGISTER(0x00470, 0x0000000C);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_12);
    gcmWRITE_REGISTER(0x00470, 0x0000000D);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_13);
    gcmWRITE_REGISTER(0x00470, 0x0000000E);
    gcmREAD_REGISTER(0x00450, &hwProfile->fe_counter_14);*/

    /* DE */
    /*gcmWRITE_REGISTER(0x00470, 0x00000000);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_0);
    gcmWRITE_REGISTER(0x00470, 0x00000100);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_1);
    gcmWRITE_REGISTER(0x00470, 0x00000200);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_2);
    gcmWRITE_REGISTER(0x00470, 0x00000300);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_3);
    gcmWRITE_REGISTER(0x00470, 0x00000400);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_4);
    gcmWRITE_REGISTER(0x00470, 0x00000500);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_5);
    gcmWRITE_REGISTER(0x00470, 0x00000600);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_6);
    gcmWRITE_REGISTER(0x00470, 0x00000700);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_7);
    gcmWRITE_REGISTER(0x00470, 0x00000800);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_8);
    gcmWRITE_REGISTER(0x00470, 0x00000900);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_9);
    gcmWRITE_REGISTER(0x00470, 0x00000A00);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_10);
    gcmWRITE_REGISTER(0x00470, 0x00000B00);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_11);
    gcmWRITE_REGISTER(0x00470, 0x00000C00);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_12);
    gcmWRITE_REGISTER(0x00470, 0x00000D00);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_13);
    gcmWRITE_REGISTER(0x00470, 0x00000E00);
    gcmREAD_REGISTER(0x00458, &hwProfile->de_counter_14);*/

    /* PE */
    gcmWRITE_REGISTER(0x00470, 0x00000000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_color_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_color_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_color_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_color_pipe);
    gcmWRITE_REGISTER(0x00470, 0x00010000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_depth_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_depth_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_depth_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_killed_by_depth_pipe);
    gcmWRITE_REGISTER(0x00470, 0x00020000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_color_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_color_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_color_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_color_pipe);
    gcmWRITE_REGISTER(0x00470, 0x00030000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_depth_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_depth_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_depth_pipe);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_pixel_count_drawn_by_depth_pipe);
    /*gcmWRITE_REGISTER(0x00470, 0x00040000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_4);
    gcmWRITE_REGISTER(0x00470, 0x00050000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_5);
    gcmWRITE_REGISTER(0x00470, 0x00060000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_6);
    gcmWRITE_REGISTER(0x00470, 0x00070000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_7);
    gcmWRITE_REGISTER(0x00470, 0x00080000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_8);
    gcmWRITE_REGISTER(0x00470, 0x00090000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_9);
    gcmWRITE_REGISTER(0x00470, 0x000A0000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_10);
    gcmWRITE_REGISTER(0x00470, 0x000B0000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_11);
    gcmWRITE_REGISTER(0x00470, 0x000C0000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_12);
    gcmWRITE_REGISTER(0x00470, 0x000D0000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_13);
    gcmWRITE_REGISTER(0x00470, 0x000E0000);
    gcmREAD_REGISTER(0x00454, &hwProfile->pe_counter_14);*/

    /* SH */
    /*gcmWRITE_REGISTER(0x00470, 0x00000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_0);
    gcmWRITE_REGISTER(0x00470, 0x01000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_1);
    gcmWRITE_REGISTER(0x00470, 0x02000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_2);
    gcmWRITE_REGISTER(0x00470, 0x03000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_3);
    gcmWRITE_REGISTER(0x00470, 0x04000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_4);
    gcmWRITE_REGISTER(0x00470, 0x05000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_5);
    gcmWRITE_REGISTER(0x00470, 0x06000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->sh_counter_6);
	*/
    gcmWRITE_REGISTER(0x00470, 0x07000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->ps_inst_counter);
    gcmWRITE_REGISTER(0x00470, 0x08000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->rendered_pixel_counter);
    gcmWRITE_REGISTER(0x00470, 0x09000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->vs_inst_counter);
    gcmWRITE_REGISTER(0x00470, 0x0A000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->rendered_vertice_counter);
    gcmWRITE_REGISTER(0x00470, 0x0B000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->vtx_branch_inst_counter);
    gcmWRITE_REGISTER(0x00470, 0x0C000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->vtx_texld_inst_counter);
    gcmWRITE_REGISTER(0x00470, 0x0D000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->pxl_branch_inst_counter);
    gcmWRITE_REGISTER(0x00470, 0x0E000000);
    gcmREAD_REGISTER(0x0045C, &hwProfile->pxl_texld_inst_counter);

    /* PA */
	/*
    gcmWRITE_REGISTER(0x00474, 0x00000000);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_pixel_count_killed_by_color_pipe);
    gcmWRITE_REGISTER(0x00474, 0x00000001);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_pixel_count_killed_by_depth_pipe);
    gcmWRITE_REGISTER(0x00474, 0x00000002);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_pixel_count_drawn_by_color_pipe);
	*/
    gcmWRITE_REGISTER(0x00474, 0x00000003);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_input_vtx_counter);
    gcmWRITE_REGISTER(0x00474, 0x00000004);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_input_prim_counter);
    gcmWRITE_REGISTER(0x00474, 0x00000005);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_output_prim_counter);
    gcmWRITE_REGISTER(0x00474, 0x00000006);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_depth_clipped_counter);
    gcmWRITE_REGISTER(0x00474, 0x00000007);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_trivial_rejected_counter);
    gcmWRITE_REGISTER(0x00474, 0x00000008);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_culled_counter);
	/*
    gcmWRITE_REGISTER(0x00474, 0x00000009);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_counter_9);
    gcmWRITE_REGISTER(0x00474, 0x0000000A);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_counter_10);
    gcmWRITE_REGISTER(0x00474, 0x0000000B);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_counter_11);
    gcmWRITE_REGISTER(0x00474, 0x0000000C);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_counter_12);
    gcmWRITE_REGISTER(0x00474, 0x0000000D);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_counter_13);
    gcmWRITE_REGISTER(0x00474, 0x0000000E);
    gcmREAD_REGISTER(0x00460, &hwProfile->pa_counter_14);*/

    /* Select SE.*/

    gcmWRITE_REGISTER(0x00474, 0x00000000);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_culled_triangle_count);
    gcmWRITE_REGISTER(0x00474, 0x00000100);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_culled_lines_count);
	/*
    gcmWRITE_REGISTER(0x00474, 0x00000200);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_2);
    gcmWRITE_REGISTER(0x00474, 0x00000300);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_3);
    gcmWRITE_REGISTER(0x00474, 0x00000400);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_4);
    gcmWRITE_REGISTER(0x00474, 0x00000500);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_5);
    gcmWRITE_REGISTER(0x00474, 0x00000600);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_6);
    gcmWRITE_REGISTER(0x00474, 0x00000700);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_7);
    gcmWRITE_REGISTER(0x00474, 0x00000800);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_8);
    gcmWRITE_REGISTER(0x00474, 0x00000900);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_9);
    gcmWRITE_REGISTER(0x00474, 0x00000A00);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_10);
    gcmWRITE_REGISTER(0x00474, 0x00000B00);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_11);
    gcmWRITE_REGISTER(0x00474, 0x00000C00);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_12);
    gcmWRITE_REGISTER(0x00474, 0x00000D00);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_13);
    gcmWRITE_REGISTER(0x00474, 0x00000E00);
    gcmREAD_REGISTER(0x00464, &hwProfile->se_counter_14);*/

    /* Select RA.*/
    gcmWRITE_REGISTER(0x00474, 0x00000000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_valid_pixel_count);
    gcmWRITE_REGISTER(0x00474, 0x00010000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_total_quad_count);
    gcmWRITE_REGISTER(0x00474, 0x00020000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_valid_quad_count_after_early_z);
    gcmWRITE_REGISTER(0x00474, 0x00030000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_total_primitive_count);
    /*gcmWRITE_REGISTER(0x00474, 0x00040000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_4);
    gcmWRITE_REGISTER(0x00474, 0x00050000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_5);
    gcmWRITE_REGISTER(0x00474, 0x00060000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_6);
    gcmWRITE_REGISTER(0x00474, 0x00070000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_7);
    gcmWRITE_REGISTER(0x00474, 0x00080000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_8);
	*/
    gcmWRITE_REGISTER(0x00474, 0x00090000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_pipe_cache_miss_counter);
    gcmWRITE_REGISTER(0x00474, 0x000A0000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_prefetch_cache_miss_counter);
	/*
    gcmWRITE_REGISTER(0x00474, 0x000B0000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_11);
    gcmWRITE_REGISTER(0x00474, 0x000C0000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_12);
    gcmWRITE_REGISTER(0x00474, 0x000D0000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_13);
    gcmWRITE_REGISTER(0x00474, 0x000E0000);
    gcmREAD_REGISTER(0x00448, &hwProfile->ra_counter_14);*/

    /* Select TX.*/
    gcmWRITE_REGISTER(0x00474, 0x00000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_total_bilinear_requests);
    gcmWRITE_REGISTER(0x00474, 0x01000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_total_trilinear_requests);
    gcmWRITE_REGISTER(0x00474, 0x02000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_total_discarded_texture_requests);
    gcmWRITE_REGISTER(0x00474, 0x03000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_total_texture_requests);
    /*gcmWRITE_REGISTER(0x00474, 0x04000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_counter_4);*/
    gcmWRITE_REGISTER(0x00474, 0x05000000);

    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_mem_read_count);
    gcmWRITE_REGISTER(0x00474, 0x06000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_mem_read_in_8B_count);
    gcmWRITE_REGISTER(0x00474, 0x07000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_cache_miss_count);
    gcmWRITE_REGISTER(0x00474, 0x08000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_cache_hit_texel_count);
    gcmWRITE_REGISTER(0x00474, 0x09000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_cache_miss_texel_count);
	/*
    gcmWRITE_REGISTER(0x00474, 0x0A000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_counter_10);
    gcmWRITE_REGISTER(0x00474, 0x0B000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_counter_11);
    gcmWRITE_REGISTER(0x00474, 0x0C000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_counter_12);
    gcmWRITE_REGISTER(0x00474, 0x0D000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_counter_13);
    gcmWRITE_REGISTER(0x00474, 0x0E000000);
    gcmREAD_REGISTER(0x0044C, &hwProfile->tx_counter_14);*/

    /* Select MC.*/
    /*gcmWRITE_REGISTER(0x00478, 0x00000000);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_0);*/
    gcmWRITE_REGISTER(0x00478, 0x00000001);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_total_read_req_8B_from_pipeline);
    gcmWRITE_REGISTER(0x00478, 0x00000002);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_total_read_req_8B_from_IP);
    gcmWRITE_REGISTER(0x00478, 0x00000003);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_total_write_req_8B_from_pipeline);
    /*gcmWRITE_REGISTER(0x00478, 0x00000004);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_4);
    gcmWRITE_REGISTER(0x00478, 0x00000005);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_5);
    gcmWRITE_REGISTER(0x00478, 0x00000006);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_6);
    gcmWRITE_REGISTER(0x00478, 0x00000007);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_7);
    gcmWRITE_REGISTER(0x00478, 0x00000008);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_8);
    gcmWRITE_REGISTER(0x00478, 0x00000009);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_9);
    gcmWRITE_REGISTER(0x00478, 0x0000000A);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_10);
    gcmWRITE_REGISTER(0x00478, 0x0000000B);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_11);
    gcmWRITE_REGISTER(0x00478, 0x0000000C);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_12);
    gcmWRITE_REGISTER(0x00478, 0x0000000D);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_13);
    gcmWRITE_REGISTER(0x00478, 0x0000000E);
    gcmREAD_REGISTER(0x00468, &hwProfile->mc_counter_14);*/

    /* Select HI.*/
    gcmWRITE_REGISTER(0x00478, 0x00000000);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_axi_cycles_read_request_stalled);
    gcmWRITE_REGISTER(0x00478, 0x00000100);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_axi_cycles_write_request_stalled);
    gcmWRITE_REGISTER(0x00478, 0x00000200);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_axi_cycles_write_data_stalled);
    /*gcmWRITE_REGISTER(0x00478, 0x00000400);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_4);
    gcmWRITE_REGISTER(0x00478, 0x00000500);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_5);
    gcmWRITE_REGISTER(0x00478, 0x00000600);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_6);
    gcmWRITE_REGISTER(0x00478, 0x00000700);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_7);
    gcmWRITE_REGISTER(0x00478, 0x00000800);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_8);
    gcmWRITE_REGISTER(0x00478, 0x00000900);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_9);
    gcmWRITE_REGISTER(0x00478, 0x00000A00);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_10);
    gcmWRITE_REGISTER(0x00478, 0x00000B00);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_11);
    gcmWRITE_REGISTER(0x00478, 0x00000C00);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_12);
    gcmWRITE_REGISTER(0x00478, 0x00000D00);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_13);
    gcmWRITE_REGISTER(0x00478, 0x00000E00);
    gcmREAD_REGISTER(0x0046C, &hwProfile->hi_counter_14);*/
	/* reset counter */
	gcmWRITE_REGISTER(0x00478,0xFFFFFFFF);
	gcmWRITE_REGISTER(0x00474,0xFFFFFFFF);
	gcmWRITE_REGISTER(0x00470,0xFFFFFFFF);
	gcmWRITE_REGISTER(0x00478,0x0);
	gcmWRITE_REGISTER(0x00474,0x0);
	gcmWRITE_REGISTER(0x00470,0x0);
	gcmWRITE_REGISTER(0x00438,0);

    /* Success. */
	return gcvSTATUS_OK;
}

#endif

