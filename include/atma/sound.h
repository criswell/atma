/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ATMA_SOUND_H
#define ATMA_SOUND_H

/*
 * Sound seam. Music and SFX are addressed by small integer ids whose
 * meaning is backend-defined (on zxn these index vt_sound patterns/ songs);
 * a game picks ids once per build or keeps them aligned via its asset
 * pipeline.
 */

#include "types.h"

/* Core-side lifecycle, driven by the engine. */
void atma_sound_init(void);
void atma_sound_shutdown(void);

/* Pumped once per tick by the engine, after update(). Backends that drive
 * music from an interrupt instead use this for bookkeeping only. */
void atma_sound_update(void);

/* Start the given music track, or stop playback. Starting a new track while
 * one plays should switch over, not require an explicit stop. */
void atma_sound_music_play(u8 track);
void atma_sound_music_stop(void);

/* Fire a one-shot sound effect; safe to call every tick. */
void atma_sound_sfx_play(u8 id);

#endif
