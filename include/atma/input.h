#ifndef ATMA_INPUT_H
#define ATMA_INPUT_H

#include "types.h"

#define ATMA_BTN_UP     0x0001
#define ATMA_BTN_DOWN   0x0002
#define ATMA_BTN_LEFT   0x0004
#define ATMA_BTN_RIGHT  0x0008
#define ATMA_BTN_FIRE1  0x0010
#define ATMA_BTN_FIRE2  0x0020
#define ATMA_BTN_START  0x0040
#define ATMA_BTN_QUIT   0x8000

void atma_input_init(void);
void atma_input_shutdown(void);

void atma_input_poll(void);

u16 atma_input_down(void);
u16 atma_input_pressed(void);
u16 atma_input_released(void);

/* Backend: return the raw button state for this frame. */
u16 atma_input_backend_read(void);
void atma_input_backend_init(void);
void atma_input_backend_shutdown(void);

#endif
