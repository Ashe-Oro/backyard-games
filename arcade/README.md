Backyard Cornhole Arcade — trackball prototype 0.1
================================================

One shared trackball. Offline. Windows x64 portable build.

PLAY
----
Unzip the whole folder. Run BackyardCornhole.exe. Keep the DLLs, resources,
and other files beside it. No Node.js, browser, account, or internet is needed.

Roll sideways to position the bag. Roll back, then forward to throw.
Forward direction aims. Differing back/forward angles add modest gesture spin,
inspired by the cabinet's A/B/C and 1/2/3 diagram. This is an approximation for
cornhole, not a reproduction of Golden Tee's ball physics.

Default buttons:
  H       cycle Push / Normal / Lob (also selects a highlighted menu item)
  Enter   confirm / next round
  Escape  pause / menu
  1       start practice from menu
  2       start a two-player match from menu
  F2      cabinet setup
  Alt+F4  exit

Menu choices follow the trackball cursor; press the mapped button to select.
The three main buttons can be remapped in Cabinet Setup. For keyboard-encoded
arcade buttons, select a binding and press the physical button. USB gamepad
buttons are supported too: button 1 selects / changes height, button 2 pauses,
and Start begins a two-player game from the menu. These can be remapped in
Cabinet Setup; no external keyboard mapper is required. Mouse clicks work for menus.
A short quiet period between turns prevents residual ball motion from throwing.

CABINET SETUP
-------------
Calibrate Trackball: make one comfortable back-and-forward stroke. This sets
that backswing to approximately 70% of the distance power range. Sensitivity
and pull range can also be adjusted manually. Settings save on this machine.

STROKE: 80% backswing distance + up to 20% averaged forward speed.
DISTANCE: backswing distance alone. Try both with the same physical strokes.
Gesture Spin: turn down to 0 to isolate direction and power during testing.
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
strokes. Compare STROKE vs DISTANCE. Try A→3, B→2, C→1 for straight stroke
pairs; A→1 / C→3 to feel the opposite-direction spin. Then play a two-player
round and check that carrying over ball motion never throws for the next player.
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
