/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file text.c
 * \brief Core text library.
 * \ingroup engine
 */

#include "atma/engine.h"

/**
 * \brief Scratch stencil for one glyph mid-unpack, reused per character.
 *
 * Sized to the format's largest possible cell (ATMA_FONT_W_MAX x
 * ATMA_FONT_H_MAX, 128 bytes). Holds {0, ink} indexed bytes between
 * unpacking a glyph's packed rows and the blit that draws them; nothing
 * survives the call, so concurrent fonts share it safely. Fonts
 * themselves live in game data via atma_font; this is never storage.
 */
static u8 glyph_buf[ATMA_FONT_W_MAX * ATMA_FONT_H_MAX];

/**
 * \brief Text renderer backing atma_gfx_text() (API contract in gfx.h).
 *
 * One glyph at a time: unpack the glyph's packed rows from
 * font->glyphs into glyph_buf as {0, ink} indexed bytes using the
 * font's own cell (font->w x font->h, stride font->w), then one
 * atma_gfx_blit per character. Blit's skip-on-0 rule makes cleared
 * bits invisible, so the caller's background shows between ink pixels.
 *
 * The per-character cost is one blit, never one pixel call: the blit
 * keeps the Layer 2 page mapping inside the backend, which is the
 * whole reason text goes through it (see the blit rationale in gfx.h).
 * Bounds come from the font struct, so a 6x8 and an 8x16 font render
 * correctly through this one implementation.
 */
void atma_gfx_text(const atma_font *font, i16 x, i16 y, const char *s, u8 ink)
{
    u8 c;
    for (; *s; ++s) {
        c = (u8)*s;
        if (c < font->first || (u8)(c - font->first) >= font->count) {
            x += font->w;
            continue;
        } else {
            const u8 *g = font->glyphs + (u8)(c - font->first) * font->h;
            u8 row, px;
            for (row = 0; row < font->h; ++row) {
                for (px = 0; px < font->w; ++px) {
                    glyph_buf[row * font->w + px] =
                        (g[row] & (0x80 >> px)) ? ink : 0;
                }
            }
            atma_gfx_blit(x, y, font->w, font->h, glyph_buf);
        }
        x += font->w;
    }
}
