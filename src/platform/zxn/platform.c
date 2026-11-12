#include <arch/zxn.h>
#include <intrinsic.h>
#include <z80.h>
#include "atma/platform.h"

static const atma_platform_info info = { "zxn", 60, 256, 192 };
static volatile u8 frames_seen;
static u8 frames_taken;

const atma_platform_info *atma_platform_info_get(void) { return &info; }

void atma_platform_init(void)
{
    frames_seen = 0;
    frames_taken = 0;
    /* TODO: install IM2 frame ISR incrementing frames_seen; force 60Hz via NextReg 0x05 */
}

void atma_platform_shutdown(void) {}

void atma_platform_set_cpu_speed(atma_cpu_speed speed)
{
    ZXN_NEXTREG(REG_TURBO_MODE, (u8)speed);
}

u8 atma_platform_wait_frame(void)
{
    u8 n;
    /* TODO: replace with ISR-driven counter; polling placeholder */
    while (frames_seen == frames_taken) {
        intrinsic_halt();
    }
    n = (u8)(frames_seen - frames_taken);
    frames_taken = frames_seen;
    return n;
}
