/*
 * main_plus.c - umac's front end for BMMacintosh: the Mac Plus.
 *
 * umac (Matt Evans, the core of pico-mac) is a Mac 128K, 512K and Plus:
 * Musashi's 68000, the VIA, the SCC (for the mouse) and a disk driver of
 * its own in place of the floppy controller, which reads and writes a disk
 * image of any size. It runs the Mac Plus's ROM (the last one, 4D1F8172),
 * patched by rom.c, with 128K to 4 MB of RAM.
 *
 * Each frame (the Mac's 60.15 Hz) runs the 68000 for a frame's time at
 * 7.8336 MHz, umac_loop()'s 5 ms at a time, then raises the vertical blank
 * (and the one second interrupt each second); bmmac_frame() shows it.
 *
 * The screen is 512x342 in black and white, at the top of RAM, and the
 * sound buffer above it: 370 bytes a frame (22.25 kHz), played as they are
 * (umac doesn't tell the VIA's volume or sound switch, so silence is
 * whatever doesn't change: a constant is taken out).
 *
 * The keyboard is the Plus's (the main keys; umac's keyboard has no
 * arrow keys or keypad): umac takes one key at a time, which is fed it
 * every other frame. The disk is read and written in place on the SD card.
 * Settings (plus_rom, plus_ram, plus_disk) are in bmmac.cfg.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "umac.h"
#include "rom.h"

#include "../../host.h"
#include "../bmmac.h"

/* umac's RAM size (machw.h's RAM_SIZE): the menu's choice. */
unsigned umac_memsize_kb = 4096;

#define PLUS_ROM_SIZE (128 * 1024)
#define SCREEN_W 512
#define SCREEN_H 342
#define CPU_HZ 7833600.0
#define FRAME_HZ 60.15

static uint8_t *rom;
static uint8_t *ram;
static FILE *disk;
static int disk_read_only;
static const struct bmmac_rom *rom_in_use;
static volatile int restart_requested;

/* ---- Settings ------------------------------------------------------------------ */

static const char *setting(const char *name)
{
	return host_option(name, "");
}

static unsigned ram_kb(void)
{
	static const unsigned sizes[] = {128, 512, 1024, 2048, 4096};
	unsigned kb = (unsigned) host_option_int("plus_ram", 4096), i;
	for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++)
		if (sizes[i] == kb)
			return kb;
	return 4096;
}

/* ---- The disk ------------------------------------------------------------------ */

static int disk_read(void *ctx, uint8_t *data, unsigned int offset, unsigned int len)
{
	(void) ctx;
	bmmac_disk_accessed();
	if (disk == NULL || fseek(disk, (long) offset, SEEK_SET) != 0
	    || fread(data, 1, len, disk) != len)
		return -1;
	return 0;
}

static int disk_write(void *ctx, uint8_t *data, unsigned int offset, unsigned int len)
{
	(void) ctx;
	bmmac_disk_accessed();
	if (disk == NULL || disk_read_only || fseek(disk, (long) offset, SEEK_SET) != 0
	    || fwrite(data, 1, len, disk) != len)
		return -1;
	return 0;
}

static void close_disk(void)
{
	if (disk != NULL) {
		fflush(disk);
		fclose(disk);
	}
	disk = NULL;
}

/* The disk image in bmmac.cfg, read and written in place: a raw image (not
   DiskCopy 4.2), a floppy's or a hard disk's. When the Mac ejects it, umac
   leaves the drive empty until the Mac restarts. */
static int open_disk(disc_descr_t *d)
{
	const char *path = setting("plus_disk");
	struct stat st;
	close_disk();
	if (path[0] == '\0')
		return FALSE;
	disk_read_only = FALSE;
	disk = fopen(path, "r+b");
	if (disk == NULL) {
		disk_read_only = TRUE;
		disk = fopen(path, "rb");
	}
	if (disk == NULL || stat(path, &st) != 0) {
		printf("BMMacintosh: cannot open the disk image %s\n", path);
		close_disk();
		return FALSE;
	}
	d->base = NULL;
	d->size = (unsigned int) st.st_size;
	d->read_only = disk_read_only;
	d->op_ctx = NULL;
	d->op_read = disk_read;
	d->op_write = disk_write;
	printf("BMMacintosh: disk %s, %u KB%s\n", path, (unsigned) (st.st_size / 1024),
	       disk_read_only ? ", read only" : "");
	return TRUE;
}

/* The Mac ejected its disk (umac's disk driver calls this). umac's own
   resets the Mac (see umac.mk); here the drive stays empty until the Mac
   is restarted from the menu. */
void umac_disc_ejected(void)
{
	printf("BMMacintosh: the Mac ejected its disk\n");
}

void emu_sync_disks(void)
{
	if (disk != NULL)
		fflush(disk);
}

/* ---- Keyboard and mouse --------------------------------------------------------- */

#define KEY_QUEUE 32

static uint8_t key_queue[KEY_QUEUE];
static int key_head, key_tail;
static uint8_t key_down[128];
static int mouse_button;

/* The Plus's keyboard has the main keys only. */
static int plus_key(int code)
{
	return code <= 0x3a && code != 0x34 && code != 0x35 && code != 0x36;
}

void emu_key(int code, int pressed)
{
	int next = (key_tail + 1) % KEY_QUEUE;
	if (code < 0 || code > 127 || !plus_key(code))
		return;
	if (key_down[code] == (pressed != 0))
		return;
	key_down[code] = pressed != 0;
	if (next == key_head)
		return;   /* full: dropped */
	/* The Plus keyboard's transition code: the key code shifted left,
	   bit 0 set (bit 7 for a release is added by umac). */
	key_queue[key_tail] = (uint8_t) ((code << 1) | 1);
	key_queue[key_tail] |= pressed ? 0 : 0x80;
	key_tail = next;
}

void emu_release_keys(void)
{
	int code;
	for (code = 0; code < 128; code++)
		if (key_down[code])
			emu_key(code, FALSE);
	mouse_button = 0;
	umac_mouse(0, 0, 0);
}

static void feed_key(void)
{
	static int wait;
	uint8_t k;
	if (key_head == key_tail || ++wait < 2)
		return;
	wait = 0;
	k = key_queue[key_head];
	key_head = (key_head + 1) % KEY_QUEUE;
	umac_kbd_event(k & 0x7f, !(k & 0x80));
}

void emu_mouse(int dx, int dy, int buttons)
{
	mouse_button = buttons & 1;
	/* umac's Y goes up. */
	umac_mouse(dx, -dy, mouse_button);
}

/* ---- Screen and sound ------------------------------------------------------------ */

static uint8_t shadow[SCREEN_W / 8 * SCREEN_H];
static int repaint = TRUE;

void emu_screen_view(int *w, int *h, double *aspect)
{
	*w = SCREEN_W;
	*h = SCREEN_H;
	*aspect = (double) SCREEN_W / SCREEN_H;
}

int emu_screen_copy(uint16_t *fb, int pitch_px, int force)
{
	const uint8_t *screen;
	int y, x, copied = FALSE;
	if (ram == NULL)
		return FALSE;
	screen = ram + umac_get_fb_offset();
	if (repaint)
		force = TRUE;
	repaint = FALSE;
	for (y = 0; y < SCREEN_H; y++) {
		const uint8_t *src = screen + y * (SCREEN_W / 8);
		uint8_t *old = shadow + y * (SCREEN_W / 8);
		uint16_t *dst = fb + y * pitch_px;
		if (!force && memcmp(src, old, SCREEN_W / 8) == 0)
			continue;
		memcpy(old, src, SCREEN_W / 8);
		for (x = 0; x < SCREEN_W; x++)
			dst[x] = (src[x >> 3] & (0x80 >> (x & 7))) ? 0x0000 : 0xffff;
		copied = TRUE;
	}
	return copied;
}

/* The Plus's sound buffer: 370 words below the top of RAM, the sample in
   each word's high byte. */
#define SOUND_SAMPLES 370
#define SOUND_OFFSET 0x300

static void play_sound(void)
{
	static int16_t out[1200 * 2];
	static double dc;
	unsigned rate = bmmac_sound_rate(), n, i;
	const uint8_t *buf;
	if (rate == 0 || ram == NULL)
		return;
	buf = ram + umac_memsize_kb * 1024 - SOUND_OFFSET;
	/* A frame's 370 samples (22.25 kHz) at the output's rate. */
	n = (unsigned) (rate / FRAME_HZ + 0.5);
	if (n > 1200)
		n = 1200;
	for (i = 0; i < n; i++) {
		unsigned k = i * SOUND_SAMPLES / n;
		double v = (double) buf[k * 2] - 128.0;
		int s;
		/* What doesn't change is silence. */
		dc += (v - dc) * 0.002;
		s = (int) ((v - dc) * 160.0);
		if (s > 32767)
			s = 32767;
		if (s < -32768)
			s = -32768;
		out[i * 2] = out[i * 2 + 1] = (int16_t) s;
	}
	bmmac_sound_play(out, n);
}

/* ---- The emulator's side of the front end ----------------------------------------- */

int emu_machine(void)
{
	return BMMAC_PLUS;
}

const char *emu_machine_text(void)
{
	static char text[64];
	unsigned kb = umac_memsize_kb;
	if (kb >= 1024)
		snprintf(text, sizeof text, "Macintosh Plus, %u MB", kb / 1024);
	else
		snprintf(text, sizeof text, "Macintosh Plus, %uK", kb);
	return text;
}

const char *emu_credits(void)
{
	return "Emulation: umac (MIT), by Matt Evans, with Karl Stenerud's\n"
	       "Musashi 68000 (MIT).";
}

double emu_frame_rate(void)
{
	return FRAME_HZ;
}

const char *const *emu_run_types(void)
{
	static const char *const types[] = {"dsk", "img", "image", "hfv", "hda", NULL};
	return types;
}

void emu_cold_reset(void)
{
	restart_requested = TRUE;
}

/* The web UI's "start this file": the disk, and the Mac starts again. */
void emu_autostart(const char *path)
{
	bmmac_save_setting("plus_disk", path);
	emu_cold_reset();
}

/* ---- Running ----------------------------------------------------------------------- */

static int start_mac(char *why, size_t size)
{
	disc_descr_t discs[DISC_NUM_DRIVES];
	const struct bmmac_rom *r = bmmac_rom_find(setting("plus_rom"));

	if (r == NULL || r->machine != BMMAC_PLUS)
		r = bmmac_rom_default(BMMAC_PLUS);
	if (r == NULL || bmmac_rom_load(r, rom, PLUS_ROM_SIZE) != PLUS_ROM_SIZE) {
		snprintf(why, size, "No Mac Plus ROM in the roms folder.\n"
		         "See roms/ROMS.txt%s.", BMMAC_RASPPI >= 4
		         ? ", or switch to the Mac II\n(menu, F12, Machine)" : "");
		return FALSE;
	}
	if (rom_patch(rom) != 0) {
		snprintf(why, size, "umac cannot patch this ROM.");
		return FALSE;
	}
	rom_in_use = r;
	if (strcmp(setting("plus_rom"), r->path) != 0)
		bmmac_save_setting("plus_rom", r->path);

	umac_memsize_kb = ram_kb();
	free(ram);
	ram = calloc(1, umac_memsize_kb * 1024);
	if (ram == NULL) {
		snprintf(why, size, "Not enough memory for the Mac's RAM.");
		return FALSE;
	}
	memset(discs, 0, sizeof discs);
	open_disk(&discs[0]);
	umac_init(ram, rom, discs);
	repaint = TRUE;
	printf("BMMacintosh: %s, ROM %s\n", emu_machine_text(), r->path);
	return TRUE;
}

void emu_main(void)
{
	char why[256];
	rom = malloc(PLUS_ROM_SIZE);
	for (;;) {
		double emulated = 0.0, wanted = 0.0;
		int frames = 0;
		restart_requested = FALSE;
		if (rom == NULL || !start_mac(why, sizeof why)) {
			printf("BMMacintosh: %s\n", why);
			bmmac_stopped(why);
			continue;
		}
		bmmac_screen_changed();
		while (!restart_requested) {
			/* A frame's time at 7.8336 MHz, in umac's 5 ms steps. */
			wanted += 1000000.0 / FRAME_HZ;
			while (emulated < wanted) {
				if (umac_loop()) {
					printf("BMMacintosh: umac stopped\n");
					bmmac_stopped("The Mac has stopped (umac could not go on).\n"
					              "Press a key to start it again.");
					restart_requested = TRUE;
					break;
				}
				emulated += 5000.0;
			}
			umac_vsync_event();
			if (++frames % 60 == 0)
				umac_1hz_event();
			feed_key();
			play_sound();
			bmmac_frame();
		}
		emu_release_keys();
		emu_sync_disks();
	}
}

/* ---- The menu ------------------------------------------------------------------------- */

enum { I_DISK = 100, I_EJECT, I_ROM, I_MEMORY, I_RESTART };

static const char *const disk_exts[] = {"dsk", "img", "image", "hfv", "hda", NULL};

static void rom_menu(void)
{
	const struct bmmac_rom *roms[32];
	const char *names[33];
	int n = 0, current = 0, i;
	bmmac_rom_scan();
	for (i = 0; i < bmmac_rom_count() && n < 32; i++) {
		const struct bmmac_rom *r = bmmac_rom_get(i);
		if (r->machine != BMMAC_PLUS)
			continue;
		if (r == rom_in_use || strcmp(setting("plus_rom"), r->path) == 0)
			current = n;
		roms[n] = r;
		names[n++] = r->name;
	}
	names[n] = NULL;
	if (n == 0) {
		osd_message("No Mac Plus ROM in the roms folder (see roms/ROMS.txt).", TRUE);
		return;
	}
	i = osd_choose("ROM", names, current);
	if (i < 0)
		return;
	bmmac_save_setting("plus_rom", roms[i]->path);
	emu_cold_reset();
}

static void memory_menu(void)
{
	static const char *const names[] = {
		"128K (Macintosh 128K)", "512K (Macintosh 512K)", "1 MB", "2 MB", "4 MB", NULL
	};
	static const unsigned sizes[] = {128, 512, 1024, 2048, 4096};
	unsigned kb = ram_kb();
	int current = 4, i;
	char text[8];
	for (i = 0; i < 5; i++)
		if (sizes[i] == kb)
			current = i;
	i = osd_choose("Memory", names, current);
	if (i < 0 || i == current)
		return;
	snprintf(text, sizeof text, "%u", sizes[i]);
	bmmac_save_setting("plus_ram", text);
	emu_cold_reset();
}

int emu_menu_items(struct osd_item *items, int max)
{
	static char ram_text[16];
	const char *path = setting("plus_disk");
	unsigned kb = ram_kb();
	int n = 0;
	unsigned i;
	if (kb >= 1024)
		snprintf(ram_text, sizeof ram_text, "%u MB", kb / 1024);
	else
		snprintf(ram_text, sizeof ram_text, "%uK", kb);
	{
		const struct osd_item mine[] = {
			{"Disk", path[0] ? bmmac_base_name(path) : "(none)", OSD_ITEM_SUBMENU, I_DISK},
			{"No Disk", NULL, 0, I_EJECT},
			{"ROM", rom_in_use ? bmmac_base_name(rom_in_use->path) : "(none)",
			 OSD_ITEM_SUBMENU, I_ROM},
			{"Memory", ram_text, OSD_ITEM_SUBMENU, I_MEMORY},
			{"Restart the Mac", NULL, 0, I_RESTART},
		};
		for (i = 0; i < sizeof mine / sizeof mine[0] && n < max; i++)
			items[n++] = mine[i];
	}
	return n;
}

int emu_menu_choose(int id)
{
	char path[512];
	switch (id) {
	case I_DISK:
		if (!bmmac_browse("Choose the Mac's Disk", "disks", disk_exts, path, sizeof path))
			return FALSE;
		bmmac_save_setting("plus_disk", path);
		emu_cold_reset();
		return TRUE;
	case I_EJECT:
		bmmac_save_setting("plus_disk", "");
		emu_cold_reset();
		return TRUE;
	case I_ROM:
		rom_menu();
		return restart_requested;
	case I_MEMORY:
		memory_menu();
		return restart_requested;
	case I_RESTART:
		emu_cold_reset();
		return TRUE;
	}
	return TRUE;
}
