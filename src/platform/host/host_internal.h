/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file host_internal.h
 * \brief Shared private declarations for the host backend's translation
 *        units.
 * \ingroup platform
 */

#ifndef ATMA_HOST_INTERNAL_H
#define ATMA_HOST_INTERNAL_H

/**
 * \brief Shared private declarations for the host backend's translation
 *        units (host.c, host_gfx.c, host_sound.c).
 *
 * \note Not installed, not for games.
 */

/**
 * \brief Toggle the host window between windowed and fullscreen.
 *
 * Bound to F11 and Alt+Enter in atma_input_backend_read().
 */
void atma_host_toggle_fullscreen(void);

#endif
