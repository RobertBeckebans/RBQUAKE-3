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
// console.c

#include "client.h"


int             g_console_field_width = 78;


#define	NUM_CON_TIMES 5

#define		CON_TEXTSIZE	32768
#define		CON_LINESTEP  2

typedef struct
{
	qboolean        initialized;
	
	short           text[CON_TEXTSIZE];
	int             current;	// line where next message will be printed
	int             x;			// offset in current line for next print
	int             display;	// bottom of console displays this line
	
	int             linewidth;	// characters across screen
	int             totallines;	// total lines in console scrollback
	
	float           xadjust;	// for wide aspect screens
	float           yadjust;	// for narrow aspect screens
	
	float           displayFrac;	// aproaches finalFrac at scr_conspeed
	float           finalFrac;	// 0.0 to 1.0 lines of console to display
	
	int             vislines;	// in scanlines
	
	int             times[NUM_CON_TIMES];	// cls.realtime time the line was generated
	// for transparent notify lines
	vec4_t          color;
} console_t;

extern console_t con;

console_t       con;

cvar_t*         con_conshadow;
cvar_t*         con_conspeed;
cvar_t*         con_notifytime;
cvar_t*         con_showDate;
cvar_t*         con_showClock;
cvar_t*         con_clock12hr;
cvar_t*         con_clockSeconds;

#define	DEFAULT_CONSOLE_WIDTH	78

vec4_t          console_color = { 1.0, 1.0, 1.0, 1.0 };


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void )
{
	// Can't toggle the console when it's the only thing available
	if( cls.state == CA_DISCONNECTED && Key_GetCatcher() == KEYCATCH_CONSOLE )
	{
		return;
	}
	
	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	
	Con_ClearNotify();
	Key_SetCatcher( Key_GetCatcher() ^ KEYCATCH_CONSOLE );
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f( void )
{
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	
	Key_SetCatcher( Key_GetCatcher() ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f( void )
{
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	Key_SetCatcher( Key_GetCatcher() ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f( void )
{
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS )
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher() ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f( void )
{
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS )
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher() ^ KEYCATCH_MESSAGE );
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f( void )
{
	int             i;
	
	for( i = 0; i < CON_TEXTSIZE; i++ )
	{
		con.text[i] = ( ColorIndex( COLOR_WHITE ) << 8 ) | ' ';
	}
	
	Con_Bottom();				// go to end
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f( void )
{
	int             l, x, i;
	short*          line;
	fileHandle_t    f;
	char            buffer[1024];
	
	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "usage: condump <filename>\n" );
		return;
	}
	
	Com_Printf( "Dumped console text to %s.\n", Cmd_Argv( 1 ) );
	
	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if( !f )
	{
		Com_Printf( "ERROR: couldn't open.\n" );
		return;
	}
	
	// skip empty lines
	for( l = con.current - con.totallines + 1; l <= con.current; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for( x = 0; x < con.linewidth; x++ )
			if( ( line[x] & 0xff ) != ' ' )
			{
				break;
			}
		if( x != con.linewidth )
		{
			break;
		}
	}
	
	// write the remaining lines
	buffer[con.linewidth] = 0;
	for( ; l <= con.current; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for( i = 0; i < con.linewidth; i++ )
		{
			buffer[i] = line[i] & 0xff;
		}
		for( x = con.linewidth - 1; x >= 0; x-- )
		{
			if( buffer[x] == ' ' )
			{
				buffer[x] = 0;
			}
			else
			{
				break;
			}
		}
		strcat( buffer, "\n" );
		FS_Write( buffer, strlen( buffer ), f );
	}
	
	FS_FCloseFile( f );
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void )
{
	int             i;
	
	for( i = 0; i < NUM_CON_TIMES; i++ )
	{
		con.times[i] = 0;
	}
}



/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void )
{
	int             i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short           tbuf[CON_TEXTSIZE];
	
	width = ( SCREEN_WIDTH / SMALLCHAR_WIDTH ) - 2;
	
	if( width == con.linewidth )
	{
		return;
	}
	
	if( width < 1 )				// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for( i = 0; i < CON_TEXTSIZE; i++ )
		
		{
			con.text[i] = ( ColorIndex( COLOR_WHITE ) << 8 ) | ' ';
		}
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;
		
		if( con.totallines < numlines )
		{
			numlines = con.totallines;
		}
		
		numchars = oldwidth;
		
		if( con.linewidth < numchars )
		{
			numchars = con.linewidth;
		}
		
		Com_Memcpy( tbuf, con.text, CON_TEXTSIZE * sizeof( short ) );
		for( i = 0; i < CON_TEXTSIZE; i++ )
		
		{
			con.text[i] = ( ColorIndex( COLOR_WHITE ) << 8 ) | ' ';
		}
		
		
		for( i = 0; i < numlines; i++ )
		{
			for( j = 0; j < numchars; j++ )
			{
				con.text[( con.totallines - 1 - i ) * con.linewidth + j] =
					tbuf[( ( con.current - i + oldtotallines ) % oldtotallines ) * oldwidth + j];
			}
		}
		
		Con_ClearNotify();
	}
	
	con.current = con.totallines - 1;
	con.display = con.current;
}

/*
==================
Cmd_CompleteTxtName
==================
*/
void Cmd_CompleteTxtName( char* args, int argNum )
{
	if( argNum == 2 )
	{
		Field_CompleteFilename( "", "txt", qfalse );
	}
}


/*
================
Con_Init
================
*/
void Con_Init( void )
{
	int             i;
	
	con_notifytime = Cvar_Get( "con_notifytime", "4", CVAR_ARCHIVE );
	con_conspeed = Cvar_Get( "con_speed", "3", CVAR_ARCHIVE );
	con_conshadow = Cvar_Get( "con_shadow", "1", CVAR_ARCHIVE );
	con_clock12hr = Cvar_Get( "con_clock12hr", "0", CVAR_ARCHIVE );
	con_clockSeconds = Cvar_Get( "con_clockSeconds", "1", CVAR_ARCHIVE );
	con_showClock = Cvar_Get( "con_showClock", "1", CVAR_ARCHIVE );
	con_showDate = Cvar_Get( "con_showDate", "1", CVAR_ARCHIVE );
	
	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for( i = 0; i < COMMAND_HISTORY; i++ )
	{
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory();
	
	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "messagemode", Con_MessageMode_f );
	Cmd_AddCommand( "messagemode2", Con_MessageMode2_f );
	Cmd_AddCommand( "messagemode3", Con_MessageMode3_f );
	Cmd_AddCommand( "messagemode4", Con_MessageMode4_f );
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );
	Cmd_SetCommandCompletionFunc( "condump", Cmd_CompleteTxtName );
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed( qboolean skipnotify )
{
	int             i;
	
	// mark time for transparent overlay
	if( con.current >= 0 )
	{
		if( skipnotify )
		{
			con.times[con.current % NUM_CON_TIMES] = 0;
		}
		else
		{
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
	
	con.x = 0;
	if( con.display == con.current )
	{
		con.display++;
	}
	con.current++;
	for( i = 0; i < con.linewidth; i++ )
	{
		con.text[( con.current % con.totallines ) * con.linewidth + i] = ( ColorIndex( COLOR_WHITE ) << 8 ) | ' ';
	}
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char* txt )
{
	int             y;
	int             c, l;
	int             color;
	qboolean        skipnotify = qfalse;	// NERVE - SMF
	int             prev;		// NERVE - SMF
	
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if( !Q_strncmp( txt, "[skipnotify]", 12 ) )
	{
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if( cl_noprint && cl_noprint->integer )
	{
		return;
	}
	
	if( !con.initialized )
	{
		con.color[0] = con.color[1] = con.color[2] = con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}
	
	color = ColorIndex( COLOR_WHITE );
	
	while( ( c = *txt ) != 0 )
	{
		if( Q_IsColorString( txt ) )
		{
			color = ColorIndex( *( txt + 1 ) );
			txt += 2;
			continue;
		}
		
		// count word length
		for( l = 0; l < con.linewidth; l++ )
		{
			if( txt[l] <= ' ' )
			{
				break;
			}
			
		}
		
		// word wrap
		if( l != con.linewidth && ( con.x + l >= con.linewidth ) )
		{
			Con_Linefeed( skipnotify );
			
		}
		
		txt++;
		
		switch( c )
		{
			case '\n':
				Con_Linefeed( skipnotify );
				break;
			case '\r':
				con.x = 0;
				break;
			default:			// display character and advance
				y = con.current % con.totallines;
				con.text[y * con.linewidth + con.x] = ( color << 8 ) | c;
				con.x++;
				if( con.x >= con.linewidth )
				{
					Con_Linefeed( skipnotify );
					con.x = 0;
				}
				break;
		}
	}
	
	
	// mark time for transparent overlay
	if( con.current >= 0 )
	{
		// NERVE - SMF
		if( skipnotify )
		{
			prev = con.current % NUM_CON_TIMES - 1;
			if( prev < 0 )
			{
				prev = NUM_CON_TIMES - 1;
			}
			con.times[prev] = 0;
		}
		else
			// -NERVE - SMF
		{
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput( vec4_t color )
{
	int             style;
	
	if( cls.state != CA_DISCONNECTED && !( Key_GetCatcher() & KEYCATCH_CONSOLE ) )
	{
		return;
	}
	
	if( con_conshadow->value > 0 )
	{
		style = UI_DROPSHADOW;
	}
	else
	{
		style = 0;
	}
	
	
	//y = con.vislines - (SMALLCHAR_HEIGHT * 2);
	
	//re.SetColor(con.color);
	
	SCR_Text_Paint( 20, 234, 0.15f, color, "]", 0, 0, style | UI_PULSE, &cls.consoleFont );
	
	Field_Draw( &g_consoleField, 26, 234, SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue, qtrue );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void )
{
	int             x, v;
	short*          text;
	int             i;
	int             time;
	int             skip;
	int             currentColor;
	vec4_t          color;
	float           alpha;
	int             offset;
	
	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );
	
	v = 10;
	for( i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++ )
	{
		if( i < 0 )
		{
			continue;
		}
		time = con.times[i % NUM_CON_TIMES];
		if( time == 0 )
		{
			continue;
		}
		time = cls.realtime - time;
		if( time > con_notifytime->value * 1000 )
		{
			continue;
		}
		
		alpha = 1.0f - ( 1.0f / ( con_notifytime->value * 1000 ) * time );
		
		text = con.text + ( i % con.totallines ) * con.linewidth;
		
		if( cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
		{
			continue;
		}
		
		for( x = 0; x < con.linewidth; x++ )
		{
			if( ( text[x] & 0xff ) == ' ' )
			{
				continue;
			}
			if( ( ( text[x] >> 8 ) & 7 ) != currentColor )
			{
				currentColor = ( text[x] >> 8 ) & 7;
				re.SetColor( g_color_table[currentColor] );
			}
			
			
			//SCR_DrawSmallChar(cl_conXOffset->integer + con.xadjust + (x + 1) * SMALLCHAR_WIDTH, v, text[x] & 0xff);
			
			Vector4Copy( g_color_table[currentColor], color );
			
			color[3] = alpha;
			
			//offset = 8 - (alpha * 8); otty: good idea, but looks strange
			offset = 0;
			
			SCR_Text_PaintSingleChar( cl_conXOffset->integer + con.xadjust + ( x + 1 ) * 5, v - offset, 0.15f, color, text[x] & 0xff,
									  0, 0, UI_DROPSHADOW, &cls.consoleFont );
		}
		
		v += 8;
	}
	
	re.SetColor( NULL );
	
	if( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
	{
		return;
	}
	
	// draw the chat line
	if( Key_GetCatcher() & KEYCATCH_MESSAGE )
	{
		const char*     s;
		
		if( chat_team )
		{
			s = "say_team:";
		}
		else
		{
			s = "say:";
		}
		
		SCR_Text_PaintAligned( 8, v, s, 0.25f, UI_LEFT, colorWhite, &cls.consoleFont );
		skip = SCR_Text_Width( s, 0.25f, 0, &cls.consoleFont ) + 7;
		
		Field_BigDraw( &chatField, skip, v + SCR_Text_Height( s, 0.25f, 0, &cls.consoleFont ) / 2, SCREEN_WIDTH - ( skip + 1 ), qtrue,
					   qtrue );
					   
		v += BIGCHAR_HEIGHT;
	}
	
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/

int             nextLine = 0;
int             lineY = 10;
int             lineX = 10;
int             vel = 3;

void Con_DrawSolidConsole( float frac )
{
	int             i, x, y;
	int             rows;
	short*          text;
	int             row;
	int             lines;
	int             currentColor;
	vec4_t          color;
	vec4_t          fontColor;
	vec4_t          fontColorHighlight;
	char            displayTime[12];
	char            displayDate[15];
	qtime_t         tm;
	qtime_t         dt;
	float           alpha;
	int             style;
	int             rowOffset = 0;
	
	lines = cls.glconfig.vidHeight * frac;
	if( lines <= 0 )
	{
		return;
	}
	
	if( lines > cls.glconfig.vidHeight )
	{
		lines = cls.glconfig.vidHeight;
	}
	
	
	if( con_conshadow->value > 0 )
	{
		style = UI_DROPSHADOW;
	}
	else
	{
		style = 0;
	}
	
	// on wide screens, we will center the text
	con.yadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, &con.yadjust, NULL, NULL );
	
	// draw the background
	//y = frac * SCREEN_HEIGHT - 2;
	
	y = 240;
	
	alpha = frac;
	
	color[0] = 0.05f;
	color[1] = 0.25f;
	color[2] = 0.30f;
	color[3] = alpha * 0.85f;
	
	re.SetColor( color );
	SCR_FillRect( 10, 10, 620, 230, color );
	re.SetColor( NULL );
	
	if( cls.realtime > nextLine )
	{
		lineY += vel;
		
		if( lineY >= 240 || lineY <= 10 )
		{
			if( vel == 3 )
			{
				vel = -3;
			}
			else
			{
				vel = 3;
			}
		}
		
		lineX += 4;
		
		if( lineX >= 620 )
		{
			lineX = 10;
		}
		
		nextLine = cls.realtime + 10;
	}
	
	color[0] = 0.7f;
	color[1] = 0.7f;
	color[2] = 0.9f;
	color[3] *= 0.1f;
	
	SCR_FillRect( lineX, 10, 1, 230, color );
	
	SCR_FillRect( 10, lineY, 620, 1, color );
	
	SCR_Text_PaintAligned( 460, lineY, va( "%i%i", lineY, ( cls.realtime - nextLine ) ), 0.175f, UI_RIGHT, color, &cls.consoleFont );
	
	i = lineY + 40 * sin( cls.realtime / 600.0f );
	
	if( i > 240 )
	{
		i = 240;
	}
	if( lineY < 10 )
	{
		i = 10;
	}
	
	SCR_FillRect( 10, i, 620, 1, color );
	
	color[0] = 0.7f;
	color[1] = 0.7f;
	color[2] = 0.9f;
	color[3] = alpha * 0.75f;
	
	SCR_FillRect( 10, 10, 620, 1, color );	//top
	SCR_FillRect( 10, 240, 620, 1, color );	//buttom
	
	SCR_FillRect( 10, 10, 1, 230, color );	//left
	SCR_FillRect( 630, 10, 1, 230, color );	//right
	
	// draw the version number
	re.SetColor( g_color_table[ColorIndex( COLOR_RED )] );
	
	Vector4Set( fontColor, 1.0f, 1.0f, 1.0f, alpha );
	Vector4Set( fontColorHighlight, 1.0f, 1.0f, 1.0f, alpha * 1.5f );
	
	y = 230;
	
	// version string
	SCR_Text_PaintAligned( 626, y, Q3_VERSION, 0.2f, UI_RIGHT | style, fontColorHighlight, &cls.consoleFont );
	y -= 10;
	
	// engine string
	SCR_Text_PaintAligned( 626, y, Q3_ENGINE, 0.2f, UI_RIGHT | style, fontColorHighlight, &cls.consoleFont );
	y -= 10;
	
	// draw the date
	if( con_showDate->integer )
	{
	
		Com_RealTime( &dt );
		displayDate[0] = '\0';
		Q_strcat( displayDate, sizeof( displayDate ), va( "%02d/%02d/%04d", dt.tm_mday, dt.tm_mon + 1, 1900 + dt.tm_year ) );
		
		SCR_Text_PaintAligned( 626, y, displayDate, 0.175f, UI_RIGHT | style, fontColorHighlight, &cls.consoleFont );
		y -= 10;
	}
	
	if( con_showClock->integer )
	{
		Com_RealTime( &tm );
		displayTime[0] = '\0';
		if( con_clock12hr->integer )
		{
			Q_strcat( displayTime, sizeof( displayTime ),
					  va( "%d:%02d", ( ( tm.tm_hour == 0 || tm.tm_hour == 12 ) ? 12 : tm.tm_hour % 12 ), tm.tm_min ) );
			if( con_clockSeconds->integer )
			{
				Q_strcat( displayTime, sizeof( displayTime ), va( ":%02d", tm.tm_sec ) );
			}
			Q_strcat( displayTime, sizeof( displayTime ), ( tm.tm_hour < 12 ) ? " AM" : " PM" );
		}
		else
		{
			Q_strcat( displayTime, sizeof( displayTime ), va( "%d:%02d", tm.tm_hour, tm.tm_min ) );
			if( con_clockSeconds->integer )
			{
				Q_strcat( displayTime, sizeof( displayTime ), va( ":%02d", tm.tm_sec ) );
			}
		}
		
		
		SCR_Text_PaintAligned( 626, y, displayTime, 0.175f, UI_RIGHT | style, fontColorHighlight, &cls.consoleFont );
		y -= 10;
	}
	
	
	
	
	// draw the text
	con.vislines = lines;
	rows = ( lines - SMALLCHAR_WIDTH ) / SMALLCHAR_WIDTH;	// rows of text to draw
	
	rows = 26;
	
	y = 222;
	
	// draw from the bottom up
	if( con.display != con.current )
	{
		// draw arrows to show the buffer is backscrolled
		//re.SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		
		if( y >= con.yadjust )
		{
			for( x = 0; x < con.linewidth - 12; x += 3 )
				SCR_Text_PaintSingleChar( con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH + 15, y, 0.15f, fontColorHighlight, '^', 0, 0,
										  style, &cls.consoleBoldFont );
										  
			y -= SMALLCHAR_HEIGHT;
			rows -= CON_LINESTEP;
			rowOffset = CON_LINESTEP;
		}
	}
	
	row = con.display;
	row -= rowOffset;
	
	if( con.x == 0 )
	{
		row--;
	}
	
	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );
	
	for( i = 0; i < rows; i++, y -= 8, row-- )
	{
		if( row < con.yadjust )
		{
			break;
		}
		
		if( con.current - row >= con.totallines )
		{
			// past scrollback wrap point
			continue;
		}
		
		text = con.text + ( row % con.totallines ) * con.linewidth;
		
		for( x = 0; x < con.linewidth; x++ )
		{
			if( ( text[x] & 0xff ) == ' ' )
			{
				continue;
			}
			
			if( ( text[x] >> 8 ) != currentColor )
			{
				currentColor = ( text[x] >> 8 );
				re.SetColor( g_color_table[currentColor] );
			}
			//SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff);
			
			Vector4Copy( g_color_table[currentColor], color );
			color[3] = alpha * 1.5f;
			
			SCR_Text_PaintSingleChar( 15 + con.xadjust + ( x + 1 ) * 5, y, 0.15f, color, text[x] & 0xff, 0, 0, style,
									  &cls.consoleFont );
									  
									  
		}
		
		
	}
	
	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput( color );
	
	re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void )
{
	// check for console width changes from a vid mode change
	Con_CheckResize();
	
	// if disconnected, render console full screen
	if( cls.state == CA_DISCONNECTED )
	{
		if( !( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) )
		{
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}
	
	if( con.displayFrac )
	{
		Con_DrawSolidConsole( con.displayFrac );
	}
	else
	{
		// draw notify lines
		if( cls.state == CA_ACTIVE )
		{
			Con_DrawNotify();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole( void )
{
	// decide on the destination height of the console
	if( Key_GetCatcher() & KEYCATCH_CONSOLE )
	{
		con.finalFrac = 0.5;    // half screen
	}
	else
	{
		con.finalFrac = 0;    // none visible
	}
	
	// scroll towards the destination height
	if( con.finalFrac < con.displayFrac )
	{
		con.displayFrac -= con_conspeed->value * cls.realFrametime * 0.001;
		if( con.finalFrac > con.displayFrac )
		{
			con.displayFrac = con.finalFrac;
		}
		
	}
	else if( con.finalFrac > con.displayFrac )
	{
		con.displayFrac += con_conspeed->value * cls.realFrametime * 0.001;
		if( con.finalFrac < con.displayFrac )
		{
			con.displayFrac = con.finalFrac;
		}
	}
	
}


void Con_PageUp( void )
{
	con.display -= CON_LINESTEP;
	if( con.current - con.display >= con.totallines )
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void )
{
	con.display += CON_LINESTEP;
	if( con.display > con.current )
	{
		con.display = con.current;
	}
}

void Con_Top( void )
{
	con.display = con.totallines;
	if( con.current - con.display >= con.totallines )
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void )
{
	con.display = con.current;
}


void Con_Close( void )
{
	if( !com_cl_running->integer )
	{
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0;			// none visible
	con.displayFrac = 0;
}
