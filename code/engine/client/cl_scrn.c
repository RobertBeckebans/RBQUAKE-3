/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2008 Adrian Fuhrmann

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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean scr_initialized; // ready to draw

cvar_t*	 cl_timegraph;
cvar_t*	 cl_debuggraph;
cvar_t*	 cl_graphheight;
cvar_t*	 cl_graphscale;
cvar_t*	 cl_graphshift;
cvar_t*	 cl_keepVidAspect;

/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void	 SCR_DrawNamedPic( float x, float y, float width, float height, const char* picname )
{
	qhandle_t hShader;

	assert( width != 0 );

	hShader = re.RegisterShader( picname );
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640( float* x, float* y, float* w, float* h )
{
	float xscale;
	float yscale;
	float xbias = 0.0f;
	float ybias = 0.0f;

	// adjust for wide screens
	xscale = cls.glconfig.vidWidth / 640.0f;
	yscale = cls.glconfig.vidHeight / 480.0f;

	if( cl_keepVidAspect->integer )
	{
		if( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
		{
			xbias  = 0.5f * ( cls.glconfig.vidWidth - ( cls.glconfig.vidHeight * 640.0f / 480.0f ) );
			xscale = yscale;
		}
		else if( cls.glconfig.vidWidth * 480 < cls.glconfig.vidHeight * 640 )
		{
			ybias  = 0.5f * ( cls.glconfig.vidHeight - ( cls.glconfig.vidWidth * 480.0f / 640.0f ) );
			yscale = xscale;
		}
	}

	// scale for screen sizes
	if( x )
	{
		*x = xbias + *x * xscale;
	}
	if( y )
	{
		*y = ybias + *y * yscale;
	}
	if( w )
	{
		*w *= xscale;
	}
	if( h )
	{
		*h *= yscale;
	}
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( float x, float y, float width, float height, const float* color )
{
	re.SetColor( color );

	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );

	re.SetColor( NULL );
}

/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader )
{
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar( int x, int y, float size, int ch )
{
	int	  row, col;
	float frow, fcol;
	float ax, ay, aw, ah;

	ch &= 255;

	if( ch == ' ' )
	{
		return;
	}

	if( y < -size )
	{
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;
	SCR_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	re.DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + size, frow + size, cls.charSetShader );
}

/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar( int x, int y, int ch )
{
	int	  row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if( ch == ' ' )
	{
		return;
	}

	if( y < -SMALLCHAR_HEIGHT )
	{
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	re.DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow, fcol + size, frow + size, cls.charSetShader );
}

// ========================================================================================

int SCR_Text_Width( const char* text, float scale, int limit, const fontInfo_t* font )
{
	int				   count, len;
	float			   out;
	const glyphInfo_t* glyph;
	float			   useScale;

	// FIXME: see ui_main.c, same problem
	//  const unsigned char *s = text;
	const char*		   s = text;

	useScale = scale * font->glyphScale;
	out		 = 0;
	if( text )
	{
		len = strlen( text );
		if( limit > 0 && len > limit )
		{
			len = limit;
		}
		count = 0;
		while( s && *s && count < len )
		{
			if( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[( int )*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

int SCR_Text_Height( const char* text, float scale, int limit, const fontInfo_t* font )
{
	int				   len, count;
	float			   max;
	const glyphInfo_t* glyph;
	float			   useScale;

	// TTimo: FIXME
	//  const unsigned char *s = text;
	const char*		   s = text;

	useScale = scale * font->glyphScale;
	max		 = 0;
	if( text )
	{
		len = strlen( text );
		if( limit > 0 && len > limit )
		{
			len = limit;
		}
		count = 0;
		while( s && *s && count < len )
		{
			if( Q_IsColorString( s ) )
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[( int )*s];
				if( max < glyph->height )
				{
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}

	return max * useScale;
}

void SCR_Text_PaintSingleChar( float x, float y, float scale, const vec4_t color, int ch, float adjust, int limit, int style, const fontInfo_t* font )
{
	const glyphInfo_t* glyph;
	float			   useScale;
	float			   yadj;

	ch &= 255;

	if( ch == ' ' )
	{
		return;
	}

	useScale = scale * font->glyphScale;

	glyph = &font->glyphs[ch];
	yadj  = useScale * glyph->top;

	if( style & UI_DROPSHADOW ) // || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
	{
		int ofs = 1; // style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

		colorBlack[3] = color[3];
		re.SetColor( colorBlack );
		SCR_Text_PaintChar( x + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
		colorBlack[3] = 1.0;
	}

	re.SetColor( color );

	SCR_Text_PaintChar( x, y - yadj, glyph->imageWidth, glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );

	re.SetColor( NULL );
}

void SCR_Text_PaintChar( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader )
{
	float w, h;

	w = width * scale;
	h = height * scale;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	re.DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void SCR_Text_Paint( float x, float y, float scale, const vec4_t color, const char* text, float adjust, int limit, int style, const fontInfo_t* font )
{
	int				   len, count;
	vec4_t			   newColor;
	const glyphInfo_t* glyph;
	float			   useScale;

	useScale = scale * font->glyphScale;
	if( text )
	{
		// TTimo: FIXME
		//      const unsigned char *s = text;
		const char* s = text;

		re.SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = strlen( text );
		if( limit > 0 && len > limit )
		{
			len = limit;
		}
		count = 0;
		while( s && *s && count < len )
		{
			glyph = &font->glyphs[( int )*s];

			if( Q_IsColorString( s ) )
			{
				memcpy( newColor, ( float* )g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				re.SetColor( newColor );
				s += 2;
				continue;
			}
			else
			{
				float yadj = useScale * glyph->top;

				if( style & UI_DROPSHADOW ) // || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
				{
					int ofs = 1; // style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

					colorBlack[3] = newColor[3];
					re.SetColor( colorBlack );
					SCR_Text_PaintChar( x + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
					colorBlack[3] = 1.0;
					re.SetColor( newColor );
				}
				SCR_Text_PaintChar( x, y - yadj, glyph->imageWidth, glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );

				x += ( glyph->xSkip * useScale ) + adjust;
				s++;
				count++;
			}
		}
		re.SetColor( NULL );
	}
}

void SCR_Text_PaintAligned( int x, int y, const char* s, float scale, int style, const vec4_t color, const fontInfo_t* font )
{
	int w, h;

	w = SCR_Text_Width( s, scale, 0, font );
	h = SCR_Text_Height( s, scale, 0, font );

	if( style & UI_CENTER )
	{
		SCR_Text_Paint( x - w / 2, y + h / 2, scale, color, s, 0, 0, style, font );
	}
	else if( style & UI_RIGHT )
	{
		SCR_Text_Paint( x - w, y + h / 2, scale, color, s, 0, 0, style, font );
	}
	else
	{
		// UI_LEFT
		SCR_Text_Paint( x, y + h / 2, scale, color, s, 0, 0, style, font );
	}
}

// ========================================================================================

/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
static void SCR_DrawStringExt( int x, int y, float size, const char* string, const float* setColor, qboolean forceColor, qboolean noColorEscape )
{
	vec4_t		color;
	const char* s;
	int			xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
	color[3]					   = setColor[3];
	re.SetColor( color );
	s  = string;
	xx = x;
	while( *s )
	{
		if( !noColorEscape && Q_IsColorString( s ) )
		{
			s += 2;
			continue;
		}
		SCR_DrawChar( xx + 2, y + 2, size, *s );
		xx += size;
		s++;
	}

	// draw the colored text
	s  = string;
	xx = x;
	re.SetColor( setColor );
	while( *s )
	{
		if( !noColorEscape && Q_IsColorString( s ) )
		{
			if( !forceColor )
			{
				Com_Memcpy( color, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawChar( xx, y, size, *s );
		xx += size;
		s++;
	}
	re.SetColor( NULL );
}

void SCR_DrawBigString( int x, int y, const char* s, float alpha, qboolean noColorEscape )
{
	float color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3]					   = alpha;
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qfalse, noColorEscape );
}

void SCR_DrawBigStringColor( int x, int y, const char* s, vec4_t color, qboolean noColorEscape )
{
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qtrue, noColorEscape );
}

/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.
==================
*/
void SCR_DrawSmallStringExt( int x, int y, const char* string, float* setColor, qboolean forceColor, qboolean noColorEscape )
{
	vec4_t		color;
	const char* s;
	int			xx;

	// draw the colored text
	s  = string;
	xx = x;
	re.SetColor( setColor );
	while( *s )
	{
		if( !noColorEscape && Q_IsColorString( s ) )
		{
			if( !forceColor )
			{
				Com_Memcpy( color, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawSmallChar( xx, y, *s );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re.SetColor( NULL );
}

/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen( const char* str )
{
	const char* s	  = str;
	int			count = 0;

	while( *s )
	{
		if( Q_IsColorString( s ) )
		{
			s += 2;
		}
		else
		{
			count++;
			s++;
		}
	}

	return count;
}

/*
** SCR_GetBigStringWidth
*/
int SCR_GetBigStringWidth( const char* str )
{
	return SCR_Strlen( str ) * 16;
}

//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording()
{
	char string[1024];
	int	 pos;

	if( !clc.demorecording )
	{
		return;
	}
	if( clc.spDemoRecording )
	{
		return;
	}

	pos = FS_FTell( clc.demofile );
	sprintf( string, "RECORDING %s: %ik", clc.demoName, pos / 1024 );

	SCR_Text_PaintAligned( 320, 45, string, 0.2f, UI_CENTER, g_color_table[7], &cls.consoleFont );
}

#ifdef USE_VOIP
/*
=================
SCR_DrawVoipMeter
=================
*/
void SCR_DrawVoipMeter()
{
	char buffer[16];
	char string[256];
	int	 limit, i;

	if( !cl_voipShowMeter->integer )
	{
		return; // player doesn't want to show meter at all.
	}
	else if( !cl_voipSend->integer )
	{
		return; // not recording at the moment.
	}
	else if( clc.state != CA_ACTIVE )
	{
		return; // not connected to a server.
	}
	else if( !cl_connectedToVoipServer )
	{
		return; // server doesn't support VoIP.
	}
	else if( clc.demoplaying )
	{
		return; // playing back a demo.
	}
	else if( !cl_voip->integer )
	{
		return; // client has VoIP support disabled.
	}

	limit = ( int )( clc.voipPower * 10.0f );
	if( limit > 10 )
	{
		limit = 10;
	}

	for( i = 0; i < limit; i++ )
	{
		buffer[i] = '*';
	}
	while( i < 10 )
	{
		buffer[i++] = ' ';
	}
	buffer[i] = '\0';

	sprintf( string, "VoIP: [%s]", buffer );
	SCR_DrawStringExt( 320 - strlen( string ) * 4, 10, 8, string, g_color_table[7], qtrue, qfalse );
}
#endif

/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

typedef struct
{
	float value;
	int	  color;
} graphsamp_t;

static int		   current;
static graphsamp_t values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void			   SCR_DebugGraph( float value, int color )
{
	values[current & 1023].value = value;
	values[current & 1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph()
{
	int	  a, x, y, w, i, h;
	float v;
	int	  color;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re.SetColor( g_color_table[0] );
	re.DrawStretchPic( x, y - cl_graphheight->integer, w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for( a = 0; a < w; a++ )
	{
		i	  = ( current - 1 - a + 1024 ) & 1023;
		v	  = values[i].value;
		color = values[i].color;
		v	  = v * cl_graphscale->integer + cl_graphshift->integer;

		if( v < 0 )
		{
			v += cl_graphheight->integer * ( 1 + ( int )( -v / cl_graphheight->integer ) );
		}
		h = ( int )v % cl_graphheight->integer;
		re.DrawStretchPic( x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init()
{
	cl_timegraph	 = Cvar_Get( "timegraph", "0", CVAR_CHEAT );
	cl_debuggraph	 = Cvar_Get( "debuggraph", "0", CVAR_CHEAT );
	cl_graphheight	 = Cvar_Get( "graphheight", "32", CVAR_CHEAT );
	cl_graphscale	 = Cvar_Get( "graphscale", "1", CVAR_CHEAT );
	cl_graphshift	 = Cvar_Get( "graphshift", "0", CVAR_CHEAT );
	cl_keepVidAspect = Cvar_Get( "cl_keepVidAspect", "0", CVAR_ARCHIVE );

	scr_initialized = qtrue;
}

//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField( stereoFrame_t stereoFrame )
{
	qboolean uiFullscreen;

	re.BeginFrame( stereoFrame );

	uiFullscreen = ( uivm && VM_Call( uivm, UI_IS_FULLSCREEN ) );

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if( uiFullscreen || clc.state < CA_LOADING )
	{
		if( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
		{
			re.SetColor( g_color_table[0] );
			re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if( uivm && !uiFullscreen )
	{
		switch( clc.state )
		{
			default:
				Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad clc.state" );
				break;
			case CA_CINEMATIC:
				SCR_DrawCinematic();
				break;
			case CA_DISCONNECTED:
				// force menu up
				S_StopAllSounds();
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
				break;
			case CA_CONNECTING:
			case CA_CHALLENGING:
			case CA_CONNECTED:
				// connecting clients will only show the connection dialog
				// refresh to update the time
				VM_Call( uivm, UI_REFRESH, cls.realtime );
				VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
				break;
			case CA_LOADING:
			case CA_PRIMED:
				// draw the game information screen and loading progress
				CL_CGameRendering( stereoFrame );

				// also draw the connection information, so it doesn't
				// flash away too briefly on local or lan games
				// refresh to update the time
				VM_Call( uivm, UI_REFRESH, cls.realtime );
				VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );
				break;
			case CA_ACTIVE:
				// always supply STEREO_CENTER as vieworg offset is now done by the engine.
				CL_CGameRendering( stereoFrame );
				SCR_DrawDemoRecording();
#ifdef USE_VOIP
				SCR_DrawVoipMeter();
#endif
				break;
		}
	}

	// the menu draws next
	if( Key_GetCatcher() & KEYCATCH_UI )
	{
		if( uivm )
		{
			VM_Call( uivm, UI_REFRESH, cls.realtime );
		}
	}

	// console draws next
	Con_DrawConsole();

	// debug graph can be drawn on top of anything
	if( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer )
	{
		SCR_DrawDebugGraph();
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen()
{
	static int recursive;

	if( !scr_initialized )
	{
		return; // not initialized yet
	}

	if( ++recursive > 2 )
	{
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	// If there is no VM, there are also no rendering commands issued. Stop the renderer in
	// that case.
	if( uivm || com_dedicated->integer )
	{
		// if running in stereo, we need to draw the frame twice
		if( cls.glconfig.stereoEnabled )
		{
			SCR_DrawScreenField( STEREO_LEFT );
			SCR_DrawScreenField( STEREO_RIGHT );
		}
		else
		{
			SCR_DrawScreenField( STEREO_CENTER );
		}

		if( com_speeds->integer )
		{
			re.EndFrame( &time_frontend, &time_backend );
		}
		else
		{
			re.EndFrame( NULL, NULL );
		}
	}

	recursive = 0;
}
