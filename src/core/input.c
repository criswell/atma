#include "atma/input.h"

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

void atma_input_poll(void)
{
    prev = cur;
    cur = atma_input_backend_read();
}

u16 atma_input_down(void)     { return cur; }
u16 atma_input_pressed(void)  { return (u16)(cur & ~prev); }
u16 atma_input_released(void) { return (u16)(~cur & prev); }
