/*
===========================================================================
Copyright (C) 1998 Steve Yeager
Copyright (C) 2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
//  acebot_movement.c - This file contains all of the
//                      movement routines for the ACE bot



#include "../g_local.h"
#include "acebot.h"

#if defined(ACEBOT)

// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime.
qboolean ACEMV_CanMove( gentity_t* self, int direction )
{
#if 1
	vec3_t          forward, right;
	vec3_t          offset, start, end;
	vec3_t          angles;
	trace_t         tr;
	
	// airborne
	if( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return qtrue;
	}
	
	// now check to see if move will move us off an edge
	VectorCopy( self->bs.viewAngles, angles );
	
	if( direction == MOVE_LEFT )
	{
		angles[YAW] += 90;
	}
	else if( direction == MOVE_RIGHT )
	{
		angles[YAW] -= 90;
	}
	else if( direction == MOVE_BACK )
	{
		angles[YAW] -= 180;
	}
	
	// set up the vectors
	AngleVectors( angles, forward, right, NULL );
	
	VectorSet( offset, 36, 0, 24 );
	G_ProjectSource( self->client->ps.origin, offset, forward, right, start );
	
	VectorSet( offset, 36, 0, -400 );
	G_ProjectSource( self->client->ps.origin, offset, forward, right, end );
	
	trap_Trace( &tr, start, NULL, NULL, end, self->s.number, MASK_PLAYERSOLID );
	
//  if((tr.fraction > 0.3 && tr.fraction != 1) || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
	if( ( tr.fraction == 1.0 ) || ( tr.contents & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_DONOTENTER ) ) )
	{
		if( ace_debug.integer )
		{
			trap_SendServerCommand( -1, va( "print \"%s:" S_COLOR_WHITE " move blocked\n\"", self->client->pers.netname ) );
		}
		return qfalse;
	}
	
	return qtrue;				// yup, can move
#else
	return qtrue;
#endif
}

// Handle special cases of crouch/jump
//
// If the move is resolved here, this function returns
// true.
qboolean ACEMV_SpecialMove( gentity_t* self )
{
	vec3_t          dir, forward, right, start, end, offset;
	vec3_t          top;
	trace_t         tr;
	
	// Get current direction
	VectorCopy( self->client->ps.viewangles, dir );
	dir[YAW] = self->bs.viewAngles[YAW];
	AngleVectors( dir, forward, right, NULL );
	
	VectorSet( offset, 18, 0, 0 );
	G_ProjectSource( self->client->ps.origin, offset, forward, right, start );
	offset[0] += 18;
	G_ProjectSource( self->client->ps.origin, offset, forward, right, end );
	
	// trace it
	start[2] += 18;				// so they are not jumping all the time
	end[2] += 18;
	
	//tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);
	trap_Trace( &tr, start, self->r.mins, self->r.maxs, end, self->s.number, MASK_PLAYERSOLID );
	
	if( tr.allsolid )
	{
		// check for crouching
		start[2] -= 14;
		end[2] -= 14;
		
		// set up for crouching check
		VectorCopy( self->r.maxs, top );
		top[2] = 0.0;			// crouching height
		
		//tr = gi.trace(start, self->mins, top, end, self, MASK_PLAYERSOLID);
		trap_Trace( &tr, start, self->r.mins, self->r.maxs, top, self->s.number, MASK_PLAYERSOLID );
		
		// crouch
		if( !tr.allsolid )
		{
			if( ACEMV_CanMove( self, MOVE_FORWARD ) )
			{
				self->client->pers.cmd.forwardmove = 127;
			}
			self->client->pers.cmd.upmove = -127;
			return qtrue;
		}
		
		// check for jump
		start[2] += 32;
		end[2] += 32;
		
		//tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);
		trap_Trace( &tr, start, self->r.mins, self->r.maxs, end, self->s.number, MASK_PLAYERSOLID );
		
		if( !tr.allsolid )
		{
			if( ACEMV_CanMove( self, MOVE_FORWARD ) )
			{
				self->client->pers.cmd.forwardmove = 127;
			}
			self->client->pers.cmd.upmove = 127;
			return qtrue;
		}
	}
	
	return qfalse;				// We did not resolve a move here
}



// Checks for obstructions in front of bot
//
// This is a function I created origianlly for ACE that
// tries to help steer the bot around obstructions.
//
// If the move is resolved here, this function returns true.
static qboolean ACEMV_CheckEyes( gentity_t* self )
{
	vec3_t          forward, right;
	vec3_t          leftstart, rightstart, focalpoint;
	vec3_t          upstart, upend;
	vec3_t          dir, offset;
	
	trace_t         traceRight, traceLeft, traceUp, traceFront;	// for eyesight
	gentity_t*      traceEntity;
	
	// get current angle and set up "eyes"
	VectorCopy( self->bs.viewAngles, dir );
	AngleVectors( dir, forward, right, NULL );
	
	// let them move to targets by walls
	if( !self->bs.moveTarget )
	{
		VectorSet( offset, 200, 0, 4 );    // focalpoint
	}
	else
	{
		VectorSet( offset, 36, 0, 4 );    // focalpoint
	}
	
	G_ProjectSource( self->client->ps.origin, offset, forward, right, focalpoint );
	
	// Check from self to focalpoint
	// Ladder code
	VectorSet( offset, 36, 0, 0 );	// set as high as possible
	G_ProjectSource( self->client->ps.origin, offset, forward, right, upend );
	
	trap_Trace( &traceFront, self->client->ps.origin, self->r.mins, self->r.maxs, upend, self->s.number, MASK_PLAYERSOLID );
	
	/*
	   if(traceFront.contents & CONTENTS_DETAIL)    // using detail brush here cuz sometimes it does not pick up ladders...??
	   {
	   self->client->pers.cmd.upmove = 127;
	   if(ACEMV_CanMove(self, MOVE_FORWARD))
	   self->client->pers.cmd.forwardmove = 127;
	   return qtrue;
	   }
	 */
	
	// if this check fails we need to continue on with more detailed checks
	if( traceFront.fraction == 1 )
	{
		if( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			self->client->pers.cmd.forwardmove = 127;
		}
		return qtrue;
	}
	
	VectorSet( offset, 0, 18, 4 );
	G_ProjectSource( self->client->ps.origin, offset, forward, right, leftstart );
	
	offset[1] -= 36;			// want to make sure this is correct
	//VectorSet(offset, 0, -18, 4);
	G_ProjectSource( self->client->ps.origin, offset, forward, right, rightstart );
	
	trap_Trace( &traceRight, rightstart, NULL, NULL, focalpoint, self->s.number, MASK_PLAYERSOLID );
	trap_Trace( &traceLeft, leftstart, NULL, NULL, focalpoint, self->s.number, MASK_PLAYERSOLID );
	
	// wall checking code, this will degenerate progressivly so the least cost
	// check will be done first.
	
	// if open space move ok
	if( traceLeft.entityNum != ENTITYNUM_WORLD )
	{
		traceEntity = &g_entities[traceLeft.entityNum];
	}
	else
	{
		traceEntity = NULL;
	}
	
	if( traceRight.fraction != 1 || traceLeft.fraction != 1 )	// || (traceEntity && strcmp(traceEntity->classname, "func_door") != 0))
	{
		// special uppoint logic to check for slopes/stairs/jumping etc.
		VectorSet( offset, 0, 18, 24 );
		G_ProjectSource( self->client->ps.origin, offset, forward, right, upstart );
		
		VectorSet( offset, 0, 0, 200 );	// scan for height above head
		G_ProjectSource( self->client->ps.origin, offset, forward, right, upend );
		trap_Trace( &traceUp, upstart, NULL, NULL, upend, self->s.number, MASK_PLAYERSOLID );
		
		VectorSet( offset, 200, 0, 200 * traceUp.fraction - 5 );	// set as high as possible
		G_ProjectSource( self->client->ps.origin, offset, forward, right, upend );
		trap_Trace( &traceUp, upstart, NULL, NULL, upend, self->s.number, MASK_PLAYERSOLID );
		
		// If the upper trace is not open, we need to turn.
		if( traceUp.fraction != 1 )
		{
			if( traceRight.fraction > traceLeft.fraction )
			{
				self->bs.viewAngles[YAW] += ( 1.0 - traceLeft.fraction ) * 45.0;
			}
			else
			{
				self->bs.viewAngles[YAW] += -( 1.0 - traceRight.fraction ) * 45.0;
			}
			
			if( ACEMV_CanMove( self, MOVE_FORWARD ) )
			{
				self->client->pers.cmd.forwardmove = 127;
			}
			return qtrue;
		}
	}
	
	return qfalse;
}

// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
//
// Modified from the original id ChangeYaw code...
void ACEMV_ChangeBotAngle( gentity_t* ent )
{
#if 1
	vec3_t          ideal_angles;
	float           ideal_yaw;
	float           ideal_pitch;
	float           current_yaw;
	float           current_pitch;
	float           move;
	float           speed;
	
	// Normalize the move angle first
	VectorNormalize( ent->bs.moveVector );
	
	current_yaw = AngleNormalize360( ent->bs.viewAngles[YAW] );
	current_pitch = AngleNormalize360( ent->bs.viewAngles[PITCH] );
	
	VectorToAngles( ent->bs.moveVector, ideal_angles );
	
	ideal_yaw = AngleNormalize360( ideal_angles[YAW] );
	ideal_pitch = AngleNormalize360( ideal_angles[PITCH] );
	
	// yaw
	if( current_yaw != ideal_yaw )
	{
		move = ideal_yaw - current_yaw;
		speed = ent->bs.turnSpeed;
		if( ideal_yaw > current_yaw )
		{
			if( move >= 180 )
			{
				move = move - 360;
			}
		}
		else
		{
			if( move <= -180 )
			{
				move = move + 360;
			}
		}
		if( move > 0 )
		{
			if( move > speed )
			{
				move = speed;
			}
		}
		else
		{
			if( move < -speed )
			{
				move = -speed;
			}
		}
		ent->bs.viewAngles[YAW] = AngleNormalize360( current_yaw + move );
	}
	
	// pitch
	if( current_pitch != ideal_pitch )
	{
		move = ideal_pitch - current_pitch;
		speed = ent->bs.turnSpeed;
		if( ideal_pitch > current_pitch )
		{
			if( move >= 180 )
			{
				move = move - 360;
			}
		}
		else
		{
			if( move <= -180 )
			{
				move = move + 360;
			}
		}
		if( move > 0 )
		{
			if( move > speed )
			{
				move = speed;
			}
		}
		else
		{
			if( move < -speed )
			{
				move = -speed;
			}
		}
		ent->bs.viewAngles[PITCH] = AngleNormalize360( current_pitch + move );
	}
#else
	
#endif
}


// Set bot to move to it's moveTarget. (following node path)
void ACEMV_MoveToGoal( gentity_t* self )
{
	// if a rocket or grenade is around deal with it
	// simple, but effective (could be rewritten to be more accurate)
	if( ( strcmp( self->bs.moveTarget->classname, "rocket" ) == 0 || strcmp( self->bs.moveTarget->classname, "grenade" ) == 0 ) )
	{
		VectorSubtract( self->bs.moveTarget->s.origin, self->client->ps.origin, self->bs.moveVector );
		ACEMV_ChangeBotAngle( self );
		
		if( ace_debug.integer )
		{
			trap_SendServerCommand( -1, va( "print \"%s: Oh crap a rocket!\n\"", self->client->pers.netname ) );
		}
		
		//trap_SendServerCommand(-1, va("%s \"%s%c%c%s\"", mode == SAY_TEAM ? "tchat" : "chat", name, Q_COLOR_ESCAPE, color, message));
		
		// strafe left/right
		if( rand() % 1 && ACEMV_CanMove( self, MOVE_LEFT ) )
		{
			self->client->pers.cmd.rightmove = -127;
		}
		else if( ACEMV_CanMove( self, MOVE_RIGHT ) )
		{
			self->client->pers.cmd.rightmove = 127;
		}
		return;
		
	}
	else
	{
		// set bot's movement direction
		VectorSubtract( self->bs.moveTarget->s.origin, self->client->ps.origin, self->bs.moveVector );
		ACEMV_ChangeBotAngle( self );
		
		if( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			self->client->pers.cmd.forwardmove = 127;
		}
		return;
	}
}

// Main movement code. (following node path)
void ACEMV_Move( gentity_t* self )
{
	int             currentNodeType = -1;
	int             nextNodeType = -1;
	
	// get current and next node back from nav code.
	if( !ACEND_FollowPath( self ) )
	{
		self->bs.state = STATE_WANDER;
		self->bs.wander_timeout = level.time + 1000;
		
		// center view
		//self->bs.viewAngles[PITCH] = 0;   //-self->client->ps.delta_angles[PITCH];
		return;
	}
	
	currentNodeType = nodes[self->bs.currentNode].type;
	nextNodeType = nodes[self->bs.nextNode].type;
	
	// move to a selected goal, if any
	if( self->bs.moveTarget )
	{
		ACEMV_MoveToGoal( self );
	}
	
	// grapple
	/*
	   if(nextNodeType == NODE_GRAPPLE)
	   {
	   ACEMV_ChangeBotAngle(self);
	   ACEIT_ChangeWeapon(self, FindItem("grapple"));
	   self->client->pers.cmd.buttons = BUTTON_ATTACK;
	   return;
	   }
	   // Reset the grapple if hangin on a graple node
	   if(currentNodeType == NODE_GRAPPLE)
	   {
	   CTFPlayerResetGrapple(self);
	   return;
	   }
	 */
	
#if 0
	// check for platforms
	if( currentNodeType != NODE_PLATFORM && nextNodeType == NODE_PLATFORM )
	{
		// check to see if lift is down?
		for( i = 0; i < num_items; i++ )
			if( item_table[i].node == self->bs.nextNode )
				if( item_table[i].ent->moverState != MOVER_POS1 )
				{
					return;    // Wait for elevator
				}
	}
#endif
	
	if( currentNodeType == NODE_PLATFORM && nextNodeType == NODE_PLATFORM )
	{
		// move to the center
		self->bs.moveVector[2] = 0;	// kill z movement
		
		if( VectorLength( self->bs.moveVector ) > 10 )
		{
			self->client->pers.cmd.forwardmove = 200;    // walk to center
		}
		
		ACEMV_ChangeBotAngle( self );
		
		return;					// No move, riding elevator
	}
	
	// jumpto nodes
	if( nextNodeType == NODE_JUMP ||
			( currentNodeType == NODE_JUMP && nextNodeType != NODE_ITEM &&
			  nodes[self->bs.nextNode].origin[2] > self->client->ps.origin[2] ) )
	{
		// set up a jump move
		if( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			self->client->pers.cmd.forwardmove = 127;
		}
		
		self->client->pers.cmd.upmove = 127;
		
		ACEMV_ChangeBotAngle( self );
		
		//VectorCopy(self->bs.moveVector, dist);
		//VectorScale(dist, 127, self->client->ps.velocity);
		return;
	}
	
	
	// ladder nodes
	/*
	   if(nextNodeType == NODE_LADDER && nodes[self->nextNode].origin[2] > self->s.origin[2])
	   {
	   // Otherwise move as fast as we can
	   self->client->pers.cmd.forwardmove = 400;
	   self->velocity[2] = 320;
	
	   ACEMV_ChangeBotAngle(self);
	
	   return;
	
	   }
	   // If getting off the ladder
	   if(currentNodeType == NODE_LADDER && nextNodeType != NODE_LADDER && nodes[self->nextNode].origin[2] > self->s.origin[2])
	   {
	   self->client->pers.cmd.forwardmove = 400;
	   self->client->pers.cmd.upmove = 200;
	   self->velocity[2] = 200;
	   ACEMV_ChangeBotAngle(self);
	   return;
	   }
	 */
	
	// water nodes
	if( currentNodeType == NODE_WATER )
	{
		// we need to be pointed up/down
		ACEMV_ChangeBotAngle( self );
		
		// ff the next node is not in the water, then move up to get out.
		if( nextNodeType != NODE_WATER && !( trap_PointContents( nodes[self->bs.nextNode].origin, self->s.number ) & MASK_WATER ) )
		{
			// exit water
			self->client->pers.cmd.upmove = 127;
		}
		
		self->client->pers.cmd.forwardmove = 100;
		return;
		
	}
	
	// falling off ledge?
	if( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		ACEMV_ChangeBotAngle( self );
		
		//self->client->ps.velocity[0] = self->bs.moveVector[0] * 360;
		//self->client->ps.velocity[1] = self->bs.moveVector[1] * 360;
		return;
	}
	
	// check to see if stuck, and if so try to free us
	// also handles crouching
	if( VectorLength( self->client->ps.velocity ) < 37 )
	{
		// keep a random factor just in case....
		if( random() > 0.1 && ACEMV_SpecialMove( self ) )
		{
			return;
		}
		
		self->bs.viewAngles[YAW] += random() * 180 - 90;
		
		if( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			self->client->pers.cmd.forwardmove = 127;
		}
		else if( ACEMV_CanMove( self, MOVE_BACK ) )
		{
			self->client->pers.cmd.forwardmove = -127;
		}
		return;
	}
	
	// otherwise move as fast as we can
	if( ACEMV_CanMove( self, MOVE_FORWARD ) )
	{
		self->client->pers.cmd.forwardmove = 127;
	}
	
	ACEMV_ChangeBotAngle( self );
}


// Wandering code (based on old ACE movement code)
void ACEMV_Wander( gentity_t* self )
{
	vec3_t          tmp;
	
	// do not move
	if( self->bs.next_move_time > level.time )
	{
		return;
	}
	
	// Special check for elevators, stand still until the ride comes to a complete stop.
	/*
	 * FIXME
	 if(self->groundentity != NULL && self->groundentity->use == Use_Plat)
	 if(self->groundentity->moveinfo.state == STATE_UP || self->groundentity->moveinfo.state == STATE_DOWN) // only move when platform not
	 {
	 self->velocity[0] = 0;
	 self->velocity[1] = 0;
	 self->velocity[2] = 0;
	 self->next_move_time = level.time + 500;
	 return;
	 }
	
	 */
	
	// touched jumppad last Frame?
	if( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		if( VectorLength( self->client->ps.velocity ) > 120 )
		{
			VectorNormalize2( self->client->ps.velocity, tmp );
			
			if( AngleBetweenVectors( self->bs.moveVector, tmp ) >= 120 )
			{
				// we might have been knocked back by someone or something ..
				if( !self->bs.moveTarget )
				{
					VectorCopy( tmp, self->bs.moveVector );
					ACEMV_ChangeBotAngle( self );
				}
			}
		}
		
		//ACEMV_ChangeBotAngle(self);
		//self->client->ps.velocity[0] = self->bs.moveVector[0] * 360;
		//self->client->ps.velocity[1] = self->bs.moveVector[1] * 360;
		//return;
	}
	
	// is there a target to move to
	if( self->bs.moveTarget )
	{
		ACEMV_MoveToGoal( self );
	}
	
	// swimming?
	VectorCopy( self->client->ps.origin, tmp );
	tmp[2] += 24;
	
	if( trap_PointContents( tmp, self->s.number ) & MASK_WATER )
	{
		// if drowning and no node, move up
		if( self->client->airOutTime > 0 )
		{
			self->client->pers.cmd.upmove = 1;
			self->bs.viewAngles[PITCH] = -45;
		}
		else
		{
			self->client->pers.cmd.upmove = 15;
		}
		
		self->client->pers.cmd.forwardmove = 100;
	}
	else
	{
		//self->client->airOutTime = 0;    // probably shound not be messing with this, but
	}
	
	// lava?
	tmp[2] -= 48;
	if( trap_PointContents( tmp, self->s.number ) & ( CONTENTS_LAVA | CONTENTS_SLIME ) )
	{
		//  safe_bprintf(PRINT_MEDIUM,"lava jump\n");
		self->bs.viewAngles[YAW] += random() * 360 - 180;
		self->client->pers.cmd.forwardmove = 127;
		self->client->pers.cmd.upmove = 127;
		return;
	}
	
	// check for special movement if we have a normal move (have to test)
	if( VectorLength( self->client->ps.velocity ) < 37 )
	{
		//if(random() > 0.1 && ACEMV_SpecialMove(self))
		//  return; //removed this because when wandering, the last thing you want is bots jumping
		//over things and going off ledges.  It's better for them to just bounce around the map.
		
		self->bs.viewAngles[YAW] += random() * 180 - 90;
		
		if( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			self->client->pers.cmd.forwardmove = 127;
		}
		else if( ACEMV_CanMove( self, MOVE_BACK ) )
		{
			self->client->pers.cmd.forwardmove = -127;
		}
		
		// if there is ground continue otherwise wait for next move
		if( /*!M_CheckBottom || */ self->s.groundEntityNum != ENTITYNUM_NONE )
		{
			if( ACEMV_CanMove( self, MOVE_FORWARD ) )
			{
				self->client->pers.cmd.forwardmove = 127;
			}
		}
		
		return;
	}
	
	if( ACEMV_CheckEyes( self ) )
	{
		return;
	}
	
	if( ACEMV_CanMove( self, MOVE_FORWARD ) )
	{
		self->client->pers.cmd.forwardmove = 127;
	}
}


qboolean ACEMV_CheckShot( gentity_t* self, vec3_t point )
{
	trace_t         tr;
	vec3_t          start, forward, right, offset;
	
	AngleVectors( self->bs.viewAngles, forward, right, NULL );
	
	VectorSet( offset, 8, 8, self->client->ps.viewheight - 8 );
	G_ProjectSource( self->client->ps.origin, offset, forward, right, start );
	
	// blocked, don't shoot
	
	trap_Trace( &tr, start, NULL, NULL, point, self->s.number, MASK_SOLID );
	if( tr.fraction < 0.3 )		//just enough to prevent self damage (by now)
	{
		return qfalse;
	}
	
	return qtrue;
}


// Attack movement routine
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
void ACEMV_Attack( gentity_t* self )
{
	float           c;
	vec3_t          target, forward, right, up;
	float           distance;
	vec3_t          oldAimVec;
	float           aimTremble[2] = { 0.11f, 0.11f };
	//float           slowness = 0.35;	//lower is slower
	
	// randomly choose a movement direction
	c = random();
	
	//if(self->s.groundEntityNum != ENTITYNUM_NONE)
	{
		if( c < 0.2 && ACEMV_CanMove( self, MOVE_LEFT ) )
		{
			self->client->pers.cmd.rightmove -= 127;
		}
		else if( c < 0.4 && ACEMV_CanMove( self, MOVE_RIGHT ) )
		{
			self->client->pers.cmd.rightmove += 127;
		}
		
		if( c < 0.6 && ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			self->client->pers.cmd.forwardmove += 127;
		}
		else if( c < 0.8 && ACEMV_CanMove( self, MOVE_FORWARD ) && self->client->ps.weapon != WP_GAUNTLET )
		{
			self->client->pers.cmd.forwardmove -= 127;
		}
		
		if( c < 0.95 && self->client->ps.weapon != WP_GAUNTLET )
		{
			self->client->pers.cmd.upmove -= 90;
		}
		else
		{
			self->client->pers.cmd.upmove += 90;
		}
	}
	
	// aim
	if( self->enemy->client )
	{
		VectorCopy( self->enemy->client->ps.origin, target );
	}
	else
	{
		VectorCopy( self->enemy->s.origin, target );
	}
	
	// modify attack angles based on accuracy (mess this up to make the bot's aim not so deadly)
	
	// save the current angles
	VectorCopy( self->bs.moveVector, oldAimVec );
	VectorNormalize( oldAimVec );
	
	VectorSubtract( target, self->client->ps.origin, forward );
	distance = VectorNormalize( forward );
	
	PerpendicularVector( up, forward );
	CrossProduct( up, forward, right );
	
	VectorMA( forward, crandom() * aimTremble[0], up, forward );
	VectorMA( forward, crandom() * aimTremble[1], right, forward );
	VectorNormalize( forward );
	
	//VectorLerp(oldAimVec, forward, slowness, forward);
	//VectorMA(oldAimVec, slowness, forward, forward);
	//VectorNormalize(forward);
	
	VectorScale( forward, distance, self->bs.moveVector );
	//ACEMV_ChangeBotAngle(self);
	VectorToAngles( self->bs.moveVector, self->bs.viewAngles );
	
	
	// don't attack too much
	if( random() < 0.8 && ACEMV_CheckShot( self, target ) )
	{
		self->client->pers.cmd.buttons = BUTTON_ATTACK;
	}
	
//  if(ace_debug.integer)
//      trap_SendServerCommand(-1, va("print \"%s: attacking %s\n\"", self->client->pers.netname, self->enemy->client->pers.netname));
}

#endif
