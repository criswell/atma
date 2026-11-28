/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file gfx.h
 * \brief Graphics seam: screen clear, scanlines, palette, hardware
 *        sprites, background pixel/blit and the text renderer's font
 *        format.
 */

#ifndef ATMA_GFX_H
#define ATMA_GFX_H

/**
 * \defgroup gfx Graphics
 * \brief Background fills, pixel/blit writes, the shared palette,
 *        hardware sprites and text.
 *
 * Deliberately minimal: background fills, two background write
 * primitives (pixel, blit) and hardware sprites, which every target
 * machine (and Layer 2 on the Next) can do. Richer primitives belong in
 * game code or a helper layer on top; the text renderer (src/core/text.c)
 * is the first such helper, built on the blit, with its font format
 * defined below.
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

/**
 * \name Font format limits
 * \brief Bounds of the engine's font format, not of the engine.
 *
 * The atma_font cell can be at most 8 pixels wide (one packed byte per
 * row, bit 7 leftmost) and 16 rows tall. These caps exist to size the
 * text renderer's glyph buffer; anything larger should be an art asset,
 * not text. The renderer bounds every loop with the font's own w and h,
 * so fonts smaller than the caps use only the buffer they need.
 * @{
 */
#define ATMA_FONT_W_MAX 8
#define ATMA_FONT_H_MAX 16
/** @} */

/** \brief Bytes in one 8x8, 4bpp tile. */
#define ATMA_TILE_BYTES (8 * 4)

/**
 * \name Tilemap geometry
 * \brief Fixed engine-owned tilemap dimensions.
 * @{
 */
#define ATMA_TILEMAP_W 40
#define ATMA_TILEMAP_H 32
#define ATMA_TILEMAP_VISIBLE_W 32
#define ATMA_TILEMAP_VISIBLE_H 24
#define ATMA_TILEMAP_RING 4
#define ATMA_MAX_TILES 256
/** @} */

/**
 * \name Tilemap entry flags
 * \brief Portability flags stored in the entry's high byte.
 *
 * Entry bits:
 * - 15-12: palette offset
 * - 11: mirror X
 * - 10: mirror Y
 * - 9: rotate 90 degrees clockwise
 * - 8: reserved, must be zero
 * - 7-0: tile number
 *
 * Compose static entries with these macros. Write 0 to bit 8.
 * Transforms apply rotation first, then mirroring, matching the
 * sprite attribute order (ATMA_SPR_ROTATE). The palette offset is
 * masked to 4 bits; values beyond 15 lose their higher bits.
 * @{
 */
#define ATMA_TILE_PALETTE_OFFSET(o) (((u16)(o) & 0x0F) << 12)
#define ATMA_TILE_MIRROR_X 0x0800
#define ATMA_TILE_MIRROR_Y 0x0400
#define ATMA_TILE_ROTATE   0x0200
#define ATMA_TILE_BLANK    0x0000
/** @} */

/**
 * \brief Pixel font type definition
 *
 * A font in the engine's text format, provided as game data (the engine
 * ships none; font8x8, converted by tools/font8x8_convert.py, is the
 * demo's font). The format:
 *
 * - glyph data is one packed byte per row, bit 7 = leftmost pixel,
 *   bit 0 = rightmost; set bit = ink pixel, clear bit = background
 *   shows through;
 * - one glyph occupies h row-bytes, glyphs laid out glyph-major
 *   (consecutive rows of one glyph together, then the next glyph), so
 *   glyph G's first row is glyphs[(G - first) * h];
 * - w is the used width, 1..8: a 6-wide font uses the leftmost 6 bits
 *   of each row byte and advances the pen 6 pixels per character.
 *
 * \param glyphs count * h row bytes, glyph-major, MSB-left.
 * \param first Char code of glyphs[0] (usually 32, space).
 * \param count Number of glyphs (95 covers printable ASCII).
 * \param w Cell width in pixels, 1..ATMA_FONT_W_MAX; also the pen advance.
 * \param h Cell height in rows, 1..ATMA_FONT_H_MAX.
 */
typedef struct {
    const u8 *glyphs;   /**< count * h row bytes, glyph-major, MSB-left */
    u8 first;           /**< Char code of glyphs[0] (usually 32, space). */
    u8 count;           /**< Glyph count; 95 covers printable ASCII. */
    u8 w;               /**< Cell width in pixels, 1..ATMA_FONT_W_MAX. */
    u8 h;               /**< Cell height in rows, 1..ATMA_FONT_H_MAX. */
} atma_font;

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
 *
 * Writes the background layer, wherever the call comes from: on every
 * backend this is persistent memory the display keeps reading, so a fill
 * in the init callback survives all frames until overdrawn.
 *
 * \param color Palette index to fill with.
 */
void atma_gfx_clear(u8 color);

/**
 * \brief Fill one full-width scanline with a palette index.
 *
 * Cheap on every target; enough for gradients and raster bars. Like
 * atma_gfx_clear, writes persistent background memory.
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

/** \brief Set one background pixel to a palette index.
 *
 *  Writes persistent background memory, like clear/hline. Coordinates
 *  off-screen are clipped silently (the sprite_set guard convention).
 */
void atma_gfx_pixel(i16 x, i16 y, u8 color);

/** \brief Write a 1-bit mask rectangle in a single colour.
 *
 *  \param mask Row-major bits, MSB-left: bit 7 of the first byte is the
 *         top-left pixel, each row packed to ceil(w/8) bytes. This is the
 *         atma_font glyph format (font8x8.h:10-14), so font rows pass
 *         directly. Padding bits in a row's last byte are ignored.
 *  \param ink Palette index written where a bit is 1. Unlike blit, 0 is
 *         allowed here: this is the only way to erase through a shape.
 *  Bits that are 0 leave the background untouched. Clipped to the
 *  screen, like blit.
 */
void atma_gfx_mask(i16 x, i16 y, u8 w, u8 h, const u8 *mask, u8 ink);

/** \brief Write a rectangle of palette indices into the background.
 *
 *  \param pixels Row-major indices, w*h entries. Index 0 means "skip":
 *         the background pixel is left untouched, the same transparency
 *         convention sprites use (gfx.h:92-96). Any other index overwrites.
 *  \param w, h  Rectangle size, up to 255 each; compose a full-width
 *         fill from hline.
 *  Rectangles are clipped to the screen; a wholly off-screen blit draws
 *  nothing.
 */
void atma_gfx_blit(i16 x, i16 y, u8 w, u8 h, const u8 *pixels);

/** \brief Draw one line of text in the engine font format.
 *
 *  Ink-only: glyph pixels are written, background pixels are skipped,
 *  so text sits on whatever the background already shows. Characters
 *  outside the font's first..first+count range advance the pen without
 *  drawing. No newline handling: one call, one line, caller loops.
 *
 *  Text writes persistent background memory: draw once from init for a
 *  fixed label, or from render() for a changing score. Redrawing over
 *  old text without erasing leaves ghost pixels; erase first with
 *  hline() rows of the background index (blit itself can never write
 *  index 0: 0 always means skip).
 *
 *  The font is passed explicitly on every call, so any number of fonts
 *  coexist; each renders through its own w and h. Monospace by
 *  construction: every character advances the pen one cell width.
 *
 *  A NULL font, an out-of-range cell, or a count with no glyph array
 *  fails loudly: one atma_log and nothing is drawn. The 8-bit machine
 *  has no stack unwinding to lean on; a corrupt font must not become
 *  silent memory corruption.
 *
 *  \param font Font to draw with (format and limits above).
 *  \param x, y Screen position of the first glyph's top-left pixel.
 *  \param s NUL-terminated string; draw stops at the NUL.
 *  \param ink Palette index drawn for set bits.
 */
void atma_gfx_text(const atma_font *font, i16 x, i16 y, const char *s, u8 ink);

/**
 * \brief Upload one 8x8, 4bpp tile definition.
 *
 * Tile id 0 is the engine convention for the blank tile; it has no
 * reserved special status. Exactly ATMA_TILE_BYTES bytes are read:
 * eight rows of four bytes, top to bottom. Within each byte, the high
 * nibble is the left pixel and the low nibble is the right pixel.
 *
 * Pixel value 0 is transparent on every backend. On the ZX Next, the
 * engine writes NextReg 0x4C so pixel index 0 is the transparent
 * index, checked before the entry's palette offset joins the lookup:
 * dot 0 is transparent under every offset, and no other dot can reach
 * it. Other nonzero nibbles are 4-bit palette indices; the tilemap
 * entry's palette-offset nibble supplies the high 4 bits.
 *
 * atma_gfx_init() zeroes the engine-owned tile definitions and map.
 * Un-uploaded tiles are therefore blank: a cell referencing an
 * un-uploaded tile draws nothing, not garbage. Tile 0 begins
 * all-transparent, so the zeroed map shows nothing; upload your
 * blank there or leave it.
 *
 * \param tile Tile id, 0..255.
 * \param pixels ATMA_TILE_BYTES bytes, row-major, or NULL to fail.
 * \retval 0 Success.
 * \retval -1 Failure; the backend logs one line.
 */
int atma_gfx_tile_set(u8 tile, const u8 *pixels);

/**
 * \brief Write one tilemap cell.
 *
 * Coordinates are tilemap-local screen-cell coordinates. (0,0) is the
 * top-left visible cell. The engine-owned 40x32 map also includes a
 * 4-cell ring around the visible 32x24 screen, with ring coordinates
 * from -4 through 35 and -4 through 27.
 *
 * The entry uses the encoding documented at the ATMA_TILE_* macros:
 * one u16, tile number in the low byte, attributes above. Entry 0 is
 * tile 0 with no attributes and is written normally; there is no
 * skip-on-zero behavior.
 *
 * The write is clipped silently to the 40x32 backing map; a wholly
 * out-of-map cell does nothing.
 *
 * \param col Destination map column, screen-cell coordinates.
 * \param row Destination map row, screen-cell coordinates.
 * \param entry Tilemap entry: tile number with ATMA_TILE_* attributes.
 */
void atma_gfx_tilemap_cell(i16 col, i16 row, u16 entry);

/**
 * \brief Write a rectangular group of tilemap cells.
 *
 * \p entries is row-major: entries[row * w + col], col changing fastest.
 * Destination coordinates are screen-cell coordinates, i16 allows the
 * ring and partially off-map rectangles to be used naturally.
 *
 * Clipping adjusts each row's start and visible width, but the source
 * stride remains the original \p w. This is a sub-rectangle of the
 * source, not a re-sliced rectangle. Writes clip to the 40x32 backing
 * map, not to the 32x24 visible screen.
 *
 * Entry 0 is a real cell containing tile 0 and attribute 0; it always
 * overwrites. A wholly out-of-map rectangle does nothing.
 *
 * \param col Destination left edge, in screen-cell coordinates.
 * \param row Destination top edge, in screen-cell coordinates.
 * \param w Source rectangle width in cells.
 * \param h Source rectangle height in cells.
 * \param entries Row-major tilemap entries, w*h values.
 */
void atma_gfx_tilemap_cells(i16 col, i16 row, u8 w, u8 h,
                            const u16 *entries);

/**
 * \brief Enable or disable the tilemap layer.
 *
 * The tilemap starts enabled at init, showing the fully transparent
 * zeroed map; this call exists for debugging and for games that want
 * the tilemap's fetch bandwidth back.
 *
 * \param on Nonzero enables the tilemap; zero disables it.
 */
void atma_gfx_tilemap_enable(u8 on);

/** @} */

#endif
