project "xreal-game"
	targetname  "game"
	targetdir 	"../../../../base"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/**.c",
		"../../../shared/q_shared.h",
		"../../../shared/g_public.h",
		"../../../shared/surfaceflags.h",
		
		--"**.c", "**.cpp", "**.h",
		"*.h",
		"bg_misc.c",
		"bg_pmove.c",
		"bg_slidemove.c",
		"g_active.c",
		"g_arenas.c",
		"g_bot.c",
		"g_client.c",
		"g_cmds.c",
		"g_combat.c",
		"g_items.c",
		"g_lua.c",
		"g_main.c",
		"g_mem.c",
		"g_misc.c",
		"g_missile.c",
		"g_mover.c",
		"g_session.c",
		"g_spawn.c",
		"g_svcmds.c",
		"g_syscalls.c",
		"g_target.c",
		"g_team.c",
		"g_trigger.c",
		"g_utils.c",
		"g_weapon.c",
		"lua_*.c",
		
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
	--excludes
	--{
	--	"g_rankings.c",
	--	"g_bullet.cpp",
	--}
	includedirs
	{
		"../../../shared",
		"../../../libs/lua/src",
	}
	defines
	{ 
		"QAGAME",
		"LUA"
	}
	
	configuration "with-bullet"
		files
		{
			"g_bullet.cpp",
		
			"../../../libs/bullet/*.h",
			"../../../libs/bullet/LinearMath/**.cpp", "../../../libs/bullet/LinearMath/**.h",
			"../../../libs/bullet/BulletCollision/**.cpp", "../../../libs/bullet/BulletCollision/**.h",
			"../../../libs/bullet/BulletDynamics/**.cpp", "../../../libs/bullet/BulletDynamics/**.h",
			"../../../libs/bullet/BulletSoftBody/**.cpp", "../../../libs/bullet/BulletSoftBody/**.h",
		}
		includedirs
		{
			"../../../libs/bullet"
		}
		defines
		{ 
			"USE_BULLET"
		}
		
	configuration "with-acebot"
		files
		{
			"acebot/**.c", "acebot/**.cpp", "acebot/**.h",
		}
		includedirs
		{
			"../game/acebot"
		}
		defines
		{ 
			"ACEBOT"
		}
		
	configuration "with-brainworks"
		files
		{
			"brainworks/**.c", "brainworks/**.cpp", "brainworks/**.h",
		}
		includedirs
		{
			"../game/brainworks"
		}
		defines
		{ 
			"BRAINWORKS"
		}		
	
	--
	-- Platform Configurations
	--
	configuration "x32"
		targetname  "qagamex86"
	
	configuration "x64"
		targetname  "qagamex86_64"
				
	-- 
	-- Project Configurations
	-- 
	configuration "vs*"
		linkoptions
		{
			"/DEF:game.def",
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
	
	configuration { "linux", "x32" }
		targetname  "qagamei386"
		targetprefix ""
	
	configuration { "linux", "x64" }
		targetname  "qagamex86_64"
		targetprefix ""

	