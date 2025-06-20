/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef __TR_TYPES_H
#define __TR_TYPES_H

// XreaL BEGIN
#define MAX_REF_LIGHTS	   1024
#define MAX_REF_ENTITIES   1023 // can't be increased without changing drawsurf bit packing
#define MAX_BONES		   128	// RB: same as MDX_MAX_BONES
#define MAX_WEIGHTS		   4	// GPU vertex skinning limit, never change this without rewriting many GLSL shaders
// XreaL END

// renderfx flags
#define RF_MINLIGHT		   0x0001 // allways have some light (viewmodel, some items)
#define RF_THIRD_PERSON	   0x0002 // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON	   0x0004 // only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK	   0x0008 // for view weapon Z crunching

#define RF_CROSSHAIR	   0x0010 // This item is a cross hair and will draw over everything similar to
// DEPTHHACK in stereo rendering mode, with the difference that the
// projection matrix won't be hacked to reduce the stereo separation as
// is done for the gun.

#define RF_NOSHADOW		   0x0040 // don't add stencil shadows

#define RF_LIGHTING_ORIGIN 0x00000020 // use refEntity->lightingOrigin instead of refEntity->origin
// for lighting.  This allows entities to sink into the floor
// with their origin going solid, and allows all parts of a
// player to get the same lighting
#define RF_SHADOW_PLANE	   0x00000040 // use refEntity->shadowPlane
#define RF_WRAP_FRAMES	   0x00000080 // mod the model frames by the maxframes to allow continuous
// animation without needing to know the frame count
#define RF_HILIGHT		   0x00000100 // more than RF_MINLIGHT.  For when an object is "Highlighted" (looked at/training identification/etc)
#define RF_BLINK		   0x00000200 // eyes in 'blink' state
#define RF_FORCENOLOD	   0x00000400

// refdef flags
#define RDF_NOWORLDMODEL   ( 1 << 0 ) // used for player configuration screen
#define RDF_NOSHADOWS	   ( 1 << 1 ) // force renderer to use faster lighting only path
#define RDF_HYPERSPACE	   ( 1 << 2 ) // teleportation effect
#define RDF_SKYBOXPORTAL   ( 1 << 3 )

#define RDF_UNDERWATER	   ( 1 << 4 ) // enable automatic underwater caustics and fog
#define RDF_DRAWINGSKY	   ( 1 << 5 )
#define RDF_SNOOPERVIEW	   ( 1 << 6 )

// XreaL BEGIN
#define RDF_NOCUBEMAP	   ( 1 << 7 ) // RB: don't use cubemaps
#define RDF_NOBLOOM		   ( 1 << 8 ) // RB: disable bloom. useful for hud models
// XreaL END

typedef struct
{
	vec3_t xyz;
	float  st[2];
	byte   modulate[4];
} polyVert_t;

typedef struct poly_s
{
	qhandle_t	hShader;
	int			numVerts;
	polyVert_t* verts;
} poly_t;

typedef enum
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_SPLASH, // ripple effect
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_CORE_TAPER, // a modified core that creates a properly texture mapped core that's wider at one end
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE, // doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

// XreaL BEGIN

#define USE_REFLIGHT				  1

// RB: defining any of the following macros would break the compatibility to old ET mods
#define USE_REFENTITY_ANIMATIONSYSTEM 1
#define USE_REFENTITY_NOSHADOWID	  1

// RB: having bone names for each refEntity_t takes several MiBs
// in backEndData_t so only use it for debugging and development
// enabling this will show the bone names with r_showSkeleton 1

#define REFBONE_NAMES				  1

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
typedef struct
{
	#if defined( REFBONE_NAMES )
	char name[64];
	#endif
	short  parentIndex; // parent index (-1 if root)
	vec3_t origin;
	quat_t rotation;
} refBone_t;

typedef enum
{
	SK_INVALID,
	SK_RELATIVE,
	SK_ABSOLUTE
} refSkeletonType_t;

typedef struct
{
	refSkeletonType_t type; // skeleton has been reset

	short			  numBones;
	refBone_t		  bones[MAX_BONES];

	vec3_t			  bounds[2]; // bounds of all applied animations
	vec3_t			  scale;
} refSkeleton_t;
#endif

// XreaL END

typedef struct
{
	refEntityType_t reType;
	int				renderfx;

	qhandle_t		hModel; // opaque type outside refresh

	// most recent data
	vec3_t			lightingOrigin; // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float			shadowPlane;	// projection shadows go here, stencils go slightly lower

	vec3_t			axis[3];		   // rotation vectors
	qboolean		nonNormalizedAxes; // axis are not normalized, i.e. they have scale
	vec3_t			origin;			   // also used as MODEL_BEAM's "from"
	int				frame;			   // also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	vec3_t			oldorigin; // also used as MODEL_BEAM's "to"
	int				oldframe;
	float			backlerp; // 0.0 = current, 1.0 = old

	// texturing
	int				skinNum;	  // inline skin index
	qhandle_t		customSkin;	  // NULL for default skin
	qhandle_t		customShader; // use one image for the entire thing

	// misc
	byte			shaderRGBA[4];	   // colors used by rgbgen entity shaders
	float			shaderTexCoord[2]; // texture coordinates used by tcMod entity modifiers
	float			shaderTime;		   // subtracted from refdef time to control effect start times

	// extra sprite information
	float			radius;
	float			rotation;

	// Ridah
	vec3_t			fireRiseDir;

	// Ridah, entity fading (gibs, debris, etc)
	int				fadeStartTime, fadeEndTime;

	float			hilightIntensity; //----(SA)  added

	int				reFlags;

	int				entityNum; // currentState.number, so we can attach rendering effects to specific entities (Zombie)

	// XreaL BEGIN

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
	// extra animation information
	refSkeleton_t skeleton;
#endif

#if defined( USE_REFENTITY_NOSHADOWID )
	// extra light interaction information
	short noShadowID;
#endif

	// XreaL END

} refEntity_t;

// ================================================================================================

// XreaL BEGIN

typedef enum
{
	RL_OMNI,		// point light
	RL_PROJ,		// spot light
	RL_DIRECTIONAL, // sun light

	RL_MAX_REF_LIGHT_TYPE
} refLightType_t;

typedef struct
{
	refLightType_t rlType;
	//  int             lightfx;

	qhandle_t	   attenuationShader;

	vec3_t		   origin;
	quat_t		   rotation;
	vec3_t		   center;
	vec3_t		   color; // range from 0.0 to 1.0, should be color normalized

	float		   scale; // r_lightScale if not set

	// omni-directional light specific
	vec3_t		   radius;

	// projective light specific
	vec3_t		   projTarget;
	vec3_t		   projRight;
	vec3_t		   projUp;
	vec3_t		   projStart;
	vec3_t		   projEnd;

	qboolean	   noShadows;
	short		   noShadowID; // don't cast shadows of all entities with this id

	qboolean	   inverseShadows; // don't cast light and draw shadows by darken the scene
								   // this is useful for drawing player shadows with shadow mapping
} refLight_t;

// XreaL END

#define MAX_RENDER_STRINGS		 8
#define MAX_RENDER_STRING_LENGTH 32

typedef struct
{
	int	   x, y, width, height;
	float  fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3]; // transformation matrix
	vec3_t blurVec;		// motion blur direction

	// time in milliseconds for shader effects and other time dependent rendering issues
	int	   time;

	int	   rdflags; // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte   areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char   text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;

typedef enum
{
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} stereoFrame_t;

// XreaL BEGIN

// cg_shadows modes
typedef enum
{
	SHADOWING_NONE,
	SHADOWING_BLOB,
	SHADOWING_ESM16,
	SHADOWING_ESM32,
	SHADOWING_VSM16,
	SHADOWING_VSM32,
	SHADOWING_EVSM32,
} shadowingMode_t;
// XreaL END

/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
typedef enum
{
	TC_NONE,
	TC_S3TC,
	TC_EXT_COMP_S3TC
} textureCompression_t;

typedef enum
{
	GLDRV_DEFAULT, // OpenGL 2.0 with compatibility profile
	GLDRV_OPENGL3, // OpenGL 3.2 with core profile
	GLDRV_MESA,	   // software based OpenGL 2.0
} glDriverType_t;

typedef enum
{
	GLHW_GENERIC,  // where everthing works the way it should
	GLHW_ATI,	   // where you don't have proper GLSL support
	GLHW_ATI_DX10, // ATI Radeon HD series DX10 hardware
	GLHW_NV_DX10   // Geforce 8/9 class DX10 hardware
} glHardwareType_t;

typedef struct
{
	char				 renderer_string[MAX_STRING_CHARS];
	char				 vendor_string[MAX_STRING_CHARS];
	char				 version_string[MAX_STRING_CHARS];
	char				 extensions_string[BIG_INFO_STRING];

	int					 maxTextureSize; // queried from GL

	int					 colorBits, depthBits, stencilBits;

	glDriverType_t		 driverType;
	glHardwareType_t	 hardwareType;

	qboolean			 deviceSupportsGamma;
	textureCompression_t textureCompression;

	int					 vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float				 windowAspect;

	int					 displayFrequency;

	// synonymous with "does rendering consume the entire screen?"
	qboolean			 isFullscreen;
	qboolean			 stereoEnabled;
	qboolean			 smpActive; // UNUSED, present for compatibility
} glconfig_t;

// XreaL BEGIN
typedef struct
{
	qboolean ARBTextureCompressionAvailable;

	int		 maxCubeMapTextureSize;

	qboolean occlusionQueryAvailable;
	int		 occlusionQueryBits;

	char	 shadingLanguageVersion[MAX_STRING_CHARS];

	int		 maxVertexUniforms;
	int		 maxVaryingFloats;
	int		 maxVertexAttribs;
	qboolean vboVertexSkinningAvailable;
	int		 maxVertexSkinningBones;

	qboolean texture3DAvailable;
	qboolean textureNPOTAvailable;

	qboolean drawBuffersAvailable;
	qboolean textureHalfFloatAvailable;
	qboolean textureFloatAvailable;
	int		 maxDrawBuffers;

	qboolean vertexArrayObjectAvailable;

	float	 maxTextureAnisotropy;
	qboolean textureAnisotropyAvailable;

	qboolean framebufferObjectAvailable;
	int		 maxRenderbufferSize;
	int		 maxColorAttachments;
	qboolean framebufferPackedDepthStencilAvailable;
	qboolean framebufferBlitAvailable;
} glconfig2_t;
// XreaL END

// =========================================
// Gordon, these MUST NOT exceed the values for SHADER_MAX_VERTEXES/SHADER_MAX_INDEXES
#define MAX_PB_VERTS	1025
#define MAX_PB_INDICIES ( MAX_PB_VERTS * 6 )

typedef struct polyBuffer_s
{
	vec4_t	  xyz[MAX_PB_VERTS];
	vec2_t	  st[MAX_PB_VERTS];
	byte	  color[MAX_PB_VERTS][4];
	int		  numVerts;

	int		  indicies[MAX_PB_INDICIES];
	int		  numIndicies;

	qhandle_t shader;
} polyBuffer_t;

// =========================================

#endif // __TR_TYPES_H
