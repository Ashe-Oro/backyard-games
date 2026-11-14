# Backyard Cornhole Arcade 0.2.22 follow-up

Replaced the bottom-center prompt and large How to play dialog with a compact
bottom-right panel: eight glossy red buttons, raised top middle pair, level
bottom row, short labels, and tiny aim/height/surface readouts. KEYS collapses
the panel to a tiny tab. Shared legend.mjs/legend.css also supply the offline
legend-shots.html fixture. No reference photo is shipped.

Pad 0 top 4 / bottom 4 (indices 6/7) emit ToggleKeys on rising edges, in menus
and play, rather than reset. They are reserved even with saved bindings.
Top 3 / bottom 3 retain reset; all other pads and controls retain their maps.
The companion localStorage key backyard-arcade-keys-tucked restores the choice
on load, defaulting popped. Storage failure leaves the toggle usable.

Preserved 4000 default speed, 80 ms linear power window, factory/custom profile
migrations, 3000 ms last-shot linger, FULL POWER label, calibration copy,
hidden/empty diagnostics, version 0.2.22, and app -> resources-app symlink.
READ-ME describes the legend and KEYS; index/observed notes stay in DEV-NOTES.

Validation: both required commands pass: default isolation reports 4/4 test
files; --test-isolation=none reports 39/39 individual tests. Tests cover native
and polled rising edges, actual key-handler toggles, persistence restoration,
reset preservation, unchanged placement, fixture parity, and old-card removal.
node --check passes for all edited JS/MJS and extracted HTML module code;
HTML import maps are excluded.

Screenshots: Chrome exists but capture is blocked by sandbox socket permissions
(setsockopt: Operation not permitted). No PNGs were produced. Run the exact
commands in capture-legend.sh from the caller environment:

```sh
sh /workspace/tailgate-games/arcade/cabinet-snapshots/0.2.22/capture-legend.sh
```

```sh
#!/bin/sh
set -eu
fixture=file:///workspace/tailgate-games/arcade/cabinet-snapshots/0.2.22/legend-shots.html
google-chrome --headless --no-sandbox --disable-gpu --user-data-dir=/tmp/p1-legend-capture-popped --window-size=1280,720 --screenshot=/workspace/docs/cornhole/p1-legend-popped.png "$fixture"
google-chrome --headless --no-sandbox --disable-gpu --user-data-dir=/tmp/p1-legend-capture-tucked --window-size=1280,720 --screenshot=/workspace/docs/cornhole/p1-legend-tucked.png "$fixture#tucked"
```

Fixture: arcade/cabinet-snapshots/0.2.22/legend-shots.html
Outputs: /workspace/docs/cornhole/p1-legend-popped.png and
/workspace/docs/cornhole/p1-legend-tucked.png. The reference JPG is untouched.

No push, cabinet install, spending, licensed binary or runtime edits, or earlier
snapshot edits. Work remains on codex/arcade-0.2.22. The environment mounts .git
read-only; commit and installation are left to the caller.
