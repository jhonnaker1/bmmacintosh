/*
 * roms.c - the Mac ROMs on the SD card.
 *
 * Apple's ROMs aren't free, so the user puts their own on the card (see
 * docs/ROMS.txt), in the roms folder: a ROM file as it is (a 128K Mac Plus
 * ROM, a 512K or 1 MB Mac II or Quadra one), or MAME's zipped sets as they
 * are (macplus.zip, macqd800.zip, maciici.zip...). A .rom file in the
 * card's root is found too. Some of MAME's sets have the ROM in
 * chips, each with every second (or fourth) byte: the Mac Plus's in two,
 * the IIci's in four. They are put back together here.
 *
 * A Mac ROM's first four bytes are a checksum of the rest (the sum of its
 * 16-bit words), which tells which ROM it is, and that it is whole. umac
 * runs the Mac Plus's last ROM (4D1F8172); Basilisk II the "32-bit clean"
 * ones, of the IIci and later (ROM version 0x067C), best a Quadra 650's or
 * 800's (F1ACAD13).
 *
 * SheepShaver (the Power Macintosh) runs two kinds of PowerPC ROM, as
 * files (not in zips): a "Mac OS ROM" file, as the New World Macs (the
 * iMac and later) load from the System Folder, which starts with
 * "<CHRP-BOOT>" and is unpacked when the Mac starts (rom_patches.cpp's
 * DecodeROM()); or an Old World PCI Power Mac's 4 MB ROM, which names its
 * board family ("Boot TNT", "Boot Gossamer"...) at 0x30D064.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "zlib.h"

#include "bmmac.h"

#define MAX_ROMS 96
#define MAX_ROM_SIZE (1024 * 1024)
/* The PowerPC ROMs: a Mac OS ROM file is under 4 MB, an Old World one
   4 MB. */
#define PPC_ROM_SIZE (4 * 1024 * 1024)

static struct bmmac_rom roms[MAX_ROMS];
static int rom_count;

/* ---- Knowing a ROM ---------------------------------------------------------- */

static const struct {
	uint32_t checksum;
	const char *name;
} known[] = {
	{0x28ba61ce, "Macintosh 128K"},
	{0x28ba4e50, "Macintosh 512K"},
	{0x4d1eeee1, "Macintosh Plus, first ROM"},
	{0x4d1eeae1, "Macintosh Plus, second ROM"},
	{0x4d1f8172, "Macintosh Plus"},
	{0xb2e362a8, "Macintosh SE"},
	{0xb306e171, "Macintosh SE FDHD"},
	{0xa49f9914, "Macintosh Classic"},
	{0x97851db6, "Macintosh II"},
	{0x9779d2c4, "Macintosh II FDHD"},
	{0x97221136, "Macintosh IIx, IIcx, SE/30"},
	{0x368cadfe, "Macintosh IIci"},
	{0x36b7fb6c, "Macintosh IIsi"},
	{0x4147dd77, "Macintosh IIfx"},
	{0x350eacf0, "Macintosh LC"},
	{0x35c28f5f, "Macintosh LC II"},
	{0x3193670e, "Macintosh Classic II"},
	{0xecbbc41c, "Macintosh LC III"},
	{0xecd99dc0, "Macintosh Color Classic"},
	{0xede66cbd, "Macintosh LC 550"},
	{0x420dbff3, "Quadra 700 and 900"},
	{0x3dc27823, "Quadra 950"},
	{0xf1a6f343, "Centris 610 and 650"},
	{0xf1acad13, "Quadra 610, 650 and 800"},
	{0xff7439ee, "Quadra 605, LC 475 and 575"},
	{0x064dc91d, "Performa 580"},
	{0x5bf10fd1, "Quadra 660AV and 840AV"},
};

static uint32_t be32(const uint8_t *p)
{
	return (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 | (uint32_t) p[2] << 8 | p[3];
}

/* Is BUF a whole Mac ROM: its checksum right? */
static int checksum_ok(const uint8_t *buf, uint32_t size)
{
	uint32_t sum = 0, i;
	if (size < 0x10000 || (size & (size - 1)) != 0)
		return FALSE;
	for (i = 4; i < size; i += 2)
		sum += (uint32_t) buf[i] << 8 | buf[i + 1];
	return sum == be32(buf);
}

/* Which machine runs the ROM in BUF: umac only the last Mac Plus ROM,
   Basilisk II the 32-bit clean ones (but not the AV Quadras', whose
   hardware it doesn't have). */
static int machine_for(const uint8_t *buf)
{
	uint32_t checksum = be32(buf);
	unsigned version = (unsigned) buf[8] << 8 | buf[9];
	if (checksum == 0x4d1f8172)
		return BMMAC_PLUS;
	if (version == 0x067c && checksum != 0x5bf10fd1)
		return BMMAC_II;
	return -1;
}

const char *bmmac_rom_describe(uint32_t checksum, int *machine)
{
	unsigned i;
	(void) machine;
	for (i = 0; i < sizeof known / sizeof known[0]; i++)
		if (known[i].checksum == checksum)
			return known[i].name;
	return NULL;
}

static void add_rom(const char *path, const uint8_t *buf, uint32_t size)
{
	struct bmmac_rom *r;
	const char *name;
	if (rom_count >= MAX_ROMS)
		return;
	r = &roms[rom_count++];
	snprintf(r->path, sizeof r->path, "%s", path);
	r->checksum = be32(buf);
	r->preferred = 0;
	r->size = size;
	r->machine = machine_for(buf);
	name = bmmac_rom_describe(r->checksum, NULL);
	if (name != NULL)
		snprintf(r->name, sizeof r->name, "%s (%08X)", name, (unsigned) r->checksum);
	else
		snprintf(r->name, sizeof r->name, "ROM %08X", (unsigned) r->checksum);
	printf("BMMacintosh: ROM %s: %s, %u KB%s\n", path, r->name, (unsigned) (size / 1024),
	       r->machine == BMMAC_PLUS ? ", Mac Plus" : r->machine == BMMAC_II ? ", Mac II" : "");
}

/* The Mac OS ROM files tried with SheepShaver (their CRC-32s): the ones
   that start Mac OS, newest first (the default is the first found), and
   the ones that don't. Those packed in "parcels" (2.5.1 and later, for Mac
   OS 9.0.4 to 9.2.2's Macs) never do: rom_patches.cpp can unpack them but
   not patch them. */
static const struct {
	uint32_t crc;
	const char *name;
} nw_good[] = {
	{0x2f6572b4, "Mac OS ROM 1.6"},
	{0xfcce2265, "Mac OS ROM 1.4"},
	{0xceace951, "Mac OS ROM 1.2"},
	{0x2172203c, "Mac OS ROM 1.1.2"},
	{0xb5df73dd, "Mac OS ROM 1.1"},
};
static const uint32_t nw_bad[] = {
	0x497f0241,    /* 1.2.1 */
	0x5148b324,    /* 1.8.1 */
};
/* The Old World ROMs (their checksums, at the start) that start no Mac OS
   in SheepShaver: the G3 desktop's "v3" (a black screen). */
static const uint32_t ow_bad[] = {
	0x78f57389,
};
/* The Old World ROMs tested (they start Mac OS 7.5.5 to 9.0.4), named by
   their Macs. */
static const struct {
	uint32_t checksum;
	const char *name;
} ow_known[] = {
	{0x96cd923d, "Power Mac 7200 to 9500 v1"},
	{0x9630c68b, "Power Mac 7200 to 9500 v2"},
	{0x960e4be9, "Power Mac 7300 to 9600 v1"},
	{0x960fc647, "Power Mac 8600, 9600 v2"},
	{0x6f5724c0, "Power Mac 6400"},
	{0x6e92fe08, "Power Mac 6500"},
	{0x79d68d63, "Power Mac G3 desktop"},
};

/* Whether the Mac OS ROM file DATA (SIZE bytes) is packed in parcels: the
   data its header's "constant lzss-offset" or "constant parcels-offset"
   points to starts "prcl" (DecodeROM()). */
static int nw_parcels(const uint8_t *data, uint32_t size)
{
	static const char *const keys[] = {" constant lzss-offset", " constant parcels-offset"};
	unsigned k;
	uint32_t i, n = size < 0x8000 ? size : 0x8000;    /* the header's text */
	for (k = 0; k < 2; k++) {
		size_t len = strlen(keys[k]);
		for (i = 6; i + len <= n; i++) {
			if (memcmp(data + i, keys[k], len) == 0) {
				char hex[7];
				uint32_t at;
				memcpy(hex, data + i - 6, 6);
				hex[6] = '\0';
				at = (uint32_t) strtoul(hex, NULL, 16);
				return at + 4 <= size && memcmp(data + at, "prcl", 4) == 0;
			}
		}
	}
	return FALSE;
}

/* A PowerPC ROM file (see the top): PATH as shown, FILE on the card, SIZE
   bytes. HEAD is its first 16 bytes, BOARD the 16 at 0x30D064 (a 4 MB
   one's), or NULL. */
static void add_ppc_rom(const char *path, const char *file, uint32_t size,
                        const uint8_t *head, const uint8_t *board)
{
	static const char *const boards[] = {
		"Boot TNT", "Boot Alchemy", "Boot Zanzibar", "Boot Gazelle", "Boot Gossamer",
		"NewWorld"
	};
	struct bmmac_rom *r;
	const char *base = strrchr(path, '/');
	unsigned i;
	if (rom_count >= MAX_ROMS)
		return;
	base = base != NULL ? base + 1 : path;
	r = &roms[rom_count];
	r->preferred = 0;
	if (memcmp(head, "<CHRP-BOOT>", 11) == 0 && size < PPC_ROM_SIZE) {
		/* A Mac OS ROM file: read whole, to know it. */
		uint8_t *data = malloc(size);
		FILE *f = fopen(file, "rb");
		uint32_t crc;
		int got = 0;
		if (data != NULL && f != NULL)
			got = fread(data, 1, size, f) == size;
		if (f != NULL)
			fclose(f);
		if (!got) {
			free(data);
			return;
		}
		crc = (uint32_t) crc32(0L, data, size);
		got = nw_parcels(data, size);
		free(data);
		for (i = 0; i < sizeof nw_bad / sizeof nw_bad[0]; i++)
			if (crc == nw_bad[i])
				got = TRUE;
		if (got) {
			printf("BMMacintosh: ROM %s: a Mac OS ROM SheepShaver cannot start (%08X), left out\n",
			       path, (unsigned) crc);
			return;
		}
		r->checksum = 0;
		snprintf(r->name, sizeof r->name, "New World: %.50s", base);
		for (i = 0; i < sizeof nw_good / sizeof nw_good[0]; i++) {
			if (crc == nw_good[i].crc) {
				snprintf(r->name, sizeof r->name, "New World: %s", nw_good[i].name);
				r->preferred = (int) (sizeof nw_good / sizeof nw_good[0] - i);
			}
		}
	} else if (size == PPC_ROM_SIZE && board != NULL) {
		unsigned b;
		for (b = 0; b < sizeof boards / sizeof boards[0]; b++)
			if (memcmp(board, boards[b], strlen(boards[b])) == 0)
				break;
		if (b == sizeof boards / sizeof boards[0])
			return;
		r->checksum = be32(head);
		for (i = 0; i < sizeof ow_bad / sizeof ow_bad[0]; i++) {
			if (r->checksum == ow_bad[i]) {
				printf("BMMacintosh: ROM %s: an Old World ROM SheepShaver cannot start (%08X), left out\n",
				       path, (unsigned) r->checksum);
				return;
			}
		}
		snprintf(r->name, sizeof r->name, "Old World %s (%08X)",
		         boards[b] + (boards[b][0] == 'B' ? 5 : 0), (unsigned) r->checksum);
		for (i = 0; i < sizeof ow_known / sizeof ow_known[0]; i++)
			if (r->checksum == ow_known[i].checksum)
				snprintf(r->name, sizeof r->name, "Old World: %s (%08X)", ow_known[i].name,
				         (unsigned) r->checksum);
	} else {
		return;
	}
	rom_count++;
	snprintf(r->path, sizeof r->path, "%s", path);
	r->size = size;
	r->machine = BMMAC_PPC;
	printf("BMMacintosh: ROM %s: %s, %u KB, Power Mac\n", path, r->name, (unsigned) (size / 1024));
}

/* ---- Zip files ----------------------------------------------------------------- */

struct zip_entry {
	char name[64];
	uint32_t offset;          /* of its local header */
	uint32_t size;            /* uncompressed */
	uint32_t csize;
	unsigned method;          /* 0 stored, 8 deflated */
};

#define MAX_ENTRIES 32

static unsigned le16(const uint8_t *p)
{
	return p[0] | p[1] << 8;
}

static uint32_t le32(const uint8_t *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t) p[3] << 24;
}

/* Reads the zip's table of contents (its central directory). */
static int zip_list(FILE *f, struct zip_entry *entries, int max)
{
	uint8_t tail[22 + 256], *eocd = NULL;
	uint8_t head[46];
	long size, start, i;
	uint32_t dir_offset;
	unsigned n, k;
	int count = 0;
	if (fseek(f, 0, SEEK_END) != 0)
		return 0;
	size = ftell(f);
	start = size > (long) sizeof tail ? size - (long) sizeof tail : 0;
	if (fseek(f, start, SEEK_SET) != 0)
		return 0;
	if (fread(tail, 1, size - start, f) != (size_t) (size - start))
		return 0;
	for (i = size - start - 22; i >= 0; i--) {
		if (le32(tail + i) == 0x06054b50) {
			eocd = tail + i;
			break;
		}
	}
	if (eocd == NULL)
		return 0;
	n = le16(eocd + 10);
	dir_offset = le32(eocd + 16);
	if (fseek(f, dir_offset, SEEK_SET) != 0)
		return 0;
	for (k = 0; k < n && count < max; k++) {
		unsigned name_len, extra_len, comment_len;
		struct zip_entry *e = &entries[count];
		if (fread(head, 1, sizeof head, f) != sizeof head || le32(head) != 0x02014b50)
			break;
		name_len = le16(head + 28);
		extra_len = le16(head + 30);
		comment_len = le16(head + 32);
		e->method = le16(head + 10);
		e->csize = le32(head + 20);
		e->size = le32(head + 24);
		e->offset = le32(head + 42);
		if (name_len < sizeof e->name) {
			if (fread(e->name, 1, name_len, f) != name_len)
				break;
			e->name[name_len] = '\0';
			name_len = 0;
		} else {
			e->name[0] = '\0';
		}
		if (fseek(f, name_len + extra_len + comment_len, SEEK_CUR) != 0)
			break;
		if (e->name[0] != '\0' && (e->method == 0 || e->method == 8))
			count++;
	}
	return count;
}

/* Reads entry E into BUF (at least E->size bytes). */
static int zip_read(FILE *f, const struct zip_entry *e, uint8_t *buf)
{
	uint8_t head[30], *in;
	z_stream z;
	int ok;
	if (fseek(f, e->offset, SEEK_SET) != 0 || fread(head, 1, sizeof head, f) != sizeof head
	    || le32(head) != 0x04034b50)
		return FALSE;
	if (fseek(f, le16(head + 26) + le16(head + 28), SEEK_CUR) != 0)
		return FALSE;
	if (e->method == 0)
		return fread(buf, 1, e->size, f) == e->size;
	in = malloc(e->csize);
	if (in == NULL)
		return FALSE;
	if (fread(in, 1, e->csize, f) != e->csize) {
		free(in);
		return FALSE;
	}
	memset(&z, 0, sizeof z);
	ok = inflateInit2(&z, -MAX_WBITS) == Z_OK;
	if (ok) {
		z.next_in = in;
		z.avail_in = e->csize;
		z.next_out = buf;
		z.avail_out = e->size;
		ok = inflate(&z, Z_FINISH) == Z_STREAM_END && z.total_out == e->size;
		inflateEnd(&z);
	}
	free(in);
	return ok;
}

static int has_ext(const char *name, const char *ext)
{
	size_t n = strlen(name), e = strlen(ext);
	return n > e && strcasecmp(name + n - e, ext) == 0;
}

/* N chips (2 or 4: the ROM's bytes in turn, the first chip's first) into
   one ROM. */
static void interleave(uint8_t *const chips[], int n, uint32_t chip_size, uint8_t *out)
{
	uint32_t i;
	int k;
	for (i = 0; i < chip_size; i++)
		for (k = 0; k < n; k++)
			out[i * n + k] = chips[k][i];
}

#define MAX_CHIPS 4

/* A ROM found in a zip: added to the list (WANT NULL), or, if it is the
   one WANT names, copied to OUT. Returns its size if it was WANT's. */
static uint32_t found_rom(const char *shown, const char *item, const uint8_t *buf,
                          uint32_t size, const char *want, uint8_t *out)
{
	char path[200];
	if (want == NULL) {
		snprintf(path, sizeof path, "%s:%s", shown, item);
		add_rom(path, buf, size);
		return 0;
	}
	if (strcmp(want, item) != 0)
		return 0;
	memcpy(out, buf, size);
	return size;
}

/* Tries the chips in ORDER (indexes into ENTRIES, whose data is in DATA)
   as one ROM. */
static uint32_t try_chips(const struct zip_entry *entries, uint8_t *const data[],
                          const int order[], int n, uint8_t *buf, const char *shown,
                          const char *want, uint8_t *out)
{
	uint8_t *chips[MAX_CHIPS];
	char item[200];
	size_t used = 0;
	int k;
	for (k = 0; k < n; k++) {
		chips[k] = data[order[k]];
		used += snprintf(item + used, sizeof item - used, "%s%s", k ? "+" : "",
		                 entries[order[k]].name);
		if (used >= sizeof item)
			return 0;
	}
	if (want != NULL && strcmp(want, item) != 0)
		return 0;
	interleave(chips, n, entries[order[0]].size, buf);
	if (!checksum_ok(buf, entries[order[0]].size * n))
		return 0;
	return found_rom(shown, item, buf, entries[order[0]].size * n, want, out);
}

/* The ROMs in zip PATH (shown as SHOWN): whole ones, and ones in chips
   (MAME's sets have the Mac 128K's, 512K's and Plus's in two, the IIci's
   in four): any two chips of a size, or a zip's four, in the order whose
   checksum is right. With WANT (an entry's name, or the chips' joined
   with "+" in their order), only that one is read, into OUT; returns its
   size. */
static uint32_t zip_roms(const char *path, const char *shown, const char *want, uint8_t *out)
{
	struct zip_entry entries[MAX_ENTRIES];
	uint8_t *data[MAX_ENTRIES];
	uint8_t *buf;
	uint32_t found = 0;
	int n, i, j, k, l, chips = 0;
	FILE *f = fopen(path, "rb");
	if (f == NULL)
		return 0;
	n = zip_list(f, entries, MAX_ENTRIES);
	memset(data, 0, sizeof data);
	buf = malloc(MAX_ROM_SIZE);
	if (buf == NULL)
		goto done;
	/* Whole ROMs; the rest, of a chip's size, are read for the next. */
	for (i = 0; i < n && found == 0; i++) {
		const struct zip_entry *e = &entries[i];
		if (e->size < 0x8000 || e->size > MAX_ROM_SIZE)
			continue;
		if (!zip_read(f, e, buf))
			continue;
		if (checksum_ok(buf, e->size)) {
			found = found_rom(shown, e->name, buf, e->size, want, out);
			continue;
		}
		if (e->size > MAX_ROM_SIZE / 2)
			continue;
		data[i] = malloc(e->size);
		if (data[i] != NULL) {
			memcpy(data[i], buf, e->size);
			chips++;
		}
	}
	/* Two chips. */
	for (i = 0; i < n && found == 0; i++) {
		for (j = 0; j < n && found == 0 && data[i] != NULL; j++) {
			int order[2] = {i, j};
			if (j == i || data[j] == NULL || entries[j].size != entries[i].size)
				continue;
			found = try_chips(entries, data, order, 2, buf, shown, want, out);
		}
	}
	/* Four, of a size: every order. */
	if (chips == 4 && found == 0) {
		int c[4], m = 0;
		for (i = 0; i < n; i++)
			if (data[i] != NULL)
				c[m++] = i;
		if (entries[c[0]].size == entries[c[1]].size && entries[c[0]].size == entries[c[2]].size
		    && entries[c[0]].size == entries[c[3]].size && entries[c[0]].size * 4 <= MAX_ROM_SIZE) {
			for (i = 0; i < 4 && found == 0; i++)
				for (j = 0; j < 4 && found == 0; j++)
					for (k = 0; k < 4 && found == 0; k++)
						for (l = 0; l < 4 && found == 0; l++) {
							int order[4] = {c[i], c[j], c[k], c[l]};
							if (i == j || i == k || i == l || j == k || j == l || k == l)
								continue;
							found = try_chips(entries, data, order, 4, buf, shown,
							                  want, out);
						}
		}
	}
done:
	for (i = 0; i < n; i++)
		free(data[i]);
	free(buf);
	fclose(f);
	return found;
}

/* ---- Looking ------------------------------------------------------------------ */

/* ONLY_ROM: only .rom files (the card's root, where the firmware is). */
static void scan_dir(const char *dir, int only_rom)
{
	char dir_path[256];
	DIR *d;
	struct dirent *e;
	uint8_t *buf;
	bmmac_card_path(dir_path, sizeof dir_path, dir, "");
	d = opendir(dir_path);
	if (d == NULL)
		return;
	buf = malloc(MAX_ROM_SIZE);
	while (buf != NULL && (e = readdir(d)) != NULL && rom_count < MAX_ROMS) {
		char path[256], shown[200];
		struct stat st;
		if (e->d_name[0] == '.')
			continue;
		bmmac_card_path(path, sizeof path, dir, e->d_name);
		snprintf(shown, sizeof shown, "%s%s%s", dir, dir[0] ? "/" : "", e->d_name);
		if (only_rom && !has_ext(e->d_name, ".rom"))
			continue;
		if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
			continue;
		if (has_ext(e->d_name, ".zip")) {
			zip_roms(path, shown, NULL, NULL);
		} else if (st.st_size >= 0x10000 && st.st_size <= MAX_ROM_SIZE) {
			FILE *f = fopen(path, "rb");
			size_t got = 0;
			if (f == NULL)
				continue;
			got = fread(buf, 1, st.st_size, f);
			fclose(f);
			if (got == (size_t) st.st_size && checksum_ok(buf, (uint32_t) got))
				add_rom(shown, buf, (uint32_t) got);
		} else if (st.st_size > MAX_ROM_SIZE && st.st_size <= PPC_ROM_SIZE) {
			uint8_t head[16], board[16];
			int has_board = FALSE;
			FILE *f = fopen(path, "rb");
			if (f == NULL)
				continue;
			if (fread(head, 1, sizeof head, f) == sizeof head) {
				has_board = fseek(f, 0x30d064, SEEK_SET) == 0
				            && fread(board, 1, sizeof board, f) == sizeof board;
				add_ppc_rom(shown, path, (uint32_t) st.st_size, head, has_board ? board : NULL);
			}
			fclose(f);
		}
	}
	free(buf);
	closedir(d);
}

int bmmac_rom_scan(void)
{
	rom_count = 0;
	scan_dir("roms", FALSE);
	scan_dir("", TRUE);
	printf("BMMacintosh: %d Mac ROM(s)\n", rom_count);
	return rom_count;
}

int bmmac_rom_count(void)
{
	return rom_count;
}

const struct bmmac_rom *bmmac_rom_get(int n)
{
	return n >= 0 && n < rom_count ? &roms[n] : NULL;
}

const struct bmmac_rom *bmmac_rom_find(const char *path)
{
	int i;
	if (path == NULL)
		return NULL;
	for (i = 0; i < rom_count; i++)
		if (strcmp(roms[i].path, path) == 0)
			return &roms[i];
	return NULL;
}

const struct bmmac_rom *bmmac_rom_default(int machine)
{
	const struct bmmac_rom *best = NULL;
	int i;
	for (i = 0; i < rom_count; i++) {
		if (roms[i].machine != machine)
			continue;
		/* The Quadra 650's and 800's ROM is Basilisk II's best. */
		if (roms[i].checksum == 0xf1acad13)
			return &roms[i];
		/* SheepShaver's: a New World Mac OS ROM file (checksum 0) before
		   an Old World 4 MB ROM, and a tested one (the newest) first. */
		if (machine == BMMAC_PPC && best != NULL
		    && ((best->checksum != 0 && roms[i].checksum == 0)
		        || (roms[i].checksum == 0 && roms[i].preferred > best->preferred)))
			best = &roms[i];
		if (best == NULL)
			best = &roms[i];
	}
	return best;
}

uint32_t bmmac_rom_load(const struct bmmac_rom *rom, uint8_t *buf, uint32_t size)
{
	char path[256], file[200];
	const char *colon;
	uint32_t got = 0;
	if (rom == NULL || rom->size > size)
		return 0;
	colon = strstr(rom->path, ".zip:");
	if (colon == NULL)
		colon = strstr(rom->path, ".ZIP:");
	if (colon != NULL) {
		snprintf(file, sizeof file, "%.*s", (int) (colon + 4 - rom->path), rom->path);
		bmmac_card_path(path, sizeof path, "", file);
		got = zip_roms(path, file, colon + 5, buf);
	} else {
		FILE *f;
		bmmac_card_path(path, sizeof path, "", rom->path);
		f = fopen(path, "rb");
		if (f != NULL) {
			got = (uint32_t) fread(buf, 1, rom->size, f);
			fclose(f);
		}
	}
	if (got != rom->size || (rom->machine != BMMAC_PPC && !checksum_ok(buf, got))) {
		printf("BMMacintosh: cannot read the ROM %s\n", rom->path);
		return 0;
	}
	return got;
}
