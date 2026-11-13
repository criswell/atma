#include "atma/engine.h"

static u8 running;      /* cleared by atma_quit(); checked by the main loop */
static u16 frame_count; /* ticks since atma_run() began; wraps at 65536 */

void atma_quit(void)
{
    running = 0;
}

u16 atma_frame_count(void)
{
    return frame_count;
}

/*
 * The main loop, and the only place that knows the tick order:
 *
 *   wait for refresh -> (skip if not due) -> poll input -> QUIT check ->
 *   update() -> sound_update() -> begin_frame() -> render() -> end_frame()
 *
 * Everything below it is per-backend, selected at link time by PLATFORM=.
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
        divisor = (u8)(((u16)info->display_hz + config->target_fps / 2)
                       / config->target_fps);
    }

    atma_input_init();
    atma_gfx_init();
    atma_sound_init();

    if (config->init) config->init();

    running = 1;
    frame_count = 0;
    pending = 0;

    while (running) {
        /*
         * pending accumulates refreshes actually elapsed — wait_frame()
         * returns >= 1, more after an overrun — so a slow tick doesn't slow
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
