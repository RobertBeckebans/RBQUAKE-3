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

#ifdef USE_LOCAL_HEADERS
	#include "SDL.h"
#else
	#include <SDL.h>
#endif

#if !SDL_VERSION_ATLEAST(1, 2, 10)
	#define SDL_GL_ACCELERATED_VISUAL 15
	#define SDL_GL_SWAP_CONTROL 16
#elif MINSDL_PATCH >= 10
	#error Code block no longer necessary, please remove
#endif

#ifdef SMP
	#include <SDL_thread.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <d3d10.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "../sys/sys_local.h"
#include "sdl_icon.h"
#include "SDL_syswm.h"




static void GLimp_GetCurrentContext( void )
{
}

static void GLimp_SetCurrentContext( qboolean enable )
{
}

// No SMP - stubs
void GLimp_RenderThreadWrapper( void* arg )
{
}

qboolean GLimp_SpawnRenderThread( void ( *function )( void ) )
{
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n" );
	return qfalse;
}

void GLimp_ShutdownRenderThread( void )
{
}

void*           GLimp_RendererSleep( void )
{
	return NULL;
}

void GLimp_FrontEndSleep( void )
{
}

void GLimp_WakeRenderer( void* data )
{
}


typedef enum
{
	RSERR_OK,
	
	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,
	RSERR_NO_DEVICE,
	
	RSERR_UNKNOWN
} rserr_t;

static SDL_Surface* screen = NULL;

cvar_t*         r_sdlDriver;

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();
	
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	screen = NULL;
	
	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	
#if !defined(USE_D3D10)
	Com_Memset( &glState, 0, sizeof( glState ) );
#endif
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void* a, const void* b )
{
	const float     ASPECT_EPSILON = 0.001f;
	SDL_Rect*       modeA = *( SDL_Rect** ) a;
	SDL_Rect*       modeB = *( SDL_Rect** ) b;
	float           aspectDiffA = fabs( ( ( float )modeA->w / ( float )modeA->h ) - displayAspect );
	float           aspectDiffB = fabs( ( ( float )modeB->w / ( float )modeB->h ) - displayAspect );
	float           aspectDiffsDiff = aspectDiffA - aspectDiffB;
	
	if( aspectDiffsDiff > ASPECT_EPSILON )
	{
		return 1;
	}
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
	{
		return -1;
	}
	else
	{
		if( modeA->w == modeB->w )
		{
			return modeA->h - modeB->h;
		}
		else
		{
			return modeA->w - modeB->w;
		}
	}
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes( void )
{
	char            buf[MAX_STRING_CHARS] = { 0 };
	SDL_Rect**      modes;
	int             numModes;
	int             i;
	
	modes = SDL_ListModes( NULL, SDL_OPENGL | SDL_FULLSCREEN );
	
	if( !modes )
	{
		ri.Printf( PRINT_WARNING, "Can't get list of available modes\n" );
		return;
	}
	
	if( modes == ( SDL_Rect** ) - 1 )
	{
		ri.Printf( PRINT_ALL, "Display supports any resolution\n" );
		return;					// can set any resolution
	}
	
	for( numModes = 0; modes[numModes]; numModes++ );
	
	if( numModes > 1 )
	{
		qsort( modes + 1, numModes - 1, sizeof( SDL_Rect* ), GLimp_CompareModes );
	}
	
	for( i = 0; i < numModes; i++ )
	{
		const char*     newModeString = va( "%ux%u ", modes[i]->w, modes[i]->h );
		
		if( strlen( newModeString ) < ( int )sizeof( buf ) - strlen( buf ) )
		{
			Q_strcat( buf, sizeof( buf ), newModeString );
		}
		else
		{
			ri.Printf( PRINT_WARNING, "Skipping mode %ux%x, buffer too small\n", modes[i]->w, modes[i]->h );
		}
	}
	
	if( *buf )
	{
		buf[strlen( buf ) - 1] = 0;
		ri.Printf( PRINT_ALL, "Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
}

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode( int mode, qboolean fullscreen )
{
	int             sdlcolorbits;
	int             colorbits, depthbits, stencilbits;
	int             tcolorbits, tdepthbits, tstencilbits;
	int             i = 0;
	SDL_Surface*    vidscreen = NULL;
	Uint32          flags = 0;
	const SDL_VideoInfo* videoInfo;
	
	ri.Printf( PRINT_ALL, "Initializing display\n" );
	
	if( displayAspect == 0.0f )
	{
#if !SDL_VERSION_ATLEAST(1, 2, 10)
		// 1.2.10 is needed to get the desktop resolution
		displayAspect = 4.0f / 3.0f;
#else
		// Guess the display aspect ratio through the desktop resolution
		// by assuming (relatively safely) that it is set at or close to
		// the display's native aspect ratio
		videoInfo = SDL_GetVideoInfo();
		displayAspect = ( float )videoInfo->current_w / ( float )videoInfo->current_h;
#endif
		
		ri.Printf( PRINT_ALL, "Estimated display aspect: %.3f\n", displayAspect );
	}
	
	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );
	
	if( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );
	
	if( fullscreen )
	{
		//flags |= SDL_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		glConfig.isFullscreen = qfalse;
	}
	
	if( !r_colorbits->value )
	{
		colorbits = 24;
	}
	else
	{
		colorbits = r_colorbits->value;
	}
	
	if( !r_depthbits->value )
	{
		depthbits = 24;
	}
	else
	{
		depthbits = r_depthbits->value;
	}
	stencilbits = r_stencilbits->value;
	
	for( i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch( i / 4 )
			{
				case 2:
					if( colorbits == 24 )
					{
						colorbits = 16;
					}
					break;
				case 1:
					if( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if( depthbits == 16 )
					{
						depthbits = 8;
					}
				case 3:
					if( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}
		
		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;
		
		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}
		
		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}
		
		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}
		
		sdlcolorbits = 4;
		if( tcolorbits == 24 )
		{
			sdlcolorbits = 8;
		}
		
#ifdef USE_ICON
		{
			SDL_Surface*    icon = SDL_CreateRGBSurfaceFrom( ( void* )CLIENT_WINDOW_ICON.pixel_data,
								   CLIENT_WINDOW_ICON.width,
								   CLIENT_WINDOW_ICON.height,
								   CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
								   CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
								   0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
								   0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
														   );
														   
			SDL_WM_SetIcon( icon, NULL );
			SDL_FreeSurface( icon );
		}
#endif
		
		SDL_WM_SetCaption( CLIENT_WINDOW_TITLE, CLIENT_WINDOW_MIN_TITLE );
		SDL_ShowCursor( 0 );
		
		if( !( vidscreen = SDL_SetVideoMode( glConfig.vidWidth, glConfig.vidHeight, colorbits, flags ) ) )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_SetVideoMode failed: %s\n", SDL_GetError() );
			continue;
		}
		
		//GLimp_GetCurrentContext();
		
		ri.Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
				   sdlcolorbits, sdlcolorbits, sdlcolorbits, tdepthbits, tstencilbits );
				   
		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}
	
	GLimp_DetectAvailableModes();
	
	if( !vidscreen )
	{
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}
	
	screen = vidscreen;
	
	//glstring = (char *)qglGetString(GL_RENDERER);
	//ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glstring);
	
	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode( int mode, qboolean fullscreen )
{
	rserr_t         err;
	
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		char            driverName[64];
		
		ri.Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO )... " );
		if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) == -1 )
		{
			ri.Printf( PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) FAILED (%s)\n", SDL_GetError() );
			return qfalse;
		}
		
		SDL_VideoDriverName( driverName, sizeof( driverName ) - 1 );
		ri.Printf( PRINT_ALL, "SDL using driver \"%s\"\n", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
	}
	
	if( fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}
	
	err = GLimp_SetMode( mode, fullscreen );
	
	switch( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;
		case RSERR_INVALID_MODE:
			ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
			return qfalse;
		default:
			break;
	}
	
	return qtrue;
}

#define R_MODE_FALLBACK 3		// 640 * 480

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( void )
{
	qboolean        success = qtrue;
	
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	
//	Sys_GLimpInit();

	// create the window and set up the context
	if( !GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer ) )
	{
		if( r_mode->integer != R_MODE_FALLBACK )
		{
			ri.Printf( PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK );
			if( !GLimp_StartDriverAndSetMode( R_MODE_FALLBACK, r_fullscreen->integer ) )
			{
				success = qfalse;
			}
		}
		else
		{
			success = qfalse;
		}
	}
	
	if( !success )
	{
		ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );
	}
	
	// This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_DEFAULT;
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = !!( SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0 );
	
	// get our config strings
	Q_strncpyz( glConfig.vendor_string, "Microsoft", sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, "D3D", sizeof( glConfig.renderer_string ) );
	if( *glConfig.renderer_string && glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] == '\n' )
	{
		glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] = 0;
	}
	Q_strncpyz( glConfig.version_string, "10.0", sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, "None", sizeof( glConfig.extensions_string ) );
	
	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );
	
	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init();
}






