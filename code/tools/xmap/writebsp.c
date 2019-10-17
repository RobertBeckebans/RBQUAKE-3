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
#include "qbsp.h"

/*
============
EmitShader
============
*/
int EmitShader( const char* shader )
{
	int             i;
	shaderInfo_t*   si;
	
	if( !shader )
	{
		shader = "noshader";
	}
	
	for( i = 0; i < numShaders; i++ )
	{
		if( !Q_stricmp( shader, dshaders[i].shader ) )
		{
			return i;
		}
	}
	
	if( i == MAX_MAP_SHADERS )
	{
		Error( "MAX_MAP_SHADERS" );
	}
	numShaders++;
	strcpy( dshaders[i].shader, shader );
	
	si = ShaderInfoForShader( shader );
	dshaders[i].surfaceFlags = si->surfaceFlags;
	dshaders[i].contentFlags = si->contents;
	
	return i;
}


/*
============
EmitPlanes

There is no oportunity to discard planes, because all of the original
brushes will be saved in the map.
============
*/
void EmitPlanes( void )
{
	int             i;
	dplane_t*       dp;
	plane_t*        mp;
	
	mp = mapPlanes;
	for( i = 0; i < numMapPlanes; i++, mp++ )
	{
		dp = &dplanes[numPlanes];
		VectorCopy( mp->normal, dp->normal );
		dp->dist = mp->dist;
		numPlanes++;
	}
}



/*
==================
EmitLeaf
==================
*/
void EmitLeaf( node_t* node )
{
	dleaf_t*        leaf_p;
	bspBrush_t*     b;
	drawSurfaceRef_t* dsr;
	
	// emit a leaf
	if( numLeafs >= MAX_MAP_LEAFS )
	{
		Error( "MAX_MAP_LEAFS" );
	}
	
	leaf_p = &dleafs[numLeafs];
	numLeafs++;
	
	leaf_p->cluster = node->cluster;
	leaf_p->area = node->area;
	
	//
	// write bounding box info
	//
	VectorCopy( node->mins, leaf_p->mins );
	VectorCopy( node->maxs, leaf_p->maxs );
	
	//
	// write the leafbrushes
	//
	leaf_p->firstLeafBrush = numLeafBrushes;
	for( b = node->brushlist; b; b = b->next )
	{
		if( numLeafBrushes >= MAX_MAP_LEAFBRUSHES )
		{
			Error( "MAX_MAP_LEAFBRUSHES" );
		}
		dleafbrushes[numLeafBrushes] = b->original->outputNumber;
		numLeafBrushes++;
	}
	leaf_p->numLeafBrushes = numLeafBrushes - leaf_p->firstLeafBrush;
	
	//
	// write the surfaces visible in this leaf
	//
	if( node->opaque )
	{
		return;					// no leaffaces in solids
	}
	
	// add the drawSurfaceRef_t drawsurfs
	leaf_p->firstLeafSurface = numLeafSurfaces;
	for( dsr = node->drawSurfReferences; dsr; dsr = dsr->nextRef )
	{
		if( numLeafSurfaces >= MAX_MAP_LEAFFACES )
		{
			Error( "MAX_MAP_LEAFFACES" );
		}
		dleafsurfaces[numLeafSurfaces] = dsr->outputNumber;
		numLeafSurfaces++;
	}
	
	
	leaf_p->numLeafSurfaces = numLeafSurfaces - leaf_p->firstLeafSurface;
}


/*
============
EmitDrawNode_r
============
*/
int EmitDrawNode_r( node_t* node )
{
	dnode_t*        n;
	int             i;
	
	if( node->planenum == PLANENUM_LEAF )
	{
		EmitLeaf( node );
		return -numLeafs;
	}
	
	// emit a node
	if( numNodes == MAX_MAP_NODES )
	{
		Error( "MAX_MAP_NODES" );
	}
	n = &dnodes[numNodes];
	numNodes++;
	
	VectorCopy( node->mins, n->mins );
	VectorCopy( node->maxs, n->maxs );
	
	if( node->planenum & 1 )
	{
		Error( "WriteDrawNodes_r: odd planenum" );
	}
	n->planeNum = node->planenum;
	
	//
	// recursively output the other nodes
	//
	for( i = 0; i < 2; i++ )
	{
		if( node->children[i]->planenum == PLANENUM_LEAF )
		{
			n->children[i] = -( numLeafs + 1 );
			EmitLeaf( node->children[i] );
		}
		else
		{
			n->children[i] = numNodes;
			EmitDrawNode_r( node->children[i] );
		}
	}
	
	return n - dnodes;
}

//=========================================================



/*
============
SetModelNumbers
============
*/
void SetModelNumbers( void )
{
	int             i;
	entity_t*       ent;
	int             models;
	char            value[10];
	const char*     classname;
	const char*     model;
	
	models = 1;
	for( i = 1; i < numEntities; i++ )
	{
		ent = &entities[i];
		
		classname = ValueForKey( ent, "classname" );
		model = ValueForKey( ent, "model" );
		
		if( ent->brushes || ent->patches ||
				( !ent->brushes && !ent->patches && model[0] != '\0' && Q_stricmp( "misc_model", classname ) ) )
		{
			sprintf( value, "*%i", models );
			models++;
			SetKeyValue( &entities[i], "model", value );
		}
	}
	
}


//===========================================================

/*
==================
BeginBSPFile
==================
*/
void BeginBSPFile( void )
{
	// these values may actually be initialized
	// if the file existed when loaded, so clear them explicitly
	numModels = 0;
	numNodes = 0;
	numBrushSides = 0;
	numLeafSurfaces = 0;
	numLeafBrushes = 0;
	
	// leave leaf 0 as an error, because leafs are referenced as
	// negative number nodes
	numLeafs = 1;
}


/*
============
EndBSPFile
============
*/
void EndBSPFile( void )
{
	char            path[1024];
	
	EmitPlanes();
	UnparseEntities();
	
	// write the map
	sprintf( path, "%s.bsp", source );
	Sys_Printf( "Writing %s\n", path );
	WriteBSPFile( path );
}


//===========================================================

/*
============
EmitBrushes
============
*/
void EmitBrushes( bspBrush_t* brushes )
{
	int             j;
	dbrush_t*       db;
	bspBrush_t*     b;
	dbrushside_t*   cp;
	
	for( b = brushes; b; b = b->next )
	{
		if( numBrushes == MAX_MAP_BRUSHES )
		{
			Error( "MAX_MAP_BRUSHES" );
		}
		b->outputNumber = numBrushes;
		db = &dbrushes[numBrushes];
		numBrushes++;
		
		db->shaderNum = EmitShader( b->contentShader->shader );
		db->firstSide = numBrushSides;
		
		// don't emit any generated backSide sides
		db->numSides = 0;
		for( j = 0; j < b->numsides; j++ )
		{
			if( b->sides[j].backSide )
			{
				continue;
			}
			
			if( numBrushSides == MAX_MAP_BRUSHSIDES )
			{
				Error( "MAX_MAP_BRUSHSIDES " );
			}
			
			cp = &dbrushsides[numBrushSides];
			db->numSides++;
			numBrushSides++;
			cp->planeNum = b->sides[j].planenum;
			
			if( b->sides[j].shaderInfo )
			{
				cp->shaderNum = EmitShader( b->sides[j].shaderInfo->shader );
			}
			else
			{
				cp->shaderNum = EmitShader( NULL );
			}
		}
	}
	
}


/*
==================
BeginModel
==================
*/
void BeginModel( entity_t* e )
{
	dmodel_t*       mod;
	
	Sys_FPrintf( SYS_VRB, "--- BeginModel ---\n" );
	
	if( numModels == MAX_MAP_MODELS )
	{
		Error( "MAX_MAP_MODELS" );
	}
	mod = &dmodels[numModels];
	
	mod->firstSurface = numDrawSurfaces;
	mod->firstBrush = numBrushes;
	
	EmitBrushes( e->brushes );
}




/*
==================
EndModel
==================
*/
void EndModel( entity_t* e, node_t* headnode )
{
	dmodel_t*       mod;
	vec3_t          mins, maxs;
	bspBrush_t*     b;
	parseMesh_t*    p;
	drawSurface_t*  ds;
	const char*     model;
	int             i, j;
	
	Sys_FPrintf( SYS_VRB, "--- EndModel ---\n" );
	
	mod = &dmodels[numModels];
	
	// calculate the AABB
	ClearBounds( mins, maxs );
	for( b = e->brushes; b; b = b->next )
	{
		if( !b->numsides )
		{
			continue;			// not a real brush (origin brush, etc)
		}
		AddPointToBounds( b->mins, mins, maxs );
		AddPointToBounds( b->maxs, mins, maxs );
	}
	
	for( p = e->patches; p; p = p->next )
	{
		for( i = 0; i < p->mesh.width * p->mesh.height; i++ )
		{
			AddPointToBounds( p->mesh.verts[i].xyz, mins, maxs );
		}
	}
	
	model = ValueForKey( e, "model" );
	if( !e->brushes && !e->patches && model[0] != '\0' )
	{
		//Sys_FPrintf(SYS_VRB, "calculating bbox from draw surfaces...\n");
		
		qboolean noVerts = qtrue;
		for( i = e->firstDrawSurf; i < numMapDrawSurfs; i++ )
		{
			ds = &mapDrawSurfs[i];
			
			if( !ds->numVerts )
			{
				continue;		// leftover from a surface subdivision
			}
			
			// HACK: don't loop only through the vertices because they can contain bad data with .lwo models ...
			for( j = 0; j < ds->numIndexes; j++ )
			{
				AddPointToBounds( ds->verts[ds->indexes[j]].xyz, mins, maxs );
				noVerts = qfalse;
			}
		}
		
		if( noVerts )
		{
			// Tr3B: model could not be loaded or something else went wrong.
			// reset to zero bounds so the entity doesn't mess up the server collision code
			VectorClear( mins );
			VectorClear( maxs );
		}
	}
	
	VectorCopy( mins, mod->mins );
	VectorCopy( maxs, mod->maxs );
	
	EmitDrawNode_r( headnode );
	mod->numSurfaces = numDrawSurfaces - mod->firstSurface;
	mod->numBrushes = numBrushes - mod->firstBrush;
	
	numModels++;
}
