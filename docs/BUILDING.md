# Build Instructions

BMMacintosh builds on Linux (Debian, Ubuntu, WSL) and macOS.

## Prerequisites

On Debian or Ubuntu:

```
sudo apt-get install build-essential git python3 curl xz-utils
```

On macOS (Apple silicon or Intel), with the Xcode command line tools and
[Homebrew](https://brew.sh):

```
xcode-select --install
brew install bash gnu-getopt
```

circle-stdlib's `configure` needs bash 4 or later and GNU `getopt`, which
`make_all.sh` puts first on the PATH on macOS. newlib's build would like
`makeinfo` (texinfo) for its manuals and Circle's Wi-Fi firmware download
uses `wget`; without them, `make_all.sh` and `make_wifi_firmware.sh` use
stand-ins (the manuals aren't needed, and `curl` does the download).

The Arm GNU Toolchain 15.2.rel1 is downloaded into
`~/.cache/bmmac/toolchains` the first time a build needs it
(`get_gnu_toolchain.sh`), built for your computer (Linux or macOS):
`arm-none-eabi` for the 32-bit Pi Zero, 2 and 3 kernels, `aarch64-none-elf`
for the 64-bit Pi 4 and 5 kernels. If it can't be downloaded,
`BMMAC_ALLOW_ANY_TOOLCHAIN=1` lets the build use whatever
`arm-none-eabi-gcc` (or `aarch64-none-elf-gcc`) is on the PATH.

## Downloading the files

```
git clone --recursive https://github.com/jhonnaker1/bmmacintosh.git
```

or, in an existing clone, `git submodule update --init --recursive`.

## Building an SD card

```
./build_sdcard.sh
```

builds everything from clean and stages the boot partition in
`build/sdcard`, with the kernels for each board: the Mac Plus's
(`macplus*.img`) for every board, `macplus8-32.img` for the Pi 3 and Pi
Zero 2 W, `macplus7.img` for the Pi 2, `macplus.img` for the Pi Zero and
Zero W, and for the Pi 4 B and Pi 400 (with `armstub8-rpi4.bin`) and the Pi
5, Pi 500 and Pi 500+ the Mac II's too: `macplus8-rpi4.img` and
`macii8-rpi4.img`, `macplus_2712.img` and `macii_2712.img`.
`./build_sdcard.sh pi3`, `./build_sdcard.sh pi2`, `./build_sdcard.sh pi0`,
`./build_sdcard.sh pi4` or `./build_sdcard.sh pi5` builds just one, and
`./build_sdcard.sh pi4 pi5` several. Copy the contents of that folder to the root of a FAT32
formatted SD card.

The PC tests (`tools/hostsim`, `tools/pi4kms` and `tools/pi4scaler`, below)
run first, and the build stops if any of them fails; `--skip-tests` leaves
them out.

While developing, `./build_sdcard.sh --skip-patches` keeps the third party
libraries already built and only rebuilds the kernels, which takes seconds.
The libraries are built for one board at a time, so without a board it
rebuilds the kernel for the board built last; a board they aren't built for
is built from clean. With
[networking](USER_GUIDE.md#networking-and-the-web-ui) on, a new kernel can
then be uploaded to the Pi from the web UI and the Pi
rebooted from there, without touching the SD card.

## The individual steps

1. `./clean_all.sh` cleans everything, including the third party builds,
   and resets Circle to its pinned revision.
2. `./make_all.sh [pi0|pi2|pi3|pi4|pi5] [--skip-patches]` applies `src/patches/` to
   Circle, builds circle-stdlib (Circle and newlib) and the Circle addons
   for one board (pi3 if none is given), then the board's kernels:
   `macplus8-32.img` for pi3, `macplus7.img` for pi2, `macplus.img` for pi0
   (Circle's Raspberry Pi 1), `macplus8-rpi4.img` and `macii8-rpi4.img`
   (64-bit) for pi4, which also builds Circle's `armstub8-rpi4.bin`,
   `macplus_2712.img` and `macii_2712.img` (64-bit) for pi5. It only needs running again after
   `clean_all.sh` or a change to the patches or circle-stdlib. To change
   board, run `clean_all.sh` first (make_all.sh refuses otherwise).
3. `source get_gnu_toolchain.sh && make` rebuilds just the board's kernels
   (for the Pi 4 and 5: `BMMAC_AARCH=64 source get_gnu_toolchain.sh &&
   make`); `make EMU=ii` or `make EMU=plus` one of them.
4. `./make_wifi_firmware.sh DIR` puts the Wi-Fi firmware in `DIR/firmware`
   (`build_sdcard.sh` does this).

The Pi 3 kernel also runs on the Pi Zero 2 W. `config.txt` has a section
per board that picks its kernel; the menu's *Machine* changes the `[pi4]`
and `[pi5]` ones. The Pi Zero has one core, so its kernel
runs the emulator on core 0 and looks after USB between frames, where the
other boards use core 1 for the emulator. See [PI4.md](PI4.md) and [PI5.md](PI5.md)
for how the Pi 4 and 5 builds differ.

## Testing on a PC

`tools/hostsim` builds the two emulator ports (Basilisk II and umac, with
`src/port/`) for the PC, `build/macii_sim` and `build/macplus_sim`, with a
simulated display, sound and input in place of the Raspberry Pi. Its
scripted tests need Mac ROMs, which aren't free: point `BMMAC_ROMS` at a
folder with yours (MAME's `macqd800.zip` and `macplus.zip`, or the ROM
files). Each Mac then boots its ROM to the "?" disk, and the tests check
the display lock and timer pacing, the sound, the mouse, the menu, saved
settings, the memory, switching Macs, web UI requests and the power key.
With System disks too (`BMMAC_II_DISK`, a raw Mac OS 8.1 hard disk, and
`BMMAC_PLUS_DISK`, an 800K System floppy) each Mac starts from its disk to
the Finder, the Mac II with the card's shared folder on its desktop.

```
BMMAC_ROMS=~/mame/roms tools/hostsim/run_tests.sh
```

Without `BMMAC_ROMS` only the tests that need no ROM run (and the rest are
skipped), as on GitHub's build. Screenshots (`.ppm`) are written to
`tools/hostsim/out/`.

To play with the port on the PC, run it live:

```
tools/hostsim/live.sh ii      (or plus)
```

and open http://127.0.0.1:8064/ in a browser: the screen, keyboard, mouse,
sound and gamepads (through the browser's Gamepad API) are in the page. The
simulator's SD card is `tools/hostsim/out/live`, set up as the Pi's: put
your ROMs in its `roms` folder. This runs BMMacintosh's port and the
emulators on the PC, not the Pi kernel. The web page is served on
127.0.0.1 only.

The simulator can also be driven by a script of key presses, mouse moves
and screenshots; see the top of `tools/hostsim/host_sim.c` for the
commands. [SIMULATOR.md](SIMULATOR.md) has more: setting it up, debugging
it with gdb or lldb, and getting a log from the Pi.

`tools/pi4scaler` tests the shared Pi 4/5 ARM smooth scaling and CRT shader
(`src/av/scaler.*`): every pixel is checked against crt-pi worked out in
floating point, and the pictures are written to `tools/pi4scaler/out/`.
`tools/pi4kms` tests the Pi 4/5 direct display drivers (`src/kms/`, see
[PI4.md](PI4.md) and [PI5.md](PI5.md)): shared filter kernels, EDID, mode
choice, HDMI packets and audio clocks, and each board's display lists and
PHY settings.
Both come from BMA800XL.

```
tools/pi4scaler/run_tests.sh
tools/pi4kms/run_tests.sh
```

## The web UI

The web UI's pages are in `src/webui/assets/`. They are compiled into the
kernel through `src/webui/webui_assets.c`, which the Makefile regenerates
with `tools/gen_webui_assets.py` whenever an asset changes.

## Layout

| Path                         | What                                        |
|------------------------------|---------------------------------------------|
| `src/main.cpp`, `kernel.*`   | Circle kernel; USB, GPIO, SD card, cores    |
| `src/network.*`              | Ethernet / Wi-Fi start up (from BMC64)      |
| `src/network_time_sync.*`    | Clock from the network (from BMC64)         |
| `src/slirp_sockets.cpp`      | The Mac II network's sockets (slirp's), on Circle's |
| `src/webui/`                 | Web UI server, file manager and pages (from BMC64) |
| `src/sdcard/`                | SD card helpers used by the web UI (from BMC64) |
| `src/fbl.*`, `crt_pi_*`      | Display layer and CRT shaders (from BMC64)  |
| `src/vchiqsound*`            | Sound through the VideoCore (from BMC64)    |
| `src/options.*`              | `cmdline.txt` options                       |
| `src/host.h`                 | Interface between kernel and emulator       |
| `src/port/bmmac.h`           | What the two Macs' front ends and the shared one offer each other |
| `src/port/frontend.c`        | The frame loop: display, pacing, sound, keyboard, mouse and gamepads, web UI requests |
| `src/port/keymap.c`          | USB HID usages to the Mac's key codes      |
| `src/port/menu.c`, `osd.c`   | The menu and its on-screen display          |
| `src/port/font8x16.h`        | The menu's font (the Linux console's)       |
| `src/port/machine.c`         | Switching Macs: `config.txt`'s `kernel=` lines |
| `src/port/roms.c`            | Finding the Mac ROMs, in MAME's zips too    |
| `src/port/ii/`               | Basilisk II's front end: memory and the frame loop (`main_ii.cpp`), video, sound, disks, prefs, clocks, the menu's items, ExtFS, `sysdeps.h` |
| `src/port/ii/basilisk.mk`    | Which Basilisk II sources are built         |
| `src/port/plus/`             | umac's front end (`main_plus.c`) and `umac.mk` |
| `src/port/fswrap.c`          | `stat()` of the SD card's root, which FatFs can't do (`--wrap=stat`) |
| `src/port/posix_extra.c`     | rmdir(), utime(), access(), creat() for newlib (ExtFS) |
| `src/patches/`               | Changes applied to Circle by `make_all.sh`  |
| `sdcard/`                    | Files for the SD card                       |
| `third_party/macemu`         | Basilisk II (submodule)                     |
| `third_party/umac`           | umac and Musashi (submodule)                |
| `third_party/zlib`           | zlib (submodule)                            |
| `third_party/circle-stdlib`  | Circle and newlib (submodule)               |
| `src/av/`                    | Shared Pi 4/5 video, ARM scaling / CRT shader and sound |
| `src/v3d/`                   | Shared Pi 4/5 GPU driver and CRT rendering  |
| `src/kms/common/`            | Shared Pi 4/5 filter kernels and mode calculations |
| `src/kms/pi4/`               | Pi 4 direct display driver and PHY          |
| `src/kms/pi5/`               | Pi 5 direct display driver and PHY          |
| `tools/hostsim`              | PC test harness                             |
| `tools/mac_media.py`         | ROM files and raw disk images from MAME's   |
| `tools/pi4scaler`            | PC test of shared Pi 4/5 ARM scaling and CRT rendering |
| `tools/pi4kms`               | PC test of Pi 4/5 display driver calculations |
