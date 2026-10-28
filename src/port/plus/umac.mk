#
# umac.mk - the umac and Musashi sources the Mac Plus kernel builds.
#
# They come unmodified from the third_party/umac submodule (Musashi is
# umac's own submodule, external/Musashi): umac's emulator (src/) without
# its SDL front end (unix_main.c) and the keyboard tables it takes from
# Mini vMac (include/keymap*.h, GPL 2; BMMacintosh has its own, keymap.c),
# with BMMacintosh's disk driver (src/port/plus/sonydrv.h) in place of
# umac's (include/sonydrv.h, whose changes are marked "GPLv2"), and
# Musashi's 68000 with the opcode handlers its m68kmake writes
# (UMAC_GEN_SRCS; see the Makefile). src/port/plus/ is the front end.
#

# SRCROOT is where the sources are from where make runs ("" for the Pi
# kernels, "../../" for tools/hostsim).

UMAC := $(SRCROOT)third_party/umac
MUSASHI := $(UMAC)/external/Musashi

UMAC_CORE_SRCS = main.c via.c scc.c rom.c disc.c
MUSASHI_SRCS = m68kcpu.c m68kdasm.c softfloat/softfloat.c
# Written by m68kmake, from m68k_in.c.
UMAC_GEN_SRCS = m68kops.c

UMAC_PORT_SRCS = main_plus.c rtc_plus.c

# umac's own build's options (its Makefile), with the RAM size a variable
# (umac_prefix.h) and Musashi's configuration umac's.
UMAC_CFLAGS = -std=gnu11 -w -DMUSASHI_CNF=\"$(abspath $(UMAC)/include/m68kconf.h)\" \
              -include $(SRCROOT)src/port/plus/umac_prefix.h
# src/port/plus first: BMMacintosh's disk driver (sonydrv.h) in place of
# umac's, whose changes are marked "GPLv2".
UMAC_INCLUDES = -I $(SRCROOT)src/port/plus -I $(UMAC)/include -I $(MUSASHI)

# umac's main.c resets the Mac when the Mac ejects its disk; with no disk
# it would start again and again. Its umac_disc_ejected() is renamed away
# here, and main_plus.c's is used instead. And its keyboard answers the
# Mac's Inquiry but not Instant, which the keypad and arrow keys need: its
# call to via_init() goes to main_plus.c's plus_via_init(), which answers
# Instant before passing the rest on.
UMAC_MAIN_CFLAGS = -Dumac_disc_ejected=umac_main_disc_ejected -Dvia_init=plus_via_init
