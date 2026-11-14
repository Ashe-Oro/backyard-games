#!/bin/sh
set -eu
fixture=file:///workspace/tailgate-games/arcade/cabinet-snapshots/0.2.22/legend-shots.html
google-chrome --headless --no-sandbox --disable-gpu --user-data-dir=/tmp/p1-legend-capture-popped --window-size=1280,720 --screenshot=/workspace/docs/cornhole/p1-legend-popped.png "$fixture"
google-chrome --headless --no-sandbox --disable-gpu --user-data-dir=/tmp/p1-legend-capture-tucked --window-size=1280,720 --screenshot=/workspace/docs/cornhole/p1-legend-tucked.png "$fixture#tucked"
