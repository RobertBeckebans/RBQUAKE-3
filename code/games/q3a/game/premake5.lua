project "q3a-game"
	targetname  "game"
	targetdir 	"../../../../baseq3"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.c",
		"../../../shared/q_shared.h",
		"../../../shared/g_public.h",
		"../../../shared/surfaceflags.h",
		
		"**.c", "**.cpp", "**.h",
		-- "*.h",
		-- "bg_misc.c",
		-- "bg_pmove.c",
		-- "bg_slidemove.c",
		-- "g_active.c",
		-- "g_arenas.c",
		-- "g_bot.c",
		-- "g_client.c",
		-- "g_cmds.c",
		-- "g_combat.c",
		-- "g_items.c",
		-- "g_lua.c",
		-- "g_main.c",
		-- "g_mem.c",
		-- "g_misc.c",
		-- "g_missile.c",
		-- "g_mover.c",
		-- "g_session.c",
		-- "g_spawn.c",
		-- "g_svcmds.c",
		-- "g_syscalls.c",
		-- "g_target.c",
		-- "g_team.c",
		-- "g_trigger.c",
		-- "g_utils.c",
		-- "g_weapon.c",
	}
	excludes
	{
		"g_rankings.c",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{ 
		"QAGAME",
	}
	
	configuration "gladiator"
		defines
		{
			"BOTLIB",
			"GLADIATOR",
		}
		
	configuration "brainworks"
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
			"BOTLIB",
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
			--"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
	
	configuration { "linux", "x32" }
		targetname  "qagamei386"
		targetprefix ""
	
	configuration { "linux", "x64" }
		targetname  "qagamex86_64"
		targetprefix ""

	