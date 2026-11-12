#include "atma/engine.h"

static u8 running;
static u16 frame_count;

void atma_quit(void)
{
    running = 0;
}

u16 atma_frame_count(void)
{
    return frame_count;
}

int atma_run(const atma_config *config)
{
    const atma_platform_info *info;
    u8 divisor;
    u8 pending;

    atma_platform_init();
    atma_platform_set_cpu_speed(config->cpu_speed);
    info = atma_platform_info_get();

    divisor = 1;
    if (config->target_fps && config->target_fps < info->display_hz) {
        divisor = (u8)(info->display_hz / config->target_fps);
        if (divisor == 0) divisor = 1;
    }

    atma_input_init();
    atma_gfx_init();
    atma_sound_init();

    if (config->init) config->init();

    running = 1;
    frame_count = 0;
    pending = 0;

    while (running) {
        pending = (u8)(pending + atma_platform_wait_frame());
        if (pending < divisor) continue;
        pending = 0;

        atma_input_poll();
        if (atma_input_down() & ATMA_BTN_QUIT) break;

        if (config->update) config->update();
        atma_sound_update();

        atma_gfx_begin_frame();
        if (config->render) config->render();
        atma_gfx_end_frame();

        ++frame_count;
    }

    if (config->shutdown) config->shutdown();

    atma_sound_shutdown();
    atma_gfx_shutdown();
    atma_input_shutdown();
    atma_platform_shutdown();
    return 0;
}
