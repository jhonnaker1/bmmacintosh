/*
 * sysdeps_ppc.h - SheepShaver's system dependent definitions for
 * BMMacintosh: the Power Macintosh (the Pi 5, and the PC simulator).
 *
 * SheepShaver's Unix port has its own (src/Unix/sysdeps.h, filled in by
 * its configure script). This is BMMacintosh's, included by
 * src/port/ii/sysdeps.h when BMMAC_PPC_BUILD is defined, so that the parts of
 * the Mac II's front end SheepShaver shares with Basilisk II (disks, the
 * shared folder, the clocks, the network) build for it unchanged.
 *
 * The CPU is SheepShaver's PowerPC interpreter (kpx_cpu), with direct
 * addressing: a Mac address plus a constant is the host address. The Mac's
 * RAM (at Mac address 0), its ROM, SheepShaver's own data and the frame
 * buffer are one block of host memory (main_ppc.cpp); its kernel data,
 * which the Mac OS ROM keeps at a fixed high address, is a small array.
 * Memory accesses go through src/port/ppc/kpx/cpu/vm.hpp, BMMacintosh's,
 * which also catches the Mac's accesses where it has no memory.
 *
 * No threads: the 60 Hz tick, the sound and the disks are driven from the
 * frame loop, which runs in the CPU's own loop (src/port/ppc/kpx/cpu/
 * spcflags.hpp counts the blocks of instructions run). No JIT (SheepShaver
 * has none for ARM).
 *
 * Copyright (C) 1997-2008 Christian Bauer and Marc Hellwig (the Unix
 * sysdeps.h this follows), GPL-2.0-or-later.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef SYSDEPS_H
#define SYSDEPS_H

#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#ifndef htons
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

/* ---- Build options -------------------------------------------------------- */

#define SHEEPSHAVER 1
#define EMULATED_PPC 1
#define REAL_ADDRESSING 0
#define DIRECT_ADDRESSING 1
#define USE_ETHER_FULL_DRIVER 1
#define POWERPC_ROM 1
#define ROM_IS_WRITE_PROTECTED 0
#define USE_SCRATCHMEM_SUBTERFUGE 1
#define ENABLE_VOSF 0
#define ENABLE_DYNGEN 0
#define SUPPORTS_EXTFS 1
#define SUPPORTS_UDP_TUNNEL 0
/* The Time Manager runs from the 60 Hz tick (timer.cpp). */
#define PRECISE_TIMING 0
#define HAVE_SIGSEGV_SKIP_INSTRUCTION 1
#define HOST_FLOAT_FORMAT IEEE_FLOAT_FORMAT
#define HAVE_FENV_H 1

/* SheepShaver's PowerPC interpreter (Unix sysdeps.h), with the decode
   cache and no JIT. */
#define PPC_REENTRANT_JIT 1
#define PPC_CHECK_INTERRUPTS 1
#define PPC_DECODE_CACHE 1
#define PPC_FLIGHT_RECORDER 0
#define PPC_PROFILE_COMPILE_TIME 0
#define PPC_PROFILE_GENERIC_CALLS 0
#define PPC_PROFILE_REGS_USE 0
#define KPX_MAX_CPUS 1

/* ---- Types --------------------------------------------------------------- */

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
#ifndef _UINT64
typedef uint64_t uint64;
#define _UINT64
#endif
typedef int64_t int64;
#define VAL64(a) (a ## LL)
#define UVAL64(a) (a ## uLL)
typedef uintptr_t uintptr;
typedef intptr_t intptr;

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE 16
#if UINTPTR_MAX > 0xffffffffu
#define SIZEOF_VOID_P 8
#define SIZEOF_LONG 8
#else
#define SIZEOF_VOID_P 4
#define SIZEOF_LONG 4
#endif

typedef off_t loff_t;
typedef char *caddr_t;

/* Time Manager times (src/port/ii/timer_ii.cpp). */
typedef struct timeval tm_time_t;

#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define VAX_FLOAT_FORMAT 2
#define IBM_FLOAT_FORMAT 3
#define C4X_FLOAT_FORMAT 4

/* ---- Byte swapping (the Mac is big endian, the Pi little) ---------------- */

static inline uint16 generic_bswap_16(uint16 x) { return __builtin_bswap16(x); }
static inline uint32 generic_bswap_32(uint32 x) { return __builtin_bswap32(x); }
static inline uint64 generic_bswap_64(uint64 x) { return __builtin_bswap64(x); }
#define bswap_16 generic_bswap_16
#define bswap_32 generic_bswap_32
#define bswap_64 generic_bswap_64
static inline uint16 tswap16(uint16 x) { return bswap_16(x); }
static inline uint32 tswap32(uint32 x) { return bswap_32(x); }
static inline uint64 tswap64(uint64 x) { return bswap_64(x); }

/* ---- No threads, so no locks --------------------------------------------- */

typedef volatile int spinlock_t;
static const spinlock_t SPIN_LOCK_UNLOCKED = 0;
static inline void spin_lock(spinlock_t *lock) { (void) lock; }
static inline void spin_unlock(spinlock_t *lock) { (void) lock; }
static inline int spin_trylock(spinlock_t *lock) { (void) lock; return 1; }

/* ---- Timing (src/port/ii/timer_ii.cpp) ------------------------------------ */

extern uint64 GetTicks_usec(void);
extern void Delay_usec(uint64 usec);

typedef struct rgb_color {
	uint8 red;
	uint8 green;
	uint8 blue;
	uint8 alpha;
} rgb_color;

/* Calling Mac OS routines (Unix sysdeps.h): the interpreter runs them
   (sheepshaver_glue.cpp). */
#define CallMacOS(type, tvect) call_macos((uintptr)tvect)
#define CallMacOS1(type, tvect, arg1) call_macos1((uintptr)tvect, (uintptr)arg1)
#define CallMacOS2(type, tvect, arg1, arg2) call_macos2((uintptr)tvect, (uintptr)arg1, (uintptr)arg2)
#define CallMacOS3(type, tvect, arg1, arg2, arg3) call_macos3((uintptr)tvect, (uintptr)arg1, (uintptr)arg2, (uintptr)arg3)
#define CallMacOS4(type, tvect, arg1, arg2, arg3, arg4) call_macos4((uintptr)tvect, (uintptr)arg1, (uintptr)arg2, (uintptr)arg3, (uintptr)arg4)
#define CallMacOS5(type, tvect, arg1, arg2, arg3, arg4, arg5) call_macos5((uintptr)tvect, (uintptr)arg1, (uintptr)arg2, (uintptr)arg3, (uintptr)arg4, (uintptr)arg5)
#define CallMacOS6(type, tvect, arg1, arg2, arg3, arg4, arg5, arg6) call_macos6((uintptr)tvect, (uintptr)arg1, (uintptr)arg2, (uintptr)arg3, (uintptr)arg4, (uintptr)arg5, (uintptr)arg6)
#define CallMacOS7(type, tvect, arg1, arg2, arg3, arg4, arg5, arg6, arg7) call_macos7((uintptr)tvect, (uintptr)arg1, (uintptr)arg2, (uintptr)arg3, (uintptr)arg4, (uintptr)arg5, (uintptr)arg6, (uintptr)arg7)

#ifdef __cplusplus
extern "C" {
#endif
extern uint32 call_macos(uint32 tvect);
extern uint32 call_macos1(uint32 tvect, uint32 arg1);
extern uint32 call_macos2(uint32 tvect, uint32 arg1, uint32 arg2);
extern uint32 call_macos3(uint32 tvect, uint32 arg1, uint32 arg2, uint32 arg3);
extern uint32 call_macos4(uint32 tvect, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4);
extern uint32 call_macos5(uint32 tvect, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5);
extern uint32 call_macos6(uint32 tvect, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5, uint32 arg6);
extern uint32 call_macos7(uint32 tvect, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5, uint32 arg6, uint32 arg7);
#ifdef __cplusplus
}
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

#endif /* SYSDEPS_H */
