/*
 * main_ppc.cpp - SheepShaver's main program for BMMacintosh: the Power
 * Macintosh (the Pi 5).
 *
 * Does what SheepShaver's Unix main_unix.cpp does, on BMMacintosh's front
 * end (src/port/frontend.c): reads the settings (macppc.prefs on the card,
 * in SheepShaver's own format), lays the Mac's memory out (see below),
 * loads the ROM ("Mac OS ROM" for the New World Macs, or an Old World
 * Power Mac's 4 MB ROM), starts the drivers (InitAll()) and the PowerPC.
 *
 * The Mac's memory is one block (direct addressing, see kpx/cpu/vm.hpp),
 * by Mac address:
 *
 *   0                  RAM
 *   ROMBase            the ROM area (ROM_AREA_SIZE, 1 MB aligned), then
 *                      the PowerPC's interrupt stack (SIG_STACK_SIZE)
 *   then               SheepShaver's data (SheepMem, 512K)
 *   then               the frame buffer (video_ppc.cpp)
 *   last               a page of zeros (SheepMem::ZeroPage())
 *
 * and the kernel data, at 0x68ffe000 and 0x5fffe000, an array of its own.
 *
 * There are no threads. The interpreter counts down ppc_ticks with each
 * block of instructions it runs (kpx/cpu/spcflags.hpp), and calls
 * ppc_check_ticks() when it runs out, every few thousand blocks, which
 * looks at the clock (see "The frame loop"). Once a frame it raises the
 * 60 Hz interrupt, and bmmac_frame() shows the screen, paces the frame and
 * reads the keyboard and mouse.
 *
 * Restarting the Mac (the menu, the web UI, Shut Down in the Finder)
 * throws mac_restart out of the CPU's loop to emu_main(), which starts
 * everything again.
 *
 * Copyright (C) 1997-2008 Christian Bauer and Marc Hellwig (main_unix.cpp,
 * which this follows), GPL-2.0-or-later.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "main.h"
#include "prefs.h"
#include "xpram.h"
#include "timer.h"
#include "adb.h"
#include "rom_patches.h"
#include "user_strings.h"
#include "xlowmem.h"
#include "thunks.h"
#include "macos_util.h"
#include "vm_alloc.h"
#include "sigsegv.h"

#include <sys/stat.h>

#include "../../host.h"
#include "../bmmac.h"
#include "ppc.h"

/* ---- SheepShaver's globals ------------------------------------------------------ */

void *TOC = NULL;
void *R13 = NULL;
uint32 RAMBase;
uint32 RAMSize;
uint32 ROMBase;
uint32 KernelDataAddr;
uint32 BootGlobsAddr;
uint32 PVR;
int64 CPUClockSpeed;
int64 BusClockSpeed;
int64 TimebaseSpeed;
uint8 *RAMBaseHost;
uint8 *ROMBaseHost;
uint32 ROMEnd;
char *vde_sock;
bool tick_inhibit;
volatile uint32 InterruptFlags = 0;

uint32  SheepMem::page_size;
uintptr SheepMem::zero_page = 0;
uintptr SheepMem::base;
uintptr SheepMem::proc;
uintptr SheepMem::data;

/* The PowerPC's interrupt stack, after the ROM area (main_unix.cpp). */
static const uint32 SIG_STACK_SIZE = 0x10000;
static uint32 sig_stack;

uintptr SignalStackBase(void)
{
	return sig_stack + SIG_STACK_SIZE;
}

/* sheepshaver_glue.cpp */
extern void emul_ppc(uint32 start);
extern void init_emul_ppc(void);
extern void exit_emul_ppc(void);

/* ---- The Mac's memory (kpx/cpu/vm.hpp) ------------------------------------------ */

uintptr mac_base;
uint32 mac_span;
uint8 gKernelData[0x2000];
static uint8 *mac_memory;           /* the block, as allocated */
static uint32 zero_page_mac;        /* the zero page's Mac address */
static uint8 *zero_page_host;
/* What the Mac writes where it has no memory (lost), and reads there
   (zeros, never written). */
static uint8 mac_stray_page[0x10000];
static const uint8 mac_stray_read[0x10000] = {0};
/* The ROM area, write-protected once Mac OS starts (kpx/cpu/vm.hpp). */
uint32 mac_rom_lo, mac_rom_len;
uint32 ppc_frame_buffer_mac;
uint8 *ppc_frame_buffer;

uint8 *mac_other_address(uint32 addr, int write)
{
	if ((addr & ~0x1fffu) == KERNEL_DATA_BASE || (addr & ~0x1fffu) == KERNEL_DATA2_BASE)
		return &gKernelData[addr & 0x1fff];
	if (addr - zero_page_mac < 0x1000u) {
		if (!write)
			return zero_page_host + (addr - zero_page_mac);
		return mac_stray_page;
	}
	/* A write to the ROM, which SheepShaver's Unix port skips. */
	if (write && addr - mac_rom_lo < mac_rom_len)
		return mac_stray_page + (addr & 0x7fff);
	static int logged;
	if (logged < 8) {
		logged++;
		printf("BMMacintosh: the Mac %s %08x, where it has no memory\n",
		       write ? "wrote to" : "read", (unsigned) addr);
	}
	if (!write)
		return (uint8 *) mac_stray_read + (addr & 0x7fff);
	return mac_stray_page + (addr & 0x7fff);
}

/* SheepShaver's data (Unix main_unix.cpp's SheepMem::Init()), placed by
   alloc_memory(). */
static uint32 sheep_mem_mac;
/* SheepMem::size (thunks.h), which is protected. */
#define SHEEP_MEM_SIZE 0x80000

bool SheepMem::Init(void)
{
	page_size = 4096;
	proc = base = sheep_mem_mac;
	data = base + size;
	zero_page = zero_page_mac;
	return true;
}

void SheepMem::Exit(void)
{
}

static bool alloc_memory(void)
{
	RAMSize = PrefsFindInt32("ramsize");
	if (RAMSize <= 1000)
		RAMSize *= 1024 * 1024;
	RAMSize &= 0xfff00000;    /* whole megabytes */
	if (RAMSize < 16 * 1024 * 1024)
		RAMSize = 16 * 1024 * 1024;
	if (RAMSize > PPC_MAX_RAM)
		RAMSize = PPC_MAX_RAM;

	uint32 rom = (RAMSize + 0xfffff) & ~0xfffffu;
	uint32 sheep = rom + ROM_AREA_SIZE + SIG_STACK_SIZE;
	uint32 fb = (sheep + SHEEP_MEM_SIZE + 0xfffff) & ~0xfffffu;
	uint32 zero = fb + PPC_FRAME_BUFFER_SIZE;
	uint32 span = zero + 0x1000;

	free(mac_memory);
	mac_memory = (uint8 *) calloc(1, (size_t) span + 4096);
	if (mac_memory == NULL) {
		printf("BMMacintosh: no memory for a %u MB Mac\n", (unsigned) (RAMSize >> 20));
		return false;
	}
	memset(gKernelData, 0, sizeof gKernelData);
	mac_base = ((uintptr) mac_memory + 4095) & ~(uintptr) 4095;
	mac_span = zero;    /* the zero page is reached through mac_other_address() */
	RAMBase = 0;
	RAMBaseHost = (uint8 *) mac_base;
	ROMBase = rom;
	ROMBaseHost = (uint8 *) mac_base + rom;
	ROMEnd = rom + ROM_AREA_SIZE;
	sig_stack = ROMEnd;
	sheep_mem_mac = sheep;
	ppc_frame_buffer_mac = fb;
	ppc_frame_buffer = (uint8 *) mac_base + fb;
	zero_page_mac = zero;
	zero_page_host = (uint8 *) mac_base + zero;
	KernelDataAddr = KERNEL_DATA_BASE;
	return true;
}

/* ---- The ROM ----------------------------------------------------------------------- */

static const struct bmmac_rom *rom_in_use;

/* The ROM from macppc.prefs, or the best one found: a "Mac OS ROM" file or
   a Power Mac's 4 MB ROM, which DecodeROM() (rom_patches.cpp) unpacks into
   the ROM area. */
static bool load_rom(void)
{
	const struct bmmac_rom *rom = bmmac_rom_find(PrefsFindString("rom"));
	if (rom == NULL || rom->machine != BMMAC_PPC)
		rom = bmmac_rom_default(BMMAC_PPC);
	if (rom == NULL)
		return false;
	uint32 size = rom->size;
	uint8 *buf = (uint8 *) malloc(size);
	if (buf == NULL)
		return false;
	size = bmmac_rom_load(rom, buf, size);
	bool ok = size != 0 && DecodeROM(buf, size);
	free(buf);
	if (!ok) {
		printf("BMMacintosh: SheepShaver cannot use the ROM %s\n", rom->path);
		return false;
	}
	rom_in_use = rom;
	if (PrefsFindString("rom") == NULL || strcmp(PrefsFindString("rom"), rom->path) != 0) {
		PrefsReplaceString("rom", rom->path);
		SavePrefs();
	}
	printf("BMMacintosh: %s, ROM %s\n", emu_machine_text(), rom->path);
	return true;
}

/* ---- Platform functions (main.h) ------------------------------------------------- */

void SetInterruptFlag(uint32 flag)
{
	InterruptFlags |= flag;
}

void ClearInterruptFlag(uint32 flag)
{
	InterruptFlags &= ~flag;
}

void DisableInterrupt(void)
{
	WriteMacInt32(XLM_IRQ_NEST, int32(ReadMacInt32(XLM_IRQ_NEST)) + 1);
}

void EnableInterrupt(void)
{
	WriteMacInt32(XLM_IRQ_NEST, int32(ReadMacInt32(XLM_IRQ_NEST)) - 1);
}

struct B2_mutex {
	int dummy;
};

B2_mutex *B2_create_mutex(void)
{
	return new B2_mutex;
}

void B2_lock_mutex(B2_mutex *mutex)
{
	(void) mutex;
}

void B2_unlock_mutex(B2_mutex *mutex)
{
	(void) mutex;
}

void B2_delete_mutex(B2_mutex *mutex)
{
	delete mutex;
}

void Dump68kRegs(M68kRegisters *r)
{
	for (int i = 0; i < 8; i++)
		printf("d%d: %08x%s", i, r->d[i], i == 3 || i == 7 ? "\n" : ", ");
	for (int i = 0; i < 8; i++)
		printf("a%d: %08x%s", i, r->a[i], i == 3 || i == 7 ? "\n" : ", ");
}

void MakeExecutable(int dummy, uint32 start, uint32 length)
{
	(void) dummy;
	if (start >= ROMBase && start < ROMBase + ROM_SIZE)
		return;
	FlushCodeCache(start, start + length);
}

void ErrorAlert(const char *text)
{
	printf("SheepShaver: error: %s\n", text);
	osd_notify(text);
}

void WarningAlert(const char *text)
{
	printf("SheepShaver: warning: %s\n", text);
	osd_notify(text);
}

bool ChoiceAlert(const char *text, const char *pos, const char *neg)
{
	printf("SheepShaver asks: %s (%s)\n", text, neg);
	(void) pos;
	return false;
}

/* Thrown out of the CPU's loop to start the Mac again. */
struct mac_restart {
	int shut_down;    /* the Mac shut itself down */
};

/* The Mac shut down (the Finder's Shut Down), or SheepShaver can't go on. */
void QuitEmulator(void)
{
	printf("SheepShaver: the Mac has shut down\n");
	throw mac_restart{1};
}

/* ---- Restarting -------------------------------------------------------------------- */

static volatile bool restart_requested;

void emu_cold_reset(void)
{
	restart_requested = true;
}

/* ---- The frame loop (ppc_check_ticks()) ------------------------------------------ */

/* The PowerPC runs by the clock: it is given blocks of instructions a
   CHUNK at a time, and after each the clock says whether it may go on or
   the frame is due. It runs until a share (cpu_share) of the frame's time
   since the last frame has passed; the rest is the frame's own work (the
   screen, which the Pi 5's ARM scales and draws) and the wait for the
   display. The share grows while the frames come on time, and shrinks
   when one is missed, so the PowerPC gets what the frame's work leaves.
   (A count of blocks for each frame, as the Mac II's instructions are
   counted, didn't do: PowerPC blocks vary too much in their cost.)

   In the PC simulator the clock stands still within a frame: each frame
   then runs max_blocks, which keeps the tests repeatable. */
#define CHUNK 8000
int32 ppc_ticks = CHUNK;
static int32 max_blocks = 4000000;
static int32 frame_blocks;          /* run since the last frame */
static int32 last_frame_blocks = 100000;
static double frame_start;          /* when the last frame was done */
static double cpu_share = 0.7;

/* The Mac's clock (as the Mac II's, main_ii.cpp): a frame's time for each
   frame run, and within one the share of a frame's blocks done. */
static uint64 mac_frames_usec;

uint64 ii_mac_time_usec(void)
{
	double done = (double) (frame_blocks + (CHUNK - ppc_ticks)) / last_frame_blocks;
	if (done < 0.0)
		done = 0.0;
	if (done > 1.0)
		done = 1.0;
	return mac_frames_usec + (uint64) (1000000.0 / 60.15 * done);
}

/* The Mac's parameter RAM is saved when it changes (main_unix.cpp's NVRAM
   watchdog), checked each second. */
static uint8 last_xpram[XPRAM_SIZE];

static void one_tick(void)
{
	static int tick_counter;
	if (++tick_counter >= 60) {
		tick_counter = 0;
		WriteMacInt32(0x20c, TimerDateTime());
		if (memcmp(last_xpram, XPRAM, XPRAM_SIZE) != 0) {
			memcpy(last_xpram, XPRAM, XPRAM_SIZE);
			SaveXPRAM();
		}
	}
	if (ReadMacInt32(XLM_IRQ_NEST) == 0) {
		SetInterruptFlag(INTFLAG_VIA);
		TriggerInterrupt();
	}
	/* The Ethernet interrupt, when the network has frames for the Mac
	   (the Mac II's ether_ii.cpp). */
	ii_ether_tick();
}

/* Every ten seconds the log says how fast the PowerPC runs: blocks of
   instructions a second of the CPU's own time, how many a frame gets, and
   the share of the time the CPU had (the rest is the frame's work: the
   screen, the sound, the input). */
static double frame_work;    /* bmmac_frame()'s time (frame()) */
double ppc_copy_time;        /* the screen's conversion (video_ppc.cpp) */

static void log_speed(int32 blocks_run, double cpu_time)
{
	static int frames;
	static double start, blocks, cpu;
	if (frames == 0)
		start = host_time();
	blocks += blocks_run;
	cpu += cpu_time;
	if (++frames < 600)
		return;
	double secs = host_time() - start;
	if (secs > 0 && cpu > 0)
		printf("SheepShaver: %.1f million blocks a CPU second, %d a frame, CPU %d%% of the time;"
		       " a frame's work %.1f ms (the screen's copy %.1f ms)\n",
		       blocks / cpu / 1e6, (int) (blocks / frames), (int) (100.0 * cpu / secs),
		       frame_work * 1000.0 / frames, ppc_copy_time * 1000.0 / frames);
	frame_work = 0;
	ppc_copy_time = 0;
	frames = 0;
	blocks = 0;
	cpu = 0;
}

static void frame(void)
{
	double now = host_time();
	log_speed(frame_blocks, now - frame_start);
	if (frame_blocks > 0)
		last_frame_blocks = frame_blocks;
	frame_blocks = 0;
	mac_frames_usec += (uint64) (1000000.0 / 60.15);
	if (!tick_inhibit)
		one_tick();
	ii_audio_frame();
	double work = host_time();
	bmmac_frame();
	frame_work += host_time() - work;
	ii_adb_frame();
	if (restart_requested) {
		restart_requested = false;
		throw mac_restart{0};
	}
	/* A frame missed (more than one and a half periods since the last):
	   less for the PowerPC. On time: a little more. A long gap (the menu)
	   says nothing. */
	double done = host_time(), period = 1.0 / emu_frame_rate();
	double gap = done - frame_start;
	if (gap < 0.25) {
		if (gap > period * 1.5)
			cpu_share -= 0.05;
		else
			cpu_share += 0.005;
		if (cpu_share < 0.2)
			cpu_share = 0.2;
		if (cpu_share > 0.95)
			cpu_share = 0.95;
	}
	frame_start = done;
}

/* On the Pi 4 and 5 another core shows the frames (src/av/video.cpp): the
   PowerPC then runs until the display has shown the last frame, which is
   when the next is due, instead of a share of each frame's time; and it
   lets Circle's tasks (the network) run every millisecond, as it no longer
   waits for the display, which is when they ran. */
void ppc_check_ticks(void)
{
	static double last_yield;
	frame_blocks += CHUNK;
	ii_audio_service();
	int due = bmmac_frame_due();
	if (due >= 0) {
		double now = host_time();
		if (due || frame_blocks >= max_blocks) {
			frame();
		} else if (now - last_yield >= 0.001) {
			host_yield();
			last_yield = now;
		}
	} else {
		double period = 1.0 / emu_frame_rate();
		if (frame_blocks >= max_blocks || host_time() - frame_start >= period * cpu_share)
			frame();
	}
	ppc_ticks = CHUNK;
}

/* ---- The emulator's side of the front end ----------------------------------------- */

int emu_machine(void)
{
	return BMMAC_PPC;
}

const char *emu_machine_text(void)
{
	static char text[96];
	const char *model = rom_in_use != NULL ? rom_in_use->name : "no ROM";
	snprintf(text, sizeof text, "%s, PowerPC, %u MB", model, (unsigned) (RAMSize >> 20));
	return text;
}

const char *emu_credits(void)
{
	return "Emulation: SheepShaver (GPL 2 or later), by Christian Bauer,\n"
	       "Marc Hellwig, Gwenole Beauchesne and others; kanjitalk755's\n"
	       "macemu.";
}

double emu_frame_rate(void)
{
	return 60.15;
}

const char *const *emu_run_types(void)
{
	static const char *const types[] = {
		"dsk", "img", "image", "hfv", "hda", "dmg", "iso", "toast", "cdr", NULL
	};
	return types;
}

/* ---- Running ---------------------------------------------------------------------- */

static void system_info(void)
{
	/* main_unix.cpp's get_system_info() for the emulated PowerPC: a 7400
	   (G4 with AltiVec). */
	PVR = 0x000c0000;
	CPUClockSpeed = 100000000;
	BusClockSpeed = 100000000;
	TimebaseSpeed = 25000000;
	int clock = PrefsFindInt32("cpuclock");
	if (clock)
		CPUClockSpeed = 1000000 * (int64) clock;
}

static bool start_mac(char *why, size_t size)
{
	if (!alloc_memory()) {
		snprintf(why, size, "Not enough memory for the Mac's RAM");
		return false;
	}
	SheepMem::Init();
	mac_rom_len = 0;    /* written while it's loaded and patched */
	if (!load_rom()) {
		snprintf(why, size, "No Power Macintosh ROM in the roms folder.\n"
		         "SheepShaver needs a \"Mac OS ROM\" file (Mac OS 8.5\n"
		         "to 9.0.4) or a PCI Power Mac's 4 MB ROM. See\n"
		         "roms/ROMS.txt.");
		return false;
	}
	ii_prepare_drives();
	const char *extfs = PrefsFindString("extfs");
	if (extfs != NULL && extfs[0])
		mkdir(extfs, 0777);
	if (!InitAll(NULL)) {
		snprintf(why, size, "SheepShaver cannot start with this ROM.\n"
		         "Choose another one (menu, F12, ROM).");
		ExitAll();
		return false;
	}
	memcpy(last_xpram, XPRAM, XPRAM_SIZE);
	/* The ROM, loaded and patched, is read-only for Mac OS. */
	mac_rom_lo = ROMBase;
	mac_rom_len = ROM_AREA_SIZE;
	/* The mouse moves the Mac's pointer by as much as it moves (as the Mac
	   II's): the keyboard and mouse code (menu_ii.cpp) sends movements. */
	ADBSetRelMouseMode(true);
	return true;
}

void emu_main(void)
{
	static int argc = 1;
	static char *args[] = {(char *) "SheepShaver", NULL};
	char **argv = args;
	char why[256];

	PrefsInit(NULL, argc, argv);
	system_info();
	max_blocks = host_option_int("ppc_max_blocks", 4000000);
	if (max_blocks < 50000)
		max_blocks = 50000;

	for (;;) {
		if (!start_mac(why, sizeof why)) {
			printf("BMMacintosh: %s\n", why);
			bmmac_stopped(why);
			continue;
		}
		bmmac_screen_changed();
		frame_blocks = 0;
		ppc_ticks = CHUNK;
		frame_start = host_time();
		int shut_down = 0;
		try {
			init_emul_ppc();
			emul_ppc(ROMBase + 0x310000);
			shut_down = 1;
		} catch (const mac_restart &r) {
			shut_down = r.shut_down;
		}
		exit_emul_ppc();
		ii_audio_stop();
		ExitAll();
		SheepMem::Exit();
		emu_sync_disks();
		if (shut_down)
			bmmac_stopped("The Mac has shut down.\n"
			              "Press a key to start it again.");
	}
}

/* ---- What the CPU and drivers ask of the host --------------------------------------- */

/* The interpreter's decode cache (vm_alloc.h): plain memory here. */
void *vm_acquire(size_t size, int options)
{
	(void) options;
	void *p = calloc(1, size);
	return p != NULL ? p : VM_MAP_FAILED;
}

int vm_release(void *addr, size_t size)
{
	(void) size;
	free(addr);
	return 0;
}

/* SheepShaver's SIGSEGV handler (sheepshaver_glue.cpp) is never called
   here: kpx/cpu/vm.hpp catches the Mac's stray accesses itself. */
sigsegv_address_t sigsegv_get_fault_address(sigsegv_info_t *sip)
{
	(void) sip;
	return 0;
}

sigsegv_address_t sigsegv_get_fault_instruction_address(sigsegv_info_t *sip)
{
	(void) sip;
	return 0;
}
