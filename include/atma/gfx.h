/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ATMA_GFX_H
#define ATMA_GFX_H

/*
 * Graphics seam. Deliberately minimal: one background clear plus hardware
 * sprites, which every target machine (and Layer 2 on the Next) can do.
 * Richer primitives belong in game code or a helper layer on top.
 */

#include "types.h"

/* Logical screen: the ZX Next's 256x192, the ceiling for every backend.
 * Compile-time constants so games can size loops and clamps without a
 * runtime lookup; atma_platform_info reports the same values. */
#define ATMA_SCREEN_W 256
#define ATMA_SCREEN_H 192

/* Core-side lifecycle, driven by the engine. */
void atma_gfx_init(void);
void atma_gfx_shutdown(void);

/* Brackets each render callback (engine calls them every tick). Backends
 * with a composited display use this to batch register writes or sync with
 * the retraces; on host they may be no-ops.
 *
 * begin_frame returns u8 liveness: nonzero when the backend has a display
 * to draw into, zero when gfx startup failed. The engine calls it once
 * before the loop starts and treats zero as "the run cannot render":
 * it logs the backend's error line, tears down in reverse init order and
 * returns nonzero from atma_run(), instead of looping invisibly forever.
 * Within the loop the return value is ignored (a backend that rendered
 * nothing every tick would be a backend bug). */
u8 atma_gfx_begin_frame(void);
void atma_gfx_end_frame(void);

/* Fill the screen with a palette index. */
void atma_gfx_clear(u8 color);

/* Fill one full-width scanline y (0..screen_h-1) with a palette index. Cheap
 * on every target; enough for gradients and raster bars. */
void atma_gfx_hline(u8 y, u8 color);

/*
 * The palette. Every color the engine draws is an index into one shared
 * table. Channels arrive as ordinary 8-bit values and are quantized by the
 * engine to the Next's 9-bit RGB333 space (top 3 bits of red, 3 of green,
 * 3 of blue), on every backend: the host is not more capable than the
 * Next, so the window shows byte-for-byte what the hardware shows. Entry
 * 0 is the transparent color for sprites. Set the palette before drawing
 * anything that depends on it (call from the game's init callback).
 *
 * Entries are live state, not upload-time constants: changing one recolors
 * every drawn pixel that references the index, sprites included, on the
 * next frame, on every platform. Palette animation is therefore portable.
 *
 * Entry 0's color is reserved: the Next matches sprite transparency by an
 * 8-bit color value rather than by index, so entry 0's color must be
 * unique among the palette's used entries (mkpats enforces this by giving
 * entry 0 a marker color no other entry shares; the zxn backend derives
 * the hardware transparency register from entry 0 whenever it is set).
 * Backgrounds should not rely on entry 0 rendering as a visible color.
 */
void atma_gfx_palette_set(u8 index, u8 r, u8 g, u8 b);

/*
 * Upload one 16x16 sprite pattern from indexed bytes: exactly
 * ATMA_PATTERN_BYTES values (one per pixel), each a palette index as set
 * with atma_gfx_palette_set(). Value 0 draws transparent. This is the
 * engine's native sprite atom (the Next's 16x16 hardware sprite); larger
 * images are a grid of patterns. Returns 0 on success, -1 on failure.
 */
#define ATMA_PATTERN_BYTES (16 * 16)
int atma_gfx_pattern_set_indexed(u8 pattern, const u8 *pixels);

/*
 * Hardware-sprite seam.
 * slot    - backend sprite index (the Next has 64 hardware sprites).
 * x, y    - position in screen pixels; i16 allows partially off-screen
 *           placements. Backends decide clipping/coordinates off-screen.
 * pattern - which bitmap to show, in the backend's pattern/attribute table.
 * flags   - backend-defined attribute bits (mirror, rotate, layer, ...).
 */
#define ATMA_SPR_MIRROR_X 0x08
#define ATMA_SPR_MIRROR_Y 0x04
#define ATMA_SPR_ROTATE   0x02 /* 90 degrees clockwise, applied before mirroring */

/*
 * Hardware limits every backend honours; the Next is the ceiling. ATMA_
 * MAX_PATTERNS bounds engine pattern ids; on the Next, 8-bit patterns cap
 * at 64 (ATMA_MAX_PATTERNS_8BIT) and 4-bit at 128. Sprite slots are capped
 * at 64 too: the useful set on the Next (64 patterns, the 64-per-scanline
 * limit), even though the slot register itself addresses 128. A backend
 * must fail (return -1, log one line) rather than silently drop an upload
 * beyond what it stores.
 */
#define ATMA_MAX_SPRITES  64
#define ATMA_MAX_PATTERNS 256
#define ATMA_MAX_PATTERNS_8BIT 64

void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags);
void atma_gfx_sprite_hide(u8 slot);

#endif
