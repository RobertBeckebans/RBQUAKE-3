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
// vis.h

#include "../common/cmdlib.h"
#include "../common/inout.h"
#include "../common/mathlib.h"
#include "../common/bspfile.h"

#define	MAX_PORTALS	32768

#define	PORTALFILE	"PRT1"

#define	ON_EPSILON	0.1

//#define MREDEBUG

// seperator caching helps a bit
#define SEPERATORCACHE

// can't have more seperators than the max number of points on a winding
#define MAX_SEPERATORS		64

typedef struct
{
	vec3_t          normal;
	float           dist;
} plane_t;

#define MAX_POINTS_ON_WINDING	64
#define	MAX_POINTS_ON_FIXED_WINDING	24	// ydnar: increased this from 12 at the expense of more memory

typedef struct
{
	int             numpoints;
	vec3_t          points[MAX_POINTS_ON_FIXED_WINDING];	// variable sized
} vwinding_t;

vwinding_t*     NewWinding( int points );
void            FreeWinding( vwinding_t* w );
vwinding_t*     CopyWinding( vwinding_t* w );


typedef struct passage_s
{
	struct passage_s* next;
	byte            cansee[1];	//all portals that can be seen through this passage
} passage_t;

typedef enum
{
	stat_none,
	stat_working,
	stat_done
} vstatus_t;

typedef struct
{
	int             num;
	qboolean        hint;		// true if this portal was created from a hint splitter
	qboolean        removed;
	plane_t         plane;		// normal pointing into neighbor
	int             leaf;		// neighbor
	
	vec3_t          origin;		// for fast clip testing
	float           radius;
	
	vwinding_t*     winding;
	vstatus_t       status;
	byte*           portalfront;	// [portals], preliminary
	byte*           portalflood;	// [portals], intermediate
	byte*           portalvis;	// [portals], final
	
	int             nummightsee;	// bit count on portalflood for sort
	passage_t*      passages;	// there are just as many passages as there
	// are portals in the leaf this portal leads to
} vportal_t;

#define	MAX_PORTALS_ON_LEAF		128
typedef struct leaf_s
{
	int             numportals;
	int             merged;
	vportal_t*      portals[MAX_PORTALS_ON_LEAF];
} leaf_t;


typedef struct pstack_s
{
	byte            mightsee[MAX_PORTALS / 8];	// bit string
	struct pstack_s* next;
	leaf_t*         leaf;
	vportal_t*      portal;		// portal exiting
	vwinding_t*     source;
	vwinding_t*     pass;
	
	vwinding_t      windings[3];	// source, pass, temp in any order
	int             freewindings[3];
	
	plane_t         portalplane;
	int             depth;
#ifdef SEPERATORCACHE
	plane_t         seperators[2][MAX_SEPERATORS];
	int             numseperators[2];
#endif
} pstack_t;

typedef struct
{
	vportal_t*      base;
	int             c_chains;
	pstack_t        pstack_head;
} threaddata_t;



extern int      numportals;
extern int      portalclusters;

extern vportal_t* portals;
extern leaf_t*  leafs;

extern int      c_portaltest, c_portalpass, c_portalcheck;
extern int      c_portalskip, c_leafskip;
extern int      c_vistest, c_mighttest;
extern int      c_chains;

extern byte*    vismap, *vismap_p, *vismap_end;	// past visfile

extern int      testlevel;

extern byte*    uncompressed;

extern int      leafbytes, leaflongs;
extern int      portalbytes, portallongs;


void            LeafFlow( int leafnum );


void            BasePortalVis( int portalnum );
void            BetterPortalVis( int portalnum );
void            PortalFlow( int portalnum );
void            PassagePortalFlow( int portalnum );
void            CreatePassages( int portalnum );
void            PassageFlow( int portalnum );

extern vportal_t* sorted_portals[MAX_MAP_PORTALS * 2];

int             CountBits( byte* bits, int numbits );
