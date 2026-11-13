#ifndef ATMA_HOST_H
#define ATMA_HOST_H

/*
 * Host-only extensions (desktop build). Nothing here exists on retro
 * targets, so game code that calls it must be guarded, e.g.
 * #ifdef ATMA_PLATFORM_HOST. The core API stays capped at ZX Next limits
 * (256x192, 64 sprites, 256 patterns); the host merely lets assets and
 * colors exceed what the Next can store.
 */

#include "types.h"

/* Palette entries are full 24-bit RGB here; the Next only has 9-bit RGB. */
void atma_host_palette_set(u8 index, u8 r, u8 g, u8 b);

/*
 * Define one sprite pattern from RGBA8888 pixels (w*h*4 bytes, alpha 0 is
 * transparent). Patterns may be any size, not just 16x16. Needs the gfx
 * backend initialized (call from the game's init callback or later).
 * Returns 0 on success, -1 on failure.
 */
int atma_host_pattern_set_rgba(u8 pattern, u16 w, u16 h, const u8 *rgba);

/*
 * Slice a PNG (or BMP) spritesheet into frame_w x frame_h patterns, row by
 * row, starting at first_pattern. Returns the number of patterns loaded, or
 * -1 on failure. PNG alpha is honoured.
 */
int atma_host_spritesheet_load(const char *path, u16 frame_w, u16 frame_h,
                               u8 first_pattern);

/*
 * Load one image (PNG or BMP) as a single pattern, scaled to w x h. Use this
 * for art authored larger than the screen allows. Returns 0 or -1.
 */
int atma_host_image_load(const char *path, u8 pattern, u16 w, u16 h);

#endif
