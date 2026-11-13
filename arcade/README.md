Backyard Cornhole Arcade — 0.2.15
================================================

Offline Cornhole for one shared trackball. Practice, two players, or the
standard eight-rival tournament. The Batocera cabinet uses the native Linux
build under Ports. The website and its saves are separate.

PLAY
----
Optional backswing: A (back-left) sets clockwise spin and slides right;
C (back-right) sets counterclockwise spin and slides left. B (straight back)
or no backswing gives zero spin. Lean scales spin continuously to maximum at
a diagonal. Forward 1 (left) and 3 (right) still aim left and right; forward
speed sets power. Spin rotates the bag and curls its landing slide, not its
flight. Sideways rolls can still place the bag. Backspin intent expires after
2.5 seconds idle or on reset/turn change. Back-to-forward can pause briefly.

| P1 button | Index | In play | In menus |
|---|---:|---|---|
| Bottom 1 | 0 | Cycle Slider / Normal / Kobe | Confirm |
| Bottom 2 | 1 | Cycle Slick / Sticky | Back / menu |
| Bottom 3 | 2 | Position left, 0.10 step | No action |
| Bottom 4 | 3 | Position right, 0.10 step | Confirm |
| Top 1 | 4 | Unbound | Back / menu |
| Top 2 | 5 | Reset ready bag and stroke to center | No action |
| Top 3 | 6 | Unbound | No action |
| Top 4 | 7 | Unbound | No action |

Start/Select indices 8/9 retain pause/confirm behavior. Explicit Cabinet Setup
bindings override defaults. Other pads retain their prior maps. One action per
press; holding a button does not repeat. Position clamps to [-0.45, 0.45] with
70 ms easing. Reset leaves thrown bags and scores alone.

Keyboard fallback: H height, F surface, R reset, Enter select, Escape pause.
F2 opens Cabinet Setup. Alt+F4 exits.

Reset Bag centers the unthrown bag and clears its windup without consuming a
throw, clearing the board, or changing scores. The gauge beside the bag shows
actual throw power and keeps the previous shot visible for five seconds.
The live board close-up is always on in play. Match and round points remain
visible; round totals recount as bags settle.

CALIBRATION
-----------
Bindings and sensitivity carry over. Legacy spin OFF settings enable backswing
spin on load; Cabinet Setup labels this Backswing spin. Human throw power is
linear in accumulated forward trackball counts after sensitivity: 0 = 0%,
200 = 50%, and 400 = 100%, regardless of report speed. Full-power counts are
fixed at 400; Release counts defaults to 400 and is adjustable from 400–1200.
A throw requires multiple reports at distinct timestamps, then one shot per gesture.
Old speed-based powerSpeed values are discarded and release gates below 400
are raised to 400 without resetting button bindings or sensitivity.
Calibrate Trackball records a reference stroke through 400 current counts and
adjusts sensitivity so its measured distance (including the final report) maps
to 400 counts / full power. It does not measure counts per second.

Slow sideways motion tolerates small downward sensor noise and eases into its
position. A quiet period after each turn prevents residual motion launching
the next bag. No device is grabbed and no global controller mapping is edited.

NATIVE BATOCERA PACKAGE
----------------------
Back up the existing package and metadata. Stage and verify the Linux ZIP
outside the ROM collection before replacing its files while Cornhole is closed.
Runtime: /userdata/system/backyard-games/BackyardCornhole-linux
Ports launcher: /userdata/roms/ports/BackyardCornhole.sh
Log: /userdata/system/logs/cornhole-native.log
Music: /userdata/music/backyard-cornhole (or choose a local folder in Music).
The runtime uses a private bundled libcups; it does not replace system libraries.

The launcher releases only Cornhole's stranded virtual exit keys before handing
back to Batocera. Batocera-Support contains the separately reviewed gameStop
hook for /userdata/system/scripts/90-release-virtual-hotkeys.sh. It releases
hotkeygen's virtual keys at game exit, never physical keys or input mappings.
Keep the existing installed hook when updating Cornhole. Backups and successful
Cornhole/Sega exit tests were preserved in the cabinet's es-diagnosis cache.

The obsolete Windows/Wine package is not the cabinet's supported launcher.
Prepare a recoverable backup outside scanned ROM folders before removing it.
Do not remove other Windows games or Wine prefixes. Artwork and metadata are
prepared in media/; their installation and Favorites verification remain pending.

DEVELOPMENT AND QA
------------------
From arcade/: npm ci; npm test; npm run build:linux.
Optional local Mac package: node build.mjs --mac.
Browser/Electron regression scripts live in tests/. They accept NODE_PATH for
the test driver, ELECTRON_PATH for the packaged app, CHROME_PATH for Chromium,
and BASE_URL for the generated site. No test fixture is included in the game.

The build generates an isolated offline copy under arcade/dist/site. The build never edits canonical
web files. Vercel excludes the whole arcade directory. Cabinet
preferences use the standalone app's own profile. Preserve that profile during
updates so calibration, mappings, music choice and tournament progress survive.

Read BACKLOG.md for implemented work, verification and the remaining physical
playtest/installation tasks. Do not report the cabinet updated from a local build.
