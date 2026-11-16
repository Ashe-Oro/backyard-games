#!/bin/sh
set -eu
snapshot=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=${1:-/workspace/docs/cornhole/legends-spin-and-keys.png}
/usr/bin/google-chrome --headless --no-sandbox --disable-gpu --user-data-dir=/tmp/cornhole-spin-legends --window-size=1280,720 --screenshot="$output" "file://$snapshot/spin-legends.html"
