# Backyard Cornhole Wii — Changelog

All notable changes to the Wii build are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).
Releases are tagged `wii-vX.Y.Z`. The version source is `VERSION` in `wii/Makefile`,
which must match `hbc/apps/backyardcornhole/meta.xml`, the in-game footer and this file.

## [Unreleased]

### Added
- Project scaffold: folder layout, toolchain notes, HBC `meta.xml`, and the Dolphin `GameSettings/BYCEAO.ini` that turns on real Wii Remotes. Not playable yet.
- Wiimote input for both player slots: buffered WPAD reports, reconnect and disconnect handling, pass-and-play, IR aim with minus/plus fallback, calm wrist-roll curl, and the documented play/menu button map.
- Candidate underhand toss detector: retained peak of an 80 ms accelerometer speed proxy, stability-gated grip, lobe/attitude confirmation, manual and automatic release, clipping guards, and cancellation on report loss. Power never uses stroke distance.
- Per-slot comfortable-toss SET POWER calibration with median, outlier retry, floor and measured clip-ceiling hooks; trace diagnostics and release-window twist helper; game integration notes.
- Deterministic host tests for speed-to-power, gesture legality, calibration and both WPAD channels, available through `host-test` or the standalone shell harness. Hardware tuning and P1 validation remain pending for `wii-v0.1.0`.
