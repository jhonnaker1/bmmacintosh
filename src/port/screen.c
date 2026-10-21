/*
 * screen.c - Hatari's screen functions for BMAtariST.
 *
 * Replaces Hatari's src/sdl/screen.c (and src/retro/screen.c, which this
 * follows): Hatari draws each frame into a 32-bit 0xRRGGBB picture here
 * (Screen_SetVideoSize() gives its size: 832x552 for an ST in low or medium
 * resolution with its borders, 640x400 in high resolution, the TT's and
 * Falcon's modes scaled up to fit), and the platform layer copies it to the
 * display's RGB565 frame buffer once a frame (bmatarist_screen_copy()),
 * when it has changed.
 *
 * Hatari's statusbar is not used (its height is 0): the disk activity is
 * drawn by BMAtariST (statusbar.c), and the menu is BMAtariST's.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include "main.h"
#include "configuration.h"
#include "conv_gen.h"
#include "conv_st.h"
#include "screen.h"
#include "statusbar.h"
#include "vdi.h"
#include "video.h"

#include "../host.h"
#include "bmatarist.h"

bool bGrabMouse = false;
bool bInFullScreen = true;    /* the display is the ST's */

static uint32_t *picture;          /* what Hatari draws */
static int picture_w, picture_h;
static int picture_changed;
static int bottom_dirty;

static uint16_t *fb;               /* the display's */
static int fb_pitch;

void bmatarist_screen_init(uint16_t *pixels, int pitch_px)
{
	fb = pixels;
	fb_pitch = pitch_px;
}

/* ---- Hatari's screen functions -------------------------------------------- */

void Screen_GetPixelFormat(uint32_t *rmask, uint32_t *gmask, uint32_t *bmask,
                           int *rshift, int *gshift, int *bshift)
{
	if (rmask)
		*rmask = 0x00ff0000;
	if (gmask)
		*gmask = 0x0000ff00;
	if (bmask)
		*bmask = 0x000000ff;
	if (rshift)
		*rshift = 16;
	if (gshift)
		*gshift = 8;
	if (bshift)
		*bshift = 0;
}

uint32_t Screen_MapRGB(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((uint32_t) red << 16) | ((uint32_t) green << 8) | blue;
}

void Screen_GetDimension(uint32_t **pixels, int *width, int *height, int *pitch)
{
	if (pixels)
		*pixels = picture;
	if (width)
		*width = picture_w;
	if (height)
		*height = picture_h;
	if (pitch)
		*pitch = picture_w * 4;
}

int Screen_GetUISocket(void)
{
	return 0;
}

void Screen_GetDesktopSize(int *width, int *height)
{
	*width = BMATARIST_FB_W;
	*height = BMATARIST_FB_H;
}

void Screen_SetTextureScale(int width, int height, int win_width,
                            int win_height, bool bForceCreation)
{
}

bool Screen_SetVideoSize(int width, int height, bool bForceChange)
{
	uint32_t *p;
	if (width == picture_w && height == picture_h && picture != NULL && !bForceChange)
		return false;
	p = calloc((size_t) width * height, sizeof(*p));
	if (p == NULL) {
		printf("BMAtariST: no memory for a %dx%d picture\n", width, height);
		return false;
	}
	free(picture);
	picture = p;
	picture_w = width;
	picture_h = height;
	picture_changed = TRUE;
	printf("BMAtariST: picture %dx%d\n", width, height);
	bmatarist_video_settings_changed();
	return true;
}

void Screen_ModeChanged(bool bForceChange)
{
	ConvST_ChangeResolution(bForceChange);
}

void Screen_SetTitle(const char *title)
{
}

void Screen_Init(void)
{
	/* The ST's picture with its borders, low resolution doubled, as
	   Hatari's own default (configuration.c); the display scales it. The
	   TT's and Falcon's modes are scaled up to this size too (conv_gen.c),
	   and with a VDI mode it is the mode's size. */
	ConfigureParams.Screen.nZoomFactor = 1.0;
	ConfigureParams.Screen.nMaxWidth = 2 * NUM_VISIBLE_LINE_PIXELS;
	ConfigureParams.Screen.nMaxHeight = 2 * NUM_VISIBLE_LINES;
	ConfigureParams.Screen.bForceMax = false;
	ConfigureParams.Screen.bKeepResolution = false;
	ConfigureParams.Screen.bShowStatusbar = false;
	ConfigureParams.Screen.bShowDriveLed = false;
	/* Every frame is shown: the display is locked to the ST's. */
	ConfigureParams.Screen.nFrameSkips = 0;
}

void Screen_UnInit(void)
{
}

void Screen_ClearScreen(void)
{
	if (picture)
		memset(picture, 0, (size_t) picture_w * picture_h * sizeof(*picture));
	picture_changed = TRUE;
}

void Screen_EnterFullScreen(void)
{
}

void Screen_ReturnFromFullScreen(void)
{
}

bool Screen_UngrabMouse(void)
{
	return false;
}

void Screen_GrabMouseIfNecessary(void)
{
}

bool Screen_Lock(void)
{
	return picture != NULL;
}

void Screen_UnLock(void)
{
}

bool Screen_Draw(bool bForceFlip)
{
	bool changed;
	if (bQuitProgram || picture == NULL)
		return false;
	changed = ConvST_DrawFrame();
	if (changed || bForceFlip)
		picture_changed = TRUE;
	return changed;
}

void Screen_GenConvUpdate(bool update_statusbar)
{
	picture_changed = TRUE;
}

uint32_t Screen_GetGenConvWidth(void)
{
	return picture_w;
}

uint32_t Screen_GetGenConvHeight(void)
{
	return picture_h;
}

int Screen_SaveBMP(const char *filename)
{
	return -1;
}

void Screen_StatusbarMessage(const char *msg, uint32_t msecs)
{
	Statusbar_AddMessage(msg, msecs);
}

void Screen_MinimizeWindow(void)
{
}

uint32_t Screen_GetMouseState(int *mx, int *my)
{
	if (mx)
		*mx = 0;
	if (my)
		*my = 0;
	return 0;
}

bool Screen_ShowCursor(bool show)
{
	return false;
}

/* ---- To the display ----------------------------------------------------- */

static void copy_lines(int y0, int y1, int w)
{
	int x, y;
	for (y = y0; y < y1; y++) {
		const uint32_t *src = picture + (size_t) y * picture_w;
		uint16_t *dst = fb + (size_t) y * fb_pitch;
		for (x = 0; x < w; x++) {
			uint32_t p = src[x];
			dst[x] = (uint16_t) (((p >> 8) & 0xf800) | ((p >> 5) & 0x07e0)
			                     | ((p >> 3) & 0x001f));
		}
	}
}

/* How many lines at the bottom are drawn over (osd.c's notification and
   statusbar.c's disk light). */
#define OVERLAY_LINES 48

void bmatarist_screen_dirty_bottom(void)
{
	bottom_dirty = TRUE;
}

int bmatarist_screen_copy(int force)
{
	int w, h;
	if (fb == NULL || picture == NULL)
		return FALSE;
	w = picture_w < BMATARIST_FB_W ? picture_w : BMATARIST_FB_W;
	h = picture_h < BMATARIST_FB_H ? picture_h : BMATARIST_FB_H;
	if (picture_changed || force) {
		copy_lines(0, h, w);
		picture_changed = FALSE;
		bottom_dirty = FALSE;
		return TRUE;
	}
	if (bottom_dirty) {
		/* What was drawn over the picture last frame. */
		copy_lines(h > OVERLAY_LINES ? h - OVERLAY_LINES : 0, h, w);
		bottom_dirty = FALSE;
	}
	return FALSE;
}

void bmatarist_screen_view(int *w, int *h, double *aspect)
{
	double par = 1.0;
	*w = picture_w < BMATARIST_FB_W ? picture_w : BMATARIST_FB_W;
	*h = picture_h < BMATARIST_FB_H ? picture_h : BMATARIST_FB_H;
	if (*w <= 0 || *h <= 0) {
		*w = 640;
		*h = 400;
	}
	/* An ST's colour picture with its borders (Hatari's 832x552: 416x276
	   ST pixels, doubled) filled a 4:3 monitor, so its pixels are a little
	   taller than wide. The monochrome monitor's 640x400 and the TT's and
	   Falcon's modes (which Hatari scales to square pixels) are shown as
	   they are. */
	if (!bUseHighRes && !Config_IsMachineTT() && !Config_IsMachineFalcon()
	    && !bUseVDIRes && *w > *h)
		par = (4.0 / 3.0) / ((double) *w / *h);
	*aspect = (double) *w / *h * par;
}
