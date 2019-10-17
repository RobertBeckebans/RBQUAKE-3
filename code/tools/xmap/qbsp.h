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

#include "../common/cmdlib.h"
#include "../common/inout.h"
#include "../common/mathlib.h"
#include "../common/scriplib.h"
#include "../common/polylib.h"
#include "../common/imagelib.h"
#include "../common/threads.h"
#include "../common/bspfile.h"

#include "shaders.h"
#include "mesh.h"


#define	MAX_PATCH_SIZE	32

#define	CLIP_EPSILON		0.1
#define	PLANENUM_LEAF		-1

#define	HINT_PRIORITY		1000

typedef struct parseMesh_s
{
	struct parseMesh_s* next;
	
	qboolean        patchDef3;
	vec_t           info[7];
	
	mesh_t          mesh;
	shaderInfo_t*   shaderInfo;
	
	qboolean        grouped;	// used during shared edge grouping
	struct parseMesh_s* groupChain;
} parseMesh_t;

typedef struct bspFace_s
{
	struct bspFace_s* next;
	int             planenum;
	int             priority;	// added to value calculation
	qboolean        checked;
	qboolean        hint;
	winding_t*      w;
} bspFace_t;

typedef struct plane_s
{
	vec3_t          normal;
	vec_t           dist;
	int             type;
	struct plane_s* hash_chain;
} plane_t;

typedef struct side_s
{
	int             planenum;
	
	float           texMat[2][3];	// brush primitive texture matrix
	// for old brush coordinates mode
	float           vecs[2][4];	// texture coordinate mapping
	
	winding_t*      winding;
	winding_t*      visibleHull;	// convex hull of all visible fragments
	
	struct shaderInfo_s* shaderInfo;
	
	int             contents;	// from shaderInfo
	int             surfaceFlags;	// from shaderInfo
	int             value;		// from shaderInfo
	
	qboolean        visible;	// choose visble planes first
	qboolean        bevel;		// don't ever use for bsp splitting, and don't bother
	// making windings for it
	qboolean        backSide;	// generated side for a xmap_backShader
} side_t;


#define	MAX_BRUSH_SIDES		1024

typedef struct bspBrush_s
{
	struct bspBrush_s* next;
	
	int             entitynum;	// editor numbering
	int             brushnum;	// editor numbering
	
	struct shaderInfo_s* contentShader;
	
	int             contents;
	qboolean        detail;
	qboolean        opaque;
	int             outputNumber;	// set when the brush is written to the file list
	
	int             portalareas[2];
	
	struct bspBrush_s* original;	// chopped up brushes will reference the originals
	
	vec3_t          mins, maxs;
	int             numsides;
	side_t          sides[6];	// variably sized
} bspBrush_t;



typedef struct drawSurface_s
{
	shaderInfo_t*   shaderInfo;
	
	bspBrush_t*     mapBrush;	// not valid for patches
	side_t*         side;		// not valid for patches
	
	struct drawSurface_s* nextOnShader;	// when sorting by shader for lightmaps
	
	int             fogNum;		// set by FogDrawSurfs
	
	int             lightmapNum;	// -1 = no lightmap
	int             lightmapX, lightmapY;
	int             lightmapWidth, lightmapHeight;
	
	int             numVerts;
	drawVert_t*     verts;
	
	int             numIndexes;
	int*            indexes;
	
	// for faces only
	int             planeNum;
	
	vec3_t          lightmapOrigin;	// also used for flares
	vec3_t          lightmapVecs[3];	// also used for flares
	
	// for patches only
	qboolean        patch;
	int             patchWidth;
	int             patchHeight;
	
	// for misc_models only
	qboolean        miscModel;
	
	qboolean        flareSurface;
} drawSurface_t;

typedef struct drawSurfaceRef_s
{
	struct drawSurfaceRef_s* nextRef;
	int             outputNumber;
} drawSurfaceRef_t;

typedef struct node_s
{
	// both leafs and nodes
	int             planenum;	// -1 = leaf node
	struct node_s*  parent;
	vec3_t          mins, maxs;	// valid after portalization
	bspBrush_t*     volume;		// one for each leaf/node
	
	// nodes only
	side_t*         side;		// the side that created the node
	struct node_s*  children[2];
	qboolean        hint;
	int             tinyportals;
	vec3_t          referencepoint;
	
	// leafs only
	qboolean        opaque;		// view can never be inside
	qboolean        areaportal;
	int             cluster;	// for portalfile writing
	int             area;		// for areaportals
	bspBrush_t*     brushlist;	// fragments of all brushes in this leaf
	drawSurfaceRef_t* drawSurfReferences;	// references to patches pushed down
	
	int             occupied;	// 1 or greater can reach entity
	entity_t*       occupant;	// for leak file testing
	
	struct portal_s* portals;	// also on nodes during construction
} node_t;

typedef struct portal_s
{
	plane_t         plane;
	node_t*         onnode;		// NULL = outside box
	node_t*         nodes[2];	// [0] = front side of plane
	struct portal_s* next[2];
	winding_t*      winding;
	
	qboolean        sidefound;	// false if ->side hasn't been checked
	qboolean        hint;
	qboolean        areaportal;
	side_t*         side;		// NULL = non-visible
} portal_t;

typedef struct
{
	node_t*         headnode;
	node_t          outside_node;
	vec3_t          mins, maxs;
} tree_t;

extern qboolean nodetail;
extern qboolean fulldetail;
extern qboolean noliquids;
extern qboolean nocurves;
extern qboolean nodoors;
extern qboolean fakemap;
extern qboolean nofog;
extern qboolean testExpand;
extern qboolean showseams;
extern qboolean debugSurfaces;
const byte      debugColors[12][3];
extern vec_t    microvolume;

extern char     outbase[32];
extern char     source[1024];

extern int      samplesize;		//sample size in units
extern int      novertexlighting;
extern int      nogridlighting;

//=============================================================================

// brush.c

int             CountBrushList( bspBrush_t* brushes );
bspBrush_t*     AllocBrush( int numsides );
void            FreeBrush( bspBrush_t* brushes );
void            FreeBrushList( bspBrush_t* brushes );
bspBrush_t*     CopyBrush( bspBrush_t* brush );
void            DrawBrushList( bspBrush_t* brush );
void            WriteBrushList( char* name, bspBrush_t* brush, qboolean onlyvis );
void            PrintBrush( bspBrush_t* brush );
qboolean        BoundBrush( bspBrush_t* brush );
qboolean        CreateBrushWindings( bspBrush_t* brush );
bspBrush_t*     BrushFromBounds( vec3_t mins, vec3_t maxs );
vec_t           BrushVolume( bspBrush_t* brush );
void            WriteBspBrushMap( char* name, bspBrush_t* list );

void            FilterDetailBrushesIntoTree( entity_t* e, tree_t* tree );
void            FilterStructuralBrushesIntoTree( entity_t* e, tree_t* tree );

//=============================================================================

typedef enum
{
	CONVERT_NOTHING,
//  CONVERT_QUAKE1,
//  CONVERT_QUAKE2,
	CONVERT_QUAKE3,
	CONVERT_QUAKE4,
	CONVERT_DOOM3,
//  CONVERT_PREY
} convertType_t;

extern convertType_t convertType;

//=============================================================================

// map.c

extern int      entitySourceBrushes;

// mapplanes[ num^1 ] will always be the mirror or mapplanes[ num ]
// nummapplanes will always be even
extern plane_t  mapPlanes[MAX_MAP_PLANES];
extern int      numMapPlanes;

extern vec3_t   mapMins, mapMaxs;

extern char     mapIndexedShaders[MAX_MAP_BRUSHSIDES][MAX_QPATH];
extern int      numMapIndexedShaders;

extern entity_t* mapEnt;

#define		MAX_BUILD_SIDES		300
extern bspBrush_t* buildBrush;


void            LoadMapFile( char* filename );
int             MapPlaneFromPoints( vec3_t p0, vec3_t p1, vec3_t p2 );
int             FindFloatPlane( vec3_t normal, vec_t dist );
int             PlaneTypeForNormal( vec3_t normal );
bspBrush_t*     FinishBrush( void );
void            AdjustBrushesForOrigin( entity_t* ent, vec3_t origin );

drawSurface_t*  AllocDrawSurf( void );
drawSurface_t*  DrawSurfaceForSide( bspBrush_t* b, side_t* s, winding_t* w );

//=============================================================================

//=============================================================================

// draw.c

extern qboolean drawFlag;

void            Draw_Winding( winding_t* w );
void            Draw_AuxWinding( winding_t* w );
void            Draw_Scene( void ( *drawFunc )( void ) );

//=============================================================================

// csg

bspBrush_t*     MakeBspBrushList( bspBrush_t* brushes, vec3_t clipmins, vec3_t clipmaxs );

//=============================================================================

// brushbsp

#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4

int             BoxOnPlaneSide( vec3_t mins, vec3_t maxs, plane_t* plane );
qboolean        WindingIsTiny( winding_t* w );

void            SplitBrush( bspBrush_t* brush, int planenum, bspBrush_t** front, bspBrush_t** back );

tree_t*         AllocTree( void );
node_t*         AllocNode( void );

tree_t*         BrushBSP( bspBrush_t* brushlist, vec3_t mins, vec3_t maxs );

//=============================================================================

// portals.c

void            MakeHeadnodePortals( tree_t* tree );
void            MakeNodePortal( node_t* node );
void            SplitNodePortals( node_t* node );

qboolean        Portal_Passable( portal_t* p );

qboolean        FloodEntities( tree_t* tree );
void            FillOutside( node_t* headnode );
void            FloodAreas( tree_t* tree );
bspFace_t*      VisibleFaces( entity_t* e, tree_t* tree );
void            FreePortal( portal_t* p );

void            MakeTreePortals( tree_t* tree );

//=============================================================================

// glfile.c

void            OutputWinding( winding_t* w, FILE* glview );
void            WriteGLView( tree_t* tree, char* source );

//=============================================================================

// leakfile.c

xmlNodePtr      LeakFile( tree_t* tree );

//=============================================================================

// prtfile.c

void            NumberClusters( tree_t* tree );
void            WritePortalFile( tree_t* tree );

//=============================================================================

// writebsp.c

void            SetModelNumbers( void );

int             EmitShader( const char* shader );
void            EmitBrushes( bspBrush_t* brushes );

void            BeginBSPFile( void );
void            EndBSPFile( void );

void            BeginModel( entity_t* e );
void            EndModel( entity_t* e, node_t* headnode );


//=============================================================================

// tree.c

void            FreeTree( tree_t* tree );
void            FreeTree_r( node_t* node );
void            PrintTree_r( node_t* node, int depth );
void            FreeTreePortals_r( node_t* node );

//=============================================================================

// patch.c

extern int      numMapPatches;

drawSurface_t*  DrawSurfaceForMesh( mesh_t* m );
void            ParsePatch( qboolean patchDef3 );
mesh_t*         SubdivideMesh( mesh_t in, float maxError, float minLength );
void            PatchMapDrawSurfs( entity_t* e );

//=============================================================================

// lightmap.c

void            AllocateLightmaps( entity_t* e );

//=============================================================================

// tjunction.c

void            FixTJunctions( entity_t* e );


//=============================================================================

// fog.c

void            FogDrawSurfs( void );
winding_t*      WindingFromDrawSurf( drawSurface_t* ds );

//=============================================================================

// facebsp.c

extern int      blockSize[3];

bspFace_t*      BspFaceForPortal( portal_t* p );
bspFace_t*      MakeStructuralBspFaceList( bspBrush_t* list );
bspFace_t*      MakeVisibleBspFaceList( bspBrush_t* list );
tree_t*         FaceBSP( bspFace_t* list );

//=============================================================================

// misc_model.c

extern int      c_triangleModels;
extern int      c_triangleSurfaces;
extern int      c_triangleVertexes;
extern int      c_triangleIndexes;

void            AddTriangleModel( entity_t* e, qboolean applyTransform );
void            AddTriangleModels( void );

//=============================================================================

// surface.c

extern drawSurface_t mapDrawSurfs[MAX_MAP_DRAW_SURFS];
extern int      numMapDrawSurfs;

drawSurface_t*  AllocDrawSurf( void );
void            MergeSides( entity_t* e, tree_t* tree );
void            SubdivideDrawSurfs( entity_t* e, tree_t* tree );
void            MakeDrawSurfaces( bspBrush_t* b );
void            ClipSidesIntoTree( entity_t* e, tree_t* tree );
void            FilterDrawsurfsIntoTree( entity_t* e, tree_t* tree );

//==============================================================================

// brush_primit.c

#define BPRIMIT_UNDEFINED  0
#define BPRIMIT_OLDBRUSHES 1
#define BPRIMIT_NEWBRUSHES 2
#define BPRIMIT_D3BRUSHES  3
#define BPRIMIT_Q4BRUSHES  4
extern int      g_bBrushPrimit;

void            ComputeAxisBase( vec3_t normal, vec3_t texX, vec3_t texY );
