/*
 * joy_ui.c - Hatari's host joysticks for BMAtariST: USB gamepads and the
 * joysticks wired to the GPIO header.
 *
 * Replaces Hatari's src/sdl/joy_ui.c (SDL's joysticks). Hatari's settings
 * give each of the ST's joystick ports (0, the mouse port; 1, the joystick
 * port; the STE's two joypad ports; the two on the printer port) a mode
 * (none, a host joystick, or the keyboard's arrow keys) and, for a host
 * joystick, which one (nJoyId). The host joysticks here are:
 *
 *   0  USB gamepad 1        2  GPIO joystick 1
 *   1  USB gamepad 2        3  GPIO joystick 2
 *
 * A gamepad's D-pad or left stick moves; A is fire, B Hatari's second
 * button (Space, for games that jump with it, if "jump on fire 2" is on),
 * and the shoulder buttons autofire.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include "main.h"
#include "configuration.h"
#include "joy.h"
#include "joy_ui.h"

#include "../host.h"
#include "bmatarist.h"

#define HOST_JOYSTICKS 4

/* Past a quarter of the range a stick counts as a direction. */
#define STICK_THRESHOLD 8192

static const char *const names[HOST_JOYSTICKS] = {
	"USB gamepad 1", "USB gamepad 2", "GPIO joystick 1", "GPIO joystick 2"
};

const char *JoyUI_GetName(int id)
{
	return id >= 0 && id < HOST_JOYSTICKS ? names[id] : "n/a";
}

int JoyUI_GetMaxId(void)
{
	return HOST_JOYSTICKS - 1;
}

/* Always all four: a gamepad or GPIO joystick can be plugged in at any
   time. */
int JoyUI_NumJoysticks(void)
{
	return HOST_JOYSTICKS;
}

bool JoyUI_ValidateJoyId(int i)
{
	JOYSTICK *joy = &ConfigureParams.Joysticks.Joy[i];
	if (joy->nJoyId >= 0 && joy->nJoyId < HOST_JOYSTICKS)
		return true;
	joy->nJoyId = 0;
	return false;
}

void JoyUI_Init(void)
{
}

void JoyUI_UnInit(void)
{
}

/* The arrow keys (USB HID usages: keymap.c gives them to Hatari as its
   "host key codes") and right Control for fire, as Hatari's own. */
void JoyUI_SetDefaultKeys(int id)
{
	JOYSTICK *joy = &ConfigureParams.Joysticks.Joy[id];
	joy->nKeyCodeUp = USAGE_UP;
	joy->nKeyCodeDown = USAGE_DOWN;
	joy->nKeyCodeLeft = USAGE_LEFT;
	joy->nKeyCodeRight = USAGE_RIGHT;
	joy->nKeyCodeFire = USAGE_RCTRL;
}

static int axis(int analog, int state, int neg_pad, int pos_pad, int gpio,
                int neg_gpio, int pos_gpio)
{
	if ((gpio & neg_gpio) || (state & neg_pad))
		return -32768;
	if ((gpio & pos_gpio) || (state & pos_pad))
		return 32767;
	if (analog >= 0)
		return analog - 32768 > 32767 ? 32767 : analog - 32768;
	return 0;
}

bool JoyUI_ReadJoystick(int nStJoyId, JOYREADING *joyread)
{
	int id = ConfigureParams.Joysticks.Joy[nStJoyId].nJoyId;
	int state = 0, gpio = 0, ax = -1, ay = -1;

	if (id < 2) {
		state = host_pad_state(id);
		if (state < 0)
			state = 0;
		ax = host_pad_axis(id, 0);
		ay = host_pad_axis(id, 1);
	} else if (id < HOST_JOYSTICKS) {
		gpio = host_gpio_joystick(id - 2);
	}

	joyread->XPos = axis(ax, state, HOST_PAD_LEFT, HOST_PAD_RIGHT, gpio,
	                     HOST_JOY_LEFT, HOST_JOY_RIGHT);
	joyread->YPos = axis(ay, state, HOST_PAD_UP, HOST_PAD_DOWN, gpio,
	                     HOST_JOY_UP, HOST_JOY_DOWN);
	/* Hatari's own dead zone is a third of the range (joy.c); a little
	   more than a quarter is enough for an analog stick. */
	if (joyread->XPos > -STICK_THRESHOLD && joyread->XPos < STICK_THRESHOLD)
		joyread->XPos = 0;
	if (joyread->YPos > -STICK_THRESHOLD && joyread->YPos < STICK_THRESHOLD)
		joyread->YPos = 0;

	joyread->Buttons = 0;
	if ((state & HOST_PAD_A) || (gpio & HOST_JOY_FIRE))
		joyread->Buttons |= JOYREADING_BUTTON1;
	if (state & HOST_PAD_B)
		joyread->Buttons |= JOYREADING_BUTTON2;
	if (state & (HOST_PAD_L | HOST_PAD_R))
		joyread->Buttons |= JOYREADING_BUTTON3;
	return true;
}

/* The fire buttons of the joystick in ST port nStJoyId: Hatari's Jaguar
   pads (STE ports) read them all. */
int JoyUI_GetRealFireButtons(int nStJoyId)
{
	JOYREADING r;
	JoyUI_ReadJoystick(nStJoyId, &r);
	return r.Buttons;
}
