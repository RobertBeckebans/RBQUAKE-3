project "xreal-cgame"
	targetname  "cgame"
	targetdir 	"../../../../base"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/**.c",
		"../../../shared/q_shared.h",
		"../../../shared/cg_public.h",
		"../../../shared/tr_types.h",
		"../../../shared/keycodes.h",
		"../../../shared/surfaceflags.h",
		
		--"**.c", "**.cpp", "**.h",
		
		"cg_animation.c",
		"cg_consolecmds.c",
		"cg_draw.c",
		"cg_drawtools.c",
		"cg_effects.c",
		"cg_ents.c",
		"cg_event.c",
		"cg_info.c",
		"cg_local.h",
		"cg_localents.c",
		"cg_lua.c",
		"cg_main.c",
		"cg_marks.c",
		"cg_newdraw.c",
		"cg_particles.c",
		"cg_players.c",
		"cg_playerstate.c",
		"cg_predict.c",
		"cg_scoreboard.c",
		"cg_servercmds.c",
		"cg_snapshot.c",
		"cg_syscalls.c",
		"cg_view.c",
		"cg_weapons.c",
		
		"../ui/ui_parse.c",

		"lua_cgame.c",
		"lua_particle.c",

		"../game/bg_**.c", "../game/bg_**.cpp", "../game/bg_**.h",
		
		"../game/lua_qmath.c",
		"../game/lua_vector.c",
		
		"../../../libs/lua/src/lapi.c",
		"../../../libs/lua/src/lcode.c",
		"../../../libs/lua/src/ldebug.c",
		"../../../libs/lua/src/ldo.c",
		"../../../libs/lua/src/ldump.c",
		"../../../libs/lua/src/lfunc.c",
		"../../../libs/lua/src/lgc.c",
		"../../../libs/lua/src/llex.c",
		"../../../libs/lua/src/lmem.c",
		"../../../libs/lua/src/lobject.c",
		"../../../libs/lua/src/lopcodes.c",
		"../../../libs/lua/src/lparser.c",
		"../../../libs/lua/src/lstate.c",
		"../../../libs/lua/src/lstring.c",
		"../../../libs/lua/src/ltable.c",
		"../../../libs/lua/src/ltm.c",
		"../../../libs/lua/src/lundump.c",
		"../../../libs/lua/src/lvm.c",
		"../../../libs/lua/src/lzio.c",
		"../../../libs/lua/src/lauxlib.c",
		"../../../libs/lua/src/lbaselib.c",
		"../../../libs/lua/src/ldblib.c",
		"../../../libs/lua/src/liolib.c",
		"../../../libs/lua/src/lmathlib.c",
		"../../../libs/lua/src/ltablib.c",
		"../../../libs/lua/src/lstrlib.c",
		"../../../libs/lua/src/loadlib.c",
		"../../../libs/lua/src/linit.c",
		"../../../libs/lua/src/loslib.c",
	}
	includedirs
	{
		"../../../shared",
		"../../../libs/lua/src",
	}
	defines
	{ 
		"CGAME",
		"LUA",
	}

	--configuration "with-bullet"
	--	files
	--	{
	--		"cg_bullet.cpp",
	--	
	--		"../../../libs/bullet/*.h",
	--		"../../../libs/bullet/LinearMath/**.cpp", "../../../libs/bullet/LinearMath/**.h",
	--		"../../../libs/bullet/BulletCollision/**.cpp", "../../../libs/bullet/BulletCollision/**.h",
	--		"../../../libs/bullet/BulletDynamics/**.cpp", "../../../libs/bullet/BulletDynamics/**.h",
	--		"../../../libs/bullet/BulletSoftBody/**.cpp", "../../../libs/bullet/BulletSoftBody/**.h",
	--	}
	--	includedirs
	--	{
	--		"../../../code/libs/bullet"
	--	}
	--	defines
	--	{ 
	--		"USE_BULLET"
	--	}
	
	--
	-- Platform Configurations
	--
	configuration "x32"
		targetname  "cgamex86"
	
	configuration "x64"
		targetname  "cgamex86_64"
				
	-- 
	-- Project Configurations
	-- 
	configuration "vs*"
		linkoptions
		{
			"/DEF:cgame.def",
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
	
	configuration { "linux", "x32" }
		targetname  "cgamei386"
		targetprefix ""
	
	configuration { "linux", "x64" }
		targetname  "cgamex86_64"
		targetprefix ""
	
