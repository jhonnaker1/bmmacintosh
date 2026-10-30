#!/bin/bash
#
# Tests the shared Pi 4/5 ARM smooth scaling and CRT shader (src/av/scaler.*)
# how a frame is shared between cores (src/av/frameshare.h), and where the
# picture goes on the display (src/av/layout.h) on the PC.
# The pictures go to tools/pi4scaler/out/.
#
#   tools/pi4scaler/run_tests.sh
#

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
make -s -C "$HERE"
rm -rf "$HERE/out"
mkdir -p "$HERE/out"
"$HERE/build/pi4scaler_test" "$HERE/out"
echo
"$HERE/build/layout_test"
echo
# ThreadSanitizer stops on some newer Linux kernels' address layout; run it
# without address space randomisation where setarch is available.
if command -v setarch > /dev/null
then
    setarch "$(uname -m)" -R "$HERE/build/share_test"
else
    "$HERE/build/share_test"
fi
