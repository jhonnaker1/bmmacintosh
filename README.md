# BMMacintosh

BMMacintosh is a bare metal Atari ST, STE, TT and Falcon emulator for the
Raspberry Pi. It runs the [Hatari](https://www.hatari-emu.org/) emulator
directly on the hardware, with no Linux underneath, the same way
[BMC64](https://github.com/randyrossi/bmc64) does for the Commodore 64,
[BMA800XL](https://github.com/aminch/bma800xl) for the Atari 800XL,
[BMCoco](https://github.com/jhonnaker1/bmcoco) for the Tandy Color Computer
and [BMApple2](https://github.com/jhonnaker1/bmapple2) for the Apple II.

BMMacintosh is based on BMApple2, which is based on BMCoco, which is based on
[BMA800XL](https://github.com/aminch/bma800xl) by Adam Minchinton, which is
based on [BMC64](https://github.com/randyrossi/bmc64) by Randy Rossi. The
emulator is replaced by Hatari; the kernel, display, sound, network and web
UI come from those projects.

BMMacintosh wouldn't be possible without the work behind the open source
projects it builds on. Thanks to their authors and contributors for making
it available; see the [third-party](docs/THIRD_PARTY.md) acknowledgements.

- Emulates the Atari ST, Mega ST, STE and Mega STE, and (on a Pi 4 or 5) the
  TT and the Falcon 030, chosen in the menu
- Starts with nothing added: [EmuTOS](https://emutos.sourceforge.io/), the
  free TOS, is on the card; your own TOS images can be added and chosen
- Floppy images (`.st`, `.msa`, `.dim`, `.stx`, `.scp`, also zipped or
  gzipped), written back to the card two seconds after the ST writes to
  them; a folder on the card as the ST's hard disk (GEMDOS drive C:), and
  ACSI and IDE hard disk images
- Frames locked to the display's refresh for perfectly smooth scrolling
  (50 Hz PAL, 60 Hz NTSC, 71 Hz on the monochrome monitor; the Pi 4 and 5
  switch the display to the ST's rate), with the sound kept in step: the
  YM2149, the STE's DMA sound and the Falcon's
- The ST's picture with its borders, for the demos that draw in them
- USB keyboards (keys go to the ST by where they are, so the TOS's own
  keyboard layout applies), USB mice as the ST's mouse, USB gamepads and
  joysticks wired to the GPIO header as the ST's joysticks, or the arrow keys
- Saved states: four slots, the whole machine
- No shutdown needed: just switch it off
- A menu (F12) for everything: floppies, hard disks, saved states, the
  machine, TOS, memory, monitor, joysticks, video, sound and network
- Optional Ethernet or Wi-Fi with BMC64's web UI: manage the SD card from a
  browser and start floppy images

## TOS

EmuTOS is in the card's `tos` folder and runs on every machine. Atari's own
TOS is copyrighted and not included: copy your own TOS images (`.img`,
`.rom` or `.tos`) into `tos` and choose one in the menu (F12, Machine, TOS).
See [docs/TOS.txt](docs/TOS.txt).

## Supported hardware

The **Raspberry Pi 4 Model B and Pi 400** (`kernel8-rpi4.img`, 64-bit) and
the **Raspberry Pi 5, Pi 500 and Pi 500+** (`kernel_2712.img`, 64-bit) are
the ones to use: Hatari emulates the ST cycle by cycle, which takes a fast
core, and the TT and Falcon (a 68030, and the Falcon's DSP) take more. On
them the display runs at the ST's exact refresh rate, and the CRT effect and
smooth scaling are drawn by the 3D GPU. See [docs/PI4.md](docs/PI4.md) and
[docs/PI5.md](docs/PI5.md).

The Raspberry Pi 3 Model B, B+ and A+ and the Pi Zero 2 W
(`kernel8-32.img`) and the Pi 2 Model B (`kernel7.img`) start too, for the
ST and STE; whether they keep up depends on the program (the menu shows the
speed, as a percentage of the real machine's). The Pi Zero and Zero W
(`kernel.img`) start, but are too slow.

## Getting started

1. Format an SD card as FAT32.
2. Copy the contents of a release zip (or of `build/sdcard/` after building)
   to the root of the card.
3. Copy your floppy images to the card (the `disks` folder is there for
   them, but any folder works).
4. Boot the Pi with a USB keyboard and mouse connected. EmuTOS starts to
   its desktop. Press **F12**, choose *Floppy Disks*, *Drive A:*, *Insert a
   Disk...*, and choose one; *Restart* in the menu starts the ST from it.

## Keys

| Key                      | Does                                              |
|--------------------------|---------------------------------------------------|
| F12                      | BMMacintosh's menu                                  |
| F1 to F10                | The ST's F1 to F10                                |
| F11 (or Print Screen)    | Help                                              |
| End (or Scroll Lock)     | Undo                                              |
| Home                     | ClrHome                                           |
| Page Up, Page Down       | The keypad's ( and )                              |
| Alt                      | Alternate                                         |

Gamepads are the joystick: the left stick or D-pad moves, **A** fires,
**B** is the second button, **X** types Space and **Y** Return. **Home**, or
**Select+Start** together, opens the menu, where the D-pad moves, **A**
chooses and **B** goes back.

See the [User Guide](docs/USER_GUIDE.md) for all the details, including the
options in `cmdline.txt` and the GPIO joystick wiring. All the documents are
listed in [docs/README.md](docs/README.md).

## Building

```
./build_sdcard.sh
```

builds everything (the kernels for all the boards; `./build_sdcard.sh pi4`
builds one) and stages the SD card files in `build/sdcard/`. It works on
Linux and macOS. See [docs/BUILDING.md](docs/BUILDING.md).

BMMacintosh can also run on a PC, in a simulator, for testing and debugging
(with gdb, lldb or strace), and live in a web page: see
[docs/SIMULATOR.md](docs/SIMULATOR.md).

## How it fits together

```
src/main.cpp, kernel.cpp   Circle kernel: USB, GPIO, SD card; runs Hatari on core 1
src/fbl.cpp                Display (dispmanx layer, optional CRT shader), from BMC64
src/vchiqsound*.cpp        Sound over HDMI or the headphone jack, from BMC64
src/network.cpp, webui/    Ethernet / Wi-Fi and the web UI, from BMC64
src/host.h                 What the kernel offers the emulator
src/port/platform.c        Hatari's front end: starting it, TOS, the main loop
src/port/screen.c          The ST's picture to the display
src/port/audio.c           Hatari's sound output
src/port/keymap.c          USB keys to ST scan codes
src/port/gui_event.c       Keyboard, mouse and gamepads into Hatari
src/port/joy_ui.c          Gamepads and GPIO joysticks as the ST's joysticks
src/port/statusbar.c       The disk activity light
src/port/menu.c, osd.c     The menu, drawn in the ST's font
src/port/config.h          Hatari's build options
src/port/hatari.mk         Hatari's (and zlib's) source list
third_party/hatari         Hatari (submodule)
third_party/zlib           zlib (submodule)
third_party/circle-stdlib  Circle and newlib (submodule)
src/av/, src/v3d/, src/kms Shared Pi 4/5 video, GPU and display drivers
tools/hostsim              Runs the emulator port on a PC for testing
```

Hatari is used unmodified: everything specific to BMMacintosh is in `src/`.
Hatari's CPU core is generated when it is built, as in Hatari's own build.

## Licence

BMMacintosh is free software under the GNU General Public License version 3;
see [LICENSE](LICENSE). It includes software from other projects under
their own licences (Hatari and EmuTOS under the GPL version 2 or later and
version 2, zlib under its own); see
[docs/THIRD_PARTY.md](docs/THIRD_PARTY.md).
