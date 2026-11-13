#ifndef ATMA_GFX_H
#define ATMA_GFX_H

/*
 * Graphics seam. Deliberately minimal: one background clear plus hardware
 * sprites, which every target machine (and Layer 2 on the Next) can do.
 * Richer primitives belong in game code or a helper layer on top.
 */

#include "types.h"

/* Core-side lifecycle, driven by the engine. */
void atma_gfx_init(void);
void atma_gfx_shutdown(void);

/* Brackets each render callback (engine calls them every tick). Backends
 * with a composited display use this to batch register writes or sync with
 * the retraces; on host they may be no-ops. */
void atma_gfx_begin_frame(void);
void atma_gfx_end_frame(void);

/* Fill the screen with a palette index. */
void atma_gfx_clear(u8 color);

/*
 * Hardware-sprite seam.
 * slot    — backend sprite index (the Next has 64 hardware sprites).
 * x, y    — position in screen pixels; i16 allows partially off-screen
 *           placements. Backends decide clipping/coordinates off-screen.
 * pattern — which bitmap to show, in the backend's pattern/attribute table.
 * flags   — backend-defined attribute bits (mirror, rotate, layer, ...).
 */
#define ATMA_SPR_MIRROR_X 0x08
#define ATMA_SPR_MIRROR_Y 0x04
#define ATMA_SPR_ROTATE   0x02 /* 90 degrees clockwise, applied before mirroring */

/* Hardware limits every backend honours; the Next is the ceiling. */
#define ATMA_MAX_SPRITES  64
#define ATMA_MAX_PATTERNS 256

void atma_gfx_sprite_set(u8 slot, i16 x, i16 y, u8 pattern, u8 flags);
void atma_gfx_sprite_hide(u8 slot);

#endif
