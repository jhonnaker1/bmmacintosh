/*
 * timing.c - Hatari's timing functions for BMAtariST.
 *
 * Replaces Hatari's src/sdl/timing.c, which sleeps at each VBL to keep the
 * emulation at the ST's speed. Here, as in src/retro/timing.c, the CPU core
 * stops at each VBL instead and returns to the platform layer's main loop
 * (platform.c), which shows the frame, waiting for the display's refresh
 * (or a timer) to keep the pace.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include "main.h"
#include "m68000.h"
#include "timing.h"

#include "../host.h"

int64_t Timing_GetTicks(void)
{
	return (int64_t) (host_time() * 1000000.0);
}

void Timing_PrintSpeed(void)
{
}

uint32_t Timing_SetRunVBLs(uint32_t vbls)
{
	static uint32_t nRunVBLs;
	if (!vbls)
		return nRunVBLs;
	nRunVBLs = vbls;
	return 0;
}

const char *Timing_SetVBLSlowdown(int factor)
{
	return NULL;
}

void Timing_WaitOnVbl(void)
{
	/* Back to the main loop for the frame to be shown. */
	M68000_SetSpecial(SPCFLAG_BRK);
	quit_program = UAE_QUIT;
}

void Timing_CheckForAccurateDelays(void)
{
}
