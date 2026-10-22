/*
 * video_ii.cpp - Basilisk II's video driver for BMMacintosh.
 *
 * The Mac draws into its own frame buffer, in its own format (1, 2, 4 or 8
 * bits a pixel through its palette, or 16 and 32-bit colour, big endian),
 * which is in the Mac's memory (main_ii.cpp). Each frame the lines that
 * changed since the last are converted to the display's RGB565: there is
 * no VOSF (Basilisk II's way of finding the changes with the MMU).
 *
 * The screen can be 640x480, 800x600 or 1024x768, in any depth: the
 * Monitors control panel changes it, and macii.prefs keeps it ("screen",
 * "displaycolordepth").
 *
 * From Basilisk II's SDL and X11 drivers (video_sdl2.cpp, video_x.cpp).
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

#include "../bmmac.h"
#include "ii.h"

/* ---- The monitor ------------------------------------------------------------------ */

class BM_monitor_desc : public monitor_desc {
public:
	BM_monitor_desc(const vector<video_mode> &available_modes, video_depth default_depth,
	                uint32 default_id)
		: monitor_desc(available_modes, default_depth, default_id) {}
	~BM_monitor_desc() {}

	virtual void switch_to_current_mode(void);
	virtual void set_palette(uint8 *pal, int num);
	virtual void set_gamma(uint8 *gamma, int num);
};

static BM_monitor_desc *monitor;
static uint16_t palette565[256];
/* The frame buffer as last converted, to find the lines that changed. */
static uint8 *shadow;
static bool repaint;

static uint16_t rgb565(int r, int g, int b)
{
	return (uint16_t) (((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

void BM_monitor_desc::switch_to_current_mode(void)
{
	const video_mode &mode = get_current_mode();
	printf("BMMacintosh: Mac screen %ux%u, %s\n", (unsigned) mode.x, (unsigned) mode.y,
	       NameOfDepth(mode.depth));
	set_mac_frame_base(Host2MacAddr(ii_frame_buffer));
	memset(ii_frame_buffer, 0, II_FRAME_BUFFER_SIZE);
	repaint = true;
	bmmac_screen_changed();
}

void BM_monitor_desc::set_palette(uint8 *pal, int num)
{
	int i;
	for (i = 0; i < num && i < 256; i++)
		palette565[i] = rgb565(pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2]);
	repaint = true;
}

void BM_monitor_desc::set_gamma(uint8 *gamma, int num)
{
	(void) gamma;
	(void) num;
}

static void add_mode(vector<video_mode> &modes, uint32 width, uint32 height,
                     uint32 resolution_id, video_depth depth)
{
	video_mode mode;
	mode.x = width;
	mode.y = height;
	mode.resolution_id = resolution_id;
	mode.bytes_per_row = TrivialBytesPerRow(width, depth);
	mode.depth = depth;
	mode.user_data = 0;
	modes.push_back(mode);
}

bool VideoInit(bool classic)
{
	static const struct {
		uint32 w, h, id;
	} sizes[] = {
		{640, 480, 0x81}, {800, 600, 0x82}, {1024, 768, 0x83}
	};
	vector<video_mode> modes;
	int width = 640, height = 480, depth_bits, s, d;
	uint32 default_id = 0x81;
	video_depth default_depth;
	const char *screen = PrefsFindString("screen");

	if (screen != NULL)
		sscanf(screen, "%*[a-z]/%d/%d", &width, &height);
	depth_bits = PrefsFindInt32("displaycolordepth");
	default_depth = depth_bits > 0 ? DepthModeForPixelDepth(depth_bits) : VDEPTH_8BIT;

	if (classic) {
		/* The compact Macs' screen (not for Basilisk II here: their ROMs
		   need 24-bit addressing, which direct addressing hasn't). */
		add_mode(modes, 512, 342, 0x80, VDEPTH_1BIT);
		default_depth = VDEPTH_1BIT;
		default_id = 0x80;
	} else {
		for (s = 0; s < 3; s++) {
			for (d = VDEPTH_1BIT; d <= VDEPTH_32BIT; d++)
				add_mode(modes, sizes[s].w, sizes[s].h, sizes[s].id, (video_depth) d);
			if ((int) sizes[s].w == width && (int) sizes[s].h == height)
				default_id = sizes[s].id;
		}
	}
	monitor = new BM_monitor_desc(modes, default_depth, default_id);
	VideoMonitors.push_back(monitor);
	free(shadow);
	shadow = (uint8 *) malloc(II_FRAME_BUFFER_SIZE);
	if (shadow == NULL)
		return false;
	memset(palette565, 0, sizeof palette565);
	palette565[0] = 0xffff;
	monitor->switch_to_current_mode();
	return true;
}

void VideoExit(void)
{
	/* The Mac's choice of screen is kept for its next start. */
	if (monitor != NULL) {
		const video_mode &mode = monitor->get_current_mode();
		char text[32];
		static const int bits[] = {1, 2, 4, 8, 16, 32};
		snprintf(text, sizeof text, "win/%u/%u", (unsigned) mode.x, (unsigned) mode.y);
		PrefsReplaceString("screen", text);
		PrefsReplaceInt32("displaycolordepth", bits[mode.depth]);
		SavePrefs();
	}
	for (size_t i = 0; i < VideoMonitors.size(); i++)
		delete VideoMonitors[i];
	VideoMonitors.clear();
	monitor = NULL;
}

void VideoQuitFullScreen(void)
{
}

void VideoInterrupt(void)
{
}

void VideoRefresh(void)
{
}

/* ---- Showing it ------------------------------------------------------------------- */

static void convert_line(const uint8 *src, uint16_t *dst, uint32 width, video_depth depth)
{
	uint32 x;
	switch (depth) {
	case VDEPTH_1BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[(src[x >> 3] >> (7 - (x & 7))) & 1];
		break;
	case VDEPTH_2BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[(src[x >> 2] >> (6 - 2 * (x & 3))) & 3];
		break;
	case VDEPTH_4BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[(src[x >> 1] >> (4 - 4 * (x & 1))) & 15];
		break;
	case VDEPTH_8BIT:
		for (x = 0; x < width; x++)
			dst[x] = palette565[src[x]];
		break;
	case VDEPTH_16BIT:
		/* xRRRRRGGGGGBBBBB, big endian */
		for (x = 0; x < width; x++) {
			unsigned v = (unsigned) src[x * 2] << 8 | src[x * 2 + 1];
			dst[x] = (uint16_t) (((v & 0x7c00) << 1) | ((v & 0x03e0) << 1)
			                     | ((v & 0x0200) >> 4) | (v & 0x001f));
		}
		break;
	case VDEPTH_32BIT:
		/* xRGB */
		for (x = 0; x < width; x++)
			dst[x] = rgb565(src[x * 4 + 1], src[x * 4 + 2], src[x * 4 + 3]);
		break;
	}
}

void emu_screen_view(int *w, int *h, double *aspect)
{
	if (monitor == NULL) {
		*w = 640;
		*h = 480;
	} else {
		const video_mode &mode = monitor->get_current_mode();
		*w = (int) mode.x;
		*h = (int) mode.y;
	}
	*aspect = (double) *w / *h;
}

int emu_screen_copy(uint16_t *fb, int pitch_px, int force)
{
	uint32 y;
	int copied = FALSE;
	if (monitor == NULL)
		return FALSE;
	const video_mode &mode = monitor->get_current_mode();
	if (repaint)
		force = TRUE;
	repaint = false;
	for (y = 0; y < mode.y; y++) {
		const uint8 *src = ii_frame_buffer + y * mode.bytes_per_row;
		uint8 *old = shadow + y * mode.bytes_per_row;
		if (!force && memcmp(src, old, mode.bytes_per_row) == 0)
			continue;
		memcpy(old, src, mode.bytes_per_row);
		convert_line(src, fb + y * pitch_px, mode.x, mode.depth);
		copied = TRUE;
	}
	return copied;
}
