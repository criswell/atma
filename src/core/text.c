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
 *
 * A NULL font, an out-of-range cell, or a count with no glyph array
 * fails loudly: one atma_log and nothing is drawn. Silent clamping
 * would corrupt a smaller-than-max font's rendering or crash on the
 * deref; the fail-loudly rule trades that for a diagnosable no-op.
 */
void atma_gfx_text(const atma_font *font, i16 x, i16 y, const char *s, u8 ink)
{
    u8 c;
    if (!font) {
        atma_log("text: font is NULL");
        return;
    }
    if (font->w < 1 || font->w > ATMA_FONT_W_MAX ||
        font->h < 1 || font->h > ATMA_FONT_H_MAX) {
        atma_log("text: font cell out of range (w 1..8, h 1..16)");
        return;
    }
    if (font->count && !font->glyphs) {
        atma_log("text: font has glyphs but no glyph array");
        return;
    }
    for (; *s; ++s) {
        c = (u8)*s;
        if (c < font->first || (u8)(c - font->first) >= font->count) {
            x += font->w;
            continue;
        } else {
           atma_gfx_mask(x, y, font->w, font->h,
              font->glyphs + (u8)(c - font->first) * font->h, ink);
        }
        x += font->w;
    }
}
