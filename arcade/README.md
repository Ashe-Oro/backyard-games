Backyard Cornhole Arcade — 0.2.13
================================================

Offline Cornhole for one shared trackball. Practice, two players, or the
standard eight-rival tournament. The Batocera cabinet uses the native Linux
build under Ports. The website and its saves are separate.

PLAY
----
Roll sideways to place the ready bag. Roll forward to throw, angling left or
right to steer the bag. Faster forward movement supplies more power. Bag
rotation affects landing, not airborne curvature.

B / raw button 3: select in menus; cycle Slider / Normal / Kobe in play.
A / raw button 4: back in menus; flip Slick / Sticky in play.
Raw 2 or 5: Reset Bag in play. This is remappable; confirm the physical label.
Start: select in menus; pause in play. Select: pause. Either player panel works.
Keyboard fallback: H height, F surface, R reset, Enter select, Escape pause.
F2 opens Cabinet Setup. Alt+F4 exits.

Reset Bag centers the unthrown bag and clears its windup without consuming a
throw, clearing the board, or changing scores. The gauge beside the bag shows
actual throw power and keeps the previous shot visible for five seconds.
The live board close-up is always on in play. Match and round points remain
visible; round totals recount as bags settle.

CALIBRATION
-----------
Bindings and sensitivity carry over. Older profiles get the wider power range,
with a default 220-count stroke and 6,500 counts/sec for full power. Both
stroke length and full-power speed can be adjusted in Cabinet Setup without
resetting bindings. Calibrate Trackball measures a comfortable full stroke as
approximately 70% power.

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

The build generates an isolated offline copy under arcade/dist/site. Canonical
web files are never edited. Vercel excludes the whole arcade directory. Cabinet
preferences use the standalone app's own profile. Preserve that profile during
updates so calibration, mappings, music choice and tournament progress survive.

Read BACKLOG.md for implemented work, verification and the remaining physical
playtest/installation tasks. Do not report the cabinet updated from a local build.
