/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "atma/input.h"

/*
 * ZX Next input backend. Kempston joystick reads from port 0x1F as five
 * active-low bits; the keyboard needs row/port scanning. Both collapse into
 * the ATMA_BTN_* bitmask core expects.
 */

void atma_input_backend_init(void) {}
void atma_input_backend_shutdown(void) {}

u16 atma_input_backend_read(void)
{
    /* TODO: Kempston/keyboard via z80 in_inp(); map Escape (or a combo) to
     * ATMA_BTN_QUIT so the engine's built-in exit works on real hardware. */
    return 0;
}
