/* -------------------------------------------------------------------------------

Copyright (C) 1999-2007 id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

----------------------------------------------------------------------------------

This code has been altered significantly from its original form, to support
several games based on the Quake III Arena engine, in the form of "Q3Map2."

------------------------------------------------------------------------------- */

/* marker */
#define PRTFILE_C

/* dependencies */
#include "q3map2.h"

/*
==============================================================================

PORTAL FILE GENERATION

Save out name.prt for qvis to read
==============================================================================
*/

#define PORTALFILE "PRT1"

FILE* pf;
int	  num_visclusters; // clusters the player can be in
int	  num_visportals;
int	  num_solidfaces;

void  WriteFloat( FILE* f, vec_t v )
{
	if( fabs( v - Q_rint( v ) ) < 0.001 )
	{
		fprintf( f, "%i ", ( int )Q_rint( v ) );
	}
	else
	{
		fprintf( f, "%f ", v );
	}
}

void CountVisportals_r( node_t* node )
{
	int		   s;
	portal_t*  p;
	winding_t* w;

	// decision node
	if( node->planenum != PLANENUM_LEAF )
	{
		CountVisportals_r( node->children[0] );
		CountVisportals_r( node->children[1] );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	for( p = node->portals; p; p = p->next[s] )
	{
		w = p->winding;
		s = ( p->nodes[1] == node );
		if( w && p->nodes[0] == node )
		{
			if( !PortalPassable( p ) )
			{
				continue;
			}
			if( p->nodes[0]->cluster == p->nodes[1]->cluster )
			{
				continue;
			}
			++num_visportals;
		}
	}
}

/*
=================
WritePortalFile_r
=================
*/
void WritePortalFile_r( node_t* node )
{
	int		   i, s;
	portal_t*  p;
	winding_t* w;
	vec3_t	   normal;
	vec_t	   dist;

	// decision node
	if( node->planenum != PLANENUM_LEAF )
	{
		WritePortalFile_r( node->children[0] );
		WritePortalFile_r( node->children[1] );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	for( p = node->portals; p; p = p->next[s] )
	{
		w = p->winding;
		s = ( p->nodes[1] == node );
		if( w && p->nodes[0] == node )
		{
			if( !PortalPassable( p ) )
			{
				continue;
			}
			if( p->nodes[0]->cluster == p->nodes[1]->cluster )
			{
				continue;
			}
			--num_visportals;
			// write out to the file

			// sometimes planes get turned around when they are very near
			// the changeover point between different axis.  interpret the
			// plane the same way vis will, and flip the side orders if needed
			// FIXME: is this still relevent?
			WindingPlane( w, normal, &dist );

			if( DotProduct( p->plane.normal, normal ) < 0.99 )
			{
				// backwards...
				fprintf( pf, "%i %i %i ", w->numpoints, p->nodes[1]->cluster, p->nodes[0]->cluster );
			}
			else
			{
				fprintf( pf, "%i %i %i ", w->numpoints, p->nodes[0]->cluster, p->nodes[1]->cluster );
			}

			/* ydnar: added this change to make antiportals work */
			if( p->compileFlags & C_HINT )
			{
				fprintf( pf, "1 " );
			}
			else
			{
				fprintf( pf, "0 " );
			}

			/* write the winding */
			for( i = 0; i < w->numpoints; i++ )
			{
				fprintf( pf, "(" );
				WriteFloat( pf, w->p[i][0] );
				WriteFloat( pf, w->p[i][1] );
				WriteFloat( pf, w->p[i][2] );
				fprintf( pf, ") " );
			}
			fprintf( pf, "\n" );
		}
	}
}

void CountSolidFaces_r( node_t* node )
{
	int		   s;
	portal_t*  p;
	winding_t* w;

	// decision node
	if( node->planenum != PLANENUM_LEAF )
	{
		CountSolidFaces_r( node->children[0] );
		CountSolidFaces_r( node->children[1] );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	for( p = node->portals; p; p = p->next[s] )
	{
		w = p->winding;
		s = ( p->nodes[1] == node );
		if( w )
		{
			if( PortalPassable( p ) )
			{
				continue;
			}
			if( p->nodes[0]->cluster == p->nodes[1]->cluster )
			{
				continue;
			}
			// write out to the file

			++num_solidfaces;
		}
	}
}

/*
=================
WriteFaceFile_r
=================
*/
void WriteFaceFile_r( node_t* node )
{
	int		   i, s;
	portal_t*  p;
	winding_t* w;

	// decision node
	if( node->planenum != PLANENUM_LEAF )
	{
		WriteFaceFile_r( node->children[0] );
		WriteFaceFile_r( node->children[1] );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	for( p = node->portals; p; p = p->next[s] )
	{
		w = p->winding;
		s = ( p->nodes[1] == node );
		if( w )
		{
			if( PortalPassable( p ) )
			{
				continue;
			}
			if( p->nodes[0]->cluster == p->nodes[1]->cluster )
			{
				continue;
			}
			// write out to the file

			if( p->nodes[0] == node )
			{
				fprintf( pf, "%i %i ", w->numpoints, p->nodes[0]->cluster );
				for( i = 0; i < w->numpoints; i++ )
				{
					fprintf( pf, "(" );
					WriteFloat( pf, w->p[i][0] );
					WriteFloat( pf, w->p[i][1] );
					WriteFloat( pf, w->p[i][2] );
					fprintf( pf, ") " );
				}
				fprintf( pf, "\n" );
			}
			else
			{
				fprintf( pf, "%i %i ", w->numpoints, p->nodes[1]->cluster );
				for( i = w->numpoints - 1; i >= 0; i-- )
				{
					fprintf( pf, "(" );
					WriteFloat( pf, w->p[i][0] );
					WriteFloat( pf, w->p[i][1] );
					WriteFloat( pf, w->p[i][2] );
					fprintf( pf, ") " );
				}
				fprintf( pf, "\n" );
			}
		}
	}
}

/*
================
NumberLeafs_r
================
*/
void NumberLeafs_r( node_t* node, int c )
{
#if 0
	portal_t*       p;
#endif
	if( node->planenum != PLANENUM_LEAF )
	{
		// decision node
		node->cluster = -99;

		if( node->has_structural_children )
		{
#if 0
			if( c >= 0 )
			{
				Sys_FPrintf( SYS_ERR, "THIS CANNOT HAPPEN\n" );
			}
#endif
			NumberLeafs_r( node->children[0], c );
			NumberLeafs_r( node->children[1], c );
		}
		else
		{
			if( c < 0 )
			{
				c = num_visclusters++;
			}
			NumberLeafs_r( node->children[0], c );
			NumberLeafs_r( node->children[1], c );
		}
		return;
	}

	node->area = -1;

	if( node->opaque )
	{
		// solid block, viewpoint never inside
		node->cluster = -1;
		return;
	}

	if( c < 0 )
	{
		c = num_visclusters++;
	}

	node->cluster = c;

#if 0
	// count the portals
	for( p = node->portals; p; )
	{
		if( p->nodes[0] == node )	// only write out from first leaf
		{
			if( PortalPassable( p ) )
			{
				num_visportals++;
			}
			else
			{
				num_solidfaces++;
			}
			p = p->next[0];
		}
		else
		{
			if( !PortalPassable( p ) )
			{
				num_solidfaces++;
			}
			p = p->next[1];
		}
	}
#endif
}

/*
================
NumberClusters
================
*/
void NumberClusters( tree_t* tree )
{
	num_visclusters = 0;
	num_visportals	= 0;
	num_solidfaces	= 0;

	Sys_FPrintf( SYS_VRB, "--- NumberClusters ---\n" );

	// set the cluster field in every leaf and count the total number of portals
	NumberLeafs_r( tree->headnode, -1 );
	CountVisportals_r( tree->headnode );
	CountSolidFaces_r( tree->headnode );

	Sys_FPrintf( SYS_VRB, "%9d visclusters\n", num_visclusters );
	Sys_FPrintf( SYS_VRB, "%9d visportals\n", num_visportals );
	Sys_FPrintf( SYS_VRB, "%9d solidfaces\n", num_solidfaces );
}

/*
================
WritePortalFile
================
*/
void WritePortalFile( tree_t* tree )
{
	char filename[1024];

	Sys_FPrintf( SYS_VRB, "--- WritePortalFile ---\n" );

	// write the file
	sprintf( filename, "%s.prt", source );
	Sys_Printf( "writing %s\n", filename );
	pf = fopen( filename, "w" );
	if( !pf )
	{
		Error( "Error opening %s", filename );
	}

	fprintf( pf, "%s\n", PORTALFILE );
	fprintf( pf, "%i\n", num_visclusters );
	fprintf( pf, "%i\n", num_visportals );
	fprintf( pf, "%i\n", num_solidfaces );

	WritePortalFile_r( tree->headnode );
	WriteFaceFile_r( tree->headnode );

	fclose( pf );
}
