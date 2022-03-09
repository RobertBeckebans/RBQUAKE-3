project "q3a-ui"
	targetname  "ui"
	targetdir 	"../../../../baseq3"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.c",
		"../../../shared/q_shared.h",
		"../../../shared/ui_public.h",
		"../../../shared/tr_types.h",
		"../../../shared/keycodes.h",
		"../../../shared/surfaceflags.h",
		
		"**.c", "**.cpp", "**.h",
		
		"../ui/**.c", "../ui/**.h",
		--"../game/bg_lib.c", "../game/bg_lib.h",
		"../game/bg_misc.c",
	}
	excludes
	{
		"ui_login.c",
		"ui_rankings.c",
		"ui_rankstatus.c",
		"ui_signup.c",
		"ui_specifyleague.c",
		
		-- ignore TA stuff
		-- "../ui/ui_local.h",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{ 
		"UI",
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
		targetname  "uix86"
	
	configuration "x64"
		targetname  "uix86_64"
				
	configuration "native"
		targetname  "uix86_64"
				
	-- 
	-- Project Configurations
	-- 
	configuration "vs*"
		linkoptions
		{
			"/DEF:ui.def",
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
	
	configuration { "linux", "x32" }
		targetname  "uii386"
		targetprefix ""
	
	configuration { "linux", "x64" }
		targetname  "uix86_64"
		targetprefix ""
	
