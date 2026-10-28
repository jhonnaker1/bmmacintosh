/*
 * config.h - what Basilisk II's slirp (third_party/macemu/BasiliskII/src/
 * slirp) asks of the system, in place of the config.h Basilisk II's
 * configure writes: for newlib on Circle (the Pi) and for the PC the
 * simulator runs on. Only slirp's sources see it.
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_SLIRP_CONFIG_H
#define BMMAC_SLIRP_CONFIG_H

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_VOID_P __SIZEOF_POINTER__
#define SIZEOF_LONG __SIZEOF_LONG__

#define HAVE_STDINT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#ifndef __circle__
/* newlib on Circle has none: slirp's own (misc.c). */
#define HAVE_INET_ATON 1
#endif

#ifdef __circle__
/* newlib on Circle has no ioctl(): slirp asks how big a waiting UDP
   datagram is (FIONREAD) only to make room for one bigger than its
   buffer, and Circle's are no bigger than an Ethernet frame (1600
   bytes) anyway. */
#define FIONREAD 0x541b
static inline int ioctl(int fd, unsigned long request, int *value)
{
	(void) fd;
	(void) request;
	*value = 0;
	return -1;
}
#else
#define HAVE_SYS_IOCTL_H 1
#ifdef __APPLE__
#define HAVE_SYS_FILIO_H 1   /* FIONREAD */
#endif
#endif

#endif
