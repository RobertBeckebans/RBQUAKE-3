/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "tr_types.h"

#define	REF_API_VERSION		16

// *INDENT-OFF*
 
//
// these are the functions exported by the refresh module
//
typedef struct
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void            (*Shutdown) (qboolean destroyWindow);
	
	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void            (*BeginRegistration) (glconfig_t * config, glconfig2_t * glconfig2);
	qhandle_t		(*RegisterModel) (const char *name);
	qhandle_t		(*RegisterSkin) (const char *name);
	qhandle_t		(*RegisterShader) (const char *name);
	qhandle_t		(*RegisterShaderNoMip) (const char *name);
	qhandle_t		(*RegisterShaderLightAttenuation) (const char *name);
	void            (*LoadWorld) (const char *name);
	
	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void            (*SetWorldVisData) (const byte * vis);
	
	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void            (*EndRegistration) (void);
	
	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void            (*ClearScene) (void);
	void            (*AddRefEntityToScene) (const refEntity_t * re);
	
	int             (*LightForPoint) (vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
	
	void            (*AddPolyToScene) (qhandle_t hShader, int numVerts, const polyVert_t * verts, int num);
	//void            (*AddPolysToScene) (qhandle_t hShader, int numVerts, const polyVert_t * verts, int numPolys);
	
	void            (*AddLightToScene) (const vec3_t org, float intensity, float r, float g, float b);
	void            (*RenderScene) (const refdef_t * fd);
	
	void            (*SetColor) (const float *rgba);	// NULL = 1,1,1,1
	void            (*DrawStretchPic) (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);	// 0 = white
	void            (*DrawRotatedPic) (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);
	void            (*DrawStretchPicGradient) (float x, float y, float w, float h, float s1, float t1, float s2, float t2,
											   qhandle_t hShader, const float *gradientColor, int gradientType);
	void            (*Add2dPolys) (polyVert_t * polys, int numverts, qhandle_t hShader);
	
	// Draw images for cinematic rendering, pass as 32 bit rgba
	void            (*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, const byte * data, int client,
									   qboolean dirty);
	void            (*UploadCinematic) (int w, int h, int cols, int rows, const byte * data, int client, qboolean dirty);
	
	void            (*BeginFrame) (stereoFrame_t stereoFrame);
	
	// if the pointers are not NULL, timing info will be returned
	void            (*EndFrame) (int *frontEndMsec, int *backEndMsec);
	
	
	int             (*MarkFragments) (int numPoints, const vec3_t * points, const vec3_t projection,
									  int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t * fragmentBuffer);
									  
	int             (*LerpTag) (orientation_t * tag, qhandle_t model, int startFrame, int endFrame, float frac, const char *tagName);
	
	void            (*ModelBounds) (qhandle_t model, vec3_t mins, vec3_t maxs);
	
	void            (*RemapShader) (const char *oldShader, const char *newShader, const char *offsetTime);
	qboolean		(*GetEntityToken) (char *buffer, int size);
	qboolean		(*inPVS) (const vec3_t p1, const vec3_t p2);
	
	void            (*RegisterFont) (const char *fontName, int pointSize, fontInfo_t * font);
	// XreaL BEGIN
	void            (*TakeVideoFrame) (int h, int w, byte * captureBuffer, byte * encodeBuffer, qboolean motionJpeg);
	
#if defined(USE_REFLIGHT)
	void            (*AddRefLightToScene) (const refLight_t * light);
#endif
	
	// RB: alternative skeletal animation system
#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	qhandle_t		(*RegisterAnimation) (const char *name);
	int				(*CheckSkeleton) (refSkeleton_t * skel, qhandle_t model, qhandle_t anim);
	int             (*BuildSkeleton) (refSkeleton_t * skel, qhandle_t anim, int startFrame, int endFrame, float frac,
									  qboolean clearOrigin);
	int             (*BlendSkeleton) (refSkeleton_t * skel, const refSkeleton_t * blend, float frac);
	int             (*BoneIndex) (qhandle_t hModel, const char *boneName);
	int             (*AnimNumFrames) (qhandle_t hAnim);
	int             (*AnimFrameRate) (qhandle_t hAnim);
#endif
	
	// XreaL END
	
	void            (*TakeScreenshotPNG) (int x, int y, int width, int height, char *fileName);
} refexport_t;
 
//
// these are the functions imported by the refresh module
//
typedef struct
{
	// print message on the local console
	void            (QDECL * Printf) (int printLevel, const char *fmt, ...);
	
	// abort the game
	void            (QDECL * Error) (int errorLevel, const char *fmt, ...);
	
	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int             (*Milliseconds) (void);
	
	int             (*RealTime) (qtime_t * qtime);
	
	// stack based memory allocation for per-level things that
	// won't be freed
	void            (*Hunk_Clear) (void);
#ifdef HUNK_DEBUG
	void           *(*Hunk_AllocDebug) (int size, ha_pref pref, char *label, char *file, int line);
#else
	void           *(*Hunk_Alloc) (int size, ha_pref pref);
#endif
	void           *(*Hunk_AllocateTempMemory) (int size);
	void            (*Hunk_FreeTempMemory) (void *block);
	
	// dynamic memory allocator for things that need to be freed
#ifdef ZONE_DEBUG
	void           *(*Z_MallocDebug) (int bytes, char *label, char *file, int line);
#else
	void           *(*Z_Malloc) (int bytes);
#endif
	void            (*Free) (void *buf);
	void            (*Tag_Free) (void);
	
	cvar_t         *(*Cvar_Get) (const char *name, const char *value, int flags);
	void            (*Cvar_Set) (const char *name, const char *value);
	void            (*Cvar_SetValue) (const char *name, float value);
	void            (*Cvar_CheckRange) (cvar_t * cv, float minVal, float maxVal, qboolean shouldBeIntegral);
	
	int             (*Cvar_VariableIntegerValue) (const char *var_name);
	
	void            (*Cmd_AddCommand) (const char *name, void (*cmd) (void));
	void            (*Cmd_RemoveCommand) (const char *name);
	
	int             (*Cmd_Argc) (void);
	char           *(*Cmd_Argv) (int i);
	
	void            (*Cmd_ExecuteText) (int exec_when, const char *text);
	
	// visualization for debugging collision detection
	byte           *(*CM_ClusterPVS)(int cluster);
	int             (*CM_PointContents)(const vec3_t p, clipHandle_t model);
	void            (*CM_DrawDebugSurface) (void (*drawPoly) (int color, int numPoints, float *points));
	
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int             (*FS_FileIsInPAK) (const char *name, int *pChecksum);
	int             (*FS_ReadFile) (const char *name, void **buf);
	void            (*FS_FreeFile) (void *buf);
	char          **(*FS_ListFiles) (const char *name, const char *extension, int *numfilesfound);
	char          **(*FS_ListFilteredFiles) (const char *name, const char *extension, char *filter, int *numfilesfound);
	void            (*FS_FreeFileList) (char **filelist);
	void            (*FS_WriteFile) (const char *qpath, const void *buffer, int size);
	                qboolean(*FS_FileExists) (const char *file);
	
	// cinematic stuff
	void            (*CIN_UploadCinematic) (int handle);
	int             (*CIN_PlayCinematic) (const char *arg0, int xpos, int ypos, int width, int height, int bits);
	e_status        (*CIN_RunCinematic) (int handle);
	
	// XreaL BEGIN
	void           *(*Sys_GetSystemHandles) (void);
	
	qboolean        (*CL_VideoRecording) (void);
	void            (*CL_WriteAVIVideoFrame) (const byte * buffer, int size);
	// XreaL END
	
	// input event handling
	void            (*IN_Init) (void* windowData);
	void            (*IN_Shutdown) (void);
	void            (*IN_Restart) (void);
} refimport_t;
 
 
// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
 
// RB: changed to GetRefAPI_t
typedef refexport_t* (*GetRefAPI_t) (int apiVersion, refimport_t * rimp);
 
#endif							// __TR_PUBLIC_H
 