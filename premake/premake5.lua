--
-- RBQUAKE-3 build configuration script
-- 
solution "RBQUAKE-3"
	configurations { "Debug", "Profile", "Release" }
	platforms {"x32", "x64"}
	
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
	trigger = "with-bullet",
	description = "Compile with Bullet physics game code support"
}

newoption
{
	trigger = "with-acebot",
	description = "Compile with AceBot game code support"
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
		"../code/shared/**.c", "../code/shared/**.h",
	
		"../code/engine/client/**.c", "../code/engine/client/**.h",
		"../code/engine/server/**.c", "../code/engine/server/**.h",
		
		"../code/engine/qcommon/**.h", 
		"../code/engine/qcommon/cmd.c",
		"../code/engine/qcommon/common.c",
		"../code/engine/qcommon/cvar.c",
		"../code/engine/qcommon/files.c",
		"../code/engine/qcommon/huffman.c",
		"../code/engine/qcommon/md4.c",
		"../code/engine/qcommon/md5.c",
		"../code/engine/qcommon/msg.c",
		"../code/engine/qcommon/vm.c",
		"../code/engine/qcommon/net_*.c",
		"../code/engine/qcommon/unzip.c",

		"../code/engine/qcommon/cm_load.c",
		"../code/engine/qcommon/cm_patch.c",
		"../code/engine/qcommon/cm_polylib.c",
		"../code/engine/qcommon/cm_test.c",
		"../code/engine/qcommon/cm_trace.c",
		"../code/engine/qcommon/cm_trisoup.c",
		
		"../code/engine/renderer/**.c", "../code/engine/renderer/**.cpp", "../code/engine/renderer/**.h",
		
		"../code/libs/gl3w/src/gl3w.c",
		"../code/libs/gl3w/include/GL3/gl3.h",
		"../code/libs/gl3w/include/GL3/gl3w.h",
		
		"../code/libs/jpeg/**.c", "../code/../libs/jpeg/**.h",
		"../code/libs/png/**.c", "../code/../libs/png/**.h",
		"../code/libs/zlib/**.c", "../code/../libs/zlib/**.h",
		"../code/libs/openexr/**.cpp", "../code/../libs/openexr/**.h",
		
		--"../code/libs/ft2/**.c", "../code/../libs/ft2/**.h",
		
		"../code/libs/freetype/src/autofit/autofit.c",
		"../code/libs/freetype/src/bdf/bdf.c",
		"../code/libs/freetype/src/cff/cff.c",
		"../code/libs/freetype/src/base/ftbase.c",
		"../code/libs/freetype/src/base/ftbitmap.c",
		"../code/libs/freetype/src/cache/ftcache.c",
		"../code/libs/freetype/src/base/ftdebug.c",
		"../code/libs/freetype/src/base/ftgasp.c",
		"../code/libs/freetype/src/base/ftglyph.c",
		"../code/libs/freetype/src/gzip/ftgzip.c",
		"../code/libs/freetype/src/base/ftinit.c",
		"../code/libs/freetype/src/lzw/ftlzw.c",
		"../code/libs/freetype/src/base/ftstroke.c",
		"../code/libs/freetype/src/base/ftsystem.c",
		"../code/libs/freetype/src/smooth/smooth.c",
		"../code/libs/freetype/src/base/ftbbox.c",
		"../code/libs/freetype/src/base/ftmm.c",
		"../code/libs/freetype/src/base/ftpfr.c",
		"../code/libs/freetype/src/base/ftsynth.c",
		"../code/libs/freetype/src/base/fttype1.c",
		"../code/libs/freetype/src/base/ftwinfnt.c",
		"../code/libs/freetype/src/pcf/pcf.c",
		"../code/libs/freetype/src/pfr/pfr.c",
		"../code/libs/freetype/src/psaux/psaux.c",
		"../code/libs/freetype/src/pshinter/pshinter.c",
		"../code/libs/freetype/src/psnames/psmodule.c",
		"../code/libs/freetype/src/raster/raster.c",
		"../code/libs/freetype/src/sfnt/sfnt.c",
		"../code/libs/freetype/src/truetype/truetype.c",
		"../code/libs/freetype/src/type1/type1.c",
		"../code/libs/freetype/src/cid/type1cid.c",
		"../code/libs/freetype/src/type42/type42.c",
		"../code/libs/freetype/src/winfonts/winfnt.c",
		
		"../code/libs/ogg/src/bitwise.c",
		"../code/libs/ogg/src/framing.c",
		
		"../code/libs/vorbis/lib/mdct.c",
		"../code/libs/vorbis/lib/smallft.c",
		"../code/libs/vorbis/lib/block.c",
		"../code/libs/vorbis/lib/envelope.c",
		"../code/libs/vorbis/lib/window.c",
		"../code/libs/vorbis/lib/lsp.c",
		"../code/libs/vorbis/lib/lpc.c",
		"../code/libs/vorbis/lib/analysis.c",
		"../code/libs/vorbis/lib/synthesis.c",
		"../code/libs/vorbis/lib/psy.c",
		"../code/libs/vorbis/lib/info.c",
		"../code/libs/vorbis/lib/floor1.c",
		"../code/libs/vorbis/lib/floor0.c",
		"../code/libs/vorbis/lib/res0.c",
		"../code/libs/vorbis/lib/mapping0.c",
		"../code/libs/vorbis/lib/registry.c",
		"../code/libs/vorbis/lib/codebook.c",
		"../code/libs/vorbis/lib/sharedbook.c",
		"../code/libs/vorbis/lib/lookup.c",
		"../code/libs/vorbis/lib/bitrate.c",
		"../code/libs/vorbis/lib/vorbisfile.c",
		
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
		
		"../code/libs/theora/lib/dec/apiwrapper.c",
		"../code/libs/theora/lib/dec/bitpack.c",
		"../code/libs/theora/lib/dec/decapiwrapper.c",
		"../code/libs/theora/lib/dec/decinfo.c",
		"../code/libs/theora/lib/dec/decode.c",
		"../code/libs/theora/lib/dec/dequant.c",
		"../code/libs/theora/lib/dec/fragment.c",
		"../code/libs/theora/lib/dec/huffdec.c",
		"../code/libs/theora/lib/dec/idct.c",
		"../code/libs/theora/lib/dec/thinfo.c",
		"../code/libs/theora/lib/dec/internal.c",
		"../code/libs/theora/lib/dec/quant.c",
		"../code/libs/theora/lib/dec/state.c",
	}
	includedirs
	{
		"../code/shared",
		"../code/libs/zlib",
		"../code/libs/gl3w/include",
		"../code/libs/freetype/include",
		"../code/libs/ogg/include",
		"../code/libs/vorbis/include",
		"../code/libs/theora/include",
		"../code/libs/speex/include",
	}
	defines
	{ 
		"STANDALONE",
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
		"../code/engine/server/sv_rankings.c",
		"../code/engine/renderer/tr_animation_mdm.c",
		"../code/engine/renderer/tr_model_mdm.c",
	}
	
	--
	-- Platform Configurations
	-- 	
	configuration "x32"
		files
		{ 
			--"code/qcommon/vm_x86.c",
			"../code/libs/theora/lib/dec/x86/mmxidct.c",
			"../code/libs/theora/lib/dec/x86/mmxfrag.c",
			"../code/libs/theora/lib/dec/x86/mmxstate.c",
			"../code/libs/theora/lib/dec/x86/x86state.c"
		}
	
	configuration "x64"
		--targetdir 	"../../bin64"
		files
		{ 
			--"qcommon/vm_x86_64.c",
			--"qcommon/vm_x86_64_assembler.c",
			"../code/libs/theora/lib/dec/x86/mmxidct.c",
			"../code/libs/theora/lib/dec/x86/mmxfrag.c",
			"../code/libs/theora/lib/dec/x86/mmxstate.c",
			"../code/libs/theora/lib/dec/x86/x86state.c"
		}
		
	--
	-- Options Configurations
	--
	configuration "with-bullet"
		defines
		{
			"USE_BULLET"
		}
		includedirs
		{
			"../code/libs/bullet"
		}
		files
		{
			"../code/qcommon/cm_bullet.cpp",
		
			"../code/libs/bullet/*.h",
			"../code/libs/bullet/LinearMath/**.cpp", "../code/libs/bullet/LinearMath/**.h",
			"../code/libs/bullet/BulletCollision/**.cpp", "../code/libs/bullet/BulletCollision/**.h",
			"../code/libs/bullet/BulletDynamics/**.cpp", "../code/libs/bullet/BulletDynamics/**.h",
			"../code/libs/bullet/BulletSoftBody/**.cpp", "../code/libs/bullet/BulletSoftBody/**.h",
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
			"../code/engine/sys/sys_main.c",
			"../code/engine/sys/sys_win32.c",
			"../code/engine/sys/con_log.c",
			"../code/engine/sys/con_win32.c",
			"../code/engine/sys/sdl_gamma.c",
			"../code/engine/sys/sdl_glimp.c",
			"../code/engine/sys/sdl_input.c",
			"../code/engine/sys/sdl_snd.c",
			
			"../code/engine/sys/qe3.ico",
			"../code/engine/sys/win_resource.rc",
		}
		defines
		{
			"USE_OPENAL",
		}
		includedirs
		{
			"../code/libs/sdl2/include",
			"../code/libs/openal/include",
		}
		libdirs
		{
			--"../code/libs/curl-7.12.2/lib"
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
		targetdir 	".."
		libdirs
		{
			"../code/libs/sdl2/lib/x86",
			"../code/libs/openal/libs/win32",
			--"../libs/curl-7.12.2/lib"
		}
		links
		{
			--"libcurl",
			"OpenAL32",
		}
		
	configuration { "vs*", "x64" }
		targetdir 	".."
		libdirs
		{
			"../code/libs/sdl2/lib/x64",
			"../code/libs/openal/libs/win64",
			--"../code/libs/curl-7.12.2/lib"
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
			"../code/engine/sys/sys_main.c",
			"../code/engine/sys/sys_unix.c",
			"../code/engine/sys/con_log.c",
			"../code/engine/sys/con_passive.c",
			"../code/engine/sys/sdl_gamma.c",
			"../code/engine/sys/sdl_glimp.c",
			"../code/engine/sys/sdl_input.c",
			"../code/engine/sys/sdl_snd.c",
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

-- game mod code
include "../code/games/xreal/game"
include "../code/games/xreal/cgame"
include "../code/games/xreal/ui"

-- tools
--include "code/tools/xmap2"
--include "code/tools/master"