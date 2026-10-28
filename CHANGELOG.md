# Changelog

## 0.1.0 – 2026-10-28

The first BMMacintosh: BMAtariST 0.1.0 (itself from BMApple2, BMCoco,
BMA800XL and BMC64), with Hatari replaced by two 68k Macs, each a kernel
of its own.

- The Mac II and Quadra (`macii*.img`, the Pi 4 and 5): Basilisk II from
  kanjitalk755's macemu, with its ARM CPU core (uae_cpu_2021: a 68040 with
  an FPU), direct addressing, 8 to 256 MB, screens of 640x480 to 1024x768
  in black and white to millions of colours, sound, and its tick, sound and
  disks driven from the frame loop (no threads, VOSF or JIT).
  Where the Mac reads or writes memory it doesn't have, it reads 0 and its
  writes are dropped, as Basilisk II on Unix skips them
- The Mac II's network: Basilisk II's slirp, in the background, through
  the Pi's Ethernet or Wi-Fi (DHCP gives the Mac 10.0.2.15; its TCP and
  UDP go out from the Pi's address), on Circle's own sockets on the Pi,
  which connect without holding up the Mac
- Hard disk images (raw or with an Apple partition map, up to 4 GB: read
  and written through FatFs itself, past newlib's 2 GB seek), two floppy drives
  and a CD-ROM drive that take images while the Mac runs, and the card's
  `shared` folder as a disk on the Mac's desktop (ExtFS, on FAT)
- The Mac Plus (`macplus*.img`, every Pi): umac by Matt Evans with Musashi,
  the Mac Plus's ROM with 128K to 4 MB, its disk read and written in place
  through a disk driver of BMMacintosh's own (Basilisk II's, GPL version 2
  or later, in place of umac's, marked "GPLv2"), its sound buffer played;
  its keyboard with the arrow keys and keypad (the Instant command umac
  doesn't answer, answered in front of it), and a locking Caps Lock; its
  clock chip, with the Pi's time and its parameter RAM kept on the card
  (`macplus.pram`); DiskCopy 4.2 images (their checksum kept right), and
  hard disks with an Apple partition map (their first HFS partition); its
  sound at the volume and from the buffer the Mac chooses, and off when
  the Mac switches it off
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
- Left out for now (see the User Guide's *Left out, for now*): Basilisk
  II's JIT (x86 only), saved states, and a second drive on the Mac Plus

Tested in the PC simulator; all seven kernels build. On a Raspberry Pi 5
the Mac II starts Mac OS 8.1; the Mac Plus, the Mac II's network and the
other Pis haven't been tried on a Pi yet.

BMAtariST's own history is in its repository:
https://github.com/jhonnaker1/bmatarist/blob/main/CHANGELOG.md
