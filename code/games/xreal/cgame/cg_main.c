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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#ifdef MISSIONPACK
	#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;
#endif

int		 forceModelModificationCount = -1;

void	 CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void	 CG_Shutdown();

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 )
{
	switch( command )
	{
		case CG_INIT:
			CG_Init( arg0, arg1, arg2 );
			return 0;
		case CG_SHUTDOWN:
			CG_Shutdown();
			return 0;
		case CG_CONSOLE_COMMAND:
			return CG_ConsoleCommand();
		case CG_DRAW_ACTIVE_FRAME:
			CG_DrawActiveFrame( arg0, arg1, arg2 );
			return 0;
		case CG_CROSSHAIR_PLAYER:
			return CG_CrosshairPlayer();
		case CG_LAST_ATTACKER:
			return CG_LastAttacker();
		case CG_KEY_EVENT:
			CG_KeyEvent( arg0, arg1 );
			return 0;
		case CG_MOUSE_EVENT:
#ifdef MISSIONPACK
			cgDC.cursorx = cgs.cursorX;
			cgDC.cursory = cgs.cursorY;
#endif
			CG_MouseEvent( arg0, arg1 );
			return 0;
		case CG_EVENT_HANDLING:
			CG_EventHandling( arg0 );
			return 0;
		default:
			CG_Error( "vmMain: unknown command %i", command );
			break;
	}
	return -1;
}

cg_t		 cg;
cgs_t		 cgs;
centity_t	 cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t	 cg_items[MAX_ITEMS];

vmCvar_t	 cg_railTrailTime;
vmCvar_t	 cg_centertime;
vmCvar_t	 cg_runpitch;
vmCvar_t	 cg_runroll;
vmCvar_t	 cg_bobup;
vmCvar_t	 cg_bobpitch;
vmCvar_t	 cg_bobroll;
vmCvar_t	 cg_swingSpeed;
vmCvar_t	 cg_shadows;
vmCvar_t	 cg_precomputedLighting;
vmCvar_t	 cg_gibs;
vmCvar_t	 cg_drawTimer;
vmCvar_t	 cg_drawFPS;
vmCvar_t	 cg_drawSnapshot;
vmCvar_t	 cg_draw3dIcons;
vmCvar_t	 cg_drawIcons;
vmCvar_t	 cg_drawAmmoWarning;
vmCvar_t	 cg_drawCrosshair;
vmCvar_t	 cg_drawCrosshairNames;
vmCvar_t	 cg_drawRewards;
vmCvar_t	 cg_crosshairSize;
vmCvar_t	 cg_crosshairX;
vmCvar_t	 cg_crosshairY;
vmCvar_t	 cg_crosshairHealth;
vmCvar_t	 cg_crosshairDot;
vmCvar_t	 cg_crosshairCircle;
vmCvar_t	 cg_crosshairCross;
vmCvar_t	 cg_crosshairPulse;
vmCvar_t	 cg_draw2D;
vmCvar_t	 cg_drawStatus;
vmCvar_t	 cg_animSpeed;
vmCvar_t	 cg_animBlend;
vmCvar_t	 cg_debugPlayerAnim;
vmCvar_t	 cg_debugWeaponAnim;
vmCvar_t	 cg_debugPosition;
vmCvar_t	 cg_debugEvents;
vmCvar_t	 cg_errorDecay;
vmCvar_t	 cg_nopredict;
vmCvar_t	 cg_noPlayerAnims;
vmCvar_t	 cg_showmiss;
vmCvar_t	 cg_footsteps;
vmCvar_t	 cg_addMarks;
vmCvar_t	 cg_brassTime;
vmCvar_t	 cg_viewsize;
vmCvar_t	 cg_drawGun;
vmCvar_t	 cg_gun_frame;
vmCvar_t	 cg_gunX;
vmCvar_t	 cg_gunY;
vmCvar_t	 cg_gunZ;
vmCvar_t	 cg_tracerChance;
vmCvar_t	 cg_tracerWidth;
vmCvar_t	 cg_tracerLength;
vmCvar_t	 cg_autoswitch;
vmCvar_t	 cg_ignore;
vmCvar_t	 cg_simpleItems;
vmCvar_t	 cg_fov;
vmCvar_t	 cg_zoomFov;
vmCvar_t	 cg_thirdPerson;
vmCvar_t	 cg_thirdPersonRange;
vmCvar_t	 cg_thirdPersonAngle;
vmCvar_t	 cg_stereoSeparation;
vmCvar_t	 cg_lagometer;
vmCvar_t	 cg_drawAttacker;
vmCvar_t	 cg_synchronousClients;
vmCvar_t	 cg_teamChatTime;
vmCvar_t	 cg_teamChatHeight;
vmCvar_t	 cg_stats;
vmCvar_t	 cg_buildScript;
vmCvar_t	 cg_forceModel;
vmCvar_t	 cg_paused;
vmCvar_t	 cg_blood;
vmCvar_t	 cg_predictItems;
vmCvar_t	 cg_deferPlayers;
vmCvar_t	 cg_drawTeamOverlay;
vmCvar_t	 cg_teamOverlayUserinfo;
vmCvar_t	 cg_drawFriend;
vmCvar_t	 cg_teamChatsOnly;

#ifdef MISSIONPACK
vmCvar_t cg_noVoiceChats;
vmCvar_t cg_noVoiceText;
#endif
vmCvar_t cg_hudFiles;
vmCvar_t cg_scorePlum;
vmCvar_t cg_smoothClients;
vmCvar_t pmove_fixed;

// vmCvar_t  cg_pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t cg_pmove_msec;
vmCvar_t cg_cameraMode;
vmCvar_t cg_cameraOrbit;
vmCvar_t cg_cameraOrbitDelay;
vmCvar_t cg_timescaleFadeEnd;
vmCvar_t cg_timescaleFadeSpeed;
vmCvar_t cg_timescale;
vmCvar_t cg_smallFont;
vmCvar_t cg_bigFont;
vmCvar_t cg_noTaunt;
vmCvar_t cg_noProjectileTrail;
vmCvar_t cg_oldRail;
vmCvar_t cg_oldRocket;
vmCvar_t cg_oldPlasma;
vmCvar_t cg_trueLightning;

vmCvar_t cg_particles;
vmCvar_t cg_particleCollision;

#ifdef MISSIONPACK
vmCvar_t cg_redTeamName;
vmCvar_t cg_blueTeamName;
vmCvar_t cg_currentSelectedPlayer;
vmCvar_t cg_currentSelectedPlayerName;
vmCvar_t cg_singlePlayer;
vmCvar_t cg_enableDust;
vmCvar_t cg_enableBreath;
vmCvar_t cg_singlePlayerActive;
vmCvar_t cg_recordSPDemo;
vmCvar_t cg_recordSPDemoName;
vmCvar_t cg_obeliskRespawnDelay;
#endif

vmCvar_t cg_drawPlayerCollision;

vmCvar_t cg_motionblur;
vmCvar_t cg_motionblurMinSpeed;

typedef struct
{
	vmCvar_t* vmCvar;
	char*	  cvarName;
	char*	  defaultString;
	int		  cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0 }, // used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE },
	{ &cg_shadows, "cg_shadows", "1", 0 },
	{ &cg_precomputedLighting, "r_precomputedLighting", "0", 0 },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_crosshairDot, "cg_crosshairDot", "0", CVAR_ARCHIVE },
	{ &cg_crosshairCircle, "cg_crosshairCircle", "0", CVAR_ARCHIVE },
	{ &cg_crosshairCross, "cg_crosshairCross", "3", CVAR_ARCHIVE },
	{ &cg_crosshairPulse, "cg_crosshairPulse", "1", CVAR_ARCHIVE }, // pulse crosshair when picking up items
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE },
	{ &cg_gunX, "cg_gunX", "0", CVAR_CHEAT },
	{ &cg_gunY, "cg_gunY", "0", CVAR_CHEAT },
	{ &cg_gunZ, "cg_gunZ", "0", CVAR_CHEAT },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup, "cg_bobup", "0.005", CVAR_CHEAT },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_animBlend, "cg_animblend", "5.0", CVAR_ARCHIVE },
	{ &cg_debugPlayerAnim, "cg_debugPlayerAnim", "0", CVAR_CHEAT },
	{ &cg_debugWeaponAnim, "cg_debugWeaponAnim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", 0 },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
#endif
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
#endif
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0 }, // force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO },
#ifdef MISSIONPACK
	{ &cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE },
	{ &cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE },
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO },
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO },
	{ &cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO },
	{ &cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO },
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE },
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE },
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO },
	{ &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE },
#endif
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT },
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE },
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0 },
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0 },
	{ &cg_timescale, "timescale", "1", 0 },
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT },

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO },
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE },
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE },
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE },
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE },
	{ &cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE },
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE },
	{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE },
	{ &cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE },
	{ &cg_particles, "cg_particles", "1", CVAR_ARCHIVE },
	{ &cg_particleCollision, "cg_particleCollision", "0", CVAR_ARCHIVE },
	{ &cg_drawPlayerCollision, "cg_drawPlayerCollision", "0", CVAR_CHEAT },
	{ &cg_motionblur, "cg_motionblur", "0.1", CVAR_ARCHIVE },
	{ &cg_motionblurMinSpeed, "cg_motionblurMinSpeed", "450", CVAR_ARCHIVE },

	//  {&cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE}
};

static int cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void	   CG_RegisterCvars()
{
	int			 i;
	cvarTable_t* cv;
	char		 var[MAX_TOKEN_CHARS];

	for( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register( NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "headmodel", DEFAULT_HEADMODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "team_model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "team_headmodel", DEFAULT_HEADMODEL, CVAR_USERINFO | CVAR_ARCHIVE );
}

/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange()
{
	int i;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		const char* clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if( !clientInfo[0] )
		{
			continue;
		}
		CG_NewClientInfo( i );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars()
{
	int			 i;
	cvarTable_t* cv;

	for( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
	{
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount )
	{
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if( cg_drawTeamOverlay.integer > 0 )
		{
			trap_Cvar_Set( "teamoverlay", "1" );
		}
		else
		{
			trap_Cvar_Set( "teamoverlay", "0" );
		}
	}

	// if force model changed
	if( forceModelModificationCount != cg_forceModel.modificationCount )
	{
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
}

int CG_CrosshairPlayer()
{
	if( cg.time > ( cg.crosshairClientTime + 1000 ) )
	{
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker()
{
	if( !cg.attackerTime )
	{
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf( const char* msg, ... )
{
	va_list argptr;
	char	text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL CG_Error( const char* msg, ... )
{
	va_list argptr;
	char	text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

#ifdef CG_LUA
	CG_ShutdownLua();
#endif

	trap_Error( text );
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error( int level, const char* error, ... )
{
	va_list argptr;
	char	text[1024];

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	trap_Error( text );
}

void QDECL Com_Printf( const char* msg, ... )
{
	va_list argptr;
	char	text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

#endif

/*
================
CG_Argv
================
*/
const char* CG_Argv( int arg )
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum )
{
	gitem_t* item;
	char	 data[MAX_QPATH];
	char *	 s, *start;
	int		 len;

	item = &bg_itemlist[itemNum];

	if( item->pickup_sound )
	{
		trap_S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if( !s || !s[0] )
	{
		return;
	}

	while( *s )
	{
		start = s;
		while( *s && *s != ' ' )
		{
			s++;
		}

		len = s - start;
		if( len >= MAX_QPATH || len < 5 )
		{
			CG_Error( "PrecacheItem: %s has bad precache string", item->classname );
			return;
		}
		memcpy( data, start, len );
		data[len] = 0;
		if( *s )
		{
			s++;
		}

		if( !strcmp( data + len - 3, "wav" ) )
		{
			trap_S_RegisterSound( data );
		}
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds()
{
	int			i;
	char		items[MAX_ITEMS + 1];
	char		name[MAX_QPATH];
	const char* soundName;

	// voice commands
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif

	cgs.media.oneMinuteSound	= trap_S_RegisterSound( "sound/feedback/1_minute.ogg" );
	cgs.media.fiveMinuteSound	= trap_S_RegisterSound( "sound/feedback/5_minute.ogg" );
	cgs.media.suddenDeathSound	= trap_S_RegisterSound( "sound/feedback/sudden_death.ogg" );
	cgs.media.oneFragSound		= trap_S_RegisterSound( "sound/feedback/1_frag.ogg" );
	cgs.media.twoFragSound		= trap_S_RegisterSound( "sound/feedback/2_frags.ogg" );
	cgs.media.threeFragSound	= trap_S_RegisterSound( "sound/feedback/3_frags.ogg" );
	cgs.media.count3Sound		= trap_S_RegisterSound( "sound/feedback/three.ogg" );
	cgs.media.count2Sound		= trap_S_RegisterSound( "sound/feedback/two.ogg" );
	cgs.media.count1Sound		= trap_S_RegisterSound( "sound/feedback/one.ogg" );
	cgs.media.countFightSound	= trap_S_RegisterSound( "sound/feedback/fight.ogg" );
	cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.ogg" );
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav" );
#endif

	if( cgs.gametype >= GT_TEAM || cg_buildScript.integer )
	{
		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav" );
		cgs.media.redLeadsSound		= trap_S_RegisterSound( "sound/feedback/redleads.ogg" );
		cgs.media.blueLeadsSound	= trap_S_RegisterSound( "sound/feedback/blueleads.ogg" );
		cgs.media.teamsTiedSound	= trap_S_RegisterSound( "sound/feedback/teamstied.ogg" );
		cgs.media.hitTeamSound		= trap_S_RegisterSound( "sound/feedback/hit_teammate.wav" );

		cgs.media.redScoredSound  = trap_S_RegisterSound( "sound/teamplay/voc_red_scores.ogg" );
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_scores.ogg" );

		cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav" );
		cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav" );

		cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav" );
		cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav" );

		cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav" );
		cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav" );

		if( cgs.gametype == GT_CTF || cg_buildScript.integer )
		{
			cgs.media.redFlagReturnedSound		 = trap_S_RegisterSound( "sound/teamplay/voc_red_returned.ogg" );
			cgs.media.blueFlagReturnedSound		 = trap_S_RegisterSound( "sound/teamplay/voc_blue_returned.ogg" );
			cgs.media.enemyTookYourFlagSound	 = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.ogg" );
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.ogg" );
		}

#ifdef MISSIONPACK
		if( cgs.gametype == GT_1FCTF || cg_buildScript.integer )
		{
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.ogg" );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.ogg" );
			cgs.media.enemyTookTheFlagSound	   = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.ogg" );
		}

		if( cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF || cg_buildScript.integer )
		{
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.ogg" );
			cgs.media.holyShitSound	   = trap_S_RegisterSound( "sound/feedback/holyshit.ogg" );
		}

		if( cgs.gametype == GT_OBELISK || cg_buildScript.integer )
		{
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_base_attack.ogg" );
		}
#else
		cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.ogg" );
		cgs.media.holyShitSound	   = trap_S_RegisterSound( "sound/feedback/holyshit.ogg" );
#endif
	}

	cgs.media.tracerSound	  = trap_S_RegisterSound( "sound/weapons/machinegun/par_shot_2.ogg" );
	cgs.media.selectSound	  = trap_S_RegisterSound( "sound/weapons/change.ogg" );
	cgs.media.wearOffSound	  = trap_S_RegisterSound( "sound/items/wearoff.ogg" );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav" );
	cgs.media.gibSound		  = trap_S_RegisterSound( "sound/player/gibelectro.ogg" );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.ogg" );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.ogg" );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.ogg" );

#ifdef MISSIONPACK
	cgs.media.useInvulnerabilitySound	  = trap_S_RegisterSound( "sound/items/invul_activate.wav" );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav" );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav" );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav" );
	cgs.media.invulnerabilityJuicedSound  = trap_S_RegisterSound( "sound/items/invul_juiced.wav" );
	cgs.media.obeliskHitSound1			  = trap_S_RegisterSound( "sound/items/obelisk_hit_01.wav" );
	cgs.media.obeliskHitSound2			  = trap_S_RegisterSound( "sound/items/obelisk_hit_02.wav" );
	cgs.media.obeliskHitSound3			  = trap_S_RegisterSound( "sound/items/obelisk_hit_03.wav" );
	cgs.media.obeliskRespawnSound		  = trap_S_RegisterSound( "sound/items/obelisk_respawn.wav" );

	cgs.media.ammoregenSound = trap_S_RegisterSound( "sound/items/cl_ammoregen.wav" );
	cgs.media.doublerSound	 = trap_S_RegisterSound( "sound/items/cl_doubler.wav" );
	cgs.media.guardSound	 = trap_S_RegisterSound( "sound/items/cl_guard.wav" );
	cgs.media.scoutSound	 = trap_S_RegisterSound( "sound/items/cl_scout.wav" );
#endif

	cgs.media.teleInSound  = trap_S_RegisterSound( "sound/player/telein.ogg" );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/player/teleout.ogg" );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn.ogg" );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.ogg" );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.ogg" );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav" );

	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav" );
#ifdef MISSIONPACK
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav" );
	cgs.media.hitSoundLowArmor	= trap_S_RegisterSound( "sound/feedback/hitlo.wav" );
#endif

	cgs.media.impressiveSound  = trap_S_RegisterSound( "sound/feedback/impressive.ogg" );
	cgs.media.excellentSound   = trap_S_RegisterSound( "sound/feedback/excellent.ogg" );
	cgs.media.deniedSound	   = trap_S_RegisterSound( "sound/feedback/denied.ogg" );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.ogg" );
	cgs.media.assistSound	   = trap_S_RegisterSound( "sound/feedback/assist.ogg" );
	cgs.media.defendSound	   = trap_S_RegisterSound( "sound/feedback/defense.ogg" );
#ifdef MISSIONPACK
	cgs.media.firstImpressiveSound	= trap_S_RegisterSound( "sound/feedback/first_impressive.wav" );
	cgs.media.firstExcellentSound	= trap_S_RegisterSound( "sound/feedback/first_excellent.wav" );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav" );
#endif

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.ogg" );
	cgs.media.tiedLeadSound	 = trap_S_RegisterSound( "sound/feedback/tiedlead.ogg" );
	cgs.media.lostLeadSound	 = trap_S_RegisterSound( "sound/feedback/lostlead.ogg" );

#ifdef MISSIONPACK
	cgs.media.voteNow	 = trap_S_RegisterSound( "sound/feedback/vote_now.ogg" );
	cgs.media.votePassed = trap_S_RegisterSound( "sound/feedback/vote_passed.ogg" );
	cgs.media.voteFailed = trap_S_RegisterSound( "sound/feedback/vote_failed.ogg" );
#endif

	cgs.media.watrInSound  = trap_S_RegisterSound( "sound/player/water_in.ogg" );
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/water_out.ogg" );
	cgs.media.watrUnSound  = trap_S_RegisterSound( "sound/player/water_under.ogg" );

	cgs.media.jumpPadSound = trap_S_RegisterSound( "sound/world/jumppad.wav" );

	for( i = 0; i < 4; i++ )
	{
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/stone%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_STONE][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/softrug%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/mech%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/energy%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound( name );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/metal%i.ogg", i + 1 );
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound( name );
	}

	// only register the items that the server says we need
	Q_strncpyz( items, CG_ConfigString( CS_ITEMS ), sizeof( items ) );

	for( i = 1; i < bg_numItems; i++ )
	{
		//      if ( items[ i ] == '1' || cg_buildScript.integer ) {
		CG_RegisterItemSounds( i );
		//      }
	}

	for( i = 1; i < MAX_SOUNDS; i++ )
	{
		soundName = CG_ConfigString( CS_SOUNDS + i );
		if( !soundName[0] )
		{
			break;
		}
		if( soundName[0] == '*' )
		{
			continue; // custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav" );
	cgs.media.medkitSound = trap_S_RegisterSound( "sound/items/use_medkit.wav" );
	cgs.media.quadSound	  = trap_S_RegisterSound( "sound/items/damage.ogg" );
	cgs.media.sfx_ric1	  = trap_S_RegisterSound( "sound/weapons/machinegun/ric1.wav" );
	cgs.media.sfx_ric2	  = trap_S_RegisterSound( "sound/weapons/machinegun/ric2.wav" );
	cgs.media.sfx_ric3	  = trap_S_RegisterSound( "sound/weapons/machinegun/ric3.wav" );
	// cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav");
	cgs.media.sfx_rockexp	= trap_S_RegisterSound( "sound/weapons/rocket/rocklx1a.ogg" );
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound( "sound/weapons/plasma/plasmx1a.wav" );
#ifdef MISSIONPACK
	cgs.media.sfx_proxexp		   = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav" );
	cgs.media.sfx_nghit			   = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav" );
	cgs.media.sfx_nghitflesh	   = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav" );
	cgs.media.sfx_nghitmetal	   = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav" );
	cgs.media.sfx_chghit		   = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav" );
	cgs.media.sfx_chghitflesh	   = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav" );
	cgs.media.sfx_chghitmetal	   = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav" );
	cgs.media.weaponHoverSound	   = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav" );
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav" );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav" );
	cgs.media.kamikazeFarSound	   = trap_S_RegisterSound( "sound/items/kam_explode_far.wav" );
	cgs.media.winnerSound		   = trap_S_RegisterSound( "sound/feedback/voc_youwin.ogg" );
	cgs.media.loserSound		   = trap_S_RegisterSound( "sound/feedback/voc_youlose.ogg" );

	cgs.media.wstbimplSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbimpl.wav" );
	cgs.media.wstbimpmSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbimpm.wav" );
	cgs.media.wstbimpdSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbimpd.wav" );
	cgs.media.wstbactvSound = trap_S_RegisterSound( "sound/weapons/proxmine/wstbactv.wav" );
#endif

	cgs.media.regenSound	= trap_S_RegisterSound( "sound/items/regen.wav" );
	cgs.media.protectSound	= trap_S_RegisterSound( "sound/items/protect3.wav" );
	cgs.media.n_healthSound = trap_S_RegisterSound( "sound/items/n_health.wav" );
	cgs.media.hgrenb1aSound = trap_S_RegisterSound( "sound/weapons/grenade/hgrenb1a.wav" );
	cgs.media.hgrenb2aSound = trap_S_RegisterSound( "sound/weapons/grenade/hgrenb2a.wav" );
}

//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics()
{
	int			 i;
	char		 items[MAX_ITEMS + 1];
	static char* sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	for( i = 0; i < 11; i++ )
	{
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );

	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.scoreboardName  = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing  = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime  = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );

	cgs.media.smokePuffShader		 = trap_R_RegisterShader( "smokePuff" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
#ifdef MISSIONPACK
	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );
	cgs.media.blueProxMine	 = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
#endif
	cgs.media.plasmaBallShader	= trap_R_RegisterShader( "sprites/plasma1" );
	cgs.media.bloodTrailShader	= trap_R_RegisterShader( "particles/blood_trail" );
	cgs.media.bloodSpurtShader	= trap_R_RegisterShader( "particles/blood_spurt" );
	cgs.media.bloodSpurt2Shader = trap_R_RegisterShader( "particles/blood_spurt" );
	cgs.media.bloodSpurt3Shader = trap_R_RegisterShader( "particles/blood_spurt" );
	cgs.media.lagometerShader	= trap_R_RegisterShader( "lagometer" );
	cgs.media.connectionShader	= trap_R_RegisterShader( "disconnected" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for( i = 0; i < NUM_CROSSHAIRS; i++ )
	{
		cgs.media.crosshairDot[i]	 = trap_R_RegisterShader( va( "hud/crosshairs/dot%i", i + 1 ) );
		cgs.media.crosshairCircle[i] = trap_R_RegisterShader( va( "hud/crosshairs/circle%i", i + 1 ) );
		cgs.media.crosshairCross[i]	 = trap_R_RegisterShader( va( "hud/crosshairs/cross%i", i + 1 ) );
	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader	 = trap_R_RegisterShader( "icons/noammo" );

	cgs.media.sparkShader = trap_R_RegisterShader( "particles/glow" );

	// globe mapping shaders
	cgs.media.shadowProjectedLightShader = trap_R_RegisterShaderLightAttenuation( "lights/shadowProjectedLight" );

	// powerup shaders
	cgs.media.quadShader		 = trap_R_RegisterShader( "powerups/quad" );
	cgs.media.quadWeaponShader	 = trap_R_RegisterShader( "powerups/quadWeapon" );
	cgs.media.battleSuitShader	 = trap_R_RegisterShader( "powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader( "powerups/battleWeapon" );
	cgs.media.invisShader		 = trap_R_RegisterShader( "powerups/invisibility" );
	cgs.media.regenShader		 = trap_R_RegisterShader( "powerups/regen" );
	cgs.media.hastePuffShader	 = trap_R_RegisterShader( "hasteSmokePuff" );

	// effects
	cgs.media.unlinkEffect = trap_R_RegisterShader( "player/unlink_effect" );

#ifdef MISSIONPACK
	if( cgs.gametype == GT_HARVESTER || cg_buildScript.integer )
	{
		cgs.media.redCubeModel	= trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
		cgs.media.redCubeIcon	= trap_R_RegisterShader( "icons/skull_red" );
		cgs.media.blueCubeIcon	= trap_R_RegisterShader( "icons/skull_blue" );
	}

	if( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer )
	{
#else
	if( cgs.gametype == GT_CTF || cg_buildScript.integer )
	{
#endif
		cgs.media.flagModel = trap_R_RegisterModel( "models/flags/flag.md5mesh" );
		CG_RegisterAnimation( &cgs.media.flagAnimations[FLAG_IDLE], "models/flags/flag_idle.md5anim", qtrue, qfalse, qtrue );
		CG_RegisterAnimation( &cgs.media.flagAnimations[FLAG_RUN], "models/flags/flag_run.md5anim", qtrue, qfalse, qtrue );

		cgs.media.redFlagSkin  = trap_R_RegisterSkin( "models/flags/flag_red.skin" );
		cgs.media.blueFlagSkin = trap_R_RegisterSkin( "models/flags/flag_blue.skin" );

		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );

		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );

#ifdef MISSIONPACK
		cgs.media.redFlagBaseModel	   = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel	   = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
#endif
	}

#ifdef MISSIONPACK
	if( cgs.gametype == GT_1FCTF || cg_buildScript.integer )
	{
		cgs.media.neutralFlagSkin = trap_R_RegisterSkin( "models/flags/flag_neutral.skin" );

		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}

	if( cgs.gametype == GT_OBELISK || cg_buildScript.integer )
	{
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		cgs.media.overloadBaseModel		= trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel	= trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel	= trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel	= trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}

	if( cgs.gametype == GT_HARVESTER || cg_buildScript.integer )
	{
		cgs.media.harvesterModel		= trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin		= trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin		= trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel( "models/powerups/obelisk/obelisk.md3" );
	}

	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
	cgs.media.dustPuffShader	= trap_R_RegisterShader( "hasteSmokePuff" );
#endif

	if( cgs.gametype >= GT_TEAM || cg_buildScript.integer )
	{
		cgs.media.friendShader	= trap_R_RegisterShader( "sprites/friend" );
		cgs.media.redQuadShader = trap_R_RegisterShader( "powerups/blueflag" );
		cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
#ifdef MISSIONPACK
		cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
#endif
	}

	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon	 = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );

	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel	   = trap_R_RegisterModel( "models/weapons/shells/s_shell.md3" );

	cgs.media.gibAbdomen   = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm	   = trap_R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest	   = trap_R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist	   = trap_R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot	   = trap_R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm   = trap_R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg	   = trap_R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull	   = trap_R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain	   = trap_R_RegisterModel( "models/gibs/brain.md3" );

	cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons/shells/s_shell.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.dishFlashModel	  = trap_R_RegisterModel( "models/weaphits/boom01.md3" );
	cgs.media.teleportFlareShader = trap_R_RegisterShader( "particles/flare2" );

#ifdef MISSIONPACK
	cgs.media.kamikazeEffectModel		  = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave			  = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
	cgs.media.kamikazeHeadModel			  = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
	cgs.media.kamikazeHeadTrail			  = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel			  = trap_R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel			  = trap_R_RegisterModel( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel		  = trap_R_RegisterModel( "models/powerups/doubler_player.md3" );
	cgs.media.ammoRegenPowerupModel		  = trap_R_RegisterModel( "models/powerups/ammo_player.md3" );
	cgs.media.invulnerabilityImpactModel  = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel  = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
	cgs.media.medkitUsageModel			  = trap_R_RegisterModel( "models/powerups/regen.md3" );
	cgs.media.heartShader				  = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );
	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
#endif

	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent  = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet	  = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend	  = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist	  = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture	  = trap_R_RegisterShaderNoMip( "medal_capture" );

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz( items, CG_ConfigString( CS_ITEMS ), sizeof( items ) );

	for( i = 1; i < bg_numItems; i++ )
	{
		if( items[i] == '1' || cg_buildScript.integer )
		{
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader   = trap_R_RegisterShader( "burnMark" );
	cgs.media.holeMarkShader   = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader   = trap_R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader  = trap_R_RegisterShader( "textures/decals/blood_splat04" );
	cgs.media.bloodMark2Shader = trap_R_RegisterShader( "textures/decals/blood_splat05" );
	cgs.media.bloodMark3Shader = trap_R_RegisterShader( "textures/decals/blood_splat06" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for( i = 1; i < cgs.numInlineModels; i++ )
	{
		char   name[10];
		vec3_t mins, maxs;
		int	   j;

		Com_sprintf( name, sizeof( name ), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for( j = 0; j < 3; j++ )
		{
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for( i = 1; i < MAX_MODELS; i++ )
	{
		const char* modelName;

		modelName = CG_ConfigString( CS_MODELS + i );
		if( !modelName[0] )
		{
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

	// debug utils
	cgs.media.debugPlayerAABB		   = trap_R_RegisterShader( "debugPlayerAABB" );
	cgs.media.debugPlayerAABB_twoSided = trap_R_RegisterShader( "debugPlayerAABB_twoSided" );

#ifdef MISSIONPACK
	// new stuff
	cgs.media.patrolShader	   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/patrol.tga" );
	cgs.media.assaultShader	   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/assault.tga" );
	cgs.media.campShader	   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/camp.tga" );
	cgs.media.followShader	   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/follow.tga" );
	cgs.media.defendShader	   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/defend.tga" );
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/team_leader.tga" );
	cgs.media.retrieveShader   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/retrieve.tga" );
	cgs.media.escortShader	   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/escort.tga" );
	cgs.media.cursor		   = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor	   = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor	   = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0]   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/flag_in_base.tga" );
	cgs.media.flagShaders[1]   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/flag_capture.tga" );
	cgs.media.flagShaders[2]   = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/flag_missing.tga" );

	trap_R_RegisterModel( "models/players/james/lower.md3" );
	trap_R_RegisterModel( "models/players/james/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/james/james.md3" );

	trap_R_RegisterModel( "models/players/janet/lower.md3" );
	trap_R_RegisterModel( "models/players/janet/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/janet/janet.md3" );
#endif

	CG_InitParticles();
}

/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString()
{
	int i;

	cg.spectatorList[0] = 0;
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR )
		{
			Q_strcat( cg.spectatorList, sizeof( cg.spectatorList ), va( "%s     ", cgs.clientinfo[i].name ) );
		}
	}
	i = strlen( cg.spectatorList );
	if( i != cg.spectatorLen )
	{
		cg.spectatorLen	  = i;
		cg.spectatorWidth = -1;
	}
}

/*
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients()
{
	int i;

	CG_LoadingClient( cg.clientNum );
	CG_NewClientInfo( cg.clientNum );

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		const char* clientInfo;

		if( cg.clientNum == i )
		{
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if( !clientInfo[0] )
		{
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char* CG_ConfigString( int index )
{
	if( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic()
{
	char* s;
	char  parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = ( char* )CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, Com_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, Com_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

#ifdef MISSIONPACK
char* CG_GetMenuBuffer( const char* filename )
{
	int			 len;
	fileHandle_t f;
	static char	 buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if( !f )
	{
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if( len >= MAX_MENUFILE )
	{
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse( int handle )
{
	pc_token_t	token;
	const char* tempStr;

	if( !trap_PC_ReadToken( handle, &token ) )
	{
		return qfalse;
	}
	if( Q_stricmp( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while( 1 )
	{
		if( !trap_PC_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if( Q_stricmp( token.string, "}" ) == 0 )
		{
			return qtrue;
		}

		// font
		if( Q_stricmp( token.string, "font" ) == 0 )
		{
			int pointSize;

			if( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.textFont );
			continue;
		}

		// smallFont
		if( Q_stricmp( token.string, "smallFont" ) == 0 )
		{
			int pointSize;

			if( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.smallFont );
			continue;
		}

		// font
		if( Q_stricmp( token.string, "bigfont" ) == 0 )
		{
			int pointSize;

			if( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
			{
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.bigFont );
			continue;
		}

		// gradientbar
		if( Q_stricmp( token.string, "gradientbar" ) == 0 )
		{
			if( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
			continue;
		}

		// enterMenuSound
		if( Q_stricmp( token.string, "menuEnterSound" ) == 0 )
		{
			if( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// exitMenuSound
		if( Q_stricmp( token.string, "menuExitSound" ) == 0 )
		{
			if( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// itemFocusSound
		if( Q_stricmp( token.string, "itemFocusSound" ) == 0 )
		{
			if( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// menuBuzzSound
		if( Q_stricmp( token.string, "menuBuzzSound" ) == 0 )
		{
			if( !PC_String_Parse( handle, &tempStr ) )
			{
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		if( Q_stricmp( token.string, "cursor" ) == 0 )
		{
			if( !PC_String_Parse( handle, &cgDC.Assets.cursorStr ) )
			{
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr );
			continue;
		}

		if( Q_stricmp( token.string, "fadeClamp" ) == 0 )
		{
			if( !PC_Float_Parse( handle, &cgDC.Assets.fadeClamp ) )
			{
				return qfalse;
			}
			continue;
		}

		if( Q_stricmp( token.string, "fadeCycle" ) == 0 )
		{
			if( !PC_Int_Parse( handle, &cgDC.Assets.fadeCycle ) )
			{
				return qfalse;
			}
			continue;
		}

		if( Q_stricmp( token.string, "fadeAmount" ) == 0 )
		{
			if( !PC_Float_Parse( handle, &cgDC.Assets.fadeAmount ) )
			{
				return qfalse;
			}
			continue;
		}

		if( Q_stricmp( token.string, "shadowX" ) == 0 )
		{
			if( !PC_Float_Parse( handle, &cgDC.Assets.shadowX ) )
			{
				return qfalse;
			}
			continue;
		}

		if( Q_stricmp( token.string, "shadowY" ) == 0 )
		{
			if( !PC_Float_Parse( handle, &cgDC.Assets.shadowY ) )
			{
				return qfalse;
			}
			continue;
		}

		if( Q_stricmp( token.string, "shadowColor" ) == 0 )
		{
			if( !PC_Color_Parse( handle, &cgDC.Assets.shadowColor ) )
			{
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse;
}

void CG_ParseMenu( const char* menuFile )
{
	pc_token_t token;
	int		   handle;

	handle = trap_PC_LoadSource( menuFile );
	if( !handle )
	{
		handle = trap_PC_LoadSource( "ui/testhud.menu" );
	}
	if( !handle )
	{
		return;
	}

	while( 1 )
	{
		if( !trap_PC_ReadToken( handle, &token ) )
		{
			break;
		}

		// if ( Q_stricmp( token, "{" ) ) {
		//   Com_Printf( "Missing { in menu file\n" );
		//   break;
		// }

		// if ( menuCount == MAX_MENUS ) {
		//   Com_Printf( "Too many menus!\n" );
		//   break;
		// }

		if( token.string[0] == '}' )
		{
			break;
		}

		if( Q_stricmp( token.string, "assetGlobalDef" ) == 0 )
		{
			if( CG_Asset_Parse( handle ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if( Q_stricmp( token.string, "menudef" ) == 0 )
		{
			// start a new menu
			Menu_New( handle );
		}
	}
	trap_PC_FreeSource( handle );
}

qboolean CG_Load_Menu( char** p )
{
	char* token;

	token = Com_ParseExt( p, qtrue );

	if( token[0] != '{' )
	{
		return qfalse;
	}

	while( 1 )
	{
		token = Com_ParseExt( p, qtrue );

		if( Q_stricmp( token, "}" ) == 0 )
		{
			return qtrue;
		}

		if( !token || token[0] == 0 )
		{
			return qfalse;
		}

		CG_ParseMenu( token );
	}
	return qfalse;
}

void CG_LoadMenus( const char* menuFile )
{
	char*		 token;
	char*		 p;
	int			 len, start;
	fileHandle_t f;
	static char	 buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
	if( !f )
	{
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if( !f )
		{
			CG_Error( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!" );
		}
	}

	if( len >= MAX_MENUDEFFILE )
	{
		trap_FS_FCloseFile( f );
		CG_Error( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	Com_Compress( buf );

	Menu_Reset();

	p = buf;

	while( 1 )
	{
		token = Com_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}' )
		{
			break;
		}

		// if ( Q_stricmp( token, "{" ) ) {
		//   Com_Printf( "Missing { in menu file\n" );
		//   break;
		// }

		// if ( menuCount == MAX_MENUS ) {
		//   Com_Printf( "Too many menus!\n" );
		//   break;
		// }

		if( Q_stricmp( token, "}" ) == 0 )
		{
			break;
		}

		if( Q_stricmp( token, "loadmenu" ) == 0 )
		{
			if( CG_Load_Menu( &p ) )
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Com_Printf( "UI menu load time = %d milli seconds\n", trap_Milliseconds() - start );
}

static qboolean CG_OwnerDrawHandleKey( int ownerDraw, int flags, float* special, int key )
{
	return qfalse;
}

static int CG_FeederCount( float feederID )
{
	int i, count;

	count = 0;
	if( feederID == FEEDER_REDTEAM_LIST )
	{
		for( i = 0; i < cg.numScores; i++ )
		{
			if( cg.scores[i].team == TEAM_RED )
			{
				count++;
			}
		}
	}
	else if( feederID == FEEDER_BLUETEAM_LIST )
	{
		for( i = 0; i < cg.numScores; i++ )
		{
			if( cg.scores[i].team == TEAM_BLUE )
			{
				count++;
			}
		}
	}
	else if( feederID == FEEDER_SCOREBOARD )
	{
		return cg.numScores;
	}
	return count;
}

void CG_SetScoreSelection( void* p )
{
	menuDef_t*	   menu = ( menuDef_t* )p;
	playerState_t* ps	= &cg.snap->ps;
	int			   i, red, blue;

	red = blue = 0;
	for( i = 0; i < cg.numScores; i++ )
	{
		if( cg.scores[i].team == TEAM_RED )
		{
			red++;
		}
		else if( cg.scores[i].team == TEAM_BLUE )
		{
			blue++;
		}
		if( ps->clientNum == cg.scores[i].client )
		{
			cg.selectedScore = i;
		}
	}

	if( menu == NULL )
	{
		// just interested in setting the selected score
		return;
	}

	if( cgs.gametype >= GT_TEAM )
	{
		int feeder = FEEDER_REDTEAM_LIST;

		i = red;
		if( cg.scores[cg.selectedScore].team == TEAM_BLUE )
		{
			feeder = FEEDER_BLUETEAM_LIST;
			i	   = blue;
		}
		Menu_SetFeederSelection( menu, feeder, i, NULL );
	}
	else
	{
		Menu_SetFeederSelection( menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL );
	}
}

// FIXME: might need to cache this info
static clientInfo_t* CG_InfoFromScoreIndex( int index, int team, int* scoreIndex )
{
	int i, count;

	if( cgs.gametype >= GT_TEAM )
	{
		count = 0;
		for( i = 0; i < cg.numScores; i++ )
		{
			if( cg.scores[i].team == team )
			{
				if( count == index )
				{
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[cg.scores[index].client];
}

static const char* CG_FeederItemText( float feederID, int index, int column, qhandle_t* handle )
{
	gitem_t*	  item;
	int			  scoreIndex = 0;
	clientInfo_t* info		 = NULL;
	int			  team		 = -1;
	score_t*	  sp		 = NULL;

	*handle = -1;

	if( feederID == FEEDER_REDTEAM_LIST )
	{
		team = TEAM_RED;
	}
	else if( feederID == FEEDER_BLUETEAM_LIST )
	{
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex( index, team, &scoreIndex );
	sp	 = &cg.scores[scoreIndex];

	if( info && info->infoValid )
	{
		switch( column )
		{
			case 0:
				if( info->powerups & ( 1 << PW_NEUTRALFLAG ) )
				{
					item	= BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle = cg_items[ITEM_INDEX( item )].icon;
				}
				else if( info->powerups & ( 1 << PW_REDFLAG ) )
				{
					item	= BG_FindItemForPowerup( PW_REDFLAG );
					*handle = cg_items[ITEM_INDEX( item )].icon;
				}
				else if( info->powerups & ( 1 << PW_BLUEFLAG ) )
				{
					item	= BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle = cg_items[ITEM_INDEX( item )].icon;
				}
				else
				{
					if( info->botSkill > 0 && info->botSkill <= 5 )
					{
						*handle = cgs.media.botSkillShaders[info->botSkill - 1];
					}
					else if( info->handicap < 100 )
					{
						return va( "%i", info->handicap );
					}
				}
				break;
			case 1:
				if( team == -1 )
				{
					return "";
				}
				else
				{
					*handle = CG_StatusHandle( info->teamTask );
				}
				break;
			case 2:
				if( cg.snap->ps.stats[STAT_CLIENTS_READY] & ( 1 << sp->client ) )
				{
					return "Ready";
				}
				if( team == -1 )
				{
					if( cgs.gametype == GT_TOURNAMENT )
					{
						return va( "%i/%i", info->wins, info->losses );
					}
					else if( info->infoValid && info->team == TEAM_SPECTATOR )
					{
						return "Spectator";
					}
					else
					{
						return "";
					}
				}
				else
				{
					if( info->teamLeader )
					{
						return "Leader";
					}
				}
				break;
			case 3:
				return info->name;
				break;
			case 4:
				return va( "%i", info->score );
				break;
			case 5:
				return va( "%4i", sp->time );
				break;
			case 6:
				if( sp->ping == -1 )
				{
					return "connecting";
				}
				return va( "%4i", sp->ping );
				break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage( float feederID, int index )
{
	return 0;
}

static void CG_FeederSelection( float feederID, int index )
{
	if( cgs.gametype >= GT_TEAM )
	{
		int i, count;
		int team = ( feederID == FEEDER_REDTEAM_LIST ) ? TEAM_RED : TEAM_BLUE;

		count = 0;
		for( i = 0; i < cg.numScores; i++ )
		{
			if( cg.scores[i].team == team )
			{
				if( index == count )
				{
					cg.selectedScore = i;
				}
				count++;
			}
		}
	}
	else
	{
		cg.selectedScore = index;
	}
}
#endif

#ifdef MISSIONPACK
static float CG_Cvar_Get( const char* cvar )
{
	char buff[128];

	memset( buff, 0, sizeof( buff ) );
	trap_Cvar_VariableStringBuffer( cvar, buff, sizeof( buff ) );
	return atof( buff );
}
#endif

#ifdef MISSIONPACK
void CG_Text_PaintWithCursor( float x, float y, float scale, vec4_t color, const char* text, int cursorPos, char cursor, int limit, int style )
{
	CG_Text_Paint( x, y, scale, color, text, 0, limit, style );
}

static int CG_OwnerDrawWidth( int ownerDraw, float scale )
{
	switch( ownerDraw )
	{
		case CG_GAME_TYPE:
			return CG_Text_Width( CG_GameTypeString(), scale, 0 );
		case CG_GAME_STATUS:
			return CG_Text_Width( CG_GetGameStatusText(), scale, 0 );
			break;
		case CG_KILLER:
			return CG_Text_Width( CG_GetKillerText(), scale, 0 );
			break;
		case CG_RED_NAME:
			return CG_Text_Width( cg_redTeamName.string, scale, 0 );
			break;
		case CG_BLUE_NAME:
			return CG_Text_Width( cg_blueTeamName.string, scale, 0 );
			break;
	}
	return 0;
}

static int CG_PlayCinematic( const char* name, float x, float y, float w, float h )
{
	return trap_CIN_PlayCinematic( name, x, y, w, h, CIN_loop );
}

static void CG_StopCinematic( int handle )
{
	trap_CIN_StopCinematic( handle );
}

static void CG_DrawCinematic( int handle, float x, float y, float w, float h )
{
	trap_CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void CG_RunCinematicFrame( int handle )
{
	trap_CIN_RunCinematic( handle );
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu()
{
	char		buff[1024];
	const char* hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor			 = &trap_R_SetColor;
	cgDC.drawHandlePic		 = &CG_DrawPic;
	cgDC.drawStretchPic		 = &trap_R_DrawStretchPic;
	cgDC.drawText			 = &CG_Text_Paint;
	cgDC.textWidth			 = &CG_Text_Width;
	cgDC.textHeight			 = &CG_Text_Height;
	cgDC.registerModel		 = &trap_R_RegisterModel;
	cgDC.modelBounds		 = &trap_R_ModelBounds;
	cgDC.fillRect			 = &CG_FillRect;
	cgDC.drawRect			 = &CG_DrawRect;
	cgDC.drawSides			 = &CG_DrawSides;
	cgDC.drawTopBottom		 = &CG_DrawTopBottom;
	cgDC.clearScene			 = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene		 = &trap_R_RenderScene;
	cgDC.registerFont		 = &trap_R_RegisterFont;
	cgDC.ownerDrawItem		 = &CG_OwnerDraw;
	cgDC.getValue			 = &CG_GetValue;
	cgDC.ownerDrawVisible	 = &CG_OwnerDrawVisible;
	cgDC.runScript			 = &CG_RunMenuScript;
	cgDC.getTeamColor		 = &CG_GetTeamColor;
	cgDC.setCVar			 = trap_Cvar_Set;
	cgDC.getCVarString		 = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue		 = CG_Cvar_Get;
	cgDC.drawTextWithCursor	 = &CG_Text_PaintWithCursor;
	// cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	// cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound	= &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount		= &CG_FeederCount;
	cgDC.feederItemImage	= &CG_FeederItemImage;
	cgDC.feederItemText		= &CG_FeederItemText;
	cgDC.feederSelection	= &CG_FeederSelection;
	// cgDC.setBinding = &trap_Key_SetBinding;
	// cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	// cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	// cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error			= &Com_Error;
	cgDC.Print			= &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	// cgDC.Pause = &CG_Pause;
	cgDC.registerSound		  = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack  = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic		  = &CG_PlayCinematic;
	cgDC.stopCinematic		  = &CG_StopCinematic;
	cgDC.drawCinematic		  = &CG_DrawCinematic;
	cgDC.runCinematicFrame	  = &CG_RunCinematicFrame;

	Init_Display( &cgDC );

	Menu_Reset();

	trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
	hudSet = buff;
	if( hudSet[0] == '\0' )
	{
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus( hudSet );
}

void CG_AssetCache()
{
	// if (Assets.textFont == NULL) {
	//   trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	// }
	// Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	// Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar			= trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic			= trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0]			= trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1]			= trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2]			= trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3]			= trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4]			= trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5]			= trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6]			= trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar			= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown	= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp	= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft	= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb		= trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar			= trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb			= trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}
#endif
/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
{
	const char* s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof( cg_entities ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_items, 0, sizeof( cg_items ) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum  = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader	  = trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.whiteShader	  = trap_R_RegisterShader( "white" );
	cgs.media.charsetProp	  = trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow = trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB	  = trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

#ifdef CG_LUA
	CG_InitLua();
#endif

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus			   = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if( strcmp( s, GAME_VERSION ) )
	{
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s				   = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

#ifdef MISSIONPACK
	String_Init();
#endif

	cg.loading = qtrue; // force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients(); // if low on memory, some clients will be deferred

#ifdef MISSIONPACK
	CG_AssetCache();
	CG_LoadHudMenu(); // load new hud stuff
#endif

	cg.loading = qfalse; // future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown()
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

	CG_Printf( "------- CGame Shutdown -------\n" );

#ifdef CG_LUA
	CG_ShutdownLua();
#endif
}

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
	  1 - team menu
	  2 - hud editor

*/
#ifndef MISSIONPACK
void CG_EventHandling( int type )
{
}

void CG_KeyEvent( int key, qboolean down )
{
}

void CG_MouseEvent( int x, int y )
{
}
#endif
