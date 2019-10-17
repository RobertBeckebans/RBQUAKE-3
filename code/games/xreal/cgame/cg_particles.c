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
// cg_particles.c

#include "cg_local.h"


#define PARTICLE_GRAVITY	40
#define	MAX_PARTICLES		1024 * 8


static cparticle_t* cg_activeParticles, *cg_freeParticles;
static cparticle_t cg_allParticles[MAX_PARTICLES];


static qboolean initparticles = qfalse;
static vec3_t   vforward, vright, vup;
static vec3_t   rforward, rright, rup;

static float    oldtime;



/*
===============
CG_InitParticles
===============
*/
void CG_InitParticles( void )
{
	int             i;
	
	memset( cg_allParticles, 0, sizeof( cg_allParticles ) );
	
	cg_freeParticles = &cg_allParticles[0];
	cg_activeParticles = NULL;
	
	for( i = 0; i < MAX_PARTICLES; i++ )
	{
		cg_allParticles[i].next = &cg_allParticles[i + 1];
		cg_allParticles[i].type = P_NONE;
	}
	cg_allParticles[MAX_PARTICLES - 1].next = NULL;
	
	oldtime = cg.time;
	
	initparticles = qtrue;
}

/*
==========================
CG_AllocParticle
==========================
*/
cparticle_t*    CG_AllocParticle()
{
	cparticle_t*    p;
	
	if( !cg_particles.integer || !cg_freeParticles )
	{
		return NULL;
	}
	
	p = cg_freeParticles;
	cg_freeParticles = p->next;
	p->next = cg_activeParticles;
	cg_activeParticles = p;
	
	// add some nice default values
	p->flags = 0;
	p->time = cg.time;
	p->startfade = cg.time;
	p->bounceFactor = 0.0f;
	
	p->color[0] = 1;
	p->color[1] = 1;
	p->color[2] = 1;
	p->color[3] = 1;
	
	p->colorVel[0] = 0;
	p->colorVel[1] = 0;
	p->colorVel[2] = 0;
	p->colorVel[3] = 0;
	
	return p;
}

/*
==========================
CG_FreeParticle
==========================
*/
void CG_FreeParticle( cparticle_t* p )
{
	p->next = cg_freeParticles;
	cg_freeParticles = p;
	
	p->type = P_NONE;
}


/*
=====================
CG_AddParticleToScene
=====================
*/
void CG_AddParticleToScene( cparticle_t* p, vec3_t org, vec4_t color )
{
	vec3_t          point;
	polyVert_t      verts[4];
	float           width;
	float           height;
	float           time, time2;
	float           ratio;
	float           invratio;
	polyVert_t      TRIverts[3];
	vec3_t          rright2, rup2;
	vec3_t          oldOrigin;
	
	if( p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY
			|| p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT )
	{
		// create a front facing polygon
		if( p->type != P_WEATHER_FLURRY )
		{
			if( p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT )
			{
				if( org[2] > p->end )
				{
					p->time = cg.time;
					VectorCopy( org, p->org );	// Ridah, fixes rare snow flakes that flicker on the ground
					
					p->org[2] = ( p->start + crandom() * 4 );
					
					
					if( p->type == P_BUBBLE_TURBULENT )
					{
						p->vel[0] = crandom() * 4;
						p->vel[1] = crandom() * 4;
					}
					
				}
			}
			else
			{
				if( org[2] < p->end )
				{
					p->time = cg.time;
					VectorCopy( org, p->org );	// Ridah, fixes rare snow flakes that flicker on the ground
					
					while( p->org[2] < p->end )
					{
						p->org[2] += ( p->start - p->end );
					}
					
					
					if( p->type == P_WEATHER_TURBULENT )
					{
						p->vel[0] = crandom() * 16;
						p->vel[1] = crandom() * 16;
					}
					
				}
			}
			
			// Rafael snow pvs check
			if( !p->link )
			{
				return;
			}
			
			color[3] = 1;
		}
		
		// Ridah, had to do this or MAX_POLYS is being exceeded in village1.bsp
		if( Distance( cg.snap->ps.origin, org ) > 1024 )
		{
			return;
		}
		
		if( p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT )
		{
			VectorMA( org, -p->height, vup, point );
			VectorMA( point, -p->width, vright, point );
			VectorCopy( point, verts[0].xyz );
			verts[0].st[0] = 0;
			verts[0].st[1] = 0;
			verts[0].modulate[0] = 255;
			verts[0].modulate[1] = 255;
			verts[0].modulate[2] = 255;
			verts[0].modulate[3] = 255 * color[3];
			
			VectorMA( org, -p->height, vup, point );
			VectorMA( point, p->width, vright, point );
			VectorCopy( point, verts[1].xyz );
			verts[1].st[0] = 0;
			verts[1].st[1] = 1;
			verts[1].modulate[0] = 255;
			verts[1].modulate[1] = 255;
			verts[1].modulate[2] = 255;
			verts[1].modulate[3] = 255 * color[3];
			
			VectorMA( org, p->height, vup, point );
			VectorMA( point, p->width, vright, point );
			VectorCopy( point, verts[2].xyz );
			verts[2].st[0] = 1;
			verts[2].st[1] = 1;
			verts[2].modulate[0] = 255;
			verts[2].modulate[1] = 255;
			verts[2].modulate[2] = 255;
			verts[2].modulate[3] = 255 * color[3];
			
			VectorMA( org, p->height, vup, point );
			VectorMA( point, -p->width, vright, point );
			VectorCopy( point, verts[3].xyz );
			verts[3].st[0] = 1;
			verts[3].st[1] = 0;
			verts[3].modulate[0] = 255;
			verts[3].modulate[1] = 255;
			verts[3].modulate[2] = 255;
			verts[3].modulate[3] = 255 * color[3];
		}
		else
		{
			VectorMA( org, -p->height, vup, point );
			VectorMA( point, -p->width, vright, point );
			VectorCopy( point, TRIverts[0].xyz );
			TRIverts[0].st[0] = 1;
			TRIverts[0].st[1] = 0;
			TRIverts[0].modulate[0] = 255;
			TRIverts[0].modulate[1] = 255;
			TRIverts[0].modulate[2] = 255;
			TRIverts[0].modulate[3] = 255 * color[3];
			
			VectorMA( org, p->height, vup, point );
			VectorMA( point, -p->width, vright, point );
			VectorCopy( point, TRIverts[1].xyz );
			TRIverts[1].st[0] = 0;
			TRIverts[1].st[1] = 0;
			TRIverts[1].modulate[0] = 255;
			TRIverts[1].modulate[1] = 255;
			TRIverts[1].modulate[2] = 255;
			TRIverts[1].modulate[3] = 255 * color[3];
			
			VectorMA( org, p->height, vup, point );
			VectorMA( point, p->width, vright, point );
			VectorCopy( point, TRIverts[2].xyz );
			TRIverts[2].st[0] = 0;
			TRIverts[2].st[1] = 1;
			TRIverts[2].modulate[0] = 255;
			TRIverts[2].modulate[1] = 255;
			TRIverts[2].modulate[2] = 255;
			TRIverts[2].modulate[3] = 255 * color[3];
		}
		
	}
	else if( p->type == P_SPRITE )
	{
		vec3_t          rr, ru;
		vec3_t          rotate_ang;
		
		VectorSet( color, 1.0, 1.0, 1.0 );
		time = cg.time - p->time;
		time2 = p->endTime - p->time;
		ratio = time / time2;
		
		width = p->width + ( ratio * ( p->endWidth - p->width ) );
		height = p->height + ( ratio * ( p->endHeight - p->height ) );
		
		if( p->roll )
		{
			VectorToAngles( cg.refdef.viewaxis[0], rotate_ang );
			rotate_ang[ROLL] += p->roll;
			AngleVectors( rotate_ang, NULL, rr, ru );
		}
		
		if( p->roll )
		{
			VectorMA( org, -height, ru, point );
			VectorMA( point, -width, rr, point );
		}
		else
		{
			VectorMA( org, -height, vup, point );
			VectorMA( point, -width, vright, point );
		}
		VectorCopy( point, verts[0].xyz );
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;
		
		if( p->roll )
		{
			VectorMA( point, 2 * height, ru, point );
		}
		else
		{
			VectorMA( point, 2 * height, vup, point );
		}
		VectorCopy( point, verts[1].xyz );
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;
		
		if( p->roll )
		{
			VectorMA( point, 2 * width, rr, point );
		}
		else
		{
			VectorMA( point, 2 * width, vright, point );
		}
		VectorCopy( point, verts[2].xyz );
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;
		
		if( p->roll )
		{
			VectorMA( point, -2 * height, ru, point );
		}
		else
		{
			VectorMA( point, -2 * height, vup, point );
		}
		VectorCopy( point, verts[3].xyz );
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}
	else if( p->type == P_SMOKE || p->type == P_SMOKE_IMPACT )
	{
		// create a front rotating facing polygon
		if( p->type == P_SMOKE_IMPACT && Distance( cg.snap->ps.origin, org ) > 1024 )
		{
			return;
		}
		
		/*
		   if(p->color == BLOODRED)
		   VectorSet(color, 0.22f, 0.0f, 0.0f);
		   else if(p->color == GREY75)
		   {
		   float           len;
		   float           greyit;
		   float           val;
		
		   len = Distance(cg.snap->ps.origin, org);
		   if(!len)
		   len = 1;
		
		   val = 4096 / len;
		   greyit = 0.25 * val;
		   if(greyit > 0.5)
		   greyit = 0.5;
		
		   VectorSet(color, greyit, greyit, greyit);
		   }
		   else
		 */
		
		VectorSet( color, 1.0, 1.0, 1.0 );
		
		time = cg.time - p->time;
		time2 = p->endTime - p->time;
		ratio = time / time2;
		
		if( cg.time > p->startfade )
		{
			invratio = 1 - ( ( cg.time - p->startfade ) / ( p->endTime - p->startfade ) );
			
			/*
			   if(p->color == EMISIVEFADE)
			   {
			   float           fval;
			
			   fval = (invratio * invratio);
			   if(fval < 0)
			   fval = 0;
			   VectorSet(color, fval, fval, fval);
			   }
			 */
			invratio *= color[3];
		}
		else
		{
			invratio = 1 * color[3];
		}
		
		if( invratio > 1 )
		{
			invratio = 1;
		}
		
		width = p->width + ( ratio * ( p->endWidth - p->width ) );
		height = p->height + ( ratio * ( p->endHeight - p->height ) );
		
		if( p->type != P_SMOKE_IMPACT )
		{
			vec3_t          temp;
			
			VectorToAngles( rforward, temp );
			p->accumroll += p->roll;
			temp[ROLL] += p->accumroll * 0.1;
			AngleVectors( temp, NULL, rright2, rup2 );
		}
		else
		{
			VectorCopy( rright, rright2 );
			VectorCopy( rup, rup2 );
		}
		
		if( p->rotate )
		{
			VectorMA( org, -height, rup2, point );
			VectorMA( point, -width, rright2, point );
		}
		else
		{
			VectorMA( org, -p->height, vup, point );
			VectorMA( point, -p->width, vright, point );
		}
		VectorCopy( point, verts[0].xyz );
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255 * color[0];
		verts[0].modulate[1] = 255 * color[1];
		verts[0].modulate[2] = 255 * color[2];
		verts[0].modulate[3] = 255 * invratio;
		
		if( p->rotate )
		{
			VectorMA( org, -height, rup2, point );
			VectorMA( point, width, rright2, point );
		}
		else
		{
			VectorMA( org, -p->height, vup, point );
			VectorMA( point, p->width, vright, point );
		}
		VectorCopy( point, verts[1].xyz );
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255 * color[0];
		verts[1].modulate[1] = 255 * color[1];
		verts[1].modulate[2] = 255 * color[2];
		verts[1].modulate[3] = 255 * invratio;
		
		if( p->rotate )
		{
			VectorMA( org, height, rup2, point );
			VectorMA( point, width, rright2, point );
		}
		else
		{
			VectorMA( org, p->height, vup, point );
			VectorMA( point, p->width, vright, point );
		}
		VectorCopy( point, verts[2].xyz );
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255 * color[0];
		verts[2].modulate[1] = 255 * color[1];
		verts[2].modulate[2] = 255 * color[2];
		verts[2].modulate[3] = 255 * invratio;
		
		if( p->rotate )
		{
			VectorMA( org, height, rup2, point );
			VectorMA( point, -width, rright2, point );
		}
		else
		{
			VectorMA( org, p->height, vup, point );
			VectorMA( point, -p->width, vright, point );
		}
		VectorCopy( point, verts[3].xyz );
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255 * color[0];
		verts[3].modulate[1] = 255 * color[1];
		verts[3].modulate[2] = 255 * color[2];
		verts[3].modulate[3] = 255 * invratio;
		
	}
	else if( p->type == P_FLAT_SCALEUP )
	{
		float           width, height;
		float           sinR, cosR;
		
		VectorSet( color, 1, 1, 1 );
		
		time = cg.time - p->time;
		time2 = p->endTime - p->time;
		ratio = time / time2;
		
		width = p->width + ( ratio * ( p->endWidth - p->width ) );
		height = p->height + ( ratio * ( p->endHeight - p->height ) );
		
		if( width > p->endWidth )
		{
			width = p->endWidth;
		}
		
		if( height > p->endHeight )
		{
			height = p->endHeight;
		}
		
		sinR = height * sin( DEG2RAD( p->roll ) ) * sqrt( 2 );
		cosR = width * cos( DEG2RAD( p->roll ) ) * sqrt( 2 );
		
		VectorCopy( org, verts[0].xyz );
		verts[0].xyz[0] -= sinR;
		verts[0].xyz[1] -= cosR;
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255 * color[0];
		verts[0].modulate[1] = 255 * color[1];
		verts[0].modulate[2] = 255 * color[2];
		verts[0].modulate[3] = 255;
		
		VectorCopy( org, verts[1].xyz );
		verts[1].xyz[0] -= cosR;
		verts[1].xyz[1] += sinR;
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255 * color[0];
		verts[1].modulate[1] = 255 * color[1];
		verts[1].modulate[2] = 255 * color[2];
		verts[1].modulate[3] = 255;
		
		VectorCopy( org, verts[2].xyz );
		verts[2].xyz[0] += sinR;
		verts[2].xyz[1] += cosR;
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255 * color[0];
		verts[2].modulate[1] = 255 * color[1];
		verts[2].modulate[2] = 255 * color[2];
		verts[2].modulate[3] = 255;
		
		VectorCopy( org, verts[3].xyz );
		verts[3].xyz[0] += cosR;
		verts[3].xyz[1] -= sinR;
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255 * color[0];
		verts[3].modulate[1] = 255 * color[1];
		verts[3].modulate[2] = 255 * color[2];
		verts[3].modulate[3] = 255;
	}
	else if( p->type == P_SPARK || p->type == P_BLOOD )
	{
		axis_t          axis;
		
		time = cg.time - p->time;
		time2 = p->endTime - p->time;
		ratio = time / time2;
		
		/*
		   if(cg.time > p->startfade)
		   {
		   invratio = 1 - ((cg.time - p->startfade) / (p->endTime - p->startfade));
		   invratio *= color[3];
		   }
		   else
		   {
		   invratio = 1 * color[3];
		   }
		
		   if(invratio > 1)
		   invratio = 1;
		 */
		
		width = p->width + ( ratio * ( p->endWidth - p->width ) );
		height = p->height + ( ratio * ( p->endHeight - p->height ) );
		
		if( width > p->endWidth )
		{
			width = p->endWidth;
		}
		
		if( height > p->endHeight )
		{
			height = p->endHeight;
		}
		
		// find orientation vectors
		VectorSubtract( cg.refdef.vieworg, org, axis[0] );
		VectorSubtract( p->oldOrg, org, axis[1] );
		CrossProduct( axis[0], axis[1], axis[2] );
		VectorNormalize( axis[1] );
		VectorNormalize( axis[2] );
		
		// find normal
		CrossProduct( axis[1], axis[2], axis[0] );
		VectorNormalize( axis[0] );
		
		VectorMA( org, -width, axis[1], oldOrigin );
		VectorScale( axis[2], height, axis[2] );
		
		verts[0].xyz[0] = org[0] - axis[2][0];
		verts[0].xyz[1] = org[1] - axis[2][1];
		verts[0].xyz[2] = org[2] - axis[2][2];
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255 * color[0];
		verts[0].modulate[1] = 255 * color[1];
		verts[0].modulate[2] = 255 * color[2];
		verts[0].modulate[3] = 255 * color[3];
		
		verts[1].xyz[0] = org[0] + axis[2][0];
		verts[1].xyz[1] = org[1] + axis[2][1];
		verts[1].xyz[2] = org[2] + axis[2][2];
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255 * color[0];
		verts[1].modulate[1] = 255 * color[1];
		verts[1].modulate[2] = 255 * color[2];
		verts[1].modulate[3] = 255 * color[3];
		
		verts[2].xyz[0] = oldOrigin[0] + axis[2][0];
		verts[2].xyz[1] = oldOrigin[1] + axis[2][1];
		verts[2].xyz[2] = oldOrigin[2] + axis[2][2];
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255 * color[0];
		verts[2].modulate[1] = 255 * color[1];
		verts[2].modulate[2] = 255 * color[2];
		verts[2].modulate[3] = 255 * color[3];
		
		verts[3].xyz[0] = oldOrigin[0] - axis[2][0];
		verts[3].xyz[1] = oldOrigin[1] - axis[2][1];
		verts[3].xyz[2] = oldOrigin[2] - axis[2][2];
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255 * color[0];
		verts[3].modulate[1] = 255 * color[1];
		verts[3].modulate[2] = 255 * color[2];
		verts[3].modulate[3] = 255 * color[3];
	}
	else if( p->type == P_LIGHTSPARK )
	{
		axis_t          axis;
		vec_t           speed;
		
		time = cg.time - p->time;
		time2 = p->endTime - p->time;
		ratio = time / time2;
		
		// find orientation vectors
		VectorSubtract( cg.refdef.vieworg, org, axis[0] );
		VectorSubtract( p->oldOrg, org, axis[1] );
		CrossProduct( axis[0], axis[1], axis[2] );
		speed = VectorNormalize( axis[1] );
		VectorNormalize( axis[2] );
		
		width = Q_min( p->width * speed, p->width );	// + (ratio * (p->endWidth - p->width));
		height = p->height;		// * speed;// + (ratio * (p->endHeight - p->height));
		
		// find normal
		CrossProduct( axis[1], axis[2], axis[0] );
		VectorNormalize( axis[0] );
		
		VectorMA( org, -width, axis[1], oldOrigin );
		VectorScale( axis[2], height, axis[2] );
		
		verts[0].xyz[0] = org[0] - axis[2][0];
		verts[0].xyz[1] = org[1] - axis[2][1];
		verts[0].xyz[2] = org[2] - axis[2][2];
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255 * color[0];
		verts[0].modulate[1] = 255 * color[1];
		verts[0].modulate[2] = 255 * color[2];
		verts[0].modulate[3] = 255 * color[3];
		
		verts[1].xyz[0] = org[0] + axis[2][0];
		verts[1].xyz[1] = org[1] + axis[2][1];
		verts[1].xyz[2] = org[2] + axis[2][2];
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255 * color[0];
		verts[1].modulate[1] = 255 * color[1];
		verts[1].modulate[2] = 255 * color[2];
		verts[1].modulate[3] = 255 * color[3];
		
		verts[2].xyz[0] = oldOrigin[0] + axis[2][0];
		verts[2].xyz[1] = oldOrigin[1] + axis[2][1];
		verts[2].xyz[2] = oldOrigin[2] + axis[2][2];
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255 * color[0];
		verts[2].modulate[1] = 255 * color[1];
		verts[2].modulate[2] = 255 * color[2];
		verts[2].modulate[3] = 255 * color[3];
		
		verts[3].xyz[0] = oldOrigin[0] - axis[2][0];
		verts[3].xyz[1] = oldOrigin[1] - axis[2][1];
		verts[3].xyz[2] = oldOrigin[2] - axis[2][2];
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255 * color[0];
		verts[3].modulate[1] = 255 * color[1];
		verts[3].modulate[2] = 255 * color[2];
		verts[3].modulate[3] = 255 * color[3];
	}
	else if( p->type == P_FLAT )
	{
		VectorCopy( org, verts[0].xyz );
		verts[0].xyz[0] -= p->height;
		verts[0].xyz[1] -= p->width;
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;
		
		VectorCopy( org, verts[1].xyz );
		verts[1].xyz[0] -= p->height;
		verts[1].xyz[1] += p->width;
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;
		
		VectorCopy( org, verts[2].xyz );
		verts[2].xyz[0] += p->height;
		verts[2].xyz[1] += p->width;
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;
		
		VectorCopy( org, verts[3].xyz );
		verts[3].xyz[0] += p->height;
		verts[3].xyz[1] -= p->width;
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}
	
	if( !p->pshader )
	{
// (SA) temp commented out for DM
//      CG_Printf ("CG_AddParticleToScene type %d p->pshader == ZERO\n", p->type);
		return;
	}
	
	if( p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY )
	{
		trap_R_AddPolyToScene( p->pshader, 3, TRIverts );
	}
	else
	{
		trap_R_AddPolyToScene( p->pshader, 4, verts );
	}
}

/*
==================
ClipVelocity

Slide off of the impacting surface
==================
*/
/*static void ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float           backoff;
	float           change;
	int             i;

	backoff = DotProduct(in, normal);

	if(backoff < 0)
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for(i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}*/


/*
===============
CG_AddParticles
===============
*/
void CG_AddParticles( void )
{
	cparticle_t*    p, *next;
	float           time, time2;
	float           grav;
	vec3_t          org;
	vec4_t          color;
	cparticle_t*    active, *tail;
	int             type;
	vec3_t          rotate_ang;
	int             contents;
	
	// Ridah, made this static so it doesn't interfere with other files
	static float    roll = 0.0;
	
	if( !initparticles )
	{
		CG_InitParticles();
	}
	
	if( !cg_particles.integer )
	{
		return;
	}
	
	VectorCopy( cg.refdef.viewaxis[0], vforward );
	VectorCopy( cg.refdef.viewaxis[1], vright );
	VectorCopy( cg.refdef.viewaxis[2], vup );
	
	VectorToAngles( cg.refdef.viewaxis[0], rotate_ang );
	roll += ( ( cg.time - oldtime ) * 0.1 );
	rotate_ang[ROLL] += ( roll * 0.9 );
	AngleVectors( rotate_ang, rforward, rright, rup );
	
	oldtime = cg.time;
	
	active = NULL;
	tail = NULL;
	
	// Tr3B: allow gravity tweaks by the server
	grav = 1;
	grav /= DEFAULT_GRAVITY;
	
	for( p = cg_activeParticles; p; p = next )
	{
		next = p->next;
		
		time = ( cg.time - p->time ) * 0.001;
		
		color[0] = p->color[0] + p->colorVel[0] * time;
		color[1] = p->color[1] + p->colorVel[1] * time;
		color[2] = p->color[2] + p->colorVel[2] * time;
		color[3] = p->color[3] + p->colorVel[3] * time;
		
		ClampColor( color );
		
		if( color[3] <= 0 )
		{
			// faded out
			CG_FreeParticle( p );
			continue;
		}
		
		if( p->type == P_SMOKE || p->type == P_BLOOD || p->type == P_SMOKE_IMPACT || p->type == P_SPARK )
		{
			if( cg.time > p->endTime )
			{
				CG_FreeParticle( p );
				continue;
			}
		}
		
		if( p->type == P_LIGHTSPARK )
		{
			if( cg.time > p->endTime )
			{
				CG_FreeParticle( p );
				continue;
			}
		}
		
		if( p->type == P_WEATHER_FLURRY )
		{
			if( cg.time > p->endTime )
			{
				CG_FreeParticle( p );
				continue;
			}
		}
		
		
		if( p->type == P_FLAT_SCALEUP_FADE )
		{
			if( cg.time > p->endTime )
			{
				CG_FreeParticle( p );
				continue;
			}
			
		}
		
		if( p->type == P_SPRITE && p->endTime < 0 )
		{
			// temporary sprite
			CG_AddParticleToScene( p, p->org, color );
			CG_FreeParticle( p );
			continue;
		}
		
		// backup old position
		VectorCopy( p->org, p->oldOrg );
		
		// calculate new position
		time2 = time * time;
		
		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;
		
		// Tr3B: ah well particles shouldn't be affected by gravity
		//org[2]-= 0.5 * -cg_gravityZ.value * time2;
		
		// Tr3B: add some collision tests
		if( cg_particleCollision.integer )
		{
			contents = trap_CM_PointContents( org, 0 );
			
			// kill all particles in solid
			if( contents & MASK_SOLID )
			{
				CG_FreeParticle( p );
				continue;
			}
			
			// kill all air only particles in water or slime
			if( p->flags & PF_AIRONLY )
			{
				if( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
				{
					CG_FreeParticle( p );
					continue;
				}
			}
			
			if( p->bounceFactor )
			{
				trace_t         trace;
				
				vec3_t          vel;
				int             hitTime;
				float           time;
				
				CG_Trace( &trace, p->oldOrg, NULL, NULL, org, -1, CONTENTS_SOLID );
				
				//trap_CM_BoxTrace(&trace, p->oldOrg, org, NULL, NULL, 0, CONTENTS_SOLID);
				//trace.entityNum = trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
				
				if( trace.fraction > 0 && trace.fraction < 1 )
				{
					// reflect the velocity on the trace plane
					hitTime = cg.time - cg.frametime + cg.frametime * trace.fraction;
					
					time = ( ( float )hitTime - p->time ) * 0.001;
					
					VectorSet( vel, p->vel[0], p->vel[1], p->vel[2] + p->accel[2] * time * grav );
					VectorReflect( vel, trace.plane.normal, p->vel );
					VectorScale( p->vel, p->bounceFactor, p->vel );
					
					// check for stop, making sure that even on low FPS systems it doesn't bobble
					if( trace.allsolid || ( trace.plane.normal[2] > 0 && ( p->vel[2] < 40 || p->vel[2] < -cg.frametime * p->vel[2] ) ) )
					{
						//if(p->vel[2] > 0.9)
						{
							VectorClear( p->vel );
							VectorClear( p->accel );
							
							p->bounceFactor = 0.0f;
							
							CG_FreeParticle( p );
							continue;
						}
						/*
						   else
						   {
						   float           dot;
						
						   //ClipVelocity(p->vel, trace.plane.normal, p->vel, p->bounceFactor);
						
						   // FIXME: check for new plane or free fall
						   dot = DotProduct(p->vel, trace.plane.normal);
						   VectorMA(p->vel, -dot, trace.plane.normal, p->vel);
						
						   dot = DotProduct(p->accel, trace.plane.normal);
						   VectorMA(p->accel, -dot, trace.plane.normal, p->accel);
						   }
						 */
					}
					
					if( p->type == P_BLOOD )
					{
						int             radius, r = 0;
						qhandle_t       shader;
						
						radius = 3 + random() * 5;
						r = rand() & 3;
						
						if( r == 0 )
						{
							shader = cgs.media.bloodMarkShader;
						}
						else if( r == 1 )
						{
							shader = cgs.media.bloodMark2Shader;
						}
						else
						{
							shader = cgs.media.bloodMark3Shader;
						}
						
						CG_ImpactMark( shader, trace.endpos, trace.plane.normal, random() * 360, 1, 1, 1, 1, qtrue, radius,
									   qfalse );
					}
					
					VectorCopy( trace.endpos, org );
					
					// reset particle
					p->time = cg.time;
					//VectorCopy(p->org, p->oldOrg);
					VectorCopy( org, p->org );
				}
			}
		}
		
		// have this always below CG_FreeParticle(p); !
		p->next = NULL;
		if( !tail )
		{
			active = tail = p;
		}
		else
		{
			tail->next = p;
			tail = p;
		}
		
		type = p->type;
		
		CG_AddParticleToScene( p, org, color );
	}
	
	cg_activeParticles = active;
}




/*
======================
CG_AddParticles
======================
*/
void CG_ParticleSnowFlurry( qhandle_t pshader, centity_t* cent )
{
	cparticle_t*    p;
	qboolean        turb = qtrue;
	
	if( !pshader )
	{
		CG_Printf( "CG_ParticleSnowFlurry pshader == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->color[3] = 0.90f;
	p->colorVel[3] = 0;
	
	p->start = cent->currentState.origin2[0];
	p->end = cent->currentState.origin2[1];
	
	p->endTime = cg.time + cent->currentState.time;
	p->startfade = cg.time + cent->currentState.time2;
	
	p->pshader = pshader;
	
	if( rand() % 100 > 90 )
	{
		p->height = 32;
		p->width = 32;
		p->color[3] = 0.10f;
	}
	else
	{
		p->height = 1;
		p->width = 1;
	}
	
	p->vel[2] = -20;
	
	p->type = P_WEATHER_FLURRY;
	
	if( turb )
	{
		p->vel[2] = -10;
	}
	
	VectorCopy( cent->currentState.origin, p->org );
	
	p->org[0] = p->org[0];
	p->org[1] = p->org[1];
	p->org[2] = p->org[2];
	
	p->vel[0] = p->vel[1] = 0;
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	
	p->vel[0] += cent->currentState.angles[0] * 32 + ( crandom() * 16 );
	p->vel[1] += cent->currentState.angles[1] * 32 + ( crandom() * 16 );
	p->vel[2] += cent->currentState.angles[2];
	
	if( turb )
	{
		p->accel[0] = crandom() * 16;
		p->accel[1] = crandom() * 16;
	}
}

void CG_ParticleSnow( qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum )
{
	cparticle_t*    p;
	
	if( !pshader )
	{
		CG_Printf( "CG_ParticleSnow pshader == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->color[3] = 0.40f;
	p->colorVel[3] = 0;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	p->height = 1;
	p->width = 1;
	
	p->vel[2] = -50;
	
	if( turb )
	{
		p->type = P_WEATHER_TURBULENT;
		p->vel[2] = -50 * 1.3;
	}
	else
	{
		p->type = P_WEATHER;
	}
	
	VectorCopy( origin, p->org );
	
	p->org[0] = p->org[0] + ( crandom() * range );
	p->org[1] = p->org[1] + ( crandom() * range );
	p->org[2] = p->org[2] + ( crandom() * ( p->start - p->end ) );
	
	p->vel[0] = p->vel[1] = 0;
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	
	if( turb )
	{
		p->vel[0] = crandom() * 16;
		p->vel[1] = crandom() * 16;
	}
	
	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

void CG_ParticleBubble( qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum )
{
	cparticle_t*    p;
	float           randsize;
	
	if( !pshader )
	{
		CG_Printf( "CG_ParticleSnow pshader == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->color[3] = 0.40f;
	p->colorVel[3] = 0;
	p->start = origin[2];
	p->end = origin2[2];
	p->pshader = pshader;
	
	randsize = 1 + ( crandom() * 0.5 );
	
	p->height = randsize;
	p->width = randsize;
	
	p->vel[2] = 50 + ( crandom() * 10 );
	
	if( turb )
	{
		p->type = P_BUBBLE_TURBULENT;
		p->vel[2] = 50 * 1.3;
	}
	else
	{
		p->type = P_BUBBLE;
	}
	
	VectorCopy( origin, p->org );
	
	p->org[0] = p->org[0] + ( crandom() * range );
	p->org[1] = p->org[1] + ( crandom() * range );
	p->org[2] = p->org[2] + ( crandom() * ( p->start - p->end ) );
	
	p->vel[0] = p->vel[1] = 0;
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	
	if( turb )
	{
		p->vel[0] = crandom() * 4;
		p->vel[1] = crandom() * 4;
	}
	
	// Rafael snow pvs check
	p->snum = snum;
	p->link = qtrue;
	
}

void CG_ParticleSmoke( qhandle_t pshader, centity_t* cent )
{
	// using cent->density = enttime
	//       cent->frame = startfade
	cparticle_t*    p;
	
	if( !pshader )
	{
		CG_Printf( "CG_ParticleSmoke == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	
	p->endTime = cg.time + cent->currentState.time;
	p->startfade = cg.time + cent->currentState.time2;
	
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	p->start = cent->currentState.origin[2];
	p->end = cent->currentState.origin2[2];
	p->pshader = pshader;
	p->rotate = qfalse;
	p->height = 8;
	p->width = 8;
	p->endHeight = 32;
	p->endWidth = 32;
	p->type = P_SMOKE;
	
	VectorCopy( cent->currentState.origin, p->org );
	
	p->vel[0] = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	
	p->vel[2] = 5;
	
	if( cent->currentState.frame == 1 )	// reverse gravity
	{
		p->vel[2] *= -1;
	}
	
	p->roll = 8 + ( crandom() * 4 );
}

void CG_ParticleBulletDebris( vec3_t org, vec3_t vel, int duration )
{
	cparticle_t*    p;
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	
	p->endTime = cg.time + duration;
	p->startfade = cg.time + duration / 2;
	
//  p->color = EMISIVEFADE;
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	
	p->height = 0.5;
	p->width = 0.5;
	p->endHeight = 0.5;
	p->endWidth = 0.5;
	
	p->pshader = cgs.media.tracerShader;
	
	p->type = P_SMOKE;
	
	VectorCopy( org, p->org );
	
	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	
	p->accel[2] = -60;
	p->vel[2] += -20;
}

void CG_ParticleDirtBulletDebris_Core( vec3_t org, vec3_t vel, int duration, float width, float height, float alpha,
									   qhandle_t shader )
{
	cparticle_t*    p;
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->endTime = cg.time + duration;
	p->startfade = cg.time + duration / 2;
	
//  p->color = EMISIVEFADE;
	p->color[3] = alpha;
	p->colorVel[3] = 0;
	
	p->height = width;
	p->width = height;
	p->endHeight = p->height;
	p->endWidth = p->width;
	
	p->rotate = 0;
	
	p->type = P_SMOKE;
	
	p->pshader = shader;
	
	VectorCopy( org, p->org );
	VectorCopy( vel, p->vel );
	VectorSet( p->accel, 0, 0, -330 );
}

int CG_NewParticleArea( int num )
{
	// const char *str;
	char*           str;
	char*           token;
	int             type;
	vec3_t          origin, origin2;
	int             i;
	float           range = 0;
	int             turb;
	int             numparticles;
	int             snum;
	
	str = ( char* )CG_ConfigString( num );
	if( !str[0] )
	{
		return ( 0 );
	}
	
	// returns type 128 64 or 32
	token = Com_Parse( &str );
	type = atoi( token );
	
	if( type == 1 )
	{
		range = 128;
	}
	else if( type == 2 )
	{
		range = 64;
	}
	else if( type == 3 )
	{
		range = 32;
	}
	else if( type == 0 )
	{
		range = 256;
	}
	else if( type == 4 )
	{
		range = 8;
	}
	else if( type == 5 )
	{
		range = 16;
	}
	else if( type == 6 )
	{
		range = 32;
	}
	else if( type == 7 )
	{
		range = 64;
	}
	
	for( i = 0; i < 3; i++ )
	{
		token = Com_Parse( &str );
		origin[i] = atof( token );
	}
	
	for( i = 0; i < 3; i++ )
	{
		token = Com_Parse( &str );
		origin2[i] = atof( token );
	}
	
	token = Com_Parse( &str );
	numparticles = atoi( token );
	
	token = Com_Parse( &str );
	turb = atoi( token );
	
	token = Com_Parse( &str );
	snum = atoi( token );
	
	for( i = 0; i < numparticles; i++ )
	{
		if( type >= 4 )
		{
			CG_ParticleBubble( cgs.media.waterBubbleShader, origin, origin2, turb, range, snum );
		}
		else
		{
			CG_ParticleSnow( cgs.media.waterBubbleShader, origin, origin2, turb, range, snum );
		}
	}
	
	return ( 1 );
}

void CG_SnowLink( centity_t* cent, qboolean particleOn )
{
	cparticle_t*    p, *next;
	int             id;
	
	id = cent->currentState.frame;
	
	for( p = cg_activeParticles; p; p = next )
	{
		next = p->next;
		
		if( p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT )
		{
			if( p->snum == id )
			{
				if( particleOn )
				{
					p->link = qtrue;
				}
				else
				{
					p->link = qfalse;
				}
			}
		}
		
	}
}

void CG_ParticleImpactSmokePuff( qhandle_t pshader, vec3_t origin )
{
	cparticle_t*    p;
	
	if( !pshader )
	{
		CG_Printf( "CG_ParticleImpactSmokePuff pshader == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->color[3] = 0.25;
	p->colorVel[3] = 0;
	p->roll = crandom() * 179;
	
	p->pshader = pshader;
	
	p->endTime = cg.time + 1000;
	p->startfade = cg.time + 100;
	
	p->width = 8.0f * ( 1.0f + random() * 0.5f );
	p->height = 8.0f * ( 1.0f + random() * 0.5f );
	
	p->endHeight = p->height * 2;
	p->endWidth = p->width * 2;
	
	p->endTime = cg.time + 500;
	
	p->type = P_SMOKE_IMPACT;
	
	VectorCopy( origin, p->org );
	VectorSet( p->vel, 0, 0, 20 );
	VectorSet( p->accel, 0, 0, 20 );
	
	p->rotate = qtrue;
}



void CG_Particle_OilParticle( qhandle_t pshader, centity_t* cent )
{
	cparticle_t*    p;
	
	int             time;
	int             time2;
	float           ratio;
	
	float           duration = 1500;
	
	time = cg.time;
	time2 = cg.time + cent->currentState.time;
	
	ratio = ( float )1 - ( ( float )time / ( float )time2 );
	
	if( !pshader )
	{
		CG_Printf( "CG_Particle_OilParticle == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	p->roll = 0;
	
	p->pshader = pshader;
	
	p->endTime = cg.time + duration;
	
	p->startfade = p->endTime;
	
	p->width = 1;
	p->height = 3;
	
	p->endHeight = 3;
	p->endWidth = 1;
	
	p->type = P_SMOKE;
	
	VectorCopy( cent->currentState.origin, p->org );
	
	p->vel[0] = ( cent->currentState.origin2[0] * ( 16 * ratio ) );
	p->vel[1] = ( cent->currentState.origin2[1] * ( 16 * ratio ) );
	p->vel[2] = ( cent->currentState.origin2[2] );
	
	p->snum = 1.0f;
	
	VectorClear( p->accel );
	
	p->accel[2] = -20;
	
	p->rotate = qfalse;
	
	p->roll = rand() % 179;
	
	p->color[3] = 0.75;
}

void CG_Particle_OilSlick( qhandle_t pshader, centity_t* cent )
{
	cparticle_t*    p;
	
	if( !pshader )
	{
		CG_Printf( "CG_Particle_OilSlick == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	
	if( cent->currentState.angles2[2] )
	{
		p->endTime = cg.time + cent->currentState.angles2[2];
	}
	else
	{
		p->endTime = cg.time + 60000;
	}
	
	p->startfade = p->endTime;
	
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	p->roll = 0;
	
	p->pshader = pshader;
	
	if( cent->currentState.angles2[0] || cent->currentState.angles2[1] )
	{
		p->width = cent->currentState.angles2[0];
		p->height = cent->currentState.angles2[0];
		
		p->endHeight = cent->currentState.angles2[1];
		p->endWidth = cent->currentState.angles2[1];
	}
	else
	{
		p->width = 8;
		p->height = 8;
		
		p->endHeight = 16;
		p->endWidth = 16;
	}
	
	p->type = P_FLAT_SCALEUP;
	
	p->snum = 1.0;
	
	VectorCopy( cent->currentState.origin, p->org );
	
	p->org[2] += 0.55 + ( crandom() * 0.5 );
	
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = 0;
	VectorClear( p->accel );
	
	p->rotate = qfalse;
	
	p->roll = rand() % 179;
	
	p->color[3] = 0.75;
}

void CG_OilSlickRemove( centity_t* cent )
{
	cparticle_t*    p, *next;
	int             id;
	
	id = 1.0f;
	
	if( !id )
	{
		CG_Printf( "CG_OilSlickRevove NULL id\n" );
	}
	
	for( p = cg_activeParticles; p; p = next )
	{
		next = p->next;
		
		if( p->type == P_FLAT_SCALEUP )
		{
			if( p->snum == id )
			{
				p->endTime = cg.time + 100;
				p->startfade = p->endTime;
				p->type = P_FLAT_SCALEUP_FADE;
				
			}
		}
		
	}
}

void CG_ParticleBlood( vec3_t org, vec3_t dir, int count )
{
	int             i, j;
	cparticle_t*    p;
	
	//float           d;
	int             r;
	
	if( !cg_blood.integer )
	{
		return;
	}
	
	for( i = 0; i < count; i++ )
	{
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		p->type = P_BLOOD;
		
		r = rand() & 3;
		if( r == 0 )
		{
			p->pshader = cgs.media.bloodSpurtShader;
		}
		else if( r == 1 )
		{
			p->pshader = cgs.media.bloodSpurt2Shader;
		}
		else
		{
			p->pshader = cgs.media.bloodSpurt3Shader;
		}
		
		p->height = 1.1f;
		p->width = 5;
		p->endHeight = 3;
		p->endWidth = 50;
		
		p->time = cg.time;
		p->endTime = cg.time + 5000;
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		p->color[3] = 1;
		
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->colorVel[3] = 0;
		
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j];
			p->vel[j] = dir[j] * 30 + crandom() * 40;
		}
		
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 4;
		
		VectorCopy( p->org, p->oldOrg );
		
		p->bounceFactor = 0.1f;
	}
	
#if 0
	for( i = 0; i < count; i++ )
	{
		if( !free_particles )
		{
			return;
		}
		p = free_particles;
		
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear( p->vel );
		p->orient = 0;
		p->flags = 0;
		p->time = cl.time;
		p->endTime = cl.time + 5000;
		p->blend_dst = GL_SRC_ALPHA;
		p->blend_src = GL_ONE_MINUS_SRC_ALPHA;
		
		p->color[0] = 0.1;
		p->color[1] = 0;
		p->color[2] = 0;
		
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		
		p->type = PT_BLOODMIST;
		p->size = 2;
		p->sizeVel = 10;
		
		d = rand() & 7;
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
		}
		
		
		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 0.7;
		
		p->alphavel = -1.0 / ( 0.5 + frand() * 0.3 );
	}
#endif
}

void CG_Particle_Bleed( qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration )
{
	cparticle_t*    p;
	
	if( !pshader )
	{
		CG_Printf( "CG_Particle_Bleed pshader == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->time = cg.time;
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	p->roll = 0;
	
	p->pshader = pshader;
	
	p->endTime = cg.time + duration;
	
	if( fleshEntityNum )
	{
		p->startfade = cg.time;
	}
	else
	{
		p->startfade = cg.time + 100;
	}
	
	p->width = 4;
	p->height = 4;
	
	p->endHeight = 4 + rand() % 3;
	p->endWidth = p->endHeight;
	
	p->type = P_SMOKE;
	
	VectorCopy( start, p->org );
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = -20;
	VectorClear( p->accel );
	
	p->rotate = qfalse;
	
	p->roll = rand() % 179;
	
//  p->color = BLOODRED;
	p->color[3] = 0.75;
}

static qboolean ValidBloodPool( vec3_t start )
{
#define EXTRUDE_DIST	0.5

	vec3_t          angles;
	vec3_t          right, up;
	vec3_t          this_pos, x_pos, center_pos, end_pos;
	float           x, y;
	float           fwidth, fheight;
	trace_t         trace;
	vec3_t          normal;
	
	fwidth = 16;
	fheight = 16;
	
	VectorSet( normal, 0, 0, 1 );
	
	VectorToAngles( normal, angles );
	AngleVectors( angles, NULL, right, up );
	
	VectorMA( start, EXTRUDE_DIST, normal, center_pos );
	
	for( x = -fwidth / 2; x < fwidth; x += fwidth )
	{
		VectorMA( center_pos, x, right, x_pos );
		
		for( y = -fheight / 2; y < fheight; y += fheight )
		{
			VectorMA( x_pos, y, up, this_pos );
			VectorMA( this_pos, -EXTRUDE_DIST * 2, normal, end_pos );
			
			CG_Trace( &trace, this_pos, NULL, NULL, end_pos, -1, CONTENTS_SOLID );
			
			
			if( trace.entityNum < ( MAX_REF_ENTITIES - 1 ) )	// may only land on world
			{
				return qfalse;
			}
			
			if( !( !trace.startsolid && trace.fraction < 1 ) )
			{
				return qfalse;
			}
			
		}
	}
	
	return qtrue;
}

void CG_BloodPool( qhandle_t pshader, vec3_t origin )
{
	cparticle_t*    p;
	qboolean        legit;
	float           rndSize;
	
	if( !pshader )
	{
		CG_Printf( "CG_BloodPool pshader == ZERO!\n" );
	}
	
	legit = ValidBloodPool( origin );
	
	if( !legit )
	{
		return;
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->endTime = cg.time + 3000;
	p->startfade = p->endTime;
	
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	p->roll = 0;
	
	p->pshader = pshader;
	
	rndSize = 0.4 + random() * 0.6;
	
	p->width = 8 * rndSize;
	p->height = 8 * rndSize;
	
	p->endHeight = 16 * rndSize;
	p->endWidth = 16 * rndSize;
	
	p->type = P_FLAT_SCALEUP;
	
	VectorCopy( origin, p->org );
	
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = 0;
	VectorClear( p->accel );
	
	p->rotate = qfalse;
	
	p->roll = rand() % 179;
	
	p->color[3] = 0.75;
	
//  p->color = BLOODRED;
}

#define NORMALSIZE	16
#define LARGESIZE	32

void CG_ParticleBloodCloud( vec3_t origin, vec3_t dir )
{
	float           length;
	float           dist;
	float           crittersize;
	vec3_t          angles, forward;
	vec3_t          point;
	cparticle_t*    p;
	int             i;
	
	dist = 0;
	
	length = VectorLength( dir );
	VectorToAngles( dir, angles );
	AngleVectors( angles, forward, NULL, NULL );
	
	crittersize = LARGESIZE;
	
	if( length )
	{
		dist = length / crittersize;
	}
	
	if( dist < 1 )
	{
		dist = 1;
	}
	
	VectorCopy( origin, point );
	
	for( i = 0; i < dist; i++ )
	{
		VectorMA( point, crittersize, forward, point );
		
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		p->time = cg.time;
		p->color[3] = 1.0;
		p->colorVel[3] = 0;
		p->roll = 0;
		
		p->pshader = cgs.media.smokePuffShader;
		
		p->endTime = cg.time + 350 + ( crandom() * 100 );
		
		p->startfade = cg.time;
		
		p->width = LARGESIZE;
		p->height = LARGESIZE;
		p->endHeight = LARGESIZE;
		p->endWidth = LARGESIZE;
		
		p->type = P_SMOKE;
		
		VectorCopy( origin, p->org );
		
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = -1;
		
		VectorClear( p->accel );
		
		p->rotate = qfalse;
		
		p->roll = rand() % 179;
		
		VectorSet( p->color, 0.22f, 0.0f, 0.0f );
		
		p->color[3] = 0.75;
	}
}

void CG_ParticleSparks( vec3_t org, vec3_t vel, int duration, float x, float y, float speed )
{
	cparticle_t*    p;
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->endTime = cg.time + duration;
	p->startfade = cg.time + duration / 2;
	
//  p->color = EMISIVEFADE;
	p->color[3] = 0.4f;
	p->colorVel[3] = 0;
	
	p->height = 0.4f;
	p->width = 5;
	p->endHeight = 3;
	p->endWidth = 30;
	
	p->pshader = cgs.media.sparkShader;
	
	p->type = P_SPARK;
	
	VectorCopy( org, p->org );
	
	p->org[0] += ( crandom() * x );
	p->org[1] += ( crandom() * y );
	
	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];
	
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	
	p->vel[0] += ( crandom() * 4 );
	p->vel[1] += ( crandom() * 4 );
	p->vel[2] += ( 20 + ( crandom() * 10 ) ) * speed;
	
	p->accel[0] = crandom() * 4;
	p->accel[1] = crandom() * 4;
}

void CG_ParticleSparks2( vec3_t org, vec3_t dir, int count )
{
	int             i, j;
	cparticle_t*    p;
	float           d;
	
	for( i = 0; i < count; i++ )
	{
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		//p->flags = PARTICLE_OVERBRIGHT | PARTICLE_DIRECTIONAL /* |PARTICLE_BOUNCE */ ;
		
		p->endTime = cg.time + 20000;
		
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 0.3f;
		p->color[3] = 1.0;
		
		p->colorVel[0] = 0;
		p->colorVel[1] = -1;
		p->colorVel[2] = -1.5;
		p->colorVel[3] = -1.0 / ( 1.5 + random() * 1.666 );
		
		p->height = 0.4f;
		p->width = 5;
		p->endHeight = 3;
		p->endWidth = 30;
		
		p->pshader = cgs.media.sparkShader;
		
		p->type = P_SPARK;
		
		d = rand() & 7;
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = crandom() * 30;
		}
		
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 3;
		
		VectorCopy( p->org, p->oldOrg );
	}
}

void CG_ParticleDust( centity_t* cent, vec3_t origin, vec3_t dir )
{
	float           length;
	float           dist;
	float           crittersize;
	vec3_t          angles, forward;
	vec3_t          point;
	cparticle_t*    p;
	int             i;
	
	dist = 0;
	
	VectorNegate( dir, dir );
	length = VectorLength( dir );
	VectorToAngles( dir, angles );
	AngleVectors( angles, forward, NULL, NULL );
	
	crittersize = LARGESIZE;
	
	if( length )
	{
		dist = length / crittersize;
	}
	
	if( dist < 1 )
	{
		dist = 1;
	}
	
	VectorCopy( origin, point );
	
	for( i = 0; i < dist; i++ )
	{
		VectorMA( point, crittersize, forward, point );
		
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		p->time = cg.time;
		p->color[3] = 5.0;
		p->colorVel[3] = 0;
		p->roll = 0;
		
		p->pshader = cgs.media.smokePuffShader;
		
		// RF, stay around for long enough to expand and dissipate naturally
		if( length )
		{
			p->endTime = cg.time + 4500 + ( crandom() * 3500 );
		}
		else
		{
			p->endTime = cg.time + 750 + ( crandom() * 500 );
		}
		
		p->startfade = cg.time;
		
		p->width = LARGESIZE;
		p->height = LARGESIZE;
		
		// RF, expand while falling
		p->endHeight = LARGESIZE * 3.0;
		p->endWidth = LARGESIZE * 3.0;
		
		if( !length )
		{
			p->width *= 0.2f;
			p->height *= 0.2f;
			
			p->endHeight = NORMALSIZE;
			p->endWidth = NORMALSIZE;
		}
		
		p->type = P_SMOKE;
		
		VectorCopy( point, p->org );
		
		p->vel[0] = crandom() * 6;
		p->vel[1] = crandom() * 6;
		p->vel[2] = random() * 20;
		
		// RF, add some gravity/randomness
		p->accel[0] = crandom() * 3;
		p->accel[1] = crandom() * 3;
		p->accel[2] = -PARTICLE_GRAVITY * 0.4;
		
		VectorClear( p->accel );
		
		p->rotate = qfalse;
		
		p->roll = rand() % 179;
		
		p->color[3] = 0.75;
	}
}

void CG_ParticleMisc( qhandle_t pshader, vec3_t origin, int size, int duration, float alpha )
{
	cparticle_t*    p;
	
	if( !pshader )
	{
		CG_Printf( "CG_ParticleMisc pshader == ZERO!\n" );
	}
	
	p = CG_AllocParticle();
	if( !p )
	{
		return;
	}
	
	p->color[3] = 1.0;
	p->colorVel[3] = 0;
	p->roll = rand() % 179;
	
	p->pshader = pshader;
	
	if( duration > 0 )
	{
		p->endTime = cg.time + duration;
	}
	else
	{
		p->endTime = duration;
	}
	
	p->startfade = cg.time;
	
	p->width = size;
	p->height = size;
	
	p->endHeight = size;
	p->endWidth = size;
	
	p->type = P_SPRITE;
	
	VectorCopy( origin, p->org );
	
	p->rotate = qfalse;
}


/*
==========================
CG_ParticleTeleportEffect

Tr3B: Tenebrae style player teleporting in or out
==========================
*/
void CG_ParticleTeleportEffect( const vec3_t origin )
{
	int             i, j, k;
	vec3_t          randVec, tempVec;
	cparticle_t*    p;
	
	// create particles inside the player box
	for( i = playerMins[0]; i < playerMaxs[0]; i += 4 )
	{
		for( j = playerMins[1]; j < playerMaxs[1]; j += 4 )
		{
			for( k = playerMins[2]; k < playerMaxs[2]; k += 4 )
			{
				p = CG_AllocParticle();
				if( !p )
				{
					return;
				}
				
				p->flags = PF_AIRONLY;
				p->endTime = cg.time + 700 + random() * 500;
				
				randVec[0] = origin[0] + i + ( rand() & 3 );
				randVec[1] = origin[1] + j + ( rand() & 3 );
				randVec[2] = origin[2] + k + ( rand() & 3 );
				VectorCopy( randVec, p->org );
				
				randVec[0] = crandom();	// between 1 and -1
				randVec[1] = crandom();
				randVec[2] = crandom();
				VectorNormalize( randVec );
				VectorScale( randVec, 64, tempVec );
				//tempVec[2] += 30;     // nudge the particles up a bit
				VectorCopy( tempVec, p->vel );
				
				// add some gravity/randomness
				p->accel[0] = crandom() * 3;
				p->accel[1] = crandom() * 3;
				p->accel[2] = -PARTICLE_GRAVITY * 3;
				
				p->color[3] = 1.0;
				p->colorVel[3] = 0;
				
				p->type = P_SMOKE;
				p->pshader = cgs.media.teleportFlareShader;
				
				p->width = 3 + random() * 2;
				p->height = p->width;
				
				p->endHeight = p->width * 0.2;
				p->endWidth = p->height * 0.2;
				
				p->startfade = cg.time;
				p->rotate = qtrue;
				p->roll = rand() % 179;
				
				p->bounceFactor = 0.6f;
				VectorCopy( p->org, p->oldOrg );	// necessary for coldet
			}
		}
	}
}

/*
==========================
CG_ParticleGibEffect
==========================
*/
void CG_ParticleGibEffect( const vec3_t origin )
{
	int             i, j, k;
	vec3_t          randVec, dir;
	cparticle_t*    p;
	
	// create particles inside the player box
	for( i = playerMins[0]; i < playerMaxs[0]; i += 4 )
	{
		for( j = playerMins[1]; j < playerMaxs[1]; j += 4 )
		{
			for( k = playerMins[2]; k < playerMaxs[2]; k += 4 )
			{
				p = CG_AllocParticle();
				if( !p )
				{
					return;
				}
				
				p->flags = PF_AIRONLY;
				
				randVec[0] = origin[0] + i + ( rand() & 3 );
				randVec[1] = origin[1] + j + ( rand() & 3 );
				randVec[2] = origin[2] + k + ( rand() & 3 );
				VectorCopy( randVec, p->org );
				
				VectorSubtract( randVec, origin, dir );
				VectorNormalize( dir );
				VectorScale( dir, 300.0f, p->vel );
				
				p->color[0] = 1.0;
				p->color[1] = 1.0;
				p->color[2] = 1.0;
				p->color[3] = 1.0;
				p->colorVel[3] = 0;
				
#if 1
				p->endTime = cg.time + 700 + random() * 500;
				p->type = P_SMOKE;
				p->pshader = cgs.media.teleportFlareShader;
				
				p->width = 3 + random() * 2;
				p->height = p->width;
				
				p->endHeight = p->width * 0.2;
				p->endWidth = p->height * 0.2;
				
				// add some gravity/randomness
				p->accel[0] = 0;	//crandom() * 3;
				p->accel[1] = 0;	//crandom() * 3;
				p->accel[2] = -PARTICLE_GRAVITY * 3;
				
				p->rotate = qtrue;
				p->roll = rand() % 179;
#else
				
				p->endTime = cg.time + 20000;
				p->type = P_LIGHTSPARK;
				p->pshader = cgs.media.sparkShader;
				
				p->height = 0.7f;
				p->width = 3.0f;
				p->endHeight = 1.3f;
				p->endWidth = 10;
				
				p->accel[0] = 0;	//crandom() * 3;
				p->accel[1] = 0;	//crandom() * 3;
				p->accel[2] = -PARTICLE_GRAVITY * 20;
#endif
				
				p->startfade = cg.time;
				
				p->bounceFactor = 0.6f;
				VectorCopy( p->org, p->oldOrg );	// necessary for coldet
			}
		}
	}
}


/*
===============
CG_ParticleRocketFire
===============
*/
void CG_ParticleRocketFire( vec3_t start, vec3_t end )
{
	vec3_t          move;
	vec3_t          vec;
	float           len;
	int             j;
	cparticle_t*    p;
	int             dec;
	
	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );
	
	dec = 1;
	VectorScale( vec, dec, vec );
	
	// FIXME: this is a really silly way to have a loop
	while( len > 0 )
	{
		len -= dec;
		
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		VectorClear( p->accel );
		p->flags = 0;
		
		p->time = cg.time;
		p->endTime = cg.time + 20000;
		
		//p->blend_dst = GL_ONE;
		//p->blend_src = GL_ONE;
		
		p->color[3] = 1.0;
		p->colorVel[3] = -2.0 / ( 0.2 + random() * 0.1 );
		
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		
		p->type = P_SMOKE;
		p->pshader = cgs.media.teleportFlareShader;
		p->width = p->height = 1;
//      p->size = 1;
//      p->sizeVel = 9;

		for( j = 0; j < 3; j++ )
		{
			p->org[j] = move[j] + crandom();
			p->accel[j] = 0;
			p->vel[j] = crandom() * 5;
		}
		
		/*
		   cont = (CL_PMpointcontents(p->org) & MASK_WATER);
		
		   if(cont)
		   {
		   CL_BubbleTrail(start, end);
		   }
		 */
		
		VectorAdd( move, vec, move );
	}
	
}

void CG_ParticleRick( vec3_t org, vec3_t dir )
{
	float           d;
	int             j, i;
	cparticle_t*    p;
	
//  VectorNormalize(dir);

#if 1
	// SPARKS
	for( i = 0; i < 3; i++ )
	{
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		VectorClear( p->accel );
		VectorClear( p->vel );
		
		p->flags = PF_AIRONLY;
		
		p->time = cg.time;
		p->endTime = cg.time + 20000;
		
		p->type = P_SPARK;
		p->pshader = cgs.media.sparkShader;
		
		p->height = 0.7f;
		p->width = 7;
		p->endHeight = 1.3f;
		p->endWidth = 30;
		
		p->color[0] = 1;
		p->color[1] = 0.7f;
		p->color[2] = 0;
		p->color[3] = 0.3f;
		
		p->colorVel[0] = 0;
		p->colorVel[1] = -1;
		p->colorVel[2] = 0;
		p->colorVel[3] = -1.0 / ( 0.7 + random() * 0.2 );
		
		d = rand() & 5;
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + d * dir[j];
			p->vel[j] = dir[j] * 30 + crandom() * 10;
		}
		VectorCopy( p->org, p->oldOrg );
		
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
	}
#endif
	
#if 1
	// SMOKE
	for( i = 0; i < 6; i++ )
	{
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		p->flags = PF_AIRONLY;
		p->time = cg.time;
		p->endTime = cg.time + 20000;
		//p->endTime = cg.time + 500;
		
		p->color[0] = 0.1f;
		p->color[1] = 0.1f;
		p->color[2] = 0.1f;
		p->color[3] = 0.60f;
		
		p->colorVel[0] = 0.7f;
		p->colorVel[1] = 0.7f;
		p->colorVel[2] = 0.7f;
		p->colorVel[3] = -1.0 / ( 0.5 + random() * 0.5 );
		
		p->type = P_SMOKE_IMPACT;
		p->pshader = cgs.media.smokePuffShader;
		
		p->width = 5.0f * ( 1.0f + random() * 0.5f );
		p->height = 5.0f * ( 1.0f + random() * 0.5f );
		
		p->endHeight = p->height * 2;
		p->endWidth = p->width * 2;
		
		//p->size = 5;
		//p->sizeVel = 11;
		
		d = rand() & 15;
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + ( ( rand() & 7 ) - 4 ) + d * dir[j];
			p->vel[j] = dir[j] * 30;
		}
		
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = 15;
	}
#endif
}

void CG_ParticleRailRick( vec3_t org, vec3_t dir, vec3_t clientColor )
{
	float           d;
	int             j, i;
	cparticle_t*    p;
	
//  VectorNormalize(dir);

	// SPARKS
	for( i = 0; i < 25; i++ )
	{
		p = CG_AllocParticle();
		if( !p )
		{
			return;
		}
		
		VectorClear( p->accel );
		VectorClear( p->vel );
		
		p->flags = PF_AIRONLY;
		
		p->time = cg.time;
		
		
#if 0
		p->endTime = cg.time + 20000;
		p->type = P_SPARK;
		p->pshader = cgs.media.sparkShader;
		p->height = 0.7f;
		p->width = 7;
		p->endHeight = 1.3f;
		p->endWidth = 30;
#else
		p->endTime = cg.time + 700 + random() * 500;
		p->type = P_SMOKE;
		p->pshader = cgs.media.teleportFlareShader;
		p->width = 3 + random() * 2;
		p->height = p->width;
		p->endHeight = p->width * 0.1;
		p->endWidth = p->height * 0.1;
#endif
		
		p->color[0] = clientColor[0];
		p->color[1] = clientColor[1];
		p->color[2] = clientColor[2];
		p->color[3] = 0.3f;
		
		//p->colorVel[0] = 0;
		//p->colorVel[1] = -1;
		//p->colorVel[2] = 0;
		//p->colorVel[3] = -1.0 / (0.3 + random() * 0.2);
		
		d = rand() & 5;
		for( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j] + d * dir[j];
			p->vel[j] = dir[j] * 90 + crandom() * 50;
		}
		
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 3;
		
		p->bounceFactor = 0.6f;
		VectorCopy( p->org, p->oldOrg );	// necessary for coldet
	}
}


/*
=================
CG_TestParticles_f
=================
*/
void CG_TestParticles_f( void )
{
	vec3_t          start, end;
	
	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], start );
	VectorMA( start, 20, cg.refdef.viewaxis[1], end );
	
//  CG_ParticleTeleportEffect(origin);
//  CG_ParticleBloodCloud(cg.testModelEntity.origin, cg.refdef.viewaxis[0]);
//  CG_BloodPool(cgs.media.bloodSpurtShader, cg.testModelEntity.origin);
//  CG_ParticleRocketFire(start, end);
//  CG_ParticleSparks2(start, cg.refdef.viewaxis[1], 50);
//  CG_ParticleRick(start, cg.refdef.viewaxis[1]);
//  CG_ParticleBlood(start, cg.refdef.viewaxis[1], 3);
	CG_ParticleGibEffect( start );
}
