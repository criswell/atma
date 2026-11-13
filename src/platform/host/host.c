#include <stdio.h>
#include <time.h>
#include "atma/platform.h"
#include "atma/input.h"
#include "atma/gfx.h"
#include "atma/sound.h"
#include "atma/engine.h"

/*
 * Host backend (the desktop build).
 *
 * Headless placeholder: no window and no audio yet - just deterministic
 * stubs so the engine loop can be built and exercised with gcc. Notably,
 * `make PLATFORM=host run` terminates on its own after 120 frames via the
 * auto-quit in the input backend below. TODO: a real window (e.g. SDL) and
 * keyboard mapping onto the ATMA_BTN_* bits.
 */

static const atma_platform_info info = { "host", 60, 256, 192 };
static struct timespec next; /* absolute time of the next display refresh */

const atma_platform_info *atma_platform_info_get(void) { return &info; }

void atma_platform_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &next);
}

void atma_platform_shutdown(void) {}
void atma_platform_set_cpu_speed(atma_cpu_speed speed) { (void)speed; }

u8 atma_platform_wait_frame(void)
{
    /* Sleep until the absolute next-refresh deadline (not "now + 16 ms"),
     * so per-tick overhead doesn't accumulate into drift. */
    next.tv_nsec += 1000000000L / info.display_hz;
    if (next.tv_nsec >= 1000000000L) {
        next.tv_nsec -= 1000000000L;
        next.tv_sec++;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);
    return 1;
}

void atma_input_backend_init(void) {}
void atma_input_backend_shutdown(void) {}

u16 atma_input_backend_read(void)
{
    /* Placeholder: pretend the user quits after ~2 seconds so the headless
     * build terminates. TODO: real keyboard input. */
    return atma_frame_count() >= 120 ? ATMA_BTN_QUIT : 0;
}

/* Host gfx: pure stubs until a window backend exists. */
void atma_gfx_init(void) {}
void atma_gfx_shutdown(void) {}
void atma_gfx_begin_frame(void) {}
void atma_gfx_end_frame(void) {}
void atma_gfx_clear(u8 color) { (void)color; }
void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags)
{
    (void)slot; (void)x; (void)y; (void)pattern; (void)flags;
}
void atma_gfx_sprite_hide(u8 slot) { (void)slot; }

/* Host sound: pure stubs until an audio backend exists. */
void atma_sound_init(void) {}
void atma_sound_shutdown(void) {}
void atma_sound_update(void) {}
void atma_sound_music_play(u8 track) { (void)track; }
void atma_sound_music_stop(void) {}
void atma_sound_sfx_play(u8 id) { (void)id; }
