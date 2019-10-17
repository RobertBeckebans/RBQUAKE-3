## ABOUT THE PORT

`RBQUAKE-3 is all about experimentation and fun.`

## XreaL Q3A total conversion in 2008

![XreaL](https://github.com/RobertBeckebans/RBQUAKE-3/raw/master/docs/xreal_scrnshot.jpg)

RBQUAKE-3 is based on XreaL and the goal of this port was to bring Q3A up to latest technology in 2009 making it closer to Doom 3 and Quake 4 while still remaining a Quake 3 port regarding the gameplay. Some results were:

* OpenGL 3.2 renderer with all deprecated and immediate mode API OpenGL calls removed
* Clever usage of vertex buffer objects (VBO) to speed up rendering of everything
* Avoids geometry processing each frame using the CPU (worst bottleneck with the Q3A engine)
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
* A new XBSP format with support for per vertex HDR light data
* Deluxe light mapping support (stores dominant light direction per texel besides color)


It uses code from the ioquake3 project ![ioq3](https://github.com/RobertBeckebans/RBQUAKE-3/raw/master/docs/ioquake3_logo.png)


## GENERAL NOTES

A short summary of the file layout:

Directory                     | Description
:---------------------------- | :------------------------------------------------
RBQUAKE-3/base/                   | XreaL media directory ( models, textures, sounds, maps, etc. )
RBQUAKE-3/code/                   | RBQUAKE-3 source code ( renderer, game code for multiple games, OS layer, etc. )
RBQUAKE-3/code/tools/xmap         | map compiler ( .map -> .bsp ) (based on q3map)
RBQUAKE-3/code/tools/xmap2        | map compiler ( .map -> .bsp ) (based on q3map2)
RBQUAKE-3/code/tools/xmaster	  | master server
RBQUAKE-3/blender/          	  | Blender plugins for ase, md3, and md5 models


## LICENSE

See docs/COPYING.txt for all the legal stuff.


## GETTING THE SOURCE CODE AND MEDIA

This project's git repository can be cloned with the following instruction set: 

`
git clone https://github.com/RobertBeckebans/RBQUAKE-3.git
`

**TODO** git lfs instructions

## COMPILING ON WINDOWS

1. Download and install the Visual Studio 2017 Community Edition.
2. Generate the VS2017 projects using Premake 5 by doubleclicking a matching configuration .bat file in the premake/ folder.
3. Use the VC2017 solution to compile what you need: RBQUAKE-3/premake/RBQUAKE-3.sln

## ".plan"
- Make it compatible again with baseq3 and provide game dlls for it
- Restore XreaL assets in basex/ mod dir based on the XreaL_PreAlpha_20080704.7z before it went into a weird art direction so it can run without relying on Quake 3
- Restore old-school cool looking q3_ui menu from the PreAlpha
- Add several Quake3 mods under code/games/ like Modifia where the authors gave me the permission to release the entire stuff under GPL
- Sync with latest ioquake3

## May or may not ".plan"
- New, widescreen menu support
- Better keyboard and gamepad UI navigation
- Replace ultra simple and stupid ACEBot with some modern deep AI thing
- Replace Quake 3 botlib with navigation meshes based on Recast (I saw this in Unvanquished)
- Slim texture loading routines based on stb_image
- Add texture compression based on [Basis Universal GPU Texture and Texture Video Compression Codec](https://github.com/binomialLLC/basis_universal)
- Scrap complex and complicated deferred and multipass forward shading with a simpler forward+ solution
- Replace OpenGL with Vulkan
- Rip & Tear renderer backend with modern approaches by [The-Forge](https://github.com/ConfettiFX/The-Forge)
- Add Temporal Anti-Aliasing
- Replace collision detection with PhysX 4.1
- Upgrade xmap2 to use Spherical Gaussians and my new RBDOOM-3-BFG JSON based map format for map making in Blender 2.8
- Add glTF 2.0 support because it is the new JPG of 3D ;)
- Replace sound system with [SoLoud](https://github.com/jarikomppa/soloud)
- Replace the entire core engine with a new engine written in Rust. (Unlikely but Rust is sexy)

## KNOWN ISSUES

- Probably not so many

## FAQ ##

**Q**: Why bother with Quake 3 in 2019 / 2020?
**A**: It is fun, period. Quake 3 was one of my favorite games of all time. It is a classic like Pacman or Chess and never gets old.

**Q**: Can I use this engine to make a commercial game?
**A**: You can but don't bother me to give you free support and you probably should use Unreal Engine 4. I am a full time game developer and don't have time for your stuff.

**Q** I'm seeing `VM_Create on UI failed`. Help!
**A**: This means the UI DLL didn't copy to the right deploy directory. It should be in the same directory as your exe, or in _baseq3_.

**Q**: How do I run the game in widescreen?
**A**: `r_mode -1; r_customwidth 1920; r_customheight 1080`

**Q**: I'm seeing `Cannot find ui/hud.txt`.
**A**: This means you're running normal Q3A with the Team Arena ui.dll. Swap it out for the proper one.

**Q**: How do I know what code you've changed?
**A**: Apart from the Git diffs, you can look for `// RB` in the source code.

**Q**: Can I use this engine to make a commercial game?
**A**: You can but don't bother me to give you free support and you probably should use Unreal Engine 4.

## MULTIUSER SUPPORT ON WINDOWS SYSTEMS

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
  xreal.exe +set fs_homepath "c:\xreal"
Note that this cvar MUST be set as a command line parameter.