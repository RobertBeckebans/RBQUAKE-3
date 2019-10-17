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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char*           cg_customSoundNames[MAX_CUSTOM_SOUNDS] =
{
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};


/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound( int clientNum, const char* soundName )
{
	clientInfo_t*   ci;
	int             i;
	
	if( soundName[0] != '*' )
	{
		return trap_S_RegisterSound( soundName );
	}
	
	if( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		clientNum = 0;
	}
	ci = &cgs.clientinfo[clientNum];
	
	for( i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i]; i++ )
	{
		if( !strcmp( soundName, cg_customSoundNames[i] ) )
		{
			return ci->sounds[i];
		}
	}
	
	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/


/*
======================
CG_ParseCharacterFile

Read a configuration file containing body.md5mesh custom
models/players/visor/character.cfg, etc
======================
*/

static qboolean CG_ParseCharacterFile( const char* filename, clientInfo_t* ci )
{
	char*           text_p, *prev;
	int             len;
	int             i;
	char*           token;
	int             skip;
	char            text[20000];
	fileHandle_t    f;
	
	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if( len <= 0 )
	{
		return qfalse;
	}
	if( len >= sizeof( text ) - 1 )
	{
		CG_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );
	
	// parse the text
	text_p = text;
	skip = 0;					// quite the compiler warning
	
	ci->footsteps = FOOTSTEP_STONE;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;
	ci->firstTorsoBoneName[0] = '\0';
	ci->lastTorsoBoneName[0] = '\0';
	ci->torsoControlBoneName[0] = '\0';
	ci->neckControlBoneName[0] = '\0';
	ci->modelScale[0] = 1;
	ci->modelScale[1] = 1;
	ci->modelScale[2] = 1;
	
	// read optional parameters
	while( 1 )
	{
		prev = text_p;			// so we can unget
		token = Com_Parse( &text_p );
		if( !token[0] )
		{
			break;
		}
		
		if( !Q_stricmp( token, "footsteps" ) )
		{
			token = Com_Parse( &text_p );
			if( !token )
			{
				break;
			}
			if( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) || !Q_stricmp( token, "stone" ) )
			{
				ci->footsteps = FOOTSTEP_STONE;
			}
			else if( !Q_stricmp( token, "boot" ) )
			{
				ci->footsteps = FOOTSTEP_BOOT;
			}
			else if( !Q_stricmp( token, "flesh" ) )
			{
				ci->footsteps = FOOTSTEP_FLESH;
			}
			else if( !Q_stricmp( token, "mech" ) )
			{
				ci->footsteps = FOOTSTEP_MECH;
			}
			else if( !Q_stricmp( token, "energy" ) )
			{
				ci->footsteps = FOOTSTEP_ENERGY;
			}
			else
			{
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		}
		else if( !Q_stricmp( token, "headoffset" ) )
		{
			for( i = 0; i < 3; i++ )
			{
				token = Com_Parse( &text_p );
				if( !token )
				{
					break;
				}
				ci->headOffset[i] = atof( token );
			}
			continue;
		}
		else if( !Q_stricmp( token, "sex" ) )
		{
			token = Com_Parse( &text_p );
			if( !token )
			{
				break;
			}
			if( token[0] == 'f' || token[0] == 'F' )
			{
				ci->gender = GENDER_FEMALE;
			}
			else if( token[0] == 'n' || token[0] == 'N' )
			{
				ci->gender = GENDER_NEUTER;
			}
			else
			{
				ci->gender = GENDER_MALE;
			}
			continue;
		}
		else if( !Q_stricmp( token, "fixedlegs" ) )
		{
			ci->fixedlegs = qtrue;
			continue;
		}
		else if( !Q_stricmp( token, "fixedtorso" ) )
		{
			ci->fixedtorso = qtrue;
			continue;
		}
		else if( !Q_stricmp( token, "firstTorsoBoneName" ) )
		{
			token = Com_Parse( &text_p );
			Q_strncpyz( ci->firstTorsoBoneName, token, sizeof( ci->firstTorsoBoneName ) );
			continue;
		}
		else if( !Q_stricmp( token, "lastTorsoBoneName" ) )
		{
			token = Com_Parse( &text_p );
			Q_strncpyz( ci->lastTorsoBoneName, token, sizeof( ci->lastTorsoBoneName ) );
			continue;
		}
		else if( !Q_stricmp( token, "torsoControlBoneName" ) )
		{
			token = Com_Parse( &text_p );
			Q_strncpyz( ci->torsoControlBoneName, token, sizeof( ci->torsoControlBoneName ) );
			continue;
		}
		else if( !Q_stricmp( token, "neckControlBoneName" ) )
		{
			token = Com_Parse( &text_p );
			Q_strncpyz( ci->neckControlBoneName, token, sizeof( ci->neckControlBoneName ) );
			continue;
		}
		else if( !Q_stricmp( token, "modelScale" ) )
		{
			for( i = 0; i < 3; i++ )
			{
				token = Com_ParseExt( &text_p, qfalse );
				if( !token )
				{
					break;
				}
				ci->modelScale[i] = atof( token );
			}
			continue;
		}
		
		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}
	
	return qtrue;
}

/*
==========================
CG_FileExists
==========================
*/
static qboolean CG_FileExists( const char* filename )
{
	int             len;
	
	len = trap_FS_FOpenFile( filename, 0, FS_READ );
	if( len > 0 )
	{
		return qtrue;
	}
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
qboolean CG_FindClientModelFile( char* filename, int length, clientInfo_t* ci, const char* modelName,
								 const char* skinName, const char* base, const char* ext )
{
	char*           team;
	
	if( cgs.gametype >= GT_TEAM )
	{
		switch( ci->team )
		{
			case TEAM_BLUE:
			{
				team = "blue";
				break;
			}
			default:
			{
				team = "red";
				break;
			}
		}
	}
	else
	{
		team = "default";
	}
	
	if( cgs.gametype >= GT_TEAM )
	{
		//                            "models/players/james/lower_red.skin"
		Com_sprintf( filename, length, "models/players/%s/%s_%s.%s", modelName, base, team, ext );
	}
	else
	{
		//                            "models/players/james/lower_lily.skin"
		Com_sprintf( filename, length, "models/players/%s/%s_%s.%s", modelName, base, skinName, ext );
	}
	
	if( CG_FileExists( filename ) )
	{
		return qtrue;
	}
	
	return qfalse;
}

static qboolean CG_RegisterPlayerAnimation( clientInfo_t* ci, const char* modelName, int anim, const char* animName,
		qboolean loop, qboolean reversed, qboolean clearOrigin )
{
	char            filename[MAX_QPATH];
	int             frameRate;
	
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s.md5anim", modelName, animName );
	ci->animations[anim].handle = trap_R_RegisterAnimation( filename );
	if( !ci->animations[anim].handle )
	{
		Com_Printf( "Failed to load animation file %s\n", filename );
		return qfalse;
	}
	
	ci->animations[anim].firstFrame = 0;
	ci->animations[anim].numFrames = trap_R_AnimNumFrames( ci->animations[anim].handle );
	frameRate = trap_R_AnimFrameRate( ci->animations[anim].handle );
	
	if( frameRate == 0 )
	{
		frameRate = 1;
	}
	ci->animations[anim].frameTime = 1000 / frameRate;
	ci->animations[anim].initialLerp = 1000 / frameRate;
	
	if( loop )
	{
		ci->animations[anim].loopFrames = ci->animations[anim].numFrames;
	}
	else
	{
		ci->animations[anim].loopFrames = 0;
	}
	
	ci->animations[anim].reversed = reversed;
	ci->animations[anim].clearOrigin = clearOrigin;
	
	return qtrue;
}


/*
==========================
CG_RegisterClientModel
==========================
*/
qboolean CG_RegisterClientModel( clientInfo_t* ci, const char* modelName, const char* skinName, const char* teamName )
{
	int             i;
	char            filename[MAX_QPATH * 2];
	
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/body.md5mesh", modelName );
	ci->bodyModel = trap_R_RegisterModel( filename );
	
	if( !ci->bodyModel )
	{
		Com_Printf( "Failed to load body mesh file  %s\n", filename );
		return qfalse;
	}
	
	
	if( ci->bodyModel )
	{
		// load the animations
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/character.cfg", modelName );
		if( !CG_ParseCharacterFile( filename, ci ) )
		{
			Com_Printf( "Failed to load character file %s\n", filename );
			return qfalse;
		}
		
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_IDLE, "idle", qtrue, qfalse, qfalse ) )
		{
			Com_Printf( "Failed to load idle animation file %s\n", filename );
			return qfalse;
		}
		
		// make LEGS_IDLE the default animation
		for( i = 0; i < MAX_PLAYER_ANIMATIONS; i++ )
		{
			if( i == LEGS_IDLE )
			{
				continue;
			}
			
			ci->animations[i] = ci->animations[LEGS_IDLE];
		}
		
		// FIXME fail register of the entire model if one of these animations is missing
		
		// FIXME add death animations
		
		CG_RegisterPlayerAnimation( ci, modelName, BOTH_DEATH1, "die", qfalse, qfalse, qfalse );
		//CG_RegisterPlayerAnimation(ci, modelName, BOTH_DEATH2, "death2", qfalse, qfalse, qfalse);
		//CG_RegisterPlayerAnimation(ci, modelName, BOTH_DEATH3, "death3", qfalse, qfalse, qfalse);
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_GESTURE, "gesture", qfalse, qfalse, qfalse ) )
		{
			ci->animations[TORSO_GESTURE] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK, "attack", qfalse, qfalse, qfalse ) )
		{
			ci->animations[TORSO_ATTACK] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK2, "idle", qfalse, qfalse, qfalse ) )
		{
			ci->animations[TORSO_ATTACK2] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_STAND2, "idle", qfalse, qfalse, qfalse ) )
		{
			ci->animations[TORSO_STAND2] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_IDLECR, "crouch", qtrue, qfalse, qfalse ) )
		{
			ci->animations[LEGS_IDLECR] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_WALKCR, "crouch_forward", qtrue, qfalse, qtrue ) )
		{
			ci->animations[LEGS_WALKCR] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACKCR, "crouch_forward", qtrue, qtrue, qtrue ) )
		{
			ci->animations[LEGS_BACKCR] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_WALK, "walk", qtrue, qfalse, qtrue ) )
		{
			ci->animations[LEGS_WALK] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACKWALK, "walk_backwards", qtrue, qfalse, qtrue ) )
		{
			ci->animations[LEGS_BACKWALK] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_RUN, "run", qtrue, qfalse, qtrue ) )
		{
			ci->animations[LEGS_RUN] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACK, "run_backwards", qtrue, qtrue, qtrue ) )
		{
			ci->animations[LEGS_BACK] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_SWIM, "swim", qtrue, qfalse, qtrue ) )
		{
			ci->animations[LEGS_SWIM] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_JUMP, "jump", qfalse, qfalse, qfalse ) )
		{
			ci->animations[LEGS_JUMP] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_LAND, "land", qfalse, qfalse, qfalse ) )
		{
			ci->animations[LEGS_LAND] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_JUMPB, "jump", qfalse, qtrue, qfalse ) )
		{
			ci->animations[LEGS_JUMPB] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_LANDB, "land", qfalse, qtrue, qfalse ) )
		{
			ci->animations[LEGS_LANDB] = ci->animations[LEGS_IDLE];
		}
		
		if( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_TURN, "step", qfalse, qfalse, qfalse ) )
		{
			ci->animations[LEGS_TURN] = ci->animations[LEGS_IDLE];
		}
		
		
		if( CG_FindClientModelFile( filename, sizeof( filename ), ci, modelName, skinName, "body", "skin" ) )
		{
			Com_Printf( "loading skin %s\n", filename );
			
			ci->bodySkin = trap_R_RegisterSkin( filename );
		}
		if( !ci->bodySkin )
		{
			Com_Printf( "Body skin load failure: %s\n", filename );
			return qfalse;
		}
	}
	
	if( CG_FindClientModelFile( filename, sizeof( filename ), ci, modelName, skinName, "icon", "tga" ) )
	{
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}
	else if( CG_FindClientModelFile( filename, sizeof( filename ), ci, modelName, skinName, "icon", "png" ) )
	{
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}
	
	if( !ci->modelIcon )
	{
		Com_Printf( "Failed to load icon file %s\n", filename );
		return qfalse;
	}
	
	return qtrue;
}


/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char* v, vec3_t color )
{
	int             val;
	
	VectorClear( color );
	
	val = atoi( v );
	
	if( val < 1 || val > 7 )
	{
		VectorSet( color, 1, 1, 1 );
		return;
	}
	
	if( val & 1 )
	{
		color[2] = 1.0f;
	}
	if( val & 2 )
	{
		color[1] = 1.0f;
	}
	if( val & 4 )
	{
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( clientInfo_t* ci )
{
	const char*     dir, *fallback;
	int             i, modelloaded;
	const char*     s;
	int             clientNum;
	char            teamname[MAX_QPATH];
	
	teamname[0] = 0;
#ifdef MISSIONPACK
	if( cgs.gametype >= GT_TEAM )
	{
		if( ci->team == TEAM_BLUE )
		{
			Q_strncpyz( teamname, cg_blueTeamName.string, sizeof( teamname ) );
		}
		else
		{
			Q_strncpyz( teamname, cg_redTeamName.string, sizeof( teamname ) );
		}
	}
	if( teamname[0] )
	{
		strcat( teamname, "/" );
	}
#endif
	modelloaded = qtrue;
	if( !CG_RegisterClientModel( ci, ci->modelName, ci->skinName, teamname ) )
	{
		if( cg_buildScript.integer )
		{
			CG_Error( "CG_RegisterClientModel( %s, %s, %s ) failed", ci->modelName, ci->skinName, teamname );
		}
		
		// fall back to default team name
		if( cgs.gametype >= GT_TEAM )
		{
			// keep skin name
			if( ci->team == TEAM_BLUE )
			{
				Q_strncpyz( teamname, DEFAULT_BLUETEAM_NAME, sizeof( teamname ) );
			}
			else
			{
				Q_strncpyz( teamname, DEFAULT_REDTEAM_NAME, sizeof( teamname ) );
			}
			
			if( !CG_RegisterClientModel( ci, DEFAULT_MODEL, ci->skinName, teamname ) )
			{
				CG_Error( "DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_MODEL, ci->skinName );
			}
		}
		else
		{
			if( !CG_RegisterClientModel( ci, DEFAULT_MODEL, "default", teamname ) )
			{
				CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
			}
		}
		modelloaded = qfalse;
	}
	
	ci->newAnims = qfalse;
	
	// sounds
	dir = ci->modelName;
	fallback = DEFAULT_MODEL;
	
	for( i = 0; i < MAX_CUSTOM_SOUNDS; i++ )
	{
		s = cg_customSoundNames[i];
		if( !s )
		{
			break;
		}
		ci->sounds[i] = 0;
		
		// if the model didn't load use the sounds of the default model
		if( modelloaded )
		{
			ci->sounds[i] = trap_S_RegisterSound( va( "sound/player/%s/%s", dir, s + 1 ) );
		}
		
		if( !ci->sounds[i] )
		{
			ci->sounds[i] = trap_S_RegisterSound( va( "sound/player/%s/%s", fallback, s + 1 ) );
		}
	}
	
	ci->deferred = qfalse;
	
	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for( i = 0; i < MAX_GENTITIES; i++ )
	{
		if( cg_entities[i].currentState.clientNum == clientNum && cg_entities[i].currentState.eType == ET_PLAYER )
		{
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t* from, clientInfo_t* to )
{
	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;
	
	Q_strncpyz( to->firstTorsoBoneName, from->firstTorsoBoneName, sizeof( to->firstTorsoBoneName ) );
	Q_strncpyz( to->lastTorsoBoneName, from->lastTorsoBoneName, sizeof( to->lastTorsoBoneName ) );
	
	Q_strncpyz( to->torsoControlBoneName, from->torsoControlBoneName, sizeof( to->torsoControlBoneName ) );
	Q_strncpyz( to->neckControlBoneName, from->neckControlBoneName, sizeof( to->neckControlBoneName ) );
	
	VectorCopy( from->modelScale, to->modelScale );
	
	to->bodyModel = from->bodyModel;
	to->bodySkin = from->bodySkin;
	
	to->modelIcon = from->modelIcon;
	
	to->newAnims = from->newAnims;
	
	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t* ci )
{
	int             i;
	clientInfo_t*   match;
	
	for( i = 0; i < cgs.maxclients; i++ )
	{
		match = &cgs.clientinfo[i];
		if( !match->infoValid )
		{
			continue;
		}
		if( match->deferred )
		{
			continue;
		}
		if( !Q_stricmp( ci->modelName, match->modelName )
				&& !Q_stricmp( ci->skinName, match->skinName )
				&& !Q_stricmp( ci->blueTeam, match->blueTeam )
				&& !Q_stricmp( ci->redTeam, match->redTeam ) && ( cgs.gametype < GT_TEAM || ci->team == match->team ) )
		{
			// this clientinfo is identical, so use it's handles
			
			ci->deferred = qfalse;
			
			CG_CopyClientInfoModel( match, ci );
			
			return qtrue;
		}
	}
	
	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t* ci )
{
	int             i;
	clientInfo_t*   match;
	
	// if someone else is already the same models and skins we
	// can just load the client info
	for( i = 0; i < cgs.maxclients; i++ )
	{
		match = &cgs.clientinfo[i];
		if( !match->infoValid || match->deferred )
		{
			continue;
		}
		if( Q_stricmp( ci->skinName, match->skinName ) || Q_stricmp( ci->modelName, match->modelName ) ||
//           Q_stricmp( ci->headModelName, match->headModelName ) ||
//           Q_stricmp( ci->headSkinName, match->headSkinName ) ||
				( cgs.gametype >= GT_TEAM && ci->team != match->team ) )
		{
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( ci );
		return;
	}
	
	// if we are in teamplay, only grab a model if the skin is correct
	if( cgs.gametype >= GT_TEAM )
	{
		for( i = 0; i < cgs.maxclients; i++ )
		{
			match = &cgs.clientinfo[i];
			if( !match->infoValid || match->deferred )
			{
				continue;
			}
			if( Q_stricmp( ci->skinName, match->skinName ) || ( cgs.gametype >= GT_TEAM && ci->team != match->team ) )
			{
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci );
		return;
	}
	
	// find the first valid clientinfo and grab its stuff
	for( i = 0; i < cgs.maxclients; i++ )
	{
		match = &cgs.clientinfo[i];
		if( !match->infoValid )
		{
			continue;
		}
		
		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}
	
	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );
	
	CG_LoadClientInfo( ci );
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum )
{
	clientInfo_t*   ci;
	clientInfo_t    newInfo;
	const char*     configstring;
	const char*     v;
	char*           slash;
	
	ci = &cgs.clientinfo[clientNum];
	
	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if( !configstring[0] )
	{
		memset( ci, 0, sizeof( *ci ) );
		return;					// player just left
	}
	
//	CG_Printf("CG_NewClientInfo: '%s'\n", configstring);

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );
	
	// isolate the player's name
	v = Info_ValueForKey( configstring, "n" );
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );
	
	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );
	
	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );
	
	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );
	
	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );
	
	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );
	
	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );
	
	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );
	
	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi( v );
	
	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi( v );
	
	v = Info_ValueForKey( configstring, "g_redteam" );
	Q_strncpyz( newInfo.redTeam, v, MAX_TEAMNAME );
	
	v = Info_ValueForKey( configstring, "g_blueteam" );
	Q_strncpyz( newInfo.blueTeam, v, MAX_TEAMNAME );
	
	// model
	v = Info_ValueForKey( configstring, "model" );
	if( cg_forceModel.integer )
	{
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char            modelStr[MAX_QPATH];
		char*           skin = "";
		
		if( cgs.gametype >= GT_TEAM )
		{
			Q_strncpyz( newInfo.modelName, DEFAULT_MODEL, sizeof( newInfo.modelName ) );
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		}
		else
		{
			/*
			if(cg_forceBrightSkins.integer)
			{
				skin = "bright";
			}
			else
			*/
			{
				trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
				if( ( slash = strchr( modelStr, '/' ) ) == NULL )
				{
					skin = "default";
				}
				else
				{
					*slash++ = 0;
				}
			}
			
			if( ( slash = strchr( modelStr, '/' ) ) != NULL )
			{
				*slash++ = 0;
			}
			
			Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );
		}
		
		if( cgs.gametype >= GT_TEAM )
		{
			// keep skin name
			slash = strchr( v, '/' );
			if( slash )
			{
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	}
	else
	{
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );
		
		/*
		if(cg_forceBrightSkins.integer)
		{
			Q_strncpyz(newInfo.skinName, "bright", sizeof(newInfo.skinName));
		
			// truncate modelName
			if((slash = strchr(newInfo.modelName, '/')) != NULL)
			{
				*slash++ = 0;
			}
		}
		else
		*/
		{
			slash = strchr( newInfo.modelName, '/' );
			if( !slash )
			{
				// modelName didn not include a skin name
				Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
			}
			else
			{
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
				// truncate modelName
				*slash = 0;
			}
		}
	}
	
	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if( !CG_ScanForExistingClientInfo( &newInfo ) )
	{
		qboolean        forceDefer;
		
		forceDefer = trap_MemoryRemaining() < 4000000;
		
		// if we are defering loads, just have it pick the first valid
		if( forceDefer || ( cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) )
		{
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( &newInfo );
			
			// if we are low on memory, leave them with this model
			if( forceDefer )
			{
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		}
		else
		{
			CG_LoadClientInfo( &newInfo );
		}
	}
	
	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void )
{
	int             i;
	clientInfo_t*   ci;
	
	// scan for a deferred player to load
	for( i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++ )
	{
		if( ci->infoValid && ci->deferred )
		{
			// if we are low on memory, leave it deferred
			if( trap_MemoryRemaining() < 4000000 )
			{
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( ci );
//          break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetPlayerLerpFrameAnimation( clientInfo_t* ci, lerpFrame_t* lf, int newAnimation )
{
	animation_t*    anim;
	
	// save old animation
	lf->old_animationNumber = lf->animationNumber;
	lf->old_animation = lf->animation;
	
	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;
	
	if( newAnimation < 0 || newAnimation >= MAX_PLAYER_ANIMATIONS )
	{
		CG_Error( "bad player animation number: %i", newAnimation );
	}
	
	anim = &ci->animations[newAnimation];
	
	lf->animation = anim;
	lf->animationStartTime = lf->frameTime + anim->initialLerp;
	
	if( cg_debugPlayerAnim.integer )
	{
		CG_Printf( "player anim: %i\n", newAnimation );
	}
	
	debug_anim_current = lf->animationNumber;
	debug_anim_old = lf->old_animationNumber;
	
	if( lf->old_animationNumber <= 0 )
	{
		// skip initial / invalid blending
		lf->blendlerp = 0.0f;
		return;
	}
	
	// TODO: blend through two blendings!
	
	if( ( lf->blendlerp <= 0.0f ) )
	{
		lf->blendlerp = 1.0f;
	}
	else
	{
		lf->blendlerp = 1.0f - lf->blendlerp;    // use old blending for smooth blending between two blended animations
	}
	
	memcpy( &lf->oldSkeleton, &lf->skeleton, sizeof( refSkeleton_t ) );
	
	//Com_Printf("new: %i old %i\n", newAnimation,lf->old_animationNumber);
	
	if( !trap_R_BuildSkeleton( &lf->oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
	{
		CG_Printf( "Can't blend skeleton\n" );
		return;
	}
}

// TODO: choose propper values and use blending speed from character.cfg
// blending is slow for testing issues
static void CG_BlendPlayerLerpFrame( lerpFrame_t* lf )
{
	if( cg_animBlend.value <= 0.0f )
	{
		lf->blendlerp = 0.0f;
		return;
	}
	
	if( ( lf->blendlerp > 0.0f ) && ( cg.time > lf->blendtime ) )
	{
#if 0
		//linear blending
		lf->blendlerp -= 0.025f;
#else
		//exp blending
		lf->blendlerp -= lf->blendlerp / cg_animBlend.value;
#endif
		if( lf->blendlerp <= 0.0f )
		{
			lf->blendlerp = 0.0f;
		}
		if( lf->blendlerp >= 1.0f )
		{
			lf->blendlerp = 1.0f;
		}
		
		lf->blendtime = cg.time + 10;
		
		debug_anim_blend = lf->blendlerp;
	}
}


/*
===============
CG_RunPlayerLerpFrame
===============
*/
static void CG_RunPlayerLerpFrame( clientInfo_t* ci, lerpFrame_t* lf, int newAnimation, float speedScale )
{
	int             f, numFrames;
	animation_t*    anim;
	qboolean        animChanged;
	
	// debugging tool to get no animations
	if( cg_animSpeed.integer == 0 )
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}
	
	// see if the animation sequence is switching
	if( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetPlayerLerpFrameAnimation( ci, lf, newAnimation );
		
		if( !lf->animation )
		{
			memcpy( &lf->oldSkeleton, &lf->skeleton, sizeof( refSkeleton_t ) );
		}
		
		animChanged = qtrue;
	}
	else
	{
		animChanged = qfalse;
	}
	
	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if( cg.time >= lf->frameTime || animChanged )
	{
		if( animChanged )
		{
			lf->oldFrame = 0;
			lf->oldFrameTime = cg.time;
		}
		else
		
		{
			lf->oldFrame = lf->frame;
			lf->oldFrameTime = lf->frameTime;
		}
		
		// get the next frame based on the animation
		anim = lf->animation;
		if( !anim->frameTime )
		{
			return;				// shouldn't happen
		}
		
		if( cg.time < lf->animationStartTime )
		{
			lf->frameTime = lf->animationStartTime;	// initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameTime;
		}
		f = ( lf->frameTime - lf->animationStartTime ) / anim->frameTime;
		f *= speedScale;		// adjust for haste, etc
		
		numFrames = anim->numFrames;
		
		if( anim->flipflop )
		{
			numFrames *= 2;
		}
		
		if( f >= numFrames )
		{
			f -= numFrames;
			
			if( anim->loopFrames )
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		
		if( anim->reversed )
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if( anim->flipflop && f >= anim->numFrames )
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - ( f % anim->numFrames );
		}
		else
		{
			lf->frame = anim->firstFrame + f;
		}
		
		if( cg.time > lf->frameTime )
		{
			lf->frameTime = cg.time;
			if( cg_debugPlayerAnim.integer )
			{
				CG_Printf( "clamp player lf->frameTime\n" );
			}
		}
	}
	
	if( lf->frameTime > cg.time + 200 )
	{
		lf->frameTime = cg.time;
	}
	
	if( lf->oldFrameTime > cg.time )
	{
		lf->oldFrameTime = cg.time;
	}
	
	// calculate current lerp value
	if( lf->frameTime == lf->oldFrameTime )
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - ( float )( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
	
	// blend old and current animation
	CG_BlendPlayerLerpFrame( lf );
	
	if( !trap_R_BuildSkeleton( &lf->skeleton, lf->animation->handle, lf->oldFrame, lf->frame, 1.0 - lf->backlerp, lf->animation->clearOrigin ) )
	{
		CG_Printf( "Can't build lf->skeleton\n" );
	}
	
	// lerp between old and new animation if possible
	if( lf->blendlerp > 0.0f )
	{
		if( !trap_R_BlendSkeleton( &lf->skeleton, &lf->oldSkeleton, lf->blendlerp ) )
		{
			CG_Printf( "Can't blend\n" );
			return;
		}
	}
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t* ci, lerpFrame_t* lf, int animationNumber )
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetPlayerLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

/*
===============
CG_PlayerAnimation
===============
*/

static void CG_PlayerAnimation( centity_t* cent )
{
	clientInfo_t*   ci;
	int             clientNum;
	float           speedScale;
	
	clientNum = cent->currentState.clientNum;
	
	if( cg_noPlayerAnims.integer )
	{
		return;
	}
	
	if( cent->currentState.powerups & ( 1 << PW_HASTE ) )
	{
		speedScale = 1.5;
	}
	else
	{
		speedScale = 1;
	}
	
	ci = &cgs.clientinfo[clientNum];
	
	// do the shuffle turn frames locally
	if( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE )
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}
	
	CG_RunPlayerLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );
}


/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance, float speed, float* angle, qboolean* swinging )
{
	float           swing;
	float           move;
	float           scale;
	
	if( !*swinging )
	{
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		
		if( swing > swingTolerance || swing < -swingTolerance )
		{
			*swinging = qtrue;
		}
	}
	
	if( !*swinging )
	{
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	
	if( scale < swingTolerance * 0.5 )
	{
		scale = 0.5;
	}
	else if( scale < swingTolerance )
	{
		scale = 1.0;
	}
	else
	{
		scale = 2.0;
	}
	
	// swing towards the destination angle
	if( swing >= 0 )
	{
		move = cg.frametime * scale * speed;
		
		if( move >= swing )
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleNormalize360( *angle + move );
	}
	else if( swing < 0 )
	{
		move = cg.frametime * scale * -speed;
		
		if( move <= swing )
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleNormalize360( *angle + move );
	}
	
	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if( swing > clampTolerance )
	{
		*angle = AngleNormalize360( destination - ( clampTolerance - 1 ) );
	}
	else if( swing < -clampTolerance )
	{
		*angle = AngleNormalize360( destination + ( clampTolerance - 1 ) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
void CG_AddPainTwitch( centity_t* cent, vec3_t torsoAngles )
{
	int             t;
	float           f;
	
	t = cg.time - cent->pe.painTime;
	
	if( t >= PAIN_TWITCH_TIME )
	{
		return;
	}
	
	f = 1.0 - ( float )t / PAIN_TWITCH_TIME;
	
	if( cent->pe.painDirection )
	{
		torsoAngles[ROLL] += 20 * f;
	}
	else
	{
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t* cent, const vec3_t sourceAngles, vec3_t legsAngles, vec3_t torsoAngles, vec3_t headAngles )
{
	float           dest;
	static int      movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	//vec3_t          velocity;
	//float           speed;
	int             dir, clientNum;
	clientInfo_t*   ci;
	
	VectorCopy( sourceAngles, headAngles );
	headAngles[YAW] = AngleNormalize360( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );
	
	// --------- yaw -------------
	
	// allow yaw to drift a bit
	if( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE
			|| ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND )
	{
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}
	
	// adjust legs for movement dir
	if( cent->currentState.eFlags & EF_DEAD )
	{
		// don't let dead bodies twitch
		dir = 0;
	}
	else
	{
		// TA: did use angles2.. now uses time2.. looks a bit funny but time2 isn't used othwise
		dir = cent->currentState.time2;
		if( dir < 0 || dir > 7 )
		{
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[dir];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[dir];
	
	// torso
	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
	
	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;
	
	
	// --------- pitch -------------
	
	// only show a fraction of the pitch angle in the torso
	if( headAngles[PITCH] > 180 )
	{
		dest = ( -360 + headAngles[PITCH] ) * 0.75f;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
	
	//
	clientNum = cent->currentState.clientNum;
	if( clientNum >= 0 && clientNum < MAX_CLIENTS )
	{
		ci = &cgs.clientinfo[clientNum];
		if( ci->fixedtorso )
		{
			torsoAngles[PITCH] = 0.0f;
		}
	}
	
	// --------- roll -------------
	
	
	// lean towards the direction of travel
	/*	VectorCopy(cent->currentState.pos.trDelta, velocity);
		speed = VectorNormalize(velocity);
		if(speed)
		{
			vec3_t          axis[3];
			float           side;
	
			speed *= 0.02f;
	
			AnglesToAxis(legsAngles, axis);
			side = speed * DotProduct(velocity, axis[1]);
			legsAngles[ROLL] -= side;
	
			side = speed * DotProduct(velocity, axis[0]);
			legsAngles[PITCH] += side;
		}
	*/
	//
	clientNum = cent->currentState.clientNum;
	if( clientNum >= 0 && clientNum < MAX_CLIENTS )
	{
		ci = &cgs.clientinfo[clientNum];
		if( ci->fixedlegs )
		{
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}
	
	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );
	
	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
}

//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail( centity_t* cent )
{
	localEntity_t*  smoke;
	vec3_t          origin;
	int             anim;
	
	if( cent->trailTime > cg.time )
	{
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if( anim != LEGS_RUN && anim != LEGS_BACK )
	{
		return;
	}
	
	cent->trailTime += 100;
	if( cent->trailTime < cg.time )
	{
		cent->trailTime = cg.time;
	}
	
	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= 16;
	
	smoke = CG_SmokePuff( origin, vec3_origin, 8, 1, 1, 1, 1, 500, cg.time, 0, 0, cgs.media.hastePuffShader );
	
	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

/*
===============
CG_BreathPuffs
===============
*/
void CG_BreathPuffs( centity_t* cent, const vec3_t headOrigin, const vec3_t headDirection )
{
#ifdef MISSIONPACK
	clientInfo_t*   ci;
	vec3_t          up, origin;
	int             contents;
	
	ci = &cgs.clientinfo[cent->currentState.number];
	
	if( !cg_enableBreath.integer )
	{
		return;
	}
	if( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
	{
		return;
	}
	if( cent->currentState.eFlags & EF_DEAD )
	{
		return;
	}
	contents = trap_CM_PointContents( headOrigin, 0 );
	if( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		return;
	}
	if( ci->breathPuffTime > cg.time )
	{
		return;
	}
	
	VectorSet( up, 0, 0, 8 );
	VectorMA( headOrigin, 8, headDirection, origin );
	//VectorMA(origin, -4, head->axis[2], origin);
	
	CG_SmokePuff( origin, up, 16, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE,
				  cgs.media.shotgunSmokePuffShader );
	ci->breathPuffTime = cg.time + 2000;
#endif
}

/*
===============
CG_DustTrail
===============
*/
void CG_DustTrail( centity_t* cent )
{
#ifdef MISSIONPACK
	int             anim;
	localEntity_t*  dust;
	vec3_t          end, vel;
	trace_t         tr;
	
	if( !cg_enableDust.integer )
	{
		return;
	}
	
	if( cent->dustTrailTime > cg.time )
	{
		return;
	}
	
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if( anim != LEGS_LANDB && anim != LEGS_LAND )
	{
		return;
	}
	
	cent->dustTrailTime += 40;
	if( cent->dustTrailTime < cg.time )
	{
		cent->dustTrailTime = cg.time;
	}
	
	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 64;
	CG_Trace( &tr, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );
	
	if( !( tr.surfaceFlags & SURF_DUST ) )
	{
		return;
	}
	
	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 16;
	
	VectorSet( vel, 0, 0, -30 );
	dust = CG_SmokePuff( end, vel, 24, .8f, .8f, 0.7f, 0.33f, 500, cg.time, 0, 0, cgs.media.shotgunSmokePuffShader );
#endif
}


/*
===============
CG_TrailItem
===============
*/
/*static void CG_TrailItem(centity_t * cent, qhandle_t hModel, qhandle_t hSkin)
{
	refEntity_t     ent;
	vec3_t          angles;
	vec3_t          axis[3];

	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	ent.customSkin = hSkin;
	trap_R_AddRefEntityToScene(&ent);
}*/


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t* cent, qhandle_t hSkin, refEntity_t* body )
{
	refEntity_t     flag;
	vec3_t          angles, dir;
	int             legsAnim, flagAnim, updateangles;
	float           angle, d;
	vec3_t          axis[3];
	int             boneIndex;
	
	// show the flag model
	memset( &flag, 0, sizeof( flag ) );
	flag.hModel = cgs.media.flagModel;
	flag.customSkin = hSkin;
	VectorCopy( body->lightingOrigin, flag.lightingOrigin );
	flag.shadowPlane = body->shadowPlane;
	flag.renderfx = body->renderfx;
	
	
	VectorCopy( cent->lerpAngles, angles );
	AnglesToAxis( angles, flag.axis );
	
	VectorClear( angles );
	
	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	
	if( legsAnim == LEGS_RUN )
	{
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}
	else
	{
		flagAnim = FLAG_IDLE;
	}
	
#if 1
	boneIndex = trap_R_BoneIndex( body->hModel, "tag_flag" );
	if( updateangles && boneIndex >= 0 && boneIndex < cent->pe.legs.skeleton.numBones )
	{
		VectorCopy( cent->currentState.pos.trDelta, dir );
		
		// add gravity
		dir[2] += 100;
		VectorNormalize( dir );
		d = DotProduct( flag.axis[2], dir );
		
		// if there is enough movement orthogonal to the flag pole
		if( fabs( d ) < 0.9 )
		{
			//
			d = DotProduct( flag.axis[0], dir );
			if( d > 1.0f )
			{
				d = 1.0f;
			}
			else if( d < -1.0f )
			{
				d = -1.0f;
			}
			angle = acos( d );
			
			d = DotProduct( flag.axis[1], dir );
			if( d < 0 )
			{
				angles[YAW] = 360 - angle * 180 / M_PI;
			}
			else
			{
				angles[YAW] = angle * 180 / M_PI;
			}
			if( angles[YAW] < 0 )
			{
				angles[YAW] += 360;
			}
			if( angles[YAW] > 360 )
			{
				angles[YAW] -= 360;
			}
			
			//VectorToAngles( cent->currentState.pos.trDelta, tmpangles );
			//angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
			// change the yaw angle
			CG_SwingAngles( angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing );
		}
		
		/*
		   d = DotProduct(pole.axis[2], dir);
		   angle = Q_acos(d);
		
		   d = DotProduct(pole.axis[1], dir);
		   if (d < 0) {
		   angle = 360 - angle * 180 / M_PI;
		   }
		   else {
		   angle = angle * 180 / M_PI;
		   }
		   if (angle > 340 && angle < 20) {
		   flagAnim = FLAG_RUNUP;
		   }
		   if (angle > 160 && angle < 200) {
		   flagAnim = FLAG_RUNDOWN;
		   }
		 */
	}
	
	// set the yaw angle
	angles[YAW] = cent->pe.flag.yawAngle;
#endif
	
	// lerp the flag animation frames
	CG_RunLerpFrame( &cent->pe.flag, cgs.media.flagAnimations, MAX_FLAG_ANIMATIONS, flagAnim, 1.0f );
	
	memcpy( &flag.skeleton, &cent->pe.flag.skeleton, sizeof( refSkeleton_t ) );
	
	// transform relative bones to absolute ones required for vertex skinning
	CG_TransformSkeleton( &flag.skeleton, NULL );
	
#if 1
	// FIXME: the flag is rotated because of the old CG_TrailItem code
	angles[YAW] += 90;
#endif
	
	AnglesToAxis( angles, flag.axis );
	
	if( !CG_PositionRotatedEntityOnBone( &flag, body, body->hModel, "tag_flag" ) )
	{
		// HACK: support the shina model as it misses a tag_flag bone
		boneIndex = trap_R_BoneIndex( body->hModel, "upper_torso" );
		if( boneIndex >= 0 && boneIndex < cent->pe.legs.skeleton.numBones )
		{
			matrix_t        bodyToBone, inverse;
			
			MatrixSetupTransformFromQuat( bodyToBone, body->skeleton.bones[boneIndex].rotation, body->skeleton.bones[boneIndex].origin );
			MatrixAffineInverse( bodyToBone, inverse );
			
			MatrixMultiplyRotation( inverse, angles[PITCH], angles[YAW], angles[ROLL] );
			MatrixToVectorsFLU( inverse, flag.axis[0], flag.axis[1], flag.axis[2] );
			
			angles[ROLL] += 90;
			angles[YAW] += 90;
			AnglesToAxis( angles, flag.axis );
			
			CG_PositionRotatedEntityOnBone( &flag, body, body->hModel, "upper_torso" );
			
			// move flag behind shina
			VectorCopy( cent->lerpAngles, angles );
			AnglesToAxis( angles, axis );
			VectorMA( flag.origin, -8, axis[0], flag.origin );
		}
		else
		{
			VectorCopy( cent->lerpAngles, angles );
			//angles[PITCH] = 0;
			//angles[ROLL] = 0;
			AnglesToAxis( angles, axis );
			
			VectorMA( cent->lerpOrigin, -8, axis[0], flag.origin );
			flag.origin[2] += 16;
			angles[YAW] += 90;
			AnglesToAxis( angles, flag.axis );
		}
	}
	
	trap_R_AddRefEntityToScene( &flag );
}


/*
===============
CG_PlayerTokens
===============
*/
void CG_PlayerTokens( centity_t* cent, int renderfx )
{
#ifdef MISSIONPACK
	int             tokens, i, j;
	float           angle;
	refEntity_t     ent;
	vec3_t          dir, origin;
	skulltrail_t*   trail;
	
	trail = &cg.skulltrails[cent->currentState.number];
	tokens = cent->currentState.generic1;
	if( !tokens )
	{
		trail->numpositions = 0;
		return;
	}
	
	if( tokens > MAX_SKULLTRAIL )
	{
		tokens = MAX_SKULLTRAIL;
	}
	
	// add skulls if there are more than last time
	for( i = 0; i < tokens - trail->numpositions; i++ )
	{
		for( j = trail->numpositions; j > 0; j-- )
		{
			VectorCopy( trail->positions[j - 1], trail->positions[j] );
		}
		VectorCopy( cent->lerpOrigin, trail->positions[0] );
	}
	trail->numpositions = tokens;
	
	// move all the skulls along the trail
	VectorCopy( cent->lerpOrigin, origin );
	for( i = 0; i < trail->numpositions; i++ )
	{
		VectorSubtract( trail->positions[i], origin, dir );
		if( VectorNormalize( dir ) > 30 )
		{
			VectorMA( origin, 30, dir, trail->positions[i] );
		}
		VectorCopy( trail->positions[i], origin );
	}
	
	memset( &ent, 0, sizeof( ent ) );
	ent.hModel = cgs.media.harvesterSkullModel;
	
	if( cgs.clientinfo[cent->currentState.clientNum].team == TEAM_BLUE )
	{
		ent.customSkin = cgs.media.redSkullSkin;
	}
	else
	{
		ent.customSkin = cgs.media.blueSkullSkin;
	}
	ent.renderfx = renderfx;
	
	VectorCopy( cent->lerpOrigin, origin );
	for( i = 0; i < trail->numpositions; i++ )
	{
		VectorSubtract( origin, trail->positions[i], ent.axis[0] );
		ent.axis[0][2] = 0;
		VectorNormalize( ent.axis[0] );
		VectorSet( ent.axis[2], 0, 0, 1 );
		CrossProduct( ent.axis[0], ent.axis[2], ent.axis[1] );
		
		VectorCopy( trail->positions[i], ent.origin );
		angle = ( ( ( cg.time + 500 * MAX_SKULLTRAIL - 500 * i ) / 16 ) & 255 ) * ( M_PI * 2 ) / 255;
		ent.origin[2] += sin( angle ) * 10;
		trap_R_AddRefEntityToScene( &ent );
		VectorCopy( trail->positions[i], origin );
	}
#endif
}


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t* cent, refEntity_t* torso, int noShadowID )
{
	int             powerups;
	clientInfo_t*   ci;
	refLight_t      light;
	float           radius;
	
	powerups = cent->currentState.powerups;
	if( !powerups )
	{
		return;
	}
	
	// quad gives a light
	if( powerups & ( 1 << PW_QUAD ) )
	{
		// add light
		memset( &light, 0, sizeof( refLight_t ) );
		
		light.rlType = RL_OMNI;
		
		VectorCopy( cent->lerpOrigin, light.origin );
		
		light.color[0] = 0.2f;
		light.color[1] = 0.2f;
		light.color[2] = 1;
		
		radius = 200 + ( rand() & 31 );
		
		light.radius[0] = radius;
		light.radius[1] = radius;
		light.radius[2] = radius;
		
		QuatClear( light.rotation );
		
		light.noShadowID = noShadowID;
		
		trap_R_AddRefLightToScene( &light );
	}
	
	// flight plays a looped sound
	if( powerups & ( 1 << PW_FLIGHT ) )
	{
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound );
	}
	
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	
	// redflag
	if( powerups & ( 1 << PW_REDFLAG ) )
	{
		CG_PlayerFlag( cent, cgs.media.redFlagSkin, torso );
		
		// add light
		memset( &light, 0, sizeof( refLight_t ) );
		
		light.rlType = RL_OMNI;
		
		VectorCopy( cent->lerpOrigin, light.origin );
		
		light.color[0] = 1.0;
		light.color[1] = 0.2f;
		light.color[2] = 0.2f;
		
		radius = 200 + ( rand() & 31 );
		
		light.radius[0] = radius;
		light.radius[1] = radius;
		light.radius[2] = radius;
		
		QuatClear( light.rotation );
		
		light.noShadowID = noShadowID;
		
		trap_R_AddRefLightToScene( &light );
	}
	
	// blueflag
	if( powerups & ( 1 << PW_BLUEFLAG ) )
	{
		CG_PlayerFlag( cent, cgs.media.blueFlagSkin, torso );
		
		// add light
		memset( &light, 0, sizeof( refLight_t ) );
		
		light.rlType = RL_OMNI;
		
		VectorCopy( cent->lerpOrigin, light.origin );
		
		light.color[0] = 0.2f;
		light.color[1] = 0.2f;
		light.color[2] = 1;
		
		radius = 200 + ( rand() & 31 );
		
		light.radius[0] = radius;
		light.radius[1] = radius;
		light.radius[2] = radius;
		
		QuatClear( light.rotation );
		
		light.noShadowID = noShadowID;
		
		trap_R_AddRefLightToScene( &light );
	}
	
	// neutralflag
	if( powerups & ( 1 << PW_NEUTRALFLAG ) )
	{
		CG_PlayerFlag( cent, cgs.media.neutralFlagSkin, torso );
		
		// add light
		memset( &light, 0, sizeof( refLight_t ) );
		
		light.rlType = RL_OMNI;
		
		VectorCopy( cent->lerpOrigin, light.origin );
		
		light.color[0] = 1;
		light.color[1] = 1;
		light.color[2] = 1;
		
		radius = 200 + ( rand() & 31 );
		
		light.radius[0] = radius;
		light.radius[1] = radius;
		light.radius[2] = radius;
		
		QuatClear( light.rotation );
		
		light.noShadowID = noShadowID;
		
		trap_R_AddRefLightToScene( &light );
	}
	
	// haste leaves smoke trails
	if( powerups & ( 1 << PW_HASTE ) )
	{
		CG_HasteTrail( cent );
	}
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t* cent, qhandle_t shader )
{
	int             rf;
	refEntity_t     ent;
	vec3_t          surfNormal;
	
	if( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
	{
		rf = RF_THIRD_PERSON;	// only show in mirrors
	}
	else
	{
		rf = 0;
	}
	
	memset( &ent, 0, sizeof( ent ) );
	
	/*
		if(cent->currentState.eFlags & EF_WALLCLIMB && !(cent->currentState.eFlags & EF_DEAD) && !(cg.intermissionStarted))
		{
			if(cent->currentState.eFlags & EF_WALLCLIMBCEILING)
				VectorSet(surfNormal, 0.0f, 0.0f, -1.0f);
			else
				VectorCopy(cent->currentState.angles2, surfNormal);
		}
		else
	*/
	{
		VectorSet( surfNormal, 0.0f, 0.0f, 1.0f );
	}
	
	VectorMA( cent->lerpOrigin, 64.0f, surfNormal, ent.origin );
	
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
void CG_PlayerSprites( centity_t* cent )
{
	int             team;
	
	if( cent->currentState.eFlags & EF_CONNECTION )
	{
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}
	
	if( cent->currentState.eFlags & EF_TALK )
	{
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}
	
	if( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive );
		return;
	}
	
	if( cent->currentState.eFlags & EF_AWARD_EXCELLENT )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent );
		return;
	}
	
	if( cent->currentState.eFlags & EF_AWARD_GAUNTLET )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalGauntlet );
		return;
	}
	
	if( cent->currentState.eFlags & EF_AWARD_DEFEND )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalDefend );
		return;
	}
	
	if( cent->currentState.eFlags & EF_AWARD_ASSIST )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalAssist );
		return;
	}
	
	if( cent->currentState.eFlags & EF_AWARD_CAP )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture );
		return;
	}
	
	team = cgs.clientinfo[cent->currentState.clientNum].team;
	if( !( cent->currentState.eFlags & EF_DEAD ) && cg.snap->ps.persistant[PERS_TEAM] == team && cgs.gametype >= GT_TEAM )
	{
		if( cg_drawFriend.integer )
		{
			CG_PlayerFloatSprite( cent, cgs.media.friendShader );
		}
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
qboolean CG_PlayerShadow( centity_t* cent, float* shadowPlane, int noShadowID )
{
	vec3_t          end, mins = { -15, -15, 0 }, maxs = { 15, 15, 2 };
	trace_t         trace;
	float           alpha;
	entityState_t*  es = &cent->currentState;
	vec3_t          surfNormal = { 0.0f, 0.0f, 1.0f };
	
	*shadowPlane = 0;
	
	if( cg_shadows.integer != 1 )
	{
		return qfalse;
	}
	
	// no shadows when invisible
	if( cent->currentState.powerups & ( 1 << PW_INVIS ) )
	{
		return qfalse;
	}
	
	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	VectorMA( cent->lerpOrigin, -SHADOW_DISTANCE, surfNormal, end );
	
	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );
	
	// no shadow if too high
	if( trace.fraction == 1.0 || trace.startsolid || trace.allsolid )
	{
		return qfalse;
	}
	
	if( surfNormal[2] < 0.0f )
	{
		*shadowPlane = trace.endpos[2] - 1.0f;
	}
	else
	{
		*shadowPlane = trace.endpos[2] + 1.0f;
	}
	
	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;
	
#if 0							// FIXME
	if( ( cg_shadows.integer >= SHADOWING_ESM16 && cg_shadows.integer <= SHADOWING_EVSM ) && cg_precomputedLighting.integer )
	{
		refLight_t      light;
		vec3_t          angles;
		
		//float         angle;
		vec3_t          projectionEnd;
		
		vec3_t          ambientLight;
		vec3_t          lightDir;
		vec3_t          lightDirInversed;
		vec3_t          directedLight;
		
		static vec3_t   mins = { -4, -4, -4 };
		static vec3_t   maxs = { 4, 4, 4 };
		
		trap_R_LightForPoint( cent->lerpOrigin, ambientLight, directedLight, lightDir );
		VectorNegate( lightDir, lightDirInversed );
		
		// add light
		memset( &light, 0, sizeof( refLight_t ) );
		
		light.rlType = RL_PROJ;
		
		
		// find light origin
		VectorMA( cent->lerpOrigin, SHADOW_DISTANCE, lightDir, light.origin );
		trap_CM_BoxTrace( &trace, cent->lerpOrigin, light.origin, mins, maxs, 0, MASK_SOLID );
		
		// no shadow if too high
		/*
		   if(trace.fraction == 1.0 || trace.startsolid || trace.allsolid)
		   {
		   return qfalse;
		   }
		 */
		
		VectorCopy( trace.endpos, light.origin );
		//VectorMA(refdef.vieworg, -200, refdef.viewaxis[0], light.origin);
		//light.origin[1] += 10;
		
		// find projection end
		VectorMA( light.origin, SHADOW_DISTANCE * 100, lightDirInversed, projectionEnd );
		trap_CM_BoxTrace( &trace, light.origin, projectionEnd, mins, maxs, 0, MASK_PLAYERSOLID );
#if 0
		if( /* trace.fraction == 1.0 || */ trace.startsolid || trace.allsolid )
		{
			return qfalse;
		}
#endif
		
#if 0
		angle = AngleBetweenVectors( lightDirInversed, surfNormal );
		if( angle > 30 )
		{
			VectorCopy( surfNormal, lightDirInversed );
		}
#endif
		
		VectorToAngles( lightDirInversed, angles );
		QuatFromAngles( light.rotation, angles[PITCH], angles[YAW], angles[ROLL] );
		
		light.color[0] = 0.9f * alpha;
		light.color[1] = 0.9f * alpha;
		light.color[2] = 0.9f * alpha;
		
		light.fovX = 35;
		light.fovY = 35;
		light.distNear = 1;
		light.distFar = Distance( light.origin, trace.endpos ) + SHADOW_DISTANCE;
		
		light.noShadowID = noShadowID;
		light.inverseShadows = qtrue;
		light.attenuationShader = cgs.media.shadowProjectedLightShader;
		
		trap_R_AddRefLightToScene( &light );
		return qtrue;
	}
#endif
	
	*shadowPlane = trace.endpos[2] + 1;
	
	if( cg_shadows.integer != 1 )
	{
		// no mark for stencil or projection shadows
		return qtrue;
	}
	
	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f )
	
	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
				   cent->pe.legs.yawAngle, alpha, alpha, alpha, 1, qfalse, 24, qtrue );
				   
	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
void CG_PlayerSplash( centity_t* cent )
{
#if 0
	vec3_t          start, end;
	trace_t         trace;
	int             contents;
	polyVert_t      verts[4];
	
	if( !cg_shadows.integer )
	{
		return;
	}
	
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;
	
	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) )
	{
		return;
	}
	
	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;
	
	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		return;
	}
	
	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );
	
	if( trace.fraction == 1.0 )
	{
		return;
	}
	
	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;
	
	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;
	
	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;
	
	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;
	
	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
#endif
}



/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t* ent, entityState_t* state, int team )
{

	if( state->eFlags & EF_DEAD )
	{
		trap_R_AddRefEntityToScene( ent );
		
		ent->customShader = cgs.media.unlinkEffect;
		trap_R_AddRefEntityToScene( ent );
	}
	else if( state->powerups & ( 1 << PW_INVIS ) )
	{
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( ent );
	}
	else
	{
		/*
		   if ( state->eFlags & EF_KAMIKAZE ) {
		   if (team == TEAM_BLUE)
		   ent->customShader = cgs.media.blueKamikazeShader;
		   else
		   ent->customShader = cgs.media.redKamikazeShader;
		   trap_R_AddRefEntityToScene( ent );
		   }
		   else { */
		trap_R_AddRefEntityToScene( ent );
		//}
		
		if( state->powerups & ( 1 << PW_QUAD ) )
		{
			if( team == TEAM_RED )
			{
				ent->customShader = cgs.media.redQuadShader;
			}
			else
			{
				ent->customShader = cgs.media.quadShader;
			}
			trap_R_AddRefEntityToScene( ent );
		}
		if( state->powerups & ( 1 << PW_REGEN ) )
		{
			if( ( ( cg.time / 100 ) % 10 ) == 1 )
			{
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene( ent );
			}
		}
		if( state->powerups & ( 1 << PW_BATTLESUIT ) )
		{
			ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene( ent );
		}
	}
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t* verts )
{
	int             i, j;
	float           incoming;
	vec3_t          ambientLight;
	vec3_t          lightDir;
	vec3_t          directedLight;
	
	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );
	
	for( i = 0; i < numVerts; i++ )
	{
		incoming = DotProduct( normal, lightDir );
		if( incoming <= 0 )
		{
			verts[i].modulate[0] = 255 * ambientLight[0];
			verts[i].modulate[1] = 255 * ambientLight[1];
			verts[i].modulate[2] = 255 * ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if( j > 1 )
		{
			j = 1;
		}
		verts[i].modulate[0] = j * 255;
		
		j = ( ambientLight[1] + incoming * directedLight[1] );
		if( j > 1 )
		{
			j = 1;
		}
		verts[i].modulate[1] = j * 255;
		
		j = ( ambientLight[2] + incoming * directedLight[2] );
		if( j > 1 )
		{
			j = 1;
		}
		verts[i].modulate[2] = j * 255;
		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
CG_Player
===============
*/
// has to be in sync with clientRespawnTime
#define DEATHANIM_TIME 1650
#define TRACE_DEPTH 64.0f
void CG_Player( centity_t* cent )
{
	clientInfo_t*   ci;
	refEntity_t     body;
	int             clientNum;
	int             renderfx;
	qboolean        shadow;
	float           shadowPlane;
	int             noShadowID;
	
	vec3_t          angles;
	vec3_t          legsAngles;
	vec3_t          torsoAngles;
	vec3_t          headAngles;
	
	matrix_t        bodyRotation;
	quat_t          torsoQuat;
	quat_t          headQuat;
	
	int             i;
	int             boneIndex;
	int             firstTorsoBone;
	int             lastTorsoBone;
	
	vec3_t          playerOrigin;
	
	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		CG_Error( "Bad clientNum on player entity" );
	}
	ci = &cgs.clientinfo[clientNum];
	
	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if( !ci->infoValid )
	{
		CG_Printf( "Bad clientInfo for player %i\n", clientNum );
		return;
	}
	
	// get the player model information
	renderfx = 0;
	if( cent->currentState.number == cg.snap->ps.clientNum )
	{
		if( !cg.renderingThirdPerson )
		{
			renderfx = RF_THIRD_PERSON;	// only draw in mirrors
		}
		else
		{
			if( cg_cameraMode.integer )
			{
				return;
			}
		}
	}
	
	
	memset( &body, 0, sizeof( body ) );
	
	// generate a new unique noShadowID to avoid that the lights of the quad damage
	// will cause bad player shadows
	noShadowID = CG_UniqueNoShadowID();
	body.noShadowID = noShadowID;
	
	AxisClear( body.axis );
	
	// get the rotation information
	VectorCopy( cent->lerpAngles, angles );
	
	// rotate lerpAngles to floor
	VectorCopy( cent->lerpAngles, angles );
	
	// normalize the pitch
	if( angles[PITCH] < -180.0f )
	{
		angles[PITCH] += 360.0f;
	}
	
	CG_PlayerAngles( cent, angles, legsAngles, torsoAngles, headAngles );
	AnglesToAxis( legsAngles, body.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent );
	
	// WIP: death effect
#if 1
	if( cent->currentState.eFlags & EF_DEAD )
	{
		int             time;
		
		if( cent->pe.deathTime <= 0 )
		{
			cent->pe.deathTime = cg.time;
			cent->pe.deathScale = 0.0f;
		}
		
		time = ( DEATHANIM_TIME - ( cg.time - cent->pe.deathTime ) );
		
		cent->pe.deathScale = 1.0f - ( 1.0f / DEATHANIM_TIME * time );
		if( cent->pe.deathScale >= 1.0f )
		{
			return;
		}
		
		body.shaderTime = -cent->pe.deathScale;
	}
	else
#endif
	{
		cent->pe.deathTime = 0;
		cent->pe.deathScale = 0.0f;
		
		body.shaderTime = 0.0f;
	}
	
	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );
	
	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane, noShadowID );
	
	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );
	
	if( cg_shadows.integer == 2 && shadow )
	{
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;	// use the same origin for all
	
	if( cgs.gametype == GT_HARVESTER )
	{
		CG_PlayerTokens( cent, renderfx );
	}
	
	// add the body
	body.hModel = ci->bodyModel;
	body.customSkin = ci->bodySkin;
	
	if( !body.hModel )
	{
		CG_Printf( "No body model for player %i\n", clientNum );
		return;
	}
	
	body.shadowPlane = shadowPlane;
	body.renderfx = renderfx;
	
	// move the origin closer into the wall with a CapTrace
#if 0
	if( cent->currentState.eFlags & EF_WALLCLIMB && !( cent->currentState.eFlags & EF_DEAD ) && !( cg.intermissionStarted ) )
	{
		vec3_t          start, end, mins, maxs;
		trace_t         tr;
		
		if( cent->currentState.eFlags & EF_WALLCLIMBCEILING )
		{
			VectorSet( surfNormal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy( cent->currentState.angles2, surfNormal );
		}
		
		VectorCopy( playerMins, mins );
		VectorCopy( playerMaxs, maxs );
		
		VectorMA( cent->lerpOrigin, -TRACE_DEPTH, surfNormal, end );
		VectorMA( cent->lerpOrigin, 1.0f, surfNormal, start );
		CG_CapTrace( &tr, start, mins, maxs, end, cent->currentState.number, MASK_PLAYERSOLID );
		
		// if the trace misses completely then just use body.origin
		// apparently capsule traces are "smaller" than box traces
		if( tr.fraction != 1.0f )
		{
			VectorCopy( tr.endpos, playerOrigin );
			
			// MD5 player models have their model origin at (0 0 0)
			VectorMA( playerOrigin, playerMins[2], surfNormal, body.origin );
		}
		else
		{
			VectorCopy( cent->lerpOrigin, playerOrigin );
			
			// MD5 player models have their model origin at (0 0 0)
			VectorMA( cent->lerpOrigin, -TRACE_DEPTH, surfNormal, body.origin );
		}
		
		VectorCopy( body.origin, body.lightingOrigin );
		VectorCopy( body.origin, body.oldorigin );	// don't positionally lerp at all
	}
	else
#endif
	{
		VectorCopy( cent->lerpOrigin, playerOrigin );
		VectorCopy( playerOrigin, body.origin );
		body.origin[2] += playerMins[2];
	}
	
	VectorCopy( body.origin, body.lightingOrigin );
	VectorCopy( body.origin, body.oldorigin );	// don't positionally lerp at all
	
	// copy legs skeleton to have a base
	memcpy( &body.skeleton, &cent->pe.legs.skeleton, sizeof( refSkeleton_t ) );
	
	if( cent->pe.legs.skeleton.numBones != cent->pe.torso.skeleton.numBones )
	{
		CG_Error( "cent->pe.legs.skeleton.numBones != cent->pe.torso.skeleton.numBones" );
		return;
	}
	
	// combine legs and torso skeletons
#if 1
	firstTorsoBone = trap_R_BoneIndex( body.hModel, ci->firstTorsoBoneName );
	
	if( firstTorsoBone >= 0 && firstTorsoBone < cent->pe.torso.skeleton.numBones )
	{
		lastTorsoBone = trap_R_BoneIndex( body.hModel, ci->lastTorsoBoneName );
		
		if( lastTorsoBone >= 0 && lastTorsoBone < cent->pe.torso.skeleton.numBones )
		{
			// copy torso bones
			for( i = firstTorsoBone; i < lastTorsoBone; i++ )
			{
				memcpy( &body.skeleton.bones[i], &cent->pe.torso.skeleton.bones[i], sizeof( refBone_t ) );
			}
		}
		
		body.skeleton.type = SK_RELATIVE;
		
		// update AABB
		for( i = 0; i < 3; i++ )
		{
			body.skeleton.bounds[0][i] =
				cent->pe.torso.skeleton.bounds[0][i] <
				cent->pe.legs.skeleton.bounds[0][i] ? cent->pe.torso.skeleton.bounds[0][i] : cent->pe.legs.skeleton.bounds[0][i];
			body.skeleton.bounds[1][i] =
				cent->pe.torso.skeleton.bounds[1][i] >
				cent->pe.legs.skeleton.bounds[1][i] ? cent->pe.torso.skeleton.bounds[1][i] : cent->pe.legs.skeleton.bounds[1][i];
		}
	}
	else
	{
		// bad no hips found
		body.skeleton.type = SK_INVALID;
	}
#endif
	
	// rotate legs
#if 0
	boneIndex = trap_R_BoneIndex( body.hModel, "origin" );
	
	if( boneIndex >= 0 && boneIndex < cent->pe.legs.skeleton.numBones )
	{
		// HACK: convert angles to bone system
		QuatFromAngles( legsQuat, legsAngles[YAW], -legsAngles[ROLL], legsAngles[PITCH] );
		QuatMultiply0( body.skeleton.bones[boneIndex].rotation, legsQuat );
	}
#endif
	
	
	// rotate torso
#if 1
	boneIndex = trap_R_BoneIndex( body.hModel, ci->torsoControlBoneName );
	
	if( boneIndex >= 0 && boneIndex < cent->pe.legs.skeleton.numBones )
	{
		// HACK: convert angles to bone system
		QuatFromAngles( torsoQuat, torsoAngles[YAW], torsoAngles[ROLL], -torsoAngles[PITCH] );
		QuatMultiply0( body.skeleton.bones[boneIndex].rotation, torsoQuat );
	}
#endif
	
	// rotate head
#if 1
	boneIndex = trap_R_BoneIndex( body.hModel, ci->neckControlBoneName );
	
	if( boneIndex >= 0 && boneIndex < cent->pe.legs.skeleton.numBones )
	{
		// HACK: convert angles to bone system
		QuatFromAngles( headQuat, headAngles[YAW], headAngles[ROLL], -headAngles[PITCH] );
		QuatMultiply0( body.skeleton.bones[boneIndex].rotation, headQuat );
	}
#endif
	
	// transform relative bones to absolute ones required for vertex skinning and tag attachments
	CG_TransformSkeleton( &body.skeleton, ci->modelScale );
	
	// add body to renderer
	CG_AddRefEntityWithPowerups( &body, &cent->currentState, ci->team );
	
	
#if 0
	// Tr3B: it might be better to have a tag_mouth bone joint
	boneIndex = trap_R_BoneIndex( body.hModel, ci->neckControlBoneName );
	if( boneIndex >= 0 && boneIndex < cent->pe.legs.skeleton.numBones )
	{
		CG_BreathPuffs( cent, body.skeleton.bones[boneIndex] );
	}
#endif
	
	CG_DustTrail( cent );
	
#ifdef MISSIONPACK
	if( cent->currentState.eFlags & EF_KAMIKAZE )
	{
		refEntity_t     skull;
		float           angle;
		vec3_t          dir, angles;
		
		
		memset( &skull, 0, sizeof( skull ) );
		
		VectorCopy( cent->lerpOrigin, skull.lightingOrigin );
		skull.shadowPlane = shadowPlane;
		skull.renderfx = renderfx;
		
		if( cent->currentState.eFlags & EF_DEAD )
		{
			// one skull bobbing above the dead body
			angle = ( ( cg.time / 7 ) & 255 ) * ( M_PI * 2 ) / 255;
			if( angle > M_PI * 2 )
			{
				angle -= ( float )M_PI * 2;
			}
			
			dir[0] = sin( angle ) * 20;
			dir[1] = cos( angle ) * 20;
			angle = ( ( cg.time / 4 ) & 255 ) * ( M_PI * 2 ) / 255;
			dir[2] = 15 + sin( angle ) * 8;
			VectorAdd( body.origin, dir, skull.origin );
			skull.origin[2] -= playerMins[2];
			
			dir[2] = 0;
			VectorCopy( dir, skull.axis[1] );
			VectorNormalize( skull.axis[1] );
			VectorSet( skull.axis[2], 0, 0, 1 );
			CrossProduct( skull.axis[1], skull.axis[2], skull.axis[0] );
			
			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );
		}
		else
		{
			// three skulls spinning around the player
			angle = ( ( cg.time / 4 ) & 255 ) * ( M_PI * 2 ) / 255;
			dir[0] = cos( angle ) * 20;
			dir[1] = sin( angle ) * 20;
			dir[2] = cos( angle ) * 20;
			VectorAdd( body.origin, dir, skull.origin );
			skull.origin[2] -= playerMins[2];
			
			angles[0] = sin( angle ) * 30;
			angles[1] = ( angle * 180 / M_PI ) + 90;
			if( angles[1] > 360 )
			{
				angles[1] -= 360;
			}
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );
			
			/*
			   dir[2] = 0;
			   VectorInverse(dir);
			   VectorCopy(dir, skull.axis[1]);
			   VectorNormalize(skull.axis[1]);
			   VectorSet(skull.axis[2], 0, 0, 1);
			   CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			 */
			
			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			
			// flip the trail because this skull is spinning in the other direction
			VectorInverse( skull.axis[1] );
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );
			
			angle = ( ( cg.time / 4 ) & 255 ) * ( M_PI * 2 ) / 255 + M_PI;
			if( angle > M_PI * 2 )
			{
				angle -= ( float )M_PI * 2;
			}
			
			dir[0] = sin( angle ) * 20;
			dir[1] = cos( angle ) * 20;
			dir[2] = cos( angle ) * 20;
			VectorAdd( body.origin, dir, skull.origin );
			skull.origin[2] -= playerMins[2];
			
			angles[0] = cos( angle - 0.5 * M_PI ) * 30;
			angles[1] = 360 - ( angle * 180 / M_PI );
			if( angles[1] > 360 )
			{
				angles[1] -= 360;
			}
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );
			
			/*
			   dir[2] = 0;
			   VectorCopy(dir, skull.axis[1]);
			   VectorNormalize(skull.axis[1]);
			   VectorSet(skull.axis[2], 0, 0, 1);
			   CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			 */
			
			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );
			
			angle = ( ( cg.time / 3 ) & 255 ) * ( M_PI * 2 ) / 255 + 0.5 * M_PI;
			if( angle > M_PI * 2 )
			{
				angle -= ( float )M_PI * 2;
			}
			
			dir[0] = sin( angle ) * 20;
			dir[1] = cos( angle ) * 20;
			dir[2] = 0;
			VectorAdd( body.origin, dir, skull.origin );
			skull.origin[2] -= playerMins[2];
			
			VectorCopy( dir, skull.axis[1] );
			VectorNormalize( skull.axis[1] );
			VectorSet( skull.axis[2], 0, 0, 1 );
			CrossProduct( skull.axis[1], skull.axis[2], skull.axis[0] );
			
			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );
		}
	}
#endif
	
#ifdef MISSIONPACK
	if( cent->currentState.powerups & ( 1 << PW_GUARD ) )
	{
		memcpy( &powerup, &torso, sizeof( torso ) );
		powerup.hModel = cgs.media.guardPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if( cent->currentState.powerups & ( 1 << PW_SCOUT ) )
	{
		memcpy( &powerup, &torso, sizeof( torso ) );
		powerup.hModel = cgs.media.scoutPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if( cent->currentState.powerups & ( 1 << PW_DOUBLER ) )
	{
		memcpy( &powerup, &torso, sizeof( torso ) );
		powerup.hModel = cgs.media.doublerPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if( cent->currentState.powerups & ( 1 << PW_AMMOREGEN ) )
	{
		memcpy( &powerup, &torso, sizeof( torso ) );
		powerup.hModel = cgs.media.ammoRegenPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if( cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) )
	{
		if( !ci->invulnerabilityStartTime )
		{
			ci->invulnerabilityStartTime = cg.time;
		}
		ci->invulnerabilityStopTime = cg.time;
	}
	else
	{
		ci->invulnerabilityStartTime = 0;
	}
	if( ( cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) || cg.time - ci->invulnerabilityStopTime < 250 )
	{
	
		memcpy( &powerup, &torso, sizeof( torso ) );
		powerup.hModel = cgs.media.invulnerabilityPowerupModel;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorCopy( cent->lerpOrigin, powerup.origin );
		
		if( cg.time - ci->invulnerabilityStartTime < 250 )
		{
			c = ( float )( cg.time - ci->invulnerabilityStartTime ) / 250;
		}
		else if( cg.time - ci->invulnerabilityStopTime < 250 )
		{
			c = ( float )( 250 - ( cg.time - ci->invulnerabilityStopTime ) ) / 250;
		}
		else
		{
			c = 1;
		}
		VectorSet( powerup.axis[0], c, 0, 0 );
		VectorSet( powerup.axis[1], 0, c, 0 );
		VectorSet( powerup.axis[2], 0, 0, c );
		trap_R_AddRefEntityToScene( &powerup );
	}
	
	t = cg.time - ci->medkitUsageTime;
	if( ci->medkitUsageTime && t < 500 )
	{
		memcpy( &powerup, &torso, sizeof( torso ) );
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.customSkin = 0;
		
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorClear( angles );
		AnglesToAxis( angles, powerup.axis );
		VectorCopy( cent->lerpOrigin, powerup.origin );
		powerup.origin[2] += -24 + ( float )t * 80 / 500;
		
		if( t > 400 )
		{
			c = ( float )( t - 1000 ) * 0xff / 100;
			powerup.shaderRGBA[0] = 0xff - c;
			powerup.shaderRGBA[1] = 0xff - c;
			powerup.shaderRGBA[2] = 0xff - c;
			powerup.shaderRGBA[3] = 0xff - c;
		}
		else
		{
			powerup.shaderRGBA[0] = 0xff;
			powerup.shaderRGBA[1] = 0xff;
			powerup.shaderRGBA[2] = 0xff;
			powerup.shaderRGBA[3] = 0xff;
		}
		trap_R_AddRefEntityToScene( &powerup );
	}
#endif
	
	// add the gun / barrel / flash
	CG_AddPlayerWeapon( &body, NULL, cent, ci->team );
	
	// add powerups floating behind the player
	CG_PlayerPowerups( cent, &body, noShadowID );
	
	// add the bounding box (if cg_drawPlayerCollision is 1)
	MatrixFromVectorsFLU( bodyRotation, body.axis[0], body.axis[1], body.axis[2] );
	CG_DrawPlayerCollision( cent, playerOrigin, bodyRotation );
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t* cent )
{
	cent->errorTime = -99999;	// guarantee no error decay added
	cent->extrapolated = qfalse;
	
	CG_ClearLerpFrame( &cgs.clientinfo[cent->currentState.clientNum], &cent->pe.legs, cent->currentState.legsAnim );
	CG_ClearLerpFrame( &cgs.clientinfo[cent->currentState.clientNum], &cent->pe.torso, cent->currentState.torsoAnim );
	
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );
	
	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );
	
	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;
	
	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;
	
	if( cg_debugPosition.integer )
	{
		CG_Printf( "%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}


/*
=================
CG_DrawPlayerCollision

Draws a bounding box around a player.  Called from CG_Player.
=================
*/
void CG_DrawPlayerCollision( centity_t* cent, const vec3_t bodyOrigin, const matrix_t bodyRotation )
{
	polyVert_t      verts[4];
	clientInfo_t*   ci;
	int             i;
	vec3_t          mins;
	vec3_t          maxs;
	float           extx, exty, extz;
	vec3_t          corners[8];
	matrix_t        transform;
	
	if( !cg_drawPlayerCollision.integer )
	{
		return;
	}
	
	// don't draw it if it's us in first-person
	if( cent->currentState.number == cg.predictedPlayerState.clientNum && !cg.renderingThirdPerson )
	{
		return;
	}
	
	// don't draw it for dead players
	if( cent->currentState.eFlags & EF_DEAD )
	{
		return;
	}
	
	// if they don't exist, forget it
	if( !cgs.media.debugPlayerAABB || !cgs.media.debugPlayerAABB_twoSided )
	{
		return;
	}
	
	// get the player's client info
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	
	VectorCopy( playerMins, mins );
	VectorCopy( playerMaxs, maxs );
	
	// if it's us
	if( cent->currentState.number == cg.predictedPlayerState.clientNum )
	{
		// use the view height
		maxs[2] = cg.predictedPlayerState.viewheight + 6;
	}
	else
	{
		int             x, zd, zu;
		
		// otherwise grab the encoded bounding box
		x = ( cent->currentState.solid & 255 );
		zd = ( ( cent->currentState.solid >> 8 ) & 255 );
		zu = ( ( cent->currentState.solid >> 16 ) & 255 ) - 32;
		
		mins[0] = mins[1] = -x;
		maxs[0] = maxs[1] = x;
		mins[2] = -zd;
		maxs[2] = zu;
	}
	
	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];
	
	
	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	
	// set the polygon's vertex colors
	if( ci->team == TEAM_RED )
	{
		for( i = 0; i < 4; i++ )
		{
			verts[i].modulate[0] = 160;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}
	else if( ci->team == TEAM_BLUE )
	{
		for( i = 0; i < 4; i++ )
		{
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 192;
			verts[i].modulate[3] = 255;
		}
	}
	else
	{
		for( i = 0; i < 4; i++ )
		{
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 128;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}
	
	//AxisCopy(tempAxis, cent->pe.lastAxis);
	
	//MatrixFromAngles(rotation, cent->lerpAngles[PITCH], cent->lerpAngles[YAW], cent->lerpAngles[ROLL]);
	//MatrixIdentity(rotation);
	MatrixSetupTransformFromRotation( transform, bodyRotation, bodyOrigin );
	
	//VectorAdd(cent->lerpOrigin, maxs, corners[3]);
	VectorCopy( maxs, corners[3] );
	
	VectorCopy( corners[3], corners[2] );
	corners[2][0] -= extx;
	
	VectorCopy( corners[2], corners[1] );
	corners[1][1] -= exty;
	
	VectorCopy( corners[1], corners[0] );
	corners[0][0] += extx;
	
	for( i = 0; i < 4; i++ )
	{
		VectorCopy( corners[i], corners[i + 4] );
		corners[i + 4][2] -= extz;
	}
	
	for( i = 0; i < 8; i++ )
	{
		MatrixTransformPoint2( transform, corners[i] );
	}
	
	// top
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[2], verts[2].xyz );
	VectorCopy( corners[3], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.debugPlayerAABB, 4, verts );
	
	// bottom
	VectorCopy( corners[7], verts[0].xyz );
	VectorCopy( corners[6], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.debugPlayerAABB, 4, verts );
	
	// top side
	VectorCopy( corners[3], verts[0].xyz );
	VectorCopy( corners[2], verts[1].xyz );
	VectorCopy( corners[6], verts[2].xyz );
	VectorCopy( corners[7], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.debugPlayerAABB_twoSided, 4, verts );
	
	// left side
	VectorCopy( corners[2], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[6], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.debugPlayerAABB_twoSided, 4, verts );
	
	// right side
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[3], verts[1].xyz );
	VectorCopy( corners[7], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.debugPlayerAABB_twoSided, 4, verts );
	
	// bottom side
	VectorCopy( corners[1], verts[0].xyz );
	VectorCopy( corners[0], verts[1].xyz );
	VectorCopy( corners[4], verts[2].xyz );
	VectorCopy( corners[5], verts[3].xyz );
	trap_R_AddPolyToScene( cgs.media.debugPlayerAABB_twoSided, 4, verts );
}
