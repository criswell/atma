#ifndef ATMA_ENGINE_H
#define ATMA_ENGINE_H

#include "types.h"
#include "platform.h"
#include "input.h"
#include "gfx.h"
#include "sound.h"

typedef struct {
    u8 target_fps;
    atma_cpu_speed cpu_speed;
    void (*init)(void);
    void (*update)(void);
    void (*render)(void);
    void (*shutdown)(void);
} atma_config;

int atma_run(const atma_config *config);
void atma_quit(void);

u16 atma_frame_count(void);

#endif
