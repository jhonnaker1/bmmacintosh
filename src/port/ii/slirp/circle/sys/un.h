/*
 * sys/un.h - for slirp, which newlib on Circle doesn't have: the Unix
 * domain socket address of slirp's "exec" redirections, which BMMacintosh
 * doesn't set up. Only the Pi's slirp sees this folder.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_SYS_UN_H
#define BMMAC_SYS_UN_H

#include <sys/socket.h>

struct sockaddr_un {
	sa_family_t sun_family;
	char sun_path[108];
};

#endif
