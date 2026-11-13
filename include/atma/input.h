#ifndef ATMA_INPUT_H
#define ATMA_INPUT_H

/*
 * Input: a 16-bit bitmask of abstract buttons, identical on every platform.
 * This is the core of the portability story — Kempston joystick, Spectrum
 * keyboard and host keys all collapse into these same bits.
 */

#include "types.h"

/* Directional + action buttons; map whatever the machine has onto these. */
#define ATMA_BTN_UP     0x0001
#define ATMA_BTN_DOWN   0x0002
#define ATMA_BTN_LEFT   0x0004
#define ATMA_BTN_RIGHT  0x0008
#define ATMA_BTN_FIRE1  0x0010
#define ATMA_BTN_FIRE2  0x0020
#define ATMA_BTN_START  0x0040
/* Engine-reserved: "end the run". Backends must map a natural quit gesture
 * (Escape on host, a key combo on Spectrum) to this bit; the engine checks
 * it every tick, so games don't have to. */
#define ATMA_BTN_QUIT   0x8000

/* Core-side lifecycle, driven by the engine. */
void atma_input_init(void);
void atma_input_shutdown(void);

/* Snapshot the backend state. Called exactly once per tick by the engine —
 * the pressed/released edges below are only valid relative to the last
 * poll, so don't call this from game code. */
void atma_input_poll(void);

/* State relative to the last poll:
 * down     — held right now
 * pressed  — went down this tick
 * released — came up this tick
 * AND with the ATMA_BTN_* masks to test individual buttons. */
u16 atma_input_down(void);
u16 atma_input_pressed(void);
u16 atma_input_released(void);

/*
 * --- Backend contract: implemented per platform, consumed by core only. ---
 * read() returns the raw button bitmask for this instant; everything else
 * (edge detection, the QUIT check) is handled above it, in core.
 */
u16 atma_input_backend_read(void);
void atma_input_backend_init(void);
void atma_input_backend_shutdown(void);

#endif
