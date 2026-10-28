/*
 * rtc_plus.c - the Mac Plus's clock chip, for umac.
 *
 * The Mac 128K, 512K and Plus keep the date and time, and 20 bytes of
 * parameter RAM (the Control Panel's settings), in a clock chip on the
 * VIA's port B: bit 0 the data line, bit 1 its clock, bit 2 the chip's
 * enable (low while the Mac talks to it). The Mac clocks a command byte
 * out to it, a bit on each rising edge of the clock, the first bit the
 * highest; then, for a write, a data byte, or, for a read, clocks a byte
 * back in, again a bit on each rising edge (the data line being an input
 * then). umac leaves those lines alone; main_plus.c's plus_via_init()
 * hands them to plus_rtc_portb() and reads the data line from
 * plus_rtc_data().
 *
 * A command byte is z a4 a3 a2 a1 a0 0 1: z is set for a read, and the
 * address a4..a0 is
 *
 *   00xab   the seconds counter's byte ab (00 the lowest): the Mac reads
 *           it at 1ab and at 0ab, and takes it when the two agree
 *   01100   the test register (written only)
 *   01101   the write protect register (written only; bit 7 set: no
 *           writes)
 *   010ab   parameter RAM 0x10 + ab
 *   1abcd   parameter RAM 0x0 + abcd
 *
 * The seconds count from 1904, the Mac's epoch: the Pi's clock (set from
 * the network) in the menu's time zone, as for the Mac II, plus whatever
 * the Mac set it to since. The parameter RAM, and that difference, are kept
 * on the card in macplus.pram.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../host.h"
#include "../bmmac.h"
#include "plus.h"

#define RTC_DATA   0x01
#define RTC_CLOCK  0x02
#define RTC_ENABLE 0x04   /* low: enabled */

/* Seconds from 1904 (the Mac's epoch) to 1970 (the C library's). */
#define MAC_EPOCH_DIFF 2082844800u

#define PRAM_SIZE 20

static uint8_t pram[PRAM_SIZE];
static int32_t seconds_offset;     /* what the Mac set, minus the Pi's time */
static int write_protected;
static int changed;

/* The transfer under way. */
enum { RTC_IDLE, RTC_COMMAND, RTC_WRITE, RTC_READ };
static int state = RTC_IDLE;
static int bits;
static uint8_t shift;
static uint8_t command;
static int data_out = 1;
static uint8_t last_portb = RTC_ENABLE;

static uint32_t pi_seconds(void)
{
	return (uint32_t) ((int64_t) time(NULL) + host_option_int("timezone", 0) * 60
	                   + MAC_EPOCH_DIFF);
}

static uint32_t mac_seconds(void)
{
	return pi_seconds() + (uint32_t) seconds_offset;
}

/* The register a command names: a seconds byte (0-3), the parameter RAM
   (0x100 + its address), the test or write protect register, or -1. */
#define REG_TEST 0x200
#define REG_PROTECT 0x201

static int command_register(uint8_t cmd)
{
	unsigned a = (cmd >> 2) & 0x1f;
	if ((cmd & 3) != 1)
		return -1;
	if (a < 8)
		return (int) (a & 3);
	if (a == 0x0c)
		return REG_TEST;
	if (a == 0x0d)
		return REG_PROTECT;
	if (a >= 0x08 && a < 0x0c)
		return 0x100 + 0x10 + (int) (a & 3);
	if (a >= 0x10)
		return 0x100 + (int) (a & 0x0f);
	return -1;
}

static uint8_t read_register(int reg)
{
	if (reg >= 0 && reg < 4)
		return (uint8_t) (mac_seconds() >> (8 * reg));
	if (reg >= 0x100 && reg < 0x100 + PRAM_SIZE)
		return pram[reg - 0x100];
	return 0;
}

static void write_register(int reg, uint8_t value)
{
	if (reg == REG_PROTECT) {
		write_protected = (value & 0x80) != 0;
		return;
	}
	if (write_protected || reg == REG_TEST)
		return;
	if (reg >= 0 && reg < 4) {
		uint32_t now = mac_seconds();
		now &= ~(0xffu << (8 * reg));
		now |= (uint32_t) value << (8 * reg);
		seconds_offset = (int32_t) (now - pi_seconds());
		changed = TRUE;
	} else if (reg >= 0x100 && reg < 0x100 + PRAM_SIZE) {
		if (pram[reg - 0x100] != value)
			changed = TRUE;
		pram[reg - 0x100] = value;
	}
}

/* The Mac wrote port B. */
void plus_rtc_portb(uint8_t portb)
{
	int rising = !(last_portb & RTC_CLOCK) && (portb & RTC_CLOCK);
	last_portb = portb;
	if (portb & RTC_ENABLE) {
		/* Not selected: a transfer starts again next time. */
		state = RTC_IDLE;
		data_out = 1;
		return;
	}
	if (state == RTC_IDLE) {
		state = RTC_COMMAND;
		bits = 0;
		shift = 0;
	}
	if (!rising)
		return;
	switch (state) {
	case RTC_COMMAND:
	case RTC_WRITE:
		shift = (uint8_t) ((shift << 1) | (portb & RTC_DATA));
		if (++bits < 8)
			return;
		bits = 0;
		if (state == RTC_WRITE) {
			write_register(command_register(command), shift);
			state = RTC_IDLE;
			return;
		}
		command = shift;
		if (command & 0x80) {
			state = RTC_READ;
			shift = read_register(command_register(command));
		} else {
			state = RTC_WRITE;
			shift = 0;
		}
		return;
	case RTC_READ:
		/* A bit for each rising edge, the highest first: the Mac reads
		   it after raising the clock. */
		data_out = (shift >> 7) & 1;
		shift = (uint8_t) (shift << 1);
		if (++bits == 8)
			state = RTC_IDLE;
		return;
	}
}

/* The data line, as the Mac reads it. */
int plus_rtc_data(void)
{
	return data_out;
}

/* ---- macplus.pram: the parameter RAM, and the clock's correction ---------------- */

static void pram_path(char *path, unsigned size)
{
	bmmac_card_path(path, size, "", "macplus.pram");
}

void plus_rtc_load(void)
{
	char path[256];
	uint8_t buf[PRAM_SIZE + 4];
	FILE *f;
	memset(pram, 0, sizeof pram);
	seconds_offset = 0;
	write_protected = FALSE;
	state = RTC_IDLE;
	last_portb = RTC_ENABLE;
	data_out = 1;
	changed = FALSE;
	pram_path(path, sizeof path);
	f = fopen(path, "rb");
	if (f == NULL)
		return;
	if (fread(buf, 1, sizeof buf, f) == sizeof buf) {
		memcpy(pram, buf, PRAM_SIZE);
		seconds_offset = (int32_t) ((uint32_t) buf[PRAM_SIZE] << 24
		                            | (uint32_t) buf[PRAM_SIZE + 1] << 16
		                            | (uint32_t) buf[PRAM_SIZE + 2] << 8
		                            | buf[PRAM_SIZE + 3]);
	}
	fclose(f);
}

void plus_rtc_save(void)
{
	char path[256];
	uint8_t buf[PRAM_SIZE + 4];
	uint32_t offset = (uint32_t) seconds_offset;
	FILE *f;
	if (!changed)
		return;
	changed = FALSE;
	memcpy(buf, pram, PRAM_SIZE);
	buf[PRAM_SIZE] = (uint8_t) (offset >> 24);
	buf[PRAM_SIZE + 1] = (uint8_t) (offset >> 16);
	buf[PRAM_SIZE + 2] = (uint8_t) (offset >> 8);
	buf[PRAM_SIZE + 3] = (uint8_t) offset;
	pram_path(path, sizeof path);
	f = fopen(path, "wb");
	if (f == NULL) {
		printf("BMMacintosh: cannot write %s\n", path);
		return;
	}
	fwrite(buf, 1, sizeof buf, f);
	fclose(f);
}
