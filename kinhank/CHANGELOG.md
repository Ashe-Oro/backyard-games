# Backyard Cornhole — Kinhank (Dolphin emulation)

All notable changes to the Kinhank (Dolphin on the Kinhank X5 Pro) target
are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).
Releases are tagged `kinhank-vX.Y.Z`. The version source is `VERSION` in
`kinhank/Makefile`, which must match this file. This target uses the same
game disc as the original Wii build and adds Dolphin `GameSettings`.

## [0.1.8] - 2026-11-25

### Changed

- Updated to Wii v0.1.8 (BYCEAO) with learned swing axes, a forward bottom-of-arc release gate and a lower factory speed reference.
- Added live swing diagnostics, buffered CSV logging with idle flush and rotation, and five Wii Remote calibration poses for DolphinBar testing. STRAP ON! and the in-game HUD show v0.1.8.

### Fixed

- The detector uses the measured tip-up Y sign and rejects early releases while keeping the bag for the next swing.

## [0.1.7] - 2026-11-22

### Changed

- Updated to Wii v0.1.7 (BYCEAO) with picker-specific swing power and improved backswing and forward labels.
- STRAP ON! and the in-game HUD show v0.1.7.

### Fixed

- Releasing B without a qualifying forward swing keeps the bag and leaves the throw count alone.

## [0.1.6] - 2026-11-22

### Changed

- Updated to the Wii v0.1.6 disc (BYCEAO). Strong NORMAL swings use a lower gain, a higher speed reference and a cap that puts a centered throw near the back of the board.
- The STRAP ON! screen and HUD show v0.1.6.

### Fixed

- A held or unknown B state at startup no longer produces a phantom Practice throw.

## [0.1.5] - 2026-11-22

### Changed

- Updated to the Wii v0.1.5 disc (BYCEAO) with 25% softer default throws and rounded keg and beer pong props.
- The STRAP ON! screen and HUD show v0.1.5. The Dolphin profile uses 3× internal resolution.

## [0.1.4] - 2026-11-21

Wii Sports Bowling controls on the Kinhank. The pointer is not used; the
DolphinBar only passes buttons and the accelerometer through.

- Same ISO and DOL as `wii-v0.1.4` (`BYCEAO`); `GameSettings/BYCEAO.ini` is
  unchanged (real Wii Remotes, `InternalResolution = 3`, `chmod 666` after
  ADB).
- Swing speed is the power and swinging left/right is the aim. The −/+
  fine-aim nudge and its `AIM: +N` label are gone; −/+ do nothing in play.
- D-pad up/down picks LOB / NORMAL / SLIDER, the only control for the arc.
  Release pitch never changes it. D-pad left/right still moves along the
  foul line.
- New in-game help text and bottom bar to match. The 1+2 overlay adds the
  last throw's power and aim.
- The version shows on the STRAP ON! screen and in the bottom-right corner
  during play (v0.1.4), so it can be checked on the TV.

## [0.1.3] - 2026-11-21

The 0.1.3 Bowling throw, packaged for Dolphin on the Kinhank at 3x internal
resolution (about 1080p). This is the current test target.

- Same ISO and DOL as `wii-v0.1.3` (`BYCEAO`); `GameSettings/BYCEAO.ini` is
  unchanged (real Wii Remotes, `InternalResolution = 3`, `chmod 666` after
  ADB).
- Point the Wii Remote up, hold B, swing back, swing forward and let go of B
  during the forward swing. B grips at once in any pose (no HOLD STILL, no
  settle time), every B release throws (a weak lob if no forward swing was
  measured), and late or bursty DolphinBar reports no longer cancel the swing.
- The 1+2 overlay adds the swing phase, B, reports per second, the largest
  report gap and the last throw's peak speed and power. Photograph it if a
  throw still misbehaves.
- At 1080p in 16:9 the HUD is laid out for 16:9 and the BOARD 27 FT inset sits
  in the top-left corner, clear of the beer-pong players.

A physical Wii Remote retest on the Kinhank is still pending. Retro installs
this bundle over ADB; see [INSTALL.md](INSTALL.md).

## [0.1.2] - 2026-11-21

The 0.1.2 throw fix, packaged for Dolphin on the Kinhank at 3x internal
resolution (about 1080p). Superseded by 0.1.3.

- Same ISO and DOL as `wii-v0.1.2` (`BYCEAO`). The original Wii target stays
  at its hardware limit (480p progressive with a component cable and 480p
  enabled in Wii Settings, otherwise 480i).
- `GameSettings/BYCEAO.ini` turns on real Wii Remotes (`WiimoteSource0 = 2`,
  `WiimoteSource1 = 2`) and sets `InternalResolution = 3`. After copying the
  ini over ADB, `chmod 666` it so Dolphin can read it; a default
  `-rw-rw---- shell:shell` file is invisible to the app uid, which then
  silently falls back to global settings. The 0.1.1 Kinhank install
  probably never applied `WiimoteSource = 2` for this reason. The ini
  format was already correct. With `chmod 666` on
  `/storage/emulated/0/Android/data/org.dolphinemu.dolphinemu/files/GameSettings/BYCEAO.ini`,
  3× is verified at 59.95 FPS. Leave Dolphin's global `GFX.ini` at 1× for
  other games.
- Preserve each Wii Remote report's B state and acceleration through the WPAD
  queue. Hold B, swing back and forward, then release to throw. Short or
  gentle forward swings use a low-power release in the measured direction. A
  stationary B tap cancels.
- Hold 1+2 for two seconds to toggle live accelerometer, B state and swing
  diagnostics.
- Removed the Wii yard's string lanterns and IR cursor.

A physical Wii Remote retest on the Kinhank is still pending. Retro installs
this bundle over ADB; see [INSTALL.md](INSTALL.md).
