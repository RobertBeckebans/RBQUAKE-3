project "xreal-ui"
	targetname  "ui"
	targetdir 	"../../../../base"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/**.c",
		"../../../shared/q_shared.h",
		"../../../shared/ui_public.h",
		"../../../shared/tr_types.h",
		"../../../shared/keycodes.h",
		"../../../shared/surfaceflags.h",
		
		"**.c", "**.cpp", "**.h",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{ 
		"UI",
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
	
