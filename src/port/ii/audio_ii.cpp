/*
 * audio_ii.cpp - Basilisk II's sound output for BMMacintosh.
 *
 * Basilisk II is an Apple Mixer "component" to the Mac's Sound Manager:
 * when the output wants more, it raises an interrupt, and the interrupt
 * handler (AudioInterrupt()) asks the Mac's mixer for a block of sound.
 * Its SDL front end does that from SDL's sound thread and waits; here
 * main_ii.cpp's frame loop does it, four times a frame, whenever the
 * output is short of half full (bmmac_sound_wanted()), and the block goes
 * out as soon as the Mac has made it.
 *
 * The Mac makes its sound at the output's own rate (48 kHz over the Pi 4
 * and 5's HDMI, 44.1 kHz elsewhere), 16-bit stereo, so it isn't resampled.
 *
 * From Basilisk II's SDL audio (audio_sdl.cpp).
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "main.h"
#include "prefs.h"
#include "audio.h"
#include "audio_defs.h"

#include "../bmmac.h"
#include "ii.h"

#define BLOCK_FRAMES 512

static bool main_mute, speaker_mute;
static uint32 main_volume = 0x01000100, speaker_volume = 0x01000100;
/* An interrupt raised, and the Mac not yet asked for the block. */
static bool irq_pending;
static int16_t mix[BLOCK_FRAMES * 2];

void AudioInit(void)
{
	unsigned rate = bmmac_sound_rate();

	AudioStatus.sample_rate = (rate ? rate : 44100) << 16;
	AudioStatus.sample_size = 16;
	AudioStatus.channels = 2;
	AudioStatus.mixer = 0;
	AudioStatus.num_sources = 0;
	audio_component_flags = cmpWantsRegisterMessage | kStereoOut | k16BitOut;

	audio_sample_rates.clear();
	audio_sample_sizes.clear();
	audio_channel_counts.clear();
	audio_sample_rates.push_back(AudioStatus.sample_rate);
	audio_sample_sizes.push_back(16);
	audio_channel_counts.push_back(2);

	irq_pending = false;
	audio_open = false;
	if (PrefsFindBool("nosound") || rate == 0)
		return;
	audio_frames_per_block = BLOCK_FRAMES;
	audio_open = true;
}

void AudioExit(void)
{
	audio_open = false;
}

void ii_audio_stop(void)
{
	irq_pending = false;
}

void audio_enter_stream(void)
{
}

void audio_exit_stream(void)
{
}

void ii_audio_service(void)
{
	if (!audio_open || irq_pending || AudioStatus.num_sources == 0)
		return;
	if (bmmac_sound_wanted() == 0)
		return;
	irq_pending = true;
	SetInterruptFlag(INTFLAG_AUDIO);
	TriggerInterrupt();
}

void ii_audio_frame(void)
{
	/* No sound playing: the output is kept going with silence. */
	if (!audio_open || AudioStatus.num_sources == 0)
		bmmac_feed_silence();
}

/* 8.8 fixed point, left in the upper 16 bits: the right channel's. */
static int volume_of(uint32 vol)
{
	return (int) (vol & 0xffff);
}

/* The Mac's interrupt handler asks the mixer for a block, which goes out. */
void AudioInterrupt(void)
{
	irq_pending = false;
	if (AudioStatus.mixer == 0) {
		WriteMacInt32(audio_data + adatStreamInfo, 0);
		return;
	}
	M68kRegisters r;
	r.a[0] = audio_data + adatStreamInfo;
	r.a[1] = AudioStatus.mixer;
	Execute68k(audio_data + adatGetSourceData, &r);

	uint32 info = ReadMacInt32(audio_data + adatStreamInfo);
	if (info == 0)
		return;
	uint32 frames = ReadMacInt32(info + scd_sampleCount);
	uint16 channels = ReadMacInt16(info + scd_numChannels);
	uint16 bits = ReadMacInt16(info + scd_sampleSize);
	const uint8 *src = Mac2HostAddr(ReadMacInt32(info + scd_buffer));
	int vol = main_mute || speaker_mute ? 0
	          : volume_of(main_volume) * volume_of(speaker_volume) / 256;
	if (frames > BLOCK_FRAMES)
		frames = BLOCK_FRAMES;
	for (uint32 i = 0; i < frames; i++) {
		int left, right;
		if (bits == 8) {
			left = (src[i * channels] - 128) << 8;
			right = channels == 2 ? (src[i * 2 + 1] - 128) << 8 : left;
		} else {
			const uint8 *p = src + i * channels * 2;
			left = (int16_t) (p[0] << 8 | p[1]);
			right = channels == 2 ? (int16_t) (p[2] << 8 | p[3]) : left;
		}
		mix[i * 2] = (int16_t) (left * vol / 256);
		mix[i * 2 + 1] = (int16_t) (right * vol / 256);
	}
	bmmac_sound_play(mix, frames);
}

bool audio_set_sample_rate(int index)
{
	return index == 0;
}

bool audio_set_sample_size(int index)
{
	return index == 0;
}

bool audio_set_channels(int index)
{
	return index == 0;
}

bool audio_get_main_mute(void)
{
	return main_mute;
}

uint32 audio_get_main_volume(void)
{
	return main_volume;
}

bool audio_get_speaker_mute(void)
{
	return speaker_mute;
}

uint32 audio_get_speaker_volume(void)
{
	return speaker_volume;
}

void audio_set_main_mute(bool mute)
{
	main_mute = mute;
}

void audio_set_main_volume(uint32 vol)
{
	main_volume = vol;
}

void audio_set_speaker_mute(bool mute)
{
	speaker_mute = mute;
}

void audio_set_speaker_volume(uint32 vol)
{
	speaker_volume = vol;
}
