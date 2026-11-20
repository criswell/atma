/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file gfx.h
 * \brief Graphics seam: screen clear, scanlines, palette and hardware
 *        sprites.
 */

#ifndef ATMA_GFX_H
#define ATMA_GFX_H

/**
 * \defgroup gfx Graphics
 * \brief Background fills, the shared palette and hardware sprites.
 *
 * Deliberately minimal: one background clear plus hardware sprites, which
 * every target machine (and Layer 2 on the Next) can do. Richer primitives
 * belong in game code or a helper layer on top.
 * @{
 */

#include "types.h"

/**
 * \name Logical screen
 * \brief The ZX Next's 256x192, the ceiling for every backend.
 *
 * Compile-time constants so games can size loops and clamps without a
 * runtime lookup; atma_platform_info reports the same values.
 * @{
 */
#define ATMA_SCREEN_W 256
#define ATMA_SCREEN_H 192
/** @} */

/** \brief Core-side graphics init, driven by the engine. */
void atma_gfx_init(void);
/** \brief Core-side graphics shutdown, driven by the engine. */
void atma_gfx_shutdown(void);

/**
 * \brief Open the frame, bracketing each render callback.
 *
 * The engine calls this every tick. Backends with a composited display use
 * it to batch register writes or sync with the retraces; on host it may be
 * a no-op.
 *
 * \return Liveness: nonzero when the backend has a display to draw into,
 *         zero when gfx startup failed. The engine calls it once before the
 *         loop starts and treats zero as "the run cannot render": it logs
 *         the backend's error line, tears down in reverse init order and
 *         returns nonzero from atma_run(), instead of looping invisibly
 *         forever. Within the loop the return value is ignored (a backend
 *         that rendered nothing every tick would be a backend bug).
 */
u8 atma_gfx_begin_frame(void);

/** \brief Close the frame opened by atma_gfx_begin_frame(). */
void atma_gfx_end_frame(void);

/**
 * \brief Fill the screen with a palette index.
 * \param color Palette index to fill with.
 */
void atma_gfx_clear(u8 color);

/**
 * \brief Fill one full-width scanline with a palette index.
 *
 * Cheap on every target; enough for gradients and raster bars.
 *
 * \param y Scanline, 0..screen_h-1.
 * \param color Palette index to fill with.
 */
void atma_gfx_hline(u8 y, u8 color);

/**
 * \brief Set one entry of the shared palette.
 *
 * Every color the engine draws is an index into one shared table. Channels
 * arrive as ordinary 8-bit values and are quantized by the engine to the
 * Next's 9-bit RGB333 space (top 3 bits of red, 3 of green, 3 of blue), on
 * every backend: the host is not more capable than the Next, so the window
 * shows byte-for-byte what the hardware shows. Entry 0 is the transparent
 * color for sprites, and transparency is by index: each backend is set up
 * so index 0 itself draws transparent (alpha 0 on the host; the Next's
 * sprite transparency index register, written 0 in atma_gfx_init),
 * leaving entry 0's RGB free for any value. Set the palette before
 * drawing anything that depends on it (call from the game's init
 * callback).
 *
 * Entries are live state, not upload-time constants: changing one recolors
 * every drawn pixel that references the index, sprites included, on the
 * next frame, on every platform. Palette animation is therefore portable.
 *
 * \param index Palette slot, 0..255.
 * \param r Red channel, 8-bit.
 * \param g Green channel, 8-bit.
 * \param b Blue channel, 8-bit.
 */
void atma_gfx_palette_set(u8 index, u8 r, u8 g, u8 b);

/** \brief Bytes in one 16x16 indexed sprite pattern. */
#define ATMA_PATTERN_BYTES (16 * 16)

/**
 * \brief Upload one 16x16 sprite pattern from indexed bytes.
 *
 * Exactly ATMA_PATTERN_BYTES values (one per pixel), each a palette index as
 * set with atma_gfx_palette_set(). Value 0 draws transparent. This is the
 * engine's native sprite atom (the Next's 16x16 hardware sprite); larger
 * images are a grid of patterns.
 *
 * \param pattern Pattern id to write.
 * \param pixels ATMA_PATTERN_BYTES palette indices, row-major.
 * \retval 0 Success.
 * \retval -1 Failure; the backend logs one line.
 */
int atma_gfx_pattern_set_indexed(u8 pattern, const u8 *pixels);

/**
 * \name Sprite attribute flags
 * \brief Backend-defined attribute bits passed to atma_gfx_sprite_set().
 * @{
 */
#define ATMA_SPR_MIRROR_X 0x08
#define ATMA_SPR_MIRROR_Y 0x04
/** 90 degrees clockwise, applied before mirroring. */
#define ATMA_SPR_ROTATE   0x02
/** @} */

/**
 * \name Hardware limits
 * \brief Limits every backend honours; the Next is the ceiling.
 *
 * ATMA_MAX_PATTERNS bounds engine pattern ids; on the Next, 8-bit patterns
 * cap at 64 (ATMA_MAX_PATTERNS_8BIT) and 4-bit at 128. Sprite slots are
 * capped at 64 too: the useful set on the Next (64 patterns, the
 * 64-per-scanline limit), even though the slot register itself addresses
 * 128. A backend must fail (return -1, log one line) rather than silently
 * drop an upload beyond what it stores.
 * @{
 */
#define ATMA_MAX_SPRITES  64
#define ATMA_MAX_PATTERNS 256
#define ATMA_MAX_PATTERNS_8BIT 64
/** @} */

/**
 * \brief Place a hardware sprite.
 *
 * \param slot Backend sprite index (the Next has 64 hardware sprites).
 * \param x Position in screen pixels; i16 allows partially off-screen
 *          placements. Backends decide clipping/coordinates off-screen.
 * \param y Position in screen pixels; see \p x.
 * \param pattern Which bitmap to show, in the backend's pattern/attribute
 *        table.
 * \param flags Backend-defined attribute bits (mirror, rotate, layer, ...);
 *        see the ATMA_SPR_* flags.
 */
void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags);

/**
 * \brief Hide a hardware sprite.
 * \param slot Backend sprite index.
 */
void atma_gfx_sprite_hide(u8 slot);

/** @} */

#endif
