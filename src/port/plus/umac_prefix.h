/*
 * umac_prefix.h - included before every umac and Musashi source file
 * (src/port/plus/umac.mk).
 *
 * umac's RAM size is fixed when it is built (UMAC_MEMSIZE, in KB); here it
 * is the menu's choice, a variable (main_plus.c), so machw.h's RAM_SIZE
 * follows it. The screen is the Mac Plus's own.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_UMAC_PREFIX_H_
#define BMMAC_UMAC_PREFIX_H_

extern unsigned umac_memsize_kb;
#define UMAC_MEMSIZE umac_memsize_kb
#define DISP_WIDTH 512
#define DISP_HEIGHT 342

#endif /* BMMAC_UMAC_PREFIX_H_ */
