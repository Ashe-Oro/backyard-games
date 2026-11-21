# Backyard Cornhole — Kinhank (Dolphin emulation)

All notable changes to the Kinhank (Dolphin on the Kinhank X5 Pro) target
are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).
Releases are tagged `kinhank-vX.Y.Z`. The version source is `VERSION` in
`kinhank/Makefile`, which must match this file. This target uses the same
game disc as the original Wii build and adds Dolphin `GameSettings`.

## [0.1.2] - 2026-11-21

The 0.1.2 throw fix, packaged for Dolphin on the Kinhank at 3x internal
resolution (about 1080p). This is the current test target.

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
