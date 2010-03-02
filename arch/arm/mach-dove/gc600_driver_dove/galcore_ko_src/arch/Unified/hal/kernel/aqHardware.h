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






#ifndef __aqhardware_h_
#define __aqhardware_h_

/* gcoHARDWARE object. */
struct _gcoHARDWARE
{
    /* Object. */
    gcsOBJECT	    	    	object;

    /* Pointer to gcoKERNEL object. */
    gcoKERNEL	    	    	kernel;

    /* Pointer to gcoOS object. */
    gcoOS   	    	    	os;

#if !USE_EVENT_QUEUE
    /* Last read interrupt status register. */
    gctUINT32	    	    	data;
#endif

    /* Chip characteristics. */
    gceCHIPMODEL    	    	chipModel;
    gctUINT32	    	    	chipRevision;
    gctUINT32	    	    	chipFeatures;
	gctUINT32	    	    	chipMinorFeatures;
    gctBOOL 	    	    	allowFastClear;
	gctUINT32					powerBaseAddress;

	gctUINT32					streamCount;
	gctUINT32					registerMax;
	gctUINT32					threadCount;
	gctUINT32					shaderCoreCount;
	gctUINT32					vertexCacheSize;
	gctUINT32					vertexOutputBufferSize;

    /* Chip status */
    gceCHIPPOWERSTATE          chipPowerState;
};

#endif /* __aqhardware_h_ */
