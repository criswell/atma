/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file sound.h
 * \brief Sound seam: music tracks and one-shot effects by integer id.
 */

#ifndef ATMA_SOUND_H
#define ATMA_SOUND_H

/**
 * \defgroup sound Sound
 * \brief Music and SFX addressed by small integer ids.
 *
 * Music and SFX are addressed by small integer ids whose meaning is
 * backend-defined (on zxn these index vt_sound patterns/songs); a game picks
 * ids once per build or keeps them aligned via its asset pipeline.
 * @{
 */

#include "types.h"

/** \brief Core-side sound init, driven by the engine. */
void atma_sound_init(void);
/** \brief Core-side sound shutdown, driven by the engine. */
void atma_sound_shutdown(void);

/**
 * \brief Pump the sound backend.
 *
 * Called once per tick by the engine, after update(). Backends that drive
 * music from an interrupt instead use this for bookkeeping only.
 */
void atma_sound_update(void);

/**
 * \brief Start the given music track.
 *
 * Starting a new track while one plays should switch over, not require an
 * explicit stop.
 *
 * \param track Backend-defined track id.
 */
void atma_sound_music_play(u8 track);

/** \brief Stop music playback. */
void atma_sound_music_stop(void);

/**
 * \brief Fire a one-shot sound effect; safe to call every tick.
 * \param id Backend-defined effect id.
 */
void atma_sound_sfx_play(u8 id);

/** @} */

#endif
