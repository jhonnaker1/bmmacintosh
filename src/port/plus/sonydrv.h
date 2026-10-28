/*
 * sonydrv.h - the Mac Plus's disk driver, BMMacintosh's.
 *
 * umac replaces the Mac Plus ROM's floppy driver (.Sony) with one that
 * hands each call to umac's host side (disc.c's disc_pv_hook()), by writing
 * the call's number (0 Open, 1 Prime, 2 Control, 3 Status) to the address
 * PV_SONY_ADDR (0xC00069); the host reads A0 (the parameter block), A1 (the
 * DCE) and, for Open, A2 (the drive status block the driver allocated).
 * umac's rom.c copies this file's bytes into the ROM.
 *
 * umac's own driver (its macsrc/sonydrv.S, include/sonydrv.h) marks its
 * changes "GPLv2", and BMMacintosh is under the GPL version 3. This is
 * BMMacintosh's: Basilisk II's .Sony replacement (rom_patches.cpp's
 * sony_driver, GPL version 2 or later), with the calls written to the
 * address directly (no register used) and its own Open, which allocates
 * the drive status block and adds the drive. It is found ahead of umac's
 * (src/port/plus/umac.mk puts this folder first on the include path).
 *
 * rom.c writes PV_SONY_ADDR into the last four bytes, which this driver
 * doesn't use.
 *
 * Hand assembled (Musashi's disassembler checked it):
 *
 *   00  dc.w $6f00,0,0,0          flags (Basilisk II's), delay, mask, menu
 *   08  dc.w Open,Prime,Control,Status,Close
 *   12  dc.b 5,'.Sony'
 * Open:
 *   18  move.l  a2,-(sp)
 *   1a  movem.l a0-a1,-(sp)
 *   1e  moveq   #30,d0            the drive status block's size
 *   20  _NewPtrSysClear
 *   22  movea.l a0,a2
 *   24  movem.l (sp)+,a0-a1
 *   28  move.l  a2,d1
 *   2a  beq.s   1$
 *   2c  move.b  #0,$c00069        Open: umac fills in the status block
 *   34  move.l  #$0001fffb,d0     drive 1, the driver's reference number -5
 *   3a  lea     6(a2),a0          the drive queue element
 *   3e  _AddDrive
 *   40  moveq   #0,d0
 *   42  movea.l (sp)+,a2
 *   44  rts
 *   46  1$ moveq #-23,d0          openErr
 *   48  movea.l (sp)+,a2
 *   4a  rts
 * Prime:
 *   4c  move.b  #1,$c00069
 *   54  bra.s   IOReturn
 * Control:
 *   56  move.b  #2,$c00069
 *   5e  cmpi.w  #1,$1a(a0)        KillIO
 *   64  bne.s   IOReturn
 *   66  rts
 * Status:
 *   68  move.b  #3,$c00069
 * IOReturn (Basilisk II's):
 *   70  move.w  6(a0),d1          immediate call?
 *   74  btst    #9,d1
 *   78  beq.s   1$
 *   7a  tst.w   d0
 *   7c  ble.s   2$
 *   7e  clr.w   d0
 *   80  2$ move.w d0,$10(a0)
 *   84  rts
 *   86  1$ tst.w d0
 *   88  ble.s   3$
 *   8a  clr.w   d0
 *   8c  rts
 *   8e  3$ move.l $8fc,-(sp)      jIODone
 *   92  rts
 * Close:
 *   94  moveq   #-24,d0           closErr
 *   96  rts
 *   98  dc.l 0                    (rom.c's PV_SONY_ADDR)
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */
	/* Driver header */
	0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x00, 0x4c, 0x00, 0x56, 0x00, 0x68, 0x00, 0x94,
	0x05, 0x2e, 0x53, 0x6f, 0x6e, 0x79,
	/* 0x18 Open */
	0x2f, 0x0a,
	0x48, 0xe7, 0x00, 0xc0,
	0x70, 0x1e,
	0xa7, 0x1e,
	0x24, 0x48,
	0x4c, 0xdf, 0x03, 0x00,
	0x22, 0x0a,
	0x67, 0x1a,
	0x13, 0xfc, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x69,
	0x20, 0x3c, 0x00, 0x01, 0xff, 0xfb,
	0x41, 0xea, 0x00, 0x06,
	0xa0, 0x4e,
	0x70, 0x00,
	0x24, 0x5f,
	0x4e, 0x75,
	0x70, 0xe9,
	0x24, 0x5f,
	0x4e, 0x75,
	/* 0x4c Prime */
	0x13, 0xfc, 0x00, 0x01, 0x00, 0xc0, 0x00, 0x69,
	0x60, 0x1a,
	/* 0x56 Control */
	0x13, 0xfc, 0x00, 0x02, 0x00, 0xc0, 0x00, 0x69,
	0x0c, 0x68, 0x00, 0x01, 0x00, 0x1a,
	0x66, 0x0a,
	0x4e, 0x75,
	/* 0x68 Status */
	0x13, 0xfc, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x69,
	/* 0x70 IOReturn */
	0x32, 0x28, 0x00, 0x06,
	0x08, 0x01, 0x00, 0x09,
	0x67, 0x0c,
	0x4a, 0x40,
	0x6f, 0x02,
	0x42, 0x40,
	0x31, 0x40, 0x00, 0x10,
	0x4e, 0x75,
	0x4a, 0x40,
	0x6f, 0x04,
	0x42, 0x40,
	0x4e, 0x75,
	0x2f, 0x38, 0x08, 0xfc,
	0x4e, 0x75,
	/* 0x94 Close */
	0x70, 0xe8,
	0x4e, 0x75,
	/* 0x98 */
	0x00, 0x00, 0x00, 0x00,
