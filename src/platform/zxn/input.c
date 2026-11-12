#include "atma/input.h"

void atma_input_backend_init(void) {}
void atma_input_backend_shutdown(void) {}

u16 atma_input_backend_read(void)
{
    /* TODO: Kempston/keyboard via z80 in_inp() */
    return 0;
}
