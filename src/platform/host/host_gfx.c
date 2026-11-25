/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file host_gfx.c
 * \brief Host graphics on the SDL3 renderer.
 * \ingroup gfx
 */

#include <SDL3/SDL.h>
#include <string.h>
#include "atma/gfx.h"
#include "atma/host.h"
#include "atma/platform.h"
#include "host_internal.h"

/**
 * \brief Host graphics on the SDL3 renderer.
 *
 * The renderer's logical presentation pins the drawing space to the Next's
 * 256x192 and letterboxes it into the window, so resizing and fullscreen
 * rescale automatically (nearest neighbour, so pixels stay crisp). Sprites
 * draw in slot order, higher slots on top, like the Next.
 *
 * The frame is rasterized into a real 256x192 target texture and only that
 * is scaled to the window, in whole-number steps. Sprites therefore snap to
 * the logical pixel grid just like Next hardware, instead of gliding over
 * sub-pixel window positions.
 *
 * The background is a byte-per-pixel Layer 2 model, not a draw list: clear
 * and hline write palette indices into l2_vram, and end_frame resolves the
 * bytes against the live palette into a texture composited under the
 * sprites. That matches the Next, where Layer 2 is persistent memory the
 * display reads every scanline: background drawn once in on_init survives
 * every frame, and mid-run palette edits recolor it. Drawing into l2_vram
 * works from anywhere, including before the first begin_frame.
 *
 * Palette discipline: the host is not more capable than the Next. Colors
 * are quantized to the Next's 9-bit RGB333 (see palette_quantize below), so
 * the window shows byte-for-byte what the hardware will. Pattern uploads
 * are indexed bytes resolved through the same table, exactly what the zxn
 * backend pokes into sprite pattern memory, and they are RE-RESOLVED when
 * the palette changes (palette_flush), so hot-swapping the palette
 * underneath a sprite recolors it here exactly as the Next's hardware
 * resolves indices at render time.
 */

/** Initial window scale factor, in logical pixels per window pixel. */
#define SCALE_START 3

/** Per-slot sprite state, as set by atma_gfx_sprite_set(). */
typedef struct {
    i16 x, y;        /**< Screen position in logical pixels. */
    u8 pattern, flags; /**< Pattern id and ATMA_SPR_* attribute bits. */
    u8 visible;      /**< Cleared by atma_gfx_sprite_hide(). */
} sprite_state;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *screen;
/** Byte-per-pixel Layer 2 model: indices, resolved to l2_tex at scanout. */
static u8 l2_vram[ATMA_SCREEN_W * ATMA_SCREEN_H];
static SDL_Texture *l2_tex;
static SDL_Texture *patterns[ATMA_MAX_PATTERNS];
static u16 pattern_w[ATMA_MAX_PATTERNS];
static u16 pattern_h[ATMA_MAX_PATTERNS];
static sprite_state sprites[ATMA_MAX_SPRITES];
static SDL_Color palette[256];
static u8 l2_dirty;

/**
 * \brief Raw indexed bytes of every uploaded pattern, kept so the palette
 *        can be re-resolved on change.
 *
 * Hot-swap parity with the Next, where pattern memory holds indices and the
 * hardware resolves them against the live palette every frame. 64 x 256
 * bytes = 16K: exactly the Next's sprite pattern memory. pattern_live marks
 * slots with an SDL texture to update.
 */
static u8 pattern_bytes[ATMA_MAX_PATTERNS_8BIT][ATMA_PATTERN_BYTES];
static u8 pattern_live[ATMA_MAX_PATTERNS_8BIT];
static u8 palette_dirty;

/**
 * \brief The Next's RGB333 color space: 3 bits of red, 3 of green, 3 of
 *        blue (9-bit color, 512 distinct values).
 *
 * Quantize every entry so host and Next show identical colors. The Next's
 * default palette is its RGB332 subset (the engine starts there via
 * palette_default).
 */
static SDL_Color palette_quantize(u8 r, u8 g, u8 b)
{
    SDL_Color c;
    c.r = (Uint8)(((r >> 5) * 255) / 7);
    c.g = (Uint8)(((g >> 5) * 255) / 7);
    c.b = (Uint8)(((b >> 5) * 255) / 7);
    c.a = 255;
    return c;
}

static void palette_default(void)
{
    int i;
    /* RGB332 expansion, which is also the Next's default 8-bit palette:
     * blue's 2 bits replicated by the quantizer's /7 rounding to fill the
     * 9-bit entry. Entry = index, so power-on colors need no setup. */
    for (i = 0; i < 256; ++i) {
        palette[i] = palette_quantize((u8)(((i >> 5) & 7) * 255 / 7),
                                      (u8)(((i >> 2) & 7) * 255 / 7),
                                      (u8)((i & 3) * 255 / 3));
    }
    palette_dirty = 1;
}

/**
 * \brief Re-resolve one pattern's indexed bytes against the live palette
 *        into its texture.
 *
 * Byte 0 stays transparent (alpha 0) regardless of entry 0's RGB: the
 * engine-wide rule, mapped on zxn via the transparency index register.
 */
static void pattern_resolve(u8 pattern)
{
    u8 rgba[ATMA_PATTERN_BYTES * 4];
    int i;

    for (i = 0; i < ATMA_PATTERN_BYTES; ++i) {
        const SDL_Color *c = &palette[pattern_bytes[pattern][i]];
        rgba[i * 4 + 0] = c->r;
        rgba[i * 4 + 1] = c->g;
        rgba[i * 4 + 2] = c->b;
        rgba[i * 4 + 3] = pattern_bytes[pattern][i] == 0 ? 0 : 255;
    }
    SDL_UpdateTexture(patterns[pattern], NULL, rgba, 16 * 4);
}

/**
 * \brief Resolve the Layer 2 model's bytes against the live palette into
 *        its texture.
 *
 * Runs when the palette changed or the VRAM was written this frame; one
 * 256x192 pass per scanout at most, like the Next resolving Layer 2 during
 * display fetch. Every byte is opaque: Layer 2 has no transparency, that
 * belongs to the sprite layer above it.
 */
static void l2_resolve(void)
{
    u8 rgba[ATMA_SCREEN_W * ATMA_SCREEN_H * 4];
    int i;
    const int n = ATMA_SCREEN_W * ATMA_SCREEN_H;

    for (i = 0; i < n; ++i) {
        const SDL_Color *c = &palette[l2_vram[i]];
        rgba[i * 4 + 0] = c->r;
        rgba[i * 4 + 1] = c->g;
        rgba[i * 4 + 2] = c->b;
        rgba[i * 4 + 3] = 255;
    }
    SDL_UpdateTexture(l2_tex, NULL, rgba, ATMA_SCREEN_W * 4);
    l2_dirty = 0;
}

/**
 * \brief Push palette changes into every live pattern texture.
 *
 * Called once per frame at most; a palette edit between frames costs one
 * batched refresh here, matching the Next's resolve-at-render semantics.
 */
static void palette_flush(void)
{
    u8 p;

    if (!palette_dirty) return;
    for (p = 0; p < ATMA_MAX_PATTERNS_8BIT; ++p) {
        if (pattern_live[p]) pattern_resolve(p);
    }
    palette_dirty = 0;
    l2_dirty = 1;
}

void atma_gfx_init(void)
{
    const atma_platform_info *pi = atma_platform_info_get();

    palette_default();
    memset(sprites, 0, sizeof sprites);

    window = SDL_CreateWindow("atma", pi->screen_w * SCALE_START,
                              pi->screen_h * SCALE_START,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_Log("atma: could not create window: %s", SDL_GetError());
        return;
    }
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("atma: could not create renderer: %s", SDL_GetError());
        return;
    }
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                               SDL_TEXTUREACCESS_TARGET, pi->screen_w, pi->screen_h);
    if (!screen) {
        SDL_Log("atma: could not create %dx%d frame texture: %s",
                pi->screen_w, pi->screen_h, SDL_GetError());
        return;
    }
    SDL_SetTextureScaleMode(screen, SDL_SCALEMODE_NEAREST);
    l2_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                               SDL_TEXTUREACCESS_STREAMING,
                               pi->screen_w, pi->screen_h);
    if (!l2_tex) {
        SDL_Log("atma: could not create Layer 2 texture: %s", SDL_GetError());
        return;
    }
    SDL_SetTextureScaleMode(l2_tex, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(renderer, pi->screen_w, pi->screen_h,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(renderer, 1);
}

void atma_gfx_shutdown(void)
{
    int i;
    for (i = 0; i < ATMA_MAX_PATTERNS; ++i) {
        if (patterns[i]) SDL_DestroyTexture(patterns[i]);
        patterns[i] = NULL;
    }
    memset(pattern_live, 0, sizeof pattern_live);
    if (screen) SDL_DestroyTexture(screen);
    screen = NULL;
    if (l2_tex) SDL_DestroyTexture(l2_tex);
    l2_tex = NULL;
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    renderer = NULL;
    window = NULL;
}

void atma_host_toggle_fullscreen(void)
{
    if (!window) return;
    SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
}

u8 atma_gfx_begin_frame(void)
{
    if (screen) SDL_SetRenderTarget(renderer, screen);
    return screen != NULL;
}

void atma_gfx_end_frame(void)
{
    int i;
    if (!renderer || !screen) return;

    /* Palette edits take effect on the frame that draws with them, like
     * the Next resolving indices at render time. */
    palette_flush();

    /* Layer 2 scanout: composite the background under the sprites. The
     * background persists in l2_vram across frames, so whatever the game
     * painted last (whenever it painted it) is what shows here; sprites
     * with byte 0 transparent let it through, the rest cover it. */
    if (l2_dirty) l2_resolve();
    SDL_SetTextureBlendMode(l2_tex, SDL_BLENDMODE_NONE);
    SDL_RenderTexture(renderer, l2_tex, NULL, NULL);

    for (i = 0; i < ATMA_MAX_SPRITES; ++i) {
        const sprite_state *s = &sprites[i];
        SDL_FRect dst;
        SDL_FlipMode flip = SDL_FLIP_NONE;
        double angle = 0.0;

        if (!s->visible || !patterns[s->pattern]) continue;

        dst.x = s->x;
        dst.y = s->y;
        dst.w = pattern_w[s->pattern];
        dst.h = pattern_h[s->pattern];
        if (s->flags & ATMA_SPR_ROTATE) angle = 90.0;
        if (s->flags & ATMA_SPR_MIRROR_X) flip |= SDL_FLIP_HORIZONTAL;
        if (s->flags & ATMA_SPR_MIRROR_Y) flip |= SDL_FLIP_VERTICAL;
        SDL_RenderTextureRotated(renderer, patterns[s->pattern], NULL, &dst,
                                 angle, NULL, flip);
    }
    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, screen, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void atma_gfx_clear(u8 color)
{
    /* Fill the Layer 2 model, not whatever the render target happens to be:
     * a call from the game's init callback (before the first begin_frame)
     * must land in VRAM the display reads, like a Next Layer 2 fill. */
    if (!l2_tex) return;
    memset(l2_vram, color, sizeof l2_vram);
    l2_dirty = 1;
}

void atma_gfx_pixel(i16 x, i16 y, u8 color)
{
    if (!l2_tex || y >= ATMA_SCREEN_H || x >= ATMA_SCREEN_W) return;
    memset(&l2_vram[y * ATMA_SCREEN_W + x], color, 1);
    l2_dirty = 1;
}

void atma_gfx_hline(u8 y, u8 color)
{
    if (!l2_tex || y >= ATMA_SCREEN_H) return;
    memset(&l2_vram[y * ATMA_SCREEN_W], color, ATMA_SCREEN_W);
    l2_dirty = 1;
}

void atma_gfx_blit(i16 x, i16 y, u8 w, u8 h, const u8 *pixels)
{
    return;
}

void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags)
{
    sprite_state *s;
    if (slot >= ATMA_MAX_SPRITES) return;
    s = &sprites[slot];
    s->x = x;
    s->y = y;
    s->pattern = pattern;
    s->flags = flags;
    s->visible = 1;
}

void atma_gfx_sprite_hide(u8 slot)
{
    if (slot < ATMA_MAX_SPRITES) sprites[slot].visible = 0;
}

void atma_gfx_palette_set(u8 index, u8 r, u8 g, u8 b)
{
    palette[index] = palette_quantize(r, g, b);
    /* Patterns hold indices, not colors: re-resolve them against the new
     * entry before the next frame draws. Host parity with the Next, where
     * this recolors every sprite using the index, instantly. */
    palette_dirty = 1;
}

int atma_gfx_pattern_set_indexed(u8 pattern, const u8 *pixels)
{
    SDL_Texture *t;

    if (!renderer) {
        SDL_Log("atma: pattern %u uploaded before gfx init", pattern);
        return -1;
    }
    if (!pixels) return -1;
    if (pattern >= ATMA_MAX_PATTERNS_8BIT) {
        SDL_Log("atma: pattern %u out of range (host stores 8-bit patterns "
                "like the Next: 0..%d)", pattern, ATMA_MAX_PATTERNS_8BIT - 1);
        return -1;
    }

    /* Stash the indexed bytes: the palette can change later, and the
     * pattern must re-resolve against it (palette hot-swap). */
    memcpy(pattern_bytes[pattern], pixels, ATMA_PATTERN_BYTES);

    t = patterns[pattern];
    if (!t) {
        t = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                              SDL_TEXTUREACCESS_STATIC, 16, 16);
        if (!t) {
            SDL_Log("atma: pattern %u texture creation failed: %s",
                    pattern, SDL_GetError());
            return -1;
        }
        SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        patterns[pattern] = t;
        pattern_w[pattern] = 16;
        pattern_h[pattern] = 16;
    }

    pattern_live[pattern] = 1;
    pattern_resolve(pattern);
    return 0;
}
