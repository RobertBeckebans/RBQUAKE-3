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


char            source[1024];
char            name[1024];

vec_t           microvolume = 1.0;
qboolean        glview;
qboolean        nodetail;
qboolean        fulldetail;
qboolean        onlyents;
qboolean        onlytextures;
qboolean        noliquids;
qboolean        leaktest;
qboolean        nocurves;
qboolean        nodoors = qfalse;
qboolean        fakemap;
qboolean        notjunc;
qboolean        nomerge;
qboolean        nofog;
qboolean        nosubdivide;
qboolean        testExpand;
qboolean        showseams;

qboolean        debugSurfaces;
const byte      debugColors[12][3] =
{
	{255, 0, 0},
	{192, 128, 128},
	{255, 255, 0},
	{192, 192, 128},
	{0, 255, 255},
	{128, 192, 192},
	{0, 0, 255},
	{128, 128, 192},
	{255, 0, 255},
	{192, 128, 192},
	{0, 255, 0},
	{128, 192, 128}
};

char            outbase[32];


static void DrawPortal( portal_t* p, qboolean areaportal )
{
	winding_t*      w;
	int             sides;
	
	sides = PortalVisibleSides( p );
	if( !sides )
	{
		return;
	}
	
	w = p->winding;
	
	if( sides == 2 )				// back side
	{
		w = ReverseWinding( w );
	}
	
	if( areaportal )
	{
		Draw_AuxWinding( w );
	}
	else
	{
		Draw_Winding( w );
	}
	
	if( sides == 2 )
	{
		FreeWinding( w );
	}
}

static void DrawTree_r( node_t* node )
{
	int             s;
	portal_t*       p, *nextp;
	winding_t*      w;
	
	if( node->planenum != PLANENUM_LEAF )
	{
		DrawTree_r( node->children[0] );
		DrawTree_r( node->children[1] );
		return;
	}
	
	// draw all the portals
	for( p = node->portals; p; p = p->next[s] )
	{
		w = p->winding;
		s = ( p->nodes[1] == node );
		
		if( w )					// && p->nodes[0] == node)
		{
			if( Portal_Passable( p ) )
			{
				continue;
			}
			
			DrawPortal( p, node->areaportal );
		}
	}
}

static tree_t*  drawTree = NULL;
static void DrawTree( void )
{
	DrawTree_r( drawTree->headnode );
}

/*
============
ProcessWorldModel
============
*/
void ProcessWorldModel( void )
{
	int             s;
	entity_t*       e;
	tree_t*         tree;
	bspFace_t*      faces;
	qboolean        leaked;
	xmlNodePtr      polyline, leaknode;
	char            level[2];
	const char*     value;
	
	e = &entities[0];
	e->firstDrawSurf = 0;		//numMapDrawSurfs;
	
	// sets integer blockSize from worldspawn "_blocksize" key if it exists
	value = ValueForKey( e, "_blocksize" );
	if( value[0] == '\0' )
	{
		value = ValueForKey( e, "blocksize" );
	}
	if( value[0] == '\0' )
	{
		value = ValueForKey( e, "chopsize" );    // sof2
	}
	if( value[0] != '\0' )
	{
		// scan 3 numbers
		s = sscanf( value, "%d %d %d", &blockSize[0], &blockSize[1], &blockSize[2] );
		
		// handle legacy case
		if( s == 1 )
		{
			blockSize[1] = blockSize[0];
			blockSize[2] = blockSize[0];
		}
	}
	Sys_Printf( "block size = { %d %d %d }\n", blockSize[0], blockSize[1], blockSize[2] );
	
	BeginModel( e );
	
	// check for patches with adjacent edges that need to LOD together
	PatchMapDrawSurfs( e );
	
	// build an initial bsp tree using all of the sides
	// of all of the structural brushes
	faces = MakeStructuralBspFaceList( entities[0].brushes );
	tree = FaceBSP( faces );
	MakeTreePortals( tree );
	FilterStructuralBrushesIntoTree( e, tree );
	
	if( drawFlag )
	{
		// draw unoptimized portals in new window
		drawTree = tree;
		Draw_Scene( DrawTree );
	}
	
	// see if the bsp is completely enclosed
	if( FloodEntities( tree ) )
	{
		// rebuild a better bsp tree using only the
		// sides that are visible from the inside
		FillOutside( tree->headnode );
		
		// chop the sides to the convex hull of
		// their visible fragments, giving us the smallest
		// polygons
		ClipSidesIntoTree( e, tree );
		
		faces = MakeVisibleBspFaceList( entities[0].brushes );
		FreeTree( tree );
		tree = FaceBSP( faces );
		MakeTreePortals( tree );
		FilterStructuralBrushesIntoTree( e, tree );
		leaked = qfalse;
	}
	else
	{
		Sys_FPrintf( SYS_NOXML, "**********************\n" );
		Sys_FPrintf( SYS_NOXML, "******* leaked *******\n" );
		Sys_FPrintf( SYS_NOXML, "**********************\n" );
		
		polyline = LeakFile( tree );
		leaknode = xmlNewNode( NULL, "message" );
		xmlNodeSetContent( leaknode, "MAP LEAKED\n" );
		xmlAddChild( leaknode, polyline );
		level[0] = ( int )'0' + SYS_ERR;
		level[1] = 0;
		xmlSetProp( leaknode, "level", ( char* )&level );
		xml_SendNode( leaknode );
		
		if( leaktest )
		{
			Sys_Printf( "--- MAP LEAKED, ABORTING LEAKTEST ---\n" );
			exit( 0 );
		}
		leaked = qtrue;
		
		// chop the sides to the convex hull of
		// their visible fragments, giving us the smallest
		// polygons
		ClipSidesIntoTree( e, tree );
	}
	
	// save out information for visibility processing
	NumberClusters( tree );
	
	if( !leaked )
	{
		WritePortalFile( tree );
	}
	
	if( glview )
	{
		// dump the portals for debugging
		WriteGLView( tree, source );
	}
	
	FloodAreas( tree );
	
	if( drawFlag )
	{
		// draw optimized portals in new window
		drawTree = tree;
		Draw_Scene( DrawTree );
	}
	
	// add references to the detail brushes
	FilterDetailBrushesIntoTree( e, tree );
	
	// create drawsurfs for triangle models
	AddTriangleModels();
	
	// drawsurfs that cross fog boundaries will need to
	// be split along the bound
	if( !nofog )
	{
		FogDrawSurfs();			// may fragment drawsurfs
	}
	
	// subdivide each drawsurf as required by shader tesselation
	if( !nosubdivide )
	{
		SubdivideDrawSurfs( e, tree );
	}
	
	// merge together all common shaders on the same plane and remove
	// all colinear points, so extra tjunctions won't be generated
	if( !nomerge )
	{
		MergeSides( e, tree );	// !@# testing
	}
	
	// add in any vertexes required to fix tjunctions
	if( !notjunc )
	{
		FixTJunctions( e );
	}
	
	// allocate lightmaps for faces and patches
	AllocateLightmaps( e );
	
	// add references to the final drawsurfs in the apropriate clusters
	FilterDrawsurfsIntoTree( e, tree );
	
	EndModel( e, tree->headnode );
	
	FreeTree( tree );
}

/*
============
ProcessSubModel
============
*/
void ProcessSubModel( int entityNum )
{
	entity_t*       e;
	tree_t*         tree;
	bspBrush_t*     b, *bc;
	node_t*         node;
	
	e = &entities[entityNum];
	e->firstDrawSurf = numMapDrawSurfs;
	
	BeginModel( e );
	
	PatchMapDrawSurfs( e );
	
	// just put all the brushes in an empty leaf
	// FIXME: patches?
	node = AllocNode();
	node->planenum = PLANENUM_LEAF;
	for( b = e->brushes; b; b = b->next )
	{
		bc = CopyBrush( b );
		bc->next = node->brushlist;
		node->brushlist = bc;
	}
	
	tree = AllocTree();
	tree->headnode = node;
	
	ClipSidesIntoTree( e, tree );
	
	// create drawsurfs for triangle models
	AddTriangleModel( e, qfalse );
	
	// subdivide each drawsurf as required by shader tesselation or fog
	if( !nosubdivide )
	{
		SubdivideDrawSurfs( e, tree );
	}
	
	// merge together all common shaders on the same plane and remove
	// all colinear points, so extra tjunctions won't be generated
	if( !nomerge )
	{
		MergeSides( e, tree );	// !@# testing
	}
	
	// add in any vertexes required to fix tjunctions
	if( !notjunc )
	{
		FixTJunctions( e );
	}
	
	// allocate lightmaps for faces and patches
	AllocateLightmaps( e );
	
	// add references to the final drawsurfs in the apropriate clusters
	FilterDrawsurfsIntoTree( e, tree );
	
	EndModel( e, node );
	
	FreeTree( tree );
}


/*
============
ProcessModels
============
*/
void ProcessModels( void )
{
	int             i;
	entity_t*       entity;
	const char*     classname;
	const char*     model;
	
	BeginBSPFile();
	
	for( i = 0; i < numEntities; i++ )
	{
		entity = &entities[i];
		
		classname = ValueForKey( entity, "classname" );
		model = ValueForKey( entity, "model" );
		
		if( entity->brushes || entity->patches ||
				( !entity->brushes && !entity->patches && model[0] != '\0' && Q_stricmp( "misc_model", classname ) ) )
		{
			Sys_FPrintf( SYS_VRB, "############### model %i ###############\n", numModels );
			
			if( i == 0 )
			{
				ProcessWorldModel();
			}
			else
			{
				ProcessSubModel( i );
			}
		}
	}
}

/*
============
Bspinfo
============
*/
void Bspinfo( int count, char** fileNames )
{
	int             i;
	char            source[1024];
	int             size;
	FILE*           f;
	
	if( count < 1 )
	{
		Sys_Printf( "No files to dump info for.\n" );
		return;
	}
	
	for( i = 0; i < count; i++ )
	{
		Sys_Printf( "---------------------\n" );
		strcpy( source, fileNames[i] );
		DefaultExtension( source, ".bsp" );
		f = fopen( source, "rb" );
		if( f )
		{
			size = Q_filelength( f );
			fclose( f );
		}
		else
		{
			size = 0;
		}
		Sys_Printf( "%s: %i\n", source, size );
		
		LoadBSPFile( source );
		PrintBSPFileSizes();
		Sys_Printf( "---------------------\n" );
	}
}


/*
============
OnlyEnts
============
*/
void OnlyEnts( void )
{
	char            out[1024];
	
	sprintf( out, "%s.bsp", source );
	LoadBSPFile( out );
	numEntities = 0;
	
	LoadMapFile( name );
	SetModelNumbers();
	
	UnparseEntities();
	
	WriteBSPFile( out );
}


/*
============
OnlyTextures
============
*/
void OnlyTextures( void )
{
	// FIXME!!!
	char            out[1024];
	int             i;
	
	Error( "-onlytextures isn't working now..." );
	
	sprintf( out, "%s.bsp", source );
	
	LoadMapFile( name );
	
	LoadBSPFile( out );
	
	// replace all the drawsurface shader names
	for( i = 0; i < numDrawSurfaces; i++ )
	{
	}
	
	WriteBSPFile( out );
}


int BspMain( int argc, char** argv )
{
	int             i;
	double          start, end;
	char            path[1024];
	
	Sys_Printf( "---- bsp ----\n" );
	
	for( i = 1; i < argc; i++ )
	{
		if( !strcmp( argv[i], "-threads" ) )
		{
			numthreads = atoi( argv[i + 1] );
			i++;
		}
		else if( !strcmp( argv[i], "-glview" ) )
		{
			glview = qtrue;
		}
		else if( !strcmp( argv[i], "-v" ) )
		{
			Sys_Printf( "verbose = true\n" );
			verbose = qtrue;
		}
		else if( !strcmp( argv[i], "-draw" ) )
		{
			Sys_Printf( "drawflag = true\n" );
			drawFlag = qtrue;
		}
		else if( !strcmp( argv[i], "-debugsurfaces" ) )
		{
			Sys_Printf( "emitting debug surfaces\n" );
			debugSurfaces = qtrue;
		}
		else if( !strcmp( argv[i], "-nowater" ) )
		{
			Sys_Printf( "nowater = true\n" );
			noliquids = qtrue;
		}
		else if( !strcmp( argv[i], "-nodetail" ) )
		{
			Sys_Printf( "nodetail = true\n" );
			nodetail = qtrue;
		}
		else if( !strcmp( argv[i], "-fulldetail" ) )
		{
			Sys_Printf( "fulldetail = true\n" );
			fulldetail = qtrue;
		}
		else if( !strcmp( argv[i], "-onlyents" ) )
		{
			Sys_Printf( "onlyents = true\n" );
			onlyents = qtrue;
		}
		else if( !strcmp( argv[i], "-onlytextures" ) )
		{
			Sys_Printf( "onlytextures = true\n" );	// FIXME: make work again!
			onlytextures = qtrue;
		}
		else if( !strcmp( argv[i], "-micro" ) )
		{
			microvolume = atof( argv[i + 1] );
			Sys_Printf( "microvolume = %f\n", microvolume );
			i++;
		}
		else if( !strcmp( argv[i], "-nofog" ) )
		{
			Sys_Printf( "nofog = true\n" );
			nofog = qtrue;
		}
		else if( !strcmp( argv[i], "-nosubdivide" ) )
		{
			Sys_Printf( "nosubdivide = true\n" );
			nosubdivide = qtrue;
		}
		else if( !strcmp( argv[i], "-leaktest" ) )
		{
			Sys_Printf( "leaktest = true\n" );
			leaktest = qtrue;
		}
		else if( !strcmp( argv[i], "-nocurves" ) )
		{
			nocurves = qtrue;
			Sys_Printf( "no curve brushes\n" );
		}
		else if( !strcmp( argv[i], "-nodoors" ) )
		{
			nodoors = qtrue;
			Sys_Printf( "no door entities\n" );
		}
		else if( !strcmp( argv[i], "-notjunc" ) )
		{
			notjunc = qtrue;
			Sys_Printf( "no tjunction fixing\n" );
		}
		else if( !strcmp( argv[i], "-expand" ) )
		{
			testExpand = qtrue;
			Sys_Printf( "Writing expanded.map.\n" );
		}
		else if( !strcmp( argv[i], "-showseams" ) )
		{
			showseams = qtrue;
			Sys_Printf( "Showing seams on terrain.\n" );
		}
		else if( !strcmp( argv[i], "-tmpout" ) )
		{
			strcpy( outbase, "/tmp" );
		}
		else if( !strcmp( argv[i], "-fakemap" ) )
		{
			fakemap = qtrue;
			Sys_Printf( "will generate fakemap.map\n" );
		}
		else if( !strcmp( argv[i], "-samplesize" ) )
		{
			samplesize = atoi( argv[i + 1] );
			if( samplesize < 1 )
			{
				samplesize = 1;
			}
			i++;
			Sys_Printf( "lightmap sample size is %dx%d units\n", samplesize, samplesize );
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
		Error( "usage: xmap -map2bsp [-<switch> [-<switch> ...]] <mapname.map>\n"
			   "\n"
			   "Switches:\n"
			   "   v              = verbose output\n"
			   "   threads <X>    = set number of threads to X\n"
			   "   nocurves       = don't emit bezier surfaces\n" "   nodoors        = disable door entities\n"
			   //"   breadthfirst   = breadth first bsp building\n"
			   //"   nobrushmerge   = don't merge brushes\n"
			   "   noliquids      = don't write liquids to map\n"
			   //"   nocsg                                = disables brush chopping\n"
			   //"   glview     = output a GL view\n"
			   "   draw           = enables mini BSP viewer\n"
			   //"   noweld     = disables weld\n"
			   //"   noshare    = disables sharing\n"
			   "   notjunc        = disables juncs\n" "   nowater        = disables water brushes\n"
			   //"   noprune    = disables node prunes\n"
			   //"   nomerge    = disables face merging\n"
			   "   nofog          = disables fogs\n"
			   "   nosubdivide    = disables subdivision of draw surfaces\n"
			   "   nodetail       = disables detail brushes\n"
			   "   fulldetail     = enables full detail\n"
			   "   onlyents       = only compile entities with bsp\n"
			   "   micro <volume>\n"
			   "                  = sets the micro volume to the given float\n" "   leaktest       = perform a leak test\n"
			   //"   chop <subdivide_size>\n"
			   //"              = sets the subdivide size to the given float\n"
			   "   samplesize <N> = set the lightmap pixel size to NxN units\n" );
	}
	
	start = I_FloatTime();
	
	ThreadSetDefault();
	
	SetQdirFromPath( argv[i] );
	
	strcpy( source, ExpandArg( argv[i] ) );
	StripExtension( source );
	
	// delete portal and line files
	sprintf( path, "%s.prt", source );
	remove( path );
	sprintf( path, "%s.lin", source );
	remove( path );
	
	strcpy( name, ExpandArg( argv[i] ) );
	if( strcmp( name + strlen( name ) - 4, ".reg" ) )
	{
		// if we are doing a full map, delete the last saved region map
		sprintf( path, "%s.reg", source );
		remove( path );
		
		DefaultExtension( name, ".map" );	// might be .reg
	}
	
	// if onlyents, just grab the entites and resave
	if( onlyents )
	{
		OnlyEnts();
		
		// shut down connection
		Broadcast_Shutdown();
		
		return 0;
	}
	
	// if onlytextures, just grab the textures and resave
	if( onlytextures )
	{
		OnlyTextures();
		
		// shut down connection
		Broadcast_Shutdown();
		
		return 0;
	}
	
	// start from scratch
	LoadShaderInfo();
	
	LoadMapFile( name );
	
	ProcessModels();
	
	SetModelNumbers();
	
	EndBSPFile();
	
	end = I_FloatTime();
	Sys_Printf( "%5.0f seconds elapsed\n", end - start );
	
	// shut down connection
	Broadcast_Shutdown();
	
	return 0;
}
