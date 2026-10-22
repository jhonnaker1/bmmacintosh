# Third-Party Software

BMMacintosh incorporates software and other materials from third-party projects. Third-party components remain subject to their respective copyright and license terms.

The original copyright, license, and attribution notices supplied with these components are retained with their source where applicable. The notices and licences for code BMMacintosh adapts from other projects are in [`sdcard/licenses/`](../sdcard/licenses/), which becomes the `licenses` folder of the SD card.

## Basilisk II

BMMacintosh's Mac II kernels incorporate [Basilisk II](https://basilisk.cebix.net/), the 68k Macintosh emulator by Christian Bauer and others, under the GNU General Public License version 2 or (at your option) any later version. The version used is [kanjitalk755's macemu](https://github.com/kanjitalk755/macemu), the maintained one, at commit `f532be3d` (the `third_party/macemu` submodule), its `BasiliskII/src`.

Its 68040 CPU core (`uae_cpu_2021`, the one Basilisk II's build uses on ARM) is ARAnyM's, from UAE by Bernd Schmidt and others, with its floating point emulation, under the same licence. BMMacintosh's front end (`src/port/ii/`) follows Basilisk II's Unix front end (`src/Unix/`), and `src/port/ii/extfs_ii.cpp` is its `extfs_unix.cpp` changed for the SD card.

Its notice is in [`sdcard/licenses/basilisk2.txt`](../sdcard/licenses/basilisk2.txt) and the GNU General Public License version 2 in [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

## umac and Musashi

BMMacintosh's Mac Plus kernels incorporate [umac](https://github.com/evansm7/umac), the Mac 128K, 512K and Plus emulator by Matt Evans (the core of [pico-mac](https://github.com/evansm7/pico-mac)), at commit `b62d3c6e` (the `third_party/umac` submodule), under the MIT licence, with its 68000, [Musashi](https://github.com/kstenerud/Musashi) by Karl Stenerud (umac's copy, its `external/Musashi` submodule at `b3144f12`), under the MIT licence, and Musashi's floating point emulation from the SoftFloat package, Release 2b, by John R. Hauser, under its own notice.

umac's disk driver code comes from Basilisk II: the parts in `src/disc.c` and `include/b2_macos_util.h` are under the GNU General Public License version 2 or later, as Basilisk II is; its replacement disk driver (`macsrc/sonydrv.S`, built into `include/sonydrv.h`) is Basilisk II's with Matt Evans's changes, marked "GPLv2". BMMacintosh doesn't build umac's SDL front end or its keyboard tables (`include/keymap.h`, `keymap_sdl.h`), which come from Mini vMac (GPL version 2 only, which BMMacintosh's GPL version 3 can't take): its keyboard code is its own (`src/port/keymap.c`, from Apple's published key codes).

Mini vMac, the other compact Mac emulator, isn't used for the same reason.

The notices are in [`sdcard/licenses/umac.txt`](../sdcard/licenses/umac.txt) and [`sdcard/licenses/musashi.txt`](../sdcard/licenses/musashi.txt).

## zlib

BMMacintosh incorporates [zlib](https://zlib.net/) 1.3.2 (the `third_party/zlib` submodule), to read MAME's zipped ROM sets, under the zlib licence. Its notice is in [`sdcard/licenses/zlib.txt`](../sdcard/licenses/zlib.txt).

## The menu's font

The menu is drawn with the Linux console's lat1-16 font (`src/port/font8x16.h`), from the kbd package by Andries Brouwer, Alexey Gladkov and Paul Gortmaker, under the GNU General Public License version 2 or later, as Circle has it (`lib/font8x16.cpp`).

## BMAtariST

BMMacintosh is based on [BMAtariST](https://github.com/jhonnaker1/bmatarist), the bare metal Atari ST emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network, web UI, menu and test harness code is used here.

## BMApple2

BMAtariST, and so BMMacintosh, is based on [BMApple2](https://github.com/jhonnaker1/bmapple2), the bare metal Apple II emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network, web UI, menu and test harness code is used here.

## BMCoco

BMApple2, and so BMMacintosh, is based on [BMCoco](https://github.com/jhonnaker1/bmcoco), the bare metal Tandy Color Computer emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network, web UI, menu and test harness code is used here.

## BMA800XL

BMCoco, and so BMMacintosh, is a fork of [BMA800XL](https://github.com/aminch/bma800xl), the bare metal Atari 800XL emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network and web UI code is used here, including its Raspberry Pi 4 and Pi 5 display, sound and GPU drivers, merged up to BMA800XL 1.4.2.

## circle-stdlib

BMMacintosh uses the [circle-stdlib project](https://github.com/smuehlst/circle-stdlib), v21, which includes [Circle](https://github.com/rsta2/circle) by Rene Stange and the newlib C library.

See [`third_party/circle-stdlib/`](https://github.com/smuehlst/circle-stdlib/tree/v21) for the version used by BMMacintosh and its accompanying copyright and license information. newlib's notices are in [`sdcard/licenses/newlib.txt`](../sdcard/licenses/newlib.txt) and [`sdcard/licenses/libgloss.txt`](../sdcard/licenses/libgloss.txt).

## GCC Runtime

BMMacintosh links runtime code from the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 15.2.rel1 (GCC), under the GNU General Public License version 3 with the GCC Runtime Library Exception 3.1. The toolchain's GCC licence section is in [`sdcard/licenses/GCC-Runtime.txt`](../sdcard/licenses/GCC-Runtime.txt).

## BMC64

BMMacintosh incorporates source code from the [BMC64 project](https://github.com/randyrossi/bmc64) by Randy Rossi and contributors.

See [BMC64](https://github.com/randyrossi/bmc64) for its source and its accompanying copyright and license information.

BMC64-derived files retain their original license and attribution notices. For files licensed under Apache-2.0, see [the included license](../sdcard/licenses/Apache-2.0.txt); modifications are identified in their headers.

## Raspberry Pi Firmware

BMMacintosh includes the Raspberry Pi boot firmware (`bootcode.bin`, `start.elf`, `fixup.dat`, and for the Raspberry Pi 4 `start4.elf` and `fixup4.dat`) and the Raspberry Pi 4 and Pi 5 device trees from the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware).

Their licences are next to them, in [`sdcard/firmware/`](../sdcard/firmware/) and on the SD card: `LICENCE.broadcom` for the firmware and `COPYING.linux` for the device trees. See the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware/tree/master/boot) for more.

## crt-pi

BMMacintosh uses the crt-pi CRT shader by davej, as included in BMC64, under the GNU General Public License version 2 or later: on the ARM, and compiled for the Raspberry Pi 4 and Pi 5's 3D GPU from [`tools/v3dshader/crt.frag`](../tools/v3dshader/crt.frag). See [`sdcard/licenses/crt-pi.txt`](../sdcard/licenses/crt-pi.txt) and [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

## Mesa

BMMacintosh uses the [Mesa 3D graphics library](https://mesa3d.org/)'s Broadcom V3D drivers in three ways. The adapted files, with their copyright notices, and Mesa's MIT licence are in [`sdcard/licenses/Mesa-MIT.txt`](../sdcard/licenses/Mesa-MIT.txt).

- **Adapted into the Raspberry Pi 4 and Pi 5 kernels** ([`src/v3d/`](../src/v3d/)): control list packet layouts and packing rules, the rendering job, texture and tile layouts, and the GPU's device facts. The V3D 4.2 (Pi 4) material is from Mesa 26.2.4, the V3D 7.1 (Pi 5) material from Mesa 25.2.8.
- **Adapted into the shader compiler tool** ([`tools/v3dshader/`](../tools/v3dshader/)): the shader preparation that Mesa's drivers do before compiling. The tool is built on a PC against Mesa 25.2.8 and is not part of the SD card.
- **As the shader compiler:** the compiled shaders in `src/v3d/` (`shader_*.h`, `shader71_*.h`) are output of Mesa 25.2.8's V3D compiler. Compiling with Mesa doesn't make them Mesa code: the crt-pi shaders are under crt-pi's licence (above), the others are BMMacintosh's.

## Linux

For the 3D GPU of the Raspberry Pi 4 and Pi 5, BMMacintosh adapts parts of the Linux kernel's V3D driver and BCM2835 power domain driver ([`src/v3d/`](../src/v3d/)), from the [Raspberry Pi kernel](https://github.com/raspberrypi/linux/tree/rpi-6.18.y) (`rpi-6.18.y`), under the GNU General Public License version 2 or later. See [`sdcard/licenses/Linux-V3D.txt`](../sdcard/licenses/Linux-V3D.txt) and [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

The Raspberry Pi 4 and Pi 5 display implementation ([`src/kms/pi4/`](../src/kms/pi4/), [`src/kms/pi5/`](../src/kms/pi5/) and the HDMI sound in [`src/av/`](../src/av/)) was independently written using hardware information documented in the Linux vc4 driver (`rpi-6.18.y`). No code from that driver was copied or incorporated.

## Circle ARM stub

For the Raspberry Pi 4, BMMacintosh includes `armstub8-rpi4.bin`, built from [Circle's `boot/armstub`](https://github.com/rsta2/circle/tree/master/boot/armstub) source, under Circle's [GNU General Public License version 3](https://github.com/rsta2/circle/blob/master/COPYING).

## Wi-Fi Firmware

BMMacintosh includes the Wi-Fi firmware for the Raspberry Pi's wireless chip from the [RPi-Distro firmware-nonfree repository](https://github.com/RPi-Distro/firmware-nonfree), downloaded when BMMacintosh is built.

See the `firmware/` folder on the SD card for the Broadcom, Cypress and Synaptics licenses that accompany it.

The Pi 5's files (`brcmfmac43455-sdio.raspberrypi,5-model-b.*`) are copies of the `brcmfmac43455-sdio.*` files under the name Circle looks for on the Pi 5, under the same Cypress license.

## BMRetroSys

[BMRetroSys](https://github.com/drunkeneye/BMRetroSys), a bare metal emulator collection for the Raspberry Pi 4 and 5 that also runs Basilisk II, was a reference for the pitfalls of running Basilisk II on Circle (what newlib lacks, its tick hook). No code from it is used.

## Other Third-Party Components

Additional third-party components and supporting materials may be present in the BMMacintosh source tree. Where such material is accompanied by its own copyright, license, or attribution notices, those notices and terms continue to apply.
