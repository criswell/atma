/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ATMA_PLATFORM_H
#define ATMA_PLATFORM_H

/*
 * Platform seam. Each backend under src/platform/<name>/ implements these
 * four entry points, plus the *_backend_* hooks declared in input.h, gfx.h
 * and sound.h. The engine (src/core/engine.c) drives all of them in a fixed
 * order; a game never calls any of this directly - it reads info below.
 */

#include "types.h"

/*
 * Clock speed request, ordered slowest to fastest. The numeric values map
 * straight onto the ZX Next turbo register's ladder (0 = 3.5 MHz, 1 = 7,
 * 2 = 14, 3 = 28, the hardware maximum), so zxn can cast and write
 * directly. Backends without a speed control silently ignore the request.
 */
typedef enum {
    ATMA_CPU_SLOW = 0,
    ATMA_CPU_MEDIUM,
    ATMA_CPU_FAST,
    ATMA_CPU_FASTEST
} atma_cpu_speed;

/*
 * What the current backend can do, advertised at runtime. display_hz drives
 * the engine's frame pacing, so a game's config.target_fps should ideally
 * divide it evenly; rates that don't get the nearest achievable cadence.
 * Screen size is the logical resolution games may assume.
 */
typedef struct {
    const char *name;
    u8 display_hz;
    u16 screen_w;
    u16 screen_h;
} atma_platform_info;

const atma_platform_info *atma_platform_info_get(void);

/* Backend lifecycle, called by the engine around atma_run()'s main loop.
 * init runs before input/gfx/sound are initialized. */
void atma_platform_init(void);
void atma_platform_shutdown(void);

void atma_platform_set_cpu_speed(atma_cpu_speed speed);

/*
 * Block until the next display refresh; return how many refreshes elapsed
 * since the last call - normally 1, more if a tick overran, so callers can
 * keep game speed honest under load. Typical implementation: HALT until the
 * frame interrupt on interrupt-driven machines, a timed sleep on hosts.
 */
u8 atma_platform_wait_frame(void);

#endif
