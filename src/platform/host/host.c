#include <stdio.h>
#include <time.h>
#include "atma/platform.h"
#include "atma/input.h"
#include "atma/gfx.h"
#include "atma/sound.h"
#include "atma/engine.h"

static const atma_platform_info info = { "host", 60, 256, 192 };
static struct timespec next;

const atma_platform_info *atma_platform_info_get(void) { return &info; }

void atma_platform_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &next);
}

void atma_platform_shutdown(void) {}
void atma_platform_set_cpu_speed(atma_cpu_speed speed) { (void)speed; }

u8 atma_platform_wait_frame(void)
{
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
    return atma_frame_count() >= 120 ? ATMA_BTN_QUIT : 0;
}

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

void atma_sound_init(void) {}
void atma_sound_shutdown(void) {}
void atma_sound_update(void) {}
void atma_sound_music_play(u8 track) { (void)track; }
void atma_sound_music_stop(void) {}
void atma_sound_sfx_play(u8 id) { (void)id; }
