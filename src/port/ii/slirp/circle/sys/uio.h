/*
 * sys/uio.h - what slirp takes from it, which newlib on Circle doesn't
 * have: struct iovec (slirp reads and writes its sockets with recv() and
 * send(), not readv() and writev()). Also <sys/socket.h>, which newlib's
 * <arpa/inet.h>, next in slirp.h, needs first. Only the Pi's slirp sees
 * this folder.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_SYS_UIO_H
#define BMMAC_SYS_UIO_H

#include <stddef.h>
#include <sys/socket.h>

struct iovec {
	void *iov_base;
	size_t iov_len;
};

#endif
