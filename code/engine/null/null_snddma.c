/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// snddma_null.c
// all other sound mixing is portable

#include "../client/client.h"

qboolean SNDDMA_Init()
{
	return qfalse;
}

int SNDDMA_GetDMAPos()
{
	return 0;
}

void SNDDMA_Shutdown()
{
}

void SNDDMA_BeginPainting()
{
}

void SNDDMA_Submit()
{
}

#ifdef USE_VOIP
void SNDDMA_StartCapture()
{
}

int SNDDMA_AvailableCaptureSamples()
{
	return 0;
}

void SNDDMA_Capture( int samples, byte* data )
{
}

void SNDDMA_StopCapture()
{
}

void SNDDMA_MasterGain( float val )
{
}
#endif

sfxHandle_t S_RegisterSound( const char* name, qboolean compressed )
{
	return 0;
}

void S_StartLocalSound( sfxHandle_t sfxHandle, int channelNum )
{
}

void S_ClearSoundBuffer()
{
}
