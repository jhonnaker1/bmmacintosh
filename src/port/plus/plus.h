/*
 * plus.h - what the parts of BMMacintosh's umac front end share.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_PLUS_H_
#define BMMAC_PLUS_H_

#include <stdint.h>

/* rtc_plus.c: the clock chip. Port B as the Mac writes it, and the data
   line as it reads it (bit 0); the parameter RAM and the clock's
   correction from and to the card (macplus.pram; saved only if they
   changed). */
void plus_rtc_portb(uint8_t portb);
int plus_rtc_data(void);
void plus_rtc_load(void);
void plus_rtc_save(void);

#endif /* BMMAC_PLUS_H_ */
