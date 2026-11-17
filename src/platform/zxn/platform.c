/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <arch/zxn.h>
#include <intrinsic.h>
#include <string.h>
#include <im2.h>
#include <z80.h>
#include "atma/platform.h"
#include "atma/gfx.h"

/*
 * ZX Spectrum Next backend — platform entry points.
 *
 * Notes on the hardware model: the frame counter below is meant to be bumped
 * from an IM2 interrupt service routine fired once per display refresh, so
 * "wait a frame" costs one HALT, and the 60 Hz refresh is forced via
 * NextReg 0x05 regardless of the machine's HDMI vs VGA timing.
 */

static const atma_platform_info info = { "zxn", 60, ATMA_SCREEN_W, ATMA_SCREEN_H };
static volatile u8 frames_seen; /* owned by the frame ISR */
static u8 frames_taken;         /* last count consumed by the engine */

const atma_platform_info *atma_platform_info_get(void) { return &info; }

/* Place the 257-byte vector table at 0x8000 (256-byte aligned) and jump vector at 0x8181.
 * Adjust addresses if your linker memory map uses 0x8000-0x8183 for code/BSS. */
#define IM2_TABLE_ADDR  ((void *)0x8000)
#define IM2_VECTOR_BYTE 0x81

IM2_DEFINE_ISR(frame_isr)
{
    ++frames_seen;
}

void atma_platform_init(void)
{
    frames_seen = 0;
    frames_taken = 0;

    /* 1. Force 60 Hz refresh via NextReg 0x05 (bit 2) */
    u8 periph1 = ZXN_READ_REG(REG_PERIPHERAL_1);
    ZXN_NEXTREGA(REG_PERIPHERAL_1, periph1 | 0x04);

    /* 2. Set up 257-byte IM2 vector table and install frame_isr */
    intrinsic_di();
    im2_Init(IM2_TABLE_ADDR);
    memset(IM2_TABLE_ADDR, IM2_VECTOR_BYTE, 257);
    im2_InstallISR(IM2_VECTOR_BYTE, frame_isr);
    intrinsic_ei();
}

void atma_platform_shutdown(void) {}

void atma_platform_set_cpu_speed(atma_cpu_speed speed)
{
    /* Runtime register write: ZXN_NEXTREG() token-pastes its arguments, so
     * the value must go through the A-register variant (ZXN_NEXTREGA). The
     * enum values match the turbo encoding 1:1: 0=28 MHz, 1=56, 2=112, 3=224. */
    ZXN_NEXTREGA(REG_TURBO_MODE, (u8)speed);
}

u8 atma_platform_wait_frame(void)
{
    u8 n;
    /* HALT parks the CPU until the next interrupt, i.e. the next refresh.
     * The u8 subtraction handles frames_seen's wraparound for free. */
    while (frames_seen == frames_taken) {
        intrinsic_halt();
    }
    n = (u8)(frames_seen - frames_taken);
    frames_taken = frames_seen;
    return n;
}
