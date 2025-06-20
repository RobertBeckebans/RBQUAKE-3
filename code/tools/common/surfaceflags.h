/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
//
// This file must be identical in the quake and utils directories

// contents flags are seperate bits
// a given brush can contribute multiple content bits

// these definitions also need to be in q_shared.h!

#define CONTENTS_SOLID			  1 // an eye is never valid in a solid
#define CONTENTS_LAVA			  8
#define CONTENTS_SLIME			  16
#define CONTENTS_WATER			  32
#define CONTENTS_FOG			  64

#define CONTENTS_NOTTEAM1		  0x0080
#define CONTENTS_NOTTEAM2		  0x0100
#define CONTENTS_NOBOTCLIP		  0x0200

#define CONTENTS_SHOOTABLE		  0x0400 // Tr3B: used for game entities that can be destroyed like railgun spheres

#define CONTENTS_AREAPORTAL		  0x8000

#define CONTENTS_PLAYERCLIP		  0x10000
#define CONTENTS_MONSTERCLIP	  0x20000

// bot specific contents types
#define CONTENTS_TELEPORTER		  0x40000
#define CONTENTS_JUMPPAD		  0x80000
#define CONTENTS_CLUSTERPORTAL	  0x100000
#define CONTENTS_DONOTENTER		  0x200000
#define CONTENTS_BOTCLIP		  0x400000
#define CONTENTS_MOVER			  0x800000

#define CONTENTS_ORIGIN			  0x1000000 // removed before bsping an entity

#define CONTENTS_BODY			  0x2000000 // should never be on a brush, only in game
#define CONTENTS_CORPSE			  0x4000000
#define CONTENTS_DETAIL			  0x8000000	 // brushes not used for the bsp
#define CONTENTS_STRUCTURAL		  0x10000000 // brushes used for the bsp
#define CONTENTS_TRANSLUCENT	  0x20000000 // don't consume surface fragments inside
#define CONTENTS_TRIGGER		  0x40000000
#define CONTENTS_NODROP			  0x80000000 // don't leave bodies or items (death fog, lava)

#define SURF_NODAMAGE			  0x1 // never give falling damage
#define SURF_SLICK				  0x2 // effects game physics
#define SURF_SKY				  0x4 // lighting from environment map
#define SURF_LADDER				  0x8
#define SURF_NOIMPACT			  0x10	  // don't make missile explosions
#define SURF_NOMARKS			  0x20	  // don't leave missile marks
#define SURF_FLESH				  0x40	  // make flesh sounds and effects
#define SURF_NODRAW				  0x80	  // don't generate a drawsurface at all
#define SURF_HINT				  0x100	  // make a primary bsp splitter
#define SURF_SKIP				  0x200	  // completely ignore, allowing non-closed brushes
#define SURF_NOLIGHTMAP			  0x400	  // surface doesn't need a lightmap
#define SURF_POINTLIGHT			  0x800	  // generate lighting info at vertexes
#define SURF_METALSTEPS			  0x1000  // clanking footsteps
#define SURF_NOSTEPS			  0x2000  // no footstep sounds
#define SURF_NONSOLID			  0x4000  // don't collide against curves with this set
#define SURF_LIGHTFILTER		  0x8000  // act as a light filter during xmap -light
#define SURF_ALPHASHADOW		  0x10000 // do per-pixel light shadow casting in xmap
#define SURF_COLLISION			  0x20000 // don't draw but use for per polygon collision detection
#define SURF_DUST				  0x40000 // leave a dust trail when walking on this surface

/* ydnar flags */

#define CONTENTS_OPAQUE			  0x02
#define CONTENTS_LIGHTGRID		  0x04

#define SURF_VERTEXLIT			  ( SURF_POINTLIGHT | SURF_NOLIGHTMAP )

/* wolfenstein flags (collisions with valid q3a flags are noted) */

#define CONTENTS_MISSILECLIP	  0x80
#define CONTENTS_ITEM			  0x100
#define CONTENTS_AI_NOSIGHT		  0x1000
#define CONTENTS_CLIPSHOT		  0x2000
#define CONTENTS_DONOTENTER_LARGE 0x400000 /* CONTENTS_BOTCLIP */

#define SURF_CERAMIC			  0x40	  /* SURF_FLESH */
#define SURF_METAL				  0x1000  /* SURF_METALSTEPS */
#define SURF_WOOD				  0x40000 /* SURF_DUST */
#define SURF_GRASS				  0x80000
#define SURF_GRAVEL				  0x100000
#define SURF_GLASS				  0x200000
#define SURF_SNOW				  0x400000
#define SURF_ROOF				  0x800000
#define SURF_RUBBLE				  0x1000000
#define SURF_CARPET				  0x2000000
#define SURF_MONSTERSLICK		  0x4000000
#define SURF_MONSLICK_W			  0x8000000
#define SURF_MONSLICK_N			  0x10000000
#define SURF_MONSLICK_E			  0x20000000
#define SURF_MONSLICK_S			  0x40000000
