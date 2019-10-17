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
#include "../common/aselib.h"
#include "../lwobject/lwo2.h"


typedef struct
{
	char            modelName[1024];
	md3Header_t*    header;
} loadedModel_t;

int             c_triangleModels;
int             c_triangleSurfaces;
int             c_triangleVertexes;
int             c_triangleIndexes;


#define	MAX_LOADED_MODELS	1024
loadedModel_t   loadedModels[MAX_LOADED_MODELS];
int             numLoadedModels;



/*
=================
MD3_Load
=================
*/
#define	LL(x) x=LittleLong(x)
static md3Header_t* MD3_Load( const char* mod_name )
{
	int             i, j;
	md3Header_t*    md3;
	md3Frame_t*     frame;
	md3Surface_t*   surf;
	md3Triangle_t*  tri;
	md3St_t*        st;
	md3XyzNormal_t* xyz;
	int             version;
	char            filename[1024];
	int             len;
	
	sprintf( filename, "%s%s", gamedir, mod_name );
	len = TryLoadFile( filename, ( void** )&md3 );
	
	if( len <= 0 )
	{
		Sys_Printf( "MD3_Load: File not found '%s'\n", filename );
		return NULL;
	}
	
	version = LittleLong( md3->version );
	if( version != MD3_VERSION )
	{
		Sys_Printf( "MD3_Load: %s has wrong version (%i should be %i)\n", mod_name, version, MD3_VERSION );
		return NULL;
	}
	
	LL( md3->ident );
	LL( md3->version );
	LL( md3->numFrames );
	LL( md3->numTags );
	LL( md3->numSurfaces );
	LL( md3->numSkins );
	LL( md3->ofsFrames );
	LL( md3->ofsTags );
	LL( md3->ofsSurfaces );
	LL( md3->ofsEnd );
	
	if( md3->numFrames < 1 )
	{
		Sys_Printf( "MD3_Load: %s has no frames\n", mod_name );
		return NULL;
	}
	
	// we don't need to swap tags in the renderer, they aren't used
	
	// swap all the frames
	frame = ( md3Frame_t* )( ( byte* ) md3 + md3->ofsFrames );
	for( i = 0; i < md3->numFrames; i++, frame++ )
	{
		frame->radius = LittleFloat( frame->radius );
		for( j = 0; j < 3; j++ )
		{
			frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
			frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
			frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
		}
	}
	
	// swap all the surfaces
	surf = ( md3Surface_t* )( ( byte* ) md3 + md3->ofsSurfaces );
	for( i = 0; i < md3->numSurfaces; i++ )
	{
	
		LL( surf->ident );
		LL( surf->flags );
		LL( surf->numFrames );
		LL( surf->numShaders );
		LL( surf->numTriangles );
		LL( surf->ofsTriangles );
		LL( surf->numVerts );
		LL( surf->ofsShaders );
		LL( surf->ofsSt );
		LL( surf->ofsXyzNormals );
		LL( surf->ofsEnd );
		
		if( surf->numVerts > SHADER_MAX_VERTEXES )
		{
			Error( "MD3_Load: %s has more than %i verts on a surface (%i)", mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if( surf->numTriangles * 3 > SHADER_MAX_INDEXES )
		{
			Error( "MD3_Load: %s has more than %i triangles on a surface (%i)",
				   mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}
		
		// swap all the triangles
		tri = ( md3Triangle_t* )( ( byte* ) surf + surf->ofsTriangles );
		for( j = 0; j < surf->numTriangles; j++, tri++ )
		{
			LL( tri->indexes[0] );
			LL( tri->indexes[1] );
			LL( tri->indexes[2] );
		}
		
		// swap all the ST
		st = ( md3St_t* )( ( byte* ) surf + surf->ofsSt );
		for( j = 0; j < surf->numVerts; j++, st++ )
		{
			st->st[0] = LittleFloat( st->st[0] );
			st->st[1] = LittleFloat( st->st[1] );
		}
		
		// swap all the XyzNormals
		xyz = ( md3XyzNormal_t* )( ( byte* ) surf + surf->ofsXyzNormals );
		for( j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++ )
		{
			xyz->xyz[0] = LittleShort( xyz->xyz[0] );
			xyz->xyz[1] = LittleShort( xyz->xyz[1] );
			xyz->xyz[2] = LittleShort( xyz->xyz[2] );
			
			xyz->normal = LittleShort( xyz->normal );
		}
		
		
		// find the next surface
		surf = ( md3Surface_t* )( ( byte* ) surf + surf->ofsEnd );
	}
	
	return md3;
}


/*
================
LoadModel
================
*/
static md3Header_t* LoadModel( const char* modelName )
{
	int             i;
	loadedModel_t*  lm;
	
	// see if we already have it loaded
	for( i = 0, lm = loadedModels; i < numLoadedModels; i++, lm++ )
	{
		if( !strcmp( modelName, lm->modelName ) )
		{
			return lm->header;
		}
	}
	
	// load it
	if( numLoadedModels == MAX_LOADED_MODELS )
	{
		Error( "MAX_LOADED_MODELS" );
	}
	numLoadedModels++;
	
	strcpy( lm->modelName, modelName );
	
	lm->header = MD3_Load( modelName );
	
	return lm->header;
}

/*
============
InsertMD3Model

Convert a model entity to raw geometry surfaces and insert it in the tree
============
*/
static void InsertMD3Model( const char* modelName, const matrix_t transform )
{
	int             i, j;
	md3Header_t*    md3;
	md3Surface_t*   surf;
	md3Shader_t*    shader;
	md3Triangle_t*  tri;
	md3St_t*        st;
	md3XyzNormal_t* xyz;
	drawVert_t*     outv;
	float           lat, lng;
	drawSurface_t*  out;
	vec3_t          tmp;
	
	// load the model
	md3 = LoadModel( modelName );
	if( !md3 )
	{
		return;
	}
	
	// each md3 surface will become a new bsp surface
	
	c_triangleModels++;
	c_triangleSurfaces += md3->numSurfaces;
	
	// expand, translate, and rotate the vertexes
	// swap all the surfaces
	surf = ( md3Surface_t* )( ( byte* ) md3 + md3->ofsSurfaces );
	for( i = 0; i < md3->numSurfaces; i++ )
	{
		// allocate a surface
		out = AllocDrawSurf();
		out->miscModel = qtrue;
		
		shader = ( md3Shader_t* )( ( byte* ) surf + surf->ofsShaders );
		
		out->shaderInfo = ShaderInfoForShader( shader->name );
		
		out->numVerts = surf->numVerts;
		out->verts = malloc( out->numVerts * sizeof( out->verts[0] ) );
		
		out->numIndexes = surf->numTriangles * 3;
		out->indexes = malloc( out->numIndexes * sizeof( out->indexes[0] ) );
		
		out->lightmapNum = -1;
		out->fogNum = -1;
		
		// emit the indexes
		c_triangleIndexes += surf->numTriangles * 3;
		tri = ( md3Triangle_t* )( ( byte* ) surf + surf->ofsTriangles );
		for( j = 0; j < surf->numTriangles; j++, tri++ )
		{
			out->indexes[j * 3 + 0] = tri->indexes[0];
			out->indexes[j * 3 + 1] = tri->indexes[1];
			out->indexes[j * 3 + 2] = tri->indexes[2];
		}
		
		// emit the vertexes
		st = ( md3St_t* )( ( byte* ) surf + surf->ofsSt );
		xyz = ( md3XyzNormal_t* )( ( byte* ) surf + surf->ofsXyzNormals );
		
		c_triangleVertexes += surf->numVerts;
		for( j = 0; j < surf->numVerts; j++, st++, xyz++ )
		{
			outv = &out->verts[j];
			
			outv->st[0] = st->st[0];
			outv->st[1] = st->st[1];
			
			outv->lightmap[0] = 0;
			outv->lightmap[1] = 0;
			
			outv->paintColor[0] = 1.0f;
			outv->paintColor[1] = 1.0f;
			outv->paintColor[2] = 1.0f;
			outv->paintColor[3] = 1.0f;
			
			// the colors will be set by the lighting pass
			outv->lightColor[0] = 1.0f;
			outv->lightColor[1] = 1.0f;
			outv->lightColor[2] = 1.0f;
			outv->lightColor[3] = 1.0f;
			
			// transform the position
			tmp[0] = MD3_XYZ_SCALE * xyz->xyz[0];
			tmp[1] = MD3_XYZ_SCALE * xyz->xyz[1];
			tmp[2] = MD3_XYZ_SCALE * xyz->xyz[2];
			
			MatrixTransformPoint( transform, tmp, outv->xyz );
			
			// decode the lat/lng normal to a 3 float normal
			lat = ( xyz->normal >> 8 ) & 0xff;
			lng = ( xyz->normal & 0xff );
			lat *= Q_PI / 128;
			lng *= Q_PI / 128;
			
			tmp[0] = cos( lat ) * sin( lng );
			tmp[1] = sin( lat ) * sin( lng );
			tmp[2] = cos( lng );
			
			// rotate the normal
			MatrixTransformNormal( transform, tmp, outv->normal );
		}
		
		// find the next surface
		surf = ( md3Surface_t* )( ( byte* ) surf + surf->ofsEnd );
	}
	
}

//==============================================================================


/*
============
InsertASEModel

Convert a model entity to raw geometry surfaces and insert it in the tree
============
*/
static void InsertASEModel( const char* modelName, const matrix_t transform )
{
	int             i, j, k;
	drawSurface_t*  out;
	int             numSurfaces;
	const char*     name;
	polyset_t*      pset;
	int             numFrames;
	char            filename[1024];
	
	static int      indexes[SHADER_MAX_INDEXES];
	int             numIndexes;
	
	static drawVert_t vertexes[SHADER_MAX_VERTEXES], vertex;
	int             numVertexes;
	
	sprintf( filename, "%s%s", gamedir, modelName );
	
	// load the model
	if( !ASE_Load( filename, qfalse, qfalse ) )
	{
		return;
	}
	
	// each ase surface will become a new bsp surface
	numSurfaces = ASE_GetNumSurfaces();
	
	c_triangleModels++;
	c_triangleSurfaces += numSurfaces;
	
	// expand, translate, and rotate the vertexes
	// swap all the surfaces
	for( i = 0; i < numSurfaces; i++ )
	{
		name = ASE_GetSurfaceName( i );
		
		pset = ASE_GetSurfaceAnimation( i, &numFrames, -1, -1, -1 );
		if( !name || !pset )
		{
			continue;
		}
		
		// allocate a surface
		out = AllocDrawSurf();
		out->miscModel = qtrue;
		
		out->shaderInfo = ShaderInfoForShader( pset->materialname );
		
		// build vertex and triangle lists
		numIndexes = 0;
		numVertexes = 0;
		
		for( j = 0; j < pset->numtriangles * 3; j++ )
		{
			int             index;
			triangle_t*     tri;
			
			index = j % 3;
			tri = &pset->triangles[j / 3];
			
			vertex.st[0] = tri->texcoords[index][0];
			vertex.st[1] = tri->texcoords[index][1];
			
			vertex.lightmap[0] = 0;
			vertex.lightmap[1] = 0;
			
			vertex.xyz[0] = tri->verts[index][0];
			vertex.xyz[1] = tri->verts[index][1];
			vertex.xyz[2] = tri->verts[index][2];
			
			vertex.normal[0] = tri->normals[index][0];
			vertex.normal[1] = tri->normals[index][1];
			vertex.normal[2] = tri->normals[index][2];
			
			vertex.paintColor[0] = ( tri->colors[index][0] );
			vertex.paintColor[1] = ( tri->colors[index][1] );
			vertex.paintColor[2] = ( tri->colors[index][2] );
			vertex.paintColor[3] = 1.0f;
			
			
			vertex.lightColor[0] = 1.0f;
			vertex.lightColor[1] = 1.0f;
			vertex.lightColor[2] = 1.0f;
			vertex.lightColor[3] = 1.0f;
			
			// add it to the vertex list if not added yet
			if( numIndexes == SHADER_MAX_INDEXES )
			{
				Sys_Printf( "SHADER_MAX_INDEXES hit\n" );
				return;
			}
			
			for( k = 0; k < numVertexes; k++ )
			{
				if( vertexes[k].xyz[0] != vertex.xyz[0] || vertexes[k].xyz[1] != vertex.xyz[k] ||
						vertexes[k].xyz[2] != vertex.xyz[2] )
				{
					continue;
				}
				
				if( vertexes[k].st[0] != vertex.st[0] || vertexes[k].st[1] != vertex.st[1] )
				{
					continue;
				}
				
				break;
			}
			
			if( k == numVertexes )
			{
				if( numVertexes == SHADER_MAX_VERTEXES )
				{
					Sys_Printf( "SHADER_MAX_VERTEXES hit\n" );
					return;
				}
				
				indexes[numIndexes++] = numVertexes;
				vertexes[numVertexes++] = vertex;
			}
			else
			{
				indexes[numIndexes++] = k;
			}
		}
		
		// emit the indexes
		out->numIndexes = numIndexes;
		out->indexes = malloc( out->numIndexes * sizeof( out->indexes[0] ) );
		
		c_triangleIndexes += numIndexes;
		
		for( j = 0; j < numIndexes; j += 3 )
		{
			out->indexes[j + 0] = indexes[j + 0];
			out->indexes[j + 1] = indexes[j + 1];
			out->indexes[j + 2] = indexes[j + 2];
		}
		
		// emit the vertexes
		out->numVerts = numVertexes;
		out->verts = malloc( out->numVerts * sizeof( out->verts[0] ) );
		
		c_triangleVertexes += numVertexes;
		
		for( j = 0; j < numVertexes; j++ )
		{
			// transform the position
			MatrixTransformPoint( transform, vertexes[j].xyz, out->verts[j].xyz );
			
			// rotate the normal
			MatrixTransformNormal( transform, vertexes[j].normal, out->verts[j].normal );
			
			out->verts[j].st[0] = vertexes[j].st[0];
			out->verts[j].st[1] = vertexes[j].st[1];
			
			out->verts[j].lightmap[0] = vertexes[j].lightmap[0];
			out->verts[j].lightmap[1] = vertexes[j].lightmap[1];
			
			out->verts[j].lightColor[0] = 1.0f;
			out->verts[j].lightColor[1] = 1.0f;
			out->verts[j].lightColor[2] = 1.0f;
			out->verts[j].lightColor[3] = 1.0f;
			
			out->verts[j].paintColor[0] = vertexes[j].paintColor[0];
			out->verts[j].paintColor[1] = vertexes[j].paintColor[1];
			out->verts[j].paintColor[2] = vertexes[j].paintColor[2];
			out->verts[j].paintColor[3] = vertexes[j].paintColor[3];
		}
	}
}



/*
============
InsertLWOModel

Convert a LWO model entity to raw geometry surfaces and insert it in the tree
============
*/
static void InsertLWOModel( const char* modelName, const matrix_t transform )
{
	int             i, j, k, l;
	char            filename[1024];
	drawSurface_t*  out;
	
//  drawVert_t     *outv;
	vec2_t          st;
	int             defaultSTAxis[2];
	vec2_t          defaultXYZtoSTScale;
	
	unsigned int    failID;
	int             failpos;
	lwObject*       obj;
	lwLayer*        layer;
	lwSurface*      surf;
	lwPolygon*      pol;
	lwPolVert*      v;
	lwPoint*        pt;
	lwVMap*         vmap;
	
	static int      indexes[SHADER_MAX_INDEXES];
	int             numIndexes;
	
	static drawVert_t vertexes[SHADER_MAX_VERTEXES], vertex;
	int             numVertexes;
	
	sprintf( filename, "%s%s", gamedir, modelName );
	
	// load the model
	obj = lwGetObject( filename, &failID, &failpos );
	if( !obj )
	{
		Sys_Printf( "%s\nLoading failed near byte %d\n\n", filename, failpos );
		return;
	}
	
	Sys_Printf( "Processing '%s'\n", filename );
	
	if( obj->nlayers != 1 )
	{
		Error( "..layers number %i != 1", obj->nlayers );
	}
	
#if 0
	Sys_FPrintf( SYS_VRB, "Layers:  %d\n"
				 "Surfaces:  %d\n"
				 "Envelopes:  %d\n"
				 "Clips:  %d\n"
				 "Points (first layer):  %d\n"
				 "Polygons (first layer):  %d\n\n",
				 obj->nlayers, obj->nsurfs, obj->nenvs, obj->nclips, obj->layer->point.count, obj->layer->polygon.count );
#endif
				 
	// create all polygons from layer[ 0 ] that belong to this surface
	layer = &obj->layer[0];
	
	// setup default st map
	st[0] = st[1] = 0.0f;		// st[0] holds max, st[1] holds max par one
	defaultSTAxis[0] = 0;
	defaultSTAxis[1] = 1;
	for( i = 0; i < 3; i++ )
	{
		float           min = layer->bbox[i];
		float           max = layer->bbox[i + 3];
		float           size = max - min;
		
		if( size > st[0] )
		{
			defaultSTAxis[1] = defaultSTAxis[0];
			defaultSTAxis[0] = i;
			
			st[1] = st[0];
			st[0] = size;
		}
		else if( size > st[1] )
		{
			defaultSTAxis[1] = i;
			st[1] = size;
		}
	}
	defaultXYZtoSTScale[0] = 4.f / st[0];
	defaultXYZtoSTScale[1] = 4.f / st[1];
	
	c_triangleModels++;
	c_triangleSurfaces += obj->nsurfs;
	
	// each LWO surface from the first layer will become a new bsp surface
	for( i = 0, surf = obj->surf; i < obj->nsurfs; i++, surf = surf->next )
	{
		// allocate a surface
		out = AllocDrawSurf();
		out->miscModel = qtrue;
		
		out->shaderInfo = ShaderInfoForShader( surf->name );
		
		out->lightmapNum = -1;
		out->fogNum = -1;
		
		// Copy vertices
		numIndexes = 0;
		numVertexes = 0;
		
		// count polygons which we want to use
		out->numIndexes = 0;
		for( j = 0; j < layer->polygon.count; j++ )
		{
			pol = &layer->polygon.pol[j];
			
			// skip all polygons that don't belong to this surface
			if( pol->surf != surf )
			{
				continue;
			}
			
			// only accept FACE surfaces
			if( pol->type != ID_FACE )
			{
				Sys_Printf( "WARNING: skipping ID_FACE polygon\n" );
				continue;
			}
			
			// only accept triangulated surfaces
			if( pol->nverts != 3 )
			{
				Sys_Printf( "WARNING: skipping non triangulated polygon\n" );
				continue;
			}
			
			for( k = 0, v = pol->v; k < pol->nverts; k++, v++ )
			{
				int             index;
				
				index = v->index;
				
				pt = &layer->point.pt[index];
				
				vertex.xyz[0] = pt->pos[0];
				vertex.xyz[1] = pt->pos[2];
				vertex.xyz[2] = pt->pos[1];
				
				vertex.st[0] = vertex.xyz[defaultSTAxis[0]] * defaultXYZtoSTScale[0];
				vertex.st[1] = vertex.xyz[defaultSTAxis[1]] * defaultXYZtoSTScale[1];
				
				vertex.lightmap[0] = 0;
				vertex.lightmap[1] = 0;
				
				vertex.paintColor[0] = surf->color.rgb[0] * surf->diffuse.val;
				vertex.paintColor[1] = surf->color.rgb[1] * surf->diffuse.val;
				vertex.paintColor[2] = surf->color.rgb[2] * surf->diffuse.val;
				vertex.paintColor[3] = 1.0f;
				
				// set dummy normal
				vertex.normal[0] = 0;
				vertex.normal[1] = 0;
				vertex.normal[2] = 1;
				
				// fetch base from points
				for( l = 0; l < pt->nvmaps; l++ )
				{
					vmap = pt->vm[l].vmap;
					index = pt->vm[l].index;
					
					if( vmap->type == LWID_( 'T', 'X', 'U', 'V' ) )
					{
						vertex.st[0] = vmap->val[index][0];
						vertex.st[1] = 1.0 - vmap->val[index][1];
					}
					
					if( vmap->type == LWID_( 'R', 'G', 'B', 'A' ) )
					{
						vertex.paintColor[0] = vmap->val[index][0] * surf->color.rgb[0] * surf->diffuse.val;
						vertex.paintColor[1] = vmap->val[index][1] * surf->color.rgb[1] * surf->diffuse.val;
						vertex.paintColor[2] = vmap->val[index][2] * surf->color.rgb[2] * surf->diffuse.val;
						vertex.paintColor[3] = vmap->val[index][3];
					}
				}
				
				// override with polyon data
				for( l = 0; l < v->nvmaps; l++ )
				{
					vmap = v->vm[l].vmap;
					index = v->vm[l].index;
					
					if( vmap->type == LWID_( 'T', 'X', 'U', 'V' ) )
					{
						vertex.st[0] = vmap->val[index][0];
						vertex.st[1] = 1.0 - vmap->val[index][1];
					}
					
					if( vmap->type == LWID_( 'R', 'G', 'B', 'A' ) )
					{
						vertex.paintColor[0] = vmap->val[index][0] * surf->color.rgb[0] * surf->diffuse.val;
						vertex.paintColor[1] = vmap->val[index][1] * surf->color.rgb[1] * surf->diffuse.val;
						vertex.paintColor[2] = vmap->val[index][2] * surf->color.rgb[2] * surf->diffuse.val;
						vertex.paintColor[3] = vmap->val[index][3];
					}
				}
				
				// Add it to the vertex list if not added yet
				if( numIndexes == SHADER_MAX_INDEXES )
				{
					Sys_Printf( "SHADER_MAX_INDEXES hit\n" );
					return;
				}
				
				for( l = 0; l < numVertexes; l++ )
				{
					if( vertexes[l].xyz[0] != vertex.xyz[0] || vertexes[l].xyz[1] != vertex.xyz[1] ||
							vertexes[l].xyz[2] != vertex.xyz[2] )
					{
						continue;
					}
					
					if( vertexes[l].st[0] != vertex.st[0] || vertexes[l].st[1] != vertex.st[1] )
					{
						continue;
					}
					
					break;
				}
				
				if( l == numVertexes )
				{
					if( numVertexes == SHADER_MAX_VERTEXES )
					{
						Sys_Printf( "SHADER_MAX_VERTEXES hit\n" );
						return;
					}
					
					indexes[numIndexes++] = numVertexes;
					vertexes[numVertexes++] = vertex;
				}
				else
				{
					indexes[numIndexes++] = l;
				}
				
			}
		}
		
		// emit the indexes
		out->numIndexes = numIndexes;
		out->indexes = malloc( out->numIndexes * sizeof( out->indexes[0] ) );
		
		c_triangleIndexes += numIndexes;
		
		for( j = 0; j < numIndexes; j += 3 )
		{
			out->indexes[j + 0] = indexes[j + 0];
			out->indexes[j + 1] = indexes[j + 1];
			out->indexes[j + 2] = indexes[j + 2];
		}
		
		// emit the vertexes
		out->numVerts = numVertexes;
		out->verts = malloc( out->numVerts * sizeof( out->verts[0] ) );
		
		c_triangleVertexes += numVertexes;
		
		for( j = 0; j < numVertexes; j++ )
		{
			// transform the position
			MatrixTransformPoint( transform, vertexes[j].xyz, out->verts[j].xyz );
			
			// rotate the normal
			MatrixTransformNormal( transform, vertexes[j].normal, out->verts[j].normal );
			
			out->verts[j].st[0] = vertexes[j].st[0];
			out->verts[j].st[1] = vertexes[j].st[1];
			
			out->verts[j].lightmap[0] = vertexes[j].lightmap[0];
			out->verts[j].lightmap[1] = vertexes[j].lightmap[1];
			
			out->verts[j].paintColor[0] = vertexes[j].paintColor[0];
			out->verts[j].paintColor[1] = vertexes[j].paintColor[1];
			out->verts[j].paintColor[2] = vertexes[j].paintColor[2];
			out->verts[j].paintColor[3] = vertexes[j].paintColor[3];
			
			out->verts[j].lightColor[0] = 1.0f;
			out->verts[j].lightColor[1] = 1.0f;
			out->verts[j].lightColor[2] = 1.0f;
			out->verts[j].lightColor[3] = 1.0f;
		}
	}
	
	lwFreeObject( obj );
}


//==============================================================================


/*
=====================
AddTriangleModel
=====================
*/
void AddTriangleModel( entity_t* entity, qboolean applyTransform )
{
	const char*     classname;
	const char*     model;
	const char*     value;
	vec3_t          origin;
	vec3_t          angles;
	matrix_t        rotation;
	matrix_t        transform;
	
	Sys_FPrintf( SYS_VRB, "----- AddTriangleModel -----\n" );
	
	MatrixIdentity( transform );
	
	if( applyTransform )
	{
		// make transform from local model space to global world space
		MatrixIdentity( rotation );
		
		GetVectorForKey( entity, "origin", origin );
		
		// get rotation matrix or "angle" (yaw) or "angles" (pitch yaw roll)
		angles[0] = angles[1] = angles[2] = 0;
		value = ValueForKey( entity, "angle" );
		if( value[0] != '\0' )
		{
			angles[1] = atof( value );
			MatrixFromAngles( rotation, angles[PITCH], angles[YAW], angles[ROLL] );
		}
		
		value = ValueForKey( entity, "angles" );
		if( value[0] != '\0' )
		{
			sscanf( value, "%f %f %f", &angles[0], &angles[1], &angles[2] );
			MatrixFromAngles( rotation, angles[PITCH], angles[YAW], angles[ROLL] );
		}
		
		value = ValueForKey( entity, "rotation" );
		if( value[0] != '\0' )
		{
			sscanf( value, "%f %f %f %f %f %f %f %f %f", &rotation[0], &rotation[1], &rotation[2],
					&rotation[4], &rotation[5], &rotation[6], &rotation[8], &rotation[9], &rotation[10] );
		}
		
		MatrixSetupTransformFromRotation( transform, rotation, origin );
	}
	
	classname = ValueForKey( entity, "classname" );
	model = ValueForKey( entity, "model" );
	if( !model[0] )
	{
		Sys_Printf( "WARNING: '%s' at %i %i %i without a model key\n", classname, ( int )origin[0], ( int )origin[1], ( int )origin[2] );
		return;
	}
	
	if( strstr( model, ".md3" ) || strstr( model, ".MD3" ) )
	{
		InsertMD3Model( model, transform );
	}
	else if( strstr( model, ".ase" ) || strstr( model, ".ASE" ) )
	{
		InsertASEModel( model, transform );
	}
	else if( strstr( model, ".lwo" ) || strstr( model, ".LWO" ) )
	{
		InsertLWOModel( model, transform );
	}
	else
	{
		Sys_Printf( "Unknown model type: %s\n", model );
	}
}



/*
=====================
AddTriangleModels
=====================
*/
void AddTriangleModels( void )
{
	int             i;
	entity_t*       entity;
	
	Sys_FPrintf( SYS_VRB, "----- AddTriangleModels -----\n" );
	
	for( i = 1; i < numEntities; i++ )
	{
		entity = &entities[i];
		
		// convert misc_models into raw geometry
		if( !Q_stricmp( "misc_model", ValueForKey( entity, "classname" ) ) )
		{
			AddTriangleModel( entity, qtrue );
		}
	}
	
	Sys_FPrintf( SYS_VRB, "%5i triangle models\n", c_triangleModels );
	Sys_FPrintf( SYS_VRB, "%5i triangle surfaces\n", c_triangleSurfaces );
	Sys_FPrintf( SYS_VRB, "%5i triangle vertexes\n", c_triangleVertexes );
	Sys_FPrintf( SYS_VRB, "%5i triangle indexes\n", c_triangleIndexes );
}
