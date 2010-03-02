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






#ifndef __context_h_
#define __context_h_


/* gcoCONTEXT structure that hold the current context. */
struct _gcoCONTEXT
{
	/* Object. */
	gcsOBJECT					object;

	/* Pointer to gcoOS object. */
	gcoOS						os;

	/* Pointer to gcoHARDWARE object. */
	gcoHARDWARE					hardware;

	/* Context ID. */
	gctUINT64					id;

	/* State mapping. */
	gctUINT32_PTR				map;

	/* Context buffer. */
	gctUINT32_PTR				buffer;
	gctUINT32					pipe3DIndex;
	gctUINT32					pipe2DIndex;
	gctUINT32					linkIndex;
	gctUINT32					inUseIndex;
	gctSIZE_T					bufferSize;

	/* Context buffer used for commitment. */
	gctSIZE_T					bytes;
	gctPHYS_ADDR				physical;
	gctPOINTER					logical;

	/* Pointer to final LINK command. */
	gctPOINTER					link;

	/* Requested pipe select for context. */
	gctUINT32					initialPipe;
	gctUINT32					entryPipe;
	gctUINT32					currentPipe;

	/* Flag to specify whether PostCommit needs to be called. */
	gctBOOL						postCommit;

	/* Busy flag. */
	volatile gctBOOL *			inUse;

	/* Variables used for building state buffer. */
	gctUINT32					lastAddress;
	gctSIZE_T					lastSize;
	gctUINT32					lastIndex;
	gctBOOL						lastFixed;
};

struct _gcoCMDBUF
{
	/* The object. */
	gcsOBJECT					object;

	/* Pointer to gcoOS object. */
	gcoOS						os;

	/* Pointer to gcoHARDWARE object. */
	gcoHARDWARE					hardware;

	/* Physical address of command buffer. */
	gctPHYS_ADDR				physical;

	/* Logical address of command buffer. */
	gctPOINTER					logical;

	/* Number of bytes in command buffer. */
	gctSIZE_T					bytes;

	/* Current offset into the command buffer. */
	gctUINT32					offset;

	/* Number of free bytes in command buffer. */
	gctSIZE_T					free;

	/* Pointer to next command buffer or gcvNULL if this is the last one. */
	gcoCMDBUF					sibling;
};

typedef struct _gcsQUEUE * gcsQUEUE_PTR;

typedef struct _gcsQUEUE
{
	/* Pointer to next gcsQUEUE structure. */
	gcsQUEUE_PTR				next;

	/* Event information. */
	gcsHAL_INTERFACE			interface;
}
gcsQUEUE;

/* Event queue. */
struct _gcoQUEUE
{
	/* The object. */
	gcsOBJECT					object;

	/* Pointer to gcoOS object. */
	gcoOS						os;

	/* Pointer to current event queue. */
	gcsQUEUE_PTR				head;
	gcsQUEUE_PTR				tail;
};

#endif /* __context_h_ */
