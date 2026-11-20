/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * \file platform.c
 * \brief ZX Spectrum Next backend: platform entry points.
 * \ingroup platform
 */

#include <arch/zxn.h>
#include "atma/platform.h"
#include "atma/gfx.h"

/**
 * \brief ZX Spectrum Next backend - platform entry points.
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
 * Refresh policy: whatever the machine is actually doing. The Next's reset
 * cadence is 50 Hz (NextReg 0x05 bit 2 clear) and this backend neither
 * forces nor assumes anything else; info.display_hz below reports 50 and
 * the raster delivers 50. A game wanting a different cadence writes
 * NextReg 0x05 itself and then knows a fact the engine cannot.
 *
 * \note The cost is that an overrun is never reported: wait_frame always
 *       returns 1, so the engine's frame divisor still paces the game but
 *       never sees a skipped refresh.
 *
 * \note Register writes go through ZXN_WRITE_REG rather than the
 *       ZXN_NEXTREGA macro: the macro relies on the peephole pass rewriting
 *       the call into an inline OUT, and a plain library call links
 *       unconditionally.
 */

/**
 * \brief Raster line to sync on.
 *
 * The active video line counts 0..311, so low byte 192 occurs exactly once
 * per frame and is unambiguous.
 */
#define FRAME_SYNC_LINE 192

/**
 * \brief What the linked backend advertises.
 *
 * display_hz is the video cadence atma_platform_wait_frame actually
 * delivers, and that is 50 Hz here: 50 is the machine's native refresh and
 * the reset state of NextReg 0x05 bit 2, and this backend deliberately
 * leaves that register alone. The register is owned by whoever writes it
 * (the boot environment, or a game that reprograms it), so claiming a rate
 * nobody enforced made the engine pace games wrong: with 60 advertised and
 * the raster really at 50, a target_fps of 60 got divisor 1 and ticked at
 * the true 50, but a game computing px/second from display_hz ran 17
 * percent fast. ZEsarUX paces at 50 even with the 60 Hz bit set, verified
 * by frame-count probe 2026-11-20.
 */
static const atma_platform_info info = { "zxn", 50, ATMA_SCREEN_W, ATMA_SCREEN_H };

const atma_platform_info *atma_platform_info_get(void) { return &info; }

void atma_platform_init(void)
{
    u8 periph2;

    /* Turbo unlock. Without this bit REG_TURBO_MODE writes are ignored and
     * atma_platform_set_cpu_speed silently does nothing. */
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

/**
 * \brief ZX Next diagnostic sink.
 *
 * SDCC builds have no console and the core defines no default log sink
 * there (no weak symbols under SDCC), so the zxn backend supplies its own.
 * A real sink can replace this without touching the core.
 *
 * \param msg Ignored: the target has no console.
 */
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
