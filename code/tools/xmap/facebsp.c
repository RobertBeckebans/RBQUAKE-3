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


int             blockSize[3] = { 0, 0, 0 };

int             c_faceLeafs;


/*
================
AllocBspFace
================
*/
bspFace_t*      AllocBspFace( void )
{
	bspFace_t*      f;
	
	f = malloc( sizeof( *f ) );
	memset( f, 0, sizeof( *f ) );
	
	return f;
}

/*
================
FreeBspFace
================
*/
void FreeBspFace( bspFace_t* f )
{
	if( f->w )
	{
		FreeWinding( f->w );
	}
	free( f );
}


/*
================
SelectSplitPlaneNum
================
*/
static void SelectSplitPlaneNum( node_t* node, bspFace_t* list, int* splitPlaneNum, int* hintSplit )
{
	bspFace_t*      split;
	bspFace_t*      check;
	bspFace_t*      bestSplit;
	int             splits, facing, front, back;
	int             side;
	plane_t*        plane;
	int             value, bestValue;
	int             i;
	vec3_t          normal;
	float           dist;
	int             planenum;
	
	*splitPlaneNum = -1;
	*hintSplit = qfalse;
	
	// ydnar 2002-06-24: changed this to split on z-axis as well
	// ydnar 2002-09-21: changed blocksize to be a vector, so mappers can specify a 3 element value
	
	// if it is crossing a 1k block boundary, force a split
	for( i = 0; i < 3; i++ )
	{
		if( blockSize[i] <= 0 )
		{
			continue;
		}
		
		dist = blockSize[i] * ( floor( node->mins[i] / blockSize[i] ) + 1 );
		if( node->maxs[i] > dist )
		{
			VectorClear( normal );
			normal[i] = 1;
			planenum = FindFloatPlane( normal, dist );
			*splitPlaneNum = planenum;
			return;
		}
	}
	
	// pick one of the face planes
	bestValue = -99999;
	bestSplit = list;
	
	for( split = list; split; split = split->next )
	{
		split->checked = qfalse;
	}
	
	for( split = list; split; split = split->next )
	{
		if( split->checked )
		{
			continue;
		}
		plane = &mapPlanes[split->planenum];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;
		for( check = list; check; check = check->next )
		{
			if( check->planenum == split->planenum )
			{
				facing++;
				check->checked = qtrue;	// won't need to test this plane again
				continue;
			}
			side = WindingOnPlaneSide( check->w, plane->normal, plane->dist );
			if( side == SIDE_CROSS )
			{
				splits++;
			}
			else if( side == SIDE_FRONT )
			{
				front++;
			}
			else if( side == SIDE_BACK )
			{
				back++;
			}
		}
		value = 5 * facing - 5 * splits;	// - abs(front-back);
		if( plane->type < 3 )
		{
			value += 5;			// axial is better
		}
		value += split->priority;	// prioritize hints higher
		
		if( value > bestValue )
		{
			bestValue = value;
			bestSplit = split;
		}
	}
	
	if( bestValue == -99999 )
	{
		return;
	}
	
	if( bestSplit->hint )
	{
		*hintSplit = qtrue;
	}
	
	*splitPlaneNum = bestSplit->planenum;
}

int CountFaceList( bspFace_t* list )
{
	int             c;
	
	c = 0;
	for( ; list; list = list->next )
	{
		c++;
	}
	return c;
}

/*
================
BuildFaceTree_r
================
*/
void BuildFaceTree_r( node_t* node, bspFace_t* list )
{
	bspFace_t*      split;
	bspFace_t*      next;
	int             side;
	plane_t*        plane;
	bspFace_t*      newFace;
	bspFace_t*      childLists[2];
	winding_t*      frontWinding, *backWinding;
	int             i;
	int             splitPlaneNum;
	int             hintSplit;
	
	i = CountFaceList( list );
	
	SelectSplitPlaneNum( node, list, &splitPlaneNum, &hintSplit );
	
	// if we don't have any more faces, this is a node
	if( splitPlaneNum == -1 )
	{
		node->planenum = PLANENUM_LEAF;
		c_faceLeafs++;
		return;
	}
	
	// partition the list
	node->planenum = splitPlaneNum;
	node->hint = hintSplit;
	plane = &mapPlanes[splitPlaneNum];
	childLists[0] = NULL;
	childLists[1] = NULL;
	for( split = list; split; split = next )
	{
		next = split->next;
		
		if( split->planenum == node->planenum )
		{
			FreeBspFace( split );
			continue;
		}
		
		side = WindingOnPlaneSide( split->w, plane->normal, plane->dist );
		
		if( side == SIDE_CROSS )
		{
			ClipWindingEpsilon( split->w, plane->normal, plane->dist, CLIP_EPSILON * 2, &frontWinding, &backWinding );
			if( frontWinding )
			{
				newFace = AllocBspFace();
				newFace->w = frontWinding;
				newFace->next = childLists[0];
				newFace->planenum = split->planenum;
				newFace->priority = split->priority;
				newFace->hint = split->hint;
				childLists[0] = newFace;
			}
			if( backWinding )
			{
				newFace = AllocBspFace();
				newFace->w = backWinding;
				newFace->next = childLists[1];
				newFace->planenum = split->planenum;
				newFace->priority = split->priority;
				newFace->hint = split->hint;
				childLists[1] = newFace;
			}
			FreeBspFace( split );
		}
		else if( side == SIDE_FRONT )
		{
			split->next = childLists[0];
			childLists[0] = split;
		}
		else if( side == SIDE_BACK )
		{
			split->next = childLists[1];
			childLists[1] = split;
		}
	}
	
	
	// recursively process children
	for( i = 0; i < 2; i++ )
	{
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		VectorCopy( node->mins, node->children[i]->mins );
		VectorCopy( node->maxs, node->children[i]->maxs );
	}
	
	for( i = 0; i < 3; i++ )
	{
		if( plane->normal[i] == 1 )
		{
			node->children[0]->mins[i] = plane->dist;
			node->children[1]->maxs[i] = plane->dist;
			break;
		}
	}
	
	for( i = 0; i < 2; i++ )
	{
		BuildFaceTree_r( node->children[i], childLists[i] );
	}
}


/*
================
FaceBSP

List will be freed before returning
================
*/
tree_t*         FaceBSP( bspFace_t* list )
{
	tree_t*         tree;
	bspFace_t*      face;
	int             i;
	int             count;
	
	Sys_FPrintf( SYS_VRB, "--- FaceBSP ---\n" );
	
	tree = AllocTree();
	
	count = 0;
	for( face = list; face; face = face->next )
	{
		count++;
		for( i = 0; i < face->w->numpoints; i++ )
		{
			AddPointToBounds( face->w->p[i], tree->mins, tree->maxs );
		}
	}
	Sys_FPrintf( SYS_VRB, "%5i faces\n", count );
	
	tree->headnode = AllocNode();
	VectorCopy( tree->mins, tree->headnode->mins );
	VectorCopy( tree->maxs, tree->headnode->maxs );
	c_faceLeafs = 0;
	
	BuildFaceTree_r( tree->headnode, list );
	
	Sys_FPrintf( SYS_VRB, "%5i leafs\n", c_faceLeafs );
	
	return tree;
}


/*
=================
BspFaceForPortal
=================
*/
bspFace_t*      BspFaceForPortal( portal_t* p )
{
	bspFace_t*      f;
	
	f = AllocBspFace();
	f->w = CopyWinding( p->winding );
	f->planenum = p->onnode->planenum & ~1;
	
	return f;
}



/*
=================
MakeStructuralBspFaceList
=================
*/
bspFace_t*      MakeStructuralBspFaceList( bspBrush_t* list )
{
	bspBrush_t*     b;
	int             i;
	side_t*         s;
	winding_t*      w;
	bspFace_t*      f, *flist;
	
	flist = NULL;
	for( b = list; b; b = b->next )
	{
		if( b->detail )
		{
			continue;
		}
		for( i = 0; i < b->numsides; i++ )
		{
			s = &b->sides[i];
			w = s->winding;
			if( !w )
			{
				continue;
			}
			f = AllocBspFace();
			f->w = CopyWinding( w );
			f->planenum = s->planenum & ~1;
			f->next = flist;
			if( s->surfaceFlags & SURF_HINT )
			{
				//f->priority = HINT_PRIORITY;
				f->hint = qtrue;
			}
			flist = f;
		}
	}
	
	return flist;
}

/*
=================
MakeVisibleBspFaceList
=================
*/
bspFace_t*      MakeVisibleBspFaceList( bspBrush_t* list )
{
	bspBrush_t*     b;
	int             i;
	side_t*         s;
	winding_t*      w;
	bspFace_t*      f, *flist;
	
	flist = NULL;
	for( b = list; b; b = b->next )
	{
		if( b->detail )
		{
			continue;
		}
		for( i = 0; i < b->numsides; i++ )
		{
			s = &b->sides[i];
			w = s->visibleHull;
			if( !w )
			{
				continue;
			}
			f = AllocBspFace();
			f->w = CopyWinding( w );
			f->planenum = s->planenum & ~1;
			f->next = flist;
			if( s->surfaceFlags & SURF_HINT )
			{
				//f->priority = HINT_PRIORITY;
				f->hint = qtrue;
			}
			flist = f;
		}
	}
	
	return flist;
}
