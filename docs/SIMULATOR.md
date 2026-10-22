# Testing and debugging BMMacintosh on a PC

BMMacintosh isn't tested in QEMU. On the Pi it runs bare metal, on the Circle
kernel with no Linux underneath, and QEMU's Raspberry Pi machines don't
emulate enough of the Pi (its VideoCore display, HDMI sound, USB and Wi-Fi
as BMMacintosh uses them) to run it. There would be nothing to strace either:
strace follows a Linux program's system calls, and BMMacintosh makes none.

What BMMacintosh has instead is a **PC simulator** (`tools/hostsim`, from
BMCoco): the same emulator code that runs on the Pi (Hatari and BMMacintosh's
front end for it, `src/port/`), built as an ordinary program for Linux or
macOS, with a simulated display, sound, keyboard, mouse, joysticks and SD
card in place of the Pi's. Because it's an ordinary program, **gdb, lldb,
strace and the rest work on it** as on any other program.

## What it can and can't show

The simulator runs the ST's emulation, TOS, the menu, the floppy and hard
disk handling, saved states, and BMMacintosh's pacing and sound logic. A
problem in any of those usually shows up in it too.

It does **not** run the Pi side: the Circle kernel, the Pi's display and
sound drivers, USB, Wi-Fi and Circle's network stack, and the SD card
driver. And a PC is much faster than a Pi: the simulator says nothing about
whether a Pi keeps up (the menu's speed line, on the Pi, does).

So the simulator is the first place to look, and if a problem doesn't show
up there, the Pi's log (at the end of this page) is the next.

## Setting it up

You need a C compiler, `make`, `git`, `curl`, `unzip` and Python 3. No
Raspberry Pi toolchain is needed for the simulator.

**Linux** (Debian, Ubuntu, Raspberry Pi OS):

```
sudo apt install build-essential git curl unzip python3 gdb strace
```

**macOS**: install Xcode's command line tools (`xcode-select --install`),
which include `clang`, `make`, `git` and the `lldb` debugger.

Get the source, with Hatari and zlib (git submodules):

```
git clone https://github.com/jhonnaker1/bmmacintosh
cd bmmac
git submodule update --init third_party/hatari third_party/zlib
```

(`git clone --recursive` works too, but also fetches circle-stdlib, which
only the Pi build needs.)

Build and run the tests:

```
tools/hostsim/run_tests.sh
```

The first build takes a few minutes: Hatari's CPU core is about 35 MB of
generated C. The tests boot EmuTOS (downloaded the first time, as for the
SD card) to its desktop and check the frame lock, the sound, the keyboard,
mouse and menu, saved states, the machine switching, a folder as drive C:
and the web UI's requests.

Each test runs in its own folder under `tools/hostsim/out/`, which stands
in for the SD card, with its log (`log.txt`) and screenshots (`.ppm`).

## Using it live

```
tools/hostsim/live.sh
```

then open <http://127.0.0.1:8064/> in a browser. The ST's screen, sound,
keyboard, mouse and gamepads are in the page, and F12 opens BMMacintosh's
menu as on the Pi. The simulator's SD card is `tools/hostsim/out/live`, set
up as the Pi's: EmuTOS in `tos`, and `disks`, `hd` and `snapshots` folders
for your files. A folder with your own TOS images can be given:

```
tools/hostsim/live.sh /path/to/your/tos/images
```

Options from `cmdline.txt` go in `BMMAC_CMDLINE`:

```
BMMAC_CMDLINE="volume=50" tools/hostsim/live.sh
```

Its messages (the same as the Pi's log, Hatari's included) go to the
terminal. Ctrl+C stops it.

## Debugging it

### Where is it stuck? (gdb or lldb)

Start it live as above, get it into the state where it's stuck, and attach
a debugger to it from another terminal.

Linux:

```
sudo gdb -p $(pgrep -f build/bmmac_sim)
(gdb) thread apply all bt
```

macOS:

```
lldb -p $(pgrep -f build/bmmac_sim)
(lldb) thread backtrace all
```

The backtrace shows where each thread is, in BMMacintosh's and Hatari's
source (the simulator is built with debugging information). When the
emulation is running normally, the main thread is somewhere under
`m68k_run()` (the ST's CPU) or `host_video_present()` (waiting for the next
frame). Type `continue` to let it go on, or `detach` and `quit` to leave it
running.

To run it under the debugger from the start instead, run it from its SD
card folder:

```
cd tools/hostsim/out/live
BMMAC_LIVE=8064 BMMAC_CMDLINE="root=." gdb ../../build/bmmac_sim
(gdb) run < /dev/null
```

and press Ctrl+C in gdb when it's stuck.

### Hatari's own logs

Hatari's settings file on the simulator's SD card (or the Pi's),
`hatari.cfg`, has a `[Log]` section: `nTextLogLevel = 4` logs everything
Hatari has to say (it's 3, warnings and information, by default), to the
same place as BMMacintosh's messages.

### Scripts

The simulator can also run a script of key presses, mouse moves and
screenshots on its standard input, with time measured in frames, so that a
problem can be repeated exactly. For example:

```
cd tools/hostsim/out/live
printf '1500 shot desktop.ppm\n1510 key 0x45 1\n1512 key 0x45 0\n1540 shot menu.ppm\n1541 quit\n' |
    BMMAC_CMDLINE="root=." ../../build/bmmac_sim
```

saves EmuTOS's desktop at frame 1500, presses F12 (USB HID usage 0x45) and
saves the menu at frame 1540. The commands are listed at the top of
`tools/hostsim/host_sim.c`, and `tools/hostsim/run_tests.sh` has many
examples.

Useful settings, as environment variables:

| Variable                        | Does                                                       |
|---------------------------------|------------------------------------------------------------|
| `BMMAC_CMDLINE`             | Options as in `cmdline.txt` (`root=.` is the SD card)      |
| `BMMAC_REFRESH`             | The display's refresh rate (default 60)                    |
| `BMMAC_SIM_RATE`            | The sound output's rate (48000 for a Pi 4 or 5's HDMI)     |
| `BMMAC_IP`                  | Pretend the network is up with this address (from frame 100) |
| `BMMAC_WIFI`                | Pretend Wi-Fi is running, with a few networks to choose from |
| `BMMAC_REFRESH_LATER`       | Change the display's refresh rate to this at frame 30      |

## On the Pi itself

On the Pi there is no debugger or strace, but there is a log. Add
`enable_logging=file` to the line in `cmdline.txt`, and BMMacintosh writes
`bmmac.log` in the root of the SD card (and to the serial port: GPIO14
TX and GPIO15 RX at 115200 baud, for a USB serial adapter, which keeps
working even when the SD card doesn't).

What the log shows:

- The Pi, the display and sound, the TOS images found and the machine.
- Hatari's own messages: the TOS loaded, disks inserted, written back and
  ejected, a GEMDOS drive's folder.
- If the emulation stops moving for 3 seconds, `Emulator has not moved for
  3 s; last seen:` and what it was doing. That line is the most useful one
  for a hang.

When something goes wrong, the most helpful report has:

1. What you did, step by step, and what happened.
2. The BMMacintosh version and which Pi.
3. `bmmac.log` from the card, with `enable_logging=file` (taking the
   card out right after the problem; the log is written as it goes).
4. Your `cmdline.txt`, `bmmac.cfg` and `hatari.cfg`.
5. If it involves a disk or program, which one, or the file if it's free
   to share.
