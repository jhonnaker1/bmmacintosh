# Updating Basilisk II, umac, zlib and Circle

## Basilisk II (macemu)

Basilisk II is a git submodule in `third_party/macemu`, from
[kanjitalk755's macemu](https://github.com/kanjitalk755/macemu), the
maintained Basilisk II and SheepShaver. BMMacintosh builds
`BasiliskII/src` with its own makefile list and front end:

| File | What it has to do with Basilisk II |
|---|---|
| `src/port/ii/basilisk.mk` | The Basilisk II sources compiled, and their options, from `BasiliskII/src/Unix/Makefile.in` and `configure.ac` (the ARM choices: `uae_cpu_2021`, `UPDATE_UAE`) |
| `src/port/ii/sysdeps.h` | Basilisk II's system definitions, which its configure writes for Unix (`src/Unix/sysdeps.h`): direct addressing, no VOSF or JIT, the memory accessors that keep the Mac inside its memory |
| `src/port/ii/main_ii.cpp` | Its `main_unix.cpp`: memory, the ROM, `InitAll()`, `Start680x0()`, and the frame loop in `cpu_do_check_ticks()` |
| `src/port/ii/video_ii.cpp`, `audio_ii.cpp`, `sys_ii.cpp`, `timer_ii.cpp`, `prefs_ii.cpp` | The platform functions Basilisk II calls (its `video_*.cpp`, `audio_*.cpp`, `sys_unix.cpp`, `timer_unix.cpp`, `prefs_unix.cpp`, `xpram_unix.cpp`) |
| `src/port/ii/extfs_ii.cpp` | Its `Unix/extfs_unix.cpp`, changed for the SD card's FAT |
| `src/port/ii/ether_ii.cpp` | Its `Unix/ether_unix.cpp`'s slirp parts: the Ethernet driver's platform functions, with slirp in the background |
| `src/port/ii/slirp/`, `src/slirp_sockets.cpp` | What slirp (`BasiliskII/src/slirp`) asks of the system: its `config.h`; on the Pi, the socket calls it makes, renamed (`circle_sockets.h`) to ones on Circle's sockets, and the headers newlib lacks (`circle/sys/`). A new slirp calling other system functions shows as a link error for the Pi |

1. Fetch and check out the new commit:

   ```
   cd third_party/macemu
   git fetch origin
   git checkout <commit>
   cd ../..
   ```

2. Compare `BasiliskII/src/Unix/Makefile.in` and `configure.ac` with
   `src/port/ii/basilisk.mk` (source files added or removed, options for
   ARM), `Unix/sysdeps.h` with `src/port/ii/sysdeps.h`, and the platform
   functions in `include/*.h` with `src/port/ii/` (a missing one shows
   when the simulator links). Compare `Unix/extfs_unix.cpp` with
   `src/port/ii/extfs_ii.cpp`.
3. Build and test on the PC first: `tools/hostsim/run_tests.sh` with your
   ROMs and disks (see [SIMULATOR.md](SIMULATOR.md)), and try some
   programs in the live simulator (`tools/hostsim/live.sh ii`). Then build
   the kernels (`./build_sdcard.sh`) and try them on a Pi.
4. Update the commit in `docs/THIRD_PARTY.md` and
   `sdcard/licenses/basilisk2.txt`, and commit.

## umac and Musashi

umac is a submodule in `third_party/umac`, from
https://github.com/evansm7/umac, with Musashi as its own submodule
(`external/Musashi`, umac's copy). `src/port/plus/umac.mk` lists the
sources BMMacintosh builds (not `src/unix_main.c` or the Mini vMac
keyboard tables) and renames umac's `umac_disc_ejected()` away (BMMacintosh
keeps the Mac running with its drive empty instead of resetting it);
`src/port/plus/umac_prefix.h` makes umac's RAM size a variable, and
`src/port/plus/sonydrv.h` is the Mac disk driver umac's `rom.c` puts in
the ROM, BMMacintosh's in place of umac's (it is found first on the
include path). If umac changes how its driver calls the host side
(`disc_pv_hook()` in `src/disc.c`, `PV_SONY_ADDR`), the driver has to
follow.

For a new commit, check out umac and its Musashi
(`git submodule update --init external/Musashi` in it), compare umac's
`Makefile` and `include/umac.h` with `src/port/plus/`, run the tests,
check the licences of any new files (umac and Musashi are MIT, umac's
disk driver Basilisk II's GPL; nothing GPL version 2 only can come in,
such as Mini vMac's), and update the commits in `docs/THIRD_PARTY.md`,
`sdcard/licenses/umac.txt` and `musashi.txt`.

## zlib

zlib is a submodule in `third_party/zlib`, at a release tag (`v1.3.2`),
for MAME's zipped ROM sets (`src/port/roms.c` uses its inflate). Check out
a newer tag, compare its source files with `ZLIB_SRCS` in the `Makefile`
and `tools/hostsim/Makefile`, and update the version in
`docs/THIRD_PARTY.md` and `sdcard/licenses/zlib.txt`.

## circle-stdlib (Circle and newlib)

circle-stdlib is a submodule in `third_party/circle-stdlib`, pinned to the
same revision BMC64 uses. `make_all.sh` applies the patches in `src/patches/`
to Circle before building:

- `circle_sysconfig_patch.diff` enables multi-core and the USB FIQ
- `circle_linux_bug_patch.diff` turns off `BUG_ON`, whose checks in Circle's
  Linux emulation (used by VCHIQ for display and sound) only allow core 0;
  the emulator calls them from core 1, as BMC64 does
- `circle_bolt_keyboard_patch.diff` adds Logitech Bolt keyboard support
  (from BMC64)

To update, move the submodule to a new circle-stdlib release, run
`git submodule update --init --recursive`, then `./build_sdcard.sh`. If a
patch no longer applies, update it from the matching BMC64 patch or by
hand. `src/fbl.cpp` and `src/vchiqsoundbasedevice.cpp` use Circle's VC4
addon and may need the same changes BMC64 makes when it updates Circle.
