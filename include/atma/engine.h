/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file engine.h
 * \brief Engine umbrella header: the run loop, its configuration and the
 *        engine-wide accessors.
 */

#ifndef ATMA_ENGINE_H
#define ATMA_ENGINE_H

/**
 * \defgroup engine Engine core
 * \brief Run loop, frame pacing and diagnostics.
 *
 * Everything under include/atma/ is the platform-agnostic API games are
 * written against. src/core/ holds the engine proper (main loop, input edge
 * detection); src/platform/<name>/ holds one backend per machine (host, zxn,
 * ...) implementing platform.h plus the *_backend_* hooks declared in
 * input.h, gfx.h and sound.h. The Makefile links core + game + exactly one
 * backend, selected by `PLATFORM=<name>`.
 * @{
 */

#include "types.h"
#include "platform.h"
#include "input.h"
#include "gfx.h"
#include "sound.h"

/**
 * \brief The knob-set a game hands to atma_run().
 *
 * \note Any callback may be NULL to skip it. They take no arguments: read
 *       engine state (input, frame count) through the accessors instead.
 *       Callbacks may call atma_quit() to end the run.
 *
 * \warning config is read throughout atma_run(), so it must stay valid for
 *          the whole run - a local variable in main() is fine, since main()
 *          blocks on atma_run() anyway.
 */
typedef struct {
    /** Desired logic+render tick rate in Hz. The engine ticks the game once
     *  every (display_hz / target_fps) display refreshes, rounded to
     *  nearest, so rates that don't evenly divide the backend's refresh
     *  rate (see atma_platform_info) get the closest achievable cadence.
     *  0 means "tick on every refresh". */
    u8 target_fps;
    /** Requested clock speed; honored where the hardware allows it (ZX Next
     *  turbo mode), ignored on platforms that can't. Read once at
     *  startup. */
    atma_cpu_speed cpu_speed;
    /** Runs once, after the platform is up and before the loop. */
    void (*init)(void);
    /** Runs once per tick, before render, so render can show what update
     *  computed. Poll input inside update (or rely on the engine's QUIT
     *  check). */
    void (*update)(void);
    /** Runs once per tick between atma_gfx_begin_frame() and
     *  atma_gfx_end_frame(). */
    void (*render)(void);
    /** Runs once after the loop exits, before the platform goes down.
     *  Release resources here, not in atexit-style hooks. */
    void (*shutdown)(void);
} atma_config;

/**
 * \brief Run the game.
 *
 * Owns the whole platform lifecycle (platform/input/gfx/sound init before
 * the loop, shutdown after) and blocks until atma_quit() is called or the
 * backend raises ATMA_BTN_QUIT.
 *
 * \param config Run configuration; must stay valid for the whole call.
 * \return 0 on a normal exit, nonzero when the platform could not start
 *         (window/graphics failure). A nonzero return means the loop never
 *         ran.
 */
int atma_run(const atma_config *config);

/**
 * \brief Ask the main loop to exit.
 *
 * Safe to call from any callback (and from atma_run's own thread of
 * execution only); the current tick finishes first.
 */
void atma_quit(void);

/**
 * \brief Ticks since atma_run() started.
 *
 * \return The frame count. Wraps at 65536 (~18 min at 60 fps), so don't keep
 *         absolute time with it; diff two samples instead.
 */
u16 atma_frame_count(void);

/**
 * \brief One portable diagnostic line, for games and engine code alike.
 *
 * Goes to stderr (via SDL) on host; on boot targets it goes wherever the
 * backend can put it (or nowhere, if the machine has no console).
 *
 * \note Never a gameplay control flow tool: it's for the "this should never
 *       happen" messages that save a debugging session.
 *
 * \param msg NUL-terminated message; the sink adds its own prefix.
 */
void atma_log(const char *msg);

/**
 * \brief Backend diagnostic sink, implemented per platform.
 *
 * Backends implement the sink: atma_log_backend receives the message.
 * GCC/Clang builds get a weak stderr default in the core; SDCC builds (no
 * weak symbols) get no core default, so every SDCC backend must define
 * this.
 *
 * \param msg Message forwarded from atma_log().
 */
void atma_log_backend(const char *msg);

/** @} */

#endif
