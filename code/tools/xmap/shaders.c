/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <string.h>
#include <math.h>
#include "../common/cmdlib.h"
#include "../common/inout.h"
#include "../common/mathlib.h"
#include "../common/imagelib.h"
#include "../common/scriplib.h"
#include "../common/qfiles.h"
#include "../common/surfaceflags.h"

#include "shaders.h"

// 5% backsplash by default
#define	DEFAULT_BACKSPLASH_FRACTION		0.05
#define	DEFAULT_BACKSPLASH_DISTANCE		24


#define	MAX_SURFACE_INFO	4096*2

shaderInfo_t    defaultInfo;
shaderInfo_t    shaderInfo[MAX_SURFACE_INFO];
int             numShaderInfo;


typedef struct
{
	char*           name;
	int             clearSolid, surfaceFlags, contents;
} infoParm_t;

// *INDENT-OFF*
infoParm_t	infoParms[] = {
	// server relevant contents
	{"water",			1,	0,	CONTENTS_WATER},
	{"slime",			1,	0,	CONTENTS_SLIME},		// mildly damaging
	{"lava",			1,	0,	CONTENTS_LAVA},			// very damaging
	{"playerclip",		1,	0,	CONTENTS_PLAYERCLIP},
	{"monsterclip",		1,	0,	CONTENTS_MONSTERCLIP},
	{"moveableclip",	1,	0,	0},						// FIXME
	{"ikclip",			1,	0,	0},						// FIXME
	{"nodrop",			1,	0,	CONTENTS_NODROP},		// don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid",		1,	SURF_NONSOLID,	0},			// clears the solid flag
	
	{"blood",			1,	0,	CONTENTS_WATER},
 
	// utility relevant attributes
	{"origin",			1,	0,	CONTENTS_ORIGIN},		// center of rotating brushes
	{"trans",			0,	0,	CONTENTS_TRANSLUCENT},	// don't eat contained surfaces
	{"translucent",		0,	0,	CONTENTS_TRANSLUCENT},	// don't eat contained surfaces
	{"detail",			0,	0,	CONTENTS_DETAIL},		// don't include in structural bsp
	{"structural",		0,	0,	CONTENTS_STRUCTURAL},	// force into structural bsp even if trnas
	{"areaportal",		1,	0,	CONTENTS_AREAPORTAL},	// divides areas
	{"clusterportal",	1,	0,  CONTENTS_CLUSTERPORTAL},	// for bots
	{"donotenter",  	1,  0,  CONTENTS_DONOTENTER},	// for bots
	{"botclip",			1,  0,  CONTENTS_BOTCLIP},		// for bots
 
	{"fog",				1,	0,	CONTENTS_FOG},			// carves surfaces entering
	{"sky",				0,	SURF_SKY,		0},			// emit light from an environment map
	{"lightfilter",		0,	SURF_LIGHTFILTER, 0},		// filter light going through it
	{"alphashadow",		0,	SURF_ALPHASHADOW, 0},		// test light on a per-pixel basis
	{"hint",			0,	SURF_HINT,		0},			// use as a primary splitter
 
	// server attributes
	{"slick",			0,	SURF_SLICK,		0},
	{"collision",		0,	SURF_COLLISION,	0},
	{"noimpact",		0,	SURF_NOIMPACT,	0},			// don't make impact explosions or marks
	{"nomarks",			0,	SURF_NOMARKS,	0},			// don't make impact marks, but still explode
	{"nooverlays",		0,	SURF_NOMARKS,	0},			// don't make impact marks, but still explode
	{"ladder",			0,	SURF_LADDER,	0},
	{"nodamage",		0,	SURF_NODAMAGE,	0},
	{"metalsteps",		0,	SURF_METALSTEPS,0},
	{"flesh",			0,	SURF_FLESH,		0},
	{"nosteps",			0,	SURF_NOSTEPS,	0},
 
	// drawsurf attributes
	{"nodraw",			0,	SURF_NODRAW,		0},		// don't generate a drawsurface (or a lightmap)
	{"pointlight",		0,	SURF_POINTLIGHT,	0},		// sample lighting at vertexes
	{"nolightmap",		0,	SURF_NOLIGHTMAP,	0},		// don't generate a lightmap
	{"nodlight",		0,	0,					0},		// OBSELETE: don't ever add dynamic lights
	{"dust",			0,	SURF_DUST,			0},		// leave a dust trail when walking on this surface
	
	// unsupported Doom3 surface types for sound effects and blood splats
	{"metal",			0,	SURF_METALSTEPS,	0},
	{"stone",			0,	0,				0},
	{"wood",			0,	0,				0},
	{"cardboard",		0,	0,				0},
	{"liquid",			0,	0,				0},
	{"glass",			0,	0,				0},
	{"plastic",			0,	0,				0},
	{"ricochet",		0,	0,				0},
	{"surftype10",		0,	0,				0},
	{"surftype11",		0,	0,				0},
	{"surftype12",		0,	0,				0},
	{"surftype13",		0,	0,				0},
	{"surftype14",		0,	0,				0},
	{"surftype15",		0,	0,				0},
	
	// other unsupported Doom3 surface types
	{"trigger",			0,	0,				0},
	{"flashlight_trigger",0,0,				0},
	{"aassolid",		0,	0,				0},
	{"aasobstacle",		0,	0,				0},
	{"nullNormal",		0,	0,				0},
	{"discrete",		0,	0,				0},
	
};
// *INDENT-ON*

/*
===============
LoadShaderImage
===============
*/

static byte*    LoadImageFile( char* filename )
{
	byte*           buffer = NULL;
	int             nLen = 0;
	
	if( FileExists( filename ) )
	{
		LoadFileBlock( filename, ( void** )&buffer );
	}
	
	if( buffer == NULL )
	{
		nLen = strlen( filename );
		filename[nLen - 3] = 'j';
		filename[nLen - 2] = 'p';
		filename[nLen - 1] = 'g';
		
		if( FileExists( filename ) )
		{
			LoadFileBlock( filename, ( void** )&buffer );
		}
	}
	return buffer;
}

/*
===============
LoadShaderImage
===============
*/
static void LoadShaderImage( shaderInfo_t* si )
{
	char            filename[1024];
	int             i, count;
	float           color[4];
	byte*           buffer;
	qboolean        bTGA = qtrue;
	
#if 0
	// look for the lightimage if it is specified
	if( si->lightimage[0] )
	{
		sprintf( filename, "%s%s", gamedir, si->lightimage );
		DefaultExtension( filename, ".tga" );
		buffer = LoadImageFile( filename, &bTGA );
		if( buffer != NULL )
		{
			goto loadTga;
		}
	}
	
	// look for the editorimage if it is specified
	if( si->editorimage[0] )
	{
		sprintf( filename, "%s%s", gamedir, si->editorimage );
		DefaultExtension( filename, ".tga" );
		buffer = LoadImageFile( filename, &bTGA );
		if( buffer != NULL )
		{
			goto loadTga;
		}
	}
	
	// just try the shader name with a .tga
	// on unix, we have case sensitivity problems...
	sprintf( filename, "%s%s.tga", gamedir, si->shader );
	buffer = LoadImageFile( filename, &bTGA );
	if( buffer != NULL )
	{
		goto loadTga;
	}
	
	sprintf( filename, "%s%s.TGA", gamedir, si->shader );
	buffer = LoadImageFile( filename, &bTGA );
	if( buffer != NULL )
	{
		goto loadTga;
	}
	
	// couldn't load anything
	Sys_Printf( "WARNING: Couldn't find image for shader %s\n", si->shader );
#endif
	
	si->color[0] = 1;
	si->color[1] = 1;
	si->color[2] = 1;
	si->width = 64;
	si->height = 64;
	si->pixels = malloc( si->width * si->height * 4 );
	memset( si->pixels, 255, si->width * si->height * 4 );
	return;
	
	// load the image to get dimensions and color
loadTga:
	if( bTGA )
	{
		LoadTGABuffer( buffer, &si->pixels, &si->width, &si->height );
	}
	else
	{
		LoadJPGBuffer( buffer, &si->pixels, &si->width, &si->height );
	}
	
	free( buffer );
	
	count = si->width * si->height;
	
	VectorClear( color );
	color[3] = 0;
	for( i = 0; i < count; i++ )
	{
		color[0] += si->pixels[i * 4 + 0];
		color[1] += si->pixels[i * 4 + 1];
		color[2] += si->pixels[i * 4 + 2];
		color[3] += si->pixels[i * 4 + 3];
	}
	ColorNormalize( color, si->color );
	VectorScale( color, 1.0 / count, si->averageColor );
}

/*
===============
AllocShaderInfo
===============
*/
static shaderInfo_t* AllocShaderInfo( void )
{
	shaderInfo_t*   si;
	
	if( numShaderInfo == MAX_SURFACE_INFO )
	{
		Error( "MAX_SURFACE_INFO" );
	}
	si = &shaderInfo[numShaderInfo];
	numShaderInfo++;
	
	// set defaults
	
	si->contents = CONTENTS_SOLID;
	
	si->backsplashFraction = DEFAULT_BACKSPLASH_FRACTION;
	si->backsplashDistance = DEFAULT_BACKSPLASH_DISTANCE;
	
	si->lightmapSampleSize = 0;
	si->hasPasses = qfalse;
	si->forceTraceLight = qfalse;
	si->forceVLight = qfalse;
	si->patchShadows = qfalse;
	si->vertexShadows = qfalse;
	si->noVertexShadows = qfalse;
	si->forceSunLight = qfalse;
	si->vertexScale = 1.0;
	si->notjunc = qfalse;
	
	return si;
}

/*
===============
ShaderInfoForShader
===============
*/
shaderInfo_t*   ShaderInfoForShader( const char* shaderName )
{
	int             i;
	shaderInfo_t*   si;
	char            shader[MAX_QPATH];
	
	// strip off extension
	strcpy( shader, shaderName );
	StripExtension( shader );
	
//  Sys_FPrintf(SYS_VRB, "looking for shader '%s' ...\n", shaderName);

	// search for it
	for( i = 0; i < numShaderInfo; i++ )
	{
		si = &shaderInfo[i];
		if( !Q_stricmp( shader, si->shader ) )
		{
			if( !si->width )
			{
				LoadShaderImage( si );
			}
			return si;
		}
	}
	
	si = AllocShaderInfo();
	strcpy( si->shader, shader );
	
	LoadShaderImage( si );
	
	return si;
}

/*
===============
ParseShaderFile
===============
*/
static void ParseShaderFile( const char* filename )
{
	int             i;
	int             numInfoParms = sizeof( infoParms ) / sizeof( infoParms[0] );
	shaderInfo_t*   si;
	
//  Sys_FPrintf(SYS_VRB,  "shaderFile: %s\n", filename );
	LoadScriptFile( filename, -1 );
	while( 1 )
	{
		if( !GetToken( qtrue ) )
		{
			break;
		}
		
		// skip shader tables
		if( !Q_stricmp( token, "table" ) )
		{
			// skip table name
			GetToken( qtrue );
			
			SkipBracedSection();
			continue;
		}
		// FIXME: support shader templates
		else if( !Q_stricmp( token, "guide" ) )
		{
			// parse shader name
			GetToken( qtrue );
			
			Sys_FPrintf( SYS_VRB, "shader '%s' is guided\n", token );
			
			//si = AllocShaderInfo();
			//strcpy(si->shader, token);
			
			// skip guide name
			GetToken( qtrue );
			
			// skip parameters
			MatchToken( "(" );
			
			while( 1 )
			{
				GetToken( qtrue );
				
				if( !token[0] )
				{
					break;
				}
				
				if( !Q_stricmp( token, ")" ) )
				{
					break;
				}
			}
			continue;
		}
		else
		{
			si = AllocShaderInfo();
			strcpy( si->shader, token );
			MatchToken( "{" );
		}
		
		while( 1 )
		{
			if( !GetToken( qtrue ) )
			{
				break;
			}
			if( !strcmp( token, "}" ) )
			{
				break;
			}
			
			// skip internal braced sections
			if( !strcmp( token, "{" ) )
			{
				si->hasPasses = qtrue;
				while( 1 )
				{
					if( !GetToken( qtrue ) )
					{
						break;
					}
					if( !strcmp( token, "}" ) )
					{
						break;
					}
				}
				continue;
			}
			
			// qer_editorimage <image>
			if( !Q_stricmp( token, "qer_editorimage" ) )
			{
				GetToken( qfalse );
				strcpy( si->editorimage, token );
				DefaultExtension( si->editorimage, ".tga" );
				continue;
			}
			
			// xmap_lightimage <image>
			if( !Q_stricmp( token, "xmap_lightimage" ) )
			{
				GetToken( qfalse );
				strcpy( si->lightimage, token );
				DefaultExtension( si->lightimage, ".tga" );
				continue;
			}
			
			// xmap_surfacelight <value>
			if( !Q_stricmp( token, "xmap_surfacelight" ) )
			{
				GetToken( qfalse );
				si->value = atoi( token );
				continue;
			}
			
			// xmap_lightsubdivide <value>
			if( !Q_stricmp( token, "xmap_lightsubdivide" ) )
			{
				GetToken( qfalse );
				si->lightSubdivide = atoi( token );
				continue;
			}
			
			// xmap_lightmapsamplesize <value>
			if( !Q_stricmp( token, "xmap_lightmapsamplesize" ) )
			{
				GetToken( qfalse );
				si->lightmapSampleSize = atoi( token );
				continue;
			}
			
			// xmap_tracelight
			if( !Q_stricmp( token, "xmap_tracelight" ) )
			{
				si->forceTraceLight = qtrue;
				continue;
			}
			
			// xmap_vlight
			if( !Q_stricmp( token, "xmap_vlight" ) )
			{
				si->forceVLight = qtrue;
				continue;
			}
			
			// xmap_patchshadows
			if( !Q_stricmp( token, "xmap_patchshadows" ) )
			{
				si->patchShadows = qtrue;
				continue;
			}
			
			// xmap_vertexshadows
			if( !Q_stricmp( token, "xmap_vertexshadows" ) )
			{
				si->vertexShadows = qtrue;
				continue;
			}
			
			// xmap_novertexshadows
			if( !Q_stricmp( token, "xmap_novertexshadows" ) )
			{
				si->noVertexShadows = qtrue;
				continue;
			}
			
			// xmap_forcesunlight
			if( !Q_stricmp( token, "xmap_forcesunlight" ) )
			{
				si->forceSunLight = qtrue;
				continue;
			}
			
			// xmap_vertexscale
			if( !Q_stricmp( token, "xmap_vertexscale" ) )
			{
				GetToken( qfalse );
				si->vertexScale = atof( token );
				continue;
			}
			
			// xmap_notjunc
			if( !Q_stricmp( token, "xmap_notjunc" ) )
			{
				si->notjunc = qtrue;
				continue;
			}
			
			// xmap_globaltexture
			if( !Q_stricmp( token, "xmap_globaltexture" ) )
			{
				si->globalTexture = qtrue;
				continue;
			}
			
			// xmap_backsplash <percent> <distance>
			if( !Q_stricmp( token, "xmap_backsplash" ) )
			{
				GetToken( qfalse );
				si->backsplashFraction = atof( token ) * 0.01;
				GetToken( qfalse );
				si->backsplashDistance = atof( token );
				continue;
			}
			
			// xmap_backshader <shader>
			if( !Q_stricmp( token, "xmap_backshader" ) )
			{
				GetToken( qfalse );
				strcpy( si->backShader, token );
				continue;
			}
			
			// xmap_flare <shader>
			if( !Q_stricmp( token, "xmap_flare" ) )
			{
				GetToken( qfalse );
				strcpy( si->flareShader, token );
				continue;
			}
			
			// light <value>
			// old style flare specification
			if( !Q_stricmp( token, "light" ) )
			{
				GetToken( qfalse );
				strcpy( si->flareShader, "flareshader" );
				continue;
			}
			
			// xmap_sun <red> <green> <blue> <intensity> <degrees> <elivation>
			// color will be normalized, so it doesn't matter what range you use
			// intensity falls off with angle but not distance 100 is a fairly bright sun
			// degree of 0 = from the east, 90 = north, etc.  altitude of 0 = sunrise/set, 90 = noon
			if( !Q_stricmp( token, "xmap_sun" ) )
			{
				float           a, b;
				
				GetToken( qfalse );
				si->sunLight[0] = atof( token );
				GetToken( qfalse );
				si->sunLight[1] = atof( token );
				GetToken( qfalse );
				si->sunLight[2] = atof( token );
				
				VectorNormalize( si->sunLight );
				
				GetToken( qfalse );
				a = atof( token );
				VectorScale( si->sunLight, a, si->sunLight );
				
				GetToken( qfalse );
				a = atof( token );
				a = a / 180 * Q_PI;
				
				GetToken( qfalse );
				b = atof( token );
				b = b / 180 * Q_PI;
				
				si->sunDirection[0] = cos( a ) * cos( b );
				si->sunDirection[1] = sin( a ) * cos( b );
				si->sunDirection[2] = sin( b );
				
				si->surfaceFlags |= SURF_SKY;
				continue;
			}
			
			// tesssize is used to force liquid surfaces to subdivide
			if( !Q_stricmp( token, "tesssize" ) )
			{
				GetToken( qfalse );
				si->subdivisions = atof( token );
				continue;
			}
			
			// cull none will set twoSided
			if( !Q_stricmp( token, "cull" ) )
			{
				GetToken( qfalse );
				if( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "disable" ) || !Q_stricmp( token, "twosided" ) )
				{
					si->twoSided = qtrue;
				}
				continue;
			}
			
			// twosided will set twoSided
			if( !Q_stricmp( token, "twosided" ) )
			{
				si->twoSided = qtrue;
				// FIXME: implies noshadows
				continue;
			}
			
			// forceOpaque will override translucent
			if( !Q_stricmp( token, "forceOpaque" ) )
			{
				si->forceOpaque = qtrue;
				continue;
			}
			
			// forceSolid will override clearSolid
			if( !Q_stricmp( token, "forceSolid" ) || !Q_stricmp( token, "solid" ) )
			{
				si->forceSolid = qtrue;
				continue;
			}
			
			// deformVertexes autosprite[2]
			// we catch this so autosprited surfaces become point
			// lights instead of area lights
			if( !Q_stricmp( token, "deformVertexes" ) )
			{
				GetToken( qfalse );
				if( !Q_strncasecmp( token, "autosprite", 10 ) )
				{
					si->autosprite = qtrue;
					si->contents = CONTENTS_DETAIL;
				}
				continue;
			}
			
			// deform sprite
			// we catch this so autosprited surfaces become point
			// lights instead of area lights
			if( !Q_stricmp( token, "deform" ) )
			{
				GetToken( qfalse );
				if( !Q_stricmp( token, "sprite" ) )
				{
					si->autosprite = qtrue;
					si->contents = CONTENTS_DETAIL;
				}
				// FIXME: support dynamic doom3 style flare surfaces
				if( !Q_stricmp( token, "flare" ) )
				{
					si->surfaceFlags |= SURF_NOMARKS;
					si->surfaceFlags |= SURF_NODRAW;
					si->surfaceFlags |= SURF_NOLIGHTMAP;
				}
				continue;
			}
			
			// noFragment
			if( !Q_stricmp( token, "noFragment" ) || !Q_stricmp( token, "xmap_noFragment" ) )
			{
				si->noFragment = qtrue;
				continue;
			}
			
			// check if this shader has shortcut passes
			if( !Q_stricmp( token, "diffusemap" ) || !Q_stricmp( token, "bumpmap" ) || !Q_stricmp( token, "specularmap" ) )
			{
				si->hasPasses = qtrue;
				continue;
			}
			
			// check for surface parameters
			if( !Q_stricmp( token, "surfaceparm" ) )
			{
				GetToken( qfalse );
				for( i = 0; i < numInfoParms; i++ )
				{
					if( !Q_stricmp( token, infoParms[i].name ) )
					{
						si->surfaceFlags |= infoParms[i].surfaceFlags;
						si->contents |= infoParms[i].contents;
						
						if( infoParms[i].clearSolid && !si->forceSolid )
						{
							si->contents &= ~CONTENTS_SOLID;
						}
						break;
					}
				}
				if( i == numInfoParms )
				{
					// we will silently ignore all tokens beginning with qer,
					// which are QuakeEdRadient parameters
					if( Q_strncasecmp( token, "qer", 3 ) )
					{
						Sys_Printf( "Unknown surfaceparm: \"%s\"\n", token );
					}
				}
				continue;
			}
			else
			{
				for( i = 0; i < numInfoParms; i++ )
				{
					if( !Q_stricmp( token, infoParms[i].name ) )
					{
						si->surfaceFlags |= infoParms[i].surfaceFlags;
						si->contents |= infoParms[i].contents;
						if( infoParms[i].clearSolid )
						{
							si->contents &= ~CONTENTS_SOLID;
						}
						break;
					}
				}
				if( i != numInfoParms )
				{
					continue;
				}
			}
			
			// ignore all other tokens on the line
			while( TokenAvailable() )
			{
				GetToken( qfalse );
			}
		}
		
		// Tr3B - default shader to invisible if no rendering passes defined
		if( !si->hasPasses )
		{
			Sys_FPrintf( SYS_VRB, "shader '%s' has no passes\n", si->shader );
			si->surfaceFlags |= SURF_NOMARKS;
			si->surfaceFlags |= SURF_NOLIGHTMAP;
			
			// collision surfaces will be ignored by the renderer
			// but the collision code wants the polygons
			if( !( si->surfaceFlags & SURF_COLLISION ) )
			{
				si->surfaceFlags |= SURF_NODRAW;
			}
			
			if( !si->forceOpaque )
			{
				si->contents |= CONTENTS_TRANSLUCENT;
			}
		}
	}
}


/*
===============
LoadShaderInfo
===============
*/
#define	MAX_SHADER_FILES	128
void LoadShaderInfo( void )
{
	char            filename[1024];
	int             i;
	char*           shaderFiles[MAX_SHADER_FILES];
	int             numShaderFiles;
	
	sprintf( filename, "%smaterials/shaderlist.txt", gamedir );
	LoadScriptFile( filename, -1 );
	
	numShaderFiles = 0;
	while( 1 )
	{
		if( !GetToken( qtrue ) )
		{
			break;
		}
		shaderFiles[numShaderFiles] = malloc( MAX_QPATH );
		strcpy( shaderFiles[numShaderFiles], token );
		numShaderFiles++;
	}
	
	for( i = 0; i < numShaderFiles; i++ )
	{
		sprintf( filename, "%smaterials/%s.mtr", gamedir, shaderFiles[i] );
		
		ParseShaderFile( filename );
		free( shaderFiles[i] );
	}
	
	Sys_FPrintf( SYS_VRB, "%5i shaderInfo\n", numShaderInfo );
}
