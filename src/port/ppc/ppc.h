/*
 * ppc.h - what the parts of BMMacintosh's SheepShaver front end share.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_PPC_H_
#define BMMAC_PPC_H_

/* The Mac II's front end, parts of which SheepShaver shares (the disks,
   the clocks, the sound, the menu): ii_mac_time_usec() is main_ppc.cpp's. */
#include "../ii/ii.h"

/* The most RAM a Power Mac gets (Mac OS 9 takes up to 1.5 GB; SheepShaver
   up to 1 GB). */
#define PPC_MAX_RAM (1024u * 1024 * 1024)

/* main_ppc.cpp: the frame buffer, in the Mac's memory after SheepShaver's
   data: the biggest screen, 1600x1200 in millions of colours. */
#define PPC_FRAME_BUFFER_SIZE (1600 * 1200 * 4)
extern uint32 ppc_frame_buffer_mac;
extern uint8 *ppc_frame_buffer;

/* The Mac II's sound (audio_ii.cpp: ii_audio_*()) and keyboard
   (menu_ii.cpp: ii_adb_frame()) are SheepShaver's too: ii.h. */

#endif /* BMMAC_PPC_H_ */
