/*
 * video_ppc.cpp - SheepShaver's video driver for BMMacintosh.
 *
 * SheepShaver's video.cpp is the Mac's video driver; this is its system
 * dependent part, as the Unix port's video_x.cpp and Basilisk II's SDL
 * driver are: the list of screen modes (VModes), switching between them,
 * the palette, and showing the screen.
 *
 * The Mac draws into its own frame buffer, in its own format (1, 2, 4 or 8
 * bits a pixel through its palette, or 16 and 32-bit colour, big endian),
 * which is in the Mac's memory (main_ppc.cpp). Each frame the lines that
 * changed since the last are converted to the display's RGB565.
 *
 * The screen can be 640x480, 800x600, 1024x768, 1152x870, 1280x1024 or
 * 1600x1200, in any depth: the Monitors control panel changes it, and
 * macppc.prefs keeps it ("screen", "displaycolordepth").
 *
 * From the Mac II's (src/port/ii/video_ii.cpp) and Basilisk II's SDL
 * driver's SheepShaver parts (video_sdl2.cpp).
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "main.h"
#include "prefs.h"
#include "video.h"
#include "video_defs.h"

#include "../../host.h"
#include "../bmmac.h"
#include "ppc.h"

static uint16_t palette565[256];
/* The frame buffer as last converted, to find the lines that changed. */
static uint8 *shadow;
static bool repaint;
static bool video_open;

static uint16_t rgb565(int r, int g, int b)
{
	return (uint16_t) (((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

/* The current mode is VModes[cur_mode]: its frame buffer starts afresh. */
static void switch_mode(void)
{
	const VideoInfo &mode = VModes[cur_mode];
	static const char *const depths[] = {"1 bit", "2 bit", "4 bit", "8 bit", "16 bit", "32 bit"};
	printf("BMMacintosh: Mac screen %ux%u, %s\n", (unsigned) mode.viXsize, (unsigned) mode.viYsize,
	       depths[(mode.viAppleMode - APPLE_1_BIT) % 6]);
	screen_base = ppc_frame_buffer_mac;
	memset(ppc_frame_buffer, 0, PPC_FRAME_BUFFER_SIZE);
	repaint = true;
	bmmac_screen_changed();
}

bool VideoInit(void)
{
	static const struct {
		uint32 w, h, id;
	} sizes[] = {
		{640, 480, APPLE_640x480}, {800, 600, APPLE_800x600}, {1024, 768, APPLE_1024x768},
		{1152, 870, APPLE_1152x900}, {1280, 1024, APPLE_1280x1024},
		{1600, 1200, APPLE_1600x1200}
	};
	/* A screen taller than the display is shrunk to it, and the Pi 5's
	   display hardware shrinks a picture to no less than 2/3: 1200 lines
	   need a display of 800 or more (as the Mac II's). */
	int display_w, display_h, width = 640, height = 480, n = 0;
	host_display_size(&display_w, &display_h);
	const char *screen = PrefsFindString("screen");
	if (screen != NULL)
		sscanf(screen, "%*[a-z]/%d/%d", &width, &height);
	int depth = DepthModeForPixelDepth(PrefsFindInt32("displaycolordepth"));
	if (PrefsFindInt32("displaycolordepth") <= 0)
		depth = APPLE_32_BIT;

	cur_mode = -1;
	for (unsigned s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
		if (display_h > 0 && 3 * display_h < 2 * (int) sizes[s].h)
			continue;
		for (int d = APPLE_1_BIT; d <= APPLE_32_BIT; d++) {
			VideoInfo *v = &VModes[n];
			v->viType = DIS_WINDOW;
			v->viRowBytes = TrivialBytesPerRow(sizes[s].w, d);
			v->viXsize = (uint16) sizes[s].w;
			v->viYsize = (uint16) sizes[s].h;
			v->viAppleMode = d;
			v->viAppleID = sizes[s].id;
			if ((int) sizes[s].w == width && (int) sizes[s].h == height && d == depth)
				cur_mode = n;
			n++;
		}
	}
	VModes[n].viType = DIS_INVALID;
	VModes[n].viRowBytes = 0;
	VModes[n].viXsize = VModes[n].viYsize = 0;
	VModes[n].viAppleMode = 0;
	VModes[n].viAppleID = 0;
	if (cur_mode < 0)
		cur_mode = APPLE_32_BIT - APPLE_1_BIT;    /* 640x480, millions */

	free(shadow);
	shadow = (uint8 *) malloc(PPC_FRAME_BUFFER_SIZE);
	if (shadow == NULL)
		return false;
	memset(palette565, 0, sizeof palette565);
	palette565[0] = 0xffff;
	video_activated = true;
	video_open = true;
	switch_mode();
	return true;
}

void VideoExit(void)
{
	/* The Mac's choice of screen is kept for its next start. */
	if (video_open) {
		const VideoInfo &mode = VModes[cur_mode];
		static const int bits[] = {1, 2, 4, 8, 16, 32};
		char text[32];
		snprintf(text, sizeof text, "win/%u/%u", (unsigned) mode.viXsize, (unsigned) mode.viYsize);
		PrefsReplaceString("screen", text);
		PrefsReplaceInt32("displaycolordepth", bits[(mode.viAppleMode - APPLE_1_BIT) % 6]);
		SavePrefs();
	}
	video_open = false;
	video_activated = false;
}

void VideoQuitFullScreen(void)
{
}

/* The video driver's VBL (the 60 Hz interrupt): the Mac's VBL tasks for
   the screen. */
void VideoVBL(void)
{
	if (private_data != NULL && private_data->interruptsEnabled)
		VSLDoInterruptService(private_data->vslServiceID);
}

void video_set_palette(void)
{
	int n = 256;
	switch (VModes[cur_mode].viAppleMode) {
	case APPLE_1_BIT: n = 2; break;
	case APPLE_2_BIT: n = 4; break;
	case APPLE_4_BIT: n = 16; break;
	default: break;
	}
	for (int i = 0; i < n; i++)
		palette565[i] = rgb565(mac_pal[i].red, mac_pal[i].green, mac_pal[i].blue);
	repaint = true;
}

void video_set_gamma(int n_colors)
{
	(void) n_colors;
}

int16 video_mode_change(VidLocals *csSave, uint32 ParamPtr)
{
	if (csSave->saveData == ReadMacInt32(ParamPtr + csData)
	    && csSave->saveMode == ReadMacInt16(ParamPtr + csMode))
		return noErr;
	for (int i = 0; VModes[i].viType != DIS_INVALID; i++) {
		if (ReadMacInt16(ParamPtr + csMode) == VModes[i].viAppleMode
		    && ReadMacInt32(ParamPtr + csData) == VModes[i].viAppleID) {
			csSave->saveMode = ReadMacInt16(ParamPtr + csMode);
			csSave->saveData = ReadMacInt32(ParamPtr + csData);
			csSave->savePage = ReadMacInt16(ParamPtr + csPage);
			cur_mode = i;
			switch_mode();
			WriteMacInt32(ParamPtr + csBaseAddr, screen_base);
			csSave->saveBaseAddr = screen_base;
			csSave->saveData = VModes[cur_mode].viAppleID;
			csSave->saveMode = VModes[cur_mode].viAppleMode;
			return noErr;
		}
	}
	return paramErr;
}

/* The Mac draws its own cursor. */
bool video_can_change_cursor(void)
{
	return false;
}

void video_set_cursor(void)
{
}

void video_set_dirty_area(int x, int y, int w, int h)
{
	(void) x;
	(void) y;
	(void) w;
	(void) h;
}

/* ---- Showing it ------------------------------------------------------------------- */

static void convert_line(const uint8 *src, uint16_t *dst, uint32 width, int mode)
{
	uint32 x;
	switch (mode) {
	case APPLE_1_BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[(src[x >> 3] >> (7 - (x & 7))) & 1];
		break;
	case APPLE_2_BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[(src[x >> 2] >> (6 - 2 * (x & 3))) & 3];
		break;
	case APPLE_4_BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[(src[x >> 1] >> (4 - 4 * (x & 1))) & 15];
		break;
	case APPLE_8_BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[src[x]];
		break;
	case APPLE_16_BIT:
		/* xRRRRRGGGGGBBBBB, big endian */
		for (x = 0; x < width; x++) {
			unsigned v = (unsigned) src[x * 2] << 8 | src[x * 2 + 1];
			dst[x] = (uint16_t) (((v & 0x7c00) << 1) | ((v & 0x03e0) << 1)
			                     | ((v & 0x0200) >> 4) | (v & 0x001f));
		}
		break;
	default:
		/* xRGB */
		for (x = 0; x < width; x++)
			dst[x] = rgb565(src[x * 4 + 1], src[x * 4 + 2], src[x * 4 + 3]);
		break;
	}
}

void emu_screen_view(int *w, int *h, double *aspect)
{
	if (!video_open) {
		*w = 640;
		*h = 480;
	} else {
		*w = VModes[cur_mode].viXsize;
		*h = VModes[cur_mode].viYsize;
	}
	*aspect = (double) *w / *h;
}

extern double ppc_copy_time;    /* main_ppc.cpp's log */

int emu_screen_copy(uint16_t *fb, int pitch_px, int force)
{
	int copied = FALSE;
	double start = host_time();
	if (!video_open)
		return FALSE;
	const VideoInfo &mode = VModes[cur_mode];
	if (repaint)
		force = TRUE;
	repaint = false;
	for (uint32 y = 0; y < mode.viYsize; y++) {
		const uint8 *src = ppc_frame_buffer + y * mode.viRowBytes;
		uint8 *old = shadow + y * mode.viRowBytes;
		if (!force && memcmp(src, old, mode.viRowBytes) == 0)
			continue;
		memcpy(old, src, mode.viRowBytes);
		convert_line(src, fb + y * pitch_px, mode.viXsize, mode.viAppleMode);
		bmmac_screen_lines((int) y, (int) y);
		copied = TRUE;
	}
	ppc_copy_time += host_time() - start;
	return copied;
}
