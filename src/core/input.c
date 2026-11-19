/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file input.c
 * \brief Core input state: edge detection over the backend snapshots.
 * \ingroup input
 */

#include "atma/input.h"

/**
 * \brief Core input state: the current and previous backend snapshots.
 *
 * The whole job of core input is to hold these two and derive edges from
 * them.
 */
static u16 cur;
static u16 prev;

void atma_input_init(void)
{
    cur = prev = 0;
    atma_input_backend_init();
}

void atma_input_shutdown(void)
{
    atma_input_backend_shutdown();
}

/**
 * \brief Slide the snapshot window.
 *
 * prev becomes last tick's state, cur the backend's state now. One call per
 * tick keeps the edges meaningful.
 */
void atma_input_poll(void)
{
    prev = cur;
    cur = atma_input_backend_read();
}

u16 atma_input_down(void)     { return cur; }
u16 atma_input_pressed(void)  { return (u16)(cur & ~prev); }  /* in cur, not in prev */
u16 atma_input_released(void) { return (u16)(~cur & prev); }  /* in prev, not in cur */
