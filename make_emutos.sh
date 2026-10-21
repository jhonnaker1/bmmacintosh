#!/bin/bash
#
# Puts EmuTOS, the free TOS, on the SD card: tos/etos1024k.img (the 1024K
# build, which Hatari runs on every machine, ST to Falcon) and its licence.
#
#   ./make_emutos.sh OUTPUT_DIRECTORY
#
# The release is downloaded from SourceForge (where the EmuTOS project
# publishes it) the first time, checked against the pinned SHA-256, and
# cached in ~/.cache/bmatarist/emutos.
#

set -euo pipefail

EMUTOS_VERSION=1.4
EMUTOS_ZIP=emutos-1024k-$EMUTOS_VERSION.zip
EMUTOS_SHA256=dc9fbef6455a24ee8955cccd565588c718ba675fd54bc5a749003ac4bbd7f7e1
EMUTOS_URL=https://downloads.sourceforge.net/project/emutos/emutos/$EMUTOS_VERSION/$EMUTOS_ZIP
EMUTOS_CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/bmatarist/emutos"

if [ "$#" -ne 1 ]
then
    echo "Usage: $0 OUTPUT_DIRECTORY" >&2
    exit 2
fi

OUTPUT_DIRECTORY=${1%/}
zip_file="$EMUTOS_CACHE_DIR/$EMUTOS_ZIP"

sha256() {
    if command -v sha256sum >/dev/null 2>&1
    then
        sha256sum "$1" | cut -d' ' -f1
    else
        shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

if [ ! -s "$zip_file" ] || [ "$(sha256 "$zip_file")" != "$EMUTOS_SHA256" ]
then
    mkdir -p "$EMUTOS_CACHE_DIR"
    echo "Downloading EmuTOS $EMUTOS_VERSION"
    curl -fsSL -o "$zip_file.part" "$EMUTOS_URL"
    if [ "$(sha256 "$zip_file.part")" != "$EMUTOS_SHA256" ]
    then
        rm -f "$zip_file.part"
        echo "$EMUTOS_ZIP is not the release BMAtariST expects (SHA-256)." >&2
        exit 1
    fi
    mv "$zip_file.part" "$zip_file"
fi

# Only the two files needed, into a fresh folder.
unpack_dir=$(mktemp -d "$EMUTOS_CACHE_DIR/.unpack.XXXXXX")
trap 'rm -rf "$unpack_dir"' EXIT
unzip -q -o "$zip_file" "emutos-1024k-$EMUTOS_VERSION/etos1024k.img" \
    "emutos-1024k-$EMUTOS_VERSION/doc/license.txt" -d "$unpack_dir"

mkdir -p "$OUTPUT_DIRECTORY/tos" "$OUTPUT_DIRECTORY/licenses"
cp "$unpack_dir/emutos-1024k-$EMUTOS_VERSION/etos1024k.img" "$OUTPUT_DIRECTORY/tos/"
cp "$unpack_dir/emutos-1024k-$EMUTOS_VERSION/doc/license.txt" \
    "$OUTPUT_DIRECTORY/licenses/emutos.txt"
echo "EmuTOS $EMUTOS_VERSION: tos/etos1024k.img"
