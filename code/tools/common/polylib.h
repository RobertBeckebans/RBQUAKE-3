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

typedef struct
{
	int	   numpoints;
	vec3_t p[4]; // variable sized
} winding_t;

#define MAX_POINTS_ON_WINDING 64

// you can define on_epsilon in the makefile as tighter
#ifndef ON_EPSILON
	#define ON_EPSILON 0.1
#endif

winding_t* AllocWinding( int points );
vec_t	   WindingArea( winding_t* w );
void	   WindingCenter( winding_t* w, vec3_t center );
void	   ClipWindingEpsilon( winding_t* in, vec3_t normal, vec_t dist, vec_t epsilon, winding_t** front, winding_t** back );
winding_t* ChopWinding( winding_t* in, vec3_t normal, vec_t dist );
winding_t* CopyWinding( winding_t* w );
winding_t* ReverseWinding( winding_t* w );
winding_t* BaseWindingForPlane( vec3_t normal, vec_t dist );
void	   CheckWinding( winding_t* w );
void	   WindingPlane( winding_t* w, vec3_t normal, vec_t* dist );
void	   RemoveColinearPoints( winding_t* w );
int		   WindingOnPlaneSide( winding_t* w, vec3_t normal, vec_t dist );
void	   FreeWinding( winding_t* w );
void	   WindingBounds( winding_t* w, vec3_t mins, vec3_t maxs );

void	   AddWindingToConvexHull( winding_t* w, winding_t** hull, vec3_t normal );

void	   ChopWindingInPlace( winding_t** w, vec3_t normal, vec_t dist, vec_t epsilon );

// Tr3B
void	   ChopWindingByBounds( winding_t** w, vec3_t mins, vec3_t maxs, vec_t boxEpsilon );

// frees the original if clipped

void	   pw( winding_t* w );
