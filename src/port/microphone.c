/*
 * microphone.c - Hatari's microphone functions for BMAtariST.
 *
 * The Falcon's microphone input (its jack) has nothing to record from on
 * the Pi: as in Hatari's libretro core, it is never started.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include "main.h"
#include "microphone.h"

bool Microphone_Start(int sampleRate)
{
	return false;
}

void Microphone_Stop(void)
{
}
