/*
 * keymap.c - USB keyboards as the ST's keyboard, for BMAtariST.
 *
 * Replaces Hatari's src/sdl/keymap.c. Keys go to the ST by where they are,
 * not by what they type (Hatari's "scancode" mapping): each PC key is the
 * ST key in the same place, so the ST's own keyboard layout (the TOS's
 * country, EmuTOS's keyboard setting) decides what it types, as on a real
 * ST with that keyboard.
 *
 * The ST's keys a PC keyboard doesn't have: Help is F11 (or Print Screen),
 * Undo is End (or Scroll Lock), the keypad's ( and ) are Page Up and Page
 * Down (as in Hatari), ClrHome is Home. F12 is BMAtariST's menu.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

const char Keymap_fileid[] = "BMAtariST keymap.c";

#include "main.h"
#include "configuration.h"
#include "ikbd.h"
#include "joy.h"
#include "keymap.h"

#include "../host.h"
#include "bmatarist.h"

#define NO_KEY 0x00

/* USB HID usage (keyboard page, 0x00-0xe7) to ST scan code. */
static const uint8_t usage_to_st[0xe8] = {
	/* 0x04-0x1d: A-Z */
	[0x04] = 0x1e, [0x05] = 0x30, [0x06] = 0x2e, [0x07] = 0x20,
	[0x08] = 0x12, [0x09] = 0x21, [0x0a] = 0x22, [0x0b] = 0x23,
	[0x0c] = 0x17, [0x0d] = 0x24, [0x0e] = 0x25, [0x0f] = 0x26,
	[0x10] = 0x32, [0x11] = 0x31, [0x12] = 0x18, [0x13] = 0x19,
	[0x14] = 0x10, [0x15] = 0x13, [0x16] = 0x1f, [0x17] = 0x14,
	[0x18] = 0x16, [0x19] = 0x2f, [0x1a] = 0x11, [0x1b] = 0x2d,
	[0x1c] = 0x15, [0x1d] = 0x2c,
	/* 0x1e-0x27: 1-9, 0 */
	[0x1e] = 0x02, [0x1f] = 0x03, [0x20] = 0x04, [0x21] = 0x05,
	[0x22] = 0x06, [0x23] = 0x07, [0x24] = 0x08, [0x25] = 0x09,
	[0x26] = 0x0a, [0x27] = 0x0b,
	[0x28] = 0x1c,  /* Return */
	[0x29] = 0x01,  /* Esc */
	[0x2a] = 0x0e,  /* Backspace */
	[0x2b] = 0x0f,  /* Tab */
	[0x2c] = 0x39,  /* Space */
	[0x2d] = 0x0c,  /* - */
	[0x2e] = 0x0d,  /* = */
	[0x2f] = 0x1a,  /* [ */
	[0x30] = 0x1b,  /* ] */
	[0x31] = 0x2b,  /* \ */
	[0x32] = 0x2b,  /* # (ISO keyboards, next to Return) */
	[0x33] = 0x27,  /* ; */
	[0x34] = 0x28,  /* ' */
	[0x35] = 0x29,  /* ` */
	[0x36] = 0x33,  /* , */
	[0x37] = 0x34,  /* . */
	[0x38] = 0x35,  /* / */
	[0x39] = 0x3a,  /* Caps Lock */
	/* 0x3a-0x43: F1-F10 */
	[0x3a] = 0x3b, [0x3b] = 0x3c, [0x3c] = 0x3d, [0x3d] = 0x3e,
	[0x3e] = 0x3f, [0x3f] = 0x40, [0x40] = 0x41, [0x41] = 0x42,
	[0x42] = 0x43, [0x43] = 0x44,
	[0x44] = 0x62,  /* F11: Help */
	[0x46] = 0x62,  /* Print Screen: Help */
	[0x47] = 0x61,  /* Scroll Lock: Undo */
	[0x49] = 0x52,  /* Insert */
	[0x4a] = 0x47,  /* Home: ClrHome */
	[0x4b] = 0x63,  /* Page Up: keypad ( */
	[0x4c] = 0x53,  /* Delete */
	[0x4d] = 0x61,  /* End: Undo */
	[0x4e] = 0x64,  /* Page Down: keypad ) */
	[0x4f] = 0x4d,  /* Right */
	[0x50] = 0x4b,  /* Left */
	[0x51] = 0x50,  /* Down */
	[0x52] = 0x48,  /* Up */
	[0x53] = 0x63,  /* Num Lock: keypad ( */
	[0x54] = 0x65,  /* keypad / */
	[0x55] = 0x66,  /* keypad * */
	[0x56] = 0x4a,  /* keypad - */
	[0x57] = 0x4e,  /* keypad + */
	[0x58] = 0x72,  /* keypad Enter */
	[0x59] = 0x6d, [0x5a] = 0x6e, [0x5b] = 0x6f,  /* keypad 1-3 */
	[0x5c] = 0x6a, [0x5d] = 0x6b, [0x5e] = 0x6c,  /* keypad 4-6 */
	[0x5f] = 0x67, [0x60] = 0x68, [0x61] = 0x69,  /* keypad 7-9 */
	[0x62] = 0x70,  /* keypad 0 */
	[0x63] = 0x71,  /* keypad . */
	[0x64] = 0x60,  /* the ISO key left of Z: < > */
	[0x67] = 0x63,  /* keypad = (Macs): keypad ( */
	[0xe0] = 0x1d,  /* left Control */
	[0xe1] = 0x2a,  /* left Shift */
	[0xe2] = 0x38,  /* left Alt: Alternate */
	[0xe4] = 0x1d,  /* right Control */
	[0xe5] = 0x36,  /* right Shift */
	[0xe6] = 0x38,  /* right Alt (AltGr): Alternate */
};

/* Which PC keys hold each ST key down: an ST key mapped from two PC keys
   (both Controls, Help) stays down until both are up. */
static uint8_t held[128];

static void st_key(uint8_t code, int pressed)
{
	if (code == NO_KEY || code >= 128)
		return;
	if (pressed) {
		if (held[code]++ == 0) {
			Keyboard.KeyStates[code] = true;
			IKBD_PressSTKey(code, true);
		}
	} else if (held[code] > 0 && --held[code] == 0) {
		IKBD_PressSTKey(code, false);
		Keyboard.KeyStates[code] = false;
	}
}

void bmatarist_key(int usage, int pressed)
{
	if (usage < 0 || usage >= (int) sizeof usage_to_st)
		return;
	/* A joystick port on the arrow keys (Hatari's joy.c, which is given
	   the USB HID usage as its host key code) takes them first. */
	if (pressed ? Joy_KeyDown(usage) : Joy_KeyUp(usage))
		return;
	st_key(usage_to_st[usage], pressed);
}

void bmatarist_release_keys(void)
{
	int code;
	for (code = 0; code < 128; code++) {
		if (held[code] > 0) {
			held[code] = 1;
			st_key((uint8_t) code, 0);
		}
	}
}

/* ---- Hatari's keymap functions ---------------------------------------------- */

void Keymap_Init(void)
{
	memset(held, 0, sizeof held);
}

void Keymap_InitShortcutDefaultKeys(void)
{
}

void Keymap_LoadRemapFile(const char *pszFileName)
{
}

/* Used by Hatari's debugger and its --key option; not in BMAtariST. */
void Keymap_SimulateCharacter(char asckey, bool press)
{
}

/* Hatari's settings keep the keyboard joystick's keys by name: here the
   name is the USB HID usage, "0x52". */
int Keymap_GetKeyFromName(const char *name)
{
	return name != NULL ? (int) strtol(name, NULL, 0) : 0;
}

const char *Keymap_GetKeyName(int keycode)
{
	static char name[8];
	if (!keycode)
		return "";
	snprintf(name, sizeof name, "0x%02x", keycode);
	return name;
}

void Keymap_SetCountry(int countrycode)
{
}

bool Keymap_IsShiftPressed(void)
{
	return held[0x2a] || held[0x36];
}
