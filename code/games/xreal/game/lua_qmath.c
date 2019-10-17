/*
===========================================================================
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
// lua_qmath.c -- qmath library for Lua

#include "g_local.h"

#if(defined(G_LUA) || defined(CG_LUA))

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int qmath_abs( lua_State* L )
{
	lua_pushnumber( L, fabs( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_sin( lua_State* L )
{
	lua_pushnumber( L, sin( DEG2RAD( luaL_checknumber( L, 1 ) ) ) );
	return 1;
}

static int qmath_cos( lua_State* L )
{
	lua_pushnumber( L, cos( DEG2RAD( luaL_checknumber( L, 1 ) ) ) );
	return 1;
}

static int qmath_tan( lua_State* L )
{
	lua_pushnumber( L, tan( DEG2RAD( luaL_checknumber( L, 1 ) ) ) );
	return 1;
}

static int qmath_asin( lua_State* L )
{
	lua_pushnumber( L, RAD2DEG( asin( luaL_checknumber( L, 1 ) ) ) );
	return 1;
}

static int qmath_acos( lua_State* L )
{
	lua_pushnumber( L, RAD2DEG( acos( luaL_checknumber( L, 1 ) ) ) );
	return 1;
}

static int qmath_atan( lua_State* L )
{
	lua_pushnumber( L, RAD2DEG( atan( luaL_checknumber( L, 1 ) ) ) );
	return 1;
}

static int qmath_atan2( lua_State* L )
{
	lua_pushnumber( L, RAD2DEG( atan2( luaL_checknumber( L, 1 ), luaL_checknumber( L, 2 ) ) ) );
	return 1;
}

static int qmath_ceil( lua_State* L )
{
	lua_pushnumber( L, ceil( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_floor( lua_State* L )
{
	lua_pushnumber( L, floor( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_fmod( lua_State* L )
{
	lua_pushnumber( L, fmod( luaL_checknumber( L, 1 ), luaL_checknumber( L, 2 ) ) );
	return 1;
}

static int qmath_modf( lua_State* L )
{
	double          ip;
	double          fp = modf( luaL_checknumber( L, 1 ), &ip );
	
	lua_pushnumber( L, ip );
	lua_pushnumber( L, fp );
	return 2;
}

static int qmath_sqrt( lua_State* L )
{
	lua_pushnumber( L, sqrt( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_pow( lua_State* L )
{
	lua_pushnumber( L, pow( luaL_checknumber( L, 1 ), luaL_checknumber( L, 2 ) ) );
	return 1;
}

static int qmath_log( lua_State* L )
{
	lua_pushnumber( L, log( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_log10( lua_State* L )
{
	lua_pushnumber( L, log10( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_exp( lua_State* L )
{
	lua_pushnumber( L, exp( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_deg( lua_State* L )
{
	lua_pushnumber( L, RAD2DEG( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_rad( lua_State* L )
{
	lua_pushnumber( L, DEG2RAD( luaL_checknumber( L, 1 ) ) );
	return 1;
}

static int qmath_frexp( lua_State* L )
{
	int             e;
	
	lua_pushnumber( L, frexp( luaL_checknumber( L, 1 ), &e ) );
	lua_pushnumber( L, e );
	return 2;
}

static int qmath_ldexp( lua_State* L )
{
	lua_pushnumber( L, ldexp( luaL_checknumber( L, 1 ), luaL_checkint( L, 2 ) ) );
	return 1;
}



static int qmath_min( lua_State* L )
{
	int             n = lua_gettop( L );	/* number of arguments */
	lua_Number      dmin = luaL_checknumber( L, 1 );
	int             i;
	
	for( i = 2; i <= n; i++ )
	{
		lua_Number      d = luaL_checknumber( L, i );
		
		if( d < dmin )
		{
			dmin = d;
		}
	}
	lua_pushnumber( L, dmin );
	return 1;
}


static int qmath_max( lua_State* L )
{
	int             n = lua_gettop( L );	/* number of arguments */
	lua_Number      dmax = luaL_checknumber( L, 1 );
	int             i;
	
	for( i = 2; i <= n; i++ )
	{
		lua_Number      d = luaL_checknumber( L, i );
		
		if( d > dmax )
		{
			dmax = d;
		}
	}
	lua_pushnumber( L, dmax );
	return 1;
}

#if 0
static int qmath_random( lua_State* L )
{
	/* the `%' avoids the (rare) case of r==1, and is needed also because on
	   some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
	lua_Number      r = ( lua_Number )( rand() % RAND_MAX ) / ( lua_Number ) RAND_MAX;
	
	switch( lua_gettop( L ) )
	{
		/* check number of arguments */
		case 0:
		{
			/* no arguments */
			lua_pushnumber( L, r );	/* Number between 0 and 1 */
			break;
		}
		case 1:
		{
			/* only upper limit */
			int             u = luaL_checkint( L, 1 );
			
			luaL_argcheck( L, 1 <= u, 1, "interval is empty" );
			lua_pushnumber( L, ( int )floor( r * u ) + 1 );	/* int between 1 and `u' */
			break;
		}
		case 2:
		{
			/* lower and upper limits */
			int             l = luaL_checkint( L, 1 );
			int             u = luaL_checkint( L, 2 );
			
			luaL_argcheck( L, l <= u, 2, "interval is empty" );
			lua_pushnumber( L, ( int )floor( r * ( u - l + 1 ) ) + l );	/* int between `l' and `u' */
			break;
		}
		default:
			return luaL_error( L, "wrong number of arguments" );
	}
	return 1;
}


static int qmath_randomseed( lua_State* L )
{
	srand( luaL_checkint( L, 1 ) );
	return 0;
}
#endif

static int qmath_rand( lua_State* L )
{
	lua_pushinteger( L, rand() );
	return 1;
}

static int qmath_random( lua_State* L )
{
	lua_pushnumber( L, random() );
	return 1;
}

static int qmath_crandom( lua_State* L )
{
	lua_pushnumber( L, crandom() );
	return 1;
}

static const luaL_reg qmathlib[] =
{
	{"abs", qmath_abs},
	{"sin", qmath_sin},
	{"cos", qmath_cos},
	{"tan", qmath_tan},
	{"asin", qmath_asin},
	{"acos", qmath_acos},
	{"atan", qmath_atan},
	{"atan2", qmath_atan2},
	{"ceil", qmath_ceil},
	{"floor", qmath_floor},
	{"fmod", qmath_fmod},
	{"modf", qmath_modf},
	{"frexp", qmath_frexp},
	{"ldexp", qmath_ldexp},
	{"sqrt", qmath_sqrt},
	{"min", qmath_min},
	{"max", qmath_max},
	{"log", qmath_log},
	{"log10", qmath_log10},
	{"exp", qmath_exp},
	{"deg", qmath_deg},
	{"pow", qmath_pow},
	{"rad", qmath_rad},
	{"rand", qmath_rand},
	{"random", qmath_random},
//  {"randomseed", qmath_randomseed},
	{"crandom", qmath_crandom},
	{NULL, NULL}
};

int luaopen_qmath( lua_State* L )
{
	luaL_register( L, "qmath", qmathlib );
	lua_pushnumber( L, M_PI );
	lua_setfield( L, -2, "pi" );
	lua_pushnumber( L, HUGE_VAL );
	lua_setfield( L, -2, "huge" );
	return 1;
}

#endif
