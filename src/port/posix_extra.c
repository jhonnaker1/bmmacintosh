/*
 * posix_extra.c - POSIX functions Basilisk II's ExtFS uses that
 * circle-newlib lacks.
 *
 * ExtFS (src/port/ii/extfs_ii.cpp) shows a folder on the SD card as a disk
 * on the Mac's desktop:
 *
 * rmdir(): it removes folders. FatFs's f_unlink(), which unlink() is,
 * removes empty folders too.
 * utime(): it sets a file's dates when the Mac does. FatFs can only set
 * the modification date, and the file's dates are left as they are.
 * access(): it checks a file is there.
 * creat(): it makes new files.
 *
 * The PC simulator (tools/hostsim) has the C library's own instead.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

int rmdir(const char *path)
{
	return unlink(path);
}

int utime(const char *path, const struct utimbuf *times)
{
	(void) path;
	(void) times;
	return 0;
}

int access(const char *path, int mode)
{
	struct stat st;
	(void) mode;
	return stat(path, &st);
}

int creat(const char *path, mode_t mode)
{
	return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}
