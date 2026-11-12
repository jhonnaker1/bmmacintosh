/*
 * vm.hpp - the PowerPC Mac's memory, for BMMacintosh.
 *
 * SheepShaver's own (kpx_cpu/src/cpu/vm.hpp), found first on the include
 * path, for direct addressing: a Mac address plus mac_base is the host
 * address. main_ppc.cpp lays the Mac's memory out as one block from Mac
 * address 0, mac_span bytes long:
 *
 *   RAM (at 0), the ROM area, SheepShaver's data, the frame buffer, and a
 *   page of zeros last (SheepMem::ZeroPage()).
 *
 * Mac OS keeps its kernel data at 0x68ffe000 (and 0x5fffe000 for some
 * ROMs): an array of its own (gKernelData, as SheepShaver's MEM_BULK has
 * it). Everything else the Mac reads or writes (hardware it probes, which
 * SheepShaver's Unix port skips in its SIGSEGV handler) goes to a page of
 * its own: writes there are lost, and reads find zeros (not what was
 * written: Mac OS 7.6 and 8.1 write to an address and read it back to find
 * memory and hardware). On the Pi a stray access would otherwise reach
 * whatever is at that host address. Writes to the zero page are lost too,
 * and so are writes to the ROM area once Mac OS starts (mac_rom_len), as
 * SheepShaver's Unix port write-protects them.
 *
 * Accesses may be unaligned, as on the PowerPC: memcpy() makes them one
 * load or store where the host allows it (64-bit ARM, and the PC).
 *
 * Kheperix (C) 2003-2005 Gwenole Beauchesne, GPL-2.0-or-later.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef VM_H
#define VM_H

#include <string.h>

/* ---- The host side: big endian values at host addresses ------------------ */

static inline uint32 vm_do_read_memory_1(uint8 *a) { return *a; }
static inline void vm_do_write_memory_1(uint8 *a, uint32 v) { *a = (uint8) v; }

static inline uint32 vm_do_read_memory_2(uint16 *a)
{
	uint16 v;
	memcpy(&v, a, 2);
	return __builtin_bswap16(v);
}
static inline void vm_do_write_memory_2(uint16 *a, uint32 v)
{
	uint16 w = __builtin_bswap16((uint16) v);
	memcpy(a, &w, 2);
}
static inline uint32 vm_do_read_memory_4(uint32 *a)
{
	uint32 v;
	memcpy(&v, a, 4);
	return __builtin_bswap32(v);
}
static inline void vm_do_write_memory_4(uint32 *a, uint32 v)
{
	v = __builtin_bswap32(v);
	memcpy(a, &v, 4);
}
static inline uint64 vm_do_read_memory_8(uint64 *a)
{
	uint64 v;
	memcpy(&v, a, 8);
	return __builtin_bswap64(v);
}
static inline void vm_do_write_memory_8(uint64 *a, uint64 v)
{
	v = __builtin_bswap64(v);
	memcpy(a, &v, 8);
}
static inline uint32 vm_do_read_memory_2_reversed(uint16 *a)
{
	uint16 v;
	memcpy(&v, a, 2);
	return v;
}
static inline void vm_do_write_memory_2_reversed(uint16 *a, uint32 v)
{
	uint16 w = (uint16) v;
	memcpy(a, &w, 2);
}
static inline uint32 vm_do_read_memory_4_reversed(uint32 *a)
{
	uint32 v;
	memcpy(&v, a, 4);
	return v;
}
static inline void vm_do_write_memory_4_reversed(uint32 *a, uint32 v)
{
	memcpy(a, &v, 4);
}

/* ---- Mac addresses ------------------------------------------------------- */

typedef uintptr vm_addr_t;

/* main_ppc.cpp: the block's host address and length (up to and not
   counting the zero page, which is the block's last page). */
extern uintptr mac_base;
extern uint32 mac_span;
/* The ROM area, write-protected while mac_rom_len isn't 0. */
extern uint32 mac_rom_lo, mac_rom_len;
/* Any other Mac address (the kernel data, the zero page, or nothing):
   main_ppc.cpp. WRITE: the access is a write (lost on the zero page and
   where the Mac has no memory). */
uint8 *mac_other_address(uint32 addr, int write);

/* SheepShaver's name for mac_base (Host2MacAddr()). */
#define VMBaseDiff mac_base

static inline uint8 *vm_do_get_real_address(vm_addr_t addr)
{
	uint32 a = (uint32) addr;
	if (__builtin_expect(a < mac_span, 1))
		return (uint8 *) (mac_base + a);
	return mac_other_address(a, 0);
}
static inline uint8 *vm_do_get_real_address_w(vm_addr_t addr)
{
	uint32 a = (uint32) addr;
	if (__builtin_expect(a < mac_span && a - mac_rom_lo >= mac_rom_len, 1))
		return (uint8 *) (mac_base + a);
	return mac_other_address(a, 1);
}
static inline vm_addr_t vm_do_get_virtual_address(uint8 *addr)
{
	return (uint32) ((uintptr) addr - mac_base);
}

static inline uint32 vm_read_memory_1(vm_addr_t addr)
{
	return vm_do_read_memory_1(vm_do_get_real_address(addr));
}
static inline uint32 vm_read_memory_2(vm_addr_t addr)
{
	return vm_do_read_memory_2((uint16 *) vm_do_get_real_address(addr));
}
static inline uint32 vm_read_memory_4(vm_addr_t addr)
{
	return vm_do_read_memory_4((uint32 *) vm_do_get_real_address(addr));
}
static inline uint64 vm_read_memory_8(vm_addr_t addr)
{
	return vm_do_read_memory_8((uint64 *) vm_do_get_real_address(addr));
}
#define vm_read_memory_1_reversed vm_read_memory_1
static inline uint32 vm_read_memory_2_reversed(vm_addr_t addr)
{
	return vm_do_read_memory_2_reversed((uint16 *) vm_do_get_real_address(addr));
}
static inline uint32 vm_read_memory_4_reversed(vm_addr_t addr)
{
	return vm_do_read_memory_4_reversed((uint32 *) vm_do_get_real_address(addr));
}
static inline void vm_write_memory_1(vm_addr_t addr, uint32 value)
{
	vm_do_write_memory_1(vm_do_get_real_address_w(addr), value);
}
static inline void vm_write_memory_2(vm_addr_t addr, uint32 value)
{
	vm_do_write_memory_2((uint16 *) vm_do_get_real_address_w(addr), value);
}
static inline void vm_write_memory_4(vm_addr_t addr, uint32 value)
{
	vm_do_write_memory_4((uint32 *) vm_do_get_real_address_w(addr), value);
}
static inline void vm_write_memory_8(vm_addr_t addr, uint64 value)
{
	vm_do_write_memory_8((uint64 *) vm_do_get_real_address_w(addr), value);
}
#define vm_write_memory_1_reversed vm_write_memory_1
static inline void vm_write_memory_2_reversed(vm_addr_t addr, uint32 value)
{
	vm_do_write_memory_2_reversed((uint16 *) vm_do_get_real_address_w(addr), value);
}
static inline void vm_write_memory_4_reversed(vm_addr_t addr, uint32 value)
{
	vm_do_write_memory_4_reversed((uint32 *) vm_do_get_real_address_w(addr), value);
}

/* Blocks: within the Mac's memory block (or the kernel data), which is
   where SheepShaver copies them. */
static inline void *vm_memset(vm_addr_t addr, int c, size_t n)
{
	return memset(vm_do_get_real_address_w(addr), c, n);
}
#ifdef __cplusplus
static inline void *vm_memcpy(void *dest, vm_addr_t src, size_t n)
{
	return memcpy(dest, vm_do_get_real_address(src), n);
}
static inline void *vm_memcpy(vm_addr_t dest, const void *src, size_t n)
{
	return memcpy(vm_do_get_real_address_w(dest), src, n);
}
#endif
static inline void *vm_memcpy(vm_addr_t dest, vm_addr_t src, size_t n)
{
	return memmove(vm_do_get_real_address_w(dest), vm_do_get_real_address(src), n);
}

#endif /* VM_H */
