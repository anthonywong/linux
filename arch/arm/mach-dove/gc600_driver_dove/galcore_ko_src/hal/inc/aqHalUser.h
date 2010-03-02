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
 * Include file for the user HAL layer.
 */

#ifndef __aqhaluser_h_
#define __aqhaluser_h_

#include "aqHal.h"
#include "aqHalDriver.h"
#include "aqHalEnum.h"
#include "gcDump.h"

#include "gcHAL_Base.h"
#include "gcHAL_2D.h"


#include "gcHAL_3D.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
******************************* Multicast values *******************************
\******************************************************************************/

/* Value types. */
typedef enum _gceVALUE_TYPE
{
	gcvVALUE_UINT,
	gcvVALUE_FIXED,
	gcvVALUE_FLOAT,
}
gceVALUE_TYPE;

/* Value unions. */
typedef union _gcuVALUE
{
	gctUINT						uintValue;
	gctFIXED_POINT				fixedValue;
	gctFLOAT					floatValue;
}
gcuVALUE;

/******************************************************************************\
******************************* Fixed Point Math *******************************
\******************************************************************************/

#define gcmXMultiply(x1, x2) \
	(gctFIXED_POINT) (((gctINT64) (x1) * (x2)) >> 16)

#define gcmXDivide(x1, x2) \
	(gctFIXED_POINT) ((((gctINT64) (x1)) << 16) / (x2))

#define gcmXMultiplyDivide(x1, x2, x3) \
	(gctFIXED_POINT) ((gctINT64) (x1) * (x2) / (x3))

/******************************************************************************\
***************************** gcsSAMPLES Structure *****************************
\******************************************************************************/

typedef struct _gcsSAMPLES
{
	gctUINT8 x;
	gctUINT8 y;
}
gcsSAMPLES;

/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

typedef struct _gcoBUFFER *				gcoBUFFER;
typedef struct _gcoDUMP *				gcoDUMP;

/******************************************************************************\
********************************** gcoHAL Object ********************************
\******************************************************************************/

gceSTATUS
gcoHAL_GetDump(
	IN gcoHAL Hal,
	OUT gcoDUMP * Dump
	);

/* Call the kernel HAL layer. */
gceSTATUS
gcoHAL_Call(
	IN gcoHAL Hal,
	IN OUT gcsHAL_INTERFACE * Interface
	);

/* Schedule an event. */
gceSTATUS
gcoHAL_ScheduleEvent(
	IN gcoHAL Hal,
	IN OUT gcsHAL_INTERFACE * Interface
	);

/******************************************************************************\
******************************* gcoHARDWARE Object ******************************
\******************************************************************************/



/*----------------------------------------------------------------------------*/
/*----------------------------- gcoHARDWARE Common ----------------------------*/

/* Verify whether the specified feature is available in hardware. */
gctBOOL
gcoHARDWARE_IsFeatureAvailable(
    IN gcoHARDWARE Hardware,
	IN gceFEATURE Feature
    );

/* Select a graphics pipe. */
gceSTATUS
gcoHARDWARE_SelectPipe(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Pipe
	);

/* Flush the current graphics pipe. */
gceSTATUS
gcoHARDWARE_FlushPipe(
	IN gcoHARDWARE Hardware
	);

/* Send semaphore down the current pipe. */
gceSTATUS
gcoHARDWARE_Semaphore(
	IN gcoHARDWARE Hardware,
	IN gceWHERE From,
	IN gceWHERE To,
	IN gceHOW How
	);

/* Load a number of load states. */
gceSTATUS
gcoHARDWARE_LoadState(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctSIZE_T Count,
	IN gctPOINTER States
	);

/* Load a number of load states. */
gceSTATUS
gcoHARDWARE_LoadStateBuffer(
	IN gcoHARDWARE Hardware,
	IN gctCONST_POINTER StateBuffer,
	IN gctSIZE_T Bytes
	);

/* Load a number of load states in fixed-point (3D pipe). */
gceSTATUS
gcoHARDWARE_LoadStateX(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctSIZE_T Count,
	IN gctPOINTER States
	);

/* Load a number of load states in floating-point (3D pipe). */
gceSTATUS
gcoHARDWARE_LoadStateF(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctSIZE_T Count,
	IN gctPOINTER States
	);

/* Load one 32-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState32(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctUINT32 Data
	);

/* Load one 32-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState32x(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctFIXED_POINT Data
	);

/* Load one 64-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState64(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctUINT64 Data
	);

/* Commit the current command buffer. */
gceSTATUS
gcoHARDWARE_Commit(
	IN gcoHARDWARE Hardware
	);

/* Stall the pipe. */
gceSTATUS
gcoHARDWARE_Stall(
	IN gcoHARDWARE Hardware
	);

/* Resolve. */
gceSTATUS
gcoHARDWARE_ResolveRect(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR SrcInfo,
	IN gcsSURF_INFO_PTR DestInfo,
	IN gcsPOINT_PTR SrcOrigin,
	IN gcsPOINT_PTR DestOrigin,
	IN gcsPOINT_PTR RectSize
	);

/* Resolve depth buffer. */
gceSTATUS
gcoHARDWARE_ResolveDepth(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 SrcTileStatusAddress,
	IN gcsSURF_INFO_PTR SrcInfo,
	IN gcsSURF_INFO_PTR DestInfo,
	IN gcsPOINT_PTR SrcOrigin,
	IN gcsPOINT_PTR DestOrigin,
	IN gcsPOINT_PTR RectSize
	);

/* Query the tile size of the given surface. */
gceSTATUS
gcoHARDWARE_GetSurfaceTileSize(
	IN gcsSURF_INFO_PTR Surface,
	OUT gctINT32 * TileWidth,
	OUT gctINT32 * TileHeight
	);

/* Query tile sizes. */
gceSTATUS
gcoHARDWARE_QueryTileSize(
	OUT gctINT32 * TileWidth2D,
	OUT gctINT32 * TileHeight2D,
	OUT gctINT32 * TileWidth3D,
	OUT gctINT32 * TileHeight3D,
	OUT gctUINT32 * StrideAlignment
	);

/* Get tile status sizes for a surface. */
gceSTATUS
gcoHARDWARE_QueryTileStatus(
	IN gcoHARDWARE Hardware,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctSIZE_T Bytes,
	OUT gctSIZE_T_PTR Size,
	OUT gctUINT_PTR Alignment,
	OUT gctUINT32_PTR Filler
	);

/* Enable tile status for a surface. */
gceSTATUS
gcoHARDWARE_EnableTileStatus(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface,
	IN gctUINT32 TileStatusAddress
	);

/* Disable tile status for a surface. */
gceSTATUS
gcoHARDWARE_DisableTileStatus(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface,
	IN gctBOOL CpuAccess
	);

/* Flush tile status cache. */
gceSTATUS
gcoHARDWARE_FlushTileStatus(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Decompress
	);

typedef enum _gceTILE_STATUS_CONTROL
{
	gcvTILE_STATUS_PAUSE,
	gcvTILE_STATUS_RESUME,
}
gceTILE_STATUS_CONTROL;

/* Pause or resume tile status. */
gceSTATUS gcoHARDWARE_PauseTileStatus(
	IN gcoHARDWARE Hardware,
	IN gceTILE_STATUS_CONTROL Control
	);

/* Lock a surface. */
gceSTATUS
gcoHARDWARE_Lock(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_NODE_PTR Node,
	OUT gctUINT32 * Address,
	OUT gctPOINTER * Memory
	);

/* Unlock a surface. */
gceSTATUS
gcoHARDWARE_Unlock(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_NODE_PTR Node,
	IN gceSURF_TYPE Type
	);

/* Call kernel for event. */
gceSTATUS
gcoHARDWARE_CallEvent(
	IN gcoHARDWARE Hardware,
	IN OUT gcsHAL_INTERFACE * Interface
	);

/* Schedule destruction for the specified video memory node. */
gceSTATUS
gcoHARDWARE_ScheduleVideoMemory(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_NODE_PTR Node
	);

/* Allocate a temporary surface with specified parameters. */
gceSTATUS
gcoHARDWARE_AllocateTemporarySurface(
	IN gcoHARDWARE Hardware,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gcsSURF_FORMAT_INFO_PTR Format,
	IN gceSURF_TYPE Type
	);

/* Free the temporary surface. */
gceSTATUS
gcoHARDWARE_FreeTemporarySurface(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Synchronized
	);

/* Convert pixel format. */
gceSTATUS
gcoHARDWARE_ConvertPixel(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER SrcPixel,
	OUT gctPOINTER TrgPixel,
	IN gctUINT SrcBitOffset,
	IN gctUINT TrgBitOffset,
	IN gcsSURF_FORMAT_INFO_PTR SrcFormat,
	IN gcsSURF_FORMAT_INFO_PTR TrgFormat,
	IN gcsBOUNDARY_PTR SrcBoundary,
	IN gcsBOUNDARY_PTR TrgBoundary
	);

/* Copy a rectangular area with format conversion. */
gceSTATUS
gcoHARDWARE_CopyPixels(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Source,
	IN gcsSURF_INFO_PTR Target,
	IN gctINT SourceX,
	IN gctINT SourceY,
	IN gctINT TargetX,
	IN gctINT TargetY,
	IN gctINT Width,
	IN gctINT Height
	);

/* Enable or disable anti-aliasing. */
gceSTATUS
gcoHARDWARE_SetAntiAlias(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

/* Write data into the command buffer. */
gceSTATUS
gcoHARDWARE_WriteBuffer(
	IN gcoHARDWARE Hardware,
	IN gctCONST_POINTER Data,
	IN gctSIZE_T Bytes,
	IN gctBOOL Aligned
	);

/* Convert RGB8 color value to YUV color space. */
void gcoHARDWARE_RGB2YUV(
	gctUINT8 R,
	gctUINT8 G,
	gctUINT8 B,
	gctUINT8_PTR Y,
	gctUINT8_PTR U,
	gctUINT8_PTR V
	);

/* Convert YUV color value to RGB8 color space. */
void gcoHARDWARE_YUV2RGB(
	gctUINT8 Y,
	gctUINT8 U,
	gctUINT8 V,
	gctUINT8_PTR R,
	gctUINT8_PTR G,
	gctUINT8_PTR B
	);

/*----------------------------------------------------------------------------*/
/*----------------------- gcoHARDWARE Fragment Processor ---------------------*/

/* Set the fragment processor configuration. */
gceSTATUS
gcoHARDWARE_SetFragmentConfiguration(
	IN gcoHARDWARE Hardware,
	IN gctBOOL ColorFromStream,
	IN gctBOOL EnableFog,
	IN gctBOOL EnableSmoothPoint,
	IN gctUINT32 ClipPlanes
	);

/* Enable/disable texture stage operation. */
gceSTATUS
gcoHARDWARE_EnableTextureStage(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gctBOOL Enable
	);

/* Program the channel enable masks for the color texture function. */
gceSTATUS
gcoHARDWARE_SetTextureColorMask(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gctBOOL ColorEnabled,
	IN gctBOOL AlphaEnabled
	);

/* Program the channel enable masks for the alpha texture function. */
gceSTATUS
gcoHARDWARE_SetTextureAlphaMask(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gctBOOL ColorEnabled,
	IN gctBOOL AlphaEnabled
	);

/* Program the constant fragment color. */
gceSTATUS
gcoHARDWARE_SetFragmentColorX(
	IN gcoHARDWARE Hardware,
	IN gctFIXED_POINT Red,
	IN gctFIXED_POINT Green,
	IN gctFIXED_POINT Blue,
	IN gctFIXED_POINT Alpha
	);

gceSTATUS
gcoHARDWARE_SetFragmentColorF(
	IN gcoHARDWARE Hardware,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	);

/* Program the constant fog color. */
gceSTATUS
gcoHARDWARE_SetFogColorX(
	IN gcoHARDWARE Hardware,
	IN gctFIXED_POINT Red,
	IN gctFIXED_POINT Green,
	IN gctFIXED_POINT Blue,
	IN gctFIXED_POINT Alpha
	);

gceSTATUS
gcoHARDWARE_SetFogColorF(
	IN gcoHARDWARE Hardware,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	);

/* Program the constant texture color. */
gceSTATUS
gcoHARDWARE_SetTetxureColorX(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gctFIXED_POINT Red,
	IN gctFIXED_POINT Green,
	IN gctFIXED_POINT Blue,
	IN gctFIXED_POINT Alpha
	);

gceSTATUS
gcoHARDWARE_SetTetxureColorF(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	);

/* Configure color texture function. */
gceSTATUS
gcoHARDWARE_SetColorTextureFunction(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gceTEXTURE_FUNCTION Function,
	IN gceTEXTURE_SOURCE Source0,
	IN gceTEXTURE_CHANNEL Channel0,
	IN gceTEXTURE_SOURCE Source1,
	IN gceTEXTURE_CHANNEL Channel1,
	IN gceTEXTURE_SOURCE Source2,
	IN gceTEXTURE_CHANNEL Channel2,
	IN gctINT Scale
	);

/* Configure alpha texture function. */
gceSTATUS
gcoHARDWARE_SetAlphaTextureFunction(
	IN gcoHARDWARE Hardware,
	IN gctINT Stage,
	IN gceTEXTURE_FUNCTION Function,
	IN gceTEXTURE_SOURCE Source0,
	IN gceTEXTURE_CHANNEL Channel0,
	IN gceTEXTURE_SOURCE Source1,
	IN gceTEXTURE_CHANNEL Channel1,
	IN gceTEXTURE_SOURCE Source2,
	IN gceTEXTURE_CHANNEL Channel2,
	IN gctINT Scale
	);

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoHARDWARE 2D ------------------------------*/

/* Translate API source color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateSourceFormat(
	IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT APIValue,
	OUT gctUINT32* HwValue,
	OUT gctUINT32* HwSwizzleValue,
	OUT gctUINT32* HwIsYUVValue
	);

/* Translate API destination color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateDestinationFormat(
    IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT APIValue,
	OUT gctUINT32* HwValue,
	OUT gctUINT32* HwSwizzleValue,
	OUT gctUINT32* HwIsYUVValue
	);

/* Translate API pattern color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePatternFormat(
    IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT APIValue,
	OUT gctUINT32* HwValue,
	OUT gctUINT32* HwSwizzleValue,
	OUT gctUINT32* HwIsYUVValue
	);

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateTransparency(
	IN gceSURF_TRANSPARENCY APIValue,
	OUT gctUINT32* HwValue
	);

/* Translate SURF API transparency mode to PE 2.0 transparency values. */
gceSTATUS
gcoHARDWARE_TranslateSurfTransparency(
	IN gceSURF_TRANSPARENCY APIValue,
	OUT gctUINT32* srcTransparency,
	OUT gctUINT32* dstTransparency,
	OUT gctUINT32* patTransparency
	);

/* Translate API transparency mode to its PE 1.0 hardware value. */
gceSTATUS
gcoHARDWARE_TranslateTransparencies(
	IN gcoHARDWARE	Hardware,
	IN gctUINT32	srcTransparency,
	IN gctUINT32	dstTransparency,
	IN gctUINT32	patTransparency,
	OUT gctUINT32*  HwValue
	);

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateSourceTransparency(
	IN gce2D_TRANSPARENCY APIValue,
	OUT gctUINT32 * HwValue
	);

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateDestinationTransparency(
	IN gce2D_TRANSPARENCY APIValue,
	OUT gctUINT32 * HwValue
	);

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePatternTransparency(
	IN gce2D_TRANSPARENCY APIValue,
	OUT gctUINT32 * HwValue
	);

/* Translate API YUV Color mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateYUVColorMode(
	IN	gce2D_YUV_COLOR_MODE APIValue,
	OUT gctUINT32 * HwValue
	);

/* Translate API pixel color multiply mode to its hardware value. */
gceSTATUS
gcoHARDWARE_PixelColorMultiplyMode(
	IN	gce2D_PIXEL_COLOR_MULTIPLY_MODE APIValue,
	OUT gctUINT32 * HwValue
	);

/* Translate API global color multiply mode to its hardware value. */
gceSTATUS
gcoHARDWARE_GlobalColorMultiplyMode(
	IN	gce2D_GLOBAL_COLOR_MULTIPLY_MODE APIValue,
	OUT gctUINT32 * HwValue
	);

/* Translate API mono packing mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateMonoPack(
	IN gceSURF_MONOPACK APIValue,
	OUT gctUINT32* HwValue
	);

/* Translate API 2D command to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateCommand(
	IN gce2D_COMMAND APIValue,
	OUT gctUINT32* HwValue
	);

/* Translate API per-pixel alpha mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelAlphaMode(
	IN gceSURF_PIXEL_ALPHA_MODE APIValue,
	OUT gctUINT32* HwValue
	);

/* Translate API global alpha mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateGlobalAlphaMode(
	IN gceSURF_GLOBAL_ALPHA_MODE APIValue,
	OUT gctUINT32* HwValue
	);

/* Translate API per-pixel color mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelColorMode(
	IN gceSURF_PIXEL_COLOR_MODE APIValue,
	OUT gctUINT32* HwValue
	);

/* Translate API alpha factor mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateAlphaFactorMode(
	IN	gcoHARDWARE Hardware,
	IN	gceSURF_BLEND_FACTOR_MODE APIValue,
	OUT gctUINT32_PTR HwValue
	);

/* Configure monochrome source. */
gceSTATUS
gcoHARDWARE_SetMonochromeSource(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 MonoTransparency,
	IN gceSURF_MONOPACK DataPack,
	IN gctBOOL CoordRelative,
	IN gctUINT32 FgColor32,
	IN gctUINT32 BgColor32
	);

/* Configure color source. */
gceSTATUS
gcoHARDWARE_SetColorSource(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface,
	IN gctBOOL CoordRelative
	);

/* Configure masked color source. */
gceSTATUS
gcoHARDWARE_SetMaskedSource(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface,
	IN gctBOOL CoordRelative,
	IN gceSURF_MONOPACK MaskPack
	);

/* Setup the source rectangle. */
gceSTATUS
gcoHARDWARE_SetSource(
	IN gcoHARDWARE Hardware,
	IN gcsRECT_PTR SrcRect
	);

/* Setup the fraction of the source origin for filter blit. */
gceSTATUS
gcoHARDWARE_SetOriginFraction(
	IN gcoHARDWARE Hardware,
	IN gctUINT16 HorFraction,
	IN gctUINT16 VerFraction
	);

/* Load 256-entry color table for INDEX8 source surfaces. */
gceSTATUS 
gcoHARDWARE_LoadPalette(
	IN gcoHARDWARE Hardware,
	IN gctUINT FirstIndex,
	IN gctUINT IndexCount,
	IN gctPOINTER ColorTable,
	IN gctBOOL ColorConvert
	);

/* Setup the source pixel swizzle. */
gceSTATUS 
gcoHARDWARE_SetSourceSwizzle(
	IN gcoHARDWARE Hardware,
	IN gceSURF_SWIZZLE Swizzle
	);

/* Setup the source pixel UV swizzle. */
gceSTATUS 
gcoHARDWARE_SetSourceSwizzleUV(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 SwizzleUV
	);

/* Setup the source global color value in ARGB8 format. */
gceSTATUS 
gcoHARDWARE_SetSourceGlobalColor(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Color
	);

/* Setup the target global color value in ARGB8 format. */
gceSTATUS 
gcoHARDWARE_SetTargetGlobalColor(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Color
	);

/* Setup the source and target pixel multiply modes. */
gceSTATUS 
gcoHARDWARE_SetMultiplyModes(
	IN gcoHARDWARE Hardware,
	IN gce2D_PIXEL_COLOR_MULTIPLY_MODE SrcPremultiplySrcAlpha,
	IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstPremultiplyDstAlpha,
	IN gce2D_GLOBAL_COLOR_MULTIPLY_MODE SrcPremultiplyGlobalMode,
	IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstDemultiplyDstAlpha
	);

/* Setup the source, target and pattern transparency modes. */
gceSTATUS 
gcoHARDWARE_SetTransparencyModes(
	IN gcoHARDWARE Hardware,
	IN gce2D_TRANSPARENCY SrcTransparency,
	IN gce2D_TRANSPARENCY DstTransparency,
	IN gce2D_TRANSPARENCY PatTransparency
	);

/* Setup the source, target and pattern transparency modes. 
   Used only for have backward compatibility.
*/
gceSTATUS 
gcoHARDWARE_SetAutoTransparency(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 FgRop,
	IN gctUINT8 BgRop
	);

/* Setup the source color key value in ARGB8 format. */
gceSTATUS 
gcoHARDWARE_SetSourceColorKeyRange(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 ColorLow,
	IN gctUINT32 ColorHigh,
	IN gctBOOL ColorPack
	);

/* Setup the YUV color space mode. */
gceSTATUS gcoHARDWARE_YUVColorMode(
	IN gcoHARDWARE Hardware,
	IN gce2D_YUV_COLOR_MODE  Mode
	);

/* Save mono colors for later programming. */
gceSTATUS gcoHARDWARE_SaveMonoColors(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 FgColor,
	IN gctUINT32 BgColor
	);

/* Save transparency color for later programming. */
gceSTATUS gcoHARDWARE_SaveTransparencyColor(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Color32
	);
	
/* Set clipping rectangle. */
gceSTATUS
gcoHARDWARE_SetClipping(
	IN gcoHARDWARE Hardware,
	IN gcsRECT_PTR Rect
	);

/* Configure destination. */
gceSTATUS
gcoHARDWARE_SetTarget(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface
	);

/* Set the target color format. */
gceSTATUS
gcoHARDWARE_SetTargetFormat(
	IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT Format
	);

/* Setup the destination color key value in ARGB8 format. */
gceSTATUS 
gcoHARDWARE_SetTargetColorKeyRange(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 ColorLow,
	IN gctUINT32 ColorHigh
	);

/* Load solid (single) color pattern. */
gceSTATUS
gcoHARDWARE_LoadSolidColorPattern(
	IN gcoHARDWARE Hardware,
	IN gctBOOL ColorConvert,
	IN gctUINT32 Color,
	IN gctUINT64 Mask
	);

/* Load monochrome pattern. */
gceSTATUS
gcoHARDWARE_LoadMonochromePattern(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 OriginX,
	IN gctUINT32 OriginY,
	IN gctBOOL ColorConvert,
	IN gctUINT32 FgColor,
	IN gctUINT32 BgColor,
	IN gctUINT64 Bits,
	IN gctUINT64 Mask
	);

/* Load color pattern. */
gceSTATUS
gcoHARDWARE_LoadColorPattern(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 OriginX,
	IN gctUINT32 OriginY,
	IN gctUINT32 Address,
	IN gceSURF_FORMAT Format,
	IN gctUINT64 Mask
	);

/* Calculate stretch factor. */
gctUINT32 
gcoHARDWARE_GetStretchFactor(
	IN gctINT32 SrcSize,
	IN gctINT32 DestSize
	);

/* Calculate the stretch factors. */
gceSTATUS
gcoHARDWARE_GetStretchFactors(
	IN gcsRECT_PTR SrcRect,
	IN gcsRECT_PTR DestRect,
	OUT gctUINT32 * HorFactor,
	OUT gctUINT32 * VerFactor
	);

/* Calculate and program the stretch factors. */
gceSTATUS
gcoHARDWARE_SetStretchFactors(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 HorFactor,
	IN gctUINT32 VerFactor
	);

/* Determines the usage of 2D resources (source/pattern/destination). */
void
gcoHARDWARE_Get2DResourceUsage(
	IN gctUINT8 FgRop,
	IN gctUINT8 BgRop,
	IN gctUINT32 Transparency,
	OUT gctBOOL_PTR UseSource,
	OUT gctBOOL_PTR UsePattern,
	OUT gctBOOL_PTR UseDestination
	);

/* Set 2D clear color in A8R8G8B8 format. */
gceSTATUS
gcoHARDWARE_Set2DClearColor(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Color,
	IN gctBOOL ColorConvert
	);

/* Enable/disable 2D BitBlt mirrorring. */
gceSTATUS
gcoHARDWARE_SetBitBlitMirror(
	IN gcoHARDWARE Hardware,
	IN gctBOOL HorizontalMirror,
	IN gctBOOL VerticalMirror
	);

/* Start a DE command. */
gceSTATUS
gcoHARDWARE_StartDE(
	IN gcoHARDWARE Hardware,
	IN gce2D_COMMAND Command,
	IN gctUINT32 SrcRectCount,
	IN gcsRECT_PTR SrcRect,
	IN gctUINT32 DestRectCount,
	IN gcsRECT_PTR DestRect,
	IN gctUINT32 FgRop,
	IN gctUINT32 BgRop
	);

/* Start a DE command to draw one or more Lines, 
   with a common or individual color. */
gceSTATUS
gcoHARDWARE_StartDELine(
	IN gcoHARDWARE Hardware,
	IN gce2D_COMMAND Command,
	IN gctUINT32 RectCount,
	IN gcsRECT_PTR DestRect,
	IN gctUINT32 ColorCount,
	IN gctUINT32_PTR Color32,
	IN gctUINT32 FgRop,
	IN gctUINT32 BgRop
	);

/* Start a DE command with a monochrome stream. */
gceSTATUS
gcoHARDWARE_StartDEStream(
	IN gcoHARDWARE Hardware,
	IN gcsRECT_PTR DestRect,
	IN gctUINT32 FgRop,
	IN gctUINT32 BgRop,
	IN gctUINT32 StreamSize,
	OUT gctPOINTER * StreamBits
	);

/* Set kernel size. */
gceSTATUS
gcoHARDWARE_SetKernelSize(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 HorKernelSize,
	IN gctUINT8 VerKernelSize
	);

/* Set filter type. */
gceSTATUS
gcoHARDWARE_SetFilterType(
	IN gcoHARDWARE Hardware,
	IN gceFILTER_TYPE FilterType
	);

/* Set the filter kernel array by user. */
gceSTATUS gcoHARDWARE_SetUserFilterKernel(
	IN gcoHARDWARE Hardware,
	IN gceFILTER_PASS_TYPE PassType,
	IN gctUINT16_PTR KernelArray
	);

/* Select the pass(es) to be done for user defined filter. */
gceSTATUS gcoHARDWARE_EnableUserFilterPasses(
	IN gcoHARDWARE Hardware,
	IN gctBOOL HorPass,
	IN gctBOOL VerPass
	);

/* Frees the kernel weight array. */
gceSTATUS
gcoHARDWARE_FreeKernelArray(
	IN gcoHARDWARE Hardware
	);

/* Frees the temporary buffer allocated by filter blit operation. */
gceSTATUS
gcoHARDWARE_FreeFilterBuffer(
	IN gcoHARDWARE Hardware
	);

/* Filter blit. */
gceSTATUS
gcoHARDWARE_FilterBlit(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR SrcSurface,
	IN gcsSURF_INFO_PTR DestSurface,
	IN gcsRECT_PTR SrcRect,
	IN gcsRECT_PTR DestRect,
	IN gcsRECT_PTR DestSubRect
	);

/* Enable alpha blending engine in the hardware and disengage the ROP engine. */
gceSTATUS
gcoHARDWARE_EnableAlphaBlend(
	IN gcoHARDWARE Hardware,
	IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
	IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
	IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
	IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
	IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
	IN gceSURF_BLEND_FACTOR_MODE DstFactorMode,
	IN gceSURF_PIXEL_COLOR_MODE SrcColorMode,
	IN gceSURF_PIXEL_COLOR_MODE DstColorMode
	);

/* Disable alpha blending engine in the hardware and engage the ROP engine. */
gceSTATUS
gcoHARDWARE_DisableAlphaBlend(
	IN gcoHARDWARE Hardware
	);

/* Set the GPU clock cycles, after which the idle 2D engine 
   will trigger a flush. */
gceSTATUS
gcoHARDWARE_SetAutoFlushCycles(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Cycles
	);

gceSTATUS 
gcoHARDWARE_ColorConvertToARGB8(
	IN gceSURF_FORMAT Format,
	IN gctUINT32 NumColors,
	IN gctUINT32_PTR Color,
	OUT gctUINT32_PTR Color32
	);

gceSTATUS 
gcoHARDWARE_ColorConvertFromARGB8(
	IN gceSURF_FORMAT Format,
	IN gctUINT32 NumColors,
	IN gctUINT32_PTR Color32,
	OUT gctUINT32_PTR Color
	);

gceSTATUS 
gcoHARDWARE_ColorPackFromARGB8(
	IN gceSURF_FORMAT Format,
	IN gctUINT32 Color32,
	OUT gctUINT32_PTR Color
	);

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoHARDWARE 3D ------------------------------*/

/* Query if a surface is renderable or not. */
gceSTATUS
gcoHARDWARE_IsSurfaceRenderable(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface
	);

/* Initialize the 3D hardware. */
gceSTATUS
gcoHARDWARE_Initialize3D(
	IN gcoHARDWARE Hardware
	);

/* Query the OpenGL ES 2.0 capabilities. */
gceSTATUS
gcoHARDWARE_QueryOpenGL2(
	OUT gctBOOL * OpenGL2
	);

/* Query the stream capabilities. */
gceSTATUS
gcoHARDWARE_QueryStreamCaps(
	IN gcoHARDWARE Hardware,
	OUT gctUINT * MaxAttributes,
	OUT gctUINT * MaxStreamSize,
	OUT gctUINT * NumberOfStreams,
	OUT gctUINT * Alignment
	);

/* Flush the evrtex caches. */
gceSTATUS
gcoHARDWARE_FlushVertex(
	IN gcoHARDWARE Hardware
	);

/* Flush the evrtex caches. */
gceSTATUS
gcoHARDWARE_FlushL2Cache(
	IN gcoHARDWARE Hardware
	);

/* Query the index capabilities. */
gceSTATUS
gcoHARDWARE_QueryIndexCaps(
	OUT gctBOOL * Index8,
	OUT gctBOOL * Index16,
	OUT gctBOOL * Index32,
	OUT gctUINT * MaxIndex
	);

/* Query the target capabilities. */
gceSTATUS
gcoHARDWARE_QueryTargetCaps(
	IN gcoHARDWARE Hardware,
	OUT gctUINT * MaxWidth,
	OUT gctUINT * MaxHeight,
	OUT gctUINT * MultiTargetCount,
	OUT gctUINT * MaxSamples
	);

/* Query the texture capabilities. */
gceSTATUS
gcoHARDWARE_QueryTextureCaps(
	OUT gctUINT * MaxWidth,
	OUT gctUINT * MaxHeight,
	OUT gctUINT * MaxDepth,
	OUT gctBOOL * Cubic,
	OUT gctBOOL * NonPowerOfTwo,
	OUT gctUINT * VertexSamplers,
	OUT gctUINT * PixelSamplers
	);

/* Query the shader support. */
gceSTATUS
gcoHARDWARE_QueryShaderCaps(
	OUT gctUINT * VertexUniforms,
	OUT gctUINT * FragmentUniforms,
	OUT gctUINT * Varyings
	);

gceSTATUS
gcoHARDWARE_GetClosestTextureFormat(
	IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT InFormat,
	OUT gceSURF_FORMAT* OutFormat
	);

/* Query the texture support. */
gceSTATUS
gcoHARDWARE_QueryTexture(
	IN gceSURF_FORMAT Format,
	IN gctUINT Level,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctUINT Depth,
	IN gctUINT Faces,
	OUT gctUINT * WidthAlignment,
	OUT gctUINT * HeightAlignment,
	OUT gctSIZE_T * SliceSize
	);

/* Upload data into a texture. */
gceSTATUS
gcoHARDWARE_UploadTexture(
	IN gcoHARDWARE Hardware,
	IN gceSURF_FORMAT TargetFormat,
	IN gctUINT32 Address,
	IN gctPOINTER Logical,
	IN gctUINT32 Offset,
	IN gctINT TargetStride,
	IN gctUINT X,
	IN gctUINT Y,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctCONST_POINTER Memory,
	IN gctINT SourceStride,
	IN gceSURF_FORMAT SourceFormat
	);

/* Flush the texture cache. */
gceSTATUS
gcoHARDWARE_FlushTexture(
	IN gcoHARDWARE Hardware
	);

/* Set the texture addressing mode. */
gceSTATUS
gcoHARDWARE_SetTextureAddressingMode(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gceTEXTURE_WHICH Which,
	IN gceTEXTURE_ADDRESSING Mode
	);

/* Set the unsigned integer texture border color. */
gceSTATUS
gcoHARDWARE_SetTextureBorderColor(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctUINT Red,
	IN gctUINT Green,
	IN gctUINT Blue,
	IN gctUINT Alpha
	);

/* Set the fixed point texture border color. */
gceSTATUS
gcoHARDWARE_SetTextureBorderColorX(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFIXED_POINT Red,
	IN gctFIXED_POINT Green,
	IN gctFIXED_POINT Blue,
	IN gctFIXED_POINT Alpha
	);

/* Set the floating point texture border color. */
gceSTATUS
gcoHARDWARE_SetTextureBorderColorF(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	);

/* Set the texture minification filter. */
gceSTATUS
gcoHARDWARE_SetTextureMinFilter(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gceTEXTURE_FILTER Filter
	);

/* Set the texture magnification filter. */
gceSTATUS
gcoHARDWARE_SetTextureMagFilter(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gceTEXTURE_FILTER Filter
	);

/* Set the texture mip map filter. */
gceSTATUS
gcoHARDWARE_SetTextureMipFilter(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gceTEXTURE_FILTER Filter
	);

/* Set the fixed point bias for the level of detail. */
gceSTATUS
gcoHARDWARE_SetTextureLODBiasX(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFIXED_POINT Bias
	);

/* Set the floating point bias for the level of detail. */
gceSTATUS
gcoHARDWARE_SetTextureLODBiasF(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFLOAT Bias
	);

/* Set the fixed point minimum value for the level of detail. */
gceSTATUS
gcoHARDWARE_SetTextureLODMinX(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFIXED_POINT LevelOfDetail
	);

/* Set the floating point minimum value for the level of detail. */
gceSTATUS
gcoHARDWARE_SetTextureLODMinF(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFLOAT LevelOfDetail
	);

/* Set the fixed point maximum value for the level of detail. */
gceSTATUS
gcoHARDWARE_SetTextureLODMaxX(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFIXED_POINT LevelOfDetail
	);

/* Set the floating point maximum value for the level of detail. */
gceSTATUS
gcoHARDWARE_SetTextureLODMaxF(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctFLOAT LevelOfDetail
	);

/* Set texture format. */
gceSTATUS
gcoHARDWARE_SetTextureFormat(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gceSURF_FORMAT Format,
	IN gctUINT Width,
	IN gctUINT Height,
	IN gctUINT Depth,
	IN gctUINT Faces
	);

/* Set texture LOD address. */
gceSTATUS
gcoHARDWARE_SetTextureLOD(
	IN gcoHARDWARE Hardware,
	IN gctINT Sampler,
	IN gctINT LevelOfDetail,
	IN gctUINT32 Address,
	IN gctINT Stride
	);

/* Clear wrapper to distinguish between software and resolve(3d) clear cases. */
gceSTATUS
gcoHARDWARE_ClearRect(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctPOINTER LogicalAddress,
	IN gctUINT32 Stride,
	IN gctINT Left,
	IN gctINT Top,
	IN gctINT Right,
	IN gctINT Bottom,
	IN gceSURF_FORMAT Format,
	IN gctUINT32 ClearValue,
	IN gctUINT8 ClearMask
	);

/* Append a TILE STATUS CLEAR command to a command queue. */
gceSTATUS
gcoHARDWARE_ClearTileStatus(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface,
	IN gctUINT32 Address,
	IN gceSURF_TYPE Type,
	IN gctUINT32 ClearValue,
	IN gctUINT8 ClearMask
	);

gceSTATUS
gcoHARDWARE_SetRenderTarget(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface
	);

gceSTATUS
gcoHARDWARE_SetDepthBuffer(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_INFO_PTR Surface
	);

gceSTATUS
gcoHARDWARE_SetAPI(
	IN gcoHARDWARE Hardware,
	IN gceAPI Api
	);

gceSTATUS
gcoHARDWARE_SetViewport(
	IN gcoHARDWARE Hardware,
	IN gctINT32 Left,
	IN gctINT32 Top,
	IN gctINT32 Right,
	IN gctINT32 Bottom
	);

gceSTATUS
gcoHARDWARE_SetScissors(
	IN gcoHARDWARE Hardware,
	IN gctINT32 Left,
	IN gctINT32 Top,
	IN gctINT32 Right,
	IN gctINT32 Bottom
	);

gceSTATUS
gcoHARDWARE_SetShading(
	IN gcoHARDWARE Hardware,
	IN gceSHADING Shading
	);

gceSTATUS
gcoHARDWARE_SetBlendEnable(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enabled
	);

gceSTATUS
gcoHARDWARE_SetBlendFunctionSource(
	IN gcoHARDWARE Hardware,
	IN gceBLEND_FUNCTION FunctionRGB,
	IN gceBLEND_FUNCTION FunctionAlpha
	);

gceSTATUS
gcoHARDWARE_SetBlendFunctionTarget(
	IN gcoHARDWARE Hardware,
	IN gceBLEND_FUNCTION FunctionRGB,
	IN gceBLEND_FUNCTION FunctionAlpha
	);

gceSTATUS
gcoHARDWARE_SetBlendMode(
	IN gcoHARDWARE Hardware,
	IN gceBLEND_MODE ModeRGB,
	IN gceBLEND_MODE ModeAlpha
	);

gceSTATUS
gcoHARDWARE_SetBlendColor(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Red,
	IN gctUINT8 Green,
	IN gctUINT8 Blue,
	IN gctUINT8 Alpha
	);

gceSTATUS
gcoHARDWARE_SetBlendColorX(
	IN gcoHARDWARE Hardware,
	IN gctFIXED_POINT Red,
	IN gctFIXED_POINT Green,
	IN gctFIXED_POINT Blue,
	IN gctFIXED_POINT Alpha
	);

gceSTATUS
gcoHARDWARE_SetBlendColorF(
	IN gcoHARDWARE Hardware,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	);

gceSTATUS
gcoHARDWARE_SetCulling(
	IN gcoHARDWARE Hardware,
	IN gceCULL Mode
	);

gceSTATUS
gcoHARDWARE_SetPointSizeEnable(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetPointSprite(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetFill(
	IN gcoHARDWARE Hardware,
	IN gceFILL Mode
	);

gceSTATUS
gcoHARDWARE_SetDepthCompare(
	IN gcoHARDWARE Hardware,
	IN gceCOMPARE DepthCompare
	);

gceSTATUS
gcoHARDWARE_SetDepthWrite(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetDepthMode(
	IN gcoHARDWARE Hardware,
	IN gceDEPTH_MODE DepthMode
	);

gceSTATUS
gcoHARDWARE_SetDepthRangeX(
	IN gcoHARDWARE Hardware,
	IN gceDEPTH_MODE DepthMode,
	IN gctFIXED_POINT Near,
	IN gctFIXED_POINT Far
	);

gceSTATUS
gcoHARDWARE_SetDepthRangeF(
	IN gcoHARDWARE Hardware,
	IN gceDEPTH_MODE DepthMode,
	IN gctFLOAT Near,
	IN gctFLOAT Far
	);

gceSTATUS
gcoHARDWARE_SetDepthScaleBiasX(
	IN gcoHARDWARE Hardware,
	IN gctFIXED_POINT DepthScale,
	IN gctFIXED_POINT DepthBias
	);

gceSTATUS
gcoHARDWARE_SetDepthScaleBiasF(
	IN gcoHARDWARE Hardware,
	IN gctFLOAT DepthScale,
	IN gctFLOAT DepthBias
	);

gceSTATUS
gcoHARDWARE_SetLastPixelEnable(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetDither(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetColorWrite(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Enable
	);

gceSTATUS
gcoHARDWARE_SetEarlyDepth(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetStencilMode(
	IN gcoHARDWARE Hardware,
	IN gceSTENCIL_MODE Mode
	);

gceSTATUS
gcoHARDWARE_SetStencilMask(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Mask
	);

gceSTATUS
gcoHARDWARE_SetStencilWriteMask(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Mask
	);

gceSTATUS
gcoHARDWARE_SetStencilReference(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Reference
	);

gceSTATUS
gcoHARDWARE_SetStencilCompare(
	IN gcoHARDWARE Hardware,
	IN gceSTENCIL_WHERE Where,
	IN gceCOMPARE Compare
	);

gceSTATUS
gcoHARDWARE_SetStencilPass(
	IN gcoHARDWARE Hardware,
	IN gceSTENCIL_WHERE Where,
	IN gceSTENCIL_OPERATION Operation
	);

gceSTATUS
gcoHARDWARE_SetStencilFail(
	IN gcoHARDWARE Hardware,
	IN gceSTENCIL_WHERE Where,
	IN gceSTENCIL_OPERATION Operation
	);

gceSTATUS
gcoHARDWARE_SetStencilDepthFail(
	IN gcoHARDWARE Hardware,
	IN gceSTENCIL_WHERE Where,
	IN gceSTENCIL_OPERATION Operation
	);

gceSTATUS
gcoHARDWARE_SetAlphaTest(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetAlphaCompare(
	IN gcoHARDWARE Hardware,
	IN gceCOMPARE Compare
	);

gceSTATUS
gcoHARDWARE_SetAlphaReference(
	IN gcoHARDWARE Hardware,
	IN gctUINT8 Reference
	);

gceSTATUS
gcoHARDWARE_SetAlphaReferenceX(
	IN gcoHARDWARE Hardware,
	IN gctFIXED_POINT Reference
	);

gceSTATUS
gcoHARDWARE_SetAlphaReferenceF(
	IN gcoHARDWARE Hardware,
	IN gctFLOAT Reference
	);

gceSTATUS
gcoHARDWARE_BindStream(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctINT Stride,
	IN gctUINT Number
	);

gceSTATUS
gcoHARDWARE_BindIndex(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gceINDEX_TYPE IndexType
	);

gceSTATUS
gcoHARDWARE_SetAntiAliasLine(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetAALineTexSlot(
	IN gcoHARDWARE Hardware,
	IN gctUINT TexSlot
	);

gceSTATUS
gcoHARDWARE_SetAALineWidth(
	IN gcoHARDWARE Hardware,
	IN gctFLOAT Width
	);


/* Draw a number of primitives. */
gceSTATUS
gcoHARDWARE_DrawPrimitives(
	IN gcoHARDWARE Hardware,
	IN gcePRIMITIVE Type,
	IN gctINT StartVertex,
	IN gctSIZE_T PrimitiveCount
	);

/* Draw a number of primitives using offsets. */
gceSTATUS
gcoHARDWARE_DrawPrimitivesOffset(
	IN gcoHARDWARE Hardware,
	IN gcePRIMITIVE Type,
	IN gctINT32 StartOffset,
	IN gctSIZE_T PrimitiveCount
	);

/* Draw a number of indexed primitives. */
gceSTATUS
gcoHARDWARE_DrawIndexedPrimitives(
	IN gcoHARDWARE Hardware,
	IN gcePRIMITIVE Type,
	IN gctINT BaseVertex,
	IN gctINT StartIndex,
	IN gctSIZE_T PrimitiveCount
	);

/* Draw a number of indexed primitives using offsets. */
gceSTATUS
gcoHARDWARE_DrawIndexedPrimitivesOffset(
	IN gcoHARDWARE Hardware,
	IN gcePRIMITIVE Type,
	IN gctINT32 BaseOffset,
	IN gctINT32 StartOffset,
	IN gctSIZE_T PrimitiveCount
	);

/* Copy data into video memory. */
gceSTATUS
gcoHARDWARE_CopyData(
	IN gcoHARDWARE Hardware,
	IN gcsSURF_NODE_PTR Memory,
	IN gctUINT32 Offset,
	IN gctCONST_POINTER Buffer,
	IN gctSIZE_T Bytes
	);

gceSTATUS
gcoHARDWARE_SetStream(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Index,
	IN gctUINT32 Address,
	IN gctUINT32 Stride
	);

gceSTATUS
gcoHARDWARE_SetAttributes(
	IN gcoHARDWARE Hardware,
	IN gcsVERTEX_ATTRIBUTES_PTR Attributes,
	IN gctUINT32 AttributeCount
	);

gceSTATUS
gcoHARDWARE_QuerySamplerBase(
	IN gcoHARDWARE Hardware,
	OUT gctSIZE_T * VertexCount,
	OUT gctINT_PTR VertexBase,
	OUT gctSIZE_T * FragmentCount,
	OUT gctINT_PTR FragmentBase
	);

gceSTATUS
gcoHARDWARE_SetDepthOnly(
	IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
	);

gceSTATUS
gcoHARDWARE_SetCentroids(
	IN gcoHARDWARE Hardware,
	IN gctUINT32	Index,
	IN gctPOINTER	Centroids
	);

/* Append a CLEAR command to a command queue. */
gceSTATUS
gcoHARDWARE_Clear(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 Address,
	IN gctUINT32 Stride,
	IN gctINT32 Left,
	IN gctINT32 Top,
	IN gctINT32 Right,
	IN gctINT32 Bottom,
	IN gceSURF_FORMAT Format,
	IN gctUINT32 ClearValue,
	IN gctUINT8 ClearMask
	);

/* Software clear. */
gceSTATUS
gcoHARDWARE_ClearSoftware(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER LogicalAddress,
	IN gctUINT32 Stride,
	IN gctINT32 Left,
	IN gctINT32 Top,
	IN gctINT32 Right,
	IN gctINT32 Bottom,
	IN gceSURF_FORMAT Format,
	IN gctUINT32 ClearValue,
	IN gctUINT8 ClearMask
	);

/* Verifies whether 2D engine is available. */
gctBOOL
gcoHARDWARE_Is2DAvailable(
    IN gcoHARDWARE Hardware
    );

/* Sets the software 2D renderer force flag. */
gceSTATUS
gcoHARDWARE_UseSoftware2D(
    IN gcoHARDWARE Hardware,
	IN gctBOOL Enable
    );

/* Sets the maximum number of brushes in the cache. */
gceSTATUS
gcoHARDWARE_SetBrushLimit(
	IN gcoHARDWARE Hardware,
	IN gctUINT MaxCount
	);

/* Return a pointer to the brush cache. */
gceSTATUS
gcoHARDWARE_GetBrushCache(
	IN gcoHARDWARE Hardware,
	IN OUT gcoBRUSH_CACHE * BrushCache
	);

/* Program the brush. */
gceSTATUS
gcoHARDWARE_FlushBrush(
	IN gcoHARDWARE Hardware,
	IN gcoBRUSH Brush
	);

/* Clear one or more rectangular areas. */
gceSTATUS
gcoHARDWARE_Clear2D(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 RectCount,
	IN gcsRECT_PTR Rect,
	IN gctUINT32 Color,
	IN gctBOOL ColorConvert,
	IN gctUINT8 FgRop,
	IN gctUINT8 BgRop
	);

/* Draw one or more Bresenham lines using a brush. */
gceSTATUS
gcoHARDWARE_Line2D(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 LineCount,
	IN gcsRECT_PTR Position,
	IN gcoBRUSH Brush,
	IN gctUINT8 FgRop,
	IN gctUINT8 BgRop
	);

/* Draw one or more Bresenham lines using solid color(s). */
gceSTATUS
gcoHARDWARE_Line2DEx(
	IN gcoHARDWARE Hardware,
	IN gctUINT32 LineCount,
	IN gcsRECT_PTR Position,
	IN gctUINT32 ColorCount,
	IN gctUINT32_PTR Color32,
	IN gctUINT8 FgRop,
	IN gctUINT8 BgRop
	);

/* Monochrome blit. */
gceSTATUS
gcoHARDWARE_MonoBlit(
	IN gcoHARDWARE Hardware,
	IN gctPOINTER StreamBits,
	IN gcsPOINT_PTR StreamSize,
	IN gcsRECT_PTR StreamRect,
	IN gceSURF_MONOPACK SrcStreamPack,
	IN gceSURF_MONOPACK DestStreamPack,
	IN gcsRECT_PTR DestRect,
	IN gctUINT32 FgRop,
	IN gctUINT32 BgRop
	);

/******************************************************************************\
******************************** gcoBUFFER Object *******************************
\******************************************************************************/

/* Construct a new gcoBUFFER object. */
gceSTATUS
gcoBUFFER_Construct(
	IN gcoHAL Hal,
	IN gcoHARDWARE Hardware,
	IN gctSIZE_T MaxSize,
	OUT gcoBUFFER * Buffer
	);

/* Destroy an gcoBUFFER object. */
gceSTATUS
gcoBUFFER_Destroy(
	IN gcoBUFFER Buffer
	);

/* Reserve space in a command buffer. */
gceSTATUS
gcoBUFFER_Reserve(
	IN gcoBUFFER Buffer,
	IN gctSIZE_T Bytes,
	IN gctBOOL Aligned,
	OUT gctPOINTER * Memory
	);

/* Write data into the command buffer. */
gceSTATUS
gcoBUFFER_Write(
	IN gcoBUFFER Buffer,
	IN gctCONST_POINTER Data,
	IN gctSIZE_T Bytes,
	IN gctBOOL Aligned
	);

/* Write 32-bit data into the command buffer. */
gceSTATUS
gcoBUFFER_Write32(
	IN gcoBUFFER Buffer,
	IN gctUINT32 Data,
	IN gctBOOL Aligned
	);

/* Write 64-bit data into the command buffer. */
gceSTATUS
gcoBUFFER_Write64(
	IN gcoBUFFER Buffer,
	IN gctUINT64 Data,
	IN gctBOOL Aligned
	);

/* Commit the command buffer. */
gceSTATUS
gcoBUFFER_Commit(
	IN gcoBUFFER Buffer,
	IN gcoCONTEXT Context,
	IN gcoQUEUE Queue
	);

/******************************************************************************\
******************************** gcoCMDBUF Object *******************************
\******************************************************************************/

typedef struct _gcsCOMMAND_INFO	* gcsCOMMAND_INFO_PTR;

/* Construct a new gcoCMDBUF object. */
gceSTATUS
gcoCMDBUF_Construct(
	IN gcoOS Os,
	IN gcoHARDWARE Hardware,
	IN gctSIZE_T Bytes,
	IN gcsCOMMAND_INFO_PTR Info,
	IN gcoCMDBUF Parent,
	OUT gcoCMDBUF * Buffer
	);

/* Destroy an gcoCMDBUF object. */
gceSTATUS
gcoCMDBUF_Destroy(
	IN gcoCMDBUF Buffer
	);

/******************************************************************************\
******************************* gcoCONTEXT Object *******************************
\******************************************************************************/

gceSTATUS
gcoCONTEXT_Construct(
	IN gcoOS Os,
	IN gcoHARDWARE Hardware,
	OUT gcoCONTEXT * Context
	);

gceSTATUS
gcoCONTEXT_Destroy(
	IN gcoCONTEXT Context
	);

gceSTATUS
gcoCONTEXT_Buffer(
	IN gcoCONTEXT Context,
	IN gctUINT32 Address,
	IN gctSIZE_T Count,
	IN gctUINT32_PTR Data
	);

gceSTATUS
gcoCONTEXT_BufferX(
	IN gcoCONTEXT Context,
	IN gctUINT32 Address,
	IN gctSIZE_T Count,
	IN gctFIXED_POINT * Data
	);

gceSTATUS
gcoCONTEXT_PreCommit(
	IN OUT gcoCONTEXT Context
	);

gceSTATUS
gcoCONTEXT_PostCommit(
	IN OUT gcoCONTEXT Context
	);

/******************************************************************************\
********************************* gcoDUMP Object ********************************
\******************************************************************************/

/* Construct a new gcoDUMP object. */
gceSTATUS
gcoDUMP_Construct(
	IN gcoOS Os,
	IN gcoHAL Hal,
	OUT gcoDUMP * Dump
	);

/* Destroy a gcoDUMP object. */
gceSTATUS
gcoDUMP_Destroy(
	IN gcoDUMP Dump
	);

/* Enable/disable dumping. */
gceSTATUS
gcoDUMP_Control(
	IN gcoDUMP Dump,
	IN gctSTRING FileName
	);

gceSTATUS
gcoDUMP_IsEnabled(
	IN gcoDUMP Dump,
	OUT gctBOOL * Enabled
	);

/* Add surface. */
gceSTATUS
gcoDUMP_AddSurface(
	IN gcoDUMP Dump,
	IN gctINT32 Width,
	IN gctINT32 Height,
	IN gceSURF_FORMAT PixelFormat,
	IN gctUINT32 Address,
	IN gctSIZE_T ByteCount
	);

/* Mark the beginning of a frame. */
gceSTATUS
gcoDUMP_FrameBegin(
	IN gcoDUMP Dump
	);

/* Mark the end of a frame. */
gceSTATUS
gcoDUMP_FrameEnd(
	IN gcoDUMP Dump
	);

/* Dump data. */
gceSTATUS
gcoDUMP_DumpData(
	IN gcoDUMP Dump,
	IN gceDUMP_TAG Type,
	IN gctUINT32 Address,
	IN gctSIZE_T ByteCount,
	IN gctCONST_POINTER Data
	);

/* Delete an address. */
gceSTATUS
gcoDUMP_Delete(
	IN gcoDUMP Dump,
	IN gctUINT32 Address
	);

/******************************************************************************\
********************************* gcoSURF Object ********************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoSURF Common ------------------------------*/

/* Color format component parameters. */
typedef struct _gcsFORMAT_COMPONENT
{
	gctUINT8					start;
	gctUINT8					width;
}
gcsFORMAT_COMPONENT;

/* RGBA color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_RGBA
{
	gcsFORMAT_COMPONENT			alpha;
	gcsFORMAT_COMPONENT			red;
	gcsFORMAT_COMPONENT			green;
	gcsFORMAT_COMPONENT			blue;
}
gcsFORMAT_CLASS_TYPE_RGBA;

/* YUV color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_YUV
{
	gcsFORMAT_COMPONENT			y;
	gcsFORMAT_COMPONENT			u;
	gcsFORMAT_COMPONENT			v;
}
gcsFORMAT_CLASS_TYPE_YUV;

/* Index color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_INDEX
{
	gcsFORMAT_COMPONENT			value;
}
gcsFORMAT_CLASS_TYPE_INDEX;

/* Luminance color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_LUMINANCE
{
	gcsFORMAT_COMPONENT			alpha;
	gcsFORMAT_COMPONENT			value;
}
gcsFORMAT_CLASS_TYPE_LUMINANCE;

/* Bump map color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_BUMP
{
	gcsFORMAT_COMPONENT			alpha;
	gcsFORMAT_COMPONENT			l;
	gcsFORMAT_COMPONENT			v;
	gcsFORMAT_COMPONENT			u;
	gcsFORMAT_COMPONENT			q;
	gcsFORMAT_COMPONENT			w;
}
gcsFORMAT_CLASS_TYPE_BUMP;

/* Depth and stencil format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_DEPTH
{
	gcsFORMAT_COMPONENT			depth;
	gcsFORMAT_COMPONENT			stencil;
}
gcsFORMAT_CLASS_TYPE_DEPTH;

/* Format parameters. */
typedef struct _gcsSURF_FORMAT_INFO
{
	/* Format code and class. */
	gceSURF_FORMAT				format;
	gceFORMAT_CLASS				fmtClass;

	/* The size of one pixel in bits. */
	gctUINT8					bitsPerPixel;

	/* Component swizzle. */
	gceSURF_SWIZZLE				swizzle;

	/* Some formats have two neighbour pixels interleaved together. */
	/* To describe such format, set the flag to 1 and add another   */
	/* like this one describing the odd pixel format.               */
	gctUINT8					interleaved;

	/* Format components. */
	union
	{
		gcsFORMAT_CLASS_TYPE_BUMP		bump;
		gcsFORMAT_CLASS_TYPE_RGBA		rgba;
		gcsFORMAT_CLASS_TYPE_YUV		yuv;
		gcsFORMAT_CLASS_TYPE_LUMINANCE	lum;
		gcsFORMAT_CLASS_TYPE_INDEX		index;
		gcsFORMAT_CLASS_TYPE_DEPTH		depth;
	};
}
gcsSURF_FORMAT_INFO;

/* Frame buffer information. */
typedef struct _gcsSURF_FRAMEBUFFER
{
	gctPOINTER					logical;
	gctUINT						width, height;
	gctINT						stride;
	gceSURF_FORMAT				format;
}
gcsSURF_FRAMEBUFFER;

/* Generic pixel component descriptors. */
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_XXX8;
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_XX8X;
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_X8XX;
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_8XXX;

#ifdef __cplusplus
}
#endif

#endif /* __aqhaluser_h_ */
