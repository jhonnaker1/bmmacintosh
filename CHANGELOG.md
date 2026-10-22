# Changelog

## 0.1.0 – 2026-10-22

The first BMMacintosh: BMAtariST 0.1.0 (itself from BMApple2, BMCoco,
BMA800XL and BMC64), with Hatari replaced by two 68k Macs, each a kernel
of its own.

- The Mac II and Quadra (`macii*.img`, the Pi 4 and 5): Basilisk II from
  kanjitalk755's macemu, with its ARM CPU core (uae_cpu_2021: a 68040 with
  an FPU), direct addressing, 8 to 256 MB, screens of 640x480 to 1024x768
  in black and white to millions of colours, sound, and its tick, sound and
  disks driven from the frame loop (no threads, VOSF, JIT or network yet).
  Where the Mac reads or writes memory it doesn't have, it reads 0 and its
  writes are dropped, as Basilisk II on Unix skips them
- Hard disk images (raw or with an Apple partition map), two floppy drives
  and a CD-ROM drive that take images while the Mac runs, and the card's
  `shared` folder as a disk on the Mac's desktop (ExtFS, on FAT)
- The Mac Plus (`macplus*.img`, every Pi): umac by Matt Evans with Musashi,
  the Mac Plus's ROM with 128K to 4 MB, its disk read and written in place,
  its sound buffer played
- Switching Macs from the menu (Machine), which rewrites `config.txt`'s
  `kernel=` lines and restarts the Pi, as BMC64 switched machines
- The user's own ROMs, as files or as MAME's zipped sets (whose chips are
  put back together); `tools/mac_media.py` makes ROM files and raw disk
  images from MAME's
- Keys pressed while the Mac starts reach it (Shift for no extensions)
- A Mac-style menu (F12) with the Linux console's font; the keyboard as a
  Mac's (Alt Command, the Windows key Option); gamepads as the mouse
- The PC simulator's tests boot both Macs' ROMs, and with the user's disks
  Mac OS 8.1 and System 4.3 to the Finder

Tested in the PC simulator; all seven kernels build. Not yet tried on a
Raspberry Pi.

BMAtariST's own history is in its repository:
https://github.com/jhonnaker1/bmatarist/blob/main/CHANGELOG.md
