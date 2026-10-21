/*
 * posix_extra.c - POSIX functions Hatari uses that circle-newlib lacks.
 *
 * scandir(), alphasort(): Hatari's GEMDOS drive (a folder on the SD card
 * as the ST's hard disk, gemdos.c) lists folders with them. Hatari's own
 * versions (scandir.c) need dirent's d_reclen, which newlib's doesn't have.
 *
 * utime(): the GEMDOS drive sets a file's date when the ST does (Fdatime).
 * FatFs could, but Circle's file system calls don't offer it: the date is
 * left as it is, which only changes what the ST's desktop shows.
 *
 * chmod(): the GEMDOS drive makes a file read only when the ST asks
 * (Fattrib). newlib declares it but Circle doesn't have it; it fails, and
 * Hatari tells the ST so.
 *
 * Only the Pi needs these: the PC test harness (tools/hostsim) has its C
 * library's. On a Mac, the harness needs Paths_GetMacScreenShotDir()
 * instead, which Hatari's Mac front end has (gui-osx/paths.m).
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMATARIST_HOST_LIBC

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>

int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcmp((*a)->d_name, (*b)->d_name);
}

int scandir(const char *dirname, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compare)(const struct dirent **, const struct dirent **))
{
	DIR *dir;
	struct dirent *entry, **list = NULL;
	size_t count = 0, allocated = 0;

	*namelist = NULL;
	dir = opendir(dirname);
	if (dir == NULL)
		return -1;
	while ((entry = readdir(dir)) != NULL) {
		struct dirent *copy;
		if (filter != NULL && !filter(entry))
			continue;
		if (count == allocated) {
			size_t n = allocated ? allocated * 2 : 32;
			struct dirent **bigger = realloc(list, n * sizeof(*list));
			if (bigger == NULL)
				goto fail;
			list = bigger;
			allocated = n;
		}
		copy = malloc(sizeof(*copy));
		if (copy == NULL)
			goto fail;
		memcpy(copy, entry, sizeof(*copy));
		list[count++] = copy;
	}
	closedir(dir);
	if (compare != NULL && count > 1)
		qsort(list, count, sizeof(*list),
		      (int (*)(const void *, const void *)) compare);
	*namelist = list;
	return (int) count;

fail:
	closedir(dir);
	while (count > 0)
		free(list[--count]);
	free(list);
	errno = ENOMEM;
	return -1;
}

int utime(const char *path, const struct utimbuf *times)
{
	(void) path;
	(void) times;
	errno = ENOSYS;
	return -1;
}

int chmod(const char *path, mode_t mode)
{
	(void) path;
	(void) mode;
	errno = ENOSYS;
	return -1;
}

#elif defined(__APPLE__)

/* Screenshots go to the folder Hatari runs in (paths.c, when this is NULL). */
char *Paths_GetMacScreenShotDir(void);
char *Paths_GetMacScreenShotDir(void)
{
	return NULL;
}

#endif /* BMATARIST_HOST_LIBC */
