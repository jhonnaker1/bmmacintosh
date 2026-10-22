/*
 * machine.c - switching between the Mac Plus and the Mac II / Quadra.
 *
 * Each is a kernel of its own (two 68k CPU cores in one kernel would
 * clash): macplus*.img (umac, every Pi) and macii*.img (Basilisk II, the
 * Pi 4 and 5). The Pi's firmware starts the one named by the kernel= line
 * of config.txt's section for the board, so switching, as BMC64 switched
 * between its machines, rewrites those lines and reboots. The Pi Zero to 3
 * have only the Mac Plus, so only the [pi4] and [pi5] sections change.
 *
 * Both kernels share the SD card: the roms and disks folders, the web UI,
 * and bmmac.cfg.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "bmmac.h"

/* The kernels' names: Circle's for each board (Rules.mk's TARGET), with
   "kernel" made the machine's name (the Makefile). Indexed by RASPPI. */
static const char *const plus_kernels[6] = {
	NULL, "macplus.img", "macplus7.img", "macplus8-32.img", "macplus8-rpi4.img",
	"macplus_2712.img"
};
static const char *const ii_kernels[6] = {
	NULL, NULL, NULL, NULL, "macii8-rpi4.img", "macii_2712.img"
};

int bmmac_machine_available(int machine)
{
	return machine == BMMAC_PLUS || (machine == BMMAC_II && BMMAC_RASPPI >= 4);
}

static const char *kernel_for(int machine, int raspi)
{
	if (raspi < 1 || raspi > 5)
		return NULL;
	return machine == BMMAC_II ? ii_kernels[raspi] : plus_kernels[raspi];
}

const char *bmmac_kernel_name(int machine)
{
	const char *name = kernel_for(machine, BMMAC_RASPPI);
	return name != NULL ? name : "";
}

/* "[pi4]" -> 4 for the sections that change, 0 for the others ("[all]",
   "[pi0]"...). */
static int section_board(const char *line)
{
	if (strncasecmp(line, "[pi4]", 5) == 0)
		return 4;
	if (strncasecmp(line, "[pi5]", 5) == 0)
		return 5;
	return 0;
}

int bmmac_switch_machine(int machine)
{
	char path[256], new_path[256], line[512];
	FILE *in, *out;
	int board = 0, changed = 0, ok;

	bmmac_card_path(path, sizeof path, "", "config.txt");
	bmmac_card_path(new_path, sizeof new_path, "", "config.new");
	in = fopen(path, "r");
	if (in == NULL) {
		printf("BMMacintosh: cannot read %s\n", path);
		return FALSE;
	}
	out = fopen(new_path, "w");
	if (out == NULL) {
		fclose(in);
		printf("BMMacintosh: cannot write %s\n", new_path);
		return FALSE;
	}
	while (fgets(line, sizeof line, in) != NULL) {
		const char *p = line + strspn(line, " \t");
		if (*p == '[') {
			board = section_board(p);
		} else if (board != 0 && strncmp(p, "kernel=", 7) == 0) {
			/* The line's end (\n or \r\n) is kept as it was. */
			const char *end = p + strcspn(p, "\r\n");
			fprintf(out, "kernel=%s%s", kernel_for(machine, board), end);
			changed++;
			continue;
		}
		fputs(line, out);
	}
	fclose(in);
	ok = fflush(out) == 0;
	ok = fsync(fileno(out)) == 0 && ok;
	ok = fclose(out) == 0 && ok;
	if (!ok || changed == 0) {
		printf("BMMacintosh: %s has no [pi4] or [pi5] kernel= line, or cannot be written\n",
		       path);
		unlink(new_path);
		return FALSE;
	}
	/* FatFs can't rename over a file. */
	if (unlink(path) != 0 || rename(new_path, path) != 0) {
		printf("BMMacintosh: cannot replace %s\n", path);
		return FALSE;
	}
	printf("BMMacintosh: config.txt now starts %s\n",
	       machine == BMMAC_II ? "the Mac II (Basilisk II)" : "the Mac Plus (umac)");
	return TRUE;
}
