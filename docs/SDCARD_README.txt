BMAtariST - bare metal Atari ST, STE, TT and Falcon emulator for the Raspberry Pi
=================================================================================

Copy everything here to the root of a FAT32 SD card and boot a Raspberry
Pi 5, 500, 4 or 400 from it (a Pi 3, Zero 2 W or 2 runs the ST and STE,
but may not keep up with everything), with a USB keyboard and mouse (and a
gamepad if you like) connected.

It starts EmuTOS, the free TOS, from the tos folder. To use Atari's own
TOS, copy your TOS images into tos (see tos/TOS.txt) and choose one in the
menu (F12, Machine, TOS). They are copyrighted, so they are not included.

Floppies: press F12, Floppy Disks, Drive A:, Insert a Disk, and choose an
image (.st, .msa, .dim, .stx, .scp, also zipped or gzipped). Restart (in
the menu) starts the ST from it. Disks the ST writes to are written back to
the card. Or start one from the web UI.

Hard disk: F12, Hard Disks, Folder as a Drive, and choose a folder on the
card (the hd folder is there for it): it's the ST's drive C:.

  F12               BMAtariST's menu      F11 (Print Screen)   Help
  End (Scroll Lock) Undo                  Home                 ClrHome
  Alt               Alternate             Page Up, Page Down   Keypad ( )

Gamepads are the joystick: stick or D-pad to move, A to fire, B the second
button, X Space, Y Return, Home (or Select+Start) for the menu.

Saved states: F12, Save State and Load State (in the snapshots folder).

Networking: choose Ethernet or Wi-Fi under Network in the menu (or add
network=ethernet or network=wifi to cmdline.txt), then open the address
shown on screen in a browser to manage the files on this card and start
floppy images.

No need to shut down: just switch it off.

Licences: BMAtariST is under the GPL version 3 (LICENSE.txt); the licenses
folder has the notices for other projects' code it includes (Hatari,
EmuTOS, zlib and the rest), and the firmware folder the Raspberry Pi and
Wi-Fi firmware's licences.

Options are in cmdline.txt; see docs/USER_GUIDE.md in BMAtariST's source
for all of them: https://github.com/jhonnaker1/bmatarist
