/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "atma/engine.h"
#include "font8x8.h"

#define START_R 100
#define START_G 155
#define START_B 255

#define DELTA_PIX 3
#define DELTA_COLOR 10

static void on_init(void)
{
    u8 i;

    atma_gfx_clear(0);

    for (i = 1; i < 6; ++i) {
        atma_gfx_palette_set(i,
            START_R + DELTA_COLOR * i,
            START_G + DELTA_COLOR * i,
            START_B + DELTA_COLOR
        );

        atma_gfx_text(&font8x8, i + DELTA_PIX * i, i + DELTA_PIX * i,
          "HELLO ATMA", i);
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
