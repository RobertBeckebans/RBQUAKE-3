/*
===========================================================================
Copyright (C) 2007 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// lua_particle.c -- particle library for Lua

#include "cg_local.h"

#ifdef CG_LUA

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int particle_Spawn(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	p = CG_AllocParticle();
	if(!p)
	{
		lua_pushnil(L);
		return 1;
	}

	lp = lua_newuserdata(L, sizeof(lua_Particle));

	luaL_getmetatable(L, "cgame.particle");
	lua_setmetatable(L, -2);

	lp->p = p;

	return 1;
}

static int particle_SetType(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->type = luaL_checkinteger(L, 2);

	return 1;
}

static int particle_SetShader(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->pshader = luaL_checkinteger(L, 2);

	return 1;
}

static int particle_SetDuration(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;
	float           duration;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	duration = luaL_checknumber(L, 2);

	p->endTime = cg.time + duration;

	return 1;
}

static int particle_SetOrigin(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;
	vec_t          *origin;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	origin = lua_getvector(L, 2);
	VectorCopy(origin, p->org);
	VectorCopy(origin, p->oldOrg);

	return 1;
}

static int particle_SetVelocity(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;
	vec_t          *v;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	v = lua_getvector(L, 2);
	VectorCopy(v, p->vel);

	return 1;
}

static int particle_SetAcceleration(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;
	vec_t          *v;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	v = lua_getvector(L, 2);
	VectorCopy(v, p->accel);

	return 1;
}

static int particle_SetColor(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->color[0] = luaL_checknumber(L, 2);
	p->color[1] = luaL_checknumber(L, 3);
	p->color[2] = luaL_checknumber(L, 4);
	p->color[3] = luaL_checknumber(L, 5);

	return 1;
}

static int particle_SetColorVelocity(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->colorVel[0] = luaL_checknumber(L, 2);
	p->colorVel[1] = luaL_checknumber(L, 3);
	p->colorVel[2] = luaL_checknumber(L, 4);
	p->colorVel[3] = luaL_checknumber(L, 5);

	return 1;
}

static int particle_SetWidth(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->width = luaL_checknumber(L, 2);

	return 1;
}

static int particle_SetHeight(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->height = luaL_checknumber(L, 2);

	return 1;
}

static int particle_SetEndWidth(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->endWidth = luaL_checknumber(L, 2);

	return 1;
}

static int particle_SetEndHeight(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->endHeight = luaL_checknumber(L, 2);

	return 1;
}

static int particle_SetRotation(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;

	lp = lua_getparticle(L, 1);
	p = lp->p;

	p->rotate = qtrue;
	p->roll = luaL_checknumber(L, 2);

	return 1;
}

static int particle_GC(lua_State * L)
{
//  G_Printf("Lua says bye to entity = %p\n", lua_getparticle(L));

	return 0;
}

static int particle_ToString(lua_State * L)
{
	cparticle_t    *p;
	lua_Particle   *lp;
	char            buf[MAX_STRING_CHARS];

	lp = lua_getparticle(L, 1);
	p = lp->p;
	Com_sprintf(buf, sizeof(buf), "particle: spawn time = %i\n", p->time);
	lua_pushstring(L, buf);

	return 1;
}

static const luaL_reg particle_ctor[] = {
	{"Spawn", particle_Spawn},
	{NULL, NULL}
};

static const luaL_reg particle_meta[] = {
	{"SetType", particle_SetType},
	{"SetShader", particle_SetShader},
	{"SetDuration", particle_SetDuration},
	{"SetOrigin", particle_SetOrigin},
	{"SetVelocity", particle_SetVelocity},
	{"SetAcceleration", particle_SetAcceleration},
	{"SetColor", particle_SetColor},
	{"SetColorVelocity", particle_SetColorVelocity},
	{"SetWidth", particle_SetWidth},
	{"SetHeight", particle_SetHeight},
	{"SetEndWidth", particle_SetEndWidth},
	{"SetEndHeight", particle_SetEndHeight},
	{"SetRotation", particle_SetRotation},
	{"__gc", particle_GC},
	{"__tostring", particle_ToString},
	{NULL, NULL}
};

int luaopen_particle(lua_State * L)
{
	luaL_newmetatable(L, "cgame.particle");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);		// pushes the metatable
	lua_settable(L, -3);		// metatable.__index = metatable

	luaL_register(L, NULL, particle_meta);
	luaL_register(L, "particle", particle_ctor);

	// add particle types
	lua_pushinteger(L, P_WEATHER);
	lua_setfield(L, -2, "WEATHER");

	lua_pushinteger(L, P_WEATHER_FLURRY);
	lua_setfield(L, -2, "WEATHER_FLURRY");

	lua_pushinteger(L, P_WEATHER_TURBULENT);
	lua_setfield(L, -2, "WEATHER_TURBULENT");

	lua_pushinteger(L, P_FLAT);
	lua_setfield(L, -2, "FLAT");

	lua_pushinteger(L, P_FLAT_SCALEUP);
	lua_setfield(L, -2, "FLAT_SCALEUP");

	lua_pushinteger(L, P_FLAT_SCALEUP_FADE);
	lua_setfield(L, -2, "FLAT_SCALEUP_FADE");

	lua_pushinteger(L, P_SMOKE);
	lua_setfield(L, -2, "SMOKE");

	lua_pushinteger(L, P_SMOKE_IMPACT);
	lua_setfield(L, -2, "SMOKE_IMPACT");

	lua_pushinteger(L, P_BLOOD);
	lua_setfield(L, -2, "BLOOD");

	lua_pushinteger(L, P_BUBBLE);
	lua_setfield(L, -2, "BUBBLE");

	lua_pushinteger(L, P_BUBBLE_TURBULENT);
	lua_setfield(L, -2, "BUBBLE_TURBULENT");

	lua_pushinteger(L, P_SPRITE);
	lua_setfield(L, -2, "SPRITE");

	lua_pushinteger(L, P_SPARK);
	lua_setfield(L, -2, "SPARK");

	return 1;
}

void lua_pushparticle(lua_State * L, cparticle_t * p)
{
	lua_Particle   *lp;

	lp = lua_newuserdata(L, sizeof(lua_Particle));

	luaL_getmetatable(L, "cgame.particle");
	lua_setmetatable(L, -2);

	lp->p = p;
}

lua_Particle   *lua_getparticle(lua_State * L, int argNum)
{
	void           *ud;

	ud = luaL_checkudata(L, argNum, "cgame.particle");
	luaL_argcheck(L, ud != NULL, argNum, "`particle' expected");
	return (lua_Particle *) ud;
}

#endif
