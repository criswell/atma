#include "atma/engine.h"
#include "fire_basin.h"
#include "light_ring.h"

static u8 basinx = ATMA_SCREEN_W / 2 - 8;
static u8 basiny = ATMA_SCREEN_H / 2 - 8;
static u8 lightx = ATMA_SCREEN_W / 2 - 17;
static u8 lighty = ATMA_SCREEN_H / 2 - 12;

/* Cycle slots in brightness order; 6 is the tint apex. */
static const u8 fire_slot[5] = { 1, 2, 3, 4, 6 };
static const u8 fire_rgb[5][3] = {
    { 145, 72,  0  },   /* #914800 */
    { 182, 72,  0  },   /* #B64800 */
    { 218, 109, 0  },   /* #DA6D00 */
    { 255, 145, 0  },   /* #FF9100 */
    { 255, 182, 72 },   /* #FFB648 */
};
static u8 phase;   /* frames since last step */
static u8 step;    /* rotation offset, 0..4 */
#define RING_PAL_BASE 11   /* basin owns 0..10; slot 11 idles, ring rows 1..7 land at 12..18 */
#define RING_THROB_LO 4    /* ring undulation: dimmest brightness step */
#define RING_THROB_HI 24   /* ring undulation: apex, full brightness */

static u8 throb = RING_THROB_LO;   /* ring brightness wave */
static i8 breath = 1;              /* direction of the wave */

static u8 ring_xlated[LIGHT_RING_CELLS][16 * 16];

static void on_init(void)
{
    u8 i;
    u16 j;

    /* Load the palette rows */
    /* fire_basin */
    for (i = 0; i < FIRE_BASIN_PALETTE_N; ++i) {
        atma_gfx_palette_set(
            fire_basin_palette[i][0],
            fire_basin_palette[i][1],
            fire_basin_palette[i][2],
            fire_basin_palette[i][3]
        );
    }
    /* light_ring */
    for (i = 1; i < LIGHT_RING_PALETTE_N; ++i) { /* skip marker row 0 */
        atma_gfx_palette_set(
            (u8)(i + RING_PAL_BASE),
            light_ring_palette[i][1],
            light_ring_palette[i][2],
            light_ring_palette[i][3]);
    }

    /* Load the spirtes */
    /* fire_basin */
    for (i = 0; i < FIRE_BASIN_CELLS; ++i) {
        if (atma_gfx_pattern_set_indexed(i, fire_basin_cells[i]) != 0) {
            atma_log("fire-palette: failed loading fire_basin");
        }
    }
    /* light_ring */
    for (i = 0; i < LIGHT_RING_CELLS; ++i)
        for (j = 0; j < 16 * 16; ++j) {
            u8 b = light_ring_cells[i][j];
            ring_xlated[i][j] = b ? (u8)(b + RING_PAL_BASE) : 0;
        }
    for (i = FIRE_BASIN_CELLS; i < FIRE_BASIN_CELLS + LIGHT_RING_CELLS; ++i) {
        if (atma_gfx_pattern_set_indexed(i, ring_xlated[i - FIRE_BASIN_CELLS]) != 0) {
            atma_log("fire-palette: failed loading light_ring");
        }
    }
}

static void on_update(void)
{
    u8 i, j;

    /* fire */
    if (++phase < 6) return;
    phase = 0;
    step = (u8)((step + 1) % 5);   /* the actual rotation */
    for (i = 0; i < 5; ++i) {
        j = (u8)((i + step) % 5);
        atma_gfx_palette_set(
            fire_slot[i], fire_rgb[j][0],
            fire_rgb[j][1], fire_rgb[j][2]
        );
    }

    /* light cast: wave the brightness up and down; ring rows are gray,
     * so scale each row's own level instead of shuffling palette rows */
    throb += breath;
    if (throb >= RING_THROB_HI) breath = -1;
    else if (throb <= RING_THROB_LO) breath = 1;

    for (i = 1; i < LIGHT_RING_PALETTE_N; ++i) {
        u8 v = (u8)((light_ring_palette[i][1] * throb) / RING_THROB_HI);
        atma_gfx_palette_set(
            (u8)(i + RING_PAL_BASE),
            v, v, v
        );
    } 
}

static void on_render(void)
{
    atma_gfx_sprite_set(1, lightx,      lighty,      1, 0);  /* ring TL */
    atma_gfx_sprite_set(2, lightx + 16, lighty,      2, 0);  /* ring TR */
    atma_gfx_sprite_set(3, lightx,      lighty + 16, 3, 0);  /* ring BL */
    atma_gfx_sprite_set(4, lightx + 16, lighty + 16, 4, 0);  /* ring BR */
    atma_gfx_sprite_set(5, basinx, basiny, 0, 0);            /* fire on top */
}

int main(void)
{
    /* 60 fps at full tilt; shutdown is NULL because the demo holds no
     * resources of its own. Field order: target_fps, cpu_speed, init,
     * update, render, shutdown (see atma_config in engine.h). */
    atma_config cfg = { 60, ATMA_CPU_FASTEST, on_init, on_update, on_render, 0 };
    return atma_run(&cfg);
}
