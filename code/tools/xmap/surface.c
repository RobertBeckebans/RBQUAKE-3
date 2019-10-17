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


drawSurface_t   mapDrawSurfs[MAX_MAP_DRAW_SURFS];
int             numMapDrawSurfs;

/*
=============================================================================

DRAWSURF CONSTRUCTION

=============================================================================
*/

/*
=================
AllocDrawSurf
=================
*/
drawSurface_t*  AllocDrawSurf( void )
{
	drawSurface_t*  ds;
	
	if( numMapDrawSurfs >= MAX_MAP_DRAW_SURFS )
	{
		Error( "MAX_MAP_DRAW_SURFS" );
	}
	ds = &mapDrawSurfs[numMapDrawSurfs];
	numMapDrawSurfs++;
	
	return ds;
}

/*
=================
DrawSurfaceForSide
=================
*/
#define	SNAP_FLOAT_TO_INT	8
#define	SNAP_INT_TO_FLOAT	(1.0/SNAP_FLOAT_TO_INT)

drawSurface_t*  DrawSurfaceForSide( bspBrush_t* b, side_t* s, winding_t* w )
{
	drawSurface_t*  ds;
	int             i, j;
	shaderInfo_t*   si;
	drawVert_t*     dv;
	float           mins[2], maxs[2];
	
	// brush primitive :
	// axis base
	vec3_t          texX, texY;
	vec_t           x, y;
	
	if( w->numpoints > 64 )
	{
		Error( "DrawSurfaceForSide: w->numpoints = %i", w->numpoints );
	}
	
	si = s->shaderInfo;
	
	ds = AllocDrawSurf();
	
	ds->shaderInfo = si;
	ds->mapBrush = b;
	ds->side = s;
	ds->fogNum = -1;
	ds->numVerts = w->numpoints;
	ds->verts = malloc( ds->numVerts * sizeof( *ds->verts ) );
	memset( ds->verts, 0, ds->numVerts * sizeof( *ds->verts ) );
	
	mins[0] = mins[1] = 99999;
	maxs[0] = maxs[1] = -99999;
	
	// compute s/t coordinates from brush primitive texture matrix
	// compute axis base
	ComputeAxisBase( mapPlanes[s->planenum].normal, texX, texY );
	
	for( j = 0; j < w->numpoints; j++ )
	{
		dv = ds->verts + j;
		
		// round the xyz to a given precision
		for( i = 0; i < 3; i++ )
		{
			dv->xyz[i] = SNAP_INT_TO_FLOAT * floor( w->p[j][i] * SNAP_FLOAT_TO_INT + 0.5 );
		}
		
		if( g_bBrushPrimit == BPRIMIT_OLDBRUSHES )
		{
			// calculate texture s/t
			dv->st[0] = s->vecs[0][3] + DotProduct( s->vecs[0], dv->xyz );
			dv->st[1] = s->vecs[1][3] + DotProduct( s->vecs[1], dv->xyz );
			dv->st[0] /= si->width;
			dv->st[1] /= si->height;
		}
		else
		{
			// calculate texture s/t from brush primitive texture matrix
			x = DotProduct( dv->xyz, texX );
			y = DotProduct( dv->xyz, texY );
			dv->st[0] = s->texMat[0][0] * x + s->texMat[0][1] * y + s->texMat[0][2];
			dv->st[1] = s->texMat[1][0] * x + s->texMat[1][1] * y + s->texMat[1][2];
		}
		
		for( i = 0; i < 2; i++ )
		{
			if( dv->st[i] < mins[i] )
			{
				mins[i] = dv->st[i];
			}
			if( dv->st[i] > maxs[i] )
			{
				maxs[i] = dv->st[i];
			}
		}
		
		// copy normal
		VectorCopy( mapPlanes[s->planenum].normal, dv->normal );
	}
	
	// adjust the texture coordinates to be as close to 0 as possible
	if( !si->globalTexture )
	{
		mins[0] = floor( mins[0] );
		mins[1] = floor( mins[1] );
		for( i = 0; i < w->numpoints; i++ )
		{
			dv = ds->verts + i;
			dv->st[0] -= mins[0];
			dv->st[1] -= mins[1];
		}
	}
	
	return ds;
}


//=========================================================================




typedef struct
{
	int             planenum;
	shaderInfo_t*   shaderInfo;
	int             count;
} sideRef_t;

#define	MAX_SIDE_REFS	MAX_MAP_PLANES

sideRef_t       sideRefs[MAX_SIDE_REFS];
int             numSideRefs;

void AddSideRef( side_t* side )
{
	int             i;
	
	for( i = 0; i < numSideRefs; i++ )
	{
		if( side->planenum == sideRefs[i].planenum && side->shaderInfo == sideRefs[i].shaderInfo )
		{
			sideRefs[i].count++;
			return;
		}
	}
	
	if( numSideRefs == MAX_SIDE_REFS )
	{
		Error( "MAX_SIDE_REFS" );
	}
	
	sideRefs[i].planenum = side->planenum;
	sideRefs[i].shaderInfo = side->shaderInfo;
	sideRefs[i].count++;
	numSideRefs++;
}


/*
=====================
MergeSides

=====================
*/
void MergeSides( entity_t* e, tree_t* tree )
{
	int             i;
	
	Sys_FPrintf( SYS_VRB, "----- MergeSides -----\n" );
	
	for( i = e->firstDrawSurf; i < numMapDrawSurfs; i++ )
	{
//          AddSideRef( side );
	}
	
	Sys_FPrintf( SYS_VRB, "%5i siderefs\n", numSideRefs );
}

//=====================================================================

/*
===================
SubdivideDrawSurf
===================
*/
void SubdivideDrawSurf( drawSurface_t* ds, winding_t* w, float subdivisions )
{
	int             i;
	int             axis;
	vec3_t          bounds[2];
	const float     epsilon = 0.1;
	int             subFloor, subCeil;
	winding_t*      frontWinding, *backWinding;
	drawSurface_t*  newds;
	
	if( !w )
	{
		return;
	}
	if( w->numpoints < 3 )
	{
		Error( "SubdivideDrawSurf: Bad w->numpoints" );
	}
	
	ClearBounds( bounds[0], bounds[1] );
	for( i = 0; i < w->numpoints; i++ )
	{
		AddPointToBounds( w->p[i], bounds[0], bounds[1] );
	}
	
	for( axis = 0; axis < 3; axis++ )
	{
		vec3_t          planePoint = { 0, 0, 0 };
		vec3_t          planeNormal = { 0, 0, 0 };
		float           d;
		
		subFloor = floor( bounds[0][axis] / subdivisions ) * subdivisions;
		subCeil = ceil( bounds[1][axis] / subdivisions ) * subdivisions;
		
		planePoint[axis] = subFloor + subdivisions;
		planeNormal[axis] = -1;
		
		d = DotProduct( planePoint, planeNormal );
		
		// subdivide if necessary
		if( subCeil - subFloor > subdivisions )
		{
			// gotta clip polygon into two polygons
			ClipWindingEpsilon( w, planeNormal, d, epsilon, &frontWinding, &backWinding );
			
			// the clip may not produce two polygons if it was epsilon close
			if( !frontWinding )
			{
				w = backWinding;
			}
			else if( !backWinding )
			{
				w = frontWinding;
			}
			else
			{
				SubdivideDrawSurf( ds, frontWinding, subdivisions );
				SubdivideDrawSurf( ds, backWinding, subdivisions );
				
				return;
			}
		}
	}
	
	// emit this polygon
	newds = DrawSurfaceForSide( ds->mapBrush, ds->side, w );
	newds->fogNum = ds->fogNum;
}


/*
=====================
SubdivideDrawSurfs

Chop up surfaces that have subdivision attributes
=====================
*/
void SubdivideDrawSurfs( entity_t* e, tree_t* tree )
{
	int             i;
	drawSurface_t*  ds;
	int             numBaseDrawSurfs;
	winding_t*      w;
	float           subdivision;
	shaderInfo_t*   si;
	
	Sys_FPrintf( SYS_VRB, "----- SubdivideDrawSurfs -----\n" );
	numBaseDrawSurfs = numMapDrawSurfs;
	for( i = e->firstDrawSurf; i < numBaseDrawSurfs; i++ )
	{
		ds = &mapDrawSurfs[i];
		
		// only subdivide brush sides, not patches or misc_models
		if( !ds->side )
		{
			continue;
		}
		
		// check subdivision for shader
		si = ds->side->shaderInfo;
		if( !si )
		{
			continue;
		}
		
		if( ds->shaderInfo->autosprite || si->autosprite )
		{
			continue;
		}
		
		subdivision = si->subdivisions;
		if( !subdivision )
		{
			continue;
		}
		
		w = WindingFromDrawSurf( ds );
		ds->numVerts = 0;		// remove this reference
		SubdivideDrawSurf( ds, w, subdivision );
	}
	
}


//===================================================================================

/*
====================
ClipSideIntoTree_r

Adds non-opaque leaf fragments to the convex hull
====================
*/
void ClipSideIntoTree_r( winding_t* w, side_t* side, node_t* node )
{
	plane_t*        plane;
	winding_t*      front, *back;
	
	if( !w )
	{
		return;
	}
	
	if( node->planenum != PLANENUM_LEAF )
	{
		if( side->planenum == node->planenum )
		{
			ClipSideIntoTree_r( w, side, node->children[0] );
			return;
		}
		if( side->planenum == ( node->planenum ^ 1 ) )
		{
			ClipSideIntoTree_r( w, side, node->children[1] );
			return;
		}
		
		plane = &mapPlanes[node->planenum];
		ClipWindingEpsilon( w, plane->normal, plane->dist, ON_EPSILON, &front, &back );
		FreeWinding( w );
		
		ClipSideIntoTree_r( front, side, node->children[0] );
		ClipSideIntoTree_r( back, side, node->children[1] );
		
		return;
	}
	
	// if opaque leaf, don't add
	if( !node->opaque )
	{
		AddWindingToConvexHull( w, &side->visibleHull, mapPlanes[side->planenum].normal );
	}
	
	FreeWinding( w );
	return;
}


/*
=====================
ClipSidesIntoTree

Creates side->visibleHull for all visible sides

The drawsurf for a side will consist of the convex hull of
all points in non-opaque clusters, which allows overlaps
to be trimmed off automatically.
=====================
*/
void ClipSidesIntoTree( entity_t* e, tree_t* tree )
{
	bspBrush_t*     b;
	int             i;
	winding_t*      w;
	side_t*         side, *newSide;
	shaderInfo_t*   si;
	
	Sys_FPrintf( SYS_VRB, "----- ClipSidesIntoTree -----\n" );
	
	for( b = e->brushes; b; b = b->next )
	{
		for( i = 0; i < b->numsides; i++ )
		{
			side = &b->sides[i];
			if( !side->winding )
			{
				continue;
			}
			w = CopyWinding( side->winding );
			side->visibleHull = NULL;
			ClipSideIntoTree_r( w, side, tree->headnode );
			
			w = side->visibleHull;
			if( !w )
			{
				continue;
			}
			si = side->shaderInfo;
			if( !si )
			{
				continue;
			}
			// don't create faces for non-visible sides
			if( si->surfaceFlags & SURF_NODRAW )
			{
				continue;
			}
			
			// always use the original quad winding for auto sprites
			if( side->shaderInfo->autosprite )
			{
				w = side->winding;
			}
			//
			if( side->bevel )
			{
				Error( "monkey tried to create draw surface for brush bevel" );
			}
			// save this winding as a visible surface
			DrawSurfaceForSide( b, side, w );
			
			// make a back side for it if needed
			if( !( si->contents & CONTENTS_FOG ) )
			{
				continue;
			}
			
			// duplicate the up-facing side
			w = ReverseWinding( w );
			
			newSide = malloc( sizeof( *side ) );
			*newSide = *side;
			newSide->visibleHull = w;
			newSide->planenum ^= 1;
			
			// save this winding as a visible surface
			DrawSurfaceForSide( b, newSide, w );
			
		}
	}
}

/*
===================================================================================

  FILTER REFERENCES DOWN THE TREE

===================================================================================
*/

/*
====================
FilterDrawSurfIntoTree

Place a reference to the given drawsurf in every leaf it contacts
We assume that the point mesh aproximation to the curve will get a
reference into all the leafs we need.
====================
*/
int FilterMapDrawSurfIntoTree( vec3_t point, drawSurface_t* ds, node_t* node )
{
	drawSurfaceRef_t* dsr;
	float           d;
	plane_t*        plane;
	int             c;
	
	if( node->planenum != PLANENUM_LEAF )
	{
		plane = &mapPlanes[node->planenum];
		d = DotProduct( point, plane->normal ) - plane->dist;
		c = 0;
		if( d >= -ON_EPSILON )
		{
			c += FilterMapDrawSurfIntoTree( point, ds, node->children[0] );
		}
		if( d <= ON_EPSILON )
		{
			c += FilterMapDrawSurfIntoTree( point, ds, node->children[1] );
		}
		return c;
	}
	
	// if opaque leaf, don't add
	if( node->opaque )
	{
		return 0;
	}
	
	// add the drawsurf if it hasn't been already
	for( dsr = node->drawSurfReferences; dsr; dsr = dsr->nextRef )
	{
		if( dsr->outputNumber == numDrawSurfaces )
		{
			return 0;			// already referenced
		}
	}
	
	dsr = malloc( sizeof( *dsr ) );
	dsr->outputNumber = numDrawSurfaces;
	dsr->nextRef = node->drawSurfReferences;
	node->drawSurfReferences = dsr;
	return 1;
}

/*
====================
FilterDrawSurfIntoTree_r

Place a reference to the given drawsurf in every leaf it is in
====================
*/
int FilterMapDrawSurfIntoTree_r( winding_t* w, drawSurface_t* ds, node_t* node )
{
	drawSurfaceRef_t* dsr;
	plane_t*        plane;
	int             total;
	winding_t*      front, *back;
	
	if( node->planenum != PLANENUM_LEAF )
	{
		plane = &mapPlanes[node->planenum];
		ClipWindingEpsilon( w, plane->normal, plane->dist, ON_EPSILON, &front, &back );
		
		total = 0;
		if( front )
		{
			total += FilterMapDrawSurfIntoTree_r( front, ds, node->children[0] );
		}
		if( back )
		{
			total += FilterMapDrawSurfIntoTree_r( back, ds, node->children[1] );
		}
		
		FreeWinding( w );
		return total;
	}
	
	// if opaque leaf, don't add
	if( node->opaque )
	{
		return 0;
	}
	
	// add the drawsurf if it hasn't been already
	for( dsr = node->drawSurfReferences; dsr; dsr = dsr->nextRef )
	{
		if( dsr->outputNumber == numDrawSurfaces )
		{
			return 0;			// already referenced
		}
	}
	
	dsr = malloc( sizeof( *dsr ) );
	dsr->outputNumber = numDrawSurfaces;
	dsr->nextRef = node->drawSurfReferences;
	node->drawSurfReferences = dsr;
	return 1;
}

/*
====================
FilterSideIntoTree_r

Place a reference to the given drawsurf in every leaf it contacts
====================
*/
int FilterSideIntoTree_r( winding_t* w, side_t* side, drawSurface_t* ds, node_t* node )
{
	drawSurfaceRef_t* dsr;
	plane_t*        plane;
	winding_t*      front, *back;
	int             total;
	
	if( !w )
	{
		return 0;
	}
	
	if( node->planenum != PLANENUM_LEAF )
	{
		if( side->planenum == node->planenum )
		{
			return FilterSideIntoTree_r( w, side, ds, node->children[0] );
		}
		if( side->planenum == ( node->planenum ^ 1 ) )
		{
			return FilterSideIntoTree_r( w, side, ds, node->children[1] );
		}
		
		plane = &mapPlanes[node->planenum];
		ClipWindingEpsilon( w, plane->normal, plane->dist, ON_EPSILON, &front, &back );
		
		total = FilterSideIntoTree_r( front, side, ds, node->children[0] );
		total += FilterSideIntoTree_r( back, side, ds, node->children[1] );
		
		FreeWinding( w );
		return total;
	}
	
	// if opaque leaf, don't add
	if( node->opaque )
	{
		return 0;
	}
	
	dsr = malloc( sizeof( *dsr ) );
	dsr->outputNumber = numDrawSurfaces;
	dsr->nextRef = node->drawSurfReferences;
	node->drawSurfReferences = dsr;
	
	FreeWinding( w );
	return 1;
}


/*
=====================
FilterFaceIntoTree
=====================
*/
int FilterFaceIntoTree( drawSurface_t* ds, tree_t* tree )
{
	int             l;
	winding_t*      w;
	
	w = WindingFromDrawSurf( ds );
	l = FilterSideIntoTree_r( w, ds->side, ds, tree->headnode );
	
	return l;
}



/*
=====================
FilterPatchSurfIntoTree
=====================
*/
#define	SUBDIVISION_LIMIT		8.0
int FilterPatchSurfIntoTree( drawSurface_t* ds, tree_t* tree )
{
	int             i, j;
	int             l;
	mesh_t          baseMesh, *subdividedMesh;
	winding_t*      w;
	
	baseMesh.width = ds->patchWidth;
	baseMesh.height = ds->patchHeight;
	baseMesh.verts = ds->verts;
	subdividedMesh = SubdivideMesh( baseMesh, SUBDIVISION_LIMIT, 32 );
	
	l = 0;
	for( i = 0; i < subdividedMesh->width - 1; i++ )
	{
		for( j = 0; j < subdividedMesh->height - 1; j++ )
		{
			w = AllocWinding( 3 );
			VectorCopy( subdividedMesh->verts[j * subdividedMesh->width + i].xyz, w->p[0] );
			VectorCopy( subdividedMesh->verts[j * subdividedMesh->width + i + 1].xyz, w->p[1] );
			VectorCopy( subdividedMesh->verts[( j + 1 ) * subdividedMesh->width + i].xyz, w->p[2] );
			w->numpoints = 3;
			l += FilterMapDrawSurfIntoTree_r( w, ds, tree->headnode );
			w = AllocWinding( 3 );
			VectorCopy( subdividedMesh->verts[j * subdividedMesh->width + i + 1].xyz, w->p[0] );
			VectorCopy( subdividedMesh->verts[( j + 1 ) * subdividedMesh->width + i + 1].xyz, w->p[1] );
			VectorCopy( subdividedMesh->verts[( j + 1 ) * subdividedMesh->width + i].xyz, w->p[2] );
			w->numpoints = 3;
			l += FilterMapDrawSurfIntoTree_r( w, ds, tree->headnode );
		}
	}
	
	// also use the old point filtering into the tree
	for( i = 0; i < subdividedMesh->width * subdividedMesh->height; i++ )
	{
		l += FilterMapDrawSurfIntoTree( subdividedMesh->verts[i].xyz, ds, tree->headnode );
	}
	
	free( subdividedMesh );
	
	return l;
}


/*
=====================
FilterMiscModelSurfIntoTree
=====================
*/
int FilterMiscModelSurfIntoTree( drawSurface_t* ds, tree_t* tree )
{
	int             i;
	int             l;
	winding_t*      w;
	
	l = 0;
	for( i = 0; i < ds->numIndexes - 2; i++ )
	{
		w = AllocWinding( 3 );
		VectorCopy( ds->verts[ds->indexes[i]].xyz, w->p[0] );
		VectorCopy( ds->verts[ds->indexes[i + 1]].xyz, w->p[1] );
		VectorCopy( ds->verts[ds->indexes[i + 2]].xyz, w->p[2] );
		w->numpoints = 3;
		l += FilterMapDrawSurfIntoTree_r( w, ds, tree->headnode );
	}
	
	// also use the old point filtering into the tree
	for( i = 0; i < ds->numVerts; i++ )
	{
		l += FilterMapDrawSurfIntoTree( ds->verts[i].xyz, ds, tree->headnode );
	}
	
	return l;
}

/*
=====================
FilterFlareSurfIntoTree
=====================
*/
int FilterFlareSurfIntoTree( drawSurface_t* ds, tree_t* tree )
{
	return FilterMapDrawSurfIntoTree( ds->lightmapOrigin, ds, tree->headnode );
}


//======================================================================

int             c_stripSurfaces, c_fanSurfaces;

/*
==================
IsTriangleDegenerate

Returns qtrue if all three points are collinear or backwards
===================
*/
#define	COLINEAR_AREA	10
static qboolean IsTriangleDegenerate( drawVert_t* points, int a, int b, int c )
{
	vec3_t          v1, v2, v3;
	float           d;
	
	VectorSubtract( points[b].xyz, points[a].xyz, v1 );
	VectorSubtract( points[c].xyz, points[a].xyz, v2 );
	CrossProduct( v1, v2, v3 );
	d = VectorLength( v3 );
	
	// assume all very small or backwards triangles will cause problems
	if( d < COLINEAR_AREA )
	{
		return qtrue;
	}
	
	return qfalse;
}

/*
===============
SurfaceAsTriFan

The surface can't be represented as a single tristrip without
leaving a degenerate triangle (and therefore a crack), so add
a point in the middle and create (points-1) triangles in fan order
===============
*/
static void SurfaceAsTriFan( dsurface_t* ds )
{
	int             i;
	int             colorSum[4];
	drawVert_t*     mid, *v;
	
	// create a new point in the center of the face
	if( numDrawVerts == MAX_MAP_DRAW_VERTS )
	{
		Error( "MAX_MAP_DRAW_VERTS" );
	}
	mid = &drawVerts[numDrawVerts];
	numDrawVerts++;
	
	colorSum[0] = colorSum[1] = colorSum[2] = colorSum[3] = 0;
	
	v = drawVerts + ds->firstVert;
	for( i = 0; i < ds->numVerts; i++, v++ )
	{
		VectorAdd( mid->xyz, v->xyz, mid->xyz );
		mid->st[0] += v->st[0];
		mid->st[1] += v->st[1];
		mid->lightmap[0] += v->lightmap[0];
		mid->lightmap[1] += v->lightmap[1];
		
		colorSum[0] += v->lightColor[0];
		colorSum[1] += v->lightColor[1];
		colorSum[2] += v->lightColor[2];
		colorSum[3] += v->lightColor[3];
	}
	
	mid->xyz[0] /= ds->numVerts;
	mid->xyz[1] /= ds->numVerts;
	mid->xyz[2] /= ds->numVerts;
	
	mid->st[0] /= ds->numVerts;
	mid->st[1] /= ds->numVerts;
	
	mid->lightmap[0] /= ds->numVerts;
	mid->lightmap[1] /= ds->numVerts;
	
	mid->lightColor[0] = colorSum[0] / ds->numVerts;
	mid->lightColor[1] = colorSum[1] / ds->numVerts;
	mid->lightColor[2] = colorSum[2] / ds->numVerts;
	mid->lightColor[3] = colorSum[3] / ds->numVerts;
	
	VectorCopy( ( drawVerts + ds->firstVert )->normal, mid->normal );
	
	// fill in indices in trifan order
	if( numDrawIndexes + ds->numVerts * 3 > MAX_MAP_DRAW_INDEXES )
	{
		Error( "MAX_MAP_DRAWINDEXES" );
	}
	ds->firstIndex = numDrawIndexes;
	ds->numIndexes = ds->numVerts * 3;
	
	//FIXME
	// should be: for ( i = 0 ; i < ds->numVerts ; i++ ) {
	// set a break point and test this in a map
	//for ( i = 0 ; i < ds->numVerts*3 ; i++ ) {
	for( i = 0; i < ds->numVerts; i++ )
	{
		drawIndexes[numDrawIndexes++] = ds->numVerts;
		drawIndexes[numDrawIndexes++] = i;
		drawIndexes[numDrawIndexes++] = ( i + 1 ) % ds->numVerts;
	}
	
	ds->numVerts++;
}


/*
================
SurfaceAsTristrip

Try to create indices that make (points-2) triangles in tristrip order
================
*/
#define	MAX_INDICES	1024
static void SurfaceAsTristrip( dsurface_t* ds )
{
	int             i;
	int             rotate;
	int             numIndices;
	int             ni = 0;
	int             a, b, c;
	int             indices[MAX_INDICES];
	
	// determine the triangle strip order
	numIndices = ( ds->numVerts - 2 ) * 3;
	if( numIndices > MAX_INDICES )
	{
		Error( "MAX_INDICES exceeded for surface" );
	}
	
	// try all possible orderings of the points looking
	// for a strip order that isn't degenerate
	for( rotate = 0; rotate < ds->numVerts; rotate++ )
	{
		for( ni = 0, i = 0; i < ds->numVerts - 2 - i; i++ )
		{
			a = ( ds->numVerts - 1 - i + rotate ) % ds->numVerts;
			b = ( i + rotate ) % ds->numVerts;
			c = ( ds->numVerts - 2 - i + rotate ) % ds->numVerts;
			
			if( IsTriangleDegenerate( drawVerts + ds->firstVert, a, b, c ) )
			{
				break;
			}
			indices[ni++] = a;
			indices[ni++] = b;
			indices[ni++] = c;
			
			if( i + 1 != ds->numVerts - 1 - i )
			{
				a = ( ds->numVerts - 2 - i + rotate ) % ds->numVerts;
				b = ( i + rotate ) % ds->numVerts;
				c = ( i + 1 + rotate ) % ds->numVerts;
				
				if( IsTriangleDegenerate( drawVerts + ds->firstVert, a, b, c ) )
				{
					break;
				}
				indices[ni++] = a;
				indices[ni++] = b;
				indices[ni++] = c;
			}
		}
		if( ni == numIndices )
		{
			break;				// got it done without degenerate triangles
		}
	}
	
	// if any triangle in the strip is degenerate,
	// render from a centered fan point instead
	if( ni < numIndices )
	{
		c_fanSurfaces++;
		SurfaceAsTriFan( ds );
		return;
	}
	
	// a normal tristrip
	c_stripSurfaces++;
	
	if( numDrawIndexes + ni > MAX_MAP_DRAW_INDEXES )
	{
		Error( "MAX_MAP_DRAW_INDEXES" );
	}
	ds->firstIndex = numDrawIndexes;
	ds->numIndexes = ni;
	
	memcpy( drawIndexes + numDrawIndexes, indices, ni * sizeof( int ) );
	numDrawIndexes += ni;
}

/*
===============
EmitPlanarSurf
===============
*/
void EmitPlanarSurf( drawSurface_t* ds )
{
	int             j;
	dsurface_t*     out;
	drawVert_t*     outv;
	
	if( numDrawSurfaces == MAX_MAP_DRAW_SURFS )
	{
		Error( "MAX_MAP_DRAW_SURFS" );
	}
	out = &drawSurfaces[numDrawSurfaces];
	numDrawSurfaces++;
	
	out->surfaceType = MST_PLANAR;
	out->shaderNum = EmitShader( ds->shaderInfo->shader );
	out->firstVert = numDrawVerts;
	out->numVerts = ds->numVerts;
	out->fogNum = ds->fogNum;
	out->lightmapNum = ds->lightmapNum;
	out->lightmapX = ds->lightmapX;
	out->lightmapY = ds->lightmapY;
	out->lightmapWidth = ds->lightmapWidth;
	out->lightmapHeight = ds->lightmapHeight;
	
	VectorCopy( ds->lightmapOrigin, out->lightmapOrigin );
	VectorCopy( ds->lightmapVecs[0], out->lightmapVecs[0] );
	VectorCopy( ds->lightmapVecs[1], out->lightmapVecs[1] );
	VectorCopy( ds->lightmapVecs[2], out->lightmapVecs[2] );
	
	for( j = 0; j < ds->numVerts; j++ )
	{
		if( numDrawVerts == MAX_MAP_DRAW_VERTS )
		{
			Error( "MAX_MAP_DRAW_VERTS" );
		}
		outv = &drawVerts[numDrawVerts];
		numDrawVerts++;
		memcpy( outv, &ds->verts[j], sizeof( *outv ) );
		
		if( debugSurfaces )
		{
			VectorCopy( debugColors[( out - drawSurfaces ) % 12], outv->lightColor );
			outv->lightColor[3] = 1.0f;
		}
		else
		{
			outv->lightColor[0] = 1.0f;
			outv->lightColor[1] = 1.0f;
			outv->lightColor[2] = 1.0f;
			outv->lightColor[3] = 1.0f;
		}
	}
	
	// create the indexes
	SurfaceAsTristrip( out );
}


/*
===============
EmitPatchSurf
===============
*/
void EmitPatchSurf( drawSurface_t* ds )
{
	int             j;
	dsurface_t*     out;
	drawVert_t*     outv;
	
	if( numDrawSurfaces == MAX_MAP_DRAW_SURFS )
	{
		Error( "MAX_MAP_DRAW_SURFS" );
	}
	out = &drawSurfaces[numDrawSurfaces];
	numDrawSurfaces++;
	
	out->surfaceType = MST_PATCH;
	out->shaderNum = EmitShader( ds->shaderInfo->shader );
	out->firstVert = numDrawVerts;
	out->numVerts = ds->numVerts;
	out->firstIndex = numDrawIndexes;
	out->numIndexes = ds->numIndexes;
	out->patchWidth = ds->patchWidth;
	out->patchHeight = ds->patchHeight;
	out->fogNum = ds->fogNum;
	out->lightmapNum = ds->lightmapNum;
	out->lightmapX = ds->lightmapX;
	out->lightmapY = ds->lightmapY;
	out->lightmapWidth = ds->lightmapWidth;
	out->lightmapHeight = ds->lightmapHeight;
	
	VectorCopy( ds->lightmapOrigin, out->lightmapOrigin );
	VectorCopy( ds->lightmapVecs[0], out->lightmapVecs[0] );
	VectorCopy( ds->lightmapVecs[1], out->lightmapVecs[1] );
	VectorCopy( ds->lightmapVecs[2], out->lightmapVecs[2] );
	
	for( j = 0; j < ds->numVerts; j++ )
	{
		if( numDrawVerts == MAX_MAP_DRAW_VERTS )
		{
			Error( "MAX_MAP_DRAW_VERTS" );
		}
		outv = &drawVerts[numDrawVerts];
		numDrawVerts++;
		memcpy( outv, &ds->verts[j], sizeof( *outv ) );
		
		if( debugSurfaces )
		{
			VectorCopy( debugColors[( out - drawSurfaces ) % 12], outv->lightColor );
			outv->lightColor[3] = 255;
		}
		else
		{
			outv->lightColor[0] = 255;
			outv->lightColor[1] = 255;
			outv->lightColor[2] = 255;
			outv->lightColor[3] = 255;
		}
	}
	
	for( j = 0; j < ds->numIndexes; j++ )
	{
		if( numDrawIndexes == MAX_MAP_DRAW_INDEXES )
		{
			Error( "MAX_MAP_DRAW_INDEXES" );
		}
		drawIndexes[numDrawIndexes] = ds->indexes[j];
		numDrawIndexes++;
	}
}

/*
===============
EmitFlareSurf
===============
*/
void EmitFlareSurf( drawSurface_t* ds )
{
	dsurface_t*     out;
	
	if( numDrawSurfaces == MAX_MAP_DRAW_SURFS )
	{
		Error( "MAX_MAP_DRAW_SURFS" );
	}
	out = &drawSurfaces[numDrawSurfaces];
	numDrawSurfaces++;
	
	out->surfaceType = MST_FLARE;
	out->shaderNum = EmitShader( ds->shaderInfo->shader );
	out->fogNum = ds->fogNum;
	
	VectorCopy( ds->lightmapOrigin, out->lightmapOrigin );
	VectorCopy( ds->lightmapVecs[0], out->lightmapVecs[0] );	// lightColor
	VectorCopy( ds->lightmapVecs[2], out->lightmapVecs[2] );
}


/*
===============
EmitModelSurf
===============
*/
void EmitModelSurf( drawSurface_t* ds )
{
	int             j;
	dsurface_t*     out;
	drawVert_t*     outv;
	
	if( numDrawSurfaces == MAX_MAP_DRAW_SURFS )
	{
		Error( "MAX_MAP_DRAW_SURFS" );
	}
	out = &drawSurfaces[numDrawSurfaces];
	numDrawSurfaces++;
	
	out->surfaceType = MST_TRIANGLE_SOUP;
	out->shaderNum = EmitShader( ds->shaderInfo->shader );
	out->firstVert = numDrawVerts;
	out->numVerts = ds->numVerts;
	out->firstIndex = numDrawIndexes;
	out->numIndexes = ds->numIndexes;
	out->patchWidth = ds->patchWidth;
	out->patchHeight = ds->patchHeight;
	out->fogNum = ds->fogNum;
	out->lightmapNum = ds->lightmapNum;
	out->lightmapX = ds->lightmapX;
	out->lightmapY = ds->lightmapY;
	out->lightmapWidth = ds->lightmapWidth;
	out->lightmapHeight = ds->lightmapHeight;
	
	VectorCopy( ds->lightmapOrigin, out->lightmapOrigin );
	VectorCopy( ds->lightmapVecs[0], out->lightmapVecs[0] );
	VectorCopy( ds->lightmapVecs[1], out->lightmapVecs[1] );
	VectorCopy( ds->lightmapVecs[2], out->lightmapVecs[2] );
	
	for( j = 0; j < ds->numVerts; j++ )
	{
		if( numDrawVerts == MAX_MAP_DRAW_VERTS )
		{
			Error( "MAX_MAP_DRAW_VERTS" );
		}
		outv = &drawVerts[numDrawVerts];
		numDrawVerts++;
		memcpy( outv, &ds->verts[j], sizeof( *outv ) );
		
		if( debugSurfaces )
		{
			VectorCopy( debugColors[( out - drawSurfaces ) % 12], outv->lightColor );
			outv->lightColor[3] = 255;
		}
		else
		{
			outv->lightColor[0] = 1.0f;
			outv->lightColor[1] = 1.0f;
			outv->lightColor[2] = 1.0f;
			outv->lightColor[3] = 1.0f;
		}
	}
	
	for( j = 0; j < ds->numIndexes; j++ )
	{
		if( numDrawIndexes == MAX_MAP_DRAW_INDEXES )
		{
			Error( "MAX_MAP_DRAW_INDEXES" );
		}
		drawIndexes[numDrawIndexes] = ds->indexes[j];
		numDrawIndexes++;
	}
}

//======================================================================

/*
==================
CreateFlareSurface

Light flares from surface lights become
==================
*/
void CreateFlareSurface( drawSurface_t* faceDs )
{
	drawSurface_t*  ds;
	int             i;
	
	ds = AllocDrawSurf();
	
	if( faceDs->shaderInfo->flareShader[0] )
	{
		ds->shaderInfo = ShaderInfoForShader( faceDs->shaderInfo->flareShader );
	}
	else
	{
		ds->shaderInfo = ShaderInfoForShader( "flareshader" );
	}
	ds->flareSurface = qtrue;
	VectorCopy( faceDs->lightmapVecs[2], ds->lightmapVecs[2] );
	
	// find midpoint
	VectorClear( ds->lightmapOrigin );
	for( i = 0; i < faceDs->numVerts; i++ )
	{
		VectorAdd( ds->lightmapOrigin, faceDs->verts[i].xyz, ds->lightmapOrigin );
	}
	VectorScale( ds->lightmapOrigin, 1.0 / faceDs->numVerts, ds->lightmapOrigin );
	
	VectorMA( ds->lightmapOrigin, 2, ds->lightmapVecs[2], ds->lightmapOrigin );
	
	VectorCopy( faceDs->shaderInfo->color, ds->lightmapVecs[0] );
	
	// FIXME: fog
}

/*
=====================
FilterDrawsurfsIntoTree

Upon completion, all drawsurfs that actually generate a reference
will have been emited to the bspfile arrays, and the references
will have valid final indexes
=====================
*/
void FilterDrawsurfsIntoTree( entity_t* e, tree_t* tree )
{
	int             i;
	drawSurface_t*  ds;
	int             refs;
	int             c_surfs, c_refs;
	
	Sys_FPrintf( SYS_VRB, "----- FilterDrawsurfsIntoTree -----\n" );
	
	c_surfs = 0;
	c_refs = 0;
	for( i = e->firstDrawSurf; i < numMapDrawSurfs; i++ )
	{
		ds = &mapDrawSurfs[i];
		
		if( !ds->numVerts && !ds->flareSurface )
		{
			continue;
		}
		
		if( ds->miscModel )
		{
			refs = FilterMiscModelSurfIntoTree( ds, tree );
			EmitModelSurf( ds );
		}
		else if( ds->patch )
		{
			refs = FilterPatchSurfIntoTree( ds, tree );
			EmitPatchSurf( ds );
		}
		else if( ds->flareSurface )
		{
			refs = FilterFlareSurfIntoTree( ds, tree );
			EmitFlareSurf( ds );
		}
		else
		{
			refs = FilterFaceIntoTree( ds, tree );
//          if ( ds->shaderInfo->value >= 1000 ) { // ds->shaderInfo->flareShader[0] ) {
			if( ds->shaderInfo->flareShader[0] )
			{
				CreateFlareSurface( ds );
			}
			EmitPlanarSurf( ds );
		}
		if( refs > 0 )
		{
			c_surfs++;
			c_refs += refs;
		}
	}
	Sys_FPrintf( SYS_VRB, "%5i emited drawsurfs\n", c_surfs );
	Sys_FPrintf( SYS_VRB, "%5i references\n", c_refs );
	Sys_FPrintf( SYS_VRB, "%5i stripfaces\n", c_stripSurfaces );
	Sys_FPrintf( SYS_VRB, "%5i fanfaces\n", c_fanSurfaces );
}
