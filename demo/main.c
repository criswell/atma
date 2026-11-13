#include "atma/engine.h"
#ifdef ATMA_PLATFORM_HOST
#include "atma/host.h"
#endif

/*
 * Bouncing-logo demo: a blue gradient with an image ricocheting off the
 * screen edges, DVD-screensaver style. Run from the repo root:
 *   make GAME_DIR=demo run
 */

#define LOGO_W 52
#define LOGO_H 24

static i16 x = 40, y = 30;
static i8 dx = 2, dy = 1;

static void on_init(void)
{
#ifdef ATMA_PLATFORM_HOST
    u8 i;
    /* Gradient uses palette indices 0..191, one per scanline. */
    for (i = 0; i < ATMA_SCREEN_H; ++i) {
        atma_host_palette_set(i, 0, (u8)(i / 6), (u8)(70 + i * 185 / ATMA_SCREEN_H));
    }
    atma_host_spritesheet_load("demo/atma.png", LOGO_W, LOGO_H, 0);
#endif
}

static void on_update(void)
{
    if (atma_input_pressed() & ATMA_BTN_START) atma_quit();

    x += dx;
    y += dy;
    if (x <= 0)                { x = 0;                dx = -dx; }
    if (x >= ATMA_SCREEN_W - LOGO_W) { x = ATMA_SCREEN_W - LOGO_W; dx = -dx; }
    if (y <= 0)                { y = 0;                dy = -dy; }
    if (y >= ATMA_SCREEN_H - LOGO_H) { y = ATMA_SCREEN_H - LOGO_H; dy = -dy; }
}

static void on_render(void)
{
    u8 line;
    for (line = 0; line < ATMA_SCREEN_H; ++line) {
        atma_gfx_hline(line, line);
    }
    atma_gfx_sprite_set(0, x, y, 0, 0);
}

int main(void)
{
    atma_config cfg = { 60, ATMA_CPU_FASTEST, on_init, on_update, on_render, 0 };
    return atma_run(&cfg);
}
