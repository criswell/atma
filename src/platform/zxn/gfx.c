/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <arch/zxn.h>
#include <string.h>
#include "atma/engine.h"
#include "atma/gfx.h"
#include "atma/platform.h"

/*
 * ZX Next graphics backend.
 *
 * Layer 2 (256x192, 8bpp) is the background: atma_gfx_clear and
 * atma_gfx_hline write palette indices straight into the three 16K banks the
 * mode occupies (9, 10, 11). The CPU has no permanent window onto Layer 2
 * memory, so writes page one 8K page at a time into MMU slot 1
 * (0x2000-0x3FFF). That slot is ROM in the 48K model this backend is built
 * for, and the engine never calls into ROM, so nothing is restored
 * afterwards. A scanline is 256 bytes and a page is 8192, so no hline ever
 * straddles a page boundary.
 *
 * Hardware sprites back atma_gfx_sprite_set: 64 slots, 16x16, 8bpp, with 16K
 * of pattern memory = 64 patterns of 256 bytes, exactly what
 * ATMA_MAX_PATTERNS_8BIT advertises. Patterns hold palette indices and the
 * hardware resolves them against the live sprite palette every frame, so
 * atma_gfx_palette_set recolors already-drawn sprites immediately, matching
 * the host backend's hot-swap behaviour.
 */

/* Layer 2 control (NextReg 0x69) is not in z88dk's arch/zxn.h. */
#define ZXN_REG_LAYER2_CONTROL 0x69
#define L2_ENABLE 0x80

/* 256x192 8bpp Layer 2 lives in 16K banks 9, 10 and 11 (48K total). */
#define L2_BANK_FIRST 9
#define L2_PAGE_BYTES 8192

/* MMU slot 1: 0x2000-0x3FFF, one 8K Layer 2 page at a time. */
#define L2_WINDOW ((u8 *)0x2000)

/* Map the 8K page holding `offset` into the Layer 2 window. */
static void l2_map(u16 offset)
{
    u8 bank = (u8)(offset >> 14);
    u8 page = (u8)((offset >> 13) & 1);
    ZXN_WRITE_REG(REG_MMU1, (u8)(((L2_BANK_FIRST + bank) << 1) | page));
}

/* One 9-bit RGB333 palette entry, written as the two-byte value form so the
 * low blue bit survives: first byte RRRGGGBB, second byte bit 0 = blue LSB. */
static void palette_entry(u8 control, u8 index, u8 r, u8 g, u8 b)
{
    ZXN_WRITE_REG(REG_PALETTE_CONTROL, control);
    ZXN_WRITE_REG(REG_PALETTE_INDEX, index);
    ZXN_WRITE_REG(REG_PALETTE_VALUE_16,
                  (u8)(((r >> 5) << 5) | (((g >> 5) & 7) << 2) | ((b >> 6) & 3)));
    ZXN_WRITE_REG(REG_PALETTE_VALUE_16, (u8)((b >> 5) & 1));
}

void atma_gfx_init(void)
{
    u16 i;

    /* 256x192 8bpp Layer 2, enabled, drawn over the ULA. */
    ZXN_WRITE_REG(ZXN_REG_LAYER2_CONTROL, L2_ENABLE);

    /* Sprite layer on, sprites over Layer 2 over ULA, sprites over border. */
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER |
                  RSLS_LAYER_PRIORITY_SLU);

    /* Index 0 is the engine-wide transparent color for sprites. */
    ZXN_WRITE_REG(REG_SPRITE_TRANSPARENCY_INDEX, 0);

    /* Power-on palette: RGB332, the same table the host backend starts from,
     * so a game that sets no palette still looks identical on both. */
    for (i = 0; i < 256; ++i) {
        atma_gfx_palette_set((u8)i,
                             (u8)(((i >> 5) & 7) * 255 / 7),
                             (u8)(((i >> 2) & 7) * 255 / 7),
                             (u8)((i & 3) * 255 / 3));
    }

    atma_gfx_clear(0);
}

void atma_gfx_shutdown(void) {}

u8 atma_gfx_begin_frame(void) { return 1; }
void atma_gfx_end_frame(void) {}

void atma_gfx_clear(u8 color)
{
    u8 bank;

    /* Three 16K banks, two 8K pages each: six window-sized fills. */
    for (bank = 0; bank < 3; ++bank) {
        ZXN_WRITE_REG(REG_MMU1, (u8)(((L2_BANK_FIRST + bank) << 1) | 0));
        memset(L2_WINDOW, color, L2_PAGE_BYTES);
        ZXN_WRITE_REG(REG_MMU1, (u8)(((L2_BANK_FIRST + bank) << 1) | 1));
        memset(L2_WINDOW, color, L2_PAGE_BYTES);
    }
}

void atma_gfx_hline(u8 y, u8 color)
{
    u16 offset = (u16)((u16)y << 8);

    l2_map(offset);
    memset(L2_WINDOW + (offset & (L2_PAGE_BYTES - 1)), color, ATMA_SCREEN_W);
}

void atma_gfx_palette_set(u8 index, u8 r, u8 g, u8 b)
{
    /* Layer 2 and sprites have separate 256-entry palettes on the Next; the
     * engine exposes one table, so both get the entry. */
    palette_entry(RPC_SELECT_LAYER_2_PALETTE_0, index, r, g, b);
    palette_entry(RPC_SELECT_SPRITES_PALETTE_0, index, r, g, b);
}

void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags)
{
    u16 ux = (u16)x;
    u16 uy = (u16)y;

    if (slot >= ATMA_MAX_SPRITES) return;

    /* Select the slot, then stream the five attribute bytes: X LSB, Y LSB,
     * flags + X MSB, visible + pattern, Y MSB. The engine's ATMA_SPR_MIRROR_X
     * / _Y / _ROTATE bits already sit on the register's bits 3 / 2 / 1. */
    IO_SPRITE_SLOT = slot;
    IO_SPRITE_ATTRIBUTE = (u8)(ux & 0xFF);
    IO_SPRITE_ATTRIBUTE = (u8)(uy & 0xFF);
    IO_SPRITE_ATTRIBUTE = (u8)(((ux >> 8) & 1) |
                               (flags & (ATMA_SPR_MIRROR_X |
                                         ATMA_SPR_MIRROR_Y |
                                         ATMA_SPR_ROTATE)));
    IO_SPRITE_ATTRIBUTE = (u8)(0x80 | 0x40 | (pattern & 0x3F));
    IO_SPRITE_ATTRIBUTE = (u8)((uy >> 8) & 1);
}

void atma_gfx_sprite_hide(u8 slot)
{
    u8 i;

    if (slot >= ATMA_MAX_SPRITES) return;

    IO_SPRITE_SLOT = slot;
    for (i = 0; i < 5; ++i) IO_SPRITE_ATTRIBUTE = 0;
}

int atma_gfx_pattern_set_indexed(u8 pattern, const u8 *pixels)
{
    u16 i;

    if (!pixels) return -1;
    if (pattern >= ATMA_MAX_PATTERNS_8BIT) {
        atma_log("zxn: pattern id beyond the 64 the hardware stores");
        return -1;
    }

    /* Port 0x303B selects the pattern as well as the sprite slot, and resets
     * the pattern pointer to the start of that pattern. Port 0x5B is then
     * pure data: every write stores a byte and auto-increments, rolling over
     * from pattern 63 back to 0. There is no address latch, so the pattern
     * number must come from 0x303B - writing two address bytes to 0x5B would
     * store them as the first two pixels. */
    IO_SPRITE_SLOT = pattern;
    for (i = 0; i < ATMA_PATTERN_BYTES; ++i) IO_SPRITE_PATTERN = pixels[i];

    return 0;
}
