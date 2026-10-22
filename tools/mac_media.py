#!/usr/bin/env python3
#
# mac_media.py - makes BMMacintosh's ROM and disk files from MAME's.
#
# BMMacintosh reads MAME's zipped ROM sets as they are (put them in the
# card's roms folder), so this is only needed for a plain ROM file, or for
# another emulator:
#
#   tools/mac_media.py roms MAME_ROMS_FOLDER_OR_ZIPS... -o OUT_FOLDER
#
# writes each Mac ROM found (whole, or put together from MAME's chips, as
# src/port/roms.c does) as OUT_FOLDER/<checksum> <machine>.rom.
#
# MAME's hard disks are CHD files, which BMMacintosh can't read:
#
#   tools/mac_media.py disk DISK.chd -o DISK.hda [--volume] [--chdman PATH]
#
# makes a raw disk image of one with MAME's chdman (on PATH, or --chdman).
# The Mac II (Basilisk II) starts from it as it is. The Mac Plus (umac)
# needs the HFS volume alone, without the disk's partition map: --volume
# writes just that (a .dsk). An .hda can be given instead of a .chd.
#
# Apple's ROMs and system software aren't free: keep what this makes for
# your own use.
#
# This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
# the Raspberry Pi. See LICENSE for licensing details.
#

import argparse
import itertools
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile

KNOWN = {
    0x28BA61CE: "Macintosh 128K",
    0x28BA4E50: "Macintosh 512K",
    0x4D1EEEE1: "Macintosh Plus v1",
    0x4D1EEAE1: "Macintosh Plus v2",
    0x4D1F8172: "Macintosh Plus",
    0xB2E362A8: "Macintosh SE",
    0xB306E171: "Macintosh SE FDHD",
    0xA49F9914: "Macintosh Classic",
    0x97851DB6: "Macintosh II",
    0x9779D2C4: "Macintosh II FDHD",
    0x97221136: "Macintosh IIx",
    0x368CADFE: "Macintosh IIci",
    0x36B7FB6C: "Macintosh IIsi",
    0x4147DD77: "Macintosh IIfx",
    0x350EACF0: "Macintosh LC",
    0x35C28F5F: "Macintosh LC II",
    0x3193670E: "Macintosh Classic II",
    0xECBBC41C: "Macintosh LC III",
    0xECD99DC0: "Macintosh Color Classic",
    0xEDE66CBD: "Macintosh LC 550",
    0x420DBFF3: "Quadra 700",
    0x3DC27823: "Quadra 950",
    0xF1A6F343: "Centris 650",
    0xF1ACAD13: "Quadra 650",
    0xFF7439EE: "Quadra 605",
    0x064DC91D: "Performa 580",
    0x5BF10FD1: "Quadra 840AV",
}


def checksum_ok(data):
    """A Mac ROM's first four bytes are the sum of its other 16-bit words."""
    if len(data) < 0x10000 or len(data) & (len(data) - 1):
        return False
    words = struct.unpack(">%dH" % ((len(data) - 4) // 2), data[4:])
    return sum(words) & 0xFFFFFFFF == struct.unpack(">I", data[:4])[0]


def interleave(chips):
    out = bytearray(len(chips[0]) * len(chips))
    for k, chip in enumerate(chips):
        out[k::len(chips)] = chip
    return bytes(out)


def roms_in_zip(path):
    """Whole ROMs, and ROMs in two or four chips, as roms.c finds them."""
    found = []
    with zipfile.ZipFile(path) as z:
        files = {i.filename: z.read(i) for i in z.infolist()
                 if 0x8000 <= i.file_size <= 0x100000}
    chips = {}
    for name, data in files.items():
        if checksum_ok(data):
            found.append((name, data))
        elif len(data) <= 0x80000:
            chips[name] = data
    for a, b in itertools.permutations(chips, 2):
        if len(chips[a]) == len(chips[b]):
            rom = interleave([chips[a], chips[b]])
            if checksum_ok(rom):
                found.append((a + "+" + b, rom))
    if len(chips) == 4 and len({len(d) for d in chips.values()}) == 1:
        for order in itertools.permutations(chips, 4):
            rom = interleave([chips[n] for n in order])
            if checksum_ok(rom):
                found.append(("+".join(order), rom))
                break
    return found


def cmd_roms(args):
    sources = []
    for src in args.sources:
        if os.path.isdir(src):
            sources += [os.path.join(src, f) for f in sorted(os.listdir(src))
                        if f.lower().endswith(".zip")]
        else:
            sources.append(src)
    os.makedirs(args.output, exist_ok=True)
    count = 0
    for src in sources:
        try:
            roms = roms_in_zip(src)
        except (zipfile.BadZipFile, OSError) as e:
            print("%s: %s" % (src, e), file=sys.stderr)
            continue
        for name, data in roms:
            checksum = struct.unpack(">I", data[:4])[0]
            machine = KNOWN.get(checksum, "unknown")
            out = os.path.join(args.output, "%08X %s.rom" % (checksum, machine))
            with open(out, "wb") as f:
                f.write(data)
            print("%s: %s -> %s" % (os.path.basename(src), name, out))
            count += 1
    print("%d ROM(s) written" % count)
    return 0 if count else 1


def hfs_partition(image):
    """The first Apple_HFS partition's (start, length) in bytes, or None."""
    with open(image, "rb") as f:
        block0 = f.read(512)
        if block0[:2] != b"ER":
            return None
        entry = f.read(512)
        if entry[:2] != b"PM":
            return None
        count = struct.unpack(">I", entry[4:8])[0]
        for i in range(count):
            if i:
                entry = f.read(512)
            start, length = struct.unpack(">II", entry[8:16])
            kind = entry[48:80].split(b"\0")[0]
            if kind == b"Apple_HFS":
                return start * 512, length * 512
    return None


def cmd_disk(args):
    source = args.source
    tmp = None
    if source.lower().endswith(".chd"):
        chdman = args.chdman or shutil.which("chdman")
        if not chdman:
            print("chdman (MAME's) is needed for a CHD: put it on PATH or give --chdman",
                  file=sys.stderr)
            return 1
        tmp = tempfile.mkdtemp()
        raw = os.path.join(tmp, "disk.hda")
        subprocess.run([chdman, "extracthd", "-i", source, "-o", raw, "-f"], check=True)
        source = raw
    try:
        if not args.volume:
            shutil.copyfile(source, args.output)
            print("%s: the whole disk" % args.output)
            return 0
        part = hfs_partition(source)
        if part is None:
            print("%s has no Apple partition map with an HFS volume" % args.source,
                  file=sys.stderr)
            return 1
        start, length = part
        with open(source, "rb") as f, open(args.output, "wb") as out:
            f.seek(start)
            left = length
            while left:
                chunk = f.read(min(left, 1 << 20))
                if not chunk:
                    break
                out.write(chunk)
                left -= len(chunk)
        print("%s: the HFS volume, %d MB" % (args.output, length >> 20))
        return 0
    finally:
        if tmp:
            shutil.rmtree(tmp)


def main():
    p = argparse.ArgumentParser(description="BMMacintosh's ROM and disk files from MAME's")
    sub = p.add_subparsers(dest="command", required=True)
    r = sub.add_parser("roms", help="plain ROM files from MAME's zipped sets")
    r.add_argument("sources", nargs="+", help="zips, or folders of them")
    r.add_argument("-o", "--output", required=True, help="folder for the .rom files")
    d = sub.add_parser("disk", help="a raw disk image from a MAME CHD (or .hda)")
    d.add_argument("source")
    d.add_argument("-o", "--output", required=True)
    d.add_argument("--volume", action="store_true",
                   help="the HFS volume alone, for the Mac Plus")
    d.add_argument("--chdman", help="MAME's chdman")
    args = p.parse_args()
    return cmd_roms(args) if args.command == "roms" else cmd_disk(args)


if __name__ == "__main__":
    sys.exit(main())
