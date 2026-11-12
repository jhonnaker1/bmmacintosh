/*
 * spcflags.hpp - the PowerPC interpreter's special flags, for BMMacintosh.
 *
 * SheepShaver's own (kpx_cpu/src/cpu/spcflags.hpp), found first on the
 * include path, with one change: empty(), which the interpreter asks after
 * every block of instructions it runs (and after every instruction when it
 * runs one routine inside another, a 68k or Mac OS call), also counts
 * them down. When the count runs out it calls ppc_check_ticks()
 * (main_ppc.cpp), which looks at the clock and runs BMMacintosh's frame
 * loop when a frame is due. SheepShaver's
 * Unix port uses threads and signals for this instead (its 60 Hz tick
 * thread raises the interrupt).
 *
 * Kheperix (C) 2003-2005 Gwenole Beauchesne, GPL-2.0-or-later.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef SPCFLAGS_H
#define SPCFLAGS_H

/* main_ppc.cpp: blocks left before the next call, and the call. */
extern int32 ppc_ticks;
void ppc_check_ticks(void);

enum {
	SPCFLAG_CPU_EXEC_RETURN			= 1 << 0,	// Return from emulation loop
	SPCFLAG_CPU_TRIGGER_INTERRUPT	= 1 << 1,	// Trigger user interrupt
	SPCFLAG_CPU_HANDLE_INTERRUPT	= 1 << 2,	// Call user interrupt handler
	SPCFLAG_CPU_ENTER_MON			= 1 << 3,	// Enter cxmon
	SPCFLAG_JIT_EXEC_RETURN			= 1 << 4,	// Return from compiled code
};

class basic_spcflags
{
	uint32 mask;

public:

	basic_spcflags()
		{ init(); }

	void init()
		{ mask = 0; }

	/* The interpreter's check: the count down too (see above). */
	bool empty() const
	{
		if (__builtin_expect(--ppc_ticks <= 0, 0))
			ppc_check_ticks();
		return mask == 0;
	}

	bool test(uint32 v) const
		{ return (mask & v); }

	void init(uint32 v)
		{ mask = v; }

	uint32 get() const
		{ return mask; }

	void set(uint32 v)
		{ mask |= v; }

	void clear(uint32 v)
		{ mask &= ~v; }
};

#endif /* SPCFLAGS_H */
