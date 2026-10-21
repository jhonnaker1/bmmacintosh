/*
 * config.h - Hatari's build options for BMAtariST.
 *
 * Hatari's CMake build normally writes this file (from cmake/config-cmake.h).
 * BMAtariST builds Hatari with its own makefile (src/port/hatari.mk), so the
 * options are set here by hand: every machine Hatari has (the DSP for the
 * Falcon included), no host libraries but zlib (third_party/zlib), and the
 * user interface, video, sound and joystick modules from src/port/.
 *
 * The same options are used on the Pi (circle-newlib) and in the PC test
 * harness (tools/hostsim), so that both build the same code. Only what
 * newlib has is turned on; Hatari has its own fallbacks for the rest
 * (scandir.c, for one).
 *
 * This file is part of BMAtariST, a bare metal Atari ST emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMATARIST_HATARI_CONFIG_H_
#define BMATARIST_HATARI_CONFIG_H_

/* zlib (third_party/zlib): zipped and gzipped disk images. */
#define HAVE_LIBZ 1
#define HAVE_ZLIB_H 1

/* newlib's headers */
#define HAVE_STRINGS_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMES_H 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_SETENV 1
#define HAVE_UTIME_H 1

/* newlib has neither, nor the d_reclen Hatari's own scandir.c needs: on the
   Pi they are BMAtariST's (posix_extra.c); the PC's C library has them. */
#define HAVE_SCANDIR 1
#define HAVE_ALPHASORT 1

/* Where Hatari would look for a system wide hatari.cfg. BMAtariST passes
   its settings itself (src/port/platform.c). */
#define CONFDIR "/"
#define BIN2DATADIR "."

/* The Falcon's DSP. */
#define ENABLE_DSP_EMU 1

/* Trace logs (--trace) cost a little speed on every chip access; BMAtariST
   has no command line to turn them on. */
#undef ENABLE_TRACING

#endif /* BMATARIST_HATARI_CONFIG_H_ */
