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

#include "q_shared.h"
#include "qcommon.h"
#include "cm_polylib.h"

#define	MAX_SUBMODELS			MAX_MODELS	// was 256
#define	BOX_MODEL_HANDLE		(MAX_SUBMODELS -1)	// was 255
#define CAPSULE_MODEL_HANDLE	(MAX_SUBMODELS -2)	// was 254


typedef struct
{
	cplane_t*       plane;
	int             children[2];	// negative numbers are leafs
} cNode_t;

typedef struct
{
	int             cluster;
	int             area;
	
	int             firstLeafBrush;
	int             numLeafBrushes;
	
	int             firstLeafSurface;
	int             numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s
{
	vec3_t          mins, maxs;
	cLeaf_t         leaf;		// submodels don't reference the main tree
} cmodel_t;

typedef struct cbrushedge_s
{
	vec3_t          p0;
	vec3_t          p1;
} cbrushedge_t;

typedef struct
{
	cplane_t*       plane;
	int             planeNum;
	int             surfaceFlags;
	int             shaderNum;
	winding_t*      winding;
} cbrushside_t;

typedef struct
{
	int             shaderNum;	// the shader that determined the contents
	int             contents;
	vec3_t          bounds[2];
	int             numsides;
	cbrushside_t*   sides;
	int             checkcount;	// to avoid repeated testings
	qboolean        collided;	// marker for optimisation
	cbrushedge_t*   edges;
	int             numEdges;
} cbrush_t;


typedef struct cPlane_s
{
	float           plane[4];
	int             signbits;	// signx + (signy<<1) + (signz<<2), used as lookup during collision
	struct cPlane_s* hashChain;
} cPlane_t;

// 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
#define MAX_FACET_BEVELS (4 + 6 + 16)

// a facet is a subdivided element of a patch aproximation or model
typedef struct
{
	int             surfacePlane;
	
	int             numBorders;
	int             borderPlanes[MAX_FACET_BEVELS];
	int             borderInward[MAX_FACET_BEVELS];
	qboolean        borderNoAdjust[MAX_FACET_BEVELS];
} cFacet_t;

typedef struct cSurfaceCollide_s
{
	vec3_t          bounds[2];
	
	int             numPlanes;	// surface planes plus edge planes
	cPlane_t*       planes;
	
	int             numFacets;
	cFacet_t*       facets;
} cSurfaceCollide_t;

typedef struct
{
	int             type;
	
	int             checkcount;	// to avoid repeated testings
	int             surfaceFlags;
	int             contents;
	
	cSurfaceCollide_t* sc;
} cSurface_t;

typedef struct
{
	int             floodnum;
	int             floodvalid;
} cArea_t;

typedef struct
{
	char            name[MAX_QPATH];
	
	int             numShaders;
	dshader_t*      shaders;
	
	int             numBrushSides;
	cbrushside_t*   brushsides;
	
	int             numPlanes;
	cplane_t*       planes;
	
	int             numNodes;
	cNode_t*        nodes;
	
	int             numLeafs;
	cLeaf_t*        leafs;
	
	int             numLeafBrushes;
	int*            leafbrushes;
	
	int             numLeafSurfaces;
	int*            leafsurfaces;
	
	int             numSubModels;
	cmodel_t*       cmodels;
	
	int             numBrushes;
	cbrush_t*       brushes;
	
	int             numClusters;
	int             clusterBytes;
	byte*           visibility;
	qboolean        vised;		// if false, visibility is just a single cluster of ffs
	
	int             numEntityChars;
	char*           entityString;
	
	int             numAreas;
	cArea_t*        areas;
	int*            areaPortals;	// [ numAreas*numAreas ] reference counts
	
	int             numSurfaces;
	cSurface_t**    surfaces;	// non-patches will be NULL
	
	int             floodvalid;
	int             checkcount;	// incremented on each trace
	
	qboolean        perPolyCollision;
} clipMap_t;


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define	SURFACE_CLIP_EPSILON	(0.125)

extern clipMap_t cm;
extern int      c_pointcontents;
extern int      c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;
extern cvar_t*  cm_noAreas;
extern cvar_t*  cm_noCurves;
extern cvar_t*  cm_forceTriangles;
extern cvar_t*  cm_showCurves;
extern cvar_t*  cm_showTriangles;


typedef struct
{
	float           startRadius;
	float           endRadius;
} biSphere_t;

// Used for oriented capsule collision detection
typedef struct
{
	float           radius;
	float           halfheight;
	vec3_t          offset;
} sphere_t;

typedef struct
{
	traceType_t     type;
	vec3_t          start;
	vec3_t          end;
	vec3_t          size[2];	// size of the box being swept through the model
	vec3_t          offsets[8];	// [signbits][x] = either size[0][x] or size[1][x]
	float           maxOffset;	// longest corner length from origin
	vec3_t          extents;	// greatest of abs(size[0]) and abs(size[1])
	vec3_t          bounds[2];	// enclosing box of start and end surrounding by size
	vec3_t          modelOrigin;	// origin of the model tracing through
	int             contents;	// ored contents of the model tracing through
	qboolean        isPoint;	// optimized case
	trace_t         trace;		// returned from trace call
	sphere_t        sphere;		// sphere for oriented capsule collision
	biSphere_t      biSphere;
	qboolean        testLateralCollision;	// whether or not to test for lateral collision
} traceWork_t;

typedef struct leafList_s
{
	int             count;
	int             maxcount;
	qboolean        overflowed;
	int*            list;
	vec3_t          bounds[2];
	int             lastLeaf;	// for overflows where each leaf can't be stored individually
	void ( *storeLeafs )( struct leafList_s* ll, int nodenum );
} leafList_t;



// cm_patch.c

/*

This file does not reference any globals, and has these entry points:

void CM_ClearLevelPatches( void );
struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, const vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, flaot *points) );


Issues for collision against curved surfaces:

Surface edges need to be handled differently than surface planes

Plane expansion causes raw surfaces to expand past expanded bounding box

Position test of a volume against a surface is tricky.

Position test of a point against a surface is not well defined, because the surface has no volume.


Tracing leading edge points instead of volumes?
Position test by tracing corner to corner? (8*7 traces -- ouch)

coplanar edges
triangulated patches
degenerate patches

  endcaps
  degenerate

WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/


#define	MAX_FACETS			1024
#define	MAX_PATCH_PLANES	4096

#define	MAX_GRID_SIZE	129

typedef struct
{
	int             width;
	int             height;
	qboolean        wrapWidth;
	qboolean        wrapHeight;
	vec3_t          points[MAX_GRID_SIZE][MAX_GRID_SIZE];	// [width][height]
} cGrid_t;

#define	SUBDIVIDE_DISTANCE	16	//4 // never more than this units away from curve
#define	PLANE_TRI_EPSILON	0.1
#define	WRAP_POINT_EPSILON	0.1


cSurfaceCollide_t* CM_GeneratePatchCollide( int width, int height, vec3_t* points );
void            CM_ClearLevelPatches( void );

// cm_trisoup.c

typedef struct
{
	int             numTriangles;
	int             indexes[SHADER_MAX_INDEXES];
	
	int             trianglePlanes[SHADER_MAX_TRIANGLES];
	
	vec3_t          points[SHADER_MAX_TRIANGLES][3];
} cTriangleSoup_t;

cSurfaceCollide_t* CM_GenerateTriangleSoupCollide( int numVertexes, vec3_t* vertexes, int numIndexes, int* indexes );


// cm_test.c
extern const cSurfaceCollide_t* debugSurfaceCollide;
extern const cFacet_t* debugFacet;
extern qboolean debugBlock;
extern vec3_t   debugBlockPoints[4];

int             CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t** list, int listsize );

void            CM_StoreLeafs( leafList_t* ll, int nodenum );
void            CM_StoreBrushes( leafList_t* ll, int nodenum );

void            CM_BoxLeafnums_r( leafList_t* ll, int nodenum );

cmodel_t*       CM_ClipHandleToModel( clipHandle_t handle );

qboolean        CM_BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
qboolean        CM_BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t point );


#if defined(USE_BULLET)
	void			CM_InitBullet();
	void			CM_ShutdownBullet();
#endif
