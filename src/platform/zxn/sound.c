#include "atma/sound.h"

/*
 * ZX Next sound backend, planned around vt_sound: vt_init in atma_sound_init,
 * music via vt_play (driven from the frame ISR), sound_update pumping any
 * per-tick bookkeeping. The u8 track/id arguments index whatever the linked
 * music data defines.
 */

/* TODO: vt_sound (vt_init / vt_play / vt_play_isr) */

void atma_sound_init(void) {}
void atma_sound_shutdown(void) {}
void atma_sound_update(void) {}
void atma_sound_music_play(u8 track) { (void)track; }
void atma_sound_music_stop(void) {}
void atma_sound_sfx_play(u8 id) { (void)id; }
