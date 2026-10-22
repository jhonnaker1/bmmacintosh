/*
 * keymap.c - the PC keyboard as the Mac's.
 *
 * USB HID usages to the Mac's key codes: the Apple Desktop Bus keyboard's
 * (Inside Macintosh: Macintosh Toolbox Essentials, figure 2-10, "Key codes
 * for the Apple Extended Keyboard"), which Basilisk II takes as they are.
 * The Mac Plus's keyboard sends the same codes for its keys (the main
 * block; umac's front end shifts them into its protocol).
 *
 * The keys around the space bar are where the Mac's are: Alt is Command
 * (next to the space bar, as Basilisk II has it), the Windows key Option.
 * Ctrl is Control. F11 is Help; F12 opens BMMacintosh's menu and isn't
 * the Mac's.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include "bmmac.h"

/* -1: not a Mac key. Indexed by the usage code (0x00-0xe7). */
static const signed char hid_to_mac[0xe8] = {
	[0x04] = 0x00,  /* A */
	[0x05] = 0x0b,  /* B */
	[0x06] = 0x08,  /* C */
	[0x07] = 0x02,  /* D */
	[0x08] = 0x0e,  /* E */
	[0x09] = 0x03,  /* F */
	[0x0a] = 0x05,  /* G */
	[0x0b] = 0x04,  /* H */
	[0x0c] = 0x22,  /* I */
	[0x0d] = 0x26,  /* J */
	[0x0e] = 0x28,  /* K */
	[0x0f] = 0x25,  /* L */
	[0x10] = 0x2e,  /* M */
	[0x11] = 0x2d,  /* N */
	[0x12] = 0x1f,  /* O */
	[0x13] = 0x23,  /* P */
	[0x14] = 0x0c,  /* Q */
	[0x15] = 0x0f,  /* R */
	[0x16] = 0x01,  /* S */
	[0x17] = 0x11,  /* T */
	[0x18] = 0x20,  /* U */
	[0x19] = 0x09,  /* V */
	[0x1a] = 0x0d,  /* W */
	[0x1b] = 0x07,  /* X */
	[0x1c] = 0x10,  /* Y */
	[0x1d] = 0x06,  /* Z */
	[0x1e] = 0x12,  /* 1 */
	[0x1f] = 0x13,  /* 2 */
	[0x20] = 0x14,  /* 3 */
	[0x21] = 0x15,  /* 4 */
	[0x22] = 0x17,  /* 5 */
	[0x23] = 0x16,  /* 6 */
	[0x24] = 0x1a,  /* 7 */
	[0x25] = 0x1c,  /* 8 */
	[0x26] = 0x19,  /* 9 */
	[0x27] = 0x1d,  /* 0 */
	[0x28] = 0x24,  /* Return */
	[0x29] = 0x35,  /* Escape */
	[0x2a] = 0x33,  /* Backspace: Delete */
	[0x2b] = 0x30,  /* Tab */
	[0x2c] = 0x31,  /* Space */
	[0x2d] = 0x1b,  /* - */
	[0x2e] = 0x18,  /* = */
	[0x2f] = 0x21,  /* [ */
	[0x30] = 0x1e,  /* ] */
	[0x31] = 0x2a,  /* \ */
	[0x32] = 0x2a,  /* # (non-US), where \ is on ISO keyboards */
	[0x33] = 0x29,  /* ; */
	[0x34] = 0x27,  /* ' */
	[0x35] = 0x32,  /* ` */
	[0x36] = 0x2b,  /* , */
	[0x37] = 0x2f,  /* . */
	[0x38] = 0x2c,  /* / */
	[0x39] = 0x39,  /* Caps Lock */
	[0x3a] = 0x7a,  /* F1 */
	[0x3b] = 0x78,  /* F2 */
	[0x3c] = 0x63,  /* F3 */
	[0x3d] = 0x76,  /* F4 */
	[0x3e] = 0x60,  /* F5 */
	[0x3f] = 0x61,  /* F6 */
	[0x40] = 0x62,  /* F7 */
	[0x41] = 0x64,  /* F8 */
	[0x42] = 0x65,  /* F9 */
	[0x43] = 0x6d,  /* F10 */
	[0x44] = 0x72,  /* F11: Help */
	[0x46] = 0x69,  /* Print Screen: F13 */
	[0x47] = 0x6b,  /* Scroll Lock: F14 */
	[0x48] = 0x71,  /* Pause: F15 */
	[0x49] = 0x72,  /* Insert: Help */
	[0x4a] = 0x73,  /* Home */
	[0x4b] = 0x74,  /* Page Up */
	[0x4c] = 0x75,  /* Delete: forward delete */
	[0x4d] = 0x77,  /* End */
	[0x4e] = 0x79,  /* Page Down */
	[0x4f] = 0x3c,  /* Right */
	[0x50] = 0x3b,  /* Left */
	[0x51] = 0x3d,  /* Down */
	[0x52] = 0x3e,  /* Up */
	[0x53] = 0x47,  /* Num Lock: Clear */
	[0x54] = 0x4b,  /* keypad / */
	[0x55] = 0x43,  /* keypad * */
	[0x56] = 0x4e,  /* keypad - */
	[0x57] = 0x45,  /* keypad + */
	[0x58] = 0x4c,  /* keypad Enter */
	[0x59] = 0x53,  /* keypad 1 */
	[0x5a] = 0x54,  /* keypad 2 */
	[0x5b] = 0x55,  /* keypad 3 */
	[0x5c] = 0x56,  /* keypad 4 */
	[0x5d] = 0x57,  /* keypad 5 */
	[0x5e] = 0x58,  /* keypad 6 */
	[0x5f] = 0x59,  /* keypad 7 */
	[0x60] = 0x5b,  /* keypad 8 */
	[0x61] = 0x5c,  /* keypad 9 */
	[0x62] = 0x52,  /* keypad 0 */
	[0x63] = 0x41,  /* keypad . */
	[0x64] = 0x0a,  /* the ISO key next to left Shift */
	[0x67] = 0x51,  /* keypad = */
	[0xe0] = 0x36,  /* left Ctrl: Control */
	[0xe1] = 0x38,  /* left Shift */
	[0xe2] = 0x37,  /* left Alt: Command */
	[0xe3] = 0x3a,  /* left Windows: Option */
	[0xe4] = 0x36,  /* right Ctrl: Control */
	[0xe5] = 0x38,  /* right Shift */
	[0xe6] = 0x37,  /* right Alt: Command */
	[0xe7] = 0x3a,  /* right Windows: Option */
};

/* Usages the table leaves out are 0 in it, which is also the A key's code:
   those are told apart by this list of the usages that are keys. */
static int is_mac_key(int usage)
{
	return usage == 0x04 || (usage < (int) sizeof hid_to_mac && hid_to_mac[usage] != 0);
}

int bmmac_mac_keycode(int usage)
{
	if (usage < 0 || usage >= (int) sizeof hid_to_mac || !is_mac_key(usage))
		return -1;
	return hid_to_mac[usage];
}
