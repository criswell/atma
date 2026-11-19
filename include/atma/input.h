/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file input.h
 * \brief Abstract button bitmask and the input backend contract.
 */

#ifndef ATMA_INPUT_H
#define ATMA_INPUT_H

/**
 * \defgroup input Input
 * \brief A 16-bit bitmask of abstract buttons, identical on every platform.
 *
 * This is the core of the portability story - Kempston joystick, Spectrum
 * keyboard and host keys all collapse into these same bits.
 * @{
 */

#include "types.h"

/**
 * \name Buttons
 * \brief Directional and action buttons; map whatever the machine has onto
 *        these.
 * @{
 */
#define ATMA_BTN_UP     0x0001
#define ATMA_BTN_DOWN   0x0002
#define ATMA_BTN_LEFT   0x0004
#define ATMA_BTN_RIGHT  0x0008
#define ATMA_BTN_FIRE1  0x0010
#define ATMA_BTN_FIRE2  0x0020
#define ATMA_BTN_START  0x0040
/** Engine-reserved: "end the run". Backends must map a natural quit gesture
 *  (Escape on host, a key combo on Spectrum) to this bit; the engine checks
 *  it every tick, so games don't have to. */
#define ATMA_BTN_QUIT   0x8000
/** @} */

/** \brief Core-side input init, driven by the engine. */
void atma_input_init(void);
/** \brief Core-side input shutdown, driven by the engine. */
void atma_input_shutdown(void);

/**
 * \brief Snapshot the backend state.
 *
 * Called exactly once per tick by the engine - the pressed/released edges
 * below are only valid relative to the last poll, so don't call this from
 * game code.
 */
void atma_input_poll(void);

/**
 * \brief Buttons held right now.
 * \return Bitmask of ATMA_BTN_* bits, relative to the last poll.
 */
u16 atma_input_down(void);

/**
 * \brief Buttons that went down this tick.
 * \return Bitmask of ATMA_BTN_* bits, relative to the last poll.
 */
u16 atma_input_pressed(void);

/**
 * \brief Buttons that came up this tick.
 * \return Bitmask of ATMA_BTN_* bits, relative to the last poll.
 */
u16 atma_input_released(void);

/**
 * \defgroup input_backend Backend input contract
 * \ingroup input
 * \brief Implemented per platform, consumed by core only.
 *
 * read() returns the raw button bitmask for this instant; everything else
 * (edge detection, the QUIT check) is handled above it, in core.
 * @{
 */

/**
 * \brief Read the raw button bitmask for this instant.
 * \return Bitmask of ATMA_BTN_* bits as the hardware reports them now.
 */
u16 atma_input_backend_read(void);

/** \brief Backend input init, called from atma_input_init(). */
void atma_input_backend_init(void);

/** \brief Backend input shutdown, called from atma_input_shutdown(). */
void atma_input_backend_shutdown(void);

/** @} */
/** @} */

#endif
