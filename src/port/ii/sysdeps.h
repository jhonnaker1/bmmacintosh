/*
 * sysdeps.h - Basilisk II's system dependent definitions for BMMacintosh.
 *
 * Basilisk II's Unix port has its own (src/Unix/sysdeps.h, filled in by
 * its configure script); this is BMMacintosh's, for the Raspberry Pi
 * (circle-newlib, 64-bit) and for the PC simulator (tools/hostsim), which
 * build the same code.
 *
 * The CPU is the UAE core Basilisk II uses on ARM (uae_cpu_2021), which
 * only has direct addressing: a Mac address plus a constant (MEMBaseDiff)
 * is the host address. The Mac's RAM, its ROM and the frame buffer are one
 * block of host memory (main.cpp). Real addressing, the Mac's addresses
 * being the host's, would need the Mac's RAM at host address 0, where the
 * Pi has its ARM stub and Circle its exception vectors.
 *
 * No threads: the 60 Hz tick, the sound and the disks are driven from the
 * frame loop, which runs in the CPU's own loop (USE_CPU_EMUL_SERVICES:
 * cpu_do_check_ticks(), main.cpp). No VOSF (the frame buffer is converted
 * whole each frame), no JIT and no network (SLIRP) yet.
 *
 * Copyright (C) 1997-2008 Christian Bauer and the Basilisk II authors (the
 * Unix sysdeps.h this follows), GPL-2.0-or-later.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
/* htons() and the others, for the ROM patches (a macro on the PC). */
#include <netinet/in.h>
#ifndef htons
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

/* ---- Build options -------------------------------------------------------- */

#define REAL_ADDRESSING 0
#ifndef DIRECT_ADDRESSING
#define DIRECT_ADDRESSING 1
#endif
#define EMULATED_68K 1
#define USE_PREFETCH_BUFFER 0
#define ROM_IS_WRITE_PROTECTED 0
#define USE_SCRATCHMEM_SUBTERFUGE 1
#define ENABLE_VOSF 0
#define SUPPORTS_EXTFS 1
#define SUPPORTS_UDP_TUNNEL 0
/* The frame loop counts the CPU's instructions (cpu_check_ticks()). */
#define USE_CPU_EMUL_SERVICES 1
/* The Time Manager runs from the 60 Hz tick (timer.cpp). */
#define PRECISE_TIMING 0

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
#if UINTPTR_MAX > 0xffffffffu
#define SIZEOF_VOID_P 8
#define SIZEOF_LONG 8
#else
#define SIZEOF_VOID_P 4
#define SIZEOF_LONG 4
#endif

typedef off_t loff_t;
typedef char *caddr_t;

/* The FPU's FTENTOX (fpu/mathlib.h): neither newlib nor macOS has
   exp10(). */
#define fp_pow10(x) pow(10.0, (x))

/* Time Manager times (timer_ii.cpp). */
typedef struct timeval tm_time_t;

#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define HOST_FLOAT_FORMAT IEEE_FLOAT_FORMAT

/* UAE CPU data types */
#define uae_s8 int8
#define uae_u8 uint8
#define uae_s16 int16
#define uae_u16 uint16
#define uae_s32 int32
#define uae_u32 uint32
#define uae_s64 int64
#define uae_u64 uint64
typedef uae_u32 uaecptr;

/* Timing functions (timer_bm.cpp) */
extern uint64 GetTicks_usec(void);
extern void Delay_usec(uint64 usec);

/* No threads, so no locks. */
typedef volatile int spinlock_t;
static const spinlock_t SPIN_LOCK_UNLOCKED = 0;
static inline void spin_lock(spinlock_t *lock) { (void) lock; }
static inline void spin_unlock(spinlock_t *lock) { (void) lock; }
static inline int spin_trylock(spinlock_t *lock) { (void) lock; return 1; }

#define unlikely(x)  __builtin_expect(!!(x), 0)

/* ---- Memory access: the Mac is big endian, the Pi little ----------------- */

/* Direct addressing makes every Mac address a host one: MEMBaseDiff plus
   it. The Mac's RAM, ROM and frame buffer are a block of mac_memory_size
   bytes at MEMBaseDiff (main_ii.cpp); the rest of the Mac's 4 GB isn't
   there. On Unix, Basilisk II catches what the Mac reads or writes there
   (a SIGSEGV, skipped with its "ignoresegv" setting, which is on); on the
   Pi it would read or write whatever is at that host address, the
   kernel's memory or a peripheral. So here such an access reads 0, and a
   write is dropped. (The ROM stays writable, as on Unix: Basilisk II keeps
   some of its drivers' data in it.) Host pointers the CPU core reads with
   these functions (its own registers) are outside the Mac's 4 GB. */
extern uintptr MEMBaseDiff;
extern uintptr mac_memory_size;
uae_u32 mac_stray_access(const void *a, int write);

static inline int mac_stray(const void *a)
{
	uintptr offset = (uintptr) a - MEMBaseDiff;
	return offset >= mac_memory_size && offset < 0x100000004ull;
}

/* The 68020 and later read and write words and longs at any address: an
   unaligned access is fine on 64-bit ARM (Normal memory) and on the PC,
   and memcpy() lets the compiler make it one. */
static inline uae_u32 do_get_mem_long(uae_u32 *a)
{
	uae_u32 v;
	if (unlikely(mac_stray(a)))
		return mac_stray_access(a, 0);
	memcpy(&v, a, 4);
	return __builtin_bswap32(v);
}
static inline uae_u32 do_get_mem_word(uae_u16 *a)
{
	uae_u16 v;
	if (unlikely(mac_stray(a)))
		return mac_stray_access(a, 0);
	memcpy(&v, a, 2);
	return __builtin_bswap16(v);
}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v)
{
	if (unlikely(mac_stray(a))) {
		mac_stray_access(a, 1);
		return;
	}
	v = __builtin_bswap32(v);
	memcpy(a, &v, 4);
}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v)
{
	uae_u16 w = __builtin_bswap16((uae_u16) v);
	if (unlikely(mac_stray(a))) {
		mac_stray_access(a, 1);
		return;
	}
	memcpy(a, &w, 2);
}
static inline uae_u32 do_get_mem_byte(uae_u8 *a)
{
	if (unlikely(mac_stray(a)))
		return mac_stray_access(a, 0);
	return *a;
}
static inline void do_put_mem_byte(uae_u8 *a, uae_u32 v)
{
	if (unlikely(mac_stray(a))) {
		mac_stray_access(a, 1);
		return;
	}
	*a = (uae_u8) v;
}
#define HAVE_OPTIMIZED_BYTESWAP_32
static inline uae_u32 do_byteswap_32(uae_u32 v) { return __builtin_bswap32(v); }
#define HAVE_OPTIMIZED_BYTESWAP_16
static inline uae_u32 do_byteswap_16(uae_u32 v) { return __builtin_bswap16((uae_u16) v); }


#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))
#define __inline__ inline
#define CPU_EMU_SIZE 0
#undef NO_INLINE_MEMORY_ACCESS
#undef MD_HAVE_MEM_1_FUNCS
#define ENUMDECL typedef enum
#define ENUMNAME(name) name
#define write_log printf

#define ASM_SYM(a)

#ifndef REGPARAM
# define REGPARAM
#endif
#define REGPARAM2

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

#define ALWAYS_INLINE   inline __attribute__((always_inline))
#define memptr uint32

#endif /* SYSDEPS_H */
