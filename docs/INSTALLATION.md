# Installation

## What you need

- A Raspberry Pi 4 Model B or Pi 400, or a Raspberry Pi 5, Pi 500 or Pi
  500+, for the Mac II and Quadra (and the Mac Plus); any other Pi (3
  Model B, B+ or A+, Zero 2 W, 2 Model B, Zero or Zero W) for the Mac Plus
- A microSD card (BMMacintosh itself needs a few MB; a Mac hard disk image
  can be up to 4 GB, FAT32's largest file)
- An HDMI display, ideally one that accepts 720p at 60 Hz
- A USB keyboard and a USB mouse; a USB gamepad works as a mouse too
- Your own Mac ROMs and system software: Apple's aren't free, so none come
  with BMMacintosh (see [ROMS.txt](ROMS.txt))

## Prepare the SD card

1. Download `bmmacintosh-v<version>.zip` from the releases page, or build
   it yourself (see [BUILDING.md](BUILDING.md)) and use `build/sdcard/`.
2. Format the SD card as FAT32.
3. Extract the zip to the root of the card, so that the kernels
   (`macii_2712.img`, `macplus_2712.img`, `macii8-rpi4.img`,
   `macplus8-rpi4.img`, `macplus8-32.img`, `macplus7.img`, `macplus.img`)
   and `config.txt` are at the top level, not in a folder (a Pi 4 or 5 with
   them in a folder shows its firmware's "not found" screen).
4. Copy your ROMs into the `roms` folder: ROM files, or MAME's zipped sets
   as they are (`macqd800.zip` for the Mac II, `macplus.zip` for the Mac
   Plus). See [ROMS.txt](ROMS.txt).
5. Copy your disk images (a System 7 or Mac OS 8 hard disk for the Mac II,
   a System floppy or hard disk volume for the Mac Plus) into `disks`.
6. Put the card in the Pi, connect the display, keyboard and mouse, and
   switch on.

There is no image to write and nothing to install on the card beyond
copying the files.

## First boot

A Pi 4 or 5 starts as a Mac II, the others as a Mac Plus. The Mac shows
its gray desktop with a flashing "?" disk: it has no disk to start from
yet. Press **F12** for the menu, then *Hard Disks*, *Add a Hard Disk
Image...* (Mac II) or *Disk* (Mac Plus), choose your disk image, and
*Restart the Mac*. The disks are remembered for next time. To switch
Macs, F12, *Machine* (the Pi restarts).

If Mac OS stops with a system error while it starts, hold Shift while it
starts: that starts it without its extensions.

USB keyboards, mice and gamepads can be plugged in and out while it runs.

The display is set to 720p at 60 Hz. If you get no picture, see
[Troubleshooting](USER_GUIDE.md#troubleshooting) in the User Guide.

## Networking (optional)

To manage the card's files from a browser, and start disk images from it,
turn on Ethernet or Wi-Fi under *Network* in the F12 menu, then reboot; the
web UI's address is shown at the bottom of the screen. For Wi-Fi, *Scan
for Wi-Fi Networks* lists the networks in range and asks for the password.
See [Networking and the web UI](USER_GUIDE.md#networking-and-the-web-ui).

## Upgrading

Copy the new release's files over the old ones, except for files you have
changed: `config.txt` and `cmdline.txt` if you edited them (the menu's
*Machine* changes `config.txt`'s `kernel=` lines: keep yours, or choose
the Mac again after). Your settings (`bmmac.cfg`, `macii.prefs`,
`macii.xpram`, `wpa_supplicant.conf`), your ROMs and disk images are not in
the release zip and stay as they are.

With networking on, a new kernel (`macii_2712.img` or `macplus_2712.img`
for a Pi 5, 500 or 500+, `macii8-rpi4.img` or `macplus8-rpi4.img` for a
Pi 4 or 400, `macplus8-32.img` for a Pi 3 or Zero 2 W, `macplus7.img` for
a Pi 2, `macplus.img` for a Pi Zero or Zero W) can also be uploaded in the
web UI's file manager and started with **Reboot**. Updates that change
other files (the Pi firmware in the root or in `firmware/`) need the card
in a computer.
