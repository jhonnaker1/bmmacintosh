#!/bin/bash
#
# Boots a BMMacintosh Pi 4 kernel in QEMU's raspi4b for a while and prints
# what it sent to the serial port, for faults in the kernel's start up
# (before the SD card), which the PC simulator can't show: it runs the
# emulators, not Circle. From BMAmiga, which found its first Pi 5's black
# screen this way (an assert in a static constructor, before the log was
# set up).
#
#   tools/qemu/run_qemu.sh [KERNEL] [SECONDS]
#
# KERNEL is macii8-rpi4.img or macplus8-rpi4.img (./build_sdcard.sh pi4
# stages them; default the Mac II's in build/sdcard). Logging goes to the
# serial port (enable_logging=uart, on QEMU's command line, which Circle
# reads as cmdline.txt). A working kernel prints Circle's first lines and
# BMMacintosh's, then waits on hardware QEMU's model hasn't (its SD card,
# as Circle drives it, and the display's HVS): QEMU tests the start, not
# the card, the picture, USB or network. To see where a kernel stops, run
# QEMU with -monitor telnet:127.0.0.1:45454,server,nowait, ask the monitor
# for "info registers", and look the PC and the frame pointer chain (X29)
# up in the kernel's .elf with aarch64-none-elf-addr2line.
#
# Needs qemu-system-aarch64 (brew install qemu); the device tree comes
# from the SD card build or Circle's boot files.
#
# This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
# the Raspberry Pi. See LICENSE for licensing details.
#

set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
KERNEL=${1:-$ROOT/build/sdcard/macii8-rpi4.img}
SECONDS_TO_RUN=${2:-20}
if [ ! -f "$KERNEL" ]; then
    echo "No kernel $KERNEL: ./build_sdcard.sh pi4 builds it." >&2
    exit 1
fi
command -v qemu-system-aarch64 > /dev/null || {
    echo "qemu-system-aarch64 isn't installed (brew install qemu)." >&2
    exit 1
}
DTB=$ROOT/build/sdcard/bcm2711-rpi-4-b.dtb
if [ ! -f "$DTB" ]; then
    DTB=$ROOT/third_party/circle-stdlib/libs/circle/boot/bcm2711-rpi-4-b.dtb
    [ -f "$DTB" ] || make -s -C "$ROOT/third_party/circle-stdlib/libs/circle/boot" > /dev/null
fi
OUT=${TMPDIR:-/tmp}/bmmac-qemu.$$
mkdir -p "$OUT"
qemu-system-aarch64 -M raspi4b -kernel "$KERNEL" -dtb "$DTB" \
    -append "enable_logging=uart" -serial file:"$OUT/serial.txt" \
    -display none -monitor none -daemonize -pidfile "$OUT/qemu.pid" 2>/dev/null || exit 1
perl -e "select(undef, undef, undef, $SECONDS_TO_RUN)"
kill "$(cat "$OUT/qemu.pid")" 2>/dev/null
cat "$OUT/serial.txt"
rm -rf "$OUT"
