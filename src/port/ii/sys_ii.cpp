/*
 * sys_ii.cpp - Basilisk II's disk access for BMMacintosh.
 *
 * The Mac's disks are image files on the SD card, read and written through
 * the C library (newlib on the Pi, on Circle's FatFs): hard disks ("disk"
 * in macii.prefs), floppy disks ("floppy") and a CD-ROM ("cdrom", an .iso
 * or .toast image). Raw images, and DiskCopy 4.2 ones (whose header is
 * skipped, FileDiskLayout()).
 *
 * There are always two floppy drives and a CD-ROM drive, which can be
 * empty ("" in macii.prefs): the menu (menu_ii.cpp) puts a disk into one
 * while the Mac runs, and the Mac ejects it. Hard disks are there from
 * the Mac's start.
 *
 * FatFs seeks with 32 bits: images up to 2 GB.
 *
 * From Basilisk II's Unix sys_unix.cpp.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include "sysdeps.h"

#include "main.h"
#include "macos_util.h"
#include "prefs.h"
#include "sys.h"
#include "sony.h"
#include "cdrom.h"

#include "../bmmac.h"
#include "ii.h"

struct bm_file {
	char name[256];
	int fd;               /* -1: no disk in the drive */
	bool read_only;
	bool removable;       /* a floppy or CD-ROM drive */
	bool is_cdrom;
	loff_t start_byte;    /* a header's size (DiskCopy 4.2) */
	loff_t size;
	bool open;            /* not yet closed by Basilisk II */
};

#define MAX_FILES 16
static bm_file files[MAX_FILES];

/* ---- Drives --------------------------------------------------------------------- */

/* Is NAME one of the "floppy" (or "cdrom") items in macii.prefs? */
static bool in_prefs(const char *item, const char *name)
{
	const char *s;
	for (int i = 0; (s = PrefsFindString(item, i)) != NULL; i++) {
		if (*s == '*')
			s++;
		if (strcmp(s, name) == 0)
			return true;
	}
	return false;
}

/* Two floppy drives and a CD-ROM drive, empty or not. */
void ii_prepare_drives(void)
{
	int n = 0;
	while (PrefsFindString("floppy", n) != NULL)
		n++;
	for (; n < 2; n++)
		PrefsAddString("floppy", "");
	if (PrefsFindString("cdrom", 0) == NULL)
		PrefsAddString("cdrom", "");
}

void SysInit(void)
{
}

void SysExit(void)
{
}

void SysAddFloppyPrefs(void)
{
}

void SysAddDiskPrefs(void)
{
}

void SysAddCDROMPrefs(void)
{
}

void SysAddSerialPrefs(void)
{
}

/* ---- Opening ---------------------------------------------------------------------- */

static bool open_image(bm_file *f, const char *name, bool read_only)
{
	uint8 data[256];
	snprintf(f->name, sizeof f->name, "%s", name);
	f->fd = -1;
	f->size = 0;
	f->start_byte = 0;
	if (name[0] == '\0')
		return true;
	f->read_only = read_only || f->is_cdrom;
	f->fd = open(name, f->read_only ? O_RDONLY : O_RDWR);
	if (f->fd < 0 && !f->read_only) {
		f->read_only = true;
		f->fd = open(name, O_RDONLY);
	}
	if (f->fd < 0) {
		printf("BMMacintosh: cannot open the disk image %s\n", name);
		return false;
	}
	loff_t size = lseek(f->fd, 0, SEEK_END);
	memset(data, 0, sizeof data);
	lseek(f->fd, 0, SEEK_SET);
	if (read(f->fd, data, sizeof data) < 0)
		printf("BMMacintosh: cannot read %s\n", name);
	FileDiskLayout(size, data, f->start_byte, f->size);
	printf("BMMacintosh: disk %s, %ld KB%s\n", name, (long) (f->size / 1024),
	       f->read_only ? ", read only" : "");
	return true;
}

void *Sys_open(const char *name, bool read_only, bool is_cdrom)
{
	bm_file *f = NULL;
	for (int i = 0; i < MAX_FILES; i++) {
		if (!files[i].open) {
			f = &files[i];
			break;
		}
	}
	if (f == NULL)
		return NULL;
	memset(f, 0, sizeof *f);
	f->is_cdrom = is_cdrom || in_prefs("cdrom", name);
	f->removable = f->is_cdrom || in_prefs("floppy", name);
	/* An empty name is an empty floppy or CD-ROM drive. */
	if (name[0] == '\0' && !f->removable)
		return NULL;
	if (!open_image(f, name, read_only) && !f->removable)
		return NULL;
	f->open = true;
	return f;
}

void Sys_close(void *arg)
{
	bm_file *f = (bm_file *) arg;
	if (f == NULL)
		return;
	if (f->fd >= 0) {
		fsync(f->fd);
		close(f->fd);
	}
	f->fd = -1;
	f->open = false;
}

size_t Sys_read(void *arg, void *buffer, loff_t offset, size_t length)
{
	bm_file *f = (bm_file *) arg;
	if (f == NULL || f->fd < 0)
		return 0;
	if (lseek(f->fd, offset + f->start_byte, SEEK_SET) < 0)
		return 0;
	bmmac_disk_accessed();
	ssize_t got = read(f->fd, buffer, length);
	return got < 0 ? 0 : (size_t) got;
}

size_t Sys_write(void *arg, void *buffer, loff_t offset, size_t length)
{
	bm_file *f = (bm_file *) arg;
	if (f == NULL || f->fd < 0 || f->read_only)
		return 0;
	if (lseek(f->fd, offset + f->start_byte, SEEK_SET) < 0)
		return 0;
	bmmac_disk_accessed();
	ssize_t put = write(f->fd, buffer, length);
	return put < 0 ? 0 : (size_t) put;
}

loff_t SysGetFileSize(void *arg)
{
	bm_file *f = (bm_file *) arg;
	return f != NULL && f->fd >= 0 ? f->size : 0;
}

/* Replaces NAME with VALUE in the "item" list of macii.prefs. */
static void replace_pref(const char *item, const char *name, const char *value)
{
	const char *s;
	for (int i = 0; (s = PrefsFindString(item, i)) != NULL; i++) {
		if (*s == '*')
			s++;
		if (strcmp(s, name) == 0) {
			PrefsReplaceString(item, value, i);
			SavePrefs();
			return;
		}
	}
}

/* The Mac ejected the disk: the drive is empty, now and at the next
   start. */
void SysEject(void *arg)
{
	bm_file *f = (bm_file *) arg;
	if (f == NULL || f->fd < 0 || !f->removable)
		return;
	printf("BMMacintosh: ejected %s\n", f->name);
	fsync(f->fd);
	close(f->fd);
	f->fd = -1;
	replace_pref(f->is_cdrom ? "cdrom" : "floppy", f->name, "");
	f->name[0] = '\0';
}

bool SysFormat(void *arg)
{
	(void) arg;
	return false;
}

bool SysIsReadOnly(void *arg)
{
	bm_file *f = (bm_file *) arg;
	return f == NULL || f->read_only;
}

bool SysIsFixedDisk(void *arg)
{
	bm_file *f = (bm_file *) arg;
	return f == NULL || !f->removable;
}

bool SysIsDiskInserted(void *arg)
{
	bm_file *f = (bm_file *) arg;
	return f != NULL && f->fd >= 0;
}

void SysPreventRemoval(void *arg)
{
	(void) arg;
}

void SysAllowRemoval(void *arg)
{
	(void) arg;
}

/* ---- Changing disks while the Mac runs (menu_ii.cpp) ---------------------------- */

/* Puts the image PATH into an empty floppy drive (or the CD-ROM drive).
   Returns FALSE if there is none, or the image can't be opened. */
bool ii_insert_disk(const char *path, bool cdrom)
{
	for (int i = 0; i < MAX_FILES; i++) {
		bm_file *f = &files[i];
		if (!f->open || !f->removable || f->is_cdrom != cdrom || f->fd >= 0)
			continue;
		char old_name[256];
		snprintf(old_name, sizeof old_name, "%s", f->name);
		if (!open_image(f, path, false) || f->fd < 0) {
			f->name[0] = '\0';
			return false;
		}
		replace_pref(cdrom ? "cdrom" : "floppy", old_name, path);
		if (cdrom)
			CDROMMountVolume(f);
		else
			SonyMountVolume(f);
		return true;
	}
	return false;
}

/* The disk in floppy drive N (0 or 1), or "". */
const char *ii_floppy_name(int n)
{
	int k = 0;
	for (int i = 0; i < MAX_FILES; i++) {
		bm_file *f = &files[i];
		if (!f->open || !f->removable || f->is_cdrom)
			continue;
		if (k++ == n)
			return f->name;
	}
	return "";
}

void emu_sync_disks(void)
{
	for (int i = 0; i < MAX_FILES; i++)
		if (files[i].open && files[i].fd >= 0 && !files[i].read_only)
			fsync(files[i].fd);
}

/* ---- CD-ROM images -------------------------------------------------------------- */

/* A CD-ROM image is one data track. */
bool SysCDReadTOC(void *arg, uint8 *toc)
{
	bm_file *f = (bm_file *) arg;
	if (f == NULL || f->fd < 0)
		return false;
	uint32 blocks = (uint32) (f->size / 2048) + 150;
	uint8 *p = toc + 2;
	*p++ = 1;               /* first track */
	*p++ = 1;               /* last track */
	*p++ = 0;               /* track 1: data */
	*p++ = 0x14;
	*p++ = 1;
	*p++ = 0;
	*p++ = 0; *p++ = 0; *p++ = 2; *p++ = 0;    /* 00:02:00 */
	*p++ = 0;               /* lead out */
	*p++ = 0x14;
	*p++ = 0xaa;
	*p++ = 0;
	*p++ = 0;
	*p++ = (uint8) (blocks / (60 * 75));
	*p++ = (uint8) (blocks / 75 % 60);
	*p++ = (uint8) (blocks % 75);
	int size = (int) (p - toc);
	toc[0] = (uint8) ((size - 2) >> 8);
	toc[1] = (uint8) (size - 2);
	return true;
}

bool SysCDGetPosition(void *arg, uint8 *pos)
{
	(void) arg;
	(void) pos;
	return false;
}

bool SysCDPlay(void *arg, uint8 start_m, uint8 start_s, uint8 start_f, uint8 end_m,
               uint8 end_s, uint8 end_f)
{
	(void) arg; (void) start_m; (void) start_s; (void) start_f;
	(void) end_m; (void) end_s; (void) end_f;
	return false;
}

bool SysCDPause(void *arg)
{
	(void) arg;
	return false;
}

bool SysCDResume(void *arg)
{
	(void) arg;
	return false;
}

bool SysCDStop(void *arg, uint8 lead_out_m, uint8 lead_out_s, uint8 lead_out_f)
{
	(void) arg; (void) lead_out_m; (void) lead_out_s; (void) lead_out_f;
	return false;
}

bool SysCDScan(void *arg, uint8 start_m, uint8 start_s, uint8 start_f, bool reverse)
{
	(void) arg; (void) start_m; (void) start_s; (void) start_f; (void) reverse;
	return false;
}

void SysCDSetVolume(void *arg, uint8 left, uint8 right)
{
	(void) arg; (void) left; (void) right;
}

void SysCDGetVolume(void *arg, uint8 &left, uint8 &right)
{
	(void) arg;
	left = right = 0;
}
