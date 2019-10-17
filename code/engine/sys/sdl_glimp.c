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

#define USE_SMP

#ifdef USE_SMP
	#include <SDL_thread.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "../sys/sys_local.h"
#include "sdl_icon.h"

#include <GL3/gl3w.h>

typedef enum
{
	RSERR_OK,
	
	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,
	
	RSERR_UNKNOWN
} rserr_t;

SDL_Window* SDL_window = NULL;
static SDL_GLContext SDL_glContext = NULL;

cvar_t*         r_allowResize;	// make window resizable
cvar_t*         r_centerWindow;
cvar_t*         r_sdlDriver;

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();
	
#if defined(SMP)
	if( renderThread != NULL )
	{
		Com_Printf( "Destroying renderer thread...\n" );
		GLimp_ShutdownRenderThread();
	}
#endif
	
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	
	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize( void )
{
	SDL_MinimizeWindow( SDL_window );
}


/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( const char* comment )
{
	/*
		static char		buf[4096];
	
		if(r_logFile->integer && GLEW_GREMEDY_string_marker)
		{
			// copy string and ensure it has a trailing '\0'
			Q_strncpyz(buf, comment, sizeof(buf));
	
			glStringMarkerGREMEDY(strlen(buf), buf);
		}
	*/
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void* a, const void* b )
{
	const float     ASPECT_EPSILON = 0.001f;
	SDL_Rect* 	   modeA = ( SDL_Rect* )a;
	SDL_Rect* 	   modeB = ( SDL_Rect* )b;
	float           aspectA = ( float )modeA->w / ( float )modeA->h;
	float           aspectB = ( float )modeB->w / ( float )modeB->h;
	int             areaA = modeA->w * modeA->h;
	int             areaB = modeB->w * modeB->h;
	float           aspectDiffA = fabs( aspectA - displayAspect );
	float           aspectDiffB = fabs( aspectB - displayAspect );
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
		return areaA - areaB;
	}
}


/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes( void )
{
	int             i;
	char buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect modes[ 128 ];
	int numModes = 0;
	
	int display = SDL_GetWindowDisplayIndex( SDL_window );
	SDL_DisplayMode windowMode;
	
	if( SDL_GetWindowDisplayMode( SDL_window, &windowMode ) < 0 )
	{
		ri.Printf( PRINT_WARNING, "Couldn't get window display mode, no resolutions detected\n" );
		return;
	}
	
	for( i = 0; i < SDL_GetNumDisplayModes( display ); i++ )
	{
		SDL_DisplayMode mode;
		
		if( SDL_GetDisplayMode( display, i, &mode ) < 0 )
		{
			continue;
		}
		
		if( !mode.w || !mode.h )
		{
			ri.Printf( PRINT_ALL, "Display supports any resolution\n" );
			return;
		}
		
		if( windowMode.format != mode.format )
		{
			continue;
		}
		
		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}
	
	if( numModes > 1 )
	{
		qsort( modes, numModes, sizeof( SDL_Rect ), GLimp_CompareModes );
	}
	
	for( i = 0; i < numModes; i++ )
	{
		const char* newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );
		
		if( strlen( newModeString ) < ( int )sizeof( buf ) - strlen( buf ) )
		{
			Q_strcat( buf, sizeof( buf ), newModeString );
		}
		else
		{
			ri.Printf( PRINT_WARNING, "Skipping mode %ux%x, buffer too small\n", modes[ i ].w, modes[ i ].h );
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
static int GLimp_SetMode( int mode, qboolean fullscreen, qboolean noborder, qboolean coreProfileContext )
{
	int 			perChannelColorBits;
	int             colorBits, depthBits, stencilBits;
	int             i = 0;
	SDL_Surface*    icon = NULL;
	Uint32 			flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	SDL_DisplayMode desktopMode;
	int 			display = 0;
	int 			x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
	
	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n" );
	
	if( r_allowResize->integer )
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}
	
	icon = SDL_CreateRGBSurfaceFrom(
			   ( void* )CLIENT_WINDOW_ICON.pixel_data,
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
		   
	// If a window exists, note its display index
	if( SDL_window != NULL )
	{
		display = SDL_GetWindowDisplayIndex( SDL_window );
	}
	
	if( SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
	{
		displayAspect = ( float )desktopMode.w / ( float )desktopMode.h;
		
		ri.Printf( PRINT_ALL, "Display aspect: %.3f\n", displayAspect );
	}
	else
	{
		Com_Memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );
		
		ri.Printf( PRINT_ALL, "Cannot estimate display aspect, assuming 1.333\n" );
	}
	
	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );
	
	if( mode == -2 )
	{
		// use desktop video resolution
		if( desktopMode.h > 0 )
		{
			glConfig.vidWidth = desktopMode.w;
			glConfig.vidHeight = desktopMode.h;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			ri.Printf( PRINT_ALL, "Cannot determine display resolution, assuming 640x480\n" );
		}
		
		glConfig.windowAspect = ( float )glConfig.vidWidth / ( float )glConfig.vidHeight;
	}
	else if( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );
	
	// Center window
	if( r_centerWindow->integer && !fullscreen )
	{
		x = ( desktopMode.w / 2 ) - ( glConfig.vidWidth / 2 );
		y = ( desktopMode.h / 2 ) - ( glConfig.vidHeight / 2 );
	}
	
	// Destroy existing state if it exists
	if( SDL_glContext != NULL )
	{
		SDL_GL_DeleteContext( SDL_glContext );
		SDL_glContext = NULL;
	}
	
	if( SDL_window != NULL )
	{
		SDL_GetWindowPosition( SDL_window, &x, &y );
		ri.Printf( PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( SDL_window );
		SDL_window = NULL;
	}
	
	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if( noborder )
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}
		
		glConfig.isFullscreen = qfalse;
	}
	
	colorBits = r_colorbits->value;
	if( ( !colorBits ) || ( colorBits >= 32 ) )
	{
		colorBits = 24;
	}
	
	if( !r_depthbits->value )
	{
		depthBits = 24;
	}
	else
	{
		depthBits = r_depthbits->value;
	}
	stencilBits = r_stencilbits->value;
	
	for( i = 0; i < 16; i++ )
	{
		int testColorBits, testDepthBits, testStencilBits;
		
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
					if( colorBits == 24 )
					{
						colorBits = 16;
					}
					break;
				case 1:
					if( depthBits == 24 )
					{
						depthBits = 16;
					}
					else if( depthBits == 16 )
					{
						depthBits = 8;
					}
				case 3:
					if( stencilBits == 24 )
					{
						stencilBits = 16;
					}
					else if( stencilBits == 16 )
					{
						stencilBits = 8;
					}
			}
		}
		
		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;
		
		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( testColorBits == 24 )
			{
				testColorBits = 16;
			}
		}
		
		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( testDepthBits == 24 )
			{
				testDepthBits = 16;
			}
			else if( testDepthBits == 16 )
			{
				testDepthBits = 8;
			}
		}
		
		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( testStencilBits == 24 )
			{
				testStencilBits = 16;
			}
			else if( testStencilBits == 16 )
			{
				testStencilBits = 8;
			}
			else
			{
				testStencilBits = 0;
			}
		}
		
		if( testColorBits == 24 )
		{
			perChannelColorBits = 8;
		}
		else
		{
			perChannelColorBits = 4;
		}
		
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );
		
		/*
				if(r_stereoEnabled->integer)
				{
					glConfig.stereoEnabled = qtrue;
					SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
				}
				else
				{
					glConfig.stereoEnabled = qfalse;
					SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
				}
		*/
		if( coreProfileContext )
		{
			// lets grab an OpenGL 3.2 core profile context
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, r_glMinMajorVersion->integer ? r_glMinMajorVersion->integer : 3 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, r_glMinMinorVersion->integer ? r_glMinMinorVersion->integer : 2 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
			
			glConfig.driverType = GLDRV_OPENGL3;
		}
		else
		{
			// set r_glCoreProfile to 0 and set the default driver type
			ri.Cvar_SetValue( "r_glCoreProfile", 0 );
			glConfig.driverType = GLDRV_DEFAULT;
		}
		
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		
		if( ( SDL_window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y, glConfig.vidWidth, glConfig.vidHeight, flags ) ) == 0 )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}
		
		if( fullscreen )
		{
			SDL_DisplayMode mode;
			
			switch( testColorBits )
			{
				case 16:
					mode.format = SDL_PIXELFORMAT_RGB565;
					break;
				case 24:
					mode.format = SDL_PIXELFORMAT_RGB24;
					break;
				default:
					ri.Printf( PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits );
					continue;
			}
			
			mode.w = glConfig.vidWidth;
			mode.h = glConfig.vidHeight;
			mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue( "r_displayRefresh" );
			mode.driverdata = NULL;
			
			if( SDL_SetWindowDisplayMode( SDL_window, &mode ) < 0 )
			{
				ri.Printf( PRINT_DEVELOPER, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
				continue;
			}
		}
		
		SDL_SetWindowTitle( SDL_window, CLIENT_WINDOW_TITLE );
		SDL_SetWindowIcon( SDL_window, icon );
		
		if( ( SDL_glContext = SDL_GL_CreateContext( SDL_window ) ) == NULL )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError( ) );
			continue;
		}
		
		SDL_GL_SetSwapInterval( r_swapInterval->integer );
		
		glConfig.colorBits = testColorBits;
		glConfig.depthBits = testDepthBits;
		glConfig.stencilBits = testStencilBits;
		
		ri.Printf( PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
				   glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
		break;
	}
	
	SDL_FreeSurface( icon );
	
	if( gl3wInit() )
	{
		// gl3wInit failed, something is seriously wrong
		ri.Error( ERR_FATAL, "GLimp_SetMode() - could not load OpenGL subsystem" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "Using OpenGL %s, GLSL %s\n", glGetString( GL_VERSION ), glGetString( GL_SHADING_LANGUAGE_VERSION ) );
	}
	
	GLimp_DetectAvailableModes();
	
	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode( int mode, qboolean fullscreen, qboolean noborder, qboolean coreProfileContext )
{
	rserr_t         err;
	
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		const char* driverName;
		
		if( SDL_Init( SDL_INIT_VIDEO ) == -1 )
		{
			ri.Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError() );
			return qfalse;
		}
		
		driverName = SDL_GetCurrentVideoDriver( );
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
	
	err = GLimp_SetMode( mode, fullscreen, noborder, coreProfileContext );
	
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

static qboolean GLimp_HaveExtension( const char* ext )
{
	const char* ptr;
	
	if( glConfig.driverType == GLDRV_OPENGL3 )
	{
		return qtrue;
	}
	
	ptr = Q_stristr( glConfig.extensions_string, ext );
	if( ptr == NULL )
	{
		return qfalse;
	}
	ptr += strlen( ext );
	return ( ( *ptr == ' ' ) || ( *ptr == '\0' ) ); // verify it's complete string.
}

/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( void )
{
	ri.Printf( PRINT_DEVELOPER, "Initializing OpenGL extensions\n" );
	
//	if(glConfig.driverType != GLDRV_OPENGL3)
	{
		// GL_ARB_occlusion_query
		glConfig2.occlusionQueryAvailable = qfalse;
		glConfig2.occlusionQueryBits = 0;
		if( GLimp_HaveExtension( "GL_ARB_occlusion_query" ) )
		{
			if( r_ext_occlusion_query->value )
			{
				glConfig2.occlusionQueryAvailable = qtrue;
				glGetQueryiv( GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &glConfig2.occlusionQueryBits );
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_occlusion_query\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_occlusion_query\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_occlusion_query not found\n" );
		}
		GL_CheckErrors();
		
		// GL_ARB_vertex_shader
		if( GLimp_HaveExtension( "GL_ARB_vertex_shader" ) )
		{
			int				reservedComponents;
			
			GL_CheckErrors();
			glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS, &glConfig2.maxVertexUniforms );
			GL_CheckErrors();
			//glGetIntegerv(GL_MAX_VARYING_FLOATS, &glConfig.maxVaryingFloats); GL_CheckErrors();
			glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &glConfig2.maxVertexAttribs );
			GL_CheckErrors();
			
			reservedComponents = 16 * 10; // approximation how many uniforms we have besides the bone matrices
			
			/*
			if(glConfig.driverType == GLDRV_MESA)
			{
				// HACK
				// restrict to number of vertex uniforms to 512 because of:
				// xreal.x86_64: nv50_program.c:4181: nv50_program_validate_data: Assertion `p->param_nr <= 512' failed
			
				glConfig2.maxVertexUniforms = Q_bound(0, glConfig2.maxVertexUniforms, 512);
			}
			*/
			
			glConfig2.maxVertexSkinningBones = ( int ) Q_bound( 0.0, ( Q_max( glConfig2.maxVertexUniforms - reservedComponents, 0 ) / 16 ), MAX_BONES );
			glConfig2.vboVertexSkinningAvailable = r_vboVertexSkinning->integer && ( ( glConfig2.maxVertexSkinningBones >= 12 ) ? qtrue : qfalse );
			
			ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_vertex_shader\n" );
		}
		else
		{
			ri.Error( ERR_FATAL, "...GL_ARB_vertex_shader not found\n" );
		}
		GL_CheckErrors();
		
		// GL_ARB_shading_language_100
		if( GLimp_HaveExtension( "GL_ARB_shading_language_100" ) )
		{
			Q_strncpyz( glConfig2.shadingLanguageVersion, ( char* )glGetString( GL_SHADING_LANGUAGE_VERSION ), sizeof( glConfig2.shadingLanguageVersion ) );
			ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_shading_language_100\n" );
		}
		else
		{
			ri.Printf( ERR_FATAL, "...GL_ARB_shading_language_100 not found\n" );
		}
		GL_CheckErrors();
		
		// GL_ARB_texture_non_power_of_two
		glConfig2.textureNPOTAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_ARB_texture_non_power_of_two" ) )
		{
			if( r_ext_texture_non_power_of_two->integer )
			{
				glConfig2.textureNPOTAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_texture_non_power_of_two\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_texture_non_power_of_two\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_texture_non_power_of_two not found\n" );
		}
		
		// GL_ARB_draw_buffers
		glConfig2.drawBuffersAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_ARB_draw_buffers" ) )
		{
			glGetIntegerv( GL_MAX_DRAW_BUFFERS, &glConfig2.maxDrawBuffers );
			
			if( r_ext_draw_buffers->integer )
			{
				glConfig2.drawBuffersAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_draw_buffers\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_draw_buffers\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_draw_buffers not found\n" );
		}
		
		// GL_ARB_half_float_pixel
		glConfig2.textureHalfFloatAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_ARB_half_float_pixel" ) )
		{
			if( r_ext_half_float_pixel->integer )
			{
				glConfig2.textureHalfFloatAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_half_float_pixel\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_half_float_pixel\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_half_float_pixel not found\n" );
		}
		
		// GL_ARB_texture_float
		glConfig2.textureFloatAvailable = qfalse;
		if( GLimp_HaveExtension( "GLEW_ARB_texture_float" ) )
		{
			if( r_ext_texture_float->integer )
			{
				glConfig2.textureFloatAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_texture_float\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_texture_float\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_texture_float not found\n" );
		}
		
		// GL_ARB_texture_compression
		glConfig.textureCompression = TC_NONE;
		if( GLimp_HaveExtension( "GL_ARB_texture_compression" ) )
		{
			if( r_ext_compressed_textures->integer )
			{
				glConfig2.ARBTextureCompressionAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_texture_compression\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_texture_compression\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_texture_compression not found\n" );
		}
		
		// GL_ARB_vertex_array_object
		glConfig2.vertexArrayObjectAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_ARB_vertex_array_object" ) )
		{
			if( r_ext_vertex_array_object->integer )
			{
				glConfig2.vertexArrayObjectAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_vertex_array_object\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_vertex_array_object\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_vertex_array_object not found\n" );
		}
		
		// GL_EXT_texture_compression_s3tc
		if( GLimp_HaveExtension( "GL_EXT_texture_compression_s3tc" ) )
		{
			if( r_ext_compressed_textures->integer )
			{
				glConfig.textureCompression = TC_S3TC;
				ri.Printf( PRINT_DEVELOPER, "...using GL_EXT_texture_compression_s3tc\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_EXT_texture_compression_s3tc\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_EXT_texture_compression_s3tc not found\n" );
		}
		
		// GL_EXT_texture3D
		glConfig2.texture3DAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_EXT_texture3D" ) )
		{
			//if(r_ext_texture3d->value)
			{
				glConfig2.texture3DAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_EXT_texture3D\n" );
			}
			/*
			else
			{
				ri.Printf(PRINT_DEVELOPER, "...ignoring GL_EXT_texture3D\n");
			}
			*/
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_EXT_texture3D not found\n" );
		}
		
		// GL_EXT_texture_filter_anisotropic
		glConfig2.textureAnisotropyAvailable = qfalse;
		/*
		if(GLimp_HaveExtension("GL_EXT_texture_filter_anisotropic"))
		{
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig2.maxTextureAnisotropy);
		
			if(r_ext_texture_filter_anisotropic->value)
			{
				glConfig2.textureAnisotropyAvailable = qtrue;
				ri.Printf(PRINT_DEVELOPER, "...using GL_EXT_texture_filter_anisotropic\n");
			}
			else
			{
				ri.Printf(PRINT_DEVELOPER, "...ignoring GL_EXT_texture_filter_anisotropic\n");
			}
		}
		else
		{
			ri.Printf(PRINT_DEVELOPER, "...GL_EXT_texture_filter_anisotropic not found\n");
		}
		GL_CheckErrors();
		*/
		
		// GL_ARB_framebuffer_object
		glConfig2.framebufferObjectAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_ARB_framebuffer_object" ) )
		{
			glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig2.maxRenderbufferSize );
			glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig2.maxColorAttachments );
			
			if( r_ext_framebuffer_object->value )
			{
				glConfig2.framebufferObjectAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_ARB_framebuffer_object\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_ARB_framebuffer_object\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_ARB_framebuffer_object not found\n" );
		}
		GL_CheckErrors();
		
		// GL_EXT_packed_depth_stencil
		glConfig2.framebufferPackedDepthStencilAvailable = qfalse;
		if( GLimp_HaveExtension( "GLEW_EXT_packed_depth_stencil" ) && glConfig.driverType != GLDRV_MESA )
		{
			if( r_ext_packed_depth_stencil->integer )
			{
				glConfig2.framebufferPackedDepthStencilAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_EXT_packed_depth_stencil\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_EXT_packed_depth_stencil\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_EXT_packed_depth_stencil not found\n" );
		}
		
		// GL_EXT_framebuffer_blit
		glConfig2.framebufferBlitAvailable = qfalse;
		if( GLimp_HaveExtension( "GL_EXT_framebuffer_blit" ) )
		{
			if( r_ext_framebuffer_blit->integer )
			{
				glConfig2.framebufferBlitAvailable = qtrue;
				ri.Printf( PRINT_DEVELOPER, "...using GL_EXT_framebuffer_blit\n" );
			}
			else
			{
				ri.Printf( PRINT_DEVELOPER, "...ignoring GL_EXT_framebuffer_blit\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_EXT_framebuffer_blit not found\n" );
		}
		
		// GL_GREMEDY_string_marker
		if( GLimp_HaveExtension( "GL_GREMEDY_string_marker" ) )
		{
			ri.Printf( PRINT_DEVELOPER, "...using GL_GREMEDY_string_marker\n" );
		}
		else
		{
			ri.Printf( PRINT_DEVELOPER, "...GL_GREMEDY_string_marker not found\n" );
		}
	}
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
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE );
	
	if( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}
	
#ifdef __linux__
	XInitThreads();
#endif
	
	Sys_GLimpInit();
	
	// Create the window and set up the context
	if( GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, r_noborder->integer, r_glCoreProfile->integer ) )
	{
		goto success;
	}
	
	// Try again, this time in a platform specific "safe mode"
	Sys_GLimpSafeInit();
	
	if( GLimp_StartDriverAndSetMode( r_mode->integer, r_fullscreen->integer, qfalse, qfalse ) )
	{
		goto success;
	}
	
	// Finally, try the default screen resolution
	if( r_mode->integer != R_MODE_FALLBACK )
	{
		ri.Printf( PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK );
		
		if( GLimp_StartDriverAndSetMode( R_MODE_FALLBACK, qfalse, qfalse, qfalse ) )
		{
			goto success;
		}
	}
	
	// Nothing worked, give up
	ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );
	
success:
	// This values force the UI to disable driver selection
	glConfig.hardwareType = GLHW_GENERIC;
	
	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer &&	SDL_SetWindowBrightness( SDL_window, 1.0f ) >= 0;
	
	// get our config strings
	Q_strncpyz( glConfig.vendor_string, ( char* )glGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, ( char* )glGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	if( *glConfig.renderer_string && glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] == '\n' )
	{
		glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] = 0;
	}
	Q_strncpyz( glConfig.version_string, ( char* )glGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	
	if( glConfig.driverType != GLDRV_OPENGL3 )
	{
		Q_strncpyz( glConfig.extensions_string, ( char* )glGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
	}
	
	if(	Q_stristr( glConfig.renderer_string, "mesa" ) ||
			Q_stristr( glConfig.renderer_string, "gallium" ) ||
			Q_stristr( glConfig.vendor_string, "nouveau" ) ||
			Q_stristr( glConfig.vendor_string, "mesa" ) )
	{
		// suckage
		glConfig.driverType = GLDRV_MESA;
	}
	
	if( Q_stristr( glConfig.renderer_string, "geforce" ) )
	{
		if( Q_stristr( glConfig.renderer_string, "8400" ) ||
				Q_stristr( glConfig.renderer_string, "8500" ) ||
				Q_stristr( glConfig.renderer_string, "8600" ) ||
				Q_stristr( glConfig.renderer_string, "8800" ) ||
				Q_stristr( glConfig.renderer_string, "9500" ) ||
				Q_stristr( glConfig.renderer_string, "9600" ) ||
				Q_stristr( glConfig.renderer_string, "9800" ) ||
				Q_stristr( glConfig.renderer_string, "gts 240" ) ||
				Q_stristr( glConfig.renderer_string, "gts 250" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 260" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 275" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 280" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 285" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 295" ) ||
				Q_stristr( glConfig.renderer_string, "gt 320" ) ||
				Q_stristr( glConfig.renderer_string, "gt 330" ) ||
				Q_stristr( glConfig.renderer_string, "gt 340" ) ||
				Q_stristr( glConfig.renderer_string, "gt 415" ) ||
				Q_stristr( glConfig.renderer_string, "gt 420" ) ||
				Q_stristr( glConfig.renderer_string, "gt 425" ) ||
				Q_stristr( glConfig.renderer_string, "gt 430" ) ||
				Q_stristr( glConfig.renderer_string, "gt 435" ) ||
				Q_stristr( glConfig.renderer_string, "gt 440" ) ||
				Q_stristr( glConfig.renderer_string, "gt 520" ) ||
				Q_stristr( glConfig.renderer_string, "gt 525" ) ||
				Q_stristr( glConfig.renderer_string, "gt 540" ) ||
				Q_stristr( glConfig.renderer_string, "gt 550" ) ||
				Q_stristr( glConfig.renderer_string, "gt 555" ) ||
				Q_stristr( glConfig.renderer_string, "gts 450" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 460" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 470" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 480" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 485" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 560" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 570" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 580" ) ||
				Q_stristr( glConfig.renderer_string, "gtx 590" ) )
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}
	}
	else if( Q_stristr( glConfig.renderer_string, "quadro fx" ) )
	{
		if( Q_stristr( glConfig.renderer_string, "3600" ) )
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}
	}
	else if( Q_stristr( glConfig.renderer_string, "rv770" ) )
	{
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if( Q_stristr( glConfig.renderer_string, "radeon hd" ) )
	{
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if( Q_stristr( glConfig.renderer_string, "eah4850" ) || Q_stristr( glConfig.renderer_string, "eah4870" ) )
	{
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if( Q_stristr( glConfig.renderer_string, "radeon" ) )
	{
		glConfig.hardwareType = GLHW_ATI;
	}
	
	// initialize extensions
	GLimp_InitExtensions();
	
	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );
	
	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init( SDL_window );
}


/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapWindow( SDL_window );
	}
	
	if( r_fullscreen->modified )
	{
		int         	fullscreen;
		qboolean    	needToToggle;
		qboolean        sdlToggled = qfalse;
		
		// Find out the current state
		fullscreen = !!( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN );
		
		if( r_fullscreen->integer && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
		{
			ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
			ri.Cvar_Set( "r_fullscreen", "0" );
			r_fullscreen->modified = qfalse;
		}
		
		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;
		
		if( needToToggle )
		{
			sdlToggled = SDL_SetWindowFullscreen( SDL_window, r_fullscreen->integer ) >= 0;
			
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if( !sdlToggled )
			{
				ri.Cmd_ExecuteText( EXEC_APPEND, "vid_restart" );
			}
			
			ri.IN_Restart();
		}
		
		r_fullscreen->modified = qfalse;
	}
}


#ifdef USE_SMP

/*
===========================================================

SMP acceleration

===========================================================
*/

static SDL_mutex* smpMutex = NULL;
static SDL_cond* renderCommandsEvent = NULL;
static SDL_cond* renderCompletedEvent = NULL;
static void ( *renderThreadFunction )( void ) = NULL;
static SDL_Thread* renderThread = NULL;

/*
===============
GLimp_SetCurrentContext
===============
*/
static void GLimp_SetCurrentContext( qboolean enable )
{
	if( enable )
	{
		SDL_GL_MakeCurrent( SDL_window, SDL_glContext );
	}
	else
	{
		SDL_GL_MakeCurrent( SDL_window, NULL );
	}
}

/*
===============
GLimp_RenderThreadWrapper
===============
*/
static int GLimp_RenderThreadWrapper( void* arg )
{
	renderThreadFunction();
	
	GLimp_SetCurrentContext( qfalse );
	
	return 0;
}

/*
===============
GLimp_SpawnRenderThread
===============
*/
qboolean GLimp_SpawnRenderThread( void ( *function )( void ) )
{
	if( renderThread != NULL ) // hopefully just a zombie at this point...
	{
		ri.Printf( PRINT_WARNING, "Already a render thread? Trying to clean it up...\n" );
		GLimp_ShutdownRenderThread();
	}
	
	smpMutex = SDL_CreateMutex();
	if( smpMutex == NULL )
	{
		ri.Printf( PRINT_WARNING, "smpMutex creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	
	renderCommandsEvent = SDL_CreateCond();
	if( renderCommandsEvent == NULL )
	{
		ri.Printf( PRINT_WARNING, "renderCommandsEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	
	renderCompletedEvent = SDL_CreateCond();
	if( renderCompletedEvent == NULL )
	{
		ri.Printf( PRINT_WARNING, "renderCompletedEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	
	renderThreadFunction = function;
	renderThread = SDL_CreateThread( GLimp_RenderThreadWrapper, "render thread", NULL );
	if( renderThread == NULL )
	{
		ri.Printf( PRINT_WARNING, "SDL_CreateThread() returned %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	
	return qtrue;
}

/*
===============
GLimp_ShutdownRenderThread
===============
*/
void GLimp_ShutdownRenderThread( void )
{
	if( renderThread != NULL )
	{
		GLimp_WakeRenderer( NULL );
		SDL_WaitThread( renderThread, NULL );
		renderThread = NULL;
		glConfig.smpActive = qfalse;
	}
	
	if( smpMutex != NULL )
	{
		SDL_DestroyMutex( smpMutex );
		smpMutex = NULL;
	}
	
	if( renderCommandsEvent != NULL )
	{
		SDL_DestroyCond( renderCommandsEvent );
		renderCommandsEvent = NULL;
	}
	
	if( renderCompletedEvent != NULL )
	{
		SDL_DestroyCond( renderCompletedEvent );
		renderCompletedEvent = NULL;
	}
	
	renderThreadFunction = NULL;
}

static volatile void* smpData = NULL;
static volatile qboolean smpDataReady;

/*
===============
GLimp_RendererSleep
===============
*/
void*           GLimp_RendererSleep( void )
{
	void*           data = NULL;
	
	GLimp_SetCurrentContext( qfalse );
	
	SDL_LockMutex( smpMutex );
	{
		smpData = NULL;
		smpDataReady = qfalse;
		
		// after this, the front end can exit GLimp_FrontEndSleep
		SDL_CondSignal( renderCompletedEvent );
		
		while( !smpDataReady )
		{
			SDL_CondWait( renderCommandsEvent, smpMutex );
		}
		
		data = ( void* )smpData;
	}
	SDL_UnlockMutex( smpMutex );
	
	GLimp_SetCurrentContext( qtrue );
	
	return data;
}

/*
===============
GLimp_SyncRenderThread
===============
*/
void GLimp_SyncRenderThread( void )
{
	GLimp_FrontEndSleep();
	
	GLimp_SetCurrentContext( qtrue );
}

/*
===============
GLimp_FrontEndSleep
===============
*/
void GLimp_FrontEndSleep( void )
{
	SDL_LockMutex( smpMutex );
	{
		while( smpData )
		{
			SDL_CondWait( renderCompletedEvent, smpMutex );
		}
	}
	SDL_UnlockMutex( smpMutex );
	
	GLimp_SetCurrentContext( qtrue );
}

/*
===============
GLimp_WakeRenderer
===============
*/
void GLimp_WakeRenderer( void* data )
{
	GLimp_SetCurrentContext( qfalse );
	
	SDL_LockMutex( smpMutex );
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;
		
		// after this, the renderer can continue through GLimp_RendererSleep
		SDL_CondSignal( renderCommandsEvent );
	}
	SDL_UnlockMutex( smpMutex );
}

#else

// No SMP - stubs
static void GLimp_SetCurrentContext( qboolean enable )
{
}

static int GLimp_RenderThreadWrapper( void* arg )
{
	return 0;
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

void GLimp_SyncRenderThread( void )
{
}

void GLimp_FrontEndSleep( void )
{
}

void GLimp_WakeRenderer( void* data )
{
}

#endif