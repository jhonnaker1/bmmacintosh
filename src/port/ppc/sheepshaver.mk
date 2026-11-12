#
# sheepshaver.mk - the SheepShaver sources the Power Macintosh kernel
# builds (the Pi 5 only, for now).
#
# They come unmodified from the third_party/macemu submodule
# (kanjitalk755's macemu, SheepShaver/src, some of whose files are links
# to Basilisk II's): the emulator's portable part with its PowerPC
# interpreter (kpx_cpu, no JIT: SheepShaver has none for ARM), the dummy
# serial, SCSI and clipboard drivers, and Basilisk II's slirp for the
# network (basilisk.mk's B2_SLIRP_SRCS). Not the Unix, SDL or other front ends. src/port/ppc/ is
# BMMacintosh's front end (SS_PORT_SRCS); the disks, the shared folder, the
# clocks and the network are the Mac II's (SS_II_SRCS, src/port/ii/, built
# with BMMAC_PPC_BUILD, which gives them src/port/ppc/sysdeps_ppc.h).
#
# src/port/ppc/kpx/cpu/ holds BMMacintosh's vm.hpp (the Mac's memory) and
# spcflags.hpp (the frame loop's count), found before kpx_cpu's own.
#
# SRCROOT is where the sources are from where make runs ("" for the Pi
# kernels, "../../" for tools/hostsim).
#

SS := $(SRCROOT)third_party/macemu/SheepShaver/src

SS_CORE_SRCS = \
	main.cpp \
	prefs.cpp \
	prefs_items.cpp \
	rom_patches.cpp \
	rsrc_patches.cpp \
	emul_op.cpp \
	name_registry.cpp \
	macos_util.cpp \
	timer.cpp \
	xpram.cpp \
	adb.cpp \
	sony.cpp \
	disk.cpp \
	cdrom.cpp \
	scsi.cpp \
	gfxaccel.cpp \
	video.cpp \
	audio.cpp \
	ether.cpp \
	thunks.cpp \
	serial.cpp \
	extfs.cpp \
	user_strings.cpp \
	dummy/clip_dummy.cpp \
	dummy/serial_dummy.cpp \
	dummy/scsi_dummy.cpp \
	dummy/prefs_editor_dummy.cpp

# The PowerPC interpreter (Unix/configure.ac's CPUSRCS without the JIT).
SS_CPU_SRCS = \
	kpx_cpu/src/mathlib/ieeefp.cpp \
	kpx_cpu/src/mathlib/mathlib.cpp \
	kpx_cpu/src/cpu/ppc/ppc-cpu.cpp \
	kpx_cpu/src/cpu/ppc/ppc-decode.cpp \
	kpx_cpu/src/cpu/ppc/ppc-execute.cpp \
	kpx_cpu/src/cpu/ppc/ppc-translate.cpp \
	kpx_cpu/src/utils/utils-cpuinfo.cpp \
	kpx_cpu/sheepshaver_glue.cpp
SS_CPU_C_SRCS = kpx_cpu/ppc-dis.c

SS_PORT_SRCS = \
	main_ppc.cpp \
	video_ppc.cpp \
	prefs_ppc.cpp

# The Mac II's, shared.
SS_II_SRCS = \
	timer_ii.cpp \
	sys_ii.cpp \
	extfs_ii.cpp \
	audio_ii.cpp \
	menu_ii.cpp \
	ether_ii.cpp

SS_DEFINES = -DBMMAC_PPC_BUILD
SS_INCLUDES = -I $(SRCROOT)src/port/ppc/kpx -I $(SRCROOT)src/port/ppc -I $(SRCROOT)src/port/ii \
	-I $(SS)/include -I $(SS)/kpx_cpu/include -I $(SS)/kpx_cpu/src -I $(SS)/CrossPlatform \
	-I $(SS)/slirp
SS_CXXFLAGS = -std=gnu++14 -w $(SS_DEFINES)
