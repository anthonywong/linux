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




#ifndef __gcoptions_h_
#define __gcoptions_h_

/*
    USE_EVENT_QUEUE
    
    This define enables the new event management code.  Instead of using one
    interrupt per event, this define will create one interrupt per commit.
*/
#define USE_EVENT_QUEUE				1

/*
    USE_MEMORY_HEAP
    
    This define enables the user memory heap.  This will reduce the system
    memory fragmentation and increase performance.
*/
#define USE_MEMORY_HEAP				1

/*
    USE_SHADER_SYMBOL_TABLE
    
    This define enables the symbol table in shader object.
*/
#define USE_SHADER_SYMBOL_TABLE		1

/*
    USE_SUPER_SAMPLING

    This define enables super-sampling support.
*/
#define USE_SUPER_SAMPLING			0

/*
    PROFILE_HAL_COUNTERS

    This define enables HAL counter profiling support.
    HW and SHADER Counter profiling depends on this.
*/
#define PROFILE_HAL_COUNTERS		1

/*
    PROFILE_HW_COUNTERS

    This define enables HW counter profiling support.
*/
#define PROFILE_HW_COUNTERS			1

/*
    PROFILE_SHADER_COUNTERS

    This define enables SHADER counter profiling support.
*/
#define PROFILE_SHADER_COUNTERS		1

/*
    USE_VALIDATION

    This define enables local validation code and means different things
	depending on the context.  This is used for debugging only.
*/
#define USE_VALIDATION				0

/*
    COMMAND_PROCESSOR_VERSION
    
    The version of the command buffer and task manager.
*/
#define COMMAND_PROCESSOR_VERSION	1

/*
    USE_COMMAND_BUFFER_POOL

    This define enables the new command buffer pool code.
*/
#define USE_COMMAND_BUFFER_POOL		1

/*
	gcdDUMP

	Thias define is used to turn on dumping for playback.
*/
#ifndef gcdDUMP
#  define gcdDUMP					0
#endif

/*
    MRVL_TAVOR_PV2_PATCH
    
    Patch physical address for TavorPV2
*/
#define MRVL_TAVOR_PV2_PATCH                0


/*
    MRVL_TAVOR_PV2_DISABLE_FASTCLEAR
    
    Disable fastclear for TavorPV2
*/
#define MRVL_TAVOR_PV2_DISABLE_FASTCLEAR    0


/*
    MRVL_TAVOR_PV2_DISABLE_YFLIP
*/
#define MRVL_TAVOR_PV2_DISABLE_YFLIP        0


/*
    MRVL Utility Options
*/
#define MRVL_FORCE_MSAA_ON                  0

/*
    MRVL_SWAP_BUFFER_IN_EVERY_DRAW
    
    This define force swapbuffer after every drawElement/drawArray.
*/
#define MRVL_SWAP_BUFFER_IN_EVERY_DRAW      0

#define MRVL_BENCH							0


#define MRVL_EANBLE_COMPRESSION_DXT         0

/* Texture coordinate generation */
#define MRVL_TEXGEN							1

/* Swap buffer optimization */
#define MRVL_OPTI_SWAP_BUFFER               1

/* Disable swap worker thread */
#define MRVL_DISABLE_SWAP_THREAD			0


/*
    Definitions for vendor, renderer and version strings
*/
#define _VENDOR_STRING_             "Marvell Technology Group Ltd"

#define _EGL_VERSION_STRING_        "EGL 1.3 Ver0.8.0.1123";      

#if defined(COMMON_LITE)
#define _OES11_VERSION_STRING_      "OpenGL ES-CL 1.1 Ver0.8.0.1123";
#else
#define _OES11_VERSION_STRING_      "OpenGL ES-CM 1.1 Ver0.8.0.1123";
#endif

#define _OES20_VERSION_STRING_      "OpenGL ES 2.0 Ver0.8.0.1123";
#define _GLSL_ES_VERSION_STRING_    "OpenGL ES GLSL ES 1.00 Ver0.8.0.1123"

#define _OPENVG_VERSION_STRING_     "OpenVG 1.1 Ver0.8.0.1123"

#define _GAL_VERSION_STRING_        "GAL Ver0.8.0.1123"

#endif /* __gcoptions_h_ */





