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
/** Per-tick velocity, in logical pixels. */
#ifdef __ZXNET
static i8 dx = 3, dy = 2;
#else
static i8 dx = 2, dy = 1;
#endif

static void on_init(void)
{
    u8 i;
    /* Load the generator's palette rows into the engine table at the same
     * slots the cell bytes use: row i of atma_patterns_palette holds
     * {engine index, r, g, b}, so cell byte N means engine palette slot N. */
    for (i = 0; i < ATMA_PATTERNS_PALETTE_N; ++i) {
        atma_gfx_palette_set(atma_patterns_palette[i][0],
                             atma_patterns_palette[i][1],
                             atma_patterns_palette[i][2],
                             atma_patterns_palette[i][3]);
    }

    /* Upload each generated cell as its own engine pattern. Cells already
     * carry palette indices; index 0 is transparent on every backend. */
    for (i = 0; i < ATMA_PATTERNS_CELLS; ++i) {
        if (atma_gfx_pattern_set_indexed(i, atma_patterns_cells[i]) != 0) {
            atma_log("demo: pattern upload failed; logo will not draw");
        }
    }
}

static void on_update(void)
{
    if (atma_input_pressed() & ATMA_BTN_START) atma_quit();

    x += dx;
    y += dy;
    if (x <= 0)                      { x = 0;                      dx = -dx; }
    if (x >= ATMA_SCREEN_W - LOGO_W) { x = ATMA_SCREEN_W - LOGO_W; dx = -dx; }
    if (y <= 0)                      { y = 0;                      dy = -dy; }
    if (y >= ATMA_SCREEN_H - LOGO_H) { y = ATMA_SCREEN_H - LOGO_H; dy = -dy; }
}

static void on_render(void)
{
    u8 cx, cy;

    /* Gradient: palette index tracks the scanline, blue fading to green. */
    for (cx = 0; cx < ATMA_SCREEN_H; ++cx) {
        atma_gfx_hline(cx, (u8)(1 + cx % 60));
    }

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
