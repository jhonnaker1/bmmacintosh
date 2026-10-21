/*
 * bmatarist.h - shared declarations for the BMAtariST Hatari port.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMATARIST_PORT_H_
#define BMATARIST_PORT_H_

#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* USB HID usage codes (keyboard page) that BMAtariST itself uses. */
enum {
	USAGE_A = 0x04,
	USAGE_RETURN = 0x28, USAGE_ESCAPE = 0x29, USAGE_BACKSPACE = 0x2a,
	USAGE_TAB = 0x2b, USAGE_SPACE = 0x2c,
	USAGE_F1 = 0x3a, USAGE_F4 = 0x3d, USAGE_F10 = 0x43, USAGE_F11 = 0x44,
	USAGE_F12 = 0x45,
	USAGE_PAUSE = 0x48, USAGE_HOME = 0x4a, USAGE_PAGEUP = 0x4b,
	USAGE_DELETE = 0x4c, USAGE_END = 0x4d, USAGE_PAGEDOWN = 0x4e,
	USAGE_RIGHT = 0x4f, USAGE_LEFT = 0x50, USAGE_DOWN = 0x51, USAGE_UP = 0x52,
	USAGE_KP_ENTER = 0x58, USAGE_POWER = 0x66,
	USAGE_LCTRL = 0xe0, USAGE_LSHIFT = 0xe1, USAGE_LALT = 0xe2,
	USAGE_RCTRL = 0xe4, USAGE_RSHIFT = 0xe5, USAGE_RALT = 0xe6
};

/* The ST has F1 to F10 of its own; F11 and F12 are free on a PC keyboard.
   F12 opens the menu (as Hatari's own F12 opens its settings, and BMCoco's
   F12 its menu); F11 is the ST's Help key (see keymap.c). */
#define KEY_MENU   USAGE_F12

/* ---- platform.c --------------------------------------------------------- */

void bmatarist_main(void);
const char *bmatarist_sd_root(void);
/* PATH in the card's DIR folder ("" for the root). */
void bmatarist_card_path(char *path, unsigned size, const char *dir, const char *name);
/* The frame buffer shown on the display (RGB565), for the on-screen
   display. */
uint16_t *bmatarist_framebuffer(int *pitch_px);
/* Shows the on-screen display's 640x460 area (TRUE) or the emulator's
   picture (FALSE). */
void bmatarist_osd_view(int on);
/* Called while a menu waits: keeps sound fed and runs network requests. */
void bmatarist_idle(void);
double bmatarist_refresh_rate(void);
int bmatarist_vsync_locked(void);
void bmatarist_video_settings_changed(void);
/* The display's resolution setting changed (the Pi 5). */
void bmatarist_display_mode_changed(void);
/* Reboot or power off, closing files first. */
void bmatarist_request_power(int power_off);
/* Web UI requests waiting to be done; menus close for them. */
int bmatarist_pending_requests(void);
/* The machine emulated: "Atari STE". */
const char *bmatarist_machine_text(void);
/* How fast the emulation runs, in percent of the real machine's speed,
   over the last second or so. */
int bmatarist_speed_percent(void);
/* The disk activity line (disk_activity): on or off. */
int bmatarist_disk_activity(void);
void bmatarist_set_disk_activity(int on);

/* Boots the machine again, as switching it off and on does (Hatari's cold
   reset), or presses its reset button (warm reset). */
void bmatarist_cold_reset(void);
void bmatarist_warm_reset(void);

/* Hatari's settings (ConfigureParams) as they are when a menu is opened,
   and putting changes made since into effect: Hatari works out what has
   to be reset (Change_CopyChangedParamsToConfiguration(), as its own
   settings dialog does), and the changes are saved in hatari.cfg. */
void bmatarist_settings_begin(void);
void bmatarist_settings_apply(int force_reset);

/* The TOS images found in the card's tos (and roms) folder: their paths
   and descriptions. */
int bmatarist_tos_count(void);
const char *bmatarist_tos_path(int n);
const char *bmatarist_tos_name(int n);

/* ---- audio.c ---------------------------------------------------------------- */

/* Opens the sound output; returns the rate in use (0 for none). */
unsigned bmatarist_sound_open(void);
void bmatarist_feed_silence(void);
/* The sound volume, 0-100 (kept in bmatarist.cfg). */
int bmatarist_volume(void);
void bmatarist_set_volume(int volume);

/* ---- screen.c ---------------------------------------------------------------- */

/* The frame buffer's size (the most Hatari's picture can be). */
#define BMATARIST_FB_W 1024
#define BMATARIST_FB_H 768

/* Sets up the frame buffer: RGB565, BMATARIST_FB_W x BMATARIST_FB_H. */
void bmatarist_screen_init(uint16_t *pixels, int pitch_px);
/* Copies Hatari's last picture to the frame buffer, if it changed (or
   FORCE). Returns TRUE if it was copied. */
int bmatarist_screen_copy(int force);
/* The picture's size, and the display's aspect ratio for it. */
void bmatarist_screen_view(int *w, int *h, double *aspect);
/* Lines at the bottom of the picture the notification and disk light are
   drawn over: they are copied again each frame. */
void bmatarist_screen_dirty_bottom(void);

/* ---- keymap.c ---------------------------------------------------------------- */

/* A key (USB HID usage) pressed or released on the ST's keyboard. */
void bmatarist_key(int usage, int pressed);
/* Releases every key held on the ST's keyboard (before a menu opens). */
void bmatarist_release_keys(void);
/* ---- gui_event.c --------------------------------------------------------- */

/* Reads the keyboard, mouse and gamepads into Hatari, once a frame (Hatari
   calls GuiEvent_EventHandler() at each VBL). Returns TRUE if the menu
   should open. */
int bmatarist_menu_requested(void);

/* ---- statusbar.c --------------------------------------------------------- */

/* Draws the disk activity line (the floppy drives' and the hard disk's)
   over the picture. */
void bmatarist_draw_disk_activity(uint16_t *fb, int pitch_px, int w, int h);

/* ---- menu.c ---------------------------------------------------------------- */

void bmatarist_menu(void);

#endif /* BMATARIST_PORT_H_ */
