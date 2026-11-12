/*
 * limit_log.h - included first in kpx_cpu's ppc-execute.cpp (the Makefiles):
 * its message for each illegal instruction is printed the first 16 times
 * only. A crashed Mac otherwise runs through zeros printing one for each
 * word, which fills the log (and the SD card) in minutes.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_PPC_LIMIT_LOG_H_
#define BMMAC_PPC_LIMIT_LOG_H_

#include <stdarg.h>
#include <stdio.h>

static inline int bmmac_limited_fprintf(FILE *f, const char *format, ...)
{
	static int printed;
	va_list ap;
	int n;
	if (printed >= 16) {
		if (printed++ == 16)
			fputs("SheepShaver: more illegal instructions, not logged\n", f);
		return 0;
	}
	printed++;
	va_start(ap, format);
	n = vfprintf(f, format, ap);
	va_end(ap);
	return n;
}

#define fprintf bmmac_limited_fprintf

#endif
