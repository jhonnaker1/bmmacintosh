/*
 * prefs_ppc.cpp - SheepShaver's settings, NVRAM and messages for
 * BMMacintosh.
 *
 * The settings are SheepShaver's own, in its own format, in macppc.prefs in
 * the card's root (which the web UI can edit), and the Mac's NVRAM in
 * macppc.nvram. A card without them starts with 256 MB, a 1024x768 screen
 * in millions of colours, and the card's "shared" folder as a disk on the
 * Mac's desktop (ExtFS).
 *
 * From SheepShaver's Unix prefs_unix.cpp, xpram_unix.cpp and
 * user_strings_unix.cpp, and the Mac II's prefs_ii.cpp.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include "sysdeps.h"

#include "prefs.h"
#include "xpram.h"
#include "user_strings.h"

#include "../bmmac.h"

/* ---- Settings --------------------------------------------------------------------- */

prefs_desc platform_prefs_items[] = {
	{"idlewait", TYPE_BOOLEAN, false, "sleep when idle"},
	{"nonet", TYPE_BOOLEAN, false, "no network for the Mac"},
	{"cpuclock", TYPE_INT32, false, "the PowerPC's clock speed reported to Mac OS (MHz)"},
	{"displaycolordepth", TYPE_INT32, false, "the screen's bits a pixel (video_ppc.cpp)"},
	{NULL, TYPE_END, false, NULL}
};

static char prefs_path[256];

/* SheepShaver's prefs.cpp calls these first and last (the Unix port's
   finds its settings file); BMMacintosh's is always macppc.prefs. */
void prefs_init(void)
{
}

void prefs_exit(void)
{
}

void AddPlatformPrefsDefaults(void)
{
	char shared[256];
	bmmac_card_path(shared, sizeof shared, "shared", "");
	/* The trailing slash off. */
	if (strlen(shared) > 1 && shared[strlen(shared) - 1] == '/')
		shared[strlen(shared) - 1] = '\0';
	PrefsReplaceString("extfs", shared);
	PrefsReplaceInt32("ramsize", 256 * 1024 * 1024);
	PrefsReplaceString("screen", "win/1024/768");
	PrefsReplaceInt32("displaycolordepth", 32);
	PrefsReplaceInt32("frameskip", 0);
	PrefsReplaceBool("idlewait", false);
	PrefsReplaceBool("nonet", false);
	PrefsReplaceBool("ignoresegv", true);
	PrefsReplaceBool("jit", false);
}

void LoadPrefs(const char *vmdir)
{
	FILE *f;
	(void) vmdir;
	bmmac_card_path(prefs_path, sizeof prefs_path, "", "macppc.prefs");
	f = fopen(prefs_path, "r");
	if (f != NULL) {
		LoadPrefsFromStream(f);
		fclose(f);
	} else {
		SavePrefs();
	}
}

void SavePrefs(void)
{
	FILE *f = fopen(prefs_path, "w");
	if (f == NULL) {
		printf("BMMacintosh: cannot write %s\n", prefs_path);
		return;
	}
	SavePrefsToStream(f);
	fclose(f);
}

/* ---- NVRAM -------------------------------------------------------------------------- */

static void nvram_path(char *path, size_t size)
{
	bmmac_card_path(path, (unsigned) size, "", "macppc.nvram");
}

void LoadXPRAM(const char *vmdir)
{
	char path[256];
	FILE *f;
	(void) vmdir;
	nvram_path(path, sizeof path);
	f = fopen(path, "rb");
	if (f == NULL)
		return;
	if (fread(XPRAM, 1, XPRAM_SIZE, f) != XPRAM_SIZE)
		printf("BMMacintosh: %s is short\n", path);
	fclose(f);
}

void SaveXPRAM(void)
{
	char path[256];
	FILE *f;
	nvram_path(path, sizeof path);
	f = fopen(path, "wb");
	if (f == NULL)
		return;
	fwrite(XPRAM, 1, XPRAM_SIZE, f);
	fclose(f);
}

void ZapPRAM(void)
{
	char path[256];
	nvram_path(path, sizeof path);
	remove(path);
}

/* ---- Messages ------------------------------------------------------------------------ */

user_string_def platform_strings[] = {
	/* The ExtFS disk's name on the Mac's desktop. */
	{STR_EXTFS_VOLUME_NAME, "SD Card"},
	{-1, NULL}
};

const char *GetString(int num)
{
	int i;
	for (i = 0; platform_strings[i].num >= 0; i++)
		if (platform_strings[i].num == num)
			return platform_strings[i].str;
	for (i = 0; common_strings[i].num >= 0; i++)
		if (common_strings[i].num == num)
			return common_strings[i].str;
	return NULL;
}
