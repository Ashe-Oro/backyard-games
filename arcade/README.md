Backyard Cornhole Arcade — trackball prototype 0.2.1
================================================

One shared trackball. Offline. Windows x64 portable build.

PLAY
----
Unzip the whole folder. Run BackyardCornhole.exe. Keep the DLLs, resources,
and other files beside it. No Node.js, browser, account, or internet is needed.

Roll sideways to position the bag. Roll back, then forward to throw.
Backswing distance sets power; the forward stroke sets release direction.
Backswing angle never creates a hook or slice. Wind can move the bag in flight.
Optional curved-finish rotation is off by default; it affects the bag on the
board, not its airborne trajectory. Throw style and bag surface use buttons.

Default buttons:
  H       cycle Slider / Normal / Kobe (also selects a highlighted menu item)
  F       flip Slick / Sticky bag surface
  Enter   confirm / next round
  Escape  pause / menu
  1       start practice from menu
  2       start a two-player match from menu
  F2      cabinet setup
  Alt+F4  exit

Menu choices follow the trackball cursor; press the mapped button to select.
The four main buttons can be remapped in Cabinet Setup. For keyboard-encoded
arcade buttons, select a binding and press the physical button. USB gamepad
buttons are supported too: button 1 selects / changes throw style; button 2
flips bag surface during play. Select / Back pauses; Start starts two-player
from the menu and pauses during play. These can be remapped in
Cabinet Setup; no external keyboard mapper is required. Mouse clicks work for menus.
A short quiet period between turns prevents residual ball motion from throwing.

CABINET SETUP
-------------
Calibrate Trackball: make one comfortable back-and-forward stroke. This sets
that backswing to approximately 70% of the distance power range. Sensitivity
and pull range can also be adjusted manually. Settings save on this machine.

STROKE: 80% backswing distance + up to 20% averaged forward speed.
DISTANCE: backswing distance alone. Try both with the same physical strokes.
Finish rotation is OFF by default. If enabled, a curved forward finish adds
a small landing effect. A straight diagonal stroke simply aims diagonally.
Slider travels low and slides more; Normal is the standard arc; Kobe is high
and lands with less slide. Slick slides farther; Sticky grips the board.
Each player keeps their own throw style and surface between turns.
Calm is the default. Wind can be enabled after calibrating the controls.
Diagnostics show device counts, FPS, and the last shot. These are setup-only.

BATOCERA / WINE
---------------
Copy the extracted BackyardCornhole.pc folder into:
  /userdata/roms/windows/
Refresh the game list. Launch from the Windows collection.
The included autorun.cmd uses Linux line endings and starts the executable.
This is a Windows game for Wine, not a MAME ROM.

If the screen is black, try the software renderer: replace autorun.cmd with
  CMD=BackyardCornhole.exe --software
The autorun-software.example.cmd file has that content. On Windows, run the
Software renderer.bat launcher instead. Software rendering may be slower.
Use Batocera's existing exit shortcut, or map an accessible key to pause and
choose Exit Game. Do not remap away your cabinet's global exit shortcut.

This package was built on macOS. The offline game, relative-input controller,
and Electron runtime are tested locally; Windows/Wine and your physical
trackball still require on-cabinet validation. Unsigned prototype, no installer.
Wine/Chromium compatibility is a feasibility gate, not a promised final engine.

TRY THIS
--------
Start in Practice with calm air. Make five short, five medium, and five long
strokes. Compare STROKE vs DISTANCE. Test each throw style on Slick and Sticky,
then angle the forward stroke to aim left/right. Play a two-player round and
check that each player retains their settings and leftover motion never throws
for the next player.
Report Batocera version, CPU/GPU, screen resolution, power mode, settings, and
whether aim/power feel repeatable. Cabinet Setup shows counts to help diagnose.

DEVELOPMENT
-----------
From arcade/: npm ci --ignore-scripts; npm test; npm run build:windows.
Offline browser test: npm run build:site, serve dist/site, open /cornhole/.
For a local Electron host, install its binary (npm rebuild electron) and npm start.
The build patches a COPY of the existing Cornhole game. Production web controls,
physics, and assets remain untouched. Only the cabinet copy enables human spin.
Electron, Three.js, and the font licenses are included with the portable build.

NATIVE BATOCERA COMPANION (VERIFIED ON CABINET)
--------------------------------------------
The Windows/Electron wrapper encountered Wine graphics failures on Batocera 42.
The same offline game runs using its native Linux companion instead. From the
Linux ZIP, put BackyardCornhole-linux/ in /userdata/system/backyard-games/ and
BackyardCornhole.sh in /userdata/roms/ports/. Make both shell launchers and the
electron binary executable. Refresh the game list and launch from Ports.

The companion privately bundles the missing libcups.so.2 and its license. It
uses the cabinet's existing GTK, NSS and Intel OpenGL libraries; it does not
replace system libraries or change other games' settings. The launcher uses
--no-sandbox because Batocera runs games as root. Content stays local/offline.
Logs: /userdata/system/logs/cornhole-native.log.
Build: npm run build:linux. Requires tar with xz support, Node, and npm dependencies.

Cabinet validation: native process and GPU renderer started; screenshot confirmed
the Cornhole menu and backyard rendered on screen. Physical trackball feel and
button calibration still need the player test. The Windows version remains
available for Windows PCs; its Wine compatibility is not established.
