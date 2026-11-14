/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ATMA_TYPES_H
#define ATMA_TYPES_H

/*
 * Fixed-width type shorthands used across the whole engine. On 8- and
 * 16-bit targets these matter: they keep struct layouts and register writes
 * stable regardless of what int resolves to on a given compiler.
 */

#include <stdint.h>

typedef uint8_t  u8;
typedef int8_t   i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;

#endif
