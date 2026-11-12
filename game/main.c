#include "atma/engine.h"

static void on_update(void)
{
    if (atma_input_pressed() & ATMA_BTN_START) atma_quit();
}

static void on_render(void)
{
    atma_gfx_clear(0);
}

int main(void)
{
    atma_config cfg = { 60, ATMA_CPU_FASTEST, 0, on_update, on_render, 0 };
    return atma_run(&cfg);
}
