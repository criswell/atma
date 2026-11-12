#ifndef ATMA_PLATFORM_H
#define ATMA_PLATFORM_H

#include "types.h"

typedef enum {
    ATMA_CPU_SLOW = 0,
    ATMA_CPU_MEDIUM,
    ATMA_CPU_FAST,
    ATMA_CPU_FASTEST
} atma_cpu_speed;

typedef struct {
    const char *name;
    u8 display_hz;
    u16 screen_w;
    u16 screen_h;
} atma_platform_info;

const atma_platform_info *atma_platform_info_get(void);

void atma_platform_init(void);
void atma_platform_shutdown(void);

void atma_platform_set_cpu_speed(atma_cpu_speed speed);

/* Block until the next display refresh; return frames elapsed since last call (>=1). */
u8 atma_platform_wait_frame(void);

#endif
