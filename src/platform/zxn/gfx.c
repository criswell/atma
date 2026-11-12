#include "atma/gfx.h"

/* TODO: zxnext_layer2 for background, zxnext_sprite for sprites */

void atma_gfx_init(void) {}
void atma_gfx_shutdown(void) {}
void atma_gfx_begin_frame(void) {}
void atma_gfx_end_frame(void) {}
void atma_gfx_clear(u8 color) { (void)color; }
void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags)
{
    (void)slot; (void)x; (void)y; (void)pattern; (void)flags;
}
void atma_gfx_sprite_hide(u8 slot) { (void)slot; }
