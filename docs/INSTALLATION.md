# Installation

## What you need

- A Raspberry Pi 4 Model B or Pi 400, or a Raspberry Pi 5, Pi 500 or Pi
  500+ (the best: the TT and Falcon need one); a Raspberry Pi 3 Model B, B+
  or A+, a Raspberry Pi Zero 2 W or a Raspberry Pi 2 Model B run the ST and
  STE, but may not keep up with everything (a Pi Zero or Zero W starts too,
  but is too slow; see the User Guide)
- A microSD card (BMMacintosh itself needs only a few MB; floppy images are
  under 1 MB each)
- An HDMI display, ideally one that accepts 720p at 60 Hz (and 50 Hz, for
  PAL programs)
- A USB keyboard and a USB mouse (GEM, the ST's desktop, needs one); a USB
  gamepad if you like

## Prepare the SD card

1. Download `bmmac-v<version>.zip` from the releases page, or build it
   yourself (see [BUILDING.md](BUILDING.md)) and use `build/sdcard/`.
2. Format the SD card as FAT32.
3. Extract the zip to the root of the card, so that the kernels
   (`kernel_2712.img`, `kernel8-rpi4.img`, `kernel8-32.img`, `kernel7.img`,
   `kernel.img`) and `config.txt` are at the top level, not in a folder (a
   Pi 4 or 5 with them in a folder shows its firmware's "not found" screen).
4. Copy your floppy images to the card. The `disks` folder is there for
   them, but any folder works. If you have images of Atari's TOS, copy them
   into the `tos` folder (see below).
5. Put the card in the Pi, connect the display, keyboard and mouse, and
   switch on.

There is no image to write and nothing to install on the card beyond
copying the files.

## TOS

The card starts EmuTOS, the free TOS (`tos/etos1024k.img`), which runs on
every machine BMMacintosh emulates, so nothing else is needed. Atari's own
TOS is copyrighted, so BMMacintosh can't include it: copy images of your own
into the `tos` folder and choose one in the menu (F12, *Machine*, *TOS*).
See [TOS.txt](TOS.txt).

## First boot

EmuTOS shows its welcome screen, tries drive A: and then shows the GEM
desktop, after about fifteen seconds (as long as a real ST with no disk in
its drive). Press **F12** for the menu: *Floppy Disks*, *Drive A:*, *Insert
a Disk...* puts a floppy image in, and *Restart* starts the ST from it. The
disks are remembered for next time.

USB keyboards, mice and gamepads can be plugged in and out while it runs.

The display is set to 720p at 60 Hz. If you get no picture, see
[Troubleshooting](USER_GUIDE.md#troubleshooting) in the User Guide.

## Networking (optional)

To manage the card's files from a browser, and start floppy images from it,
turn on Ethernet or Wi-Fi under *Network* in the F12 menu, then reboot; the
web UI's address is shown at the bottom of the screen. For Wi-Fi, *Scan
for Wi-Fi Networks* lists the networks in range and asks for the password.
See [Networking and the web UI](USER_GUIDE.md#networking-and-the-web-ui).

## Upgrading

Copy the new release's files over the old ones, except for files you have
changed: `config.txt` and `cmdline.txt` if you edited them. Your settings
(`bmmac.cfg`, `hatari.cfg`, `wpa_supplicant.conf`), your TOS images,
disk images and saved states are not in the release zip and stay as they are.

With networking on, a new kernel (`kernel_2712.img` for a Pi 5, 500 or
500+, `kernel8-rpi4.img` for a Pi 4 or 400, `kernel8-32.img` for a Pi 3 or
Zero 2 W, `kernel7.img` for a Pi 2, `kernel.img` for a Pi Zero or Zero W)
can also be uploaded in the web UI's file manager and started with
**Reboot**. Updates that change other files (the Pi firmware in the root or
in `firmware/`) need the card in a computer.
