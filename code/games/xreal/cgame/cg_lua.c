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
//
// cg_lua.c

#include "cg_local.h"

#ifdef CG_LUA

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


static lua_State *cg_luaState = NULL;

/*
============
CG_InitLua
============
*/
void CG_InitLua()
{
	int             numdirs;
	char            filename[MAX_QPATH];
	char            dirlist[1024];
	char           *dirptr;
	int             i;
	int             dirlen;

	//vec3_t          in;
	//float           out;

	CG_Printf("------- CGame Lua Initialization -------\n");

	cg_luaState = lua_open();

	// Lua standard lib
	luaopen_base(cg_luaState);
	luaopen_string(cg_luaState);

	// Quake lib
	luaopen_particle(cg_luaState);
	// TODO luaopen_localEntity(cg_luaState);
	luaopen_cgame(cg_luaState);
	luaopen_qmath(cg_luaState);
	luaopen_vector(cg_luaState);

	// get all effects from effects/*.lua files
	numdirs = trap_FS_GetFileList("effects", ".lua", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		Q_strncpyz(filename, "effects/", sizeof(filename));
		Q_strcat(filename, sizeof(filename), dirptr);

		CG_LoadLuaScript(filename);
	}

#if 0
	CG_DumpLuaStack();

	// run some tests
	VectorSet(in, 5, 7, 3);
	CG_RunLuaFunction("TestVectors", "v>f", in, &out);
	CG_Printf("result of testVectors() is %f\n", out);
	//CG_Printf("result of testVectors() is %i %i %i\n", (int)out[0], (int)out[1], (int)out[2]);
#endif

	CG_Printf("-----------------------------------\n");
}



/*
=================
CG_ShutdownLua
=================
*/
void CG_ShutdownLua()
{
	CG_Printf("------- Game Lua Finalization -------\n");

	if(cg_luaState)
	{
		lua_close(cg_luaState);
		cg_luaState = NULL;
	}

	CG_Printf("-----------------------------------\n");
}


/*
=================
CG_LoadLuaScript
=================
*/
void CG_LoadLuaScript(const char *filename)
{
	int             len;
	fileHandle_t    f;
	char           *buf;

	CG_Printf("...loading '%s'\n", filename);

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f)
	{
		CG_Printf(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}

	buf = malloc(len + 1);

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	if(luaL_loadbuffer(cg_luaState, buf, strlen(buf), filename))
		CG_Printf("G_RunLuaScript: cannot load lua file: %s\n", lua_tostring(cg_luaState, -1));

	if(lua_pcall(cg_luaState, 0, 0, 0))
		CG_Printf("G_RunLuaScript: cannot pcall: %s\n", lua_tostring(cg_luaState, -1));

	free(buf);
}

/*
=================
CG_RunLuaFunction
=================
*/
void CG_RunLuaFunction(const char *func, const char *sig, ...)
{
	va_list         vl;
	int             narg, nres;	// number of arguments and results
	lua_State      *L = cg_luaState;

	if(!func || !func[0])
		return;

	va_start(vl, sig);
	lua_getglobal(L, func);		// get function

	// push arguments
	narg = 0;
	while(*sig)
	{
		switch (*sig++)
		{
			case 'f':
				// float argument
				lua_pushnumber(L, va_arg(vl, double));

				break;

			case 'i':
				// int argument
				lua_pushnumber(L, va_arg(vl, int));

				break;

			case 's':
				// string argument
				lua_pushstring(L, va_arg(vl, char *));

				break;

				/*
				   TODO ?
				   case 'p':
				   // particle argument
				   lua_pushparticle(L, va_arg(vl, lua_Particle *));
				   break;
				 */

			case 'v':
				// vector argument
				lua_pushvector(L, va_arg(vl, vec_t *));
				break;

			case '>':
				goto endwhile;

			default:
				CG_Printf("CG_RunLuaFunction: invalid option (%c)\n", *(sig - 1));
		}
		narg++;
		luaL_checkstack(L, 1, "too many arguments");
	}
  endwhile:

	// do the call
	nres = strlen(sig);			// number of expected results
	if(lua_pcall(L, narg, nres, 0) != 0)	// do the call
		CG_Printf("CG_RunLuaFunction: error running function `%s': %s\n", func, lua_tostring(L, -1));

	// retrieve results
	nres = -nres;				// stack index of first result
	while(*sig)
	{							// get results
		switch (*sig++)
		{

			case 'f':
				// float result
				if(!lua_isnumber(L, nres))
					CG_Printf("CG_RunLuaFunction: wrong result type\n");
				*va_arg(vl, float *) = lua_tonumber(L, nres);

				break;

			case 'i':
				// int result
				if(!lua_isnumber(L, nres))
					CG_Printf("CG_RunLuaFunction: wrong result type\n");
				*va_arg(vl, int *) = (int)lua_tonumber(L, nres);

				break;

			case 's':
				// string result
				if(!lua_isstring(L, nres))
					CG_Printf("CG_RunLuaFunction: wrong result type\n");
				*va_arg(vl, const char **) = lua_tostring(L, nres);

				break;

#if 0
				FIXME this causes a crash case 'v':
					// string result
				if              (!lua_getvector(L, nres))
					                CG_Printf("CG_RunLuaFunction: wrong result type\n");

				*va_arg(vl, vec_t **) = lua_getvector(L, nres);

				break;
#endif

			default:
				CG_Printf("CG_RunLuaFunction: invalid option (%c)\n", *(sig - 1));
		}
		nres++;
	}
	va_end(vl);
}


/*
=================
CG_DumpLuaStack
=================
*/
void CG_DumpLuaStack()
{
	int             i;
	lua_State      *L = cg_luaState;
	int             top = lua_gettop(L);

	for(i = 1; i <= top; i++)
	{
		// repeat for each level
		int             t = lua_type(L, i);

		switch (t)
		{
			case LUA_TSTRING:
				// strings
				CG_Printf("`%s'", lua_tostring(L, i));
				break;

			case LUA_TBOOLEAN:
				// booleans
				CG_Printf(lua_toboolean(L, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:
				// numbers
				CG_Printf("%g", lua_tonumber(L, i));
				break;

			default:
				// other values
				CG_Printf("%s", lua_typename(L, t));
				break;

		}
		CG_Printf("  ");		// put a separator
	}
	CG_Printf("\n");			// end the listing
}


/*
=================
CG_RestartLua_f
=================
*/
void CG_RestartLua_f(void)
{
	CG_ShutdownLua();
	CG_InitLua();
}

#endif
