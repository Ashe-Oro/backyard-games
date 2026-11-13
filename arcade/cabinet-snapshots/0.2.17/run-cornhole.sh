#!/bin/sh
ARCADE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export DISPLAY="${DISPLAY:-$(getLocalXDisplay)}"
export LD_LIBRARY_PATH="$ARCADE_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$ARCADE_DIR" || exit 1
exec ./electron --no-sandbox --ozone-platform=x11 --disable-dev-shm-usage "$@"
