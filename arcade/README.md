Backyard Cornhole Arcade — trackball prototype 0.2.4
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

STROKE: 65% backswing distance + up to 35% averaged forward speed.
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

## Batocera input

Linux builds use the cabinet's existing Python `evdev` package to read relative mouse counts and gamepad buttons directly. No browser pointer lock is needed. Devices are rediscovered every two seconds; no input devices are grabbed and no system controller mappings are changed. The mini-keyboard mouse remains usable in menus. Calibration uses raw device counts rather than accelerated cursor pixels.

On this cabinet, B (button 3) confirms menu selections and cycles SLIDER / NORMAL / KOBE in play. A (button 4) goes back in menus and flips SLICK / STICKY in play. Start confirms in menus and pauses in play; Select pauses. Either player's panel works. Use the joystick to navigate menus, or hover a button with the trackball and press B. Bindings can be changed in Cabinet Setup.

Cabinet Setup displays connected mice, controller count, and movement totals. Roll back then forward in Calibrate Trackball to set the pull range. Existing sensitivity and calibration settings are retained. Keyboard controls remain available. Windows/macOS continue to use relative pointer capture and the browser Gamepad API.

### Golden Tee reference

The cabinet's Golden Tee '98 config inherits the MAME 2003-Plus default action pair with no per-game input remap. RetroPad B/A correspond to raw cabinet buttons 3/4; its global RetroArch mouse index 1 selects the trackball instead of the mini-keyboard touchpad. Golden Tee's default action sequences also include mouse buttons 1/2. Cornhole therefore supports both paths: in play, left mouse button cycles throw style and right mouse button flips bag surface. In menus, normal left-click selects the hovered item and right-click goes back. Dedicated trackball-panel button wiring still needs a physical press test; the configuration alone cannot identify which blue button sends which event.

## Web / arcade boundary

The web game in `cornhole/` is the canonical input for packaging. Desktop-only input, physics adjustments, UI and dependencies live in `arcade/`. The builder copies web files into `arcade/dist/site`, applies the arcade adaptations there, and starts from a clean generated tree on each build. Never apply cabinet changes directly to the web files or point a web server/Vercel output directory at the arcade build.

The root `.vercelignore` excludes the entire `arcade` directory, including its dependencies and generated binaries, from web deployments. The website does not import arcade scripts/styles or expose an arcade-mode switch. The controller requires both the generated-page marker and arcade host bridge before it touches settings, DOM or input listeners.

Desktop storage is under Electron's existing Backyard Cornhole Arcade profile and the `backyard://game` origin; browser saves remain on their HTTPS website origin. The app's offline protocol and network block keep web deployments out of the installed cabinet game. Packaging uses read-only copies of shared assets, so a website deployment cannot change an installed arcade build; a new arcade package must be built and installed separately.

Run `npm test --prefix arcade` before publishing a change. Isolation regression checks hash the web sources before/after a real arcade build, reject arcade imports in web sources, verify stale generated files are removed and check that the deployment exclusion remains present. Existing control tests also run.

## Cabinet playtest pass (0.2.4)

- Centered match scoreboard stays visible through throws, pauses, setup and round results. It shows match totals, live round points and remaining bags. Practice shows points on the current board.
- Arcade target moves back two feet (29 ft). Its geometry, aiming and simulation-derived power windows all use the same new distance.
- Factory stroke calibration is now 320 counts back / 80 counts forward. Stroke power uses 65% pull distance plus 35% sustained forward speed, so a deliberate forward roll has more effect than a soft reversal. Custom measured sensitivity/range and button bindings survive the settings migration; only old factory values upgrade. The v2 save is retained for rollback, and new settings use v3.
- A hole score or a knocked-in bag triggers one of three dog celebrations: hops, a victory spin, or a playful bow. Poses return to idle without drift, and pausing freezes the animation with the game.

Verification note: the 0.2.4 native input-bridge and scoreboard/celebration tests pass in Electron. The separate macOS pointer-lock smoke test currently fails to acquire capture on this host; Batocera uses direct evdev input and does not depend on that path. Physical stroke feel still needs cabinet playtesting.
