#include <SDL3/SDL.h>
#include <string.h>
#include "atma/gfx.h"
#include "atma/host.h"
#include "atma/platform.h"
#include "host_internal.h"

/*
 * Host graphics on the SDL3 renderer. The renderer's logical presentation
 * pins the drawing space to the Next's 256x192 and letterboxes it into the
 * window, so resizing and fullscreen rescale automatically (nearest
 * neighbour, so pixels stay crisp). Sprites draw in slot order, higher slots
 * on top, like the Next.
 *
 * The frame is rasterized into a real 256x192 target texture and only that
 * is scaled to the window, in whole-number steps. Sprites therefore snap to
 * the logical pixel grid just like Next hardware, instead of gliding over
 * sub-pixel window positions.
 */

#define SCALE_START 3

typedef struct {
    i16 x, y;
    u8 pattern, flags;
    u8 visible;
} sprite_state;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *screen;
static SDL_Texture *patterns[ATMA_MAX_PATTERNS];
static u16 pattern_w[ATMA_MAX_PATTERNS];
static u16 pattern_h[ATMA_MAX_PATTERNS];
static sprite_state sprites[ATMA_MAX_SPRITES];
static SDL_Color palette[256];

static void palette_default(void)
{
    int i;
    /* RGB332 expansion, which is also the Next's default 8-bit palette. */
    for (i = 0; i < 256; ++i) {
        palette[i].r = (Uint8)(((i >> 5) & 7) * 255 / 7);
        palette[i].g = (Uint8)(((i >> 2) & 7) * 255 / 7);
        palette[i].b = (Uint8)((i & 3) * 255 / 3);
        palette[i].a = 255;
    }
}

void atma_gfx_init(void)
{
    const atma_platform_info *pi = atma_platform_info_get();

    palette_default();
    memset(sprites, 0, sizeof sprites);

    window = SDL_CreateWindow("atma", pi->screen_w * SCALE_START,
                              pi->screen_h * SCALE_START,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) return;
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) return;
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                               SDL_TEXTUREACCESS_TARGET, pi->screen_w, pi->screen_h);
    if (!screen) return;
    SDL_SetTextureScaleMode(screen, SDL_SCALEMODE_NEAREST);
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
    if (screen) SDL_DestroyTexture(screen);
    screen = NULL;
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

void atma_gfx_begin_frame(void)
{
    if (screen) SDL_SetRenderTarget(renderer, screen);
}

void atma_gfx_end_frame(void)
{
    int i;
    if (!renderer || !screen) return;

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
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, palette[color].r, palette[color].g,
                           palette[color].b, 255);
    SDL_RenderClear(renderer);
}

void atma_gfx_hline(u8 y, u8 color)
{
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, palette[color].r, palette[color].g,
                           palette[color].b, 255);
    SDL_RenderLine(renderer, 0, y + 0.5f, atma_platform_info_get()->screen_w, y + 0.5f);
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

void atma_host_palette_set(u8 index, u8 r, u8 g, u8 b)
{
    palette[index].r = r;
    palette[index].g = g;
    palette[index].b = b;
    palette[index].a = 255;
}

int atma_host_pattern_set_rgba(u8 pattern, u16 w, u16 h, const u8 *rgba)
{
    SDL_Texture *t;
    if (!renderer || !w || !h) return -1;

    t = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_STATIC, w, h);
    if (!t) return -1;
    SDL_UpdateTexture(t, NULL, rgba, w * 4);
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

    if (patterns[pattern]) SDL_DestroyTexture(patterns[pattern]);
    patterns[pattern] = t;
    pattern_w[pattern] = w;
    pattern_h[pattern] = h;
    return 0;
}

int atma_host_image_load(const char *path, u8 pattern, u16 w, u16 h)
{
    SDL_Surface *loaded, *rgba, *scaled;
    int ok = -1;

    if (!renderer) return -1;
    loaded = SDL_LoadPNG(path);
    if (!loaded) loaded = SDL_LoadBMP(path);
    if (!loaded) return -1;
    rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!rgba) return -1;
    scaled = SDL_ScaleSurface(rgba, w, h, SDL_SCALEMODE_LINEAR);
    SDL_DestroySurface(rgba);
    if (!scaled) return -1;
    ok = atma_host_pattern_set_rgba(pattern, w, h, scaled->pixels);
    SDL_DestroySurface(scaled);
    return ok;
}

int atma_host_spritesheet_load(const char *path, u16 frame_w, u16 frame_h,
                               u8 first_pattern)
{
    SDL_Surface *loaded, *sheet;
    int cols, rows, r, c, count = 0;

    if (!renderer || !frame_w || !frame_h) return -1;

    loaded = SDL_LoadPNG(path);
    if (!loaded) loaded = SDL_LoadBMP(path);
    if (!loaded) return -1;
    sheet = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!sheet) return -1;

    cols = sheet->w / frame_w;
    rows = sheet->h / frame_h;
    for (r = 0; r < rows; ++r) {
        for (c = 0; c < cols; ++c) {
            SDL_Surface *frame;
            int idx = first_pattern + count;
            SDL_Rect src;
            int ok;

            if (idx >= ATMA_MAX_PATTERNS) goto done;
            src.x = c * frame_w;
            src.y = r * frame_h;
            src.w = frame_w;
            src.h = frame_h;
            frame = SDL_CreateSurface(frame_w, frame_h, SDL_PIXELFORMAT_RGBA32);
            if (!frame) goto done;
            SDL_SetSurfaceBlendMode(sheet, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(sheet, &src, frame, NULL);
            ok = atma_host_pattern_set_rgba((u8)idx, frame_w, frame_h, frame->pixels);
            SDL_DestroySurface(frame);
            if (ok != 0) goto done;
            ++count;
        }
    }
done:
    SDL_DestroySurface(sheet);
    return count;
}
