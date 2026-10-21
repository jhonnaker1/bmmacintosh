/*
 * statusbar.c - Hatari's statusbar functions for BMAtariST: the disk
 * activity line.
 *
 * Replaces Hatari's src/sdl/statusbar.c, which draws a bar below the
 * picture. BMAtariST has none (its height is 0); the floppy drives' and
 * the hard disk's lights, which Hatari sets here, are shown instead at the
 * bottom left of the picture, as in BMCoco and BMApple2: while a floppy's
 * motor turns, its drive and what the floppy controller is doing (the
 * command, head position, track, sector and side, from fdc.c), by a red
 * light lit while it is busy; "no disk" in red if the drive is empty, so a
 * program stuck loading can be seen to be. Hard disk access (ACSI, SCSI,
 * IDE, or a GEMDOS drive) shows "HD" for a moment. The disk_activity option
 * (Video, Show Disk Activity) turns it off.
 *
 * Hatari's messages (Statusbar_AddMessage()) go to the log and, the
 * important ones, to BMAtariST's notification line.
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

const char Statusbar_fileid[] = "BMAtariST statusbar.c";

#include "main.h"
#include "configuration.h"
#include "fdc.h"
#include "floppy.h"
#include "statusbar.h"

#include "../host.h"
#include "bmatarist.h"
#include "osd.h"

static drive_led_t floppy_led[2];
static double hd_until;           /* the hard disk light's on until then */
static double shown_until;
static double lit_until;
static char text[64];
static int error;

int Statusbar_GetHeightForSize(int width, int height)
{
	return 0;
}

int Statusbar_SetHeight(int width, int height)
{
	return 0;
}

int Statusbar_GetHeight(void)
{
	return 0;
}

void Statusbar_EnableHDLed(drive_led_t state)
{
	if (state != LED_STATE_OFF)
		hd_until = host_time() + 0.25;
}

void Statusbar_SetFloppyLed(drive_index_t drive, drive_led_t state)
{
	if (drive == DRIVE_LED_A || drive == DRIVE_LED_B)
		floppy_led[drive] = state;
}

void Statusbar_InitialSetup(void)
{
}

void Statusbar_AddMessage(const char *msg, uint32_t msecs)
{
	if (msg != NULL && *msg != '\0')
		printf("Hatari: %s\n", msg);
}

void Statusbar_UpdateInfo(void)
{
}

/* ---- The disk activity line ----------------------------------------------- */

void bmatarist_draw_disk_activity(uint16_t *fb, int pitch_px, int w, int h)
{
	double now;
	int drive;
	if (!bmatarist_disk_activity())
		return;
	now = host_time();
	for (drive = 0; drive < 2; drive++) {
		char fdc[48];
		if (floppy_led[drive] == LED_STATE_OFF)
			continue;
		FDC_Get_Statusbar_Text(fdc, sizeof fdc);
		error = !EmulationDrives[drive].bDiskInserted;
		snprintf(text, sizeof text, "%c: %s%s", 'A' + drive, fdc,
		         error ? " no disk" : "");
		if (floppy_led[drive] == LED_STATE_ON_BUSY)
			lit_until = now + 0.1;
		shown_until = now + 2.0;
		break;
	}
	if (drive == 2 && now < hd_until) {
		snprintf(text, sizeof text, "HD");
		error = FALSE;
		lit_until = hd_until;
		shown_until = now + 1.0;
	}
	if (now > shown_until)
		return;
	osd_draw_disk_status(fb, pitch_px, w, h, text, now <= lit_until, error);
	bmatarist_screen_dirty_bottom();
}
