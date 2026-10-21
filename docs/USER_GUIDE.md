# BMAtariST User Guide

BMAtariST turns a Raspberry Pi into an Atari ST, STE, Mega ST or Mega STE,
or (on a Pi 4 or 5) a TT or Falcon. It runs the Hatari emulator on the bare
hardware, so it starts in a couple of seconds and can be switched off at any
time.

## The SD card

| File or folder   | What it is                                                  |
|------------------|-------------------------------------------------------------|
| `kernel8-rpi4.img`, `kernel_2712.img`, `kernel8-32.img`, `kernel7.img`, `kernel.img` | BMAtariST for the Pi 4 and Pi 400, the Pi 5, 500 and 500+, the Pi 3 and Zero 2 W, the Pi 2, and the Pi Zero and Zero W |
| `bootcode.bin`, `start.elf`, `fixup.dat`, `start4.elf`, `fixup4.dat`, `armstub8-rpi4.bin`, `*.dtb`, `overlays/` | Raspberry Pi firmware |
| `config.txt`     | Raspberry Pi firmware settings: the display mode            |
| `cmdline.txt`    | BMAtariST's options (see Options)                           |
| `bmatarist.cfg`  | Settings changed in BMAtariST's menu (written by BMAtariST) |
| `hatari.cfg`     | Hatari's settings: the machine, TOS, memory, the disks in the drives, joysticks (written by Hatari when the menu changes them) |
| `hatari.nvram`   | The TT's and Falcon's NVRAM (written by Hatari)              |
| `tos/`           | TOS images: EmuTOS, and your own (see TOS)                  |
| `disks/`         | For your floppy images (any folder works)                   |
| `hd/`            | For a folder that is the ST's hard disk, and hard disk images |
| `snapshots/`     | Saved states                                                |
| `firmware/`      | Wi-Fi firmware                                              |

A floppy the ST has written to is written back to its image on the card two
seconds after it stops writing, and BMAtariST pushes everything out to the
card every second, so the Pi can be switched off at any time (give it a few
seconds after saving).

## TOS

The ST's operating system is in the card's `tos` folder. EmuTOS
(`etos1024k.img`), the free TOS, is there from the start and runs on every
machine: BMAtariST starts with it. Atari's own TOS images are copyrighted
and not included: copy yours into `tos` (any `.img`, `.rom` or `.tos` file
of a TOS's size) and choose one in the menu: *Machine*, *TOS*. See
[TOS.txt](TOS.txt) for which TOS suits which machine.

EmuTOS's 1024K version is a US one: it runs the ST at 60 Hz (NTSC). Its
desktop's *Options* menu sets its language and keyboard.

## Floppy disks

*Floppy Disks* in the menu: *Drive A:* (or *B:*), *Insert a Disk...*, and
choose an image. `.st`, `.msa`, `.dim`, `.stx` (Pasti) and `.scp` images
work, also zipped (`.zip`) or gzipped (`.gz`). The ST sees the disk change
as when a real disk is swapped; to start from it, choose *Restart* in the
menu (as switching the ST off and on). From the web UI, clicking a floppy
image does both.

Disks the ST writes to are written back to their image (`.st`, `.msa` and
`.dim`; the others when they are ejected, as Hatari does). *Write
Protection* makes every disk read only (*On*) or follows the image file's
own read only flag (*Automatic*).

*Disk 2 into B: Too*: inserting a game's first disk, `GAME_A.ST` or
`GAME_1.ST`, also puts `GAME_B.ST` or `GAME_2.ST` in drive B:.

*Fast Floppy Access* makes the floppy controller fast, rather than as slow
as a real one. A few programs (copy protections) need the real speed.

## Hard disks

*Hard Disks* in the menu:

- **Folder as a Drive (GEMDOS)**: a folder on the card becomes the ST's
  drive C:. The ST reads and writes the files in it as they are, so files
  copied to the folder (from a computer, or the web UI) are on the ST's
  drive, and the other way round. Programs in its `AUTO` folder run when
  the ST starts. *Start from the Hard Disk* makes it the boot drive.
- **ACSI Hard Disk Image** (the ST's own hard disk interface) and **IDE
  Hard Disk Image** (the Falcon's): a hard disk image file, partitioned and
  formatted by the ST's own hard disk driver (AHDI, HDDRIVER and the like)
  or by EmuTOS, which reads them directly.

Changing a hard disk restarts the ST.

## Saved states

*Save State* keeps the whole machine (its memory, chips and CPU, and the
disks in its drives) in one of four slots, in the card's `snapshots`
folder; *Load State* brings it back exactly as it was. A state is for the
same BMAtariST version and machine.

## The menu

**F12** (or a gamepad's Home, or Select and Start together) opens the menu,
and the ST stops while it's open. The arrow keys (or the D-pad) move,
**Enter** (or A) chooses, **Esc** (or B) goes back, and F12 again closes it.

| Item          | What it has |
|---------------|-------------|
| Floppy Disks  | The two drives: insert, eject, write protection |
| Hard Disks    | A folder as drive C:, ACSI and IDE hard disk images |
| Save State, Load State | Four slots |
| Reset         | The ST's reset button: TOS starts again, memory kept |
| Restart       | Switching the ST off and on |
| Machine       | The machine, TOS, memory, TT-RAM, monitor, blitter, CPU speed, cycle exact CPU, fast boot |
| Joysticks     | What is in the ST's joystick and mouse ports |
| Video         | Screen size, frame lock, CRT effect, smooth scaling, disk activity |
| Sound         | Volume, HDMI or the headphone jack |
| Network       | Ethernet or Wi-Fi, the web UI |
| About         | Versions, the machine, the display, how fast the emulation runs |

The menu's last line shows how fast the emulation runs, as a percentage of
the real machine's speed. Under 100% the Pi can't keep up with the machine
or program.

The ST's settings (machine, TOS, disks, joysticks) are Hatari's, kept in
`hatari.cfg`; a change that needs it restarts the ST, as in Hatari's own
settings dialog. BMAtariST's own (video, sound, network) are kept in
`bmatarist.cfg`.

## Keyboard and mouse

Keys go to the ST by where they are on the keyboard, not by what they type:
the TOS's own keyboard layout decides what each one types, as on an ST with
that keyboard. So set the language of the TOS (or EmuTOS's *Options*,
*Desktop configuration*) to match your keyboard.

The ST's keys a PC keyboard doesn't have:

| PC key                   | ST key          |
|--------------------------|-----------------|
| F11 (or Print Screen)    | Help            |
| End (or Scroll Lock)     | Undo            |
| Home                     | ClrHome         |
| Insert                   | Insert          |
| Page Up, Page Down       | The keypad's ( and ) |
| Alt (either)             | Alternate       |
| The key left of Z (ISO keyboards) | The ST's < > key |

F12 is BMAtariST's menu. The power key (a Pi 400's Fn+F10) shuts down.

A USB mouse is the ST's mouse (in its mouse port, unless a joystick is set
there): the left and right buttons are the ST's, and the middle button
double clicks.

## Machines and speed

*Machine*, *Machine* chooses an Atari ST, Mega ST, STE, Mega STE, TT or
Falcon 030; each gets its own CPU (a 68000 at 8 MHz, the TT's 68030 at 32
MHz with its FPU, the Falcon's 68030 at 16 MHz with its DSP). The TT and the
Falcon need a Raspberry Pi 4 or 5, and a TOS for them (their own, or
EmuTOS).

*Memory* is the ST-RAM (512 KB to 14 MB); *TT-RAM* is the TT's and
Falcon's fast RAM. *Monitor*: the SM124 monochrome monitor (the ST's high
resolution, 640x400 at 71 Hz), a colour one (low and medium resolution),
VGA (the TT's and Falcon's) or a TV. *Blitter* adds the blitter to an ST
(the STE and later always have it). *CPU Speed* runs the CPU at 16 or 32
MHz, which some programs like (and others don't). *Cycle Exact CPU* times
every instruction as the real CPU does, which demos and some games need;
off, the emulation takes less of the Pi. *Fast Boot* skips TOS's memory
test.

## Joysticks

The ST has two joystick ports: the joystick port, which most games use, and
the mouse port. *Joysticks* in the menu sets what is in each:

- **USB Gamepad 1** or **2**: the left stick or D-pad moves; A fires, B is
  the second button (with *B Button Presses Space*, it presses Space, for
  games that jump with it), the shoulder buttons autofire.
- **GPIO Joystick 1** or **2**: a joystick wired to the GPIO header (below).
- **Arrow Keys**: the cursor keys move and right Ctrl fires (hold Shift to
  type the arrow keys instead).

With a joystick in the mouse port the USB mouse isn't the ST's. By default
gamepad 1 is in the joystick port and the mouse in the mouse port.

| Gamepad button     | ST                                  |
|--------------------|-------------------------------------|
| A                  | Fire                                |
| B                  | Second button (Space, if chosen)    |
| L, R               | Autofire                            |
| X                  | Space                               |
| Y                  | Return                              |
| Home, Select+Start | BMAtariST's menu                    |

**Joysticks on the GPIO header**: set `gpio_joysticks=1` in `cmdline.txt`,
then reboot. The wiring is BMC64's "GPIO Config 1". The pins are active low
(each switch connects the pin to ground) and BMAtariST turns on the Pi's
pull-ups:

| Signal | Joystick 1 (GPIO) | Joystick 2 (GPIO) |
|--------|-------------------|-------------------|
| Up     | 17                | 5                 |
| Down   | 18                | 6                 |
| Left   | 27                | 12                |
| Right  | 22                | 13                |
| Fire   | 23                | 19                |

In the menus either GPIO joystick moves the cursor and fire chooses, as a
gamepad's D-pad and A do.

Ground is on pins 6, 9, 14, 20, 25, 30, 34 or 39 of the header. These are
digital (switch) joysticks, like an Atari-style stick with an adapter; an
Atari ST's own joysticks are digital too, so one wired to the GPIO header
through its 9-pin plug works as it is (with its pins on the GPIO pins above
and its ground on a ground pin).

## Display and timing

An ST shows 50.05 frames a second in PAL, 60.04 in NTSC (as EmuTOS's US
version sets it) and 71.2 on the monochrome monitor. When the display's
refresh rate is within 2% of the ST's, each frame is shown on exactly one
display refresh, which gives perfectly smooth scrolling. Otherwise the
emulation keeps exact time and frames are shown as they come. `config.txt`
sets the display to 720p at 60 Hz; for a PAL ST choose `hdmi_mode=19` (50
Hz). On a Pi 4 or 5 BMAtariST sets the display to the ST's exact rate
itself, whichever it is (see [PI4.md](PI4.md)). `vsync=on` or `vsync=off` in
`cmdline.txt` (or *Frame Lock* in the menu) forces one or the other.

The picture is the ST's with its borders (Hatari's 832x552: low
resolution's pixels doubled, medium's lines doubled), so demos that draw in
the borders are shown, at a monitor's 4:3; the monochrome monitor's 640x400
and the TT's and Falcon's modes are shown as they are.

## Networking and the web UI

BMAtariST can join your network over Ethernet or Wi-Fi and serve a web page
for managing the SD card from any browser on the same network, so files can
be copied to and from the Pi without taking the card out. It is the web UI
from BMC64.

Turn it on in the menu's *Network*. For Ethernet, set *Network* to Ethernet
and reboot. For Wi-Fi, choose *Scan for Wi-Fi Networks*: the first time,
it offers to switch Wi-Fi on and reboot; after that it lists the networks
it can hear, strongest first. Pick yours: it fills in *Wi-Fi Network*, and
if the network needs a password, *Wi-Fi Password* is next. Then reboot.
Set *Wi-Fi Country* to your two letter country code too; it decides which
channels are allowed. A hidden network can be typed in under *Wi-Fi
Network* instead. Or, in `cmdline.txt` (on the same single line as the
other options):

- Ethernet: `network=ethernet`
- Wi-Fi: `network=wifi`, and copy `wpa_supplicant.conf.example` to
  `wpa_supplicant.conf` in the root of the card with your network's name,
  password and country.

Wi-Fi needs the files in the `firmware` folder, which `build_sdcard.sh` puts
there.

Once the Pi has an address, it is shown at the bottom of the screen for a
few seconds (`WEB UI: HTTP://192.168.1.50/`) and written to the log.
Open that address in a browser.

The web UI has:

- **Dashboard**: the Pi's model, address, uptime, temperature, power and
  throttling warnings and free space on the card. Buttons to **Restart**
  the ST (as switching it off and on), **Reboot** BMAtariST, **Shut
  Down** (the Pi halts; switch it off, then on again to restart; a Pi 400
  switches itself off, and its power key starts it again), and **Disable
  Web UI** until the next reboot.
- **Files**: browse the SD card; upload (or drag and drop) files into a
  folder, download, rename, delete, and create folders. Clicking a floppy
  image **starts it**: it goes in drive A: and the ST starts again. `cmdline.txt`, `config.txt`,
  `bmatarist.cfg`, `hatari.cfg` and `wpa_supplicant.conf` can be **edited**
  in the browser; changes take effect after a reboot, and the previous
  version is kept as `.bak`.

Uploads can't overwrite `cmdline.txt`, `config.txt` or `wpa_supplicant.conf`
(edit them instead), or anything in `firmware`. Everything else, including
the kernels, can be replaced, so a new build of BMAtariST can be uploaded and
started with **Reboot**. A mistake in `cmdline.txt` or `config.txt` can stop
the Pi starting, and then needs the card in a computer (the `.bak` copy
makes that easy).

The web UI is open to anyone on your network. To protect it with a PIN, set
`webui_pin=1234` in `cmdline.txt`; the browser then asks for a user name
(anything) and the PIN. To network without the web UI, set `webui=0`.

The network also sets the Pi's clock, so files saved on the card get the
right date and time. Set `timezone` to your offset from UTC in minutes, for
example `timezone=600` for UTC+10. (Hatari gives the ST the Pi's date and time when it starts.)

## Options

`cmdline.txt` is a single line of `name=value` options separated by spaces.
BMAtariST's menu saves the options it changes to `bmatarist.cfg`, one
`name=value` per line, and those win over `cmdline.txt`. Either file can be
edited from the web UI. Everything about the emulated machine is in
Hatari's settings, `hatari.cfg`, which the menu changes.

| Option              | Values                       | Default  | What it does |
|---------------------|------------------------------|----------|--------------|
| `aspect`            | number                       | `1.0`    | Multiplies the width of the picture |
| `disk_activity`     | `0`, `1`                     | `1`      | While a floppy drive turns, show its drive and what the floppy controller does at the bottom left (Video, Show Disk Activity) |
| `scale`             | 0.1 to 1.0                   | `1.0`    | Height of the picture as a fraction of the screen (Video, Screen Size; less for a TV that cuts off the edges) |
| `smooth`            | `0`, `1`                     | `0`      | Bilinear filtering instead of sharp pixels |
| `crt_shader`        | `0`, `1`                     | `0`      | CRT look (scanlines) from BMC64 |
| `crt_curvature`     | `0`, `1`                     | `0`      | With `crt_shader`, a curved screen |
| `crt_gpu`           | `0`, `1`                     | `1`      | Pi 4 and 5: the 3D GPU draws the smooth and CRT pictures; `0` has the ARM draw them (see [PI4.md](PI4.md)) |
| `vsync`             | `auto`, `on`, `off`          | `auto`   | Lock frames to the display (see Display and timing) |
| `resolution`        | `auto`, `1920x1080`, `1280x720` | `auto` | Pi 5 only, with `video_driver=kms`: the display's resolution; `auto` keeps the one the firmware chose (the Pi 5's firmware ignores `hdmi_mode`) |
| `video_driver`      | `kms`, `hvs`, `firmware`     | `kms`    | Pi 4 and 5: `kms` runs the display at the ST's exact frame rate; `hvs` leaves the mode set in `config.txt`; `firmware` uses the firmware's framebuffer, if the others give trouble (see [PI4.md](PI4.md) and [PI5.md](PI5.md)) |
| `sound`             | `0`, `1`                     | `1`      | Sound on or off |
| `volume`            | 0 to 100                     | `100`    | Sound volume |
| `audio_out`         | `auto`, `hdmi`, `headphones` | `auto`   | Where the sound goes (`auto`: HDMI, or the headphone jack if the display takes no sound) |
| `audio_buffer_ms`   | 20 to 500                    | `60`     | Sound buffer; raise it if the sound breaks up |
| `gpio_joysticks`    | `0`, `1`                     | `0`      | Joysticks on the GPIO header |
| `enable_logging`    | `off`, `uart`, `file`        | `off`    | Where the log goes (see Logging) |
| `network`           | `off`, `ethernet`, `wifi`    | `off`    | Networking (see Networking and the web UI) |
| `webui`             | `0`, `1`                     | `1`      | Web UI when networking is on |
| `webui_pin`         | digits                       | none     | PIN for the web UI |
| `timezone`          | minutes east of UTC          | `0`      | Time zone for the network clock |
| `usbpowerdelay`     | milliseconds                 | Circle's | Circle: wait for USB devices to power up (by default 300 ms, 510 ms behind a hub) |

## Logging

BMAtariST logs what it is doing, which helps when something doesn't work:
the version, the display and sound set up, the TOS images found, devices
coming and going, network status, and Hatari's own messages (the TOS
loaded, disks inserted and written back, anything it didn't like). Choose
where it goes with `enable_logging` in `cmdline.txt`:

- **Off** (the default, also when `enable_logging` isn't set): no log.
- **UART**: the serial port, GPIO14 TX and GPIO15 RX at 115200 baud
  (all boards, the Pi 5 too). Needs a USB serial adapter.
- **File**: the serial port, and `bmatarist.log` in the root of the SD
  card. The file starts with what was logged since boot, is written to the
  card every few seconds (and is complete after a crash, a hang or
  switching off), and starts again at boot once it is over 1 MB. Download
  it from the web UI's file manager to send with a bug report. Writing it
  costs a little time on each frame, so switch it off once you have what
  you need.

## Troubleshooting

**"Hatari did not start" or "Failed to load TOS image"**: the TOS chosen
isn't there or doesn't suit the machine. Delete `hatari.cfg` (from the web
UI, or with the card in a computer) and BMAtariST starts with EmuTOS again,
or choose another TOS in the menu.

**A TOS doesn't start on a machine**: TOS 1.0x is for the ST, 1.06 and 1.62
for the STE, 3.0x for the TT and 4.0x for the Falcon (see
[TOS.txt](TOS.txt)). EmuTOS runs on all of them.

**No picture**: uncomment `hdmi_safe=1` in `config.txt`. If that works, your
display doesn't accept 720p at 60 Hz; try another `hdmi_mode`.

**No sound over HDMI**: try `hdmi_drive=2` in `config.txt`, or
`audio_out=hdmi` in `cmdline.txt`.

**Sound breaks up, or the menu's speed line is under 100%**: the Pi can't
keep up with that machine or program. Turn off *Cycle Exact CPU* (Machine),
use an ST rather than a TT or Falcon, or a faster Pi (a Pi 4 or 5). For the
sound alone, raise `audio_buffer_ms` in `cmdline.txt`, e.g. to 100.

**A program seems stuck loading**: watch the bottom left of the screen.
While a floppy drive's motor turns, a red light shows there (lit while the
floppy controller works) with the drive and what it is doing, as in `A:
RS:01 HD:17 TR:17 SE:03 SI:0` (reading a sector, the head on track 17).
`no disk` in red means the drive is empty.

**The keys type the wrong characters**: the TOS's keyboard layout isn't
your keyboard's (see Keyboard and mouse).

**The TV cuts off the top and bottom of the picture or the menu**: many TVs
enlarge the picture and crop its edges (overscan). Make *Video*, *Screen
Size* smaller (90% usually does), or turn overscan off on the TV ("Just
Scan", "Screen Fit" or "1:1").

**Keyboard, mouse or gamepad not working**: unplug it and plug it in again;
devices can come and go while the emulator runs. Some keyboards need a
longer `usbpowerdelay` (add `usbpowerdelay=1000` to `cmdline.txt`).

**8BitDo M30 2.4G buttons are wrong**: its receiver can be left in Switch
mode, where it acts as a Switch Pro controller. With the pad connected,
hold Home (the heart) and D-pad Down for 5 seconds until the LEDs flash; it
goes back to its own mode and keeps it.

**The joystick doesn't move**: check *Joysticks* in the menu: most games
read the joystick port, not the mouse port.
