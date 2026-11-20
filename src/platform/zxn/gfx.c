/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file gfx.c
 * \brief ZX Next graphics backend: Layer 2 background and hardware sprites.
 * \ingroup gfx
 */

#include <arch/zxn.h>
#include <string.h>
#include "atma/engine.h"
#include "atma/gfx.h"
#include "atma/platform.h"

/**
 * \brief ZX Next graphics backend.
 *
 * Layer 2 (256x192, 8bpp) is the background: atma_gfx_clear and
 * atma_gfx_hline write palette indices straight into the three 16K banks the
 * mode occupies (8, 9, 10). The CPU has no permanent window onto Layer 2
 * memory, so writes page one 8K page at a time into MMU slot 1
 * (0x2000-0x3FFF). Nothing this program links lives there: the crt places
 * code from 0x8000 up (verified against the link map), the crt starts with
 * interrupts disabled so nothing will ever vector through that area, and the
 * program does not return to the boot environment mid-run, so the slot is
 * never restored. A scanline is 256 bytes and a page is 8192, so no hline
 * ever straddles a page boundary.
 *
 * Hardware sprites back atma_gfx_sprite_set: 64 slots, 16x16, 8bpp, with 16K
 * of pattern memory = 64 patterns of 256 bytes, exactly what
 * ATMA_MAX_PATTERNS_8BIT advertises. Patterns hold palette indices and the
 * hardware resolves them against the live sprite palette every frame, so
 * atma_gfx_palette_set recolors already-drawn sprites immediately, matching
 * the host backend's hot-swap behaviour.
 */

/** Layer 2 control (NextReg 0x69) is not in z88dk's arch/zxn.h. */
#define ZXN_REG_LAYER2_CONTROL 0x69
#define L2_ENABLE 0x80 /**< Layer 2 enable bit in NextReg 0x69. */

/**
 * \brief First Layer 2 bank.
 *
 * 256x192 8bpp Layer 2 lives in 16K banks 8, 9 and 10 (48K total). This is
 * the power-on state (NextReg 0x12 resets to 8, shadow NextReg 0x13 to 11),
 * but it is NOT inherited at run time: the .nex loader re-pages Layer 2
 * before the program runs (official nexload.asm and ZEsarUX's loader both
 * write NextReg 0x12 = 9, shadow = 12). atma_gfx_init therefore writes the
 * register itself, so the display always reads the banks the engine writes.
 * With banks 8/9/10 selected and the loader's 9/10/11 in force, the picture
 * shows the frame scrolled one full bank (64 rows) up and the bottom third
 * reads unwritten RAM; verified byte-level in ZEsarUX 2026-11-20.
 */
#define L2_BANK_FIRST 8
#define L2_PAGE_BYTES 8192 /**< Bytes per MMU page. */

/** MMU slot 1: 0x2000-0x3FFF, one 8K Layer 2 page at a time. */
#define L2_WINDOW ((u8 *)0x2000)

/**
 * \brief Map the 8K page holding \p offset into the Layer 2 window.
 * \param offset Byte offset into the 48K Layer 2 area.
 */
static void l2_map(u16 offset)
{
    u8 bank = (u8)(offset >> 14);
    u8 page = (u8)((offset >> 13) & 1);
    ZXN_WRITE_REG(REG_MMU1, (u8)(((L2_BANK_FIRST + bank) << 1) | page));
}

/**
 * \brief Write one 9-bit RGB333 palette entry.
 *
 * Written as the two-byte value form so the low blue bit survives: first
 * byte RRRGGGBB, second byte bit 0 = blue LSB.
 *
 * \param control Palette control value selecting the target palette.
 * \param index Entry to write.
 * \param r Red channel, 8-bit.
 * \param g Green channel, 8-bit.
 * \param b Blue channel, 8-bit.
 */
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

    /* Own the Layer 2 banking. The .nex loader leaves NextReg 0x12 = 9
     * (Layer 2 on banks 9/10/11, per the official nexload reset sequence),
     * but this engine pages Layer 2 as banks 8/9/10. Write the register
     * so the display reads the banks l2_map()/atma_gfx_clear() write. */
    ZXN_WRITE_REG(REG_LAYER_2_RAM_PAGE, L2_BANK_FIRST);

    /* 256x192 8bpp Layer 2, enabled, drawn over the ULA. */
    ZXN_WRITE_REG(ZXN_REG_LAYER2_CONTROL, L2_ENABLE);

    /* Sprite layer on, sprites over Layer 2 over ULA, sprites over border. */
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER |
                  RSLS_LAYER_PRIORITY_SLU);

    /* Sprite transparency: boot value until the game's palette arrives.
     * Entry 0's own color drives this register from then on (see
     * atma_gfx_palette_set). */
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

    /* Sprite transparency on the Next is a color match, not an index match:
     * NextReg 0x4B holds one 8-bit RRRGGGBB value, and every sprite-palette
     * entry whose top 8 bits equal it draws transparent (so a value always
     * covers two RGB333 entries). Deriving the register from entry 0's own
     * color keeps the engine's index-0 rule exact on hardware, provided
     * entry 0's 8-bit color is unique among the palette's used entries;
     * mkpats guarantees that when it generates assets. */
    if (index == 0) {
        ZXN_WRITE_REG(REG_SPRITE_TRANSPARENCY_INDEX,
                      (u8)(((r >> 5) << 5) | (((g >> 5) & 7) << 2)
                           | ((b >> 6) & 3)));
    }
}

void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags)
{
    /* The sprite coordinate system is border-based 320x256 even when
     * sprites never draw on the border: screen (0, 0) is attribute
     * (32, 32), so add the border margin before splitting into attribute
     * bytes. Partially off-screen placements keep working: screen -16
     * becomes border 16, just left of the visible area. */
    u16 ux = (u16)(x + 32);
    u16 uy = (u16)(y + 32);

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

    /* Four bytes, not five: byte 4 written with the E bit clear marks a
     * complete four-byte attribute record, and the hardware's attribute
     * pointer advances to the next slot right here. A fifth write would
     * land in that slot's X LSB and corrupt it. */
    IO_SPRITE_SLOT = slot;
    for (i = 0; i < 4; ++i) IO_SPRITE_ATTRIBUTE = 0;
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
