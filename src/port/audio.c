/*
 * audio.c - Hatari's sound output for BMAtariST.
 *
 * Replaces Hatari's src/sdl/audio.c (SDL's audio callback) with the kernel's
 * sound output (host.h), as src/retro/audio.c does for libretro: Hatari's
 * sound.c mixes the YM2149, the STE's DMA sound and the Falcon's into
 * AudioMixBuffer (16-bit stereo) as the emulation goes, and calls
 * Audio_Unlock() after each batch; the batch is written out from there.
 *
 * The emulation runs at the display's pace or a timer's, never quite the
 * sound output's (an ST's 50.05 Hz locked to a 50 Hz display is 0.1% fast),
 * so the output is kept about half full: the sound is squeezed or
 * stretched a little, the more the further the buffer is from half full,
 * up to 2% (as BMCoco and BMApple2 do). Dropping a whole sample, or what
 * doesn't fit, clicks.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

const char Audio_fileid[] = "BMAtariST audio.c";

#include "main.h"
#include "audio.h"
#include "configuration.h"
#include "sound.h"

#include "../host.h"
#include "bmatarist.h"

bool bSoundWorking = false;
int SoundBufferSize = 1024 / 4;      /* in samples: Hatari's lead (sound.c) */
int SdlAudioBufferSize = 0;
int pulse_swallowing_count = 0;

static bool playing;
static unsigned out_rate;            /* 0: no sound */
static unsigned out_latency;         /* frames */
static int16_t silence[2048 * 2];
static int volume = -1;              /* 0-100, -1 until read */

/* ---- Volume ---------------------------------------------------------------- */

int bmatarist_volume(void)
{
	if (volume < 0) {
		volume = host_option_int("volume", 100);
		if (volume < 0)
			volume = 0;
		if (volume > 100)
			volume = 100;
	}
	return volume;
}

void bmatarist_set_volume(int v)
{
	char text[8];
	volume = v < 0 ? 0 : v > 100 ? 100 : v;
	snprintf(text, sizeof text, "%d", volume);
	if (!host_option_set("volume", text) || !host_options_save())
		printf("BMAtariST: cannot save volume= in bmatarist.cfg\n");
}

/* N stereo frames to the output, at the volume. */
static void write_frames(const int16_t *samples, unsigned n)
{
	static int16_t scaled[1024 * 2];
	int v = bmatarist_volume();
	if (v >= 100) {
		host_sound_write(samples, n);
		return;
	}
	while (n > 0) {
		unsigned chunk = n > 1024 ? 1024 : n, i;
		for (i = 0; i < chunk * 2; i++)
			scaled[i] = (int16_t) (samples[i] * v / 100);
		host_sound_write(scaled, chunk);
		samples += chunk * 2;
		n -= chunk;
	}
}

/* ---- Output ---------------------------------------------------------------- */

#define AO_RATE 48000

unsigned bmatarist_sound_open(void)
{
	unsigned ms = (unsigned) host_option_int("audio_buffer_ms", 60);
	if (ms < 20)
		ms = 20;
	if (ms > 500)
		ms = 500;
	/* The output may run at another rate (the VideoCore's 44.1 kHz on the
	   Pi Zero to 3): Hatari then makes its sound at that rate. The
	   latency is asked for in 44.1 kHz frames (host.h). */
	out_rate = host_sound_open(AO_RATE, 2, 44100 * ms / 1000);
	out_latency = (out_rate ? out_rate : AO_RATE) * ms / 1000;
	if (out_rate == 0)
		printf("BMAtariST: no sound\n");
	return out_rate;
}

void bmatarist_feed_silence(void)
{
	unsigned frames;
	if (out_rate == 0)
		return;
	frames = host_sound_free_frames();
	if (frames > out_latency / 2) {
		frames -= out_latency / 2;
		while (frames > 0) {
			unsigned n = frames > 2048 ? 2048 : frames;
			host_sound_write(silence, n);
			frames -= n;
		}
	}
}

#define PLAY_CHUNK 512

/* N stereo frames resampled to WANT (linear interpolation, in 16.16 fixed
   point) into OUT. */
static void resample(const int16_t *in, unsigned n, int16_t *out, unsigned want)
{
	unsigned i, ch;
	for (i = 0; i < want; i++) {
		uint32_t pos = want > 1 ? (uint32_t) (((uint64_t) i * (n - 1) << 16) / (want - 1)) : 0;
		unsigned k = pos >> 16;
		int frac = (int) (pos & 0xffff);
		for (ch = 0; ch < 2; ch++) {
			int a = in[k * 2 + ch];
			int b = k + 1 < n ? in[(k + 1) * 2 + ch] : a;
			out[i * 2 + ch] = (int16_t) (a + (((b - a) * frac) >> 16));
		}
	}
}

/* Up to PLAY_CHUNK stereo frames to the output, squeezed or stretched to
   keep the buffer about half full. */
static void play_chunk(const int16_t *samples, unsigned n)
{
	static int16_t buf[(PLAY_CHUNK * 2 + 1) * 2];
	static double carry;   /* the fraction of a frame left over */
	unsigned free_frames, used, want;
	free_frames = host_sound_free_frames();
	used = free_frames < out_latency ? out_latency - free_frames : 0;
	if (n < 2) {
		want = n;
	} else if (used < out_latency / 4) {
		/* Running dry (the emulation slower than real time: a Pi too
		   slow for it): stretched to up to twice its length, lower in
		   pitch, rather than gaps. */
		unsigned quarter = out_latency / 4;
		want = n + 1 + n * (quarter - used) / quarter;
		if (want > PLAY_CHUNK * 2 + 1)
			want = PLAY_CHUNK * 2 + 1;
		carry = 0.0;
	} else {
		double half = out_latency / 2.0;
		double error = (half - used) / half;   /* 1 empty, -1 full */
		double exact;
		if (error > 1.0)
			error = 1.0;
		if (error < -1.0)
			error = -1.0;
		exact = n * (1.0 + 0.02 * error) + carry;
		want = (unsigned) exact;
		carry = exact - want;
	}
	if (want != n) {
		resample(samples, n, buf, want);
		samples = buf;
		n = want;
	}
	if (n > free_frames)
		n = free_frames;          /* no room at all (far ahead): drop it */
	if (n > 0)
		write_frames(samples, n);
}

/* N stereo frames to the output. */
static void play(const int16_t *samples, unsigned n)
{
	while (n > 0) {
		unsigned m = n > PLAY_CHUNK ? PLAY_CHUNK : n;
		play_chunk(samples, m);
		samples += m * 2;
		n -= m;
	}
}

/* ---- Hatari's audio functions ---------------------------------------------- */

/* Hatari makes its sound at the output's rate: the platform layer passes
   it with --sound, and out_rate is already known by then (it opens the
   output first). */
void Audio_Init(void)
{
	if (!ConfigureParams.Sound.bEnableSound) {
		bSoundWorking = false;
		return;
	}
	bSoundWorking = true;
	Audio_EnableAudio(ConfigureParams.Sound.bEnableSound);
}

void Audio_UnInit(void)
{
	Audio_EnableAudio(false);
	bSoundWorking = false;
}

void Audio_Lock(void)
{
}

/* Some samples have just been made (sound.c): out with them. */
void Audio_Unlock(void)
{
	int n = nGeneratedSamples;
	int pos = AudioMixBuffer_pos_read;
	if (playing && out_rate != 0) {
		while (n > 0) {
			int m = n;
			if (pos + m > AUDIOMIXBUFFER_SIZE)
				m = AUDIOMIXBUFFER_SIZE - pos;
			play(&AudioMixBuffer[pos][0], (unsigned) m);
			pos = (pos + m) & AUDIOMIXBUFFER_SIZE_MASK;
			n -= m;
		}
	}
	AudioMixBuffer_pos_read = (AudioMixBuffer_pos_read + nGeneratedSamples)
	                          & AUDIOMIXBUFFER_SIZE_MASK;
	nGeneratedSamples = 0;
}

void Audio_FreeSoundBuffer(void)
{
}

void Audio_EnableAudio(bool bEnable)
{
	playing = bEnable && bSoundWorking;
}
