/****************************************************************************
*
*    Copyright (c) 2002 - 2008 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************
*
*
*****************************************************************************/






#ifndef __aqhaluserint_h_
#define __aqhaluserint_h_

#ifdef LINUX
#define PENDING_FREED_MEMORY_SIZE_LIMIT		(4 * 1024 * 1024)
#endif

/******************************************************************************\
********************************* gcoHAL object *********************************
\******************************************************************************/

struct _gcoHAL
{
	/* The object. */
	gcsOBJECT				object;

	/* Context passed in during creation. */
	gctPOINTER				context;

	/* Pointer to an gcoOS object. */
	gcoOS					os;

	/* Pointer to an gcoHARDWARE object. */
	gcoHARDWARE				hardware;

	/* Pointer to the gco2D and gco3D objects. */
	gco2D					engine2D;
	gcoVG					engineVG;
	gco3D					engine3D;

#ifdef PENDING_FREED_MEMORY_SIZE_LIMIT
	/* Pending freed memory size */
	gctSIZE_T				pendingFreedMemorySize;
#endif

	/* Pointer to te gcoDUMP object. */
	gcoDUMP					dump;

#if VIVANTE_PROFILER
    gcsPROFILER             profiler;
#endif

    /* Process handle */
    gctHANDLE               process;

    /* Pointer to version string */
    gctCONST_STRING         version;

#if MRVL_BENCH
	/* timer object for bench mark */
	gcoAPIBENCH	apiBench;
#endif
};


/******************************************************************************\
********************************* gcoSURF object ********************************
\******************************************************************************/

typedef struct _gcsSURF_NODE
{
	/* Surface memory pool. */
	gcePOOL					pool;

	/* Lock count for the surface. */
	gctINT					lockCount;

	/* If not zero, the node is locked in the kernel. */
	gctBOOL					lockedInKernel;

	/* Number of planes in the surface for planar format support. */
	gctUINT					count;

	/* Node valid flag for the surface pointers. */
	gctBOOL					valid;

	/* The physical addresses of the surface. */
	gctUINT32				physical;
	gctUINT32				physical2;
	gctUINT32				physical3;

	/* The logical addresses of the surface. */
	gctUINT8_PTR			logical;
	gctUINT8_PTR			logical2;
	gctUINT8_PTR			logical3;

	/* Linear size and filler for tile status. */
	gctSIZE_T				size;
	gctUINT32				filler;
	gctBOOL					firstLock;

	union _gcuSURF_NODE_LIST
	{
		/* Allocated through HAL. */
		struct _gcsMEM_NODE_NORMAL
		{
			gcuVIDMEM_NODE_PTR	node;
		}
		normal;

		/* Wrapped around user-allocated surface (gcvPOOL_USER). */
		struct _gcsMEM_NODE_WRAPPED
		{
			gctBOOL				logicalMapped;
			gctPOINTER			mappingInfo;
		}
		wrapped;
	}
	u;
}
gcsSURF_NODE;

typedef struct _gcsSURF_INFO
{
	/* Type usage and format of surface. */
	gceSURF_TYPE			type;
	gceSURF_FORMAT			format;

	/* Surface size. */
	gcsRECT					rect;
	gctUINT					alignedWidth;
	gctUINT					alignedHeight;
	gctBOOL					is16Bit;

	/* Rotation flag. */
	gceSURF_ROTATION		rotation;
	gceORIENTATION			orientation;

	/* Surface stride and size. */
	gctUINT					stride;
	gctUINT					size;

	/* YUV planar surface parameters. */
	gctUINT					uOffset;
	gctUINT					vOffset;
	gctUINT					uStride;
	gctUINT					vStride;

	/* Video memory node for surface. */
	gcsSURF_NODE			node;

	/* Samples. */
	gcsSAMPLES				samples;
	gctBOOL					vaa;

	/* Tile status. */
	gctBOOL					tileStatusDisabled;
	gctBOOL					superTiled;
	gctUINT32				clearValue;

	/* Hierarchical Z buffer pointer. */
	gcsSURF_NODE			hzNode;
}
gcsSURF_INFO;

struct _gcoSURF
{
	/* Object. */
	gcsOBJECT				object;

	/* Pointer to an gcoHAL object. */
	gcoHAL					hal;

	/* Surface information structure. */
	struct _gcsSURF_INFO	info;

	/* Depth of the surface in planes. */
	gctUINT					depth;

	gctBOOL					resolvable;

    /* Video memory node for tile status. */
    gcsSURF_NODE			tileStatusNode;
	gcsSURF_NODE			hzTileStatusNode;

	/* Surface color type. */
	gceSURF_COLOR_TYPE      colorType;

	/* Automatic stride calculation. */
	gctBOOL					autoStride;

	/* User pointers for the surface wrapper. */
	gctPOINTER				logical;
	gctUINT32				physical;

	/* Reference count of surface. */
	gctINT32				referenceCount;
};

/******************************************************************************\
******************************** gcoQUEUE Object *******************************
\******************************************************************************/

/* Construct a new gcoQUEUE object. */
gceSTATUS
gcoQUEUE_Construct(
	IN gcoOS Os,
	OUT gcoQUEUE * Queue
	);

/* Destroy a gcoQUEUE object. */
gceSTATUS
gcoQUEUE_Destroy(
	IN gcoQUEUE Queue
	);

/* Append an event to a gcoQUEUE object. */
gceSTATUS
gcoQUEUE_AppendEvent(
	IN gcoQUEUE Queue,
	IN gcsHAL_INTERFACE * Interface
	);

/* Commit and event queue. */
gceSTATUS
gcoQUEUE_Commit(
	IN gcoQUEUE Queue
	);

#endif /* __aqhaluserint_h_ */
