#ifndef ATMA_SOUND_H
#define ATMA_SOUND_H

#include "types.h"

void atma_sound_init(void);
void atma_sound_shutdown(void);

void atma_sound_update(void);

void atma_sound_music_play(u8 track);
void atma_sound_music_stop(void);
void atma_sound_sfx_play(u8 id);

#endif
