project "q3a-cgame"
	targetname  "cgame"
	targetdir 	"../../../../baseq3"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.c",
		"../../../shared/q_shared.h",
		"../../../shared/cg_public.h",
		"../../../shared/tr_types.h",
		"../../../shared/keycodes.h",
		"../../../shared/surfaceflags.h",
		
		--"**.c", "**.cpp", "**.h",
		
		"cg_consolecmds.c",
		"cg_draw.c",
		"cg_drawtools.c",
		"cg_effects.c",
		"cg_ents.c",
		"cg_event.c",
		"cg_info.c",
		"cg_local.h",
		"cg_localents.c",
		"cg_main.c",
		"cg_marks.c",
		--"cg_newdraw.c",
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
		
		--"../ui/ui_parse.c",

		"../game/bg_*.c",
		"../game/bg_*.cpp",
		"../game/bg_*.h",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{ 
		"CGAME",
	}
	
	configuration "standalone"
		defines
		{
			"STANDALONE"
		}
	
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
			--"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
	
	configuration { "linux", "x32" }
		targetname  "cgamei386"
		targetprefix ""
	
	configuration { "linux", "x64" }
		targetname  "cgamex86_64"
		targetprefix ""
	
