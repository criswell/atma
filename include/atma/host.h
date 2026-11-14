/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ATMA_HOST_H
#define ATMA_HOST_H

/*
 * Host-only extensions (desktop build). Nothing here exists on retro
 * targets, so game code that calls it must be guarded, e.g.
 * #ifdef ATMA_PLATFORM_HOST.
 *
 * Currently empty: palette and patterns are portable engine features now
 * (atma_gfx_palette_set, atma_gfx_pattern_set_indexed in gfx.h), and the
 * runtime image loaders went away with the decision that assets are
 * compiled in (tools/mkpats.py). This header remains the home for genuine
 * host conveniences (window title, screenshot dumps, debug overlays).
 */

#include "types.h"

#endif
