# Backyard Cornhole Wii — Changelog

All notable changes to the Wii build are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).
Releases are tagged `wii-vX.Y.Z`. The version source is `VERSION` in `wii/Makefile`,
which must match `hbc/apps/backyardcornhole/meta.xml`, the in-game footer and this file.
This changelog covers the original Wii target (480p/480i, Homebrew Channel zip and ISO).
The Kinhank (Dolphin, 3×) target has its own changelog and `kinhank-vX.Y.Z` tags; see
[../kinhank/CHANGELOG.md](../kinhank/CHANGELOG.md) and [../TARGETS.md](../TARGETS.md).

## [0.1.6] - 2026-11-22

### Changed

- Raised the default swing-speed reference to 3.344 m/s (1.9 g excess), lowered the power gain to 0.55, and capped power at 0.760. A calm, centered NORMAL throw at the cap lands 8.90 m from release.
- SET POWER can still tune an easy toss to the hole. Existing saves at the previous factory speed pick up the new default.

### Fixed

- Starting Practice with B held, or before the active remote reports its B state, no longer grips a bag. Once B is up, a fresh B-down in play grips immediately.

## [0.1.5] - 2026-11-22

### Changed

- Reduced Wii Remote throw power by 25% across the previous linear range. Stronger swings can still reach full power.
- Rounded the keg, beer pong cups and ball, and the cups at the keg.

## [0.1.4] - 2026-11-21

Wii Sports Bowling controls: the D-pad sets up the throw, the Wii Remote's swing does the rest, and the pointer is never used.

- Swing speed is the power and swinging left or right is the aim. Nothing else changes either one.
- Removed the −/+ fine-aim nudge and its `AIM: +N` label. In play, −/+ now do nothing (they no longer reset the swing either). In menus they still adjust settings.
- The arc comes only from the throw-type picker, D-pad up/down: LOB / NORMAL / SLIDER (the bottom bar now says LOB where it said KOBE). The remote's pitch at release never changes the arc.
- D-pad left/right still steps along the foul line. A (surface), 2 (reset), 1 (help), HOME (pause) and the 1+2 overlay are unchanged.
- New help text, one control per line: B HOLD, SWING BACK & FORWARD, LET GO / SWING SPEED = POWER / SWING LEFT/RIGHT TO AIM / TWIST TO CURL / D-PAD UP/DOWN KOBE-NORMAL-SLIDER / D-PAD LEFT/RIGHT MOVE / A SURFACE / 2 RESET / 1 HELP / HOME PAUSE. The bottom bar shows the B state, the picker, the surface and D-pad MOVE.
- The 1+2 overlay adds the last throw's power and aim.
- The version (v0.1.4) shows on the STRAP ON! screen and in the bottom-right corner during play.

## [0.1.3] - 2026-11-21

Throw like Wii Sports Bowling: point the Wii Remote up, hold B, swing back, swing forward and let go of B during the forward swing.

- B grips on the report it goes down, in any pose and while moving. No settle period, no stillness check, no HOLD STILL message.
- A continuous back-and-forward swing now throws on the B-up report. Power comes from the forward swing's speed; lateral motion still aims and wrist twist still curls, mirrored for left hands.
- Letting go of B always throws. With no forward swing to measure (a tap, a release on the backswing, a swing that ended long ago) you get a weak lob, never nothing.
- Late, missing or bursty Wii Remote reports no longer cancel the swing. Gaps up to 100 ms are bridged, gaps up to 250 ms are accepted, and a longer stall restarts the motion measurement but keeps the grip. A press during the bag flight or the turn change grips as soon as the turn opens.
- The 1+2 debug overlay adds the swing phase, B, reports per second, the largest report gap and the last throw's peak speed and power.
- At 16:9 (the console's widescreen setting, Dolphin's default) the HUD is laid out for 16:9: text keeps its proportions, right-side pieces sit on the real screen edge, and the BOARD 27 FT inset moves to the top-left corner, clear of the beer-pong table. 4:3 is unchanged.

## [0.1.2] - 2026-11-21
- Targets: original Wii (480p progressive with a component cable and 480p enabled in Wii Settings, otherwise 480i; 16:9 when the console is set to widescreen) and emulation (Dolphin on the Kinhank). Emulation's GameSettings `BYCEAO.ini` now renders at 3x internal resolution, about 1080p.

- Preserve each Wii Remote report's B state and acceleration through the WPAD queue. Hold B, swing back and forward, then release to throw. Short or gentle forward swings use a low-power release in the measured direction. A stationary B tap cancels.
- Hold 1+2 for two seconds to toggle live accelerometer, B state and swing diagnostics.
- Removed the Wii yard's string lanterns and IR cursor.

## [0.1.1] - 2026-11-18

- Boot with no DSP HLE warning on stock Dolphin 5.0-19870 (Kinhank). Link the official libogc 2.5.0-2 ASND mixer, whose ucode CRC `0xdbbeeb61` (padded `0xbad876ef`) is in that Dolphin's known list, instead of libogc 3.2.1's CRC `0x5dbf8bf1`.
- Real Wii Remotes over a DolphinBar are no longer ignored: every frame calls `WPAD_ScanPads`, each channel uses `WPAD_FMT_BTNS_ACC_IR` plus `WPAD_SetVRes`, and buttons fall back to the ScanPads snapshot when the event queue stays empty. The strap and other menus accept A or 2 from any connected remote. The IR cursor is optional and never required for input.
- `wii/test/run-wiimote-e2e.sh` is the automated ship gate: a generated Dolphin TAS `.dtm` (option 1) plays an emulated Wiimote on the Null backend at 60 FPS through the strap screen, Practice, a detector-matched toss and a scored bag, records FPS, and fails if the DSP ucode warning is still in the log.

## [0.1.0] - 2026-11-16

- Build and real-Wii install guides included.
- Hit the backyard in Practice, 2 Player or an eight-rival Tournament.
- Hold B, swing back, swing forward and release. Slide along the foul line and curl bags with your wrist. Lefties welcome; no pointing needed.
- One arrow shows direction and power. Enjoy the backyard party, wind, dog celebrations and your own SD-card MP3s.
- Save your career and player settings. Clear practice mid-flight and keep throwing without a bag cap.
- First real HBC and BYCEAO USA/NTSC release, with verified disc contents, checksums and isolated Dolphin smoke tooling. Real-Wiimote feel still awaits playtesting.

## Development history

### Added
- Project scaffold: folder layout, toolchain notes, HBC `meta.xml`, and the Dolphin `GameSettings/BYCEAO.ini` that turns on real Wii Remotes. Included in this release.
- Wiimote input for both player slots: buffered WPAD reports, reconnect and disconnect handling, pass-and-play, accelerometer swing-direction aim with minus/plus fine trim, lateral/release-twist spin, setup/pause per-slot handedness, and the documented play/menu button map.
- Candidate underhand toss detector: retained peak of an 80 ms accelerometer speed proxy, stability-gated grip, lobe/attitude confirmation, manual and automatic release, clipping guards, and cancellation on report loss. Power never uses stroke distance.
- Per-slot comfortable-toss SET POWER calibration with median, outlier retry, floor and measured clip-ceiling hooks; trace diagnostics and release-window twist helper; game integration notes.
- Deterministic host tests for speed-to-power, gesture legality, calibration and both WPAD channels, available through `host-test` or the standalone shell harness. Hardware tuning and P1 validation remain pending for `wii-v0.1.0`.

- Playable game: Practice (live stats, CLEAR BOARD in the pause menu, unlimited bags), 2 Player (two remotes or pass-and-play) and the Tournament (8 rivals, 4 circuits, circuit wind, career save, champion), with round summaries, FINAL and career result screens.
- Physics, scoring and match flow ported from arcade 1.0.3: 240 Hz bag physics with landing curl, bag-to-bag contact, cancellation scoring to 21, last scorer throws first, knocked in/off callouts, KOBE power remap and the shared-per-round crosswind with the arcade's CALM/ON policy. Golden tests compare the C core with the pinned 1.0.3 page: WIN table, 66 shots, 8 multi-bag scenes, wind and RNG.
- Tournament CPU: casual and planCareerShot ports as a resumable, frame-budgeted search (budget measured at boot). It matches 1.0.3's choices on 24 seeded rival fixtures.
- Wiimote integration: swing speed sets power; swing direction aims straight from the stance (no auto-aim at the hole); swing/twist spin feeds landing curl. HAND (LEFT/RIGHT), SET POWER, POWER and CURL per player slot in SETTINGS and the pause menu; CONNECT REMOTE prompts; help legend on 1. No pointer or sensor bar.
- GX renderer: the 1.0.3 camera (18° FOV) and fog, the backyard voxels (fences, trees, hedges, stones, grill, lanterns), hay bales with the live wind pennant, the board with its sunburst art and a real hole, puffy bags with the down-face surface rule, held bag, the dog with all three celebrations, confetti, board dust, flight trail, grill smoke, and the BOARD · 27 FT close-up inset.
- HUD: scoreboard with pips and round edge, turn banner, callouts, wind panel, status/help legend, HOLD STILL, menus and panels; rival portraits from the 1.0.3 pixel art.
- One arrow on your bag shows where the throw is going and how hard: it appears as you swing forward, grows and warms in color with power, stays about two seconds after you let go, then fades. There is no separate power meter.
- MUSIC: play your own MP3s from the SD card (`apps/backyardcornhole/music`, or pick another folder). Play/pause, next track, refresh and volume, from the main menu or the pause menu.
- The backyard party: guests at the keg (and the occasional keg stand), a beer pong match that plays out differently each time, and the cook at the grill who tosses the dog a burger.
- Wind streaks drift across the yard when the wind is up.
- Saves to `sd:/apps/backyardcornhole/data/save.dat` (big-endian fields + CRC-32, crash-safe tmp/bak rename): career, per-slot profiles, help, WIND. Plays on if the SD card is missing.
- Packaging: NTSC-only video (480p/480i, 16:9 aware), HBC `boot.dol` + `meta.xml` + generated icon, Dolphin ISO with game ID `BYCEAO` (USA region, own apploader, WIT-composed, main.dol identical to boot.dol), `GameSettings/BYCEAO.ini` (`WiimoteSource0 = 2`, `WiimoteSource1 = 2`), release bundle with SHA256SUMS; rumble on release, board and CORNHOLE!.

### Changed
- HBC description no longer says "point to aim"; swing left/right aims.

### Fixed
- CLEAR BOARD while a bag is in the air no longer freezes practice; you get a fresh bag right away.
- Pausing on a round summary, FINAL or tournament result and resuming brings that screen back, so NEXT ROUND is never lost.
- Practice no longer stops at 256 bags with BOARD FULL; throw as long as you like.
- A throw is refused while the CONNECT REMOTE prompt has play frozen (found by the Dolphin soak when the remote idled off after 5 minutes).
- Require two-sided calm apex/finish dwell to prevent backswing braking auto-throws; classify the post-apex forward lobe so bottom-of-arc B releases throw immediately. Add physical pendulum regressions for both hands, held-B backswings and speed ordering.
