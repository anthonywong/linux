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




/*
 * Include file for the HAL bases.
 */

#ifndef __gchal_base_h_
#define __gchal_base_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "aqHalEnum.h"
#include "aqTypes.h"

/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

typedef struct _gcoHAL *				gcoHAL;
typedef struct _gcoOS *					gcoOS;
typedef struct _gco2D *					gco2D;
typedef struct _gcoVG *					gcoVG;
typedef struct _gco3D *					gco3D;
typedef struct _gcoSURF *				gcoSURF;
typedef struct _gcsSURF_INFO *			gcsSURF_INFO_PTR;
typedef struct _gcsSURF_NODE *			gcsSURF_NODE_PTR;
typedef struct _gcsSURF_FORMAT_INFO *	gcsSURF_FORMAT_INFO_PTR;
typedef struct _gcsPOINT *				gcsPOINT_PTR;
typedef struct _gcsSIZE *				gcsSIZE_PTR;
typedef struct _gcsRECT *				gcsRECT_PTR;
typedef struct _gcsBOUNDARY *			gcsBOUNDARY_PTR;

/******************************************************************************\
********************************* Enumerations *********************************
\******************************************************************************/

/* Video memory pool type. */
typedef enum _gcePOOL
{
	gcvPOOL_UNKNOWN,
	gcvPOOL_DEFAULT,
	gcvPOOL_LOCAL,
	gcvPOOL_LOCAL_INTERNAL,
	gcvPOOL_LOCAL_EXTERNAL,
	gcvPOOL_UNIFIED,
	gcvPOOL_SYSTEM,
	gcvPOOL_VIRTUAL,
	gcvPOOL_USER
}
gcePOOL;

/* Blending functions. */
typedef enum _gceBLEND_FUNCTION
{
	gcvBLEND_ZERO,
	gcvBLEND_ONE,
	gcvBLEND_SOURCE_COLOR,
	gcvBLEND_INV_SOURCE_COLOR,
	gcvBLEND_SOURCE_ALPHA,
	gcvBLEND_INV_SOURCE_ALPHA,
	gcvBLEND_TARGET_COLOR,
	gcvBLEND_INV_TARGET_COLOR,
	gcvBLEND_TARGET_ALPHA,
	gcvBLEND_INV_TARGET_ALPHA,
	gcvBLEND_SOURCE_ALPHA_SATURATE,
	gcvBLEND_CONST_COLOR,
	gcvBLEND_INV_CONST_COLOR,
	gcvBLEND_CONST_ALPHA,
	gcvBLEND_INV_CONST_ALPHA,
}
gceBLEND_FUNCTION;

/* Blending modes. */
typedef enum _gceBLEND_MODE
{
	gcvBLEND_ADD,
	gcvBLEND_SUBTRACT,
	gcvBLEND_REVERSE_SUBTRACT,
	gcvBLEND_MIN,
	gcvBLEND_MAX,
}
gceBLEND_MODE;

/* API flags. */
typedef enum _gceAPI
{
	gcvAPI_D3D					= 0x1,
	gcvAPI_OPENGL				= 0x2,
}
gceAPI;

/* Depth modes. */
typedef enum _gceDEPTH_MODE
{
	gcvDEPTH_NONE,
	gcvDEPTH_Z,
	gcvDEPTH_W,
}
gceDEPTH_MODE;

typedef enum _gceWHERE
{
	gcvWHERE_COMMAND,
	gcvWHERE_RASTER,
	gcvWHERE_PIXEL,
}
gceWHERE;

typedef enum _gceHOW
{
	gcvHOW_SEMAPHORE			= 0x1,
	gcvHOW_STALL				= 0x2,
	gcvHOW_SEMAPHORE_STALL		= 0x3,
}
gceHOW;

/******************************************************************************\
********************************* gcoHAL Object *********************************
\******************************************************************************/

/* Construct a new gcoHAL object. */
gceSTATUS
gcoHAL_Construct(
	IN gctPOINTER Context,
	IN gcoOS Os,
	OUT gcoHAL * Hal
	);

/* Destroy an gcoHAL object. */
gceSTATUS
gcoHAL_Destroy(
	IN gcoHAL Hal
	);

/* Get pointer to gco2D object. */
gceSTATUS
gcoHAL_Get2DEngine(
	IN gcoHAL Hal,
	OUT gco2D * Engine
	);

/* Get pointer to gcoVG object. */
gceSTATUS
gcoHAL_GetVGEngine(
	IN gcoHAL Hal,
	OUT gcoVG * Engine
	);

/* Get pointer to gco3D object. */
gceSTATUS
gcoHAL_Get3DEngine(
	IN gcoHAL Hal,
	OUT gco3D * Engine
	);

/* Verify whether the specified feature is available in hardware. */
gctBOOL
gcoHAL_IsFeatureAvailable(
    IN gcoHAL Hal,
	IN gceFEATURE Feature
    );

/* Query the identity of the hardware. */
gceSTATUS
gcoHAL_QueryChipIdentity(
	IN gcoHAL Hal,
	OUT gceCHIPMODEL* ChipModel,
	OUT gctUINT32* ChipRevision,
	OUT gctUINT32* ChipFeatures,
	OUT gctUINT32* ChipMinorFeatures
	);

/* Query the amount of video memory. */
gceSTATUS
gcoHAL_QueryVideoMemory(
	IN gcoHAL Hal,
	OUT gctPHYS_ADDR * InternalAddress,
	OUT gctSIZE_T * InternalSize,
	OUT gctPHYS_ADDR * ExternalAddress,
	OUT gctSIZE_T * ExternalSize,
	OUT gctPHYS_ADDR * ContiguousAddress,
	OUT gctSIZE_T * ContiguousSize
	);

/* Map video memory. */
gceSTATUS
gcoHAL_MapMemory(
	IN gcoHAL Hal,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T NumberOfBytes,
	OUT gctPOINTER * Logical
	);

/* Unmap video memory. */
gceSTATUS
gcoHAL_UnmapMemory(
	IN gcoHAL Hal,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T NumberOfBytes,
	IN gctPOINTER Logical
	);

/* Schedule an unmap of a buffer mapped through its physical address. */
gceSTATUS
gcoHAL_ScheduleUnmapMemory(
	IN gcoHAL Hal,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T NumberOfBytes,
	IN gctPOINTER Logical
	);

/* Schedule an unmap of a user buffer using event mechanism. */
gceSTATUS
gcoHAL_ScheduleUnmapUserMemory(
	IN gcoHAL Hal,
	IN gctPOINTER Info,
	IN gctSIZE_T Size,
	IN gctUINT32 Address,
	IN gctPOINTER Memory
	);

/* Commit the current command buffer. */
gceSTATUS
gcoHAL_Commit(
	IN gcoHAL Hal,
	IN gctBOOL Stall
	);

/* Query the tile capabilities. */
gceSTATUS
gcoHAL_QueryTiled(
	IN gcoHAL Hal,
	OUT gctINT32 * TileWidth2D,
	OUT gctINT32 * TileHeight2D,
	OUT gctINT32 * TileWidth3D,
	OUT gctINT32 * TileHeight3D
	);

gceSTATUS
gcoHAL_Compact(
	IN gcoHAL Hal
	);

void
gcoHAL_ProfileStart(
	IN gcoHAL Hal
	);

void
gcoHAL_ProfileEnd(
	IN gcoHAL Hal,
	IN gctCONST_STRING Title
	);

/* Power Management */
gceSTATUS
gcoHAL_SetPowerManagementState(
	IN gcoHAL Hal,
    IN gceCHIPPOWERSTATE State
    );

gceSTATUS
gcoHAL_QueryPowerManagementState(
	IN gcoHAL Hal,
    OUT gceCHIPPOWERSTATE *State
    );

/* Set the filter type for filter blit. */
gceSTATUS
gcoHAL_SetFilterType(
	IN gcoHAL Hal,
	IN gceFILTER_TYPE FilterType
	);

/******************************************************************************\
********************************** gcoOS Object *********************************
\******************************************************************************/

/* Construct a new gcoOS object. */
gceSTATUS
gcoOS_Construct(
	IN gctPOINTER Context,
	OUT gcoOS * Os
	);

/* Destroy an gcoOS object. */
gceSTATUS
gcoOS_Destroy(
	IN gcoOS Os
	);

/* Get the base address for the physical memory. */
gceSTATUS
gcoOS_GetBaseAddress(
	IN gcoOS Os,
	OUT gctUINT32_PTR BaseAddress
	);

/* Allocate memory. */
gceSTATUS
gcoOS_AllocateMemory(
	IN gcoOS Os,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Memory
	);

/* Free memory. */
gceSTATUS
gcoOS_FreeMemory(
	IN gcoOS Os,
	IN gctPOINTER Memory
	);

/* Device I/O Control call to the kernel HAL layer. */
gceSTATUS
gcoOS_DeviceControl(
	IN gcoOS Os,
	IN gctUINT32 IoControlCode,
	IN gctPOINTER InputBuffer,
	IN gctSIZE_T InputBufferSize,
	IN gctPOINTER OutputBuffer,
	IN gctSIZE_T OutputBufferSize
	);

typedef enum _gceFILE_MODE
{
	gcvFILE_CREATE			= 0,
	gcvFILE_APPEND,
	gcvFILE_READ,
	gcvFILE_CREATETEXT,
	gcvFILE_APPENDTEXT,
	gcvFILE_READTEXT,
}
gceFILE_MODE;

/* Open a file. */
gceSTATUS
gcoOS_Open(
	IN gcoOS Os,
	IN gctCONST_STRING FileName,
	IN gceFILE_MODE Mode,
	OUT gctFILE * File
	);

/* Close a file. */
gceSTATUS
gcoOS_Close(
	IN gcoOS Os,
	IN gctFILE File
	);

/* Read data from a file. */
gceSTATUS
gcoOS_Read(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctSIZE_T ByteCount,
	IN gctPOINTER Data,
	OUT gctSIZE_T * ByteRead
	);

/* Write data to a file. */
gceSTATUS
gcoOS_Write(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctSIZE_T ByteCount,
	IN gctCONST_POINTER Data
	);

typedef enum _gceFILE_WHENCE
{
	gcvFILE_SEEK_SET,
	gcvFILE_SEEK_CUR,
	gcvFILE_SEEK_END
}
gceFILE_WHENCE;

/* Set the current position of a file. */
gceSTATUS
gcoOS_Seek(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctUINT32 Offset,
	IN gceFILE_WHENCE Whence
	);

/* Set the current position of a file. */
gceSTATUS
gcoOS_SetPos(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctUINT32 Position
	);

/* Get the current position of a file. */
gceSTATUS
gcoOS_GetPos(
	IN gcoOS Os,
	IN gctFILE File,
	OUT gctUINT32 * Position
	);

/* Perform a memory copy. */
gceSTATUS
gcoOS_MemCopy(
	IN gctPOINTER Destination,
	IN gctCONST_POINTER Source,
	IN gctSIZE_T Bytes
	);

/* Perform a memory fill. */
gceSTATUS
gcoOS_MemFill(
	IN gctPOINTER Destination,
	IN gctUINT8 Filler,
	IN gctSIZE_T Bytes
	);

/* Zero memory. */
gceSTATUS
gcoOS_ZeroMemory(
	IN gctPOINTER Memory,
	IN gctSIZE_T Bytes
	);

gceSTATUS
gcoOS_StrLen(
	IN gctCONST_STRING String,
	OUT gctSIZE_T * Length
	);

gceSTATUS
gcoOS_StrDup(
	IN gcoOS Os,
	IN gctCONST_STRING String,
	OUT gctSTRING * Target
	);

/* Copy a string. */
gceSTATUS
gcoOS_StrCopy(
	IN gctSTRING Destination,
	IN gctCONST_STRING Source
	);

/* Append a string. */
gceSTATUS
gcoOS_StrCat(
	IN gctSTRING Destination,
	IN gctCONST_STRING Source
	);

/* Compare two strings. */
gceSTATUS
gcoOS_StrCmp(
	IN gctCONST_STRING String1,
	IN gctCONST_STRING String2
	);

/* Compare characters of two strings. */
gceSTATUS
gcoOS_StrNCmp(
	IN gctCONST_STRING String1,
	IN gctCONST_STRING String2,
	IN gctSIZE_T Count
	);

/* Find the last occurance of a character inside a string. */
gceSTATUS
gcoOS_StrFindReverse(
	IN gctCONST_STRING String,
	IN gctINT8 Character,
    OUT gctSTRING * Output
	);

/* Convert string to float. */
gceSTATUS
gcoOS_StrToFloat(
	IN gctCONST_STRING String,
	OUT gctFLOAT * Float
	);

/* Convert string to integer. */
gceSTATUS
gcoOS_StrToInt(
	IN gctCONST_STRING String,
	OUT gctINT * Int
	);

gceSTATUS
gcoOS_MemCmp(
	IN gctCONST_POINTER Memory1,
	IN gctCONST_POINTER Memory2,
	IN gctSIZE_T Bytes
	);

gceSTATUS
gcoOS_PrintStr(
	OUT gctSTRING String,
	IN OUT gctUINT * Offset,
	IN gctCONST_STRING Format,
	...
	);

gceSTATUS
gcoOS_PrintStrV(
	OUT gctSTRING String,
	IN OUT gctUINT * Offset,
	IN gctCONST_STRING Format,
	IN gctPOINTER Arguments
	);

gceSTATUS
gcoOS_PrintNStrV(
	OUT gctSTRING String,
	IN OUT gctUINT * Offset,
	IN gctSIZE_T Count,
	IN gctCONST_STRING Format,
	IN gctPOINTER Arguments
	);

gceSTATUS
gcoOS_LoadLibrary(
	IN gcoOS Os,
	IN gctCONST_STRING Library,
	OUT gctHANDLE * Handle
	);

gceSTATUS
gcoOS_FreeLibrary(
	IN gcoOS Os,
	IN gctHANDLE Handle
	);

gceSTATUS
gcoOS_GetProcAddress(
	IN gcoOS Os,
	IN gctHANDLE Handle,
	IN gctCONST_STRING Name,
	OUT gctPOINTER * Function
	);

gceSTATUS
gcoOS_Compact(
	IN gcoOS Os
	);

void
gcoOS_ProfileStart(
	IN gcoOS Os
	);

void
gcoOS_ProfileEnd(
	IN gcoOS Os,
	IN gctCONST_STRING Title
	);

#if MRVL_BENCH

/*
** Check point for timer.
** To add new profiling point, modify below two members:
*/
typedef enum _apiBenchIndex  {
	APIBENCH_INDEX_FRAME,
	APIBENCH_INDEX_DRAWELEMENTS,
	APIBENCH_INDEX_BUILDSTREAM,
	APIBENCH_INDEX_INDEXBUFFER,
	APIBENCH_INDEX_UPDATESTATE,
	APIBENCH_INDEX_UPDATESTATE1,
	APIBENCH_INDEX_SWAPBUFFERS,
	APIBENCH_INDEX_MAX
}apiBenchIndex;

#define APIBENCHNAME { 	\
	"Frame",			\
	"DrawElement",		\
	"_buildStream",		\
	"IndexBuffer",		\
	"UpdateState",		\
	"UpdateState1",		\
	"eglSwapBuffers",	\
}

/*
** Check point for state update
**
*/
typedef enum _apiBenchStateIndex {
	APIBENCH_STATE_INDEX_FRAME,
	APIBENCH_STATE_INDEX_TEXTURE,
	APIBENCH_STATE_INDEX_SHADER,
	APIBENCH_STATE_INDEX_MAX
}apiBenchStateIndex;

#define APIBENCHSTATENAME {	\
	"Frame",				\
	"Texture",				\
	"Shader",				\
}

// Timer structure to store the profile result
typedef struct _gcoTIMER
{
	gctUINT32				start;
	gctUINT32				end;
	gctUINT32				totalTime;
	gctUINT32				count;	
}gcoTIMER;

// global structure for api bench
typedef struct _gcoAPIBENCH
{
	/* frame count */
	gctUINT32			frameCount;

	/* timer object for bench mark */
	gcoTIMER			timer[APIBENCH_INDEX_MAX];

	/* total sent command size */
	gctUINT32			commandSize;

	/* commit times */
	gctUINT32			commitNumber;

	/* profile the state update, use same structure of timer */
	gcoTIMER			stateCounter[APIBENCH_STATE_INDEX_MAX];

}gcoAPIBENCH;

// Frame count to print
#define PROFILE_DRIVER_FRAME_PRINT		50


void gcoOS_APIBenchStart(
	IN gcoOS Os,
	IN gctUINT32 timerIndex
	);

void gcoOS_APIBenchEnd(
	IN gcoOS Os,
	IN gctUINT32 timerIndex
	);

void gcoOS_APIBenchPrint(
	IN gcoOS Os
	);

void gcoOS_APIBenchInit(
	IN gcoOS Os
	);

void gcoOS_APIBenchFrame(
	IN gcoOS Os
	);

void gcoOS_APIBenchCommand(
	IN gcoOS Os,
	IN gctUINT32 size
	);

void gcoOS_APIBenchCommit(
	IN gcoOS Os
	);

void gcoOS_APIBenchStateEnd(
	IN gcoOS Os,
	IN gctUINT32 stateIndex
	);

void gcoOS_APIBenchStateStart(
	IN gcoOS Os,
	IN gctUINT32 stateIndex
	);

#else


#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------- Atoms ----------------------------------*/

typedef struct gcsATOM * gcsATOM_PTR;

/* Construct an atom. */
gceSTATUS
gcoOS_AtomConstruct(
	IN gcoOS Os,
	OUT gcsATOM_PTR * Atom
	);

/* Destroy an atom. */
gceSTATUS
gcoOS_AtomDestroy(
	IN gcoOS Os,
	IN gcsATOM_PTR Atom
	);

/* Increment an atom. */
gceSTATUS
gcoOS_AtomIncrement(
	IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue
	);

/* Decrement an atom. */
gceSTATUS
gcoOS_AtomDecrement(
	IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue
	);

gctHANDLE 
gcoOS_GetCurrentProcessID(
	void
	);

/*----------------------------------------------------------------------------*/
/*----------------------------------- Time -----------------------------------*/

/* Get the number of milliseconds since the system started. */
gctUINT32
gcoOS_GetTicks(
	void
	);

/******************************************************************************\
**************************** Coordinate Structures *****************************
\******************************************************************************/

typedef struct _gcsPOINT
{
	gctINT32					x;
	gctINT32					y;
}
gcsPOINT;

typedef struct _gcsSIZE
{
	gctINT32					width;
	gctINT32					height;
}
gcsSIZE;

typedef struct _gcsRECT
{
	gctINT32					left;
	gctINT32					top;
	gctINT32					right;
	gctINT32					bottom;
}
gcsRECT;


/******************************************************************************\
********************************* gcoSURF Object ********************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoSURF Common ------------------------------*/

/* Color format classes. */
typedef enum _gceFORMAT_CLASS
{
	gcvFORMAT_CLASS_RGBA		= 4500,
	gcvFORMAT_CLASS_YUV,
	gcvFORMAT_CLASS_INDEX,
	gcvFORMAT_CLASS_LUMINANCE,
	gcvFORMAT_CLASS_BUMP,
	gcvFORMAT_CLASS_DEPTH,
}
gceFORMAT_CLASS;

/* Special enums for width field in gcsFORMAT_COMPONENT. */
typedef enum _gceCOMPONENT_CONTROL
{
	gcvCOMPONENT_NOTPRESENT		= 0x00,
	gcvCOMPONENT_DONTCARE		= 0x80,
	gcvCOMPONENT_WIDTHMASK		= 0x7F,
	gcvCOMPONENT_ODD			= 0x80
}
gceCOMPONENT_CONTROL;

/* Construct a new gcoSURF object. */
gceSTATUS
gcoSURF_Construct(
	IN gcoHAL Hal,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctUINT Depth,
	IN gceSURF_TYPE Type,
	IN gceSURF_FORMAT Format,
	IN gcePOOL Pool,
	OUT gcoSURF * Surface
	);

/* Destroy an gcoSURF object. */
gceSTATUS
gcoSURF_Destroy(
	IN gcoSURF Surface
	);

/* Map user-allocated surface. */
gceSTATUS
gcoSURF_MapUserSurface(
	IN gcoSURF Surface,
	IN gctUINT Alignment,
	IN gctPOINTER Logical,
	IN gctUINT32 Physical
	);

/* Set the color type of the surface. */
gceSTATUS
gcoSURF_SetColorType(
	IN gcoSURF Surface,
	IN gceSURF_COLOR_TYPE ColorType
	);

/* Get the color type of the surface. */
gceSTATUS
gcoSURF_GetColorType(
	IN gcoSURF Surface,
	OUT gceSURF_COLOR_TYPE *ColorType
	);

/* Set the surface ration angle. */
gceSTATUS
gcoSURF_SetRotation(
	IN gcoSURF Surface,
	IN gceSURF_ROTATION Rotation
	);

/* Verify and return the state of the tile status mechanism. */
gceSTATUS
gcoSURF_IsTileStatusSupported(
	IN gcoSURF Surface
	);

/* Enable tile status for the specified surface. */
gceSTATUS
gcoSURF_EnableTileStatus(
	IN gcoSURF Surface
	);

/* Disable tile status for the specified surface. */
gceSTATUS
gcoSURF_DisableTileStatus(
	IN gcoSURF Surface,
	IN gctBOOL Decompress
	);

/* Get surface size. */
gceSTATUS
gcoSURF_GetSize(
	IN gcoSURF Surface,
	OUT gctUINT * Width,
	OUT gctUINT * Height,
	OUT gctUINT * Depth
	);

/* Get surface aligned sizes. */
gceSTATUS
gcoSURF_GetAlignedSize(
	IN gcoSURF Surface,
	OUT gctUINT * Width,
	OUT gctUINT * Height,
	OUT gctINT * Stride
	);

/* Get surface type and format. */
gceSTATUS
gcoSURF_GetFormat(
	IN gcoSURF Surface,
	OUT gceSURF_TYPE * Type,
	OUT gceSURF_FORMAT * Format
	);

/* Lock the surface. */
gceSTATUS
gcoSURF_Lock(
	IN gcoSURF Surface,
	IN OUT gctUINT32 * Address,
	IN OUT gctPOINTER * Memory
	);

/* Unlock the surface. */
gceSTATUS
gcoSURF_Unlock(
	IN gcoSURF Surface,
	IN gctPOINTER Memory
	);

/* Return pixel format parameters. */
gceSTATUS
gcoSURF_QueryFormat(
	IN gceSURF_FORMAT Format,
	OUT gcsSURF_FORMAT_INFO_PTR * Info
	);

/* Compute the color pixel mask. */
gceSTATUS
gcoSURF_ComputeColorMask(
	IN gcsSURF_FORMAT_INFO_PTR Format,
	OUT gctUINT32_PTR ColorMask
	);

/* Flush the surface. */
gceSTATUS
gcoSURF_Flush(
	IN gcoSURF Surface
	);

/* Fill surface with a value. */
gceSTATUS
gcoSURF_Fill(
	IN gcoSURF Surface,
	IN gcsPOINT_PTR Origin,
	IN gcsSIZE_PTR Size,
	IN gctUINT32 Value,
	IN gctUINT32 Mask
	);

/* Alpha blend two surfaces together. */
gceSTATUS
gcoSURF_Blend(
	IN gcoSURF SrcSurface,
	IN gcoSURF DestSurface,
	IN gcsPOINT_PTR SrcOrig,
	IN gcsPOINT_PTR DestOrigin,
	IN gcsSIZE_PTR Size,
	IN gceSURF_BLEND_MODE Mode
	);

/* Quick static surface initialization. */
gceSTATUS
gcoSURF_WrapUserBuffer(
	IN gcoHAL Hal,
	IN gceSURF_TYPE Type,
	IN gceSURF_FORMAT Format,
	IN gctINT Width,
	IN gctINT Height,
	IN gctINT Stride,
	IN gctCONST_POINTER Buffer,
	IN gctINT Index,
	OUT gcoSURF * Surface
	);

/* Create a new gcoSURF wrapper object. */
gceSTATUS
gcoSURF_ConstructWrapper(
	IN gcoHAL Hal,
	OUT gcoSURF * Surface
	);

/* Set the underlying buffer for the surface wrapper. */
gceSTATUS
gcoSURF_SetBuffer(
	IN gcoSURF Surface,
	IN gceSURF_TYPE Type,
	IN gceSURF_FORMAT Format,
	IN gctUINT Stride,
	IN gctPOINTER Logical,
	IN gctUINT32 Physical
	);

/* Set the size of the surface in pixels and map the underlying buffer. */
gceSTATUS
gcoSURF_SetWindow(
	IN gcoSURF Surface,
	IN gctUINT X,
	IN gctUINT Y,
	IN gctUINT Width,
	IN gctUINT Height
	);


/******************************************************************************\
******************************* gcsRECT Structure ******************************
\******************************************************************************/

/* Initialize rectangle structure. */
gceSTATUS
gcsRECT_Set(
	OUT gcsRECT_PTR Rect,
	IN gctINT32 Left,
	IN gctINT32 Top,
	IN gctINT32 Right,
	IN gctINT32 Bottom
	);

/* Return the width of the rectangle. */
gceSTATUS
gcsRECT_Width(
	IN gcsRECT_PTR Rect,
	OUT gctINT32 * Width
	);

/* Return the height of the rectangle. */
gceSTATUS
gcsRECT_Height(
	IN gcsRECT_PTR Rect,
	OUT gctINT32 * Height
	);

/* Ensure that top left corner is to the left and above the right bottom. */
gceSTATUS
gcsRECT_Normalize(
	IN OUT gcsRECT_PTR Rect
	);

/* Compare two rectangles. */
gceSTATUS
gcsRECT_IsEqual(
	IN gcsRECT_PTR Rect1,
	IN gcsRECT_PTR Rect2,
	OUT gctBOOL * Equal
	);

/* Compare the sizes of two rectangles. */
gceSTATUS
gcsRECT_IsOfEqualSize(
	IN gcsRECT_PTR Rect1,
	IN gcsRECT_PTR Rect2,
	OUT gctBOOL * EqualSize
	);


/******************************************************************************\
**************************** gcsBOUNDARY Structure *****************************
\******************************************************************************/

typedef struct _gcsBOUNDARY
{
	gctINT						x;
	gctINT						y;
	gctINT						width;
	gctINT						height;
}
gcsBOUNDARY;


/******************************************************************************\
******************************* Debugging Macros *******************************
\******************************************************************************/

#if (DBG || defined(DEBUG)) && !defined(_DEBUG)
#	define _DEBUG
#endif

void gcoOS_SetDebugLevel(
	IN gctUINT32 Level
	);

void gcoOS_SetDebugZone(
	IN gctUINT32 Zone
	);

void
gcoOS_SetDebugFile(
	IN gctCONST_STRING FileName
	);

/*******************************************************************************
**
**	gcmFATAL
**
**		Print a message to the debugger and execute a break point.
**
**	ARGUMENTS:
**
**		message	Message.
**		...		Optional arguments.
*/
#ifdef _DEBUG
	void
	gcoOS_DebugFatal(
		IN gctSTRING Message,
		...
		);
#	define gcmFATAL				gcoOS_DebugFatal
#elif defined(UNDER_CE)
#	define gcmFATAL				__noop
#else
#	define gcmFATAL(...)
#endif

#define gcvLEVEL_NONE			-1
#define gcvLEVEL_ERROR			0
#define gcvLEVEL_WARNING		1
#define gcvLEVEL_INFO			2
#define gcvLEVEL_VERBOSE		3

void
gcoOS_DebugTrace(
	IN gctUINT32 Level,
	IN gctSTRING Message,
	...
	);

#define gcmPRINT				gcoOS_DebugTrace

/*******************************************************************************
**
**	gcmTRACE
**
**		Print a message to the debugfer if the correct level has been set.  In
**		retail mode this macro does nothing.
**
**	ARGUMENTS:
**
**		level	Level of message.
**		message	Message.
**		...		Optional arguments.
*/
#ifdef _DEBUG
#	define gcmTRACE				gcoOS_DebugTrace
#elif defined(UNDER_CE)
#	define gcmTRACE				__noop
#else
#	define gcmTRACE(...)
#endif

/* Debug zones. */
#define gcvZONE_OS				(1 << 0)
#define gcvZONE_HARDWARE		(1 << 1)
#define gcvZONE_HEAP			(1 << 2)

/* Kernel zones. */
#define gcvZONE_KERNEL			(1 << 3)
#define gcvZONE_VIDMEM			(1 << 4)
#define gcvZONE_COMMAND			(1 << 5)
#define gcvZONE_DRIVER			(1 << 6)
#define gcvZONE_CMODEL			(1 << 7)
#define gcvZONE_MMU				(1 << 8)
#define gcvZONE_EVENT			(1 << 9)

/* User zones. */
#define gcvZONE_HAL				(1 << 3)
#define gcvZONE_BUFFER			(1 << 4)
#define gcvZONE_CONTEXT			(1 << 5)
#define gcvZONE_SURFACE			(1 << 6)
#define gcvZONE_INDEX			(1 << 7)
#define gcvZONE_STREAM			(1 << 8)
#define gcvZONE_TEXTURE			(1 << 9)
#define gcvZONE_2D				(1 << 10)
#define gcvZONE_3D				(1 << 11)
#define gcvZONE_COMPILER		(1 << 12)
#define gcvZONE_MEMORY			(1 << 13)
#define gcvZONE_STATE			(1 << 14)
#define gcvZONE_AUX				(1 << 15)

/* OpenVG-specific zones. */
#define gcvZONE_PAINT			(1 << 16)
#define gcvZONE_FONT			(1 << 17)
#define gcvZONE_IMAGE			(1 << 18)
#define gcvZONE_PATH			(1 << 19)
#define gcvZONE_FILTER			(1 << 20)
#define gcvZONE_MASK			(1 << 21)
#define gcvZONE_UTILITY			(1 << 22)
#define gcvZONE_VG				((1 << (22 - 16 + 1)) - 1) << 16

/* EGL-specific zones. */
#define gcvZONE_EGL				(1 << 23)


/*******************************************************************************
**
**	gcmTRACE_ZONE
**
**		Print a message to the debugger if the correct level and zone has been
**		set.  In retail mode this macro does nothing.
**
**	ARGUMENTS:
**
**		level	Level of message.
**		zone	Zone of message.
**		message	Message.
**		...		Optional arguments.
*/
#ifdef _DEBUG
	void
	gcoOS_DebugTraceZone(
		IN gctUINT32 Level,
		IN gctUINT32 Zone,
		IN gctSTRING Message,
		...
		);
#	define gcmTRACE_ZONE			gcoOS_DebugTraceZone
#elif defined(UNDER_CE)
#	define gcmTRACE_ZONE			__noop
#else
#	define gcmTRACE_ZONE(...)
#endif

/*******************************************************************************
**
**	gcmDUMP
**
**		Print a dump message. 
**
**	ARGUMENTS:
**
**		gctSTRING	Message.
**
**		...			Optional arguments.
*/

#if gcdDUMP
void gcoOS_Print(
	IN gctSTRING Message,
	...
	);
#  define gcmDUMP				gcoOS_Print
#elif defined UNDER_CE
#  define gcmDUMP				__noop
#else
#  define gcmDUMP(...)
#endif

/*******************************************************************************
**
**	gcmDUMP_BUFFER
**
**		Print a buffer to the dump. 
**
**	ARGUMENTS:
**
**		gctSTRING Tag
**			Tag for dump.
**
**		gctUINT32 Physical
**			Physical address of buffer.
**
**		gctPOINTER Logical
**			Logical address of buffer.
**
**		gctUINT32 Offset
**			Offset into buffer.
**
**		gctSIZE_T Bytes
**			Number of bytes.
*/

#if gcdDUMP
void gcfDumpBuffer(
	IN gctSTRING Tag,
	IN gctUINT32 Physical,
	IN gctPOINTER Logical,
	IN gctUINT32 Offset,
	IN gctSIZE_T Bytes
	);
#  define gcmDUMP_BUFFER		gcfDumpBuffer
#elif defined UNDER_CE
#  define gcmDUMP_BUFFER		__noop
#else
#  define gcmDUMP_BUFFER(...)
#endif

/*******************************************************************************
**
**	gcmTRACE_RELEASE
**
**		Print a message to the shader debugger. 
**
**	ARGUMENTS:
**
**		message	Message.
**		...		Optional arguments.
*/

#define gcmTRACE_RELEASE				gcoOS_DebugShaderTrace

void 
gcoOS_DebugShaderTrace(
    IN char* Message,
    ...
    );

void
gcoOS_SetDebugShaderFiles(
    IN gctCONST_STRING VSFileName,
	IN gctCONST_STRING FSFileName
	);

void
gcoOS_SetDebugShaderFileType(
    IN gctUINT32 ShaderType
    );

/*******************************************************************************
**
**	gcmBREAK
**
**		Break into the debugger.  In retail mode this macro does nothing.
**
**	ARGUMENTS:
**
**		None.
*/
#ifdef _DEBUG
	void
	gcoOS_DebugBreak(
		void
		);
#	define gcmBREAK				gcoOS_DebugBreak
#else
#	define gcmBREAK()
#endif

/*******************************************************************************
**
**	gcmASSERT
**
**		Evaluate an expression and break into the debugger if the expression
**		evaluates to false.  In retail mode this macro does nothing.
**
**	ARGUMENTS:
**
**		exp		Expression to evaluate.
*/
#ifdef _DEBUG
#	define gcmASSERT(exp) \
		do \
		{ \
			if (!(exp)) \
			{ \
				gcmTRACE( \
					gcvLEVEL_ERROR, \
					"gcmASSERT: File %s, Line %d\n:= %s", \
						__FILE__, __LINE__, #exp); \
				gcmBREAK(); \
			} \
		} \
		while (gcvFALSE)
#else
#	define gcmASSERT(exp)
#endif

/*******************************************************************************
**
**	gcmVERIFY
**
**		Verify if an expression returns true.  If the expression does not
**		evaluates to true, an assertion will happen in debug mode.
**
**	ARGUMENTS:
**
**		exp		Expression to evaluate.
*/
#ifdef _DEBUG
#	define gcmVERIFY(exp)			gcmASSERT(exp)
#else
#	define gcmVERIFY(exp)			exp
#endif

/*******************************************************************************
**
**	gcmVERIFY_OK
**
**		Verify a fucntion returns gcvSTATUS_OK.  If the function does not return
**		gcvSTATUS_OK, an assertion will happen in debug mode.
**
**	ARGUMENTS:
**
**		func	Function to evaluate.
*/
#ifdef _DEBUG
	void
	gcoOS_Verify(
		IN gceSTATUS _Status
		);
#	define gcmVERIFY_OK(func) gcoOS_Verify(func)
#else
#	define gcmVERIFY_OK(func) func
#endif

/*******************************************************************************
**
**	gcmERR_BREAK
**
**		Executes a break statement on error.
**
**	ASSUMPTIONS:
**
**		'status' variable of gceSTATUS type must be defined.
**
**	ARGUMENTS:
**
**		func	Function to evaluate.
*/
#define gcmERR_BREAK(func) \
	status = func; \
	if (gcmIS_ERROR(status)) \
	{ \
		gcmTRACE( \
			gcvLEVEL_ERROR, \
			"gcmERR_BREAK: status=%d @ line=%d in %s.", \
			status, __LINE__, __FILE__ \
			); \
		break; \
	}

/*******************************************************************************
**
**	gcmERR_RETURN
**
**		Executes a return statement on error.
**
**	ASSUMPTIONS:
**
**		'status' variable of gceSTATUS type must be defined.
**
**	ARGUMENTS:
**
**		func	Function to evaluate.
*/
#define gcmERR_RETURN(func) \
	status = func; \
	if (gcmIS_ERROR(status)) \
	{ \
		gcmBREAK(); \
		return status; \
	}

/*******************************************************************************
**
**	gcmONERROR
**
**		Jump to the error handler in case there is an error.
**
**	ASSUMPTIONS:
**
**		'status' variable of gceSTATUS type must be defined.
**
**	ARGUMENTS:
**
**		func	Function to evaluate.
*/
#define gcmONERROR(func) \
	status = func; \
	if (gcmIS_ERROR(status)) \
	{ \
		gcmTRACE( \
			gcvLEVEL_ERROR, \
			"gcmERR_GOTO: status=%d @ line=%d in %s.", \
			status, __LINE__, __FILE__ \
			); \
		goto OnError; \
	}

/*******************************************************************************
**
**	gcmVERIFY_LOCK
**
**		Verifies whether the surface is locked.
**
**	ARGUMENTS:
**
**		surfaceInfo	Pointer to the surface iniformational structure.
*/
#define gcmVERIFY_LOCK(surfaceInfo) \
	if (!surfaceInfo->node.valid) \
	{ \
		status = gcvSTATUS_MEMORY_UNLOCKED; \
		break; \
	}

/*******************************************************************************
**
**	gcmVERIFY_NODE_LOCK
**
**		Verifies whether the surface node is locked.
**
**	ARGUMENTS:
**
**		surfaceInfo	Pointer to the surface iniformational structure.
*/
#define gcmVERIFY_NODE_LOCK(surfaceNode) \
	if (!surfaceNode->valid) \
	{ \
		status = gcvSTATUS_MEMORY_UNLOCKED; \
		break; \
	}

/*******************************************************************************
**
**	gcmBADOBJECT_BREAK
**
**		Executes a break statement on bad object.
**
**	ARGUMENTS:
**
**		obj		Object to test.
**		t		Expected type of the object.
*/
#define gcmBADOBJECT_BREAK(obj, t) \
	if ( (obj == gcvNULL) || (((gcsOBJECT*)(obj))->type != t) ) \
	{ \
		status = gcvSTATUS_INVALID_OBJECT; \
		break; \
	} \

/*******************************************************************************
**
**	gcmCHECK_STATUS
**
**		Executes a break statement on error.
**
**	ASSUMPTIONS:
**
**		'status' variable of gceSTATUS type must be defined.
**
**	ARGUMENTS:
**
**		func	Function to evaluate.
*/
#define gcmCHECK_STATUS(func) \
	last = func; \
	if (gcmIS_ERROR(last)) \
	{ \
		gcmTRACE( \
			gcvLEVEL_ERROR, \
			"gcmCHECK_STATUS: status=%d @ line=%d in %s.", \
			last, __LINE__, __FILE__ \
			); \
		status = last; \
	}

/*******************************************************************************
**
**	gcmVERIFY_ARGUMENT
**
**		Assert if an argument does not apply to teh speficied expression.  If
**		the argument evaluates to false, AQSTATUS_INVALD_ARGUMENT will be
**		returned from the current function.  In retail mode this macro does
**		nothing.
**
**	ARGUMENTS:
**
**		arg		Argument to evaluate.
*/
#ifdef _DEBUG
#	define gcmVERIFY_ARGUMENT(arg) \
		do \
		{ \
			if (!(arg)) \
			{ \
				gcmASSERT(arg); \
				return gcvSTATUS_INVALID_ARGUMENT; \
			} \
		} \
		while (gcvFALSE)
#else
#	define gcmVERIFY_ARGUMENT(arg)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __gchal_base_h_ */
