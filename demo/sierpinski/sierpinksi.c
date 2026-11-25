/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "atma/engine.h"

#define FRACTAL_COLOR 31
#define FRACTAL_ROWS 128

static void on_init(void)
{
    u16 row;
    u16 col;

    atma_gfx_clear(0);
    atma_gfx_palette_set(FRACTAL_COLOR, 255, 255, 255);

    for (row = 0; row < FRACTAL_ROWS; ++row) {
        u16 left = (ATMA_SCREEN_W - FRACTAL_ROWS) / 2;

        for (col = 0; col <= row; ++col) {
            /* Entries of Pascal's triangle are odd exactly where
             * col & (row - col) is zero. */
            if ((col & (row - col)) == 0) {
                atma_gfx_pixel((i16)(left + col), (i16)row, FRACTAL_COLOR);
            }
        }
    }
}

int main(void)
{
    atma_config cfg = {
        60,
        ATMA_CPU_FASTEST,
        on_init,
        0,
        0,
        0
    };

    return atma_run(&cfg);
}
