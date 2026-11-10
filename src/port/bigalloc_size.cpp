/*
 * bigalloc_size.cpp - the size of a block of Circle's heap, for
 * src/port/bigalloc.c (read from the block's header, as Circle's own
 * realloc() does).
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <circle/heapallocator.h>
#include <stddef.h>

extern "C" size_t bigalloc_block_size(const void *block)
{
	const THeapBlockHeader *header =
		(const THeapBlockHeader *) ((uintptr) block - sizeof (THeapBlockHeader));
	return header->nSize;
}
