/*
 * hatari_prefix.h - included before every Hatari source file BMAtariST
 * builds (-include, see the Makefile).
 *
 * A few of Hatari's headers count on <stdint.h>, <stdbool.h> or <stdio.h>
 * having been included before them (debug/symbols.h, for one), which other
 * builds happen to do; they are included here first, so every file builds
 * on its own. And on the Pi, scandir() and alphasort(), which newlib's
 * <dirent.h> doesn't declare (they are posix_extra.c's).
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMATARIST_HATARI_PREFIX_H_
#define BMATARIST_HATARI_PREFIX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef BMATARIST_HOST_LIBC
struct dirent;
int scandir(const char *dirname, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compare)(const struct dirent **, const struct dirent **));
int alphasort(const struct dirent **a, const struct dirent **b);
#endif

#endif /* BMATARIST_HATARI_PREFIX_H_ */
