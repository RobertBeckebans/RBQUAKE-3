#!/bin/sh

rm -rf engine/
rm -rf games/
rm -rf shared/
rm -rf libs/

mkdir -p libs
mkdir -p engine
mkdir -p engine/null
mkdir -p engine/collision
mkdir -p engine/sound
mkdir -p games/default
mkdir -p shared
mkdir -p tools

git mv curl-7.54.0 libs/curl
git mv jpeg-8c libs/jpeg
git mv libogg-1.3.3 libs/ogg
git mv libvorbis-1.3.6 libs/vorbis
git mv opus-1.2.1 libs/opus
git mv opusfile-0.9 libs/opusfile
git mv SDL2 libs/sdl
git mv zlib libs/zlib

git mv renderercommon/tr_types.h shared/
git mv cgame/cg_public.h shared/
git mv game/bg_public.h shared/
git mv game/g_public.h shared/
git mv qcommon/q_math.c shared/
git mv qcommon/q_shared.c shared/
git mv qcommon/q_shared.h shared/
git mv qcommon/q_platform.h shared/
git mv game/surfaceflags.h shared/
git mv client/keycodes.h shared/
git mv ui/ui_public.h shared/

git mv botlib shared/botlib
#git mv game/botlib.h shared/
#git mv game/be_aas.h shared/
#git mv game/be_ai_char.h shared/
#git mv game/be_ai_chat.h shared/
#git mv game/be_ai_gen.h shared/
#git mv game/be_ai_goal.h shared/
#git mv game/be_ai_move.h shared/
#git mv game/be_ai_weap.h shared/
#git mv game/be_ea.h shared/

git mv qcommon/cm_* engine/collision/
git mv client/snd_* engine/sound/
git mv qcommon engine/
git mv client engine/
git mv renderercommon engine/
git mv renderergl1 engine/
git mv renderergl2 engine/
git mv sys engine/
git mv sdl/* engine/sys/

git mv server engine/

git mv cgame games/default/
git mv game games/default/
git mv q3_ui games/default/

git mv bspc tools/
git mv splines engine/
