# Backyard Cornhole Wii — Changelog

All notable changes to the Wii build are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).
Releases are tagged `wii-vX.Y.Z`. The version source is `VERSION` in `wii/Makefile`,
which must match `hbc/apps/backyardcornhole/meta.xml`, the in-game footer and this file.

## [Unreleased]

### Added
- Project scaffold: folder layout, toolchain notes, HBC `meta.xml`, and the Dolphin `GameSettings/BYCEAO.ini` that turns on real Wii Remotes. Not playable yet.
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
