#!/bin/bash
#
# Runs BMAtariST in the host simulator in real time, shown in a web page:
#
#   tools/hostsim/live.sh [TOS_DIRECTORY] [PORT]
#
# then open http://127.0.0.1:PORT/ (default port 8064). The simulator's SD
# card is tools/hostsim/out/live, set up as the Pi's: EmuTOS in its tos
# folder (with the TOS images in TOS_DIRECTORY, if given), floppy images in
# disks, hard disk folders and images in hd. Choosing Reboot (or Shut Down)
# in the menu starts it again. Ctrl+C stops it.
#
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
TOS_DIR=${1:-}
PORT=${2:-8064}
CARD=$HERE/out/live

make -s -C "$HERE" -j4 || exit 1

mkdir -p "$CARD"/{disks,hd,snapshots}
"$HERE/../../make_emutos.sh" "$CARD" > /dev/null || exit 1
if [ -n "$TOS_DIR" ]; then
    cp "$TOS_DIR"/* "$CARD/tos/" 2>/dev/null
fi

cd "$CARD" || exit 1
while true; do
    BMATARIST_LIVE=$PORT BMATARIST_CMDLINE="root=$CARD ${BMATARIST_CMDLINE:-}" \
        "$HERE/build/bmatarist_sim" < /dev/null
    status=$?
    [ $status -ne 0 ] && exit $status
    echo "[live] restarting"
    sleep 1
done
