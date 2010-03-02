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
#include <stdarg.h>

/******************************************************************************\
******************************** Debug Variables *******************************
\******************************************************************************/

static gceSTATUS gk_LastError  = gcvSTATUS_OK;
static gctUINT32 gk_DebugLevel = 0;
static gctUINT32 gk_DebugZone  = 0;

static void _Print(
	char *Message,
	va_list Arguments
	)
{
	char buffer[1024];
	int n;
	
	/* Print message to buffer. */
	n = vsnprintf(buffer, sizeof(buffer), Message, Arguments);
	if ((n <= 0) || (buffer[n - 1] != '\n'))
	{
		/* Append new-line. */
		strncat(buffer, "\n", sizeof(buffer));
	}

	/* Output to debugger. */	
	printk(buffer);
}

/******************************************************************************\
********************************* Debug Macros *********************************
\******************************************************************************/

#define _DEBUGPRINT(Message) \
{ \
	va_list arguments; \
	\
	va_start(arguments, Message); \
	_Print(Message, arguments); \
	va_end(arguments); \
}

/******************************************************************************\
********************************** Debug Code **********************************
\******************************************************************************/

/*******************************************************************************
**
**	gcoOS_DebugTrace
**
**	Send a leveled message to the debugger.
**
**	INPUT:
**
**		gctUINT32 Level
**			Debug level of message.
**
**		char * Message
**			Pointer to message.
**
**		...
**			Optional arguments.
**
**	OUTPUT:
**
**		Nothing.
*/

void gcoOS_DebugTrace(
	IN gctUINT32 Level,
	IN char *Message,
	...
	)
{

	if (Level > gk_DebugLevel)
	{
		return;
	}

	_DEBUGPRINT(Message);
}


/*******************************************************************************
**
**	gcoOS_DebugTraceZone
**
**	Send a leveled and zoned message to the debugger.
**
**	INPUT:
**
**		gctUINT32 Level
**			Debug level for message.
**
**		gctUINT32 Zone
**			Debug zone for message.
**
**		char * Message
**			Pointer to message.
**
**		...
**			Optional arguments.
**
**	OUTPUT:
**
**		Nothing.
*/

void gcoOS_DebugTraceZone(
	IN gctUINT32 Level,
	IN gctUINT32 Zone,
	IN char * Message,
	...
	)
{

	if ((Level > gk_DebugLevel) || !(Zone & gk_DebugZone))
	{
		return;
	}

	_DEBUGPRINT(Message);
}


/*******************************************************************************
**
**	gcoOS_DebugBreak
**
**	Break into the debugger.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		Nothing.
*/
void gcoOS_DebugBreak(
	void
	)
{
	gcoOS_DebugTrace(
		gcvLEVEL_INFO,
		"%s(%d)\n",
		__FUNCTION__,
		__LINE__
		);
}

/*******************************************************************************
**
**	gcoOS_DebugFatal
**
**	Send a message to the debugger and break into the debugger.
**
**	INPUT:
**
**		char * Message
**			Pointer to message.
**
**		...
**			Optional arguments.
**
**	OUTPUT:
**
**		Nothing.
*/
void
gcoOS_DebugFatal(
	IN char* Message,
	...
	)
{
	_DEBUGPRINT(Message);

	/* Break into the debugger. */
	gcoOS_DebugBreak();
}

/*******************************************************************************
**
**	gcoOS_SetDebugLevel
**
**	Set the debug level.
**
**	INPUT:
**
**		gctUINT32 Level
**			New debug level.
**
**	OUTPUT:
**
**		Nothing.
*/

void gcoOS_SetDebugLevel(
	gctUINT32 Level
	)
{
	gk_DebugLevel = Level;
}


/*******************************************************************************
**
**	gcoOS_SetDebugZone
**
**	Set the debug zone.
**
**	INPUT:
**
**		gctUINT32 Zone
**			New debug zone.
**
**	OUTPUT:
**
**		Nothing.
*/
void gcoOS_SetDebugZone(
	gctUINT32 Zone
	)
{
	gk_DebugZone = Zone;
}

/*******************************************************************************
**
**	gcoOS_Verify
**
**	Called to verify the result of a function call.
**
**	INPUT:
**
**		gceSTATUS Status
**			Function call result.
**
**	OUTPUT:
**
**		Nothing.
*/

void
gcoOS_Verify(
	IN gceSTATUS Status
	)
{
	gk_LastError = Status;
	//gcmASSERT(Status == gcvSTATUS_OK);
}
