/*
 * circle_sockets.h - the BSD socket calls of Basilisk II's slirp, on the
 * Pi, renamed to src/slirp_sockets.cpp's, on Circle's own sockets.
 *
 * newlib's socket calls on Circle hold the lock on the table of open files
 * while they wait (connect() for seconds), so a file opened meanwhile
 * would wait too, and the Pi with it (see host_tcp_connect() in host.h);
 * they can't connect without waiting either, as slirp asks. These can:
 * connect() returns EINPROGRESS and a Circle task connects. The sockets
 * are numbered apart from files (0 to 31), and only slirp's sources see
 * the renames (the Makefile includes this before anything else in them).
 *
 * This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMMAC_CIRCLE_SOCKETS_H
#define BMMAC_CIRCLE_SOCKETS_H

#define socket bmslirp_socket
#define connect bmslirp_connect
#define bind bmslirp_bind
#define listen bmslirp_listen
#define accept bmslirp_accept
#define send bmslirp_send
#define recv bmslirp_recv
#define sendto bmslirp_sendto
#define recvfrom bmslirp_recvfrom
#define select bmslirp_select
#define close bmslirp_close
#define shutdown bmslirp_shutdown
#define getsockname bmslirp_getsockname
#define setsockopt bmslirp_setsockopt
#define fcntl bmslirp_fcntl
#define gethostname bmslirp_gethostname
#define gethostbyname bmslirp_gethostbyname
#define inet_ntoa bmslirp_inet_ntoa
#define inet_addr bmslirp_inet_addr
/* slirp's "exec" redirections (misc.c), never set up: programs to start,
   on a pty. */
#define getuid bmslirp_getuid
#define execvp bmslirp_execvp
#define chown bmslirp_chown
#define chmod bmslirp_chmod
/* /etc/resolv.conf: the Pi's DNS server (slirp.c's get_dns_addr()). */
#define fopen bmslirp_fopen

/* What newlib on Circle leaves out: struct hostent and gethostbyname()
   (<netdb.h>), inet_ntoa() (declared, not there). */
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};
#define h_addr h_addr_list[0]

int bmslirp_gethostname(char *name, size_t length);
struct hostent *bmslirp_gethostbyname(const char *name);
char *bmslirp_inet_ntoa(struct in_addr address);

#endif
