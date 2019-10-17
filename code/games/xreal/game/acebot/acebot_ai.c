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
//  acebot_ai.c -      This file contains all of the
//                     AI routines for the ACE II bot.



#include "../g_local.h"
#include "acebot.h"

#if defined(ACEBOT)


/*
ACEAI_StartFrame
Simililar to int BotAIStartFrame(int time)
will be called each bot frame after running G_RunFrame
so entities like items can settle down in the level before we think
about pathing the level
*/

void ACEAI_StartFrame( int time )
{
	//G_Printf("ACEAI_StartFrame()\n");
	
	ACEIT_BuildItemNodeTable( qtrue );
}

void ACEAI_CheckServerCommands( gentity_t* self )
{
	char            buf[1024];
	
	while( trap_BotGetServerCommand( self->client - level.clients, buf, sizeof( buf ) ) )
	{
#if 0
		//have buf point to the command and args to the command arguments
		if( ace_debug.integer )
		{
			G_Printf( "ACEAI_CheckServerCommands for %s: '%s'\n", self->client->pers.netname, buf );
		}
		
		// TODO check for orders by team mates
#endif
	}
}

void ACEAI_CheckSnapshotEntities( gentity_t* self )
{
	int             sequence, entnum;
	
	// parse through the bot's list of snapshot entities and scan each of them
	sequence = 0;
	while( ( entnum = trap_BotGetSnapshotEntity( self - g_entities, sequence++ ) ) >= 0 )	// && (entnum < MAX_CLIENTS))
		;						//BotScanEntity(bs, &g_entities[entnum], &scan, scan_mode);
}

/*
=============
ACEAI_Think

Main Think function for bot
=============
*/
void ACEAI_Think( gentity_t* self )
{
	int             i;
	int             clientNum;
	char            userinfo[MAX_INFO_STRING];
	char*           team;
	
	//if(ace_debug.integer)
	//  G_Printf("ACEAI_Think(%s)\n", self->client->pers.netname);
	
	clientNum = self->client - level.clients;
	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	
	// is the bot part of a team when gameplay has changed?
	if( self->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		if( ace_debug.integer )
		{
			trap_SendServerCommand( -1, va( "print \"%s: I am a spectator, choosing a team...\n\"", self->client->pers.netname ) );
		}
		
		team = Info_ValueForKey( userinfo, "team" );
		if( !team || !*team )
		{
			if( g_gametype.integer >= GT_TEAM )
			{
				if( PickTeam( clientNum ) == TEAM_RED )
				{
					team = "red";
				}
				else
				{
					team = "blue";
				}
			}
			else
			{
				team = "red";
			}
		}
		
		// this sets the team status and updates the userinfo as well
		trap_BotClientCommand( self - g_entities, va( "team %s", team ) );
		return;
	}
	
	// set up client movement
	VectorCopy( self->client->ps.viewangles, self->bs.viewAngles );
	VectorSet( self->client->ps.delta_angles, 0, 0, 0 );
	
	// FIXME: needed?
	memset( &self->client->pers.cmd, 0, sizeof( self->client->pers.cmd ) );
	
	self->enemy = NULL;
	self->bs.moveTarget = NULL;
	
	// do this to avoid a time out
	ACEAI_CheckServerCommands( self );
	
	ACEAI_CheckSnapshotEntities( self );
	
	// force respawn
	if( self->health <= 0 )
	{
		self->client->buttons = 0;
		self->client->pers.cmd.buttons = BUTTON_ATTACK;
	}
	
	if( self->bs.state == STATE_WANDER && self->bs.wander_timeout < level.time )
	{
		// pick a new long range goal
		ACEAI_PickLongRangeGoal( self );
	}
	
#if 0
	// kill the bot if completely stuck somewhere
	if( VectorLength( self->client->ps.velocity ) > 37 )	//
	{
		self->bs.suicide_timeout = level.time + 10000;
	}
	
	if( self->bs.suicide_timeout < level.time )
	{
		self->client->ps.stats[STAT_HEALTH] = self->health = -999;
		player_die( self, self, self, 100000, MOD_SUICIDE );
	}
#endif
	
	// find any short range goal
	ACEAI_PickShortRangeGoal( self );
	
	// look for enemies
	if( ACEAI_FindEnemy( self ) )
	{
		ACEAI_ChooseWeapon( self );
		ACEMV_Attack( self );
	}
	else
	{
		// execute the move, or wander
		if( self->bs.state == STATE_WANDER )
		{
			ACEMV_Wander( self );
		}
		else if( self->bs.state == STATE_MOVE )
		{
			ACEMV_Move( self );
		}
	}
	
#if 0
	if( ace_debug.integer )
	{
		trap_SendServerCommand( -1, va( "print \"%s: state %dl!\n\"", self->client->pers.netname, self->bs.state ) );
	}
#endif
	
	// set approximate ping
	if( !g_synchronousClients.integer )
	{
		self->client->pers.cmd.serverTime = level.time - ( 10 + floor( random() * 25 ) + 1 );
	}
	else
	{
		self->client->ps.commandTime = level.time;
	}
	
	// show random ping values in scoreboard
	//self->client->ping = ucmd.msec;
	
	// copy the state that the cgame is currently sending
	//cmd->weapon = cl.cgameUserCmdValue;
	
	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	//cmd->serverTime = cl.serverTime;
	
	for( i = 0; i < 3; i++ )
	{
		self->client->pers.cmd.angles[i] = ANGLE2SHORT( self->bs.viewAngles[i] );
	}
	
	// send command through id's code, and update server information about this client
	trap_BotUserCommand( self - g_entities, &self->client->pers.cmd );
	
	//ClientThink_real(self);
	//self->nextthink = level.time + FRAMETIME;
}

/*
=============
ACEAI_InFront

returns 1 if the entity is in front (in sight) of self
=============
*/
qboolean ACEAI_InFront( gentity_t* self, gentity_t* other )
{
	vec3_t          vec;
	float           angle;
	vec3_t          forward;
	
	AngleVectors( self->bs.viewAngles, forward, NULL, NULL );
	VectorSubtract( other->s.origin, self->client->ps.origin, vec );
	VectorNormalize( vec );
	angle = AngleBetweenVectors( vec, forward );
	
	if( angle <= 85 )
	{
		return qtrue;
	}
	
	return qfalse;
}

/*
=============
ACEAI_Visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
qboolean ACEAI_Visible( gentity_t* self, gentity_t* other )
{
	vec3_t          spot1;
	vec3_t          spot2;
	trace_t         trace;
	
	//if(!self->client || !other->client)
	//  return qfalse;
	
	VectorCopy( self->client->ps.origin, spot1 );
	//spot1[2] += self->client->ps.viewheight;
	
	VectorCopy( other->client->ps.origin, spot2 );
	//spot2[2] += other->client->ps.viewheight;
	
	trap_Trace( &trace, spot1, NULL, NULL, spot2, self->s.number, MASK_PLAYERSOLID );
	
	if( trace.entityNum == other->s.number )
	{
		return qtrue;
	}
	
	return qfalse;
}

// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly.
// Do not call it for every think cycle.
void ACEAI_PickLongRangeGoal( gentity_t* self )
{
	int             i;
	int             node;
	float           weight, bestWeight = 0.0f;
	int             currentNode, goalNode;
	gentity_t*      goalEnt;
	gclient_t*      cl;
	gentity_t*      ent;
	gentity_t*      player;
	float           cost;
	
	goalNode = INVALID;
	goalEnt = NULL;
	
	// look for a target
	currentNode = ACEND_FindClosestReachableNode( self, NODE_DENSITY, NODE_ALL );
	
	self->bs.currentNode = currentNode;
	
	if( !ace_pickLongRangeGoal.integer || currentNode == INVALID )
	{
		self->bs.state = STATE_WANDER;
		self->bs.wander_timeout = level.time + 1000;
		self->bs.goalNode = -1;
		return;
	}
	
	// look for items
	for( i = 0, ent = &g_entities[0]; i < level.numEntities; i++, ent++ )
	{
		if( !ent->inuse )
		{
			continue;
		}
		
		if( !ent->item )
		{
			continue;
		}
		
		if( ent->node == INVALID )
		{
			continue;
		}
		
		// the same pickup rules are used for client side and server side
		if( !BG_CanItemBeGrabbed( g_gametype.integer, &ent->s, &self->client->ps ) )
		{
			continue;
		}
		
		// ignore items that were picked up recently or are not fully spawned yet
		if( ent->s.eFlags & EF_NODRAW )
		{
			continue;
		}
		
		cost = ACEND_FindCost( currentNode, ent->node );
		
		if( cost == INVALID || cost < 2 )	// ignore invalid and very short hops
		{
			continue;
		}
		
		weight = ACEIT_ItemNeed( self, ent );
		
		weight *= random();		// Allow random variations
		weight /= cost;			// Check against cost of getting there
		
		if( weight > bestWeight )
		{
			bestWeight = weight;
			goalNode = ent->node;
			goalEnt = ent;
		}
	}
	
	// this should be its own function and is for now just
	// finds a player to set as the goal
	for( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;
		player = level.gentities + cl->ps.clientNum;
		
		if( player == self )
		{
			continue;
		}
		
		if( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}
		
		if( player->health <= 0 )
		{
			continue;
		}
		
		node = ACEND_FindClosestReachableNode( player, NODE_DENSITY, NODE_ALL );
		cost = ACEND_FindCost( currentNode, node );
		
		if( cost == INVALID || cost < 3 )	// ignore invalid and very short hops
		{
			continue;
		}
		
		// player carrying the flag?
		if( g_gametype.integer == GT_CTF && !OnSameTeam( self, player ) &&
				( player->client->ps.powerups[PW_REDFLAG] || player->client->ps.powerups[PW_BLUEFLAG] ) )
		{
			weight = 2.0f;
		}
		else
		{
			weight = 0.3f;
		}
		
		weight *= random();		// Allow random variations
		weight /= cost;			// Check against cost of getting there
		
		if( weight > bestWeight )
		{
			bestWeight = weight;
			goalNode = node;
			goalEnt = player;
		}
	}
	
	// if do not find a goal, go wandering....
	if( bestWeight == 0.0f || goalNode == INVALID )
	{
		self->bs.goalNode = INVALID;
		self->bs.state = STATE_WANDER;
		self->bs.wander_timeout = level.time + 1000;
		
		if( ace_debug.integer )
		{
			trap_SendServerCommand( -1, va( "print \"%s: did not find a LR goal, wandering..\n\"", self->client->pers.netname ) );
		}
		return;					// no path?
	}
	
	// OK, everything valid, let's start moving to our goal
	self->bs.state = STATE_MOVE;
	self->bs.tries = 0;			// reset the count of how many times we tried this goal
	
	if( goalEnt != NULL && ace_debug.integer )
		trap_SendServerCommand( -1,
								va( "print \"%s: selected a %s at node %d for LR goal\n\"", self->client->pers.netname,
									goalEnt->classname, goalNode ) );
									
	ACEND_SetGoal( self, goalNode );
}


// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
void ACEAI_PickShortRangeGoal( gentity_t* self )
{
	gentity_t*      target;
	float           weight, bestWeight = 0.0f;
	gentity_t*      best;
	float           shortRange = 200;
	
	if( !ace_pickShortRangeGoal.integer )
	{
		return;
	}
	
	best = NULL;
	
	// look for a target (should make more efficent later)
	target = G_FindRadius( NULL, self->client->ps.origin, shortRange );
	
	while( target )
	{
		if( target->classname == NULL )
		{
			return;    //goto nextTarget;
		}
		
		if( target == self )
		{
			goto nextTarget;
		}
		
		// missile avoidance code
		// set our moveTarget to be the rocket or grenade fired at us.
		if( !Q_stricmp( target->classname, "rocket" ) || !Q_stricmp( target->classname, "grenade" ) )
		{
			if( ace_debug.integer )
			{
				trap_SendServerCommand( -1, va( "print \"%s: ROCKET ALERT!\n\"", self->client->pers.netname ) );
			}
			
			best = target;
			bestWeight++;
			break;
		}
		
#if 0
		// so players can't sneak RIGHT up on a bot
		if( !Q_stricmp( target->classname, "player" ) )
		{
			if( target->health && !OnSameTeam( self, target ) )
			{
				best = target;
				bestWeight++;
				break;
			}
		}
#endif
		
		if( ACEIT_IsReachable( self, target->s.origin ) )
		{
			if( ACEAI_InFront( self, target ) )
			{
				if( target->item )
				{
					// the same pickup rules are used for client side and server side
					if( !BG_CanItemBeGrabbed( g_gametype.integer, &target->s, &self->client->ps ) )
					{
						goto nextTarget;
					}
					
					// ignore items that were picked up recently or are not fully spawned yet
					if( target->s.eFlags & EF_NODRAW )
					{
						goto nextTarget;
					}
					
					weight = ACEIT_ItemNeed( self, target );
					
					if( weight > bestWeight )
					{
						bestWeight = weight;
						best = target;
					}
				}
			}
		}
		
		// next target
nextTarget:
		target = G_FindRadius( target, self->client->ps.origin, shortRange );
	}
	
	if( bestWeight )
	{
		self->bs.moveTarget = best;
		
		if( ace_debug.integer && self->bs.goalEntity != self->bs.moveTarget )
			trap_SendServerCommand( -1,
									va( "print \"%s: selected a %s for SR goal\n\"", self->client->pers.netname,
										self->bs.moveTarget->classname ) );
										
		self->bs.goalEntity = best;
	}
}

// Scan for enemy (simplifed for now to just pick any visible enemy)
qboolean ACEAI_FindEnemy( gentity_t* self )
{
	int             i;
	gclient_t*      cl;
	gentity_t*      player;
	float           enemyRange;
	float           bestRange = 99999;
	
	if( !ace_attackEnemies.integer )
	{
		return qfalse;
	}
	
	for( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;
		player = level.gentities + cl->ps.clientNum;
		
		if( player == self )
		{
			continue;
		}
		
		if( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}
		
		if( player->health <= 0 )
		{
			continue;
		}
		
		// don't attack team mates
		if( OnSameTeam( self, player ) )
		{
			continue;
		}
		
		enemyRange = Distance( self->client->ps.origin, player->client->ps.origin );
		
		if( ACEAI_InFront( self, player ) && ACEAI_Visible( self, player ) &&
				trap_InPVS( self->client->ps.origin, player->client->ps.origin ) && enemyRange < bestRange )
		{
			/*
			   if(ace_debug.integer && self->enemy != player)
			   {
			   if(self->enemy == NULL)
			   trap_SendServerCommand(-1, va("print \"%s: found enemy %s\n\"", self->client->pers.netname, player->client->pers.netname));
			   else
			   trap_SendServerCommand(-1, va("print \"%s: found better enemy %s\n\"", self->client->pers.netname, player->client->pers.netname));
			   }
			 */
			
			self->enemy = player;
			bestRange = enemyRange;
		}
	}
	
	// FIXME ? bad design
	return self->enemy != NULL;
}

// Hold fire with RL/BFG?
qboolean ACEAI_CheckShot( gentity_t* self )
{
	trace_t         tr;
	
	trap_Trace( &tr, self->client->ps.origin, tv( -8, -8, -8 ), tv( 8, 8, 8 ), self->enemy->client->ps.origin, self->s.number,
				MASK_SHOT );
				
	// if blocked, do not shoot
	if( tr.entityNum == self->enemy->s.number )
	{
		return qtrue;
	}
	
	return qfalse;
}

// Choose the best weapon for bot (simplified)
void ACEAI_ChooseWeapon( gentity_t* self )
{
	float           range;
	vec3_t          v;
	
	// if no enemy, then what are we doing here?
	if( !self->enemy )
	{
		return;
	}
	
	// base selection on distance
	VectorSubtract( self->client->ps.origin, self->enemy->client->ps.origin, v );
	range = VectorLength( v );
	
	// always favor the railgun
	if( ACEIT_ChangeWeapon( self, WP_RAILGUN ) )
	{
		return;
	}
	
	// longer range
	if( range > 300 )
	{
		// choose BFG if enough ammo
		if( self->client->ps.ammo[WP_BFG] > 45 )
			if( ACEAI_CheckShot( self ) && ACEIT_ChangeWeapon( self, WP_BFG ) )
			{
				return;
			}
			
		if( ACEAI_CheckShot( self ) && ACEIT_ChangeWeapon( self, WP_ROCKET_LAUNCHER ) )
		{
			return;
		}
	}
	
	// only use GL in certain ranges and only on targets at or below our level
	if( range > 100 && range < 500 && self->enemy->client->ps.origin[2] - 20 < self->client->ps.origin[2] )
		if( ACEIT_ChangeWeapon( self, WP_GRENADE_LAUNCHER ) )
		{
			return;
		}
		
	if( range > 100 && range < LIGHTNING_RANGE )
		if( ACEIT_ChangeWeapon( self, WP_LIGHTNING ) )
		{
			return;
		}
		
#ifdef MISSIONPACK
	// only use CG when ammo > 50
	if( self->client->pers.inventory[ITEMLIST_BULLETS] >= 50 )
		if( ACEIT_ChangeWeapon( self, FindItem( "chaingun" ) ) )
		{
			return;
		}
#endif
		
	if( ACEIT_ChangeWeapon( self, WP_PLASMAGUN ) )
	{
		return;
	}
	
	if( ACEIT_ChangeWeapon( self, WP_SHOTGUN ) )
	{
		return;
	}
	
	if( ACEIT_ChangeWeapon( self, WP_MACHINEGUN ) )
	{
		return;
	}
	
	if( ACEIT_ChangeWeapon( self, WP_GAUNTLET ) )
	{
		return;
	}
	
	return;
}


#endif
