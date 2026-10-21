/*
 * gui_event.c - the keyboard, mouse and gamepads into Hatari, for
 * BMAtariST.
 *
 * Replaces Hatari's src/sdl/gui_event.c (SDL's event loop). Hatari's IKBD
 * calls GuiEvent_EventHandler() each time it sends the ST its keyboard,
 * mouse and joystick reports (ikbd.c, about once a frame); the keys
 * pressed and the mouse's moves since go to the ST from here. The
 * joysticks are read by Hatari itself (joy_ui.c).
 *
 * F12, the power key, a gamepad's Home button or Select and Start together
 * aren't the ST's: they ask for the menu (or to switch off), which the main
 * loop (platform.c) opens between frames.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include "main.h"
#include "configuration.h"
#include "conv_st.h"
#include "gui_event.h"
#include "ikbd.h"
#include "video.h"

#include "../host.h"
#include "bmatarist.h"

static int menu_requested;
static int pad_last_menu[HOST_PADS];
static int pad_last_keys[HOST_PADS];

int bmatarist_menu_requested(void)
{
	int r = menu_requested;
	menu_requested = FALSE;
	return r;
}

void GuiEvent_WarpMouse(int x, int y, bool restore)
{
}

static void keyboard(void)
{
	int usage, pressed;
	while (host_key_event(&usage, &pressed)) {
		/* The power key: Shut Down (the Pi 400 sends it for Fn+F10). */
		if (usage == USAGE_POWER) {
			if (pressed)
				bmatarist_request_power(1);
			continue;
		}
		if (usage == KEY_MENU) {
			if (pressed)
				menu_requested = TRUE;
			continue;
		}
		bmatarist_key(usage, pressed);
	}
}

static void mouse(void)
{
	static int ax, ay;
	int dx, dy, buttons;
	if (!host_mouse_read(&dx, &dy, &buttons))
		return;
	/* Ignored for the first frames after a reset: TOS 4.04 gets confused
	   by a mouse moving that early (as in Hatari's own front ends). */
	if (nVBLs < 10)
		return;
	/* In a zoomed picture, the ST's pointer moves at the PC's pointer's
	   speed on the display: what's lost dividing is kept for later. */
	if (nScreenZoomX > 1) {
		dx += ax;
		ax = dx % nScreenZoomX;
		dx /= nScreenZoomX;
	}
	if (nScreenZoomY > 1) {
		dy += ay;
		ay = dy % nScreenZoomY;
		dy /= nScreenZoomY;
	}
	KeyboardProcessor.Mouse.dx += dx;
	KeyboardProcessor.Mouse.dy += dy;
	if (buttons & 1) {
		if (Keyboard.LButtonDblClk == 0)
			Keyboard.bLButtonDown |= BUTTON_MOUSE;
	} else {
		Keyboard.bLButtonDown &= ~BUTTON_MOUSE;
	}
	if (buttons & 2)
		Keyboard.bRButtonDown |= BUTTON_MOUSE;
	else
		Keyboard.bRButtonDown &= ~BUTTON_MOUSE;
	/* The middle button double-clicks the left one (as in Hatari). */
	if ((buttons & 4) && Keyboard.LButtonDblClk == 0)
		Keyboard.LButtonDblClk = 1;
}

/* Gamepad buttons that are keys: X is Space, Y Return (as in BMCoco and
   BMApple2), Start is Return too, for games' "press a key". */
static const struct {
	int bit;
	int usage;
} pad_keys[] = {
	{HOST_PAD_X, USAGE_SPACE},
	{HOST_PAD_Y, USAGE_RETURN},
};

static void pads(void)
{
	int pad;
	for (pad = 0; pad < HOST_PADS; pad++) {
		int state = host_pad_state(pad);
		int menu;
		unsigned k;
		if (state < 0)
			state = 0;
		menu = (state & HOST_PAD_HOME)
		       || ((state & HOST_PAD_SELECT) && (state & HOST_PAD_START));
		if (menu && !pad_last_menu[pad])
			menu_requested = TRUE;
		pad_last_menu[pad] = menu;
		for (k = 0; k < sizeof pad_keys / sizeof pad_keys[0]; k++) {
			int down = (state & pad_keys[k].bit) != 0;
			if (down != ((pad_last_keys[pad] & pad_keys[k].bit) != 0))
				bmatarist_key(pad_keys[k].usage, down);
		}
		pad_last_keys[pad] = state;
	}
}

void GuiEvent_EventHandler(void)
{
	keyboard();
	mouse();
	pads();
}
