#
# basilisk.mk - the Basilisk II sources the Mac II kernel builds.
#
# They come unmodified from the third_party/macemu submodule
# (kanjitalk755's macemu, BasiliskII/src): the emulator's portable part
# with the CPU core it uses on ARM (uae_cpu_2021: the UAE 68040 with
# direct addressing, the "uae" FPU, no JIT), and the dummy serial,
# Ethernet, SCSI and clipboard drivers. Not the Unix, SDL or other front
# ends, SLIRP or the JIT; src/port/ii/ is BMMacintosh's front end
# (B2_PORT_SRCS), with its own ExtFS for the SD card (from the Unix one).
#
# The CPU core's tables and opcode handlers are written by two programs
# built for the computer doing the build, build68k and gencpu, as Basilisk
# II's build does (B2_GEN_SRCS; see the Makefile). cpuemu.cpp is compiled
# in eight parts (PART_1 to PART_8), as there.
#
# SRCROOT is where the sources are from where make runs ("" for the Pi
# kernels, "../../" for tools/hostsim).
#
# When updating macemu, check BasiliskII/src/Unix/Makefile.in and
# configure.ac for added or removed files (see docs/UPDATING.md).
#

B2 := $(SRCROOT)third_party/macemu/BasiliskII/src

B2_CORE_SRCS = \
	main.cpp \
	prefs.cpp \
	prefs_items.cpp \
	sony.cpp \
	disk.cpp \
	cdrom.cpp \
	scsi.cpp \
	video.cpp \
	audio.cpp \
	ether.cpp \
	serial.cpp \
	extfs.cpp \
	adb.cpp \
	timer.cpp \
	rom_patches.cpp \
	rsrc_patches.cpp \
	emul_op.cpp \
	macos_util.cpp \
	xpram.cpp \
	user_strings.cpp \
	slot_rom.cpp \
	dummy/clip_dummy.cpp \
	dummy/ether_dummy.cpp \
	dummy/serial_dummy.cpp \
	dummy/scsi_dummy.cpp \
	dummy/prefs_editor_dummy.cpp \
	$(B2_UAE)/basilisk_glue.cpp \
	$(B2_UAE)/newcpu.cpp \
	$(B2_UAE)/readcpu.cpp \
	$(B2_UAE)/fpu/fpu_uae.cpp

# The CPU core: uae_cpu_2021 (as Basilisk II's build has it on ARM), or
# uae_cpu, the older one.
B2_UAE ?= uae_cpu_2021

# Written by build68k (cpudefs.cpp) and gencpu (the rest).
ifeq ($(B2_UAE),uae_cpu_2021)
B2_CORE_SRCS += $(B2_UAE)/memory.cpp
B2_GEN_SRCS = cpudefs.cpp cpustbl.cpp cpufunctbl.cpp
B2_UAE_DEFINES = -DUPDATE_UAE
else
B2_CORE_SRCS += $(B2_UAE)/memory.cpp
B2_GEN_SRCS = cpudefs.cpp cpustbl.cpp
B2_UAE_DEFINES =
endif
B2_CPUEMU_PARTS = 1 2 3 4 5 6 7 8

B2_PORT_SRCS = \
	main_ii.cpp \
	video_ii.cpp \
	audio_ii.cpp \
	sys_ii.cpp \
	timer_ii.cpp \
	prefs_ii.cpp \
	menu_ii.cpp \
	extfs_ii.cpp

# Basilisk II's options for ARM (Unix/configure.ac): the newer UAE core
# (UPDATE_UAE) and its FPU. Its sources aren't all warning free: -w.
B2_DEFINES = $(B2_UAE_DEFINES) -DFPU_UAE
B2_INCLUDES = -I $(SRCROOT)src/port/ii -I $(B2)/$(B2_UAE) -I $(B2)/include
B2_CXXFLAGS = -std=gnu++14 -w $(B2_DEFINES)
