# Third-Party Software

BMAtariST incorporates software and other materials from third-party projects. Third-party components remain subject to their respective copyright and license terms.

The original copyright, license, and attribution notices supplied with these components are retained with their source where applicable. The notices and licences for code BMAtariST adapts from other projects are in [`sdcard/licenses/`](../sdcard/licenses/), which becomes the `licenses` folder of the SD card.

## Hatari

BMAtariST incorporates [Hatari](https://www.hatari-emu.org/), the Atari ST, STE, TT and Falcon emulator by the Hatari team, under the GNU General Public License version 2 or (at your option) any later version. The version used is a commit of its master branch after the 2.6.1 release (the `third_party/hatari` submodule, from `https://github.com/hatari/hatari.git`), which has its user interfaces split out of the emulation core.

Hatari's 68000-68060 CPU core comes from WinUAE and UAE, and its floating point emulation from the SoftFloat package (under its own notice, in `src/cpu/softfloat/`). BMAtariST's menu uses the Atari character set from Hatari's font (`src/gui-sdl/font10x16.h`). BMAtariST's front end (`src/port/`) has the same functions as Hatari's libretro front end (`src/retro/`), which it follows.

Its notice is in [`sdcard/licenses/hatari.txt`](../sdcard/licenses/hatari.txt) and the GNU General Public License version 2 in [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

## EmuTOS

The SD card includes [EmuTOS](https://emutos.sourceforge.io/) 1.4 (`tos/etos1024k.img`, from the release on SourceForge, fetched by `make_emutos.sh`), a free TOS, under the GNU General Public License version 2. It is a separate program the emulated ST runs, not part of the kernels. Its licence is [`licenses/emutos.txt`](../sdcard/licenses/) on the card; its source is at https://github.com/emutos/emutos (tag `VERSION_1_4`).

## zlib

BMAtariST incorporates [zlib](https://zlib.net/) 1.3.2 (the `third_party/zlib` submodule), for zipped and gzipped disk images, under the zlib licence. Its notice is in [`sdcard/licenses/zlib.txt`](../sdcard/licenses/zlib.txt).

## BMApple2

BMAtariST is based on [BMApple2](https://github.com/jhonnaker1/bmapple2), the bare metal Apple II emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network, web UI, menu and test harness code is used here.

## BMCoco

BMApple2, and so BMAtariST, is based on [BMCoco](https://github.com/jhonnaker1/bmcoco), the bare metal Tandy Color Computer emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network, web UI, menu and test harness code is used here.

## BMA800XL

BMCoco, and so BMAtariST, is a fork of [BMA800XL](https://github.com/aminch/bma800xl), the bare metal Atari 800XL emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network and web UI code is used here, including its Raspberry Pi 4 and Pi 5 display, sound and GPU drivers, merged up to BMA800XL 1.4.2.

## circle-stdlib

BMAtariST uses the [circle-stdlib project](https://github.com/smuehlst/circle-stdlib), v21, which includes [Circle](https://github.com/rsta2/circle) by Rene Stange and the newlib C library.

See [`third_party/circle-stdlib/`](https://github.com/smuehlst/circle-stdlib/tree/v21) for the version used by BMAtariST and its accompanying copyright and license information. newlib's notices are in [`sdcard/licenses/newlib.txt`](../sdcard/licenses/newlib.txt) and [`sdcard/licenses/libgloss.txt`](../sdcard/licenses/libgloss.txt).

## GCC Runtime

BMAtariST links runtime code from the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 15.2.rel1 (GCC), under the GNU General Public License version 3 with the GCC Runtime Library Exception 3.1. The toolchain's GCC licence section is in [`sdcard/licenses/GCC-Runtime.txt`](../sdcard/licenses/GCC-Runtime.txt).

## BMC64

BMAtariST incorporates source code from the [BMC64 project](https://github.com/randyrossi/bmc64) by Randy Rossi and contributors.

See [BMC64](https://github.com/randyrossi/bmc64) for its source and its accompanying copyright and license information.

BMC64-derived files retain their original license and attribution notices. For files licensed under Apache-2.0, see [the included license](../sdcard/licenses/Apache-2.0.txt); modifications are identified in their headers.

## Raspberry Pi Firmware

BMAtariST includes the Raspberry Pi boot firmware (`bootcode.bin`, `start.elf`, `fixup.dat`, and for the Raspberry Pi 4 `start4.elf` and `fixup4.dat`) and the Raspberry Pi 4 and Pi 5 device trees from the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware).

Their licences are next to them, in [`sdcard/firmware/`](../sdcard/firmware/) and on the SD card: `LICENCE.broadcom` for the firmware and `COPYING.linux` for the device trees. See the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware/tree/master/boot) for more.

## crt-pi

BMAtariST uses the crt-pi CRT shader by davej, as included in BMC64, under the GNU General Public License version 2 or later: on the ARM, and compiled for the Raspberry Pi 4 and Pi 5's 3D GPU from [`tools/v3dshader/crt.frag`](../tools/v3dshader/crt.frag). See [`sdcard/licenses/crt-pi.txt`](../sdcard/licenses/crt-pi.txt) and [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

## Mesa

BMAtariST uses the [Mesa 3D graphics library](https://mesa3d.org/)'s Broadcom V3D drivers in three ways. The adapted files, with their copyright notices, and Mesa's MIT licence are in [`sdcard/licenses/Mesa-MIT.txt`](../sdcard/licenses/Mesa-MIT.txt).

- **Adapted into the Raspberry Pi 4 and Pi 5 kernels** ([`src/v3d/`](../src/v3d/)): control list packet layouts and packing rules, the rendering job, texture and tile layouts, and the GPU's device facts. The V3D 4.2 (Pi 4) material is from Mesa 26.2.4, the V3D 7.1 (Pi 5) material from Mesa 25.2.8.
- **Adapted into the shader compiler tool** ([`tools/v3dshader/`](../tools/v3dshader/)): the shader preparation that Mesa's drivers do before compiling. The tool is built on a PC against Mesa 25.2.8 and is not part of the SD card.
- **As the shader compiler:** the compiled shaders in `src/v3d/` (`shader_*.h`, `shader71_*.h`) are output of Mesa 25.2.8's V3D compiler. Compiling with Mesa doesn't make them Mesa code: the crt-pi shaders are under crt-pi's licence (above), the others are BMAtariST's.

## Linux

For the 3D GPU of the Raspberry Pi 4 and Pi 5, BMAtariST adapts parts of the Linux kernel's V3D driver and BCM2835 power domain driver ([`src/v3d/`](../src/v3d/)), from the [Raspberry Pi kernel](https://github.com/raspberrypi/linux/tree/rpi-6.18.y) (`rpi-6.18.y`), under the GNU General Public License version 2 or later. See [`sdcard/licenses/Linux-V3D.txt`](../sdcard/licenses/Linux-V3D.txt) and [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

The Raspberry Pi 4 and Pi 5 display implementation ([`src/kms/pi4/`](../src/kms/pi4/), [`src/kms/pi5/`](../src/kms/pi5/) and the HDMI sound in [`src/av/`](../src/av/)) was independently written using hardware information documented in the Linux vc4 driver (`rpi-6.18.y`). No code from that driver was copied or incorporated.

## Circle ARM stub

For the Raspberry Pi 4, BMAtariST includes `armstub8-rpi4.bin`, built from [Circle's `boot/armstub`](https://github.com/rsta2/circle/tree/master/boot/armstub) source, under Circle's [GNU General Public License version 3](https://github.com/rsta2/circle/blob/master/COPYING).

## Wi-Fi Firmware

BMAtariST includes the Wi-Fi firmware for the Raspberry Pi's wireless chip from the [RPi-Distro firmware-nonfree repository](https://github.com/RPi-Distro/firmware-nonfree), downloaded when BMAtariST is built.

See the `firmware/` folder on the SD card for the Broadcom, Cypress and Synaptics licenses that accompany it.

The Pi 5's files (`brcmfmac43455-sdio.raspberrypi,5-model-b.*`) are copies of the `brcmfmac43455-sdio.*` files under the name Circle looks for on the Pi 5, under the same Cypress license.

## BMRetroSys

[BMRetroSys](https://github.com/drunkeneye/BMRetroSys), a bare metal emulator collection for the Raspberry Pi 4 and 5 that also runs Hatari, was a reference for the pitfalls of running Hatari on Circle (restarting the CPU core after a reset, what newlib lacks). No code from it is used.

## Other Third-Party Components

Additional third-party components and supporting materials may be present in the BMAtariST source tree. Where such material is accompanied by its own copyright, license, or attribution notices, those notices and terms continue to apply.
