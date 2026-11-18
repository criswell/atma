/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <arch/zxn.h>
#include <z80.h>
#include "atma/input.h"

/*
 * ZX Next input backend. Two sources collapse into the same ATMA_BTN_*
 * bitmask: the Kempston joystick on port 0x1F (five active-low bits) and the
 * Spectrum keyboard, scanned a row at a time through port 0xFE with one bit
 * of A15..A8 pulled low to select the row.
 *
 * Keyboard mapping follows the usual Spectrum convention - Q/A/O/P for the
 * four directions, SPACE to fire, ENTER to start - and BREAK (CAPS SHIFT +
 * SPACE) is the quit gesture the engine's ATMA_BTN_QUIT check expects.
 */

#define KEMPSTON_PORT 0x1F
#define KEMP_RIGHT 0x01
#define KEMP_LEFT  0x02
#define KEMP_DOWN  0x04
#define KEMP_UP    0x08
#define KEMP_FIRE  0x10

/* Read one keyboard half-row; bits 0-4 come back active low. */
static u8 kbd_row(u8 row)
{
    u16 port = (u16)(0xFE | ((u16)(~(1u << row) & 0xFF) << 8));
    return (u8)(z80_inp(port) & 0x1F);
}

#define KEY_DOWN(row, bit) ((kbd_row(row) & (1u << (bit))) == 0)

void atma_input_backend_init(void) {}
void atma_input_backend_shutdown(void) {}

u16 atma_input_backend_read(void)
{
    u16 b = 0;
    u8 kempston = (u8)~z80_inp(KEMPSTON_PORT);

    if (kempston & KEMP_UP)    b |= ATMA_BTN_UP;
    if (kempston & KEMP_DOWN)  b |= ATMA_BTN_DOWN;
    if (kempston & KEMP_LEFT)  b |= ATMA_BTN_LEFT;
    if (kempston & KEMP_RIGHT) b |= ATMA_BTN_RIGHT;
    if (kempston & KEMP_FIRE)  b |= ATMA_BTN_FIRE1;

    if (KEY_DOWN(2, 0)) b |= ATMA_BTN_UP;    /* Q */
    if (KEY_DOWN(1, 0)) b |= ATMA_BTN_DOWN;  /* A */
    if (KEY_DOWN(5, 1)) b |= ATMA_BTN_LEFT;  /* O */
    if (KEY_DOWN(5, 0)) b |= ATMA_BTN_RIGHT; /* P */
    if (KEY_DOWN(7, 0)) b |= ATMA_BTN_FIRE1; /* SPACE */
    if (KEY_DOWN(7, 2)) b |= ATMA_BTN_FIRE2; /* M */
    if (KEY_DOWN(6, 0)) b |= ATMA_BTN_START; /* ENTER */

    /* BREAK: CAPS SHIFT (row 0 bit 0) plus SPACE (row 7 bit 0). */
    if (KEY_DOWN(0, 0) && KEY_DOWN(7, 0)) b |= ATMA_BTN_QUIT;

    return b;
}
