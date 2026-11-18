/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <arch/zxn.h>
#include "atma/platform.h"
#include "atma/gfx.h"

/*
 * ZX Spectrum Next backend — platform entry points.
 *
 * Frame pacing polls the raster instead of using HALT. Two facts about this
 * target force that: the zxn CRT starts with interrupts disabled
 * (TAR__crt_enable_eidi = 0x01 in crt_config.inc) and neither this program
 * nor its crt installs any interrupt vector at 0x0038, so a HALT would
 * never resume and IM 1 would jump to whatever the boot environment left
 * there. IM 2 would work but needs a 257-byte page-aligned vector table
 * plus an ISR living in the same 256-byte page, which is a lot of machinery
 * for a frame tick. Polling NextReg 0x1F costs a few dozen T-states per
 * sample and needs no table, no ISR and no assumption about the memory
 * map.
 *
 * The cost is that an overrun is never reported: wait_frame always returns 1,
 * so the engine's frame divisor still paces the game but never sees a skipped
 * refresh.
 *
 * Register writes go through ZXN_WRITE_REG rather than the ZXN_NEXTREGA
 * macro: the macro relies on the peephole pass rewriting the call into an
 * inline OUT, and a plain library call links unconditionally.
 */

/* Raster line to sync on. The active video line counts 0..311, so low byte
 * 192 occurs exactly once per frame and is unambiguous. */
#define FRAME_SYNC_LINE 192

static const atma_platform_info info = { "zxn", 60, ATMA_SCREEN_W, ATMA_SCREEN_H };

const atma_platform_info *atma_platform_info_get(void) { return &info; }

void atma_platform_init(void)
{
    u8 periph1;
    u8 periph2;

    /* Force the 60 Hz refresh (NextReg 0x05 bit 2) so the engine's pacing
     * and the game's target_fps agree regardless of HDMI vs VGA timing. */
    periph1 = ZXN_READ_REG(REG_PERIPHERAL_1);
    ZXN_WRITE_REG(REG_PERIPHERAL_1, (u8)(periph1 | RP1_RATE_60));

    /* Unlock the turbo register. Without this bit REG_TURBO_MODE writes are
     * ignored and atma_platform_set_cpu_speed silently does nothing. */
    periph2 = ZXN_READ_REG(REG_PERIPHERAL_2);
    ZXN_WRITE_REG(REG_PERIPHERAL_2, (u8)(periph2 | RP2_ENABLE_TURBO));
}

void atma_platform_shutdown(void) {}

void atma_platform_set_cpu_speed(atma_cpu_speed speed)
{
    /* The turbo register's ladder is 3.5 / 7 / 14 / 28 MHz (the RTM_*
     * constants in z88dk's arch/zxn.h); 28 is the TBBlue maximum. The
     * enum's values map onto it 1:1, slowest to fastest. */
    ZXN_WRITE_REG(REG_TURBO_MODE, (u8)speed);
}

/* SDCC builds have no console and the core defines no default log sink
 * there (no weak symbols under SDCC), so the zxn backend supplies its own.
 * A real sink can replace this without touching the core. */
void atma_log_backend(const char *msg)
{
    (void)msg;
}

u8 atma_platform_wait_frame(void)
{
    /* Spin until the raster reaches the sync line, which happens once per
     * refresh. */
    while (ZXN_READ_REG(REG_ACTIVE_VIDEO_LINE_L) != FRAME_SYNC_LINE) {
    }
    return 1;
}
