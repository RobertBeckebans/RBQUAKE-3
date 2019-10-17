/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
//
// cg_animation.c

#include "cg_local.h"


qboolean CG_RegisterAnimation(animation_t * anim, const char *filename,
										   qboolean loop, qboolean reversed, qboolean clearOrigin)
{
	int             frameRate;

	anim->handle = trap_R_RegisterAnimation(filename);
	if(!anim->handle)
	{
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

	anim->firstFrame = 0;
	anim->numFrames = trap_R_AnimNumFrames(anim->handle);
	frameRate = trap_R_AnimFrameRate(anim->handle);

	if(frameRate == 0)
	{
		frameRate = 1;
	}
	anim->frameTime = 1000 / frameRate;
	anim->initialLerp = 1000 / frameRate;

	if(loop)
	{
		anim->loopFrames = anim->numFrames;
	}
	else
	{
		anim->loopFrames = 0;
	}

	anim->reversed = reversed;
	anim->clearOrigin = clearOrigin;

	return qtrue;
}

void CG_SetLerpFrameAnimation(lerpFrame_t * lf, animation_t * anims, int animsNum, int newAnimation)
{
	animation_t    *anim;

	// save old animation
	lf->old_animationNumber = lf->animationNumber;
	lf->old_animation = lf->animation;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if(newAnimation < 0 || newAnimation >= animsNum)
	{
		CG_Error("CG_SetLerpFrameAnimation: bad animation number: %i", newAnimation);
	}

	anim = &anims[newAnimation];

	lf->animation = anim;
	lf->animationStartTime = lf->frameTime + anim->initialLerp;

	if(lf->old_animationNumber <= 0)
	{
		// skip initial / invalid blending
		lf->blendlerp = 0.0f;
		return;
	}

	// TODO: blend through two blendings!

	if((lf->blendlerp <= 0.0f))
		lf->blendlerp = 1.0f;
	else
		lf->blendlerp = 1.0f - lf->blendlerp;	// use old blending for smooth blending between two blended animations

	memcpy(&lf->oldSkeleton, &lf->skeleton, sizeof(refSkeleton_t));

	//Com_Printf("new: %i old %i\n", newAnimation,lf->old_animationNumber);

	if(!trap_R_BuildSkeleton(&lf->oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin))
	{
		CG_Printf("CG_SetLerpFrameAnimation: can't blend skeleton\n");
		return;
	}
}

/*
===============
CG_RunLerpFrame
===============
*/
void CG_RunLerpFrame(lerpFrame_t * lf, animation_t * anims, int animsNum, int newAnimation, float speedScale)
{
	int             f, numFrames;
	animation_t    *anim;
	qboolean        animChanged;

	// debugging tool to get no animations
	if(cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if(newAnimation != lf->animationNumber || !lf->animation)
	{
		CG_SetLerpFrameAnimation(lf, anims, animsNum, newAnimation);

		if(!lf->animation)
		{
			memcpy(&lf->oldSkeleton, &lf->skeleton, sizeof(refSkeleton_t));
		}

		animChanged = qtrue;
	}
	else
	{
		animChanged = qfalse;
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if(cg.time >= lf->frameTime || animChanged)
	{
		if(animChanged)
		{
			lf->oldFrame = 0;
			lf->oldFrameTime = cg.time;
		}
		else

		{
			lf->oldFrame = lf->frame;
			lf->oldFrameTime = lf->frameTime;
		}

		// get the next frame based on the animation
		anim = lf->animation;
		if(!anim->frameTime)
		{
			return;				// shouldn't happen
		}

		if(cg.time < lf->animationStartTime)
		{
			lf->frameTime = lf->animationStartTime;	// initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameTime;
		}
		f = (lf->frameTime - lf->animationStartTime) / anim->frameTime;
		f *= speedScale;		// adjust for haste, etc

		numFrames = anim->numFrames;

		if(anim->flipflop)
		{
			numFrames *= 2;
		}

		if(f >= numFrames)
		{
			f -= numFrames;

			if(anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}

		if(anim->reversed)
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if(anim->flipflop && f >= anim->numFrames)
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f % anim->numFrames);
		}
		else
		{
			lf->frame = anim->firstFrame + f;
		}

		if(cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;
		}
	}

	if(lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if(lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}

	// calculate current lerp value
	if(lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}

	// blend old and current animation
	if(cg_animBlend.value <= 0.0f)
	{
		lf->blendlerp = 0.0f;
	}

	if((lf->blendlerp > 0.0f) && (cg.time > lf->blendtime))
	{
#if 0
		// linear blending
		lf->blendlerp -= 0.025f;
#else
		// exp blending
		lf->blendlerp -= lf->blendlerp / cg_animBlend.value;
#endif
		if(lf->blendlerp <= 0.0f)
			lf->blendlerp = 0.0f;
		if(lf->blendlerp >= 1.0f)
			lf->blendlerp = 1.0f;

		lf->blendtime = cg.time + 10;
	}

	if(!trap_R_BuildSkeleton(&lf->skeleton, lf->animation->handle, lf->oldFrame, lf->frame, 1.0 - lf->backlerp, lf->animation->clearOrigin))
	{
		CG_Printf("Can't build lf->skeleton\n");
	}

	// lerp between old and new animation if possible
	if(lf->blendlerp > 0.0f)
	{
		if(!trap_R_BlendSkeleton(&lf->skeleton, &lf->oldSkeleton, lf->blendlerp))
		{
			CG_Printf("Can't blend\n");
			return;
		}
	}
}
