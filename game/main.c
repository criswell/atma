/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file main.c
 * \brief Minimal game: clears the screen and quits on START.
 */

#include "atma/engine.h"

/** \brief Quit when START is pressed. */
static void on_update(void)
{
    if (atma_input_pressed() & ATMA_BTN_START) atma_quit();
}

/** \brief Clear the screen to palette index 0. */
static void on_render(void)
{
    atma_gfx_clear(0);
}

int main(void)
{
    /* 60 fps at full tilt; init and shutdown are NULL because this game
     * holds no resources of its own. Field order: target_fps, cpu_speed,
     * init, update, render, shutdown (see atma_config in engine.h). */
    atma_config cfg = { 60, ATMA_CPU_FASTEST, 0, on_update, on_render, 0 };
    return atma_run(&cfg);
}
