
____  ___                     .____     
\   \/  /______   ____ _____  |    |    
 \     /\_  __ \_/ __ \\__  \ |    |    
 /     \ |  | \/\  ___/ / __ \|    |___ 
/___/\  \|__|    \___  >____  /_______ \
      \_/            \/     \/        \/

_________________________________________


XreaL Readme - http://sourceforge.net/projects/xreal

Thank you for downloading XreaL.



_______________________________________

CONTENTS
_______________________________



This file contains the following sections:

	1) SYSTEM REQUIREMENTS

	2) LICENSE

	3) GENERAL NOTES
	
	4) GETTING THE SOURCE CODE AND MEDIA

	5) COMPILING ON WIN32 WITH VISUAL C++ 2010 EXPRESS EDITION

	6) COMPILING ON GNU/LINUX
	
	7) CHANGES
	
	8) FEATURES
	
	9) CONSOLE VARIABLES
	
	10) KNOWN ISSUES
	
	11) BUG REPORTS
	
	12) USING HTTP/FTP DOWNLOAD SUPPORT (SERVER)
	
	13) USING HTTP/FTP DOWNLOAD SUPPORT (CLIENT)
	
	14) MULTIUSER SUPPORT ON WINDOWS SYSTEMS
	
	15) CONTRIBUTIONS
	
	16) DONATIONS



___________________________________

1) SYSTEM REQUIREMENTS
__________________________



Minimum system requirements:

	CPU: 2 GHz Intel compatible
	System Memory: 512MB
	Graphics card: Any graphics card that supports Direct3D 10 and OpenGL >= 3.2

Recommended system requirements:

	CPU: 3 GHz + Intel compatible
	System Memory: 1024MB+
	Graphics card: Geforce 8800 GT, ATI HD 4850 or higher. 




_______________________________

2) LICENSE
______________________

See COPYING.txt for all the legal stuff.



_______________________________

3) GENERAL NOTES
______________________

A short summary of the file layout:

XreaL/base/					XreaL media directory ( models, textures, sounds, maps, etc. )
XreaL/base/code				XreaL game code ( game, cgame, and ui )
XreaL/blender/				Blender plugins for ase, md3, and md5 models
XreaL/code/					XreaL source code ( renderer, game code, OS layer, etc. )
XreaL/code/tools/common		Framework source code for command line tools like xmap
XreaL/code/tools/xmap2		Map compiler ( .map -> .bsp ) (based on q3map2)
XreaL/code/tools/master		Master server
XreaL/darkradiant/			DarkRadiant level editor work dir with configuration files



____________________________________________

4) GETTING THE SOURCE CODE AND MEDIA
___________________________________

This project's SourceForge.net Subversion repository can be checked out through SVN with the following instruction set: 

svn co https://xreal.svn.sourceforge.net/svnroot/xreal/trunk/xreal XreaL



___________________________________________________________________

5) COMPILING ON WIN32 WITH VISUAL C++ 2010 EXPRESS EDITION
__________________________________________________________

1. Download and install the Visual C++ 2010 Express Edition.

2. Generate the VC10 projects using Premake:

	> premake4.exe vs2010

3. Use the VC10 solution to compile what you need:
	XreaL/XreaL.sln
	


__________________________________

6) COMPILING ON GNU/LINUX
_________________________


1. You need the following dependencies in order to compile XreaL with all features:
 
	On Debian or Ubuntu:

		> apt-get install libcurl4-openssl-dev libsdl1.2-dev libopenal-dev libglu1-mesa-dev
	
	On Fedora

		> yum install SDL-devel openal-devel mesa-libGLU-devel


2. Download and extract Premake 4.x to the XreaL/ root directory or install it using your
	Linux distribution's package system.

3. Generate the Makefiles using Premake:

	> ./premake4 gmake 
	
4. Compile XreaL targets with

	> make

If you want to build for x86_64 then type:

	> make config=release64


Type "./premake4 --help" or "make help" for more compile options.


___________________________________________________

7) CHANGES
__________________________________________

See CHANGELOG.txt for full list of all changes.

	
___________________________________________________

8) General XreaL id Tech 3 Features
__________________________________________
	
XreaL
	* Modern OpenGL 3.2 renderer with all deprecated OpenGL calls removed
	* Clever usage of vertex buffer objects (VBO) to speed up rendering of everything
	* Avoids geometry processing each frame using the CPU (worst bottleneck with the Q3A engine)
	* Renders up to 500 000 - 1 000 000 polygons at 80 - 200 fps on current hardware (DX10 generation)
	* Optional GPU occlusion culling (improved Coherent Hierarchy Culling) useful for rendering large city scenes
	* Doom 3 .MD5mesh/.MD5anim skeletal model and animation support
	* Unreal Actor X .PSK/.PSA skeletal model and animation support
	* True 64 bit HDR lighting with adaptive tone mapping
	* Advanced projective and omni-directional soft shadow mapping methods like EVSM
	* Real-time sun lights with parallel-split shadow maps
	* Optional deferred shading
	* Relief mapping that can be enabled by materials
	* Optional uniform lighting and shadowing model like in Doom 3 including globe mapping
	* Supports almost all Quake 3, Enemy Territory and Doom 3 material shader keywords
	* TGA, PNG, JPG and DDS format support for textures
	* Usage of frame buffer objects (FBO) to perform offscreen rendering effects
	* Improved TrueType font support that does not require external tools
	* Linux 64-bit support
	* Linux sound backend using SDL
	* .avi recorder from ioquake3 including sound support
	* Optimized collision detection routines
	* Support for Omni-bot

XMap2
	* Based on q3map2 by Randy 'ydnar' Reddig including additional fixes by the NetRadiant edition
	* Supports Doom 3 and Quake 4 .map formats
	* Built-in mini BSP viewer using -draw
	

___________________________________________________

9) CONSOLE VARIABLES
__________________________________________


r_mode							Sets the window or fullscreen resolution. ET:XreaL has more entries than the original engine.
								0 = 320x240
								1 = 400x300
								2 = 512x384
								3 = 640x480
								4 = 800x600
								5 = 960x720
								6 = 1024x768
								7 = 1152x864
								8 = 1280x720 (16:9)
								9 = 1280x768 (16:10)
								10 = 1280x800 (16:10)
								11 = 1280x1024
								12 = 1360x768 (16:9)
								13 = 1440x900 (16:10)
								14 = 1680x1050 (16:10)
								15 = 1600x1200
								16 = 1920x1080 (16:9)
								17 = 1920x1200 (16:10)
								18 = 2048x1536
								19 = 2560x1600 (16:10)


cg_shadows						Sets the shadows quality (higher value -> more expensive and better quality)
								0 = Off
								1 = Blob shadow
								2 = Exponential Shadow Mapping (16-bit quality)
								3 = Exponential Shadow Mapping (32-bit quality)
								4 = Variance Shadow Mapping (16-bit quality)
								5 = Variance Shadow Mapping (32-bit quality)
								6 = Exponential Variance Shadow Mapping (32-bit quality)
								

r_dynamicLight					Enable dynamic lights
r_dynamicLightCastShadows		Enable dynamic lights to cast shadows with all interacting surfaces (expensive)

r_vboVertexSkinning				Enables skeletal animation rendering on the GPU
								Pros:
									- Can be much faster with Nvidia cards
								Cons:
									- Can be slower than the CPU path on ATI cards

r_normalMapping					Enables bump mapping
								0 = Disables it and allows faster ET style render mode
								1 = Enables it with simple Blinn-Phong specular lighting
								
r_parallaxMapping				Enables Relief Mapping if materials support it
									
				
r_cameraPostFX					Enable camera postprocessing effects like filmgrain
r_cameraFilmGrain				Enable camera film grain simulation
r_cameraFilmGrainScale			Set the grain scale
r_cameraVignette				Enable vignetting postprocessing effect

r_bloom							Enable bloom postprocessing effect
r_bloomPasses					Number of times the blur filter is applied
r_bloomBlur						Amount to scale the X anx Y axis sample offsets


r_hdrRendering					Enable High Dynamic Range lighting (experimental)

// ----------------------------------------------------------------------------
HDR variables that are cheat protected but might be interesting for some people

r_hdrKey						Middle gray value used in HDR tone mapping
								0 computes it dynamically
								0.72 default

r_hdrMinLuminance				Minimum luminance value threshold
r_hdrMaxLuminance				Maximum luminance value threshold

r_hdrDebug						Shows min, max and average luminance detected by the scene input

// ----------------------------------------------------------------------------

r_deferredShading				(experimental)
								0 = Renders dynamic lights using Forward Shading like in Doom 3 (default)
								1 = Renders dynamic lights using Deferred Shading like in S.T.A.L.K.E.R.
								Pros:
									- It can render hundreds of dynamic lights that don't cast shadows really fast
									- It stabilizes the fps with heavy weapon fire
								Cons:
									- It lowers the fps if there are no lights because the first depth pass is more expensive
									- It's not compatible with all default ET shader files yet
									

// ----------------------------------------------------------------------------
Variables interesting for developers and mappers

r_showTris						Shows all fast GPU path triangles blue and slow CPU path triangles red
								Blue triangles indicate that those batches don't have to be moved through the PCIe bridge.
								All required geometry and shaders are already available in the GPU memory.
								
								Red triangles indicate that some geometry has to be processed by the CPU before it can be rendered.
								This usually happens with deformVertexes shader commands.
								
r_showBatches					Draws all batches (geometry, material and lightmap combination) as individual colors.
								General rule: more batches -> less performance
								Avoid many many tiny lightmaps like in the TCE mod and rather use lightmaps with 2048^2 for better batching.
								
r_showLightMaps					Draw all lightmaps (requires glsl_restart)
								
r_showDeluxeMaps				Draw all directional lightmaps (requires glsl_restart)

r_showLightGrid					Draws all lightgrid points with color and direction.



// ----------------------------------------------------------------------------



___________________________________________________

10) KNOWN ISSUES
__________________________________________

	* Light bleeding problems with cg_shadows 4 - 5 which are typical for variance shadow mapping

___________________________________________________

11) BUG REPORTS
__________________________________________

XreaL is not perfect, it is not bug free as every other software.
For fixing as much problems as possible we need as much bug reports as possible.
We cannot fix anything if we do not know about the problems.

The best way for telling us about a bug is by submitting a bug report at our SourceForge bug tracker page:

	http://sourceforge.net/tracker/?group_id=27204&atid=389772

The most important fact about this tracker is that we cannot simply forget to fix the bugs which are posted there. 
It is also a great way to keep track of fixed stuff.

If you want to report an issue with the game, you should make sure that your report includes all information useful to characterize and reproduce the bug.

    * Search on Google
    * Include the computer's hardware and software description ( CPU, RAM, 3D Card, distribution, kernel etc. )
    * If appropriate, send a console log, a screenshot, an strace ..
    * If you are sending a console log, make sure to enable developer output:

              XreaL.exe +set developer 1 +set logfile 2

NOTE: We cannot help you with OS-specific issues like configuring OpenGL correctly, configuring ALSA or configuring the network.
	

	
___________________________________________________



__________________________________________________________

12) USING HTTP/FTP DOWNLOAD SUPPORT (SERVER)
______________________________________________

You can enable redirected downloads on your server by using the 'sets'
command to put the sv_dlURL cvar into your SERVERINFO string and
ensure sv_allowDownloads is set to 1.
 
sv_dlURL is the base of the URL that contains your custom .pk3 files
the client will append both fs_game and the filename to the end of
this value.  For example, if you have sv_dlURL set to
"http://xreal.sourceforge.net/", fs_game is "base", and the client is
missing "test.pk3", it will attempt to download from the URL
"http://xreal.sourceforge.net/base/test.pk3"

sv_allowDownload's value is now a bitmask made up of the following
flags:
    1 - ENABLE
    2 - do not use HTTP/FTP downloads
    4 - do not use UDP downloads
    8 - do not ask the client to disconnect when using HTTP/FTP

Server operators who are concerned about potential "leeching" from their
HTTP servers from other XreaL servers can make use of the HTTP_REFERER
that XreaL sets which is "XreaL://{SERVER_IP}:{SERVER_PORT}".  For,
example, Apache's mod_rewrite can restrict access based on HTTP_REFERER.


________________________________________________________

13) USING HTTP/FTP DOWNLOAD SUPPORT (CLIENT)
_____________________________________________

Simply setting cl_allowDownload to 1 will enable HTTP/FTP downloads on 
the clients side assuming XreaL was compiled with USE_CURL=1.
Like sv_allowDownload, cl_allowDownload also uses a bitmask value
supporting the following flags:
    1 - ENABLE
    2 - do not use HTTP/FTP downloads
    4 - do not use UDP downloads



________________________________________________________

14) MULTIUSER SUPPORT ON WINDOWS SYSTEMS
___________________________________________

On Windows, all user specific files such as autogenerated configuration,
demos, videos, screenshots, and autodownloaded pk3s are now saved in a
directory specific to the user who is running XreaL.

On NT-based systems such as Windows XP, this is usually a directory named:
  "C:\Documents and Settings\%USERNAME%\Application Data\XreaL\"

On Windows Vista, this would be:
  "C:\Users\%USERNAME%\Application Data\XreaL\"

Windows 95, Windows 98, and Windows ME will use a directory like:
  "C:\Windows\Application Data\XreaL"
in single-user mode, or:
  "C:\Windows\Profiles\%USERNAME%\Application Data\XreaL"
if multiple logins have been enabled.

You can revert to the old single-user behaviour by setting the fs_homepath
cvar to the directory where XreaL is installed.  For example:
  XreaL.exe +set fs_homepath "c:\xreal"
Note that this cvar MUST be set as a command line parameter.


___________________________________________________

15) CONTRIBUTIONS
__________________________________________

If you want to contribute media assets like textures, models or sounds to the project then:

	1) Don't derivate them from the original Quake 3 Arena assets. If you want to add textures or models then you have to create them from scratch.
	
	2) Release your assets under the "Creative Commons Attribution-ShareAlike 3.0 Unported" license.
		See http://creativecommons.org/licenses/by-sa/3.0/ for more details.


___________________________________________________

16) DONATIONS
__________________________________________

If you think that this project is cool and helps you with your projects or you just have fun then make a small donation, please.

Click on one of the PayPal buttons to donate money to XreaL:
	
	http://sourceforge.net/donate/index.php?group_id=27204
