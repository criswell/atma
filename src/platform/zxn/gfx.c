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

static u8 slot1_mapped_page = 0xFF; /* none yet */

/**
 * \brief Map Layer 2 page \p page (bank*2 + half, 0..5) into the Layer 2
 * window, skipping the register write when the same absolute page already
 * sits there. The cache assumes atma owns slot 1 for the whole run: any
 * code that pages slot 1 must go through here, always with a value built
 * the way l2_map builds it (the register wants the absolute 8K RAM page,
 * L2_BANK_FIRST * 2 and up).
 * \param page Layer 2 page number: bank * 2 + half, relative to
 *        L2_BANK_FIRST.
 */
static void l2_map_page(u8 page)
{
    u8 mmu = (u8)((L2_BANK_FIRST << 1) + page);
    if (mmu == slot1_mapped_page) return;
    slot1_mapped_page = mmu;
    ZXN_WRITE_REG(REG_MMU1, mmu);
}

/**
 * \brief Map the 8K page holding \p offset into the Layer 2 window.
 * \param offset Byte offset into the 48K Layer 2 area.
 */
static void l2_map(u16 offset)
{
    l2_map_page((u8)(((offset >> 14) * 2) | ((offset >> 13) & 1)));
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

    /* Sprite transparency: boot value until the game's palette arrives. */
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

    /* Three 16K banks, two 8K pages each: six window-sized fills. Routed
     * through l2_map_page so the mapping cache stays honest. */
    for (bank = 0; bank < 3; ++bank) {
        l2_map_page((u8)(bank << 1));
        memset(L2_WINDOW, color, L2_PAGE_BYTES);
        l2_map_page((u8)((bank << 1) | 1));
        memset(L2_WINDOW, color, L2_PAGE_BYTES);
    }
}

void atma_gfx_pixel(i16 x, i16 y, u8 color)
{
    u16 offset;

    /* Clip before converting to unsigned: negative coordinates must not
     * wrap around into Layer 2 memory. */
    if (x < 0 || x >= ATMA_SCREEN_W ||
        y < 0 || y >= ATMA_SCREEN_H) {
        return;
    }

    /* Layer 2 is 256 pixels wide, so each row starts 256 bytes after
     * the previous one. */
    offset = (u16)((u16)y * ATMA_SCREEN_W + (u16)x);

    /* Map the 8K Layer 2 page containing this pixel into the CPU window,
     * then write at its offset within that page. */
    l2_map(offset);
    L2_WINDOW[offset & (L2_PAGE_BYTES - 1)] = color;
}

/* SDCC passes params on the stack: src at ix+4, dst at ix+6, n at ix+8.
 * Caller cleans the stack. The #asm block must live inside the function
 * body (SDCC rejects file-scope asm), and SDCC's enter_ix prologue has
 * already pushed ix and set ix = sp when the block runs, so the block
 * uses that frame directly and falls through to SDCC's pop ix / ret. */
static void l2_row_copy(const u8 *src, u8 *dst, u16 n)
{
    src; dst; n;
#asm
    ld l,(ix+4)
    ld h,(ix+5)    ; hl = src
    ld e,(ix+6)
    ld d,(ix+7)    ; de = dst
    ld c,(ix+8)
    ld b,(ix+9)    ; bc = n
    ld a,b
    or c
    jr z,l2rc_done ; LDIRX with bc=0 would copy 64K
    xor a          ; skip-on-0: LDIRX does not write bytes equal to A
    ldirx
.l2rc_done
#endasm
}

void atma_gfx_blit(i16 x, i16 y, u8 w, u8 h, const u8 *pixels)
{
    u16 row;
    const u8 *src;

    if (!pixels || !w || !h) return;

    src = pixels;
    for (row = 0; row < h; ++row, src += w) {
        i32 dst_y = (i32)y + (i32)row;
        i32 src_x = 0;
        i32 dst_x = (i32)x;
        i32 count;
        u16 dst_offset;
        u8 *dst;

        if (dst_y < 0 || dst_y >= ATMA_SCREEN_H) continue;

        /* Clip the left edge, advancing the source start as needed. */
        if (dst_x < 0) {
            src_x = -dst_x;
            dst_x = 0;
        }

        if (dst_x >= ATMA_SCREEN_W || src_x >= w) continue;

        count = (i32)w - src_x;
        if (count > ATMA_SCREEN_W - dst_x)
            count = ATMA_SCREEN_W - dst_x;
        if (count <= 0) continue;

        dst_offset = (u16)((u32)dst_y * ATMA_SCREEN_W + (u32)dst_x);
        l2_map(dst_offset);
        dst = L2_WINDOW + (dst_offset & (L2_PAGE_BYTES - 1));

        /* The whole per-pixel loop, skip-on-0 included, is one LDIRX.
         * src must not live in MMU slot 1 (0x2000-0x3FFF): the helper's
         * DE writes through the same window its HL reads. Nothing links
         * there (see the file comment), so that holds by construction.
         * LDIRX is interruptible mid-copy; safe today because the engine
         * ISR never writes slot 1. */
        l2_row_copy(src + (u16)src_x, dst, (u16)count);
    }
}

void atma_gfx_mask(i16 x, i16 y, u8 w, u8 h, const u8 *mask, u8 ink)
{
    u16 row;
    u16 stride;
    const u8 *mrow;

    if (!mask || !w || !h) return;

    stride = (u16)((w + 7) >> 3);
    mrow = mask;
    for (row = 0; row < h; ++row, mrow += stride) {
        i32 dst_y = (i32)y + (i32)row;
        i32 src_bit = 0;
        i32 dst_x = (i32)x;
        i32 count;
        u16 dst_offset;
        u16 col;
        u8 *dst;

        if (dst_y < 0 || dst_y >= ATMA_SCREEN_H) continue;
        if (dst_x < 0) { src_bit = -dst_x; dst_x = 0; }
        if (dst_x >= ATMA_SCREEN_W || src_bit >= (i32)w) continue;

        count = (i32)w - src_bit;
        if (count > ATMA_SCREEN_W - dst_x)
            count = ATMA_SCREEN_W - dst_x;
        if (count <= 0) continue;

        dst_offset = (u16)((u32)dst_y * ATMA_SCREEN_W + (u32)dst_x);
        l2_map(dst_offset);
        dst = L2_WINDOW + (dst_offset & (L2_PAGE_BYTES - 1));

        for (col = 0; col < (u16)count; ++col) {
            u16 bit = (u16)src_bit + col;
            if (mrow[bit >> 3] & (0x80 >> (bit & 7))) dst[col] = ink;
        }
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
