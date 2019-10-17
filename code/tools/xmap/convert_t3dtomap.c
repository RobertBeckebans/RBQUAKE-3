/*
===========================================================================
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


static qboolean ParseKeyValue( const char* token )
{
	char            key[MAXTOKEN];
	char            value[MAXTOKEN];
	
	//if(!GetToken(qtrue))
	//  Error("ParseKeyValue: EOF without closing brace");
	
	Q_strncpyz( key, token, sizeof( key ) );
	
	while( TokenAvailable() )
	{
		GetToken( qfalse );
		
		if( !strcmp( token, "=" ) )
		{
			break;
		}
		
		Q_strcat( key, sizeof( key ), token );
	}
	
	value[0] = '\0';
	while( TokenAvailable() )
	{
		GetToken( qfalse );
		Q_strcat( value, sizeof( value ), token );
	}
	
	Sys_Printf( "ParseKeyValue: %s=%s\n", key, value );
	
	return qtrue;
}

static qboolean ParseLocation()
{
	char            value[MAXTOKEN];
	
	//  Location=(X=-2176.000000,Y=-1280.000000,Z=0.000000)
	
	GetToken( qfalse );			// =
	GetToken( qfalse );			// (
	GetToken( qfalse );			// X
	GetToken( qfalse );			// =
	
	GetToken( qfalse );
	Q_strncpyz( value, token, sizeof( value ) );
	
	
	GetToken( qfalse );			// ,
	GetToken( qfalse );			// Y
	GetToken( qfalse );			// =
	
	GetToken( qfalse );
	Q_strcat( value, sizeof( value ), " " );
	Q_strcat( value, sizeof( value ), token );
	
	GetToken( qfalse );			// ,
	GetToken( qfalse );			// Z
	GetToken( qfalse );			// =
	
	GetToken( qfalse );
	Q_strcat( value, sizeof( value ), " " );
	Q_strcat( value, sizeof( value ), token );
	
	GetToken( qfalse );			// )
	
	//Sys_Printf("location: %f %f %f\n", mapEnt->origin[0], mapEnt->origin[1], mapEnt->origin[2]);
	SetKeyValue( mapEnt, "origin", value );
	
	return qtrue;
}



static qboolean ParseObjectClass()
{
	Sys_Printf( "ParseObjectClass()\n" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseObjectClass: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseObjectClass: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Object" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseActorClassBrush: token '%s'\n", token);
	}
	while( 1 );
	
	return qtrue;
}

static qboolean ParseActorClassWorldInfo()
{
	Sys_Printf( "ParseActorClassWorldInfo()\n" );
	
	if( ( numEntities - 1 ) != 0 )
	{
		Error( "numEntities != 0 with Class=WorldInfo" );
	}
	
	SetKeyValue( mapEnt, "classname", "worldspawn" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseActorClassWorldInfo: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseActorClassWorldInfo: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Actor" ) )
			{
				break;
			}
		}
		
		Sys_Printf( "ParseActorClassWorldInfo: token '%s'\n", token );
		
		if( !strcmp( token, "Begin" ) )
		{
			if( !GetToken( qfalse ) )
			{
				break;
			}
			
			if( !strcmp( token, "Object" ) )
			{
				if( !ParseObjectClass() )
				{
					return qfalse;
				}
			}
		}
		else
		{
			ParseKeyValue( token );
		}
	}
	while( 1 );
	
	return qtrue;
}


static qboolean ParsePolygon()
{
	int             i;
	side_t*         side;
	vec3_t          origin;
	vec3_t          normal;
	float           dist;
	vec3_t          planepts[4];
	int             planeptsNum;
	
	Sys_Printf( "ParsePolygon()\n" );
	
	side = &buildBrush->sides[buildBrush->numsides];
	memset( side, 0, sizeof( *side ) );
	buildBrush->numsides++;
	
	planeptsNum = 0;
	
	/*
	   Begin Polygon Flags=3584 Link=2
	   Origin   +02048.000000,+02048.000000,-01012.000000
	   Normal   +00001.000000,+00000.000000,+00000.000000
	   TextureU +00000.000000,-00001.000000,+00000.000000
	   TextureV +00000.000000,+00000.000000,-00001.000000
	   Vertex   +06272.000000,+06144.000000,-02960.000000
	   Vertex   +06272.000000,+06144.000000,+03184.000000
	   Vertex   +06272.000000,-06144.000000,+03184.000000
	   Vertex   +06272.000000,-06144.000000,-02960.000000
	   End Polygon
	 */
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParsePolygon: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParsePolygon: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Polygon" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParsePolygon: token '%s'\n", token);
		
		if( !strcmp( token, "Origin" ) )
		{
			// Origin   +02048.000000,+02048.000000,-01012.000000
			
			GetToken( qfalse );
			origin[0] = atof( token );
			
			GetToken( qfalse );	// ,
			
			GetToken( qfalse );
			origin[1] = atof( token );
			
			GetToken( qfalse );	// ,
			
			GetToken( qfalse );
			origin[2] = atof( token );
			
			//Sys_Printf("origin: %f %f %f\n", origin[0], origin[1], origin[2]);
		}
		else if( !strcmp( token, "Normal" ) )
		{
			GetToken( qfalse );
			normal[0] = atof( token );
			
			GetToken( qfalse );	// ,
			
			GetToken( qfalse );
			normal[1] = atof( token );
			
			GetToken( qfalse );	// ,
			
			GetToken( qfalse );
			normal[2] = atof( token );
			
			//Sys_Printf("normal: %f %f %f\n", normal[0], normal[1], normal[2]);
		}
		else if( !strcmp( token, "Vertex" ) )
		{
			if( planeptsNum < 4 )
			{
				GetToken( qfalse );
				planepts[planeptsNum][0] = atof( token );
				
				GetToken( qfalse );	// ,
				
				GetToken( qfalse );
				planepts[planeptsNum][1] = atof( token );
				
				GetToken( qfalse );	// ,
				
				GetToken( qfalse );
				planepts[planeptsNum][2] = atof( token );
				
				//Sys_Printf("vertex: %f %f %f\n", planepts[planeptsNum][0], planepts[planeptsNum][1], planepts[planeptsNum][2]);
				
				planeptsNum++;
			}
			else
			{
				SkipRestOfLine();
			}
		}
	}
	while( 1 );
	
#if 0
	VectorNormalize( normal );
	dist = DotProduct( origin, normal );
	side->planenum = FindFloatPlane( normal, dist );
#else
	//side->planenum = MapPlaneFromPoints(planepts[0], planepts[1], planepts[2]);
	side->planenum = MapPlaneFromPoints( planepts[2], planepts[1], planepts[0] );
#endif
	
	// FIXME calculate texMat later
	side->texMat[0][0] = 1;
	side->texMat[0][1] = 0;
	side->texMat[0][2] = 0;
	side->texMat[1][0] = 0;
	side->texMat[1][1] = 1;
	side->texMat[1][2] = 0;
	
	
	return qtrue;
}

static qboolean ParsePolyList()
{
	Sys_Printf( "ParsePolyList()\n" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParsePolyList: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParsePolyList: EOF without closing brace" );
			}
			
			if( !strcmp( token, "PolyList" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParsePolyList: token '%s'\n", token);
		
		if( !strcmp( token, "Begin" ) )
		{
			if( !GetToken( qfalse ) )
			{
				break;
			}
			
			if( !strcmp( token, "Polygon" ) )
			{
				if( !ParsePolygon() )
				{
					return qfalse;
				}
			}
		}
	}
	while( 1 );
	
	return qtrue;
}

static qboolean ParseBrush()
{
	bspBrush_t*     b;
	
	Sys_Printf( "ParseBrush()\n" );
	
	buildBrush->numsides = 0;
	buildBrush->detail = qfalse;
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseBrush: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseBrush: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Brush" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseBrush: token '%s'\n", token);
		
		if( !strcmp( token, "Begin" ) )
		{
			if( !GetToken( qfalse ) )
			{
				break;
			}
			
			if( !strcmp( token, "PolyList" ) )
			{
				if( !ParsePolyList() )
				{
					return qfalse;
				}
			}
		}
	}
	while( 1 );
	
	buildBrush->portalareas[0] = -1;
	buildBrush->portalareas[1] = -1;
	buildBrush->entitynum = numEntities - 1;
	buildBrush->brushnum = entitySourceBrushes;
	
	// if there are mirrored planes, the entire brush is invalid
	if( !RemoveDuplicateBrushPlanes( buildBrush ) )
	{
		return qtrue;
	}
	
	// get the content for the entire brush
	SetBrushContents( buildBrush );
	
	// allow detail brushes to be removed
	if( nodetail && ( buildBrush->contents & CONTENTS_DETAIL ) )
	{
		FreeBrush( buildBrush );
		return qtrue;
	}
	
	// allow water brushes to be removed
	if( noliquids && ( buildBrush->contents & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) )
	{
		FreeBrush( buildBrush );
		return qtrue;
	}
	
	b = FinishBrush();
	if( !b )
	{
		return qtrue;
	}
	
	return qtrue;
}

static qboolean ParseActorClassBrush()
{
	Sys_Printf( "ParseActorClassBrush()\n" );
	
	SetKeyValue( mapEnt, "classname", "func_static" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseActorClassBrush: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseActorClassBrush: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Actor" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseActorClassBrush: token '%s'\n", token);
		
		if( !strcmp( token, "Begin" ) )
		{
			if( !GetToken( qfalse ) )
			{
				break;
			}
			
			if( !strcmp( token, "Object" ) )
			{
				if( !ParseObjectClass() )
				{
					return qfalse;
				}
			}
			else if( !strcmp( token, "Brush" ) )
			{
				if( !ParseBrush() )
				{
					return qfalse;
				}
			}
		}
		else if( !Q_stricmp( token, "Name" ) )
		{
			GetToken( qfalse );
			if( strcmp( token, "=" ) )
			{
				Error( "ParseActorClassBrush: = not found, found %s", token );
			}
			
			GetToken( qfalse );
			//Sys_Printf("ParseActorClassBrush: Name=%s\n", token);
			SetKeyValue( mapEnt, "name", token );
			SetKeyValue( mapEnt, "model", token );
		}
		else if( !strcmp( token, "Location" ) )
		{
			ParseLocation( token );
		}
	}
	while( 1 );
	
	return qtrue;
}

static qboolean ParseActorClassPointLight()
{
	Sys_Printf( "ParseActorClassPointLight()\n" );
	
	SetKeyValue( mapEnt, "classname", "light" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseActorClassPointLight: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseActorClassPointLight: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Actor" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseActorClassPointLight: token '%s'\n", token);
		
		if( !Q_stricmp( token, "SphereRadius" ) )
		{
			float           radius;
			
			GetToken( qfalse );
			if( strcmp( token, "=" ) )
			{
				Error( "ParseActorClassPointLight: = not found, found %s", token );
			}
			
			GetToken( qfalse );
			radius = atof( token );
			Sys_Printf( "ParseActorClassPointLight: SphereRadius=%f\n", radius );
			
			SetKeyValue( mapEnt, "light_radius", va( "%f %f %f", radius / 2.0, radius / 2.0, radius / 2.0 ) );
		}
		else if( !strcmp( token, "Location" ) )
		{
			ParseLocation( token );
		}
	}
	while( 1 );
	
	return qtrue;
}

static qboolean ParseActorClassPlayerStart()
{
	Sys_Printf( "ParseActorClassPlayerStart()\n" );
	
	SetKeyValue( mapEnt, "classname", "info_player_deathmatch" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseActorClassPlayerStart: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseActorClassPlayerStart: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Actor" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseActorClassPlayerStart: token '%s'\n", token);
		
		if( !strcmp( token, "Location" ) )
		{
			ParseLocation( token );
		}
	}
	while( 1 );
	
	return qtrue;
}

/*
================
ParseActor
================
*/
qboolean ParseActor( void )
{
	int             i;
	entity_t*       otherEnt;
	epair_t*        e;
	const char*     classname;
	const char*     name;
	const char*     name2;
	const char*     model;
	char            lastToken[MAXTOKEN];
	
	if( numEntities == MAX_MAP_ENTITIES )
	{
		Error( "numEntities == MAX_MAP_ENTITIES" );
	}
	
	entitySourceBrushes = 0;
	
	mapEnt = &entities[numEntities];
	numEntities++;
	memset( mapEnt, 0, sizeof( *mapEnt ) );
	
	SetKeyValue( mapEnt, "classname", "unknown" );
	
	lastToken[0] = '\0';
	strncpy( lastToken, "Actor", strlen( "Actor" ) );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseActor: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseActor: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Actor" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseActor: token '%s'\n", token);
		
		if( !strcmp( token, "Class" ) )	// && !strcmp(lastToken, "Actor"))
		{
			GetToken( qfalse );
			if( strcmp( token, "=" ) )
			{
				Error( "ParseActor: = not found, found %s", token );
			}
			
			GetToken( qfalse );
			if( !Q_stricmp( token, "WorldInfo" ) )
			{
				if( !ParseActorClassWorldInfo() )
				{
					return qfalse;
				}
				break;
			}
			else if( !Q_stricmp( token, "Brush" ) )
			{
				if( !ParseActorClassBrush() )
				{
					return qfalse;
				}
				break;
			}
			else if( !Q_stricmp( token, "PointLight" ) )
			{
				if( !ParseActorClassPointLight() )
				{
					return qfalse;
				}
				break;
			}
			else if( !Q_stricmp( token, "PlayerStart" ) )
			{
				if( !ParseActorClassPlayerStart() )
				{
					return qfalse;
				}
				break;
			}
			else
			{
				//Sys_Printf("ParseActor: Class '%s'\n", token);
			}
		}
		else if( !strcmp( token, "Location" ) )
		{
			ParseLocation( token );
		}
		else if( !strcmp( token, "WeaponPickupClass" ) )
		{
			ParseKeyValue( token );
		}
		
		
		strncpy( lastToken, token, sizeof( token ) );
		
		//entitySourceBrushes++;
		
		/*
		   else
		   {
		   // parse a key / value pair
		   e = ParseEpair();
		   e->next = mapEnt->epairs;
		   mapEnt->epairs = e;
		   }
		 */
	}
	while( 1 );
	
	classname = ValueForKey( mapEnt, "classname" );
	name = ValueForKey( mapEnt, "name" );
	model = ValueForKey( mapEnt, "model" );
	GetVectorForKey( mapEnt, "origin", mapEnt->origin );
	
	// Tr3B: check for empty name
	if( !name[0] && numEntities != 1 )
	{
		name = UniqueEntityName( mapEnt, classname );
		if( !name[0] )
		{
			xml_Select( "UniqueEntityName failed", numEntities - 1, 0, qtrue );
		}
		
		SetKeyValue( mapEnt, "name", name );
		name = ValueForKey( mapEnt, "name" );
	}
	
	// Tr3B: check for bad duplicated names
	for( i = 0; i < numEntities; i++ )
	{
		otherEnt = &entities[i];
		
		if( mapEnt == otherEnt )
		{
			continue;
		}
		
		name2 = ValueForKey( otherEnt, "name" );
		
		if( !Q_stricmp( name, name2 ) )
		{
			xml_Select( "duplicated entity name", numEntities - 1, 0, qfalse );
			
			name = UniqueEntityName( mapEnt, classname );
			if( !name[0] )
			{
				xml_Select( "UniqueEntityName failed", numEntities - 1, 0, qtrue );
			}
			
			SetKeyValue( mapEnt, "name", name );
			name = ValueForKey( mapEnt, "name" );
			break;
		}
	}
	
	
#if 0
	// HACK: check if "model" key has no value
	if( HasKey( mapEnt, "model" ) && !model[0] )
	{
		Sys_FPrintf( SYS_WRN, "WARNING: entity '%s' has empty model key\n", name );
		SetKeyValue( mapEnt, "model", name );
	}
#endif
	
#if 0
	// HACK: convert Doom3's func_static entities with custom models into misc_models
	if( convertType == CONVERT_NOTHING && !Q_stricmp( "func_static", classname ) && !mapEnt->brushes && !mapEnt->patches &&
			model[0] != '\0' )
	{
		SetKeyValue( mapEnt, "classname", "misc_model" );
	}
#endif
	
#if 0
	// TODO: we should support Doom3 style doors in engine code completely
	if( nodoors && !Q_stricmp( "func_door", classname ) && !mapEnt->brushes && !mapEnt->patches && model[0] != '\0' )
	{
		numEntities--;
		return qtrue;
	}
#endif
	
#if 0
	// HACK:
	if( !Q_stricmp( "func_rotating", classname ) && !mapEnt->brushes && !mapEnt->patches && model[0] != '\0' )
	{
		numEntities--;
		return qtrue;
	}
#endif
	
	// HACK: determine if this is a func_static that can be merged into worldspawn
	if( convertType == CONVERT_NOTHING && !Q_stricmp( "func_static", classname ) && name[0] != '\0' && model[0] != '\0' &&
			!Q_stricmp( name, model ) )
	{
		bspBrush_t*     brush;
		vec3_t          originNeg;
		
		VectorNegate( mapEnt->origin, originNeg );
		AdjustBrushesForOrigin( mapEnt, originNeg );
		
		// NOTE: func_static entities should contain always detail brushes
		for( brush = mapEnt->brushes; brush != NULL; brush = brush->next )
		{
			if( !( brush->contents & CONTENTS_DETAIL ) || !brush->detail )
			{
				//c_detail++;
				//c_structural--;
				brush->detail = qtrue;
			}
		}
		
		if( !strcmp( "1", ValueForKey( mapEnt, "noclipmodel" ) ) || !strcmp( "0", ValueForKey( mapEnt, "solid" ) ) )
		{
			for( brush = mapEnt->brushes; brush != NULL; brush = brush->next )
			{
				brush->contents &= ~CONTENTS_SOLID;
			}
		}
		
		MoveBrushesToWorld( mapEnt );
		MovePatchesToWorld( mapEnt );
		
		//c_mergedFuncStatics++;
		numEntities--;
		return qtrue;
	}
	
	// if there was an origin brush, offset all of the planes and texinfo
	// for all the brushes in the entity
	if( convertType == CONVERT_NOTHING )
	{
		if( mapEnt->origin[0] || mapEnt->origin[1] || mapEnt->origin[2] )
		{
			if( ( name[0] != '\0' && model[0] != '\0' && !Q_stricmp( name, model ) ) )	// || !Q_stricmp("worldspawn", classname))
			{
				AdjustBrushesForOrigin( mapEnt, vec3_origin );
				AdjustPatchesForOrigin( mapEnt );
			}
			else
			{
				AdjustBrushesForOrigin( mapEnt, mapEnt->origin );
				AdjustPatchesForOrigin( mapEnt );
			}
		}
	}
	
	// group_info entities are just for editor grouping
	// ignored
	// FIXME: leak!
	if( !strcmp( "group_info", classname ) )
	{
		numEntities--;
		return qtrue;
	}
	
	// group entities are just for editor convenience
	// toss all brushes into the world entity
	if( convertType == CONVERT_NOTHING && !strcmp( "func_group", classname ) )
	{
		vec3_t          originNeg;
		
		// HACK: this is needed for Quake4 maps
		VectorNegate( mapEnt->origin, originNeg );
		AdjustBrushesForOrigin( mapEnt, originNeg );
		
		if( !strcmp( "1", ValueForKey( mapEnt, "terrain" ) ) )
		{
			SetTerrainTextures();
		}
		MoveBrushesToWorld( mapEnt );
		MovePatchesToWorld( mapEnt );
		numEntities--;
		return qtrue;
	}
	
	return qtrue;
}

qboolean ParseLevel()
{
	Sys_Printf( "ParseLevel()\n" );
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseLevel: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseLevel: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Level" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseLevel: token '%s'\n", token);
		
		if( !strcmp( token, "Begin" ) )
		{
			if( !GetToken( qtrue ) )
			{
				break;
			}
			
			if( !strcmp( token, "Actor" ) )
			{
				if( !ParseActor() )
				{
					return qfalse;
				}
			}
		}
		else if( !Q_stricmp( token, "Name" ) )
		{
			GetToken( qfalse );
			if( strcmp( token, "=" ) )
			{
				Error( "LoadT3DFile: = not found, found %s", token );
			}
			
			GetToken( qfalse );
			Sys_Printf( "ParseLevel: NAME=%s\n", token );
		}
	}
	while( 1 );
}

/*
================
LoadT3dFile
================
*/
void LoadT3DFile( char* filename )
{
	bspBrush_t*     b;
	
	Sys_FPrintf( SYS_VRB, "--- LoadT3DFile ---\n" );
	Sys_Printf( "Loading t3d file %s\n", filename );
	
	LoadScriptFile( filename, -1 );
	
	numEntities = 0;
	numMapDrawSurfs = 0;
//  c_detail = 0;
//  c_mergedFuncStatics = 0;

	g_bBrushPrimit = BPRIMIT_UNDEFINED;
	
	// allocate a very large temporary brush for building
	// the brushes as they are loaded
	buildBrush = AllocBrush( MAX_BUILD_SIDES );
	
	// parse Begin Map
	GetToken( qfalse );
	if( strcmp( token, "Begin" ) )
	{
		Error( "LoadT3DFile: Begin not found, found %s", token );
	}
	
	GetToken( qfalse );
	if( strcmp( token, "Map" ) )
	{
		Error( "LoadT3DFile: Map not found, found %s", token );
	}
	
	do
	{
		if( !GetToken( qtrue ) )
		{
			Error( "ParseActor: EOF without closing brace" );
		}
		
		if( !strcmp( token, "End" ) )
		{
			if( !GetToken( qfalse ) )
			{
				Error( "ParseActor: EOF without closing brace" );
			}
			
			if( !strcmp( token, "Map" ) )
			{
				break;
			}
		}
		
		//Sys_Printf("ParseActor: token '%s'\n", token);
		
		if( !strcmp( token, "Begin" ) )
		{
			if( !GetToken( qtrue ) )
			{
				break;
			}
			
			if( !strcmp( token, "Level" ) )
			{
				if( !ParseLevel() )
				{
					break;
				}
			}
		}
		
	}
	while( 1 );
	
	ClearBounds( mapMins, mapMaxs );
	for( b = entities[0].brushes; b; b = b->next )
	{
		AddPointToBounds( b->mins, mapMins, mapMaxs );
		AddPointToBounds( b->maxs, mapMins, mapMaxs );
	}
	
	Sys_FPrintf( SYS_VRB, "%5i total world brushes\n", CountBrushList( entities[0].brushes ) );
//  Sys_FPrintf(SYS_VRB, "%5i detail brushes\n", c_detail);
//  Sys_FPrintf(SYS_VRB, "%5i patches\n", numMapPatches);
//  Sys_FPrintf(SYS_VRB, "%5i boxbevels\n", c_boxbevels);
//  Sys_FPrintf(SYS_VRB, "%5i edgebevels\n", c_edgebevels);
//  Sys_FPrintf(SYS_VRB, "%5i merged func_static entities\n", c_mergedFuncStatics);
	Sys_FPrintf( SYS_VRB, "%5i entities\n", numEntities );
//  Sys_FPrintf(SYS_VRB, "%5i planes\n", numMapPlanes);
//  Sys_FPrintf(SYS_VRB, "%5i areaportals\n", c_areaportals);
	Sys_FPrintf( SYS_VRB, "size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", mapMins[0], mapMins[1], mapMins[2],
				 mapMaxs[0], mapMaxs[1], mapMaxs[2] );
}


int ConvertT3DToMap( int argc, char** argv )
{
	int             i;
	double          start, end;
	char            source[1024];
	char            name[1024];
	char            save[1024];
	
	Sys_Printf( "---- convert t3d to map ----\n" );
	
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
		Error( "usage: xmap -t3d2map [-<switch> [-<switch> ...]] <mapname.map>\n"
			   "\n" "Switches:\n" "   v              = verbose output\n" );
		//"   quake1       = convert from QuakeWorld to XreaL\n"
		//"   quake2       = convert from Quake2 to XreaL\n"
		//"   quake3         = convert from Quake3 to XreaL\n"
		//"   quake4         = convert from Quake4 to XreaL\n");
	}
	
	start = I_FloatTime();
	
	ThreadSetDefault();
	
	SetQdirFromPath( argv[i] );
	
	strcpy( source, ExpandArg( argv[i] ) );
	StripExtension( source );
	
	strcpy( name, ExpandArg( argv[i] ) );
	DefaultExtension( name, ".t3d" );
	
	// start from scratch
	LoadShaderInfo();
	
	LoadT3DFile( name );
	
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
