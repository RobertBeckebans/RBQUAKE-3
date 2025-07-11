--
-- RBQUAKE-3 build configuration script
-- 
solution "RBQUAKE-3-LEGACY"
	configurations { "Debug", "Profile", "Release" }
	platforms {"x32", "x64"}
	characterset ("ASCII")
	
	configuration "Debug"
		defines
		{
			"_DEBUG"
		}
		symbols "On"
		vectorextensions "SSE"
		warnings "Extra"
		
	configuration "Profile"
		defines
		{
			"NDEBUG",
		}
		symbols "On"
		vectorextensions "SSE"
		optimize "Speed"
		warnings "Extra"

	configuration "Release"
		defines
		{
			"NDEBUG"
		}
		symbols "Off"
		vectorextensions "SSE"
		optimize "Speed"
		warnings "Extra"
	
	configuration { "vs*" }
		targetdir ".."
		flags
		{
			"NoManifest",
			"NoMinimalRebuild",
			"No64BitChecks",
		}
		exceptionhandling "Off"
		editandcontinue "Off"
		systemversion "latest"
		buildoptions
		{
			-- multi processor support
			"/MP",
			
			-- warnings to ignore:
			-- "/wd4711", -- smells like old people
			
			-- warnings to force
			
			-- An accessor overrides, with or without the virtual keyword, a base class accessor function,
			-- but the override or new specifier was not part of the overriding function signature.
			"/we4485",
		}
		
	
	configuration { "vs*", "Debug" }
		buildoptions
		{
			-- turn off Smaller Type Check
			--"/RTC-",
		
			-- turn off Basic Runtime Checks
			--"/RTC1-",
		}
			
	configuration { "vs*", "Profile" }
		buildoptions
		{
			-- Produces a program database (PDB) that contains type information and symbolic debugging information for use with the debugger
			-- /Zi does imply /debug
			"/Zi",
			
			-- turn off Whole Program Optimization
			--"/GL-",
			
			-- Inline Function Expansion: Any Suitable (/Ob2)
			--"/Ob2",
			
			-- enable Intrinsic Functions
			"/Oi",
			
			-- Favor fast code
			"/Ot",
			
			-- Omit Frame Pointers - FIXME: maybe not for profile builds?
			"/Oy",
		}
		linkoptions
		{
			-- turn off Whole Program Optimization
			-- "/LTCG-",
			
			-- create .pdb file
			"/DEBUG",
		}
		
	configuration { "vs*", "Release" }
		buildoptions
		{
			-- turn off Whole Program Optimization
			--"/GL-",
			
			-- Inline Function Expansion: Any Suitable (/Ob2)
			"/Ob2",
			
			-- Favor fast code
			"/Ot",
			
			-- enable Intrinsic Functions
			"/Oi",
			
			-- Omit Frame Pointers
			"/Oy",
		}
	
--
-- Options
--
newoption
{
	trigger = "standalone",
	description = "Compile XreaL game code"
}

newoption
{
	trigger = "bullet",
	description = "Compile with Bullet physics game code support"
}

newoption
{
	trigger = "acebot",
	description = "Compile with AceBot game code support"
}

newoption
{
	trigger = "gladiator",
	description = "Compile with default Quake 3 gladiator bot support"
}

--newoption
--{
--	trigger = "with-freetype",
--	description = "Compile with freetype support"
--}
		
--newoption
--{
--	trigger = "with-openal",
--	value = "TYPE",
--	description = "Specify which OpenAL library",
--	allowed = 
--	{
--		{ "none", "No support for OpenAL" },
--		{ "dlopen", "Dynamically load OpenAL library if available" },
--		{ "link", "Link the OpenAL library as normal" },
--		{ "openal-dlopen", "Dynamically load OpenAL library if available" },
--		{ "openal-link", "Link the OpenAL library as normal" }
--	}
--}

--		
-- Platform specific defaults
--

-- Default to dlopen version of OpenAL
--if not _OPTIONS["with-openal"] then
--	_OPTIONS["with-openal"] = "dlopen"
--end
--if _OPTIONS["with-openal"] then
--	_OPTIONS["with-openal"] = "openal-" .. _OPTIONS["with-openal"]
--end

-- main engine code
project "RBQuake3"
	targetname  "RBQuake3"
	language    "C++"
	kind        "WindowedApp"
	files
	{
		"../shared/*.c", "../shared/*.h",
	
		"../engine/client/*.c", "../engine/client/*.h",
		"../engine/server/*.c", "../engine/server/*.h",
		
		"../engine/sound/*.c", "../engine/sound/*.h",
		
		"../engine/qcommon/**.h", 
		"../engine/qcommon/cmd.c",
		"../engine/qcommon/common.c",
		"../engine/qcommon/cvar.c",
		"../engine/qcommon/files.c",
		"../engine/qcommon/huffman.c",
		"../engine/qcommon/md4.c",
		"../engine/qcommon/md5.c",
		"../engine/qcommon/msg.c",
		"../engine/qcommon/vm.c",
		"../engine/qcommon/net_*.c",
		"../engine/qcommon/unzip.c",
		"../engine/qcommon/parse.c",  -- by Tremulous to avoid botlib dependency

		"../engine/collision/*.c", "../engine/collision/*.h",
		--"../engine/qcommon/cm_load.c",
		--"../engine/qcommon/cm_patch.c",
		--"../engine/qcommon/cm_polylib.c",
		--"../engine/qcommon/cm_test.c",
		--"../engine/qcommon/cm_trace.c",
		--"../engine/qcommon/cm_trisoup.c",
		
		"../engine/renderer/**.c", "../engine/renderer/**.cpp", "../engine/renderer/**.h",
		
		"../libs/gl3w/src/gl3w.c",
		"../libs/gl3w/include/GL3/gl3.h",
		"../libs/gl3w/include/GL3/gl3w.h",
		
		"../libs/jpeg/**.c", "../../libs/jpeg/**.h",
		"../libs/png/**.c", "../../libs/png/**.h",
		"../libs/zlib/**.c", "../../libs/zlib/**.h",
		"../libs/openexr/**.cpp", "../../libs/openexr/**.h",
		
		--"../libs/ft2/**.c", "../../libs/ft2/**.h",
		
		"../libs/freetype/src/autofit/autofit.c",
		"../libs/freetype/src/bdf/bdf.c",
		"../libs/freetype/src/cff/cff.c",
		"../libs/freetype/src/base/ftbase.c",
		"../libs/freetype/src/base/ftbitmap.c",
		"../libs/freetype/src/cache/ftcache.c",
		"../libs/freetype/src/base/ftdebug.c",
		"../libs/freetype/src/base/ftgasp.c",
		"../libs/freetype/src/base/ftglyph.c",
		"../libs/freetype/src/gzip/ftgzip.c",
		"../libs/freetype/src/base/ftinit.c",
		"../libs/freetype/src/lzw/ftlzw.c",
		"../libs/freetype/src/base/ftstroke.c",
		"../libs/freetype/src/base/ftsystem.c",
		"../libs/freetype/src/smooth/smooth.c",
		"../libs/freetype/src/base/ftbbox.c",
		"../libs/freetype/src/base/ftmm.c",
		"../libs/freetype/src/base/ftpfr.c",
		"../libs/freetype/src/base/ftsynth.c",
		"../libs/freetype/src/base/fttype1.c",
		"../libs/freetype/src/base/ftwinfnt.c",
		"../libs/freetype/src/pcf/pcf.c",
		"../libs/freetype/src/pfr/pfr.c",
		"../libs/freetype/src/psaux/psaux.c",
		"../libs/freetype/src/pshinter/pshinter.c",
		"../libs/freetype/src/psnames/psmodule.c",
		"../libs/freetype/src/raster/raster.c",
		"../libs/freetype/src/sfnt/sfnt.c",
		"../libs/freetype/src/truetype/truetype.c",
		"../libs/freetype/src/type1/type1.c",
		"../libs/freetype/src/cid/type1cid.c",
		"../libs/freetype/src/type42/type42.c",
		"../libs/freetype/src/winfonts/winfnt.c",
		
		"../libs/ogg/src/bitwise.c",
		"../libs/ogg/src/framing.c",
		
		"../libs/vorbis/lib/mdct.c",
		"../libs/vorbis/lib/smallft.c",
		"../libs/vorbis/lib/block.c",
		"../libs/vorbis/lib/envelope.c",
		"../libs/vorbis/lib/window.c",
		"../libs/vorbis/lib/lsp.c",
		"../libs/vorbis/lib/lpc.c",
		"../libs/vorbis/lib/analysis.c",
		"../libs/vorbis/lib/synthesis.c",
		"../libs/vorbis/lib/psy.c",
		"../libs/vorbis/lib/info.c",
		"../libs/vorbis/lib/floor1.c",
		"../libs/vorbis/lib/floor0.c",
		"../libs/vorbis/lib/res0.c",
		"../libs/vorbis/lib/mapping0.c",
		"../libs/vorbis/lib/registry.c",
		"../libs/vorbis/lib/codebook.c",
		"../libs/vorbis/lib/sharedbook.c",
		"../libs/vorbis/lib/lookup.c",
		"../libs/vorbis/lib/bitrate.c",
		"../libs/vorbis/lib/vorbisfile.c",
		
		-- "../libs/speex/bits.c",
		-- "../libs/speex/buffer.c",
		-- "../libs/speex/cb_search.c",
		-- "../libs/speex/exc_10_16_table.c",
		-- "../libs/speex/exc_10_32_table.c",
		-- "../libs/speex/exc_20_32_table.c",
		-- "../libs/speex/exc_5_256_table.c",
		-- "../libs/speex/exc_5_64_table.c",
		-- "../libs/speex/exc_8_128_table.c",
		-- "../libs/speex/fftwrap.c",
		-- "../libs/speex/filterbank.c",
		-- "../libs/speex/filters.c",
		-- "../libs/speex/gain_table.c",
		-- "../libs/speex/gain_table_lbr.c",
		-- "../libs/speex/hexc_10_32_table.c",
		-- "../libs/speex/hexc_table.c",
		-- "../libs/speex/high_lsp_tables.c",
		-- "../libs/speex/jitter.c",
		-- "../libs/speex/kiss_fft.c",
		-- "../libs/speex/kiss_fftr.c",
		-- "../libs/speex/lsp_tables_nb.c",
		-- "../libs/speex/ltp.c",
		-- "../libs/speex/mdf.c",
		-- "../libs/speex/modes.c",
		-- "../libs/speex/modes_wb.c",
		-- "../libs/speex/nb_celp.c",
		-- "../libs/speex/preprocess.c",
		-- "../libs/speex/quant_lsp.c",
		-- "../libs/speex/resample.c",
		-- "../libs/speex/sb_celp.c",
		-- "../libs/speex/speex_smallft.c",
		-- "../libs/speex/speex.c",
		-- "../libs/speex/speex_callbacks.c",
		-- "../libs/speex/speex_header.c",
		-- "../libs/speex/speex_lpc.c",
		-- "../libs/speex/speex_lsp.c",
		-- "../libs/speex/speex_window.c",
		-- "../libs/speex/vbr.c",
		-- "../libs/speex/stereo.c",
		-- "../libs/speex/vq.c",
		
		"../libs/theora/lib/dec/apiwrapper.c",
		"../libs/theora/lib/dec/bitpack.c",
		"../libs/theora/lib/dec/decapiwrapper.c",
		"../libs/theora/lib/dec/decinfo.c",
		"../libs/theora/lib/dec/decode.c",
		"../libs/theora/lib/dec/dequant.c",
		"../libs/theora/lib/dec/fragment.c",
		"../libs/theora/lib/dec/huffdec.c",
		"../libs/theora/lib/dec/idct.c",
		"../libs/theora/lib/dec/thinfo.c",
		"../libs/theora/lib/dec/internal.c",
		"../libs/theora/lib/dec/quant.c",
		"../libs/theora/lib/dec/state.c",
	}
	includedirs
	{
		"../shared",
		"../libs/zlib",
		"../libs/gl3w/include",
		"../libs/freetype/include",
		"../libs/ogg/include",
		"../libs/vorbis/include",
		"../libs/theora/include",
		"../libs/speex/include",
	}
	defines
	{ 
		--"STANDALONE",
		"REF_HARD_LINKED",
		"GLEW_STATIC",
		"BUILD_FREETYPE",
		"FT2_BUILD_LIBRARY",
		"USE_CODEC_VORBIS",
		--"USE_VOIP",
		"USE_CIN_THEORA",
		"USE_ALLOCA",
		"FLOATING_POINT",
		--"USE_CURL", 
		--"USE_MUMBLE",
		--"USE_INTERNAL_GLFW",
		--"USE_INTERNAL_GLEW",
	}
	excludes
	{
		"../engine/server/sv_rankings.c",
		"../engine/renderer/tr_animation_mdm.c",
		"../engine/renderer/tr_model_mdm.c",
	}
	
	--
	-- Platform Configurations
	-- 	
	configuration "x32"
		files
		{ 
			--"code/qcommon/vm_x86.c",
			"../libs/theora/lib/dec/x86/mmxidct.c",
			"../libs/theora/lib/dec/x86/mmxfrag.c",
			"../libs/theora/lib/dec/x86/mmxstate.c",
			"../libs/theora/lib/dec/x86/x86state.c"
		}
	
	configuration "x64"
		--targetdir 	"../../bin64"
		files
		{ 
			--"qcommon/vm_x86_64.c",
			--"qcommon/vm_x86_64_assembler.c",
			"../libs/theora/lib/dec/x86/mmxidct.c",
			"../libs/theora/lib/dec/x86/mmxfrag.c",
			"../libs/theora/lib/dec/x86/mmxstate.c",
			"../libs/theora/lib/dec/x86/x86state.c"
		}
		
	--
	-- Options Configurations
	--
	configuration "standalone"
		defines
		{
			"STANDALONE"
		}
		
	configuration "gladiator"
		defines
		{
			"BOTLIB",
			"GLADIATOR",
		}
		includedirs
		{
			"../shared/botshared",
		}
		files
		{
			"../engine/botlib/*.c", "../engine/botlib/*.h",
			"../shared/botshared/*.c", "../shared/botshared/*.h",
		}
	
	configuration "bullet"
		defines
		{
			"USE_BULLET"
		}
		includedirs
		{
			"../libs/bullet"
		}
		files
		{
			"../qcommon/cm_bullet.cpp",
		
			"../libs/bullet/*.h",
			"../libs/bullet/LinearMath/**.cpp", "../libs/bullet/LinearMath/**.h",
			"../libs/bullet/BulletCollision/**.cpp", "../libs/bullet/BulletCollision/**.h",
			"../libs/bullet/BulletDynamics/**.cpp", "../libs/bullet/BulletDynamics/**.h",
			"../libs/bullet/BulletSoftBody/**.cpp", "../libs/bullet/BulletSoftBody/**.h",
		}
	
	--configuration "with-freetype"
	--	links        { "freetype" }
	--	buildoptions { "`pkg-config --cflags freetype2`" }
	--	defines      { "BUILD_FREETYPE" }

	--configuration "openal-dlopen"
	--	defines      
	--	{
	--		"USE_OPENAL",
	--		"USE_OPENAL_DLOPEN",
	--		"USE_OPENAL_LOCAL_HEADERS"
	--	}
		
	--configuration "openal-link"
	--	links        { "openal " }
	--	defines      { "USE_OPENAL" }

	--configuration { "vs*", "Release" }
	-- newaction {
		-- trigger = "prebuild",
		-- description = "Compile libcurl.lib",
		-- execute = function ()
			-- os.execute("cd ../libs/curl-7.12.2;cd lib;nmake /f Makefile.vc6 CFG=release")
		-- end
	-- }
	
	-- 
	-- Project Configurations
	-- 
	configuration "vs*"
		flags       { "WinMain" }
		files
		{
			"../engine/sys/sys_main.c",
			"../engine/sys/sys_win32.c",
			"../engine/sys/con_log.c",
			"../engine/sys/con_win32.c",
			"../engine/sys/sdl_gamma.c",
			"../engine/sys/sdl_glimp.c",
			"../engine/sys/sdl_input.c",
			"../engine/sys/sdl_snd.c",
			
			"../engine/sys/qe3.ico",
			"../engine/sys/win_resource.rc",
		}
		defines
		{
			"USE_OPENAL",
		}
		includedirs
		{
			"../libs/sdl2/include",
			"../libs/openal/include",
		}
		libdirs
		{
			--"../libs/curl-7.12.2/lib"
		}
		
		links
		{
			"SDL2",
			"SDL2main",
			"winmm",
			"wsock32",
			"opengl32",
			"user32",
			"advapi32",
			"ws2_32",
			"Psapi"
		}
		buildoptions
		{
			--"/MT"
		}
		linkoptions 
		{
			"/LARGEADDRESSAWARE",
			--"/NODEFAULTLIB:libcmt.lib",
			--"/NODEFAULTLIB:libcmtd.lib"
			--"/NODEFAULTLIB:libc"
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
		
		
	configuration { "vs*", "x32" }
		targetdir 	"../.."
		libdirs
		{
			"../libs/sdl2/lib/x86",
			"../libs/openal/libs/win32",
			--"../libs/curl-7.12.2/lib"
		}
		links
		{
			--"libcurl",
			"OpenAL32",
		}
		
	configuration { "vs*", "x64" }
		targetdir 	"../.."
		libdirs
		{
			"../libs/sdl2/lib/x64",
			"../libs/openal/libs/win64",
			--"../libs/curl-7.12.2/lib"
		}
		links
		{
			--"libcurl",
			"OpenAL32",
		}

	configuration { "linux", "gmake" }
		buildoptions
		{
			"`pkg-config --cflags sdl2`",
			"`pkg-config --cflags libcurl`",
		}
		linkoptions
		{
			"`pkg-config --libs sdl2`",
			"`pkg-config --libs libcurl`",
		}
		links
		{
			--"libcurl",
			"openal",
		}
	
	configuration { "linux", "x32" }
		targetdir 	"../bin/linux-x86"
		
	configuration { "linux", "x64" }
		targetdir 	"../bin/linux-x86_64"
	
	configuration { "linux", "native" }
		targetdir 	"../bin/linux-native"
	
	configuration "linux"
		targetname  "xreal"
		files
		{
			"../engine/sys/sys_main.c",
			"../engine/sys/sys_unix.c",
			"../engine/sys/con_log.c",
			"../engine/sys/con_passive.c",
			"../engine/sys/sdl_gamma.c",
			"../engine/sys/sdl_glimp.c",
			"../engine/sys/sdl_input.c",
			"../engine/sys/sdl_snd.c",
		}
		--buildoptions
		--{
		--	"-pthread"
		--}
		links
		{
			"GL",
		}
		defines
		{
            "PNG_NO_ASSEMBLER_CODE",
		}

-- Quake 3 game mod code based on ioq3
if not _OPTIONS["standalone"] then
include "../games/default/game"
include "../games/default/cgame"
include "../games/default/q3_ui"
end

if _OPTIONS["standalone"] then
include "../games/xreal/game"
include "../games/xreal/cgame"
include "../games/xreal/ui"
end

-- tools
--include "code/tools/xmap2"
--include "code/tools/master"