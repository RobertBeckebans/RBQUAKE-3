/*
===========================================================================
Copyright (C) 1998 Steve Yeager
Copyright (C) 1999-2005 Id Software, Inc.
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
//  acebot_spawn.c - This file contains all of the
//                   spawing support routines for the ACE bot.


#include "../g_local.h"
#include "acebot.h"

#if defined(ACEBOT)

static int      g_numBots;
static char*    g_botInfos[MAX_BOTS];


int             g_numArenas;
static char*    g_arenaInfos[MAX_ARENAS];


extern gentity_t* podium1;
extern gentity_t* podium2;
extern gentity_t* podium3;

static int ACESP_ParseInfos( char* buf, int max, char* infos[] )
{
	char*           token;
	int             count;
	char            key[MAX_TOKEN_CHARS];
	char            info[MAX_INFO_STRING];
	
	count = 0;
	
	while( 1 )
	{
		token = Com_Parse( &buf );
		if( !token[0] )
		{
			break;
		}
		if( strcmp( token, "{" ) )
		{
			Com_Printf( "Missing { in info file\n" );
			break;
		}
		
		if( count == max )
		{
			Com_Printf( "Max infos exceeded\n" );
			break;
		}
		
		info[0] = '\0';
		while( 1 )
		{
			token = Com_ParseExt( &buf, qtrue );
			if( !token[0] )
			{
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if( !strcmp( token, "}" ) )
			{
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );
			
			token = Com_ParseExt( &buf, qfalse );
			if( !token[0] )
			{
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = G_Alloc( strlen( info ) + strlen( "\\num\\" ) + strlen( va( "%d", MAX_ARENAS ) ) + 1 );
		if( infos[count] )
		{
			strcpy( infos[count], info );
			count++;
		}
	}
	return count;
}

static void ACESP_LoadArenasFromFile( char* filename )
{
	int             len;
	fileHandle_t    f;
	char            buf[MAX_ARENAS_TEXT];
	
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if( !f )
	{
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if( len >= MAX_ARENAS_TEXT )
	{
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}
	
	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	g_numArenas += ACESP_ParseInfos( buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas] );
}

static void ACESP_LoadArenas( void )
{
	int             numdirs;
	vmCvar_t        arenasFile;
	char            filename[128];
	char            dirlist[1024];
	char*           dirptr;
	int             i, n;
	int             dirlen;
	
	g_numArenas = 0;
	
	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM );
	if( *arenasFile.string )
	{
		ACESP_LoadArenasFromFile( arenasFile.string );
	}
	else
	{
		ACESP_LoadArenasFromFile( "scripts/arenas.txt" );
	}
	
	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, 1024 );
	dirptr = dirlist;
	for( i = 0; i < numdirs; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		ACESP_LoadArenasFromFile( filename );
	}
	trap_Printf( va( "%i arenas parsed\n", g_numArenas ) );
	
	for( n = 0; n < g_numArenas; n++ )
	{
		Info_SetValueForKey( g_arenaInfos[n], "num", va( "%i", n ) );
	}
}


static const char* ACESP_GetArenaInfoByMap( const char* map )
{
	int             n;
	
	for( n = 0; n < g_numArenas; n++ )
	{
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 )
		{
			return g_arenaInfos[n];
		}
	}
	
	return NULL;
}

static void ACESP_SpawnBots( char* botList )
{
	char*           bot;
	char*           p;
	float           skill;
	char            bots[MAX_INFO_VALUE];
	
	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;
	
	skill = ace_spSkill.integer;
	if( skill < 1 )
	{
		trap_Cvar_Set( "g_spSkill", "1" );
		skill = 1;
	}
	else if( skill > 5 )
	{
		trap_Cvar_Set( "g_spSkill", "5" );
		skill = 5;
	}
	
	Q_strncpyz( bots, botList, sizeof( bots ) );
	p = &bots[0];
	while( *p )
	{
		//skip spaces
		while( *p && *p == ' ' )
		{
			p++;
		}
		if( !p )
		{
			break;
		}
		
		// mark start of bot name
		bot = p;
		
		// skip until space of null
		while( *p && *p != ' ' )
		{
			p++;
		}
		if( *p )
		{
			*p++ = 0;
		}
		
		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap_SendConsoleCommand( EXEC_INSERT, va( "addbot %s %f free\n", bot, skill ) );
	}
}

static void ACESP_LoadBotsFromFile( char* filename )
{
	int             len;
	fileHandle_t    f;
	char            buf[MAX_BOTS_TEXT];
	
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if( !f )
	{
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if( len >= MAX_BOTS_TEXT )
	{
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}
	
	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	g_numBots += ACESP_ParseInfos( buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots] );
}

static void ACESP_LoadBots( void )
{
	int             numdirs;
	char            filename[128];
	char            dirlist[1024];
	char*           dirptr;
	int             i;
	int             dirlen;
	
	g_numBots = 0;
	
	if( *ace_botsFile.string )
	{
		ACESP_LoadBotsFromFile( ace_botsFile.string );
	}
	else
	{
		ACESP_LoadBotsFromFile( "scripts/bots.txt" );
	}
	
	// get all bots from .bot files
	numdirs = trap_FS_GetFileList( "scripts", ".bot", dirlist, 1024 );
	dirptr = dirlist;
	for( i = 0; i < numdirs; i++, dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		ACESP_LoadBotsFromFile( filename );
	}
	
	trap_Printf( va( "%i bots parsed\n", g_numBots ) );
}



/*static char    *ACESP_GetBotInfoByNumber(int num)
{
	if(num < 0 || num >= g_numBots)
	{
		trap_Printf(va(S_COLOR_RED "Invalid bot number: %i\n", num));
		return NULL;
	}
	return g_botInfos[num];
}*/


static char*    ACESP_GetBotInfoByName( const char* name )
{
	int             n;
	char*           value;
	
	for( n = 0; n < g_numBots; n++ )
	{
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if( !Q_stricmp( value, name ) )
		{
			return g_botInfos[n];
		}
	}
	
	return NULL;
}

void ACESP_InitBots( qboolean restart )
{
	int             fragLimit;
	int             timeLimit;
	const char*     arenainfo;
	char*           strValue;
	char            map[MAX_QPATH];
	char            serverinfo[MAX_INFO_STRING];
	
	ACESP_LoadBots();
	ACESP_LoadArenas();
	
	if( g_gametype.integer == GT_SINGLE_PLAYER )
	{
		trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );
		Q_strncpyz( map, Info_ValueForKey( serverinfo, "mapname" ), sizeof( map ) );
		arenainfo = ACESP_GetArenaInfoByMap( map );
		if( !arenainfo )
		{
			return;
		}
		
		strValue = Info_ValueForKey( arenainfo, "fraglimit" );
		fragLimit = atoi( strValue );
		if( fragLimit )
		{
			trap_Cvar_Set( "fraglimit", strValue );
		}
		else
		{
			trap_Cvar_Set( "fraglimit", "0" );
		}
		
		strValue = Info_ValueForKey( arenainfo, "timelimit" );
		timeLimit = atoi( strValue );
		if( timeLimit )
		{
			trap_Cvar_Set( "timelimit", strValue );
		}
		else
		{
			trap_Cvar_Set( "timelimit", "0" );
		}
		
		if( !fragLimit && !timeLimit )
		{
			trap_Cvar_Set( "fraglimit", "10" );
			trap_Cvar_Set( "timelimit", "0" );
		}
		
		if( !restart )
		{
			ACESP_SpawnBots( Info_ValueForKey( arenainfo, "bots" ) );
		}
	}
}

void ACESP_SpawnBot( char* name, float skill, char* team )
{
	int             clientNum;
	char*           botinfo;
	
//  gentity_t      *bot;
	char*           key;
	char*           s;
	char*           botname;
	char*           model;
	char            userinfo[MAX_INFO_STRING];
	
	G_Printf( "ACESP_SpawnBot(%s, %f, %s)\n", name, skill, team );
	
	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if( clientNum == -1 )
	{
		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
		G_Printf( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}
	
	// get the botinfo from bots.txt
	botinfo = ACESP_GetBotInfoByName( name );
	if( !botinfo )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: Bot '%s' not defined, using default values\n", name );
	}
	
	// create the bot's userinfo
	userinfo[0] = '\0';
	
	botname = Info_ValueForKey( botinfo, "name" );
	if( !botname[0] )
	{
		if( !name || !name[0] )
		{
			botname = va( "ACEBot%d", clientNum );
		}
		else
		{
			botname = name;
		}
	}
	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va( "%1.2f", skill ) );
	
	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if( !*model )
	{
		model = DEFAULT_MODEL;
	}
	Info_SetValueForKey( userinfo, key, model );
	
	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if( !*s )
	{
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );
	
	key = "color1";
	s = Info_ValueForKey( botinfo, key );
	if( !*s )
	{
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );
	
	key = "color2";
	s = Info_ValueForKey( botinfo, key );
	if( !*s )
	{
		s = "5";
	}
	Info_SetValueForKey( userinfo, key, s );
	
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
	Info_SetValueForKey( userinfo, "team", team );
	
	//Info_SetValueForKey(userinfo, "characterfile", Info_ValueForKey(botinfo, "aifile"));
	//Info_SetValueForKey(userinfo, "skill", va("%5.2f", skill));
	
	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );
	
	// have it connect to the game as a normal client
	if( ClientConnect( clientNum, qtrue, qtrue ) != NULL )
	{
		return;
	}
	
	ClientBegin( clientNum );
}


// Remove a bot by name or all bots
void ACESP_RemoveBot( char* name )
{
#if 0
	int             i;
	qboolean        freed = false;
	gentity_t*      bot;
	
	for( i = 0; i < maxclients->value; i++ )
	{
		bot = g_edicts + i + 1;
		if( bot->inuse )
		{
			if( bot->is_bot && ( strcmp( bot->client->pers.netname, name ) == 0 || strcmp( name, "all" ) == 0 ) )
			{
				bot->health = 0;
				player_die( bot, bot, bot, 100000, vec3_origin );
				// don't even bother waiting for death frames
				bot->deadflag = DEAD_DEAD;
				bot->inuse = false;
				freed = true;
				ACEIT_PlayerRemoved( bot );
				safe_bprintf( PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname );
			}
		}
	}
	
	if( !freed )
	{
		safe_bprintf( PRINT_MEDIUM, "%s not found\n", name );
	}
	
	//ACESP_SaveBots();         // Save them again
#endif
}

qboolean ACESP_BotConnect( int clientNum, qboolean restart )
{
#if 1
	char            userinfo[MAX_INFO_STRING];
	
	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	
	//Q_strncpyz(settings.characterfile, Info_ValueForKey(userinfo, "characterfile"), sizeof(settings.characterfile));
	//settings.skill = atof(Info_ValueForKey(userinfo, "skill"));
	//Q_strncpyz(settings.team, Info_ValueForKey(userinfo, "team"), sizeof(settings.team));
	
	//if(!BotAISetupClient(clientNum, &settings, restart))
	//{
	//  trap_DropClient(clientNum, "BotAISetupClient failed");
	//  return qfalse;
	//}
	
	/*
	   bot = &g_entities[clientNum];
	
	   // set bot state
	   bot = g_entities + clientNum;
	
	   bot->enemy = NULL;
	   bot->bs.moveTarget = NULL;
	   bot->bs.state = STATE_MOVE;
	
	   // set the current node
	   bot->bs.currentNode = ACEND_FindClosestReachableNode(bot, NODE_DENSITY, NODE_ALL);
	   bot->bs.goalNode = bot->bs.currentNode;
	   bot->bs.nextNode = bot->bs.currentNode;
	   bot->bs.next_move_time = level.time;
	   bot->bs.suicide_timeout = level.time + 15000;
	 */
	
	return qtrue;
#endif
}


void ACESP_SetupBotState( gentity_t* self )
{
	int             clientNum;
	char            userinfo[MAX_INFO_STRING];
	//char           *team;
	
	//G_Printf("ACESP_SetupBotState()\n");
	
	clientNum = self->client - level.clients;
	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	
	self->classname = "acebot";
	self->enemy = NULL;
	
	self->bs.turnSpeed = 35;	// FIXME 100 is deadly fast
	self->bs.moveTarget = NULL;
	self->bs.state = STATE_MOVE;
	
	// set the current node
	self->bs.currentNode = ACEND_FindClosestReachableNode( self, NODE_DENSITY, NODE_ALL );
	self->bs.goalNode = self->bs.currentNode;
	self->bs.nextNode = self->bs.currentNode;
	self->bs.lastNode = INVALID;
	self->bs.next_move_time = level.time;
	self->bs.suicide_timeout = level.time + 15000;
	
	/*
	   // is the bot part of a team when gameplay has changed?
	   team = Info_ValueForKey(userinfo, "team");
	   if(!team || !*team)
	   {
	   if(g_gametype.integer >= GT_TEAM)
	   {
	   if(PickTeam(clientNum) == TEAM_RED)
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
	   //Info_SetValueForKey(userinfo, "team", team);
	
	   // need to send this or bots will be spectators
	   trap_BotClientCommand(self - g_entities, va("team %s", team));
	   }
	 */
	
	//if(g_gametype.integer >= GT_TEAM)
	//  trap_BotClientCommand(self - g_entities, va("team %s", Info_ValueForKey(userinfo, "team")));
}



#endif
