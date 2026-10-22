#!/bin/bash
#
# Runs BMMacintosh in the host simulator in real time, shown in a web page:
#
#   tools/hostsim/live.sh [ii|plus] [ROMS_FOLDER] [PORT]
#
# then open http://127.0.0.1:PORT/ (default port 8064). ii is the Mac II
# and Quadra (Basilisk II, the default), plus the Mac Plus (umac). The
# simulator's SD card is tools/hostsim/out/live, set up as the Pi's: the
# ROMs in ROMS_FOLDER (MAME's zips, or ROM files) are linked into its roms
# folder, and disk images go in disks. Choosing Reboot (or Shut Down, or
# switching Macs) in the menu starts it again. Ctrl+C stops it.
#
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
MACHINE=${1:-ii}
ROMS_DIR=${2:-}
PORT=${3:-8064}
CARD=$HERE/out/live

case "$MACHINE" in
    ii|plus) ;;
    *) echo "Usage: $0 [ii|plus] [ROMS_FOLDER] [PORT]" >&2; exit 1 ;;
esac

make -s -C "$HERE" -j4 || exit 1

mkdir -p "$CARD"/{roms,disks,shared}
if [ -n "$ROMS_DIR" ]; then
    for f in "$ROMS_DIR"/mac* "$ROMS_DIR"/*.rom "$ROMS_DIR"/*.ROM; do
        [ -f "$f" ] && ln -sf "$f" "$CARD/roms/"
    done
fi

cd "$CARD" || exit 1
while true; do
    BMMAC_LIVE=$PORT BMMAC_CMDLINE="root=$CARD ${BMMAC_CMDLINE:-}" \
        "$HERE/build/mac${MACHINE}_sim" < /dev/null
    status=$?
    [ $status -ne 0 ] && exit $status
    echo "[live] restarting"
    sleep 1
done
