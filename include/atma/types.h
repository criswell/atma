/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file types.h
 * \brief Fixed-width type shorthands used across the whole engine.
 */

#ifndef ATMA_TYPES_H
#define ATMA_TYPES_H

/**
 * \defgroup types Types
 * \brief Fixed-width shorthands, stable across 8-, 16- and 32-bit targets.
 *
 * On 8- and 16-bit targets these matter: they keep struct layouts and
 * register writes stable regardless of what int resolves to on a given
 * compiler.
 * @{
 */

#include <stdint.h>

typedef uint8_t  u8;   /**< Unsigned 8-bit. */
typedef int8_t   i8;   /**< Signed 8-bit. */
typedef uint16_t u16;  /**< Unsigned 16-bit. */
typedef int16_t  i16;  /**< Signed 16-bit. */
typedef uint32_t u32;  /**< Unsigned 32-bit. */
typedef int32_t  i32;  /**< Signed 32-bit. */

/** @} */

#endif
