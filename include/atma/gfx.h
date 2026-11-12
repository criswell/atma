#ifndef ATMA_GFX_H
#define ATMA_GFX_H

#include "types.h"

void atma_gfx_init(void);
void atma_gfx_shutdown(void);

void atma_gfx_begin_frame(void);
void atma_gfx_end_frame(void);

void atma_gfx_clear(u8 color);

void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags);
void atma_gfx_sprite_hide(u8 slot);

#endif
