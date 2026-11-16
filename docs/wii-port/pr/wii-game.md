# Backyard Cornhole Wii 0.1.0 — first playable release build

Owner: Ashe · Branch: `wii/game` · Approved game: `513a596` · Packaging merge: `315cdf2`.
Local work only; nothing pushed. Recommend tag `wii-v0.1.0` after the fresh
Dolphin smoke passes. The release build is packaged; fresh boot proof is blocked
by the current sandbox and is still required before calling this verified.

## What players get

- Practice with live stats, unlimited bags and CLEAR BOARD; 2 Player with two
  remotes or pass-and-play; an eight-rival Tournament across four circuits.
- Arcade 1.0.3 physics, cancellation scoring to 21, CPU opponents, wind and
  career saves. Golden tests compare the native C core with the pinned game.
- The GX backyard, dog celebrations, party guests, beer pong, grill cook, wind
  streaks, shot trail, board inset and one combined direction/power arrow.
- Your own MP3 music from SD, crash-safe career/player saves, and continued
  play when an SD card is unavailable.
- Review fixes: CLEAR BOARD works mid-flight, pause restores result/round
  screens, practice continues beyond 256 bags, and disconnected players cannot
  throw through CONNECT REMOTE. MUSIC and party animation are implemented.

## Controls

Hold **B**, backswing, forward swing, **release B**. Swing speed sets power;
swing direction aims. **D-pad left/right slides along the foul line**. Sideways
motion and wrist twist at release add spin. Choose LEFT/RIGHT handedness
separately for each player in SETTINGS or pause. **No IR or pointing needed.**

D-pad up/down adjusts height; A selects slick/sticky; minus/plus trims aim;
2 resets the bag; 1 opens help; HOME pauses. SET POWER calibrates each player.
Real-Wiimote feel and provisional calibration limits remain unverified.

## Packaging decision and build

Keep `wii/game`'s canonical pipeline: it builds the approved game and its own
apploader, retains the real metadata/icon, and extracts the ISO to verify its
DOL. Normally merge `wii/packaging` at `b7cd6d6`, resolving its README conflict in
favor of approved game behavior. Retain its isolated smoke runner and strengthen
its manual install handoff. Its Makefile now delegates to the real pipeline;
the historical stub and legacy Python packager never run in release targets.

```sh
make -C wii release OUT=/workspace/uploads/wii-game
```

Produces standalone `boot.dol`, HBC `apps/backyardcornhole/` and HBC ZIP,
`BackyardCornhole-BYCEAO.iso`, `GameSettings/BYCEAO.ini`, `KINHANK_INSTALL.md`,
full release ZIP and SHA256 manifests. Disc is BYCEAO, USA/NTSC, IOS 58;
video selects NTSC 480p or 480i. Both shipped remote sources are `2` (Real).
The build checks HBC/changelog versions and DOL identity; ZIP metadata is fixed.
A fixed WIT-generated homebrew ticket removes random per-build title keys.
Two consecutive releases produce identical ISOs and manifests.

Manual handoff: ISO goes to `/storage/3050BAA350BA6EEA0/Roms/wii/`; copy the INI
into the active Dolphin user folder's `GameSettings/`. Use real Wii Remotes via
**MAYFLASH DolphinBar Mode 4**. These are instructions only; no hardware accessed.

## Validation

| Check | Result |
|---|---|
| Forced native release rebuild | Pass; no compiler warnings |
| `make -C wii host-test` | 33 input, 7 WPAD, 938 physics, 2,256 game, 209,137 party checks pass; generated files current |
| Shared drift/guards | 32/32 pass |
| Pinned 1.0.3 snapshot | 48/48 pass |
| ISO/HBC DOL identity, ZIP CRC, SHA256 manifests | Pass |
| Repeated release ISO/manifest comparison | Byte-identical |
| Fresh 25-second Dolphin NoGUI title/menu smoke | **Blocked before rendering**: Xvfb socket bind denied; direct headless attempt aborts |
| Extra top-level arcade suite | 4/6 files pass; two blocked by sandbox subprocess EPERM |

Logs and report: `/workspace/docs/cornhole/wii-port/release/` and
`smoke/REPORT.md`. No fresh screenshot exists; prior game screenshots are not
presented as this release's smoke evidence. Rerun with a permitted local display:

```sh
make -C wii smoke OUT=/workspace/uploads/wii-game
```

Inspect `smoke/screenshot.png` after the default 25 seconds to confirm the real
game title/menu renders. Process survival alone is not a visual pass.

## Follow-ups

- Fresh Dolphin boot/render proof is still required.
- Real-Wiimote swing feel, calibration limits and timing remain unverified.
- Full-speed Android/Kinhank match and real Wii frame-time checks are owed.
- Scene shadows/clouds, planner granularity, SD music format parity, GameTDB ID
  collision check and Docker build verification remain tracked follow-ups.

## Changelog: 0.1.0

Hit the backyard in Practice, 2 Player or Tournament. Swing to throw, slide along
the foul line and curl bags with your wrist; lefties welcome. One arrow shows
direction and power. Enjoy the party, dog celebrations, wind and your own music.
Save your career, clear practice mid-flight and keep throwing without a bag cap.
First real HBC and BYCEAO release package; real-Wiimote playtesting still owed.

Bundle baseline is the requested `06022bc`, even though the current local
`origin/main` reference is `c4b3ee7`. No rebase or remote update was performed.
