/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file main.c
 * \brief Bouncing-logo demo: a gradient with the atma logo ricocheting off
 *        the screen edges.
 */

#include "atma/engine.h"
#include "atma_patterns.h"

/**
 * \brief Bouncing-logo demo.
 *
 * A blue/green gradient with the atma logo ricocheting off the screen edges,
 * DVD-screensaver style. Fully portable: no #ifdef, no runtime files. The
 * logo is 8 generated 16x16 cell patterns (demo/atma_patterns.c, from
 * atma.png via tools/mkpats.py) drawn as a 4x2 grid through sprite slots,
 * exactly what Next hardware shows.
 *
 * Run from the repo root: `make GAME_DIR=demo run`
 */

/** Grid geometry of the generated cells. */
#define LOGO_CELLS_X ATMA_PATTERNS_COLS       /* 4 */
#define LOGO_CELLS_Y ATMA_PATTERNS_ROWS       /* 2 */
#define LOGO_W (LOGO_CELLS_X * 16)            /* 64 */
#define LOGO_H (LOGO_CELLS_Y * 16)            /* 32 */

/** Slot n in row-major grid order; slot 0 is the logo's top-left cell. */
#define LOGO_SLOT(cx, cy) (u8)((cy) * LOGO_CELLS_X + (cx))

/** Screen position of the logo's top-left corner. */
static i16 x = 40, y = 30;

/* velocity in px/second; convert to 8.8 fixed point per tick, once */
#define VY_PX_PER_SEC 60
#define VX_PX_PER_SEC 30
static i16 vx_q8, vy_q8;   /* per-tick movement, 8.8 */

static void on_init(void)
{
    u8 i;
    u8 cx;
    /* Load the generator's palette rows into the engine table at the same
     * slots the cell bytes use: row i of atma_patterns_palette holds
     * {engine index, r, g, b}, so cell byte N means engine palette slot N. */
    for (i = 0; i < ATMA_PATTERNS_PALETTE_N; ++i) {
        atma_gfx_palette_set(
            atma_patterns_palette[i][0],
            atma_patterns_palette[i][1],
            atma_patterns_palette[i][2],
            atma_patterns_palette[i][3]
        );
    }

    /* Upload each generated cell as its own engine pattern. Cells already
     * carry palette indices; index 0 is transparent on every backend. */
    for (i = 0; i < ATMA_PATTERNS_CELLS; ++i) {
        if (atma_gfx_pattern_set_indexed(i, atma_patterns_cells[i]) != 0) {
            atma_log("demo: pattern upload failed; logo will not draw");
        }
    }

    /* Gradient: palette index tracks the scanline, blue fading to green. */
    for (cx = 0; cx < ATMA_SCREEN_H; ++cx) {
        atma_gfx_hline(cx, (u8)(1 + cx % 60));
    }
    u8 hz = atma_platform_info_get()->display_hz;
    u8 tick_hz = hz < 60 ? hz : 60;          /* effective tick rate */
    vx_q8 = (i16)(VX_PX_PER_SEC * 256 / tick_hz);
    vy_q8 = (i16)(VY_PX_PER_SEC * 256 / tick_hz);
}

static void on_update(void)
{
    static i16 fx, fy;                        /* fractional remainders */
    if (atma_input_pressed() & ATMA_BTN_START) atma_quit();

    fx += vx_q8;  fy += vy_q8;
    x = (i16)(x + (fx >> 8));  fx &= 0xFF;
    y = (i16)(y + (fy >> 8));  fy &= 0xFF;
    if (x <= 0)                      { x = 0;                      vx_q8 = -vx_q8; }
    if (x >= ATMA_SCREEN_W - LOGO_W) { x = ATMA_SCREEN_W - LOGO_W; vx_q8 = -vx_q8; }
    if (y <= 0)                      { y = 0;                      vy_q8 = -vy_q8; }
    if (y >= ATMA_SCREEN_H - LOGO_H) { y = ATMA_SCREEN_H - LOGO_H; vy_q8 = -vy_q8; }
}

static void on_render(void)
{
    u8 cx, cy;

    for (cy = 0; cy < LOGO_CELLS_Y; ++cy) {
        for (cx = 0; cx < LOGO_CELLS_X; ++cx) {
            atma_gfx_sprite_set(LOGO_SLOT(cx, cy),
                                (i16)(x + cx * 16), (i16)(y + cy * 16),
                                LOGO_SLOT(cx, cy), 0);
        }
    }
}

int main(void)
{
    /* 60 fps at full tilt; shutdown is NULL because the demo holds no
     * resources of its own. Field order: target_fps, cpu_speed, init,
     * update, render, shutdown (see atma_config in engine.h). */
    atma_config cfg = { 60, ATMA_CPU_FASTEST, on_init, on_update, on_render, 0 };
    return atma_run(&cfg);
}
