/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
#include "qbsp.h"


convertType_t   convertType = CONVERT_NOTHING;


/*
==================
WriteMapFile
==================
*/
static void WriteMapFile( char* filename )
{
	FILE*           f;
	int             i, j, k, l;
	entity_t*       entity;
	epair_t*        ep;
	bspBrush_t*     brush;
	side_t*         side;
	plane_t*        plane;
	parseMesh_t*    pm;
	
//  winding_t      *w;
	shaderInfo_t*   si;
	
	Sys_Printf( "writing %s\n", filename );
	f = fopen( filename, "wb" );
	if( !f )
	{
		Error( "Can't write %s\b", filename );
	}
	
	fprintf( f, "Version 2\n" );
	
	for( i = 0; i < numEntities; i++ )
	{
		entity = &entities[i];
		
		// write entity header
		fprintf( f, "// entity %i\n", i );
		fprintf( f, "{\n" );
		
		// write epairs
		for( ep = entity->epairs; ep; ep = ep->next )
		{
			fprintf( f, "\"%s\" \"%s\"\n", ep->key, ep->value );
		}
		
		// write brush list
		for( j = 0, brush = entity->brushes; brush; j++, brush = brush->next )
		{
			fprintf( f, "// brush %i\n", j );
			fprintf( f, "{\n" );
			fprintf( f, "brushDef3\n" );
			fprintf( f, "{\n" );
			for( k = 0, side = brush->sides; k < brush->numsides; k++, side++ )
			{
				// write plane equation
				plane = &mapPlanes[side->planenum];
				
				fprintf( f, "( %f %f %f %f ) ", plane->normal[0], plane->normal[1], plane->normal[2], -plane->dist );
				
				// write texture matrix
				Write2DMatrix( f, 2, 3, ( float* )side->texMat );
				
				si = side->shaderInfo;
				fprintf( f, " \"%s\"", si->shader );
				
				// support detail flags
				if( side->contents & CONTENTS_DETAIL )
				{
					fprintf( f, " %i 0 0\n", CONTENTS_DETAIL );
				}
				else
				{
					fprintf( f, " 0 0 0\n" );
				}
				
			}
			fprintf( f, "}\n" );
			fprintf( f, "}\n" );
		}
		
		// write patch list
		for( j = 0, pm = entity->patches; pm; j++, pm = pm->next )
		{
		
			fprintf( f, "// patch %i\n", j );
			fprintf( f, "{\n" );
			
			if( pm->patchDef3 )
			{
				fprintf( f, "patchDef3\n" );
			}
			else
			{
				fprintf( f, "patchDef2\n" );
			}
			
			fprintf( f, "{\n" );
			
			// write shader
			si = pm->shaderInfo;
			fprintf( f, "\"%s\"\n", si->shader );
			
			// write patch dimensions
			if( pm->patchDef3 )
				fprintf( f, "( %i %i %i %i %i %i %i )\n", ( int )pm->info[0], ( int )pm->info[1], ( int )pm->info[2], ( int )pm->info[3],
						 ( int )pm->info[4], ( int )pm->info[5], ( int )pm->info[6] );
			else
			
				fprintf( f, "( %i %i %i %i %i )\n", ( int )pm->info[0], ( int )pm->info[1], ( int )pm->info[2], ( int )pm->info[3],
						 ( int )pm->info[4] );
						 
			fprintf( f, "(\n" );
			for( k = 0; k < pm->mesh.width; k++ )
			{
				fprintf( f, "(" );
				for( l = 0; l < pm->mesh.height; l++ )
				{
					// write drawVert_t::xyz + st
					Write1DMatrix( f, 5, pm->mesh.verts[l * pm->mesh.width + k].xyz );
				}
				fprintf( f, ")\n" );
			}
			fprintf( f, ")\n" );
			
			fprintf( f, "}\n" );
			fprintf( f, "}\n" );
		}
		
		fprintf( f, "}\n" );
	}
	
	fclose( f );
}


int ConvertMapToMap( int argc, char** argv )
{
	int             i;
	double          start, end;
	char            source[1024];
	char            name[1024];
	char            save[1024];
	
	Sys_Printf( "---- convert map to map ----\n" );
	
	for( i = 1; i < argc; i++ )
	{
		if( !strcmp( argv[i], "-threads" ) )
		{
			numthreads = atoi( argv[i + 1] );
			i++;
		}
		else if( !strcmp( argv[i], "-v" ) )
		{
			Sys_Printf( "verbose = true\n" );
			verbose = qtrue;
		}
		else if( !strcmp( argv[i], "-quake3" ) )
		{
			convertType = CONVERT_QUAKE3;
			Sys_Printf( "converting from Quake3 to XreaL\n" );
		}
		else if( !strcmp( argv[i], "-quake4" ) )
		{
			convertType = CONVERT_QUAKE4;
			Sys_Printf( "converting from Quake4 to XreaL\n" );
		}
		else if( !strcmp( argv[i], "-connect" ) )
		{
			Broadcast_Setup( argv[++i] );
		}
		else if( argv[i][0] == '-' )
		{
			Error( "Unknown option \"%s\"", argv[i] );
		}
		else
		{
			break;
		}
	}
	
	if( i != argc - 1 )
	{
		Error( "usage: xmap -map2map [-<switch> [-<switch> ...]] <mapname.map>\n"
			   "\n" "Switches:\n" "   v              = verbose output\n"
			   //"   quake1       = convert from QuakeWorld to XreaL\n"
			   //"   quake2       = convert from Quake2 to XreaL\n"
			   "   quake3         = convert from Quake3 to XreaL\n" "   quake4         = convert from Quake4 to XreaL\n" );
	}
	
	start = I_FloatTime();
	
	ThreadSetDefault();
	
	SetQdirFromPath( argv[i] );
	
	strcpy( source, ExpandArg( argv[i] ) );
	StripExtension( source );
	
	strcpy( name, ExpandArg( argv[i] ) );
	DefaultExtension( name, ".map" );
	
	// start from scratch
	LoadShaderInfo();
	
	LoadMapFile( name );
	
	//
	strcpy( save, ExpandArg( argv[i] ) );
	StripExtension( save );
	strcat( save, "_converted" );
	DefaultExtension( save, ".map" );
	
	WriteMapFile( save );
	
	end = I_FloatTime();
	Sys_Printf( "%5.0f seconds elapsed\n", end - start );
	
	// shut down connection
	Broadcast_Shutdown();
	
	return 0;
}
