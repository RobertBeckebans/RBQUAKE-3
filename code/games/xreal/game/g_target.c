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
//
#include "g_local.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t* ent, gentity_t* other, gentity_t* activator )
{
	gentity_t*      t;
	trace_t         trace;
	
	if( !activator->client )
	{
		return;
	}
	
	if( !ent->target )
	{
		return;
	}
	
	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while( ( t = G_Find( t, FOFS( name ), ent->target ) ) != NULL )
	{
		if( !t->item )
		{
			continue;
		}
		Touch_Item( t, activator, &trace );
		
		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity( t );
	}
}

void SP_target_give( gentity_t* ent )
{
	ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups( gentity_t* ent, gentity_t* other, gentity_t* activator )
{
	if( !activator->client )
	{
		return;
	}
	
	if( activator->client->ps.powerups[PW_REDFLAG] )
	{
		Team_ReturnFlag( TEAM_RED );
	}
	else if( activator->client->ps.powerups[PW_BLUEFLAG] )
	{
		Team_ReturnFlag( TEAM_BLUE );
	}
	else if( activator->client->ps.powerups[PW_NEUTRALFLAG] )
	{
		Team_ReturnFlag( TEAM_FREE );
	}
	
	memset( activator->client->ps.powerups, 0, sizeof( activator->client->ps.powerups ) );
}

void SP_target_remove_powerups( gentity_t* ent )
{
	ent->use = Use_target_remove_powerups;
}


//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t* ent )
{
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t* ent, gentity_t* other, gentity_t* activator )
{
	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t* ent )
{
	// check delay for backwards compatability
	if( !G_SpawnFloat( "delay", "0", &ent->wait ) )
	{
		G_SpawnFloat( "wait", "1", &ent->wait );
	}
	
	if( !ent->wait )
	{
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score( gentity_t* ent, gentity_t* other, gentity_t* activator )
{
	AddScore( activator, ent->r.currentOrigin, ent->count );
}

void SP_target_score( gentity_t* ent )
{
	if( !ent->count )
	{
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) red_only blue_only private
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print( gentity_t* ent, gentity_t* other, gentity_t* activator )
{
	if( activator->client && ent->priv )
	{
		trap_SendServerCommand( activator - g_entities, va( "cp \"%s\"", ent->message ) );
		return;
	}
	
	if( ent->red_only )
	{
		G_TeamCommand( TEAM_RED, va( "cp \"%s\"", ent->message ) );
		return;
	}
	if( ent->blue_only )
	{
		G_TeamCommand( TEAM_BLUE, va( "cp \"%s\"", ent->message ) );
		return;
	}
	
	trap_SendServerCommand( -1, va( "cp \"%s\"", ent->message ) );
}

void SP_target_print( gentity_t* ent )
{
	G_SpawnBoolean( "red_only", "0", &ent->red_only );
	G_SpawnBoolean( "blue_only", "0", &ent->blue_only );
	G_SpawnBoolean( "private", "0", &ent->priv );
	
	ent->use = Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void Use_Target_Speaker( gentity_t* ent, gentity_t* other, gentity_t* activator )
{
	if( ent->soundLooping )
	{
		// looping sound toggles
		if( ent->s.loopSound )
		{
			ent->s.loopSound = 0;    // turn it off
		}
		else
		{
			ent->s.loopSound = ent->soundIndex;    // start it
		}
	}
	else
	{
		// normal sound
		if( ent->soundActivator )
		{
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->soundIndex );
		}
		else if( ent->soundGlobal )
		{
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->soundIndex );
		}
		else
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundIndex );
		}
	}
}

void SP_target_speaker( gentity_t* ent )
{
	char            buffer[MAX_QPATH];
	char*           s;
	
	if( G_SpawnString( "s_sound", "NOSOUND", &s ) )
	{
		G_SpawnBoolean( "s_looping", "0", &ent->soundLooping );
		G_SpawnBoolean( "s_waitfortrigger", "0", &ent->soundWaitForTrigger );
		G_SpawnBoolean( "s_global", "0", &ent->soundGlobal );
		G_SpawnBoolean( "s_activator", "0", &ent->soundActivator );
		G_SpawnFloat( "wait", "0", &ent->wait );
		G_SpawnFloat( "random", "0", &ent->random );
	}
	else if( G_SpawnString( "s_shader", "NOSOUND", &s ) )
	{
		// Doom3 compatibility mode
		G_SpawnBoolean( "s_looping", "0", &ent->soundLooping );
		G_SpawnBoolean( "s_waitfortrigger", "0", &ent->soundWaitForTrigger );
		G_SpawnBoolean( "s_global", "0", &ent->soundGlobal );
		G_SpawnBoolean( "s_activator", "0", &ent->soundActivator );
		G_SpawnFloat( "wait", "0", &ent->wait );
		G_SpawnFloat( "random", "0", &ent->random );
	}
	else if( G_SpawnString( "noise", "NOSOUND", &s ) )
	{
		// Q3A compatibility mode
		ent->soundLooping = ent->spawnflags & 1 ? qtrue : qfalse;
		ent->soundWaitForTrigger = ent->spawnflags & 2 ? qtrue : qfalse;
		ent->soundGlobal = ent->spawnflags & 4 ? qtrue : qfalse;
		ent->soundActivator = ent->spawnflags & 8 ? qtrue : qfalse;
	}
	else
	{
		//G_Error("speaker without a noise key at %s", vtos(ent->s.origin));
		G_Printf( S_COLOR_YELLOW "WARNING: speaker '%s' without a noise key at %s", ent->name, vtos( ent->s.origin ) );
	}
	
	// force all client relative sounds to be "activator" speakers that
	// play on the entity that activates it
	if( s[0] == '*' )
	{
		ent->soundActivator = qtrue;
	}
	
	Q_strncpyz( buffer, s, sizeof( buffer ) );
	Com_DefaultExtension( buffer, sizeof( buffer ), ".wav" );
	
	ent->soundIndex = G_SoundIndex( buffer );
	
	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->soundIndex;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;
	
	
	// check for prestarted looping sound
	if( ent->soundLooping && !ent->soundWaitForTrigger )
	{
		ent->s.loopSound = ent->soundIndex;
	}
	
	ent->use = Use_Target_Speaker;
	
	if( ent->soundGlobal )
	{
		ent->r.svFlags |= SVF_BROADCAST;
	}
	
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	
	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think( gentity_t* self )
{
	vec3_t          end;
	trace_t         tr;
	vec3_t          point;
	
	// if pointed at another entity, set movedir to point at it
	if( self->enemy )
	{
		VectorMA( self->enemy->s.origin, 0.5, self->enemy->r.mins, point );
		VectorMA( point, 0.5, self->enemy->r.maxs, point );
		VectorSubtract( point, self->s.origin, self->movedir );
		VectorNormalize( self->movedir );
	}
	
	// fire forward and see what we hit
	VectorMA( self->s.origin, 2048, self->movedir, end );
	
	trap_Trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE );
	
	if( tr.entityNum )
	{
		// hurt it if we can
		G_Damage( &g_entities[tr.entityNum], self, self->activator, self->movedir,
				  tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER );
	}
	
	VectorCopy( tr.endpos, self->s.origin2 );
	
	trap_LinkEntity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on( gentity_t* self )
{
	if( !self->activator )
	{
		self->activator = self;
	}
	target_laser_think( self );
}

void target_laser_off( gentity_t* self )
{
	trap_UnlinkEntity( self );
	self->nextthink = 0;
}

void target_laser_use( gentity_t* self, gentity_t* other, gentity_t* activator )
{
	self->activator = activator;
	if( self->nextthink > 0 )
	{
		target_laser_off( self );
	}
	else
	{
		target_laser_on( self );
	}
}

void target_laser_start( gentity_t* self )
{
	gentity_t*      ent;
	
	self->s.eType = ET_BEAM;
	
	if( self->target )
	{
		ent = G_Find( NULL, FOFS( name ), self->target );
		if( !ent )
		{
			G_Printf( "%s at %s: %s is a bad target\n", self->classname, vtos( self->s.origin ), self->target );
		}
		self->enemy = ent;
	}
	else
	{
		G_SetMovedir( self->s.angles, self->movedir );
	}
	
	self->use = target_laser_use;
	self->think = target_laser_think;
	
	if( !self->damage )
	{
		self->damage = 9999;
	}
	
	if( self->start_on )
	{
		target_laser_on( self );
	}
	else
	{
		target_laser_off( self );
	}
}

void SP_target_laser( gentity_t* self )
{
	G_SpawnBoolean( "start_on", "0", &self->start_on );
	
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use( gentity_t* self, gentity_t* other, gentity_t* activator )
{
	gentity_t*      dest;
	
	if( !activator->client )
	{
		return;
	}
	dest = G_PickTarget( self->target );
	if( !dest )
	{
		G_Printf( "Couldn't find teleporter destination\n" );
		return;
	}
	
	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t* self )
{
	if( !self->name )
	{
		G_Printf( "untargeted %s at %s\n", self->classname, vtos( self->s.origin ) );
	}
	
	self->use = target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them
*/
void target_relay_use( gentity_t* self, gentity_t* other, gentity_t* activator )
{
	if( self->red_only && activator->client && activator->client->sess.sessionTeam != TEAM_RED )
	{
		return;
	}
	
	if( self->blue_only && activator->client && activator->client->sess.sessionTeam != TEAM_BLUE )
	{
		return;
	}
	
	if( self->random )
	{
		gentity_t*      ent;
		
		ent = G_PickTarget( self->target );
		if( ent && ent->use )
		{
			ent->use( ent, self, activator );
		}
		return;
	}
	G_UseTargets( self, activator );
}

void SP_target_relay( gentity_t* self )
{
	G_SpawnBoolean( "red_only", "0", &self->red_only );
	G_SpawnBoolean( "blue_only", "0", &self->blue_only );
	G_SpawnFloat( "random", "0", &self->random );
	
	self->use = target_relay_use;
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/
void target_kill_use( gentity_t* self, gentity_t* other, gentity_t* activator )
{
	G_Damage( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
}

void SP_target_kill( gentity_t* self )
{
	self->use = target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position( gentity_t* self )
{
	G_SetOrigin( self, self->s.origin );
}

/*QUAKED target_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_null( gentity_t* self )
{
	G_SetOrigin( self, self->s.origin );
}

static void target_location_linkup( gentity_t* ent )
{
	int             i;
	int             n;
	
	if( level.locationLinked )
	{
		return;
	}
	
	level.locationLinked = qtrue;
	
	level.locationHead = NULL;
	
	trap_SetConfigstring( CS_LOCATIONS, "unknown" );
	
	for( i = 0, ent = g_entities, n = 1; i < level.numEntities; i++, ent++ )
	{
		if( ent->classname && !Q_stricmp( ent->classname, "target_location" ) )
		{
			// lets overload some variables!
			ent->health = n;	// use for location marking
			trap_SetConfigstring( CS_LOCATIONS + n, ent->message );
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}
	
	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t* self )
{
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;	// Let them all spawn first
	
	G_SetOrigin( self, self->s.origin );
}

//==========================================================

/*
===============
target_fx_use

Use function for effects system
===============
*/
/*
static void target_fx_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->s.eFlags ^= EF_NODRAW;
}
*/

static void target_fx_think( gentity_t* self )
{
	G_AddEvent( self, EV_EFFECT, self->s.modelindex );
	
	if( self->wait > 0 )
	{
		//ent->think = multi_wait;
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		self->touch = NULL;
		self->nextthink = level.time + FRAMETIME;
		self->think = G_FreeEntity;
	}
}


/*QUAKED target_fx (0 0 1) (-8 -8 -8) (8 8 8)
*/
void SP_target_fx( gentity_t* self )
{
	char*           effectName;
	int             startOn = 0;
	
	self->s.eType = ET_INVISIBLE;
	
	G_SpawnFloat( "wait", "0.5", &self->wait );
	G_SpawnFloat( "random", "0", &self->random );
	
	G_SpawnInt( "start_on", "0", &startOn );
	if( !startOn )
	{
		self->s.eFlags |= EF_NODRAW;
	}
	
	G_SpawnString( "luaThink", "", &effectName );
	self->s.modelindex = G_EffectIndex( effectName );
	
	G_SetOrigin( self, self->s.origin );
	
	// save angles
	VectorCopy( self->s.angles, self->s.apos.trBase );
	self->s.apos.trType = TR_STATIONARY;
	self->s.apos.trTime = 0;
	self->s.apos.trDuration = 0;
	VectorClear( self->s.apos.trDelta );
	//VectorCopy(origin, ent->r.currentAOrigin);
	
	VectorClear( self->r.mins );
	VectorClear( self->r.maxs );
	trap_LinkEntity( self );
	
	self->think = target_fx_think;
	self->nextthink = level.time + 1000;
}
