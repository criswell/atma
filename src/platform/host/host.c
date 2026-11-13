#include <SDL3/SDL.h>
#include "atma/platform.h"
#include "atma/input.h"
#include "atma/gfx.h"
#include "atma/engine.h"
#include "host_internal.h"

/*
 * Host backend (the desktop build), on SDL3.
 *
 * This file owns SDL lifetime, frame pacing and input. Graphics live in
 * host_gfx.c and sound in host_sound.c. The logical machine is the ZX
 * Spectrum Next (256x192, 64 sprites); the window scales it.
 */

static const atma_platform_info info = { "host", 60, ATMA_SCREEN_W, ATMA_SCREEN_H };
static Uint64 next_ns;
static Uint64 frame_ns;
static u16 quit_requested;

const atma_platform_info *atma_platform_info_get(void) { return &info; }

void atma_platform_init(void)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    frame_ns = SDL_NS_PER_SECOND / info.display_hz;
    next_ns = SDL_GetTicksNS();
}

void atma_platform_shutdown(void)
{
    SDL_Quit();
}

void atma_platform_set_cpu_speed(atma_cpu_speed speed) { (void)speed; }

u8 atma_platform_wait_frame(void)
{
    Uint64 now;
    Uint64 late;

    /* Absolute deadlines so per-tick overhead doesn't accumulate as drift. */
    next_ns += frame_ns;
    now = SDL_GetTicksNS();
    if (now < next_ns) {
        SDL_DelayPrecise(next_ns - now);
        return 1;
    }

    late = (now - next_ns) / frame_ns;
    if (late > 10) {
        next_ns = now;
        late = 10;
    } else {
        next_ns += late * frame_ns;
    }
    return (u8)(1 + late);
}

void atma_input_backend_init(void)
{
    quit_requested = 0;
}

void atma_input_backend_shutdown(void) {}

u16 atma_input_backend_read(void)
{
    SDL_Event e;
    const bool *k;
    u16 bits = 0;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            quit_requested = ATMA_BTN_QUIT;
        } else if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if (e.key.scancode == SDL_SCANCODE_F11 ||
                (e.key.scancode == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_ALT))) {
                atma_host_toggle_fullscreen();
            }
        }
    }

    k = SDL_GetKeyboardState(NULL);
    if (k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W]) bits |= ATMA_BTN_UP;
    if (k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S]) bits |= ATMA_BTN_DOWN;
    if (k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A]) bits |= ATMA_BTN_LEFT;
    if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) bits |= ATMA_BTN_RIGHT;
    if (k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_J]) bits |= ATMA_BTN_FIRE1;
    if (k[SDL_SCANCODE_X] || k[SDL_SCANCODE_K]) bits |= ATMA_BTN_FIRE2;
    if (k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_SPACE]) bits |= ATMA_BTN_START;
    if (k[SDL_SCANCODE_ESCAPE]) bits |= ATMA_BTN_QUIT;

    return (u16)(bits | quit_requested);
}
