/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "atma/gfx.h"

/*
 * ZX Next graphics backend.
 *
 * Plan: Layer 2 (zxnext_layer2) carries the background/atma_gfx_clear, and
 * the Next's hardware sprites (zxnext_sprite) back atma_gfx_sprite_set —
 * 64 sprites, 256x192 fits both systems. begin/end_frame will batch the
 * per-frame register pokes.
 */

/* TODO: zxnext_layer2 for background, zxnext_sprite for sprites */

void atma_gfx_init(void) {}
void atma_gfx_shutdown(void) {}
u8 atma_gfx_begin_frame(void) { return 1; }
void atma_gfx_end_frame(void) {}
void atma_gfx_clear(u8 color) { (void)color; }
void atma_gfx_hline(u8 y, u8 color) { (void)y; (void)color; }
void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags)
{
    (void)slot; (void)x; (void)y; (void)pattern; (void)flags;
}
void atma_gfx_sprite_hide(u8 slot) { (void)slot; }
void atma_gfx_palette_set(u8 index, u8 r, u8 g, u8 b)
{
    (void)index; (void)r; (void)g; (void)b;
}
int atma_gfx_pattern_set_indexed(u8 pattern, const u8 *pixels)
{
    (void)pattern; (void)pixels;
    return -1;
}
