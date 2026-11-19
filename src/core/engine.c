/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file engine.c
 * \brief The main loop, frame pacing and the core diagnostic sink.
 * \ingroup engine
 */

#include <stdio.h>
#include "atma/engine.h"

/** Cleared by atma_quit(); checked by the main loop. */
static u8 running;
/** Ticks since atma_run() began; wraps at 65536. */
static u16 frame_count;

/**
 * \brief Core's diagnostic sink.
 *
 * GCC/Clang builds get a weak stderr default that a backend may override.
 * SDCC has no weak symbols, so the core defines nothing there and every
 * SDCC backend supplies its own atma_log_backend (the zxn one is a no-op: no
 * console on the target).
 */
#if !defined(__SDCC)
#if defined(__GNUC__)
__attribute__((weak))
#endif
void atma_log_backend(const char *msg)
{
    fprintf(stderr, "atma: %s\n", msg);
}
#endif

void atma_quit(void)
{
    running = 0;
}

u16 atma_frame_count(void)
{
    return frame_count;
}

void atma_log(const char *msg)
{
    atma_log_backend(msg);
}

/**
 * \brief The main loop, and the only place that knows the tick order.
 *
 * Each tick runs, in order: wait for the next refresh (skipping the tick if
 * it is not due yet), poll input, check ATMA_BTN_QUIT, then update(),
 * sound_update(), begin_frame(), render() and end_frame().
 *
 * Everything below it is per-backend, selected at link time by PLATFORM=.
 *
 * \note Failure policy: a backend that cannot start (no window, no renderer)
 *       must say so on stderr and leave a liveness flag the engine can
 *       check. The engine then refuses to enter the loop and reports failure
 *       to the caller, instead of running invisible at 60 Hz forever.
 */
int atma_run(const atma_config *config)
{
    const atma_platform_info *info;
    u8 divisor;
    u8 pending;

    atma_platform_init();
    atma_platform_set_cpu_speed(config->cpu_speed);
    info = atma_platform_info_get();

    /*
     * Frame pacing: wait_frame() runs us at display_hz, and a game tick is
     * due once every `divisor` refreshes. The division rounds to nearest,
     * so a rate that doesn't divide the refresh evenly lands on the
     * closest achievable cadence (40 fps on 60 Hz gives divisor 2, i.e. a
     * steady 30) instead of truncating up to full speed. A target_fps of 0
     * (or >= display_hz) gives divisor 1: tick on every refresh. Rounds
     * can never yield 0 here, since target_fps < display_hz.
     */
    divisor = 1;
    if (config->target_fps && config->target_fps < info->display_hz) {
        /* (zsdcc emits warning 110, "conditional flow changed by
         * optimizer", on this expression: benign optimizer noise.) */
        divisor = (u8)(((u16)info->display_hz + config->target_fps / 2)
                       / config->target_fps);
    }

    atma_input_init();
    atma_gfx_init();
    atma_sound_init();

    if (config->init) config->init();

    /* The gfx backend reports startup failure through begin_frame's
     * liveness return (documented in gfx.h); a run with no display would
     * be invisible busywork, so fail the run here, loudly. */
    if (!atma_gfx_begin_frame()) {
        if (config->shutdown) config->shutdown();
        atma_sound_shutdown();
        atma_gfx_shutdown();
        atma_input_shutdown();
        atma_platform_shutdown();
        return 1;
    }

    running = 1;
    frame_count = 0;
    pending = 0;

    while (running) {
        /*
         * pending accumulates refreshes actually elapsed; wait_frame()
         * returns >= 1, more after an overrun, so a slow tick doesn't slow
         * the game further. Surplus refreshes are dropped rather than
         * banked: at most one tick per pass, no catch-up spiral.
         */
        pending = (u8)(pending + atma_platform_wait_frame());
        if (pending < divisor) continue; /* not due yet; ride another refresh */
        pending = 0;

        /* Poll first so update()/render() see this tick's edges, and so the
         * backend's quit gesture ends the loop without game code involved. */
        atma_input_poll();
        if (atma_input_down() & ATMA_BTN_QUIT) break;

        if (config->update) config->update();
        atma_sound_update();

        /* Render is bracketed so backends can batch or sync around it. */
        atma_gfx_begin_frame();
        if (config->render) config->render();
        atma_gfx_end_frame();

        ++frame_count;
    }

    if (config->shutdown) config->shutdown();

    /* Tear down in the reverse of init. */
    atma_sound_shutdown();
    atma_gfx_shutdown();
    atma_input_shutdown();
    atma_platform_shutdown();
    return 0;
}
