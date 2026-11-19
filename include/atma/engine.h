/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ATMA_ENGINE_H
#define ATMA_ENGINE_H

/*
 * atma - engine umbrella header.
 *
 * Everything under include/atma/ is the platform-agnostic API games are
 * written against. src/core/ holds the engine proper (main loop, input edge
 * detection); src/platform/<name>/ holds one backend per machine (host, zxn,
 * ...) implementing platform.h plus the *_backend_* hooks declared in
 * input.h, gfx.h and sound.h. The Makefile links core + game + exactly one
 * backend, selected by `PLATFORM=<name>`.
 */

#include "types.h"
#include "platform.h"
#include "input.h"
#include "gfx.h"
#include "sound.h"

/*
 * The knob-set a game hands to atma_run().
 *
 * target_fps  - desired logic+render tick rate in Hz. The engine ticks the
 *               game once every (display_hz / target_fps) display
 *               refreshes, rounded to nearest, so rates that don't evenly
 *               divide the backend's refresh rate (see
 *               atma_platform_info) get the closest achievable cadence.
 *               0 means "tick on every refresh".
 * cpu_speed   - requested clock speed; honored where the hardware allows it
 *               (ZX Next turbo mode), ignored on platforms that can't. Read
 *               once at startup.
 * init        - runs once, after the platform is up and before the loop.
 * update      - runs once per tick, before render, so render can show what
 *               update computed. Poll input inside update (or rely on the
 *               engine's QUIT check).
 * render      - runs once per tick between atma_gfx_begin_frame() and
 *               atma_gfx_end_frame().
 * shutdown    - runs once after the loop exits, before the platform goes
 *               down. Release resources here, not in atexit-style hooks.
 *
 * Any callback may be NULL to skip it. They take no arguments: read engine
 * state (input, frame count) through the accessors instead. Callbacks may
 * call atma_quit() to end the run.
 *
 * config is read throughout atma_run(), so it must stay valid for the whole
 * run - a local variable in main() is fine, since main() blocks on
 * atma_run() anyway.
 */
typedef struct {
    u8 target_fps;
    atma_cpu_speed cpu_speed;
    void (*init)(void);
    void (*update)(void);
    void (*render)(void);
    void (*shutdown)(void);
} atma_config;

/*
 * Run the game. Owns the whole platform lifecycle (platform/input/gfx/sound
 * init before the loop, shutdown after) and blocks until atma_quit() is
 * called or the backend raises ATMA_BTN_QUIT. Returns 0 on a normal exit,
 * nonzero when the platform could not start (window/graphics failure); a
 * nonzero return means the loop never ran.
 */
int atma_run(const atma_config *config);

/* Ask the main loop to exit. Safe to call from any callback (and from
 * atma_run's own thread of execution only); the current tick finishes
 * first. */
void atma_quit(void);

/* Ticks since atma_run() started. Wraps at 65536 (~18 min at 60 fps), so
 * don't keep absolute time with it; diff two samples instead. */
u16 atma_frame_count(void);

/* One portable diagnostic line, for games and engine code alike. Goes to
 * stderr (via SDL) on host; on boot targets it goes wherever the backend
 * can put it (or nowhere, if the machine has no console). Never a
 * gameplay control flow tool: it's for the "this should never happen"
 * messages that save a debugging session.
 *
 * Backends implement the sink: atma_log_backend receives the message.
 * GCC/Clang builds get a weak stderr default in the core; SDCC builds
 * (no weak symbols) get no core default, so every SDCC backend must
 * define this. */
void atma_log_backend(const char *msg);
void atma_log(const char *msg);

#endif
