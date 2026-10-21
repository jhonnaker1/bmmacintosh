# Changelog

## 0.1.0 – unreleased

The first BMAtariST: BMApple2 0.4.2 (itself from BMCoco, BMA800XL and
BMC64), with KEGS replaced by Hatari (master after 2.6.1).

- Emulates the Atari ST, Mega ST, STE and Mega STE, and the TT and Falcon
  030 (for a Pi 4 or 5), with Hatari's own CPU, chips and timing
- EmuTOS 1.4 on the card, so it starts with nothing added; the user's own
  TOS images in the tos folder, chosen in the menu
- Floppy images (.st, .msa, .dim, .stx, .scp, zipped or gzipped), written
  back to the card two seconds after the ST writes to them, so the Pi can
  be switched off at any time; a folder on the card as drive C: (GEMDOS),
  ACSI and IDE hard disk images
- Saved states, four slots
- The ST's picture with its borders, locked to the display's refresh (50,
  60 or 71 Hz, which the Pi 4 and 5 switch the display to), with the sound
  kept in step at 44.1 or 48 kHz
- USB keyboards by key position (F11 Help, End Undo), USB mice, USB
  gamepads and GPIO joysticks in either joystick port, or the arrow keys
- A menu (F12) in the ST's font for the disks, states, machine, TOS,
  memory, monitor, joysticks, video, sound and network; the speed of the
  emulation, as a percentage of the real machine's
- The web UI starts floppy images and edits hatari.cfg; an Atari ST theme
- The PC simulator boots EmuTOS in its tests, and runs live in a browser

BMApple2's own history is in its repository:
https://github.com/jhonnaker1/bmapple2/blob/main/CHANGELOG.md
