# Backyard Cornhole Arcade — Changelog

All notable changes to the arcade (Electron / Batocera cabinet) build are documented in this file.
Entries 1.0.0–1.0.3 moved here unchanged from the repository-root `CHANGELOG.md`. The 1.0.4 entry was added when `origin/main` (`f462cc9`) was merged.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).

Tags `v1.0.0`–`v1.0.5` are arcade release tags and are never moved.
Future arcade releases are tagged `arcade-vX.Y.Z`, after the release-source reconciliation in
`docs/wii-port/REPO_LAYOUT.md` §6 PR R.

## [Unreleased]

### Changed
- Repo layout only: the arcade build now reads the website's Cornhole files from `web/`. The game, controls and cabinet install are unchanged.

## [1.0.8] - 2026-11-15

### Changed
- The game is ready for your next toss as soon as you can throw.
- A missed or late spin no longer comes out weak.
- A soft glow on the bag shows when you can roll.

## [1.0.7] - 2026-11-15

### Changed
- The big vertical power bar is gone.
- One compact arrow on your bag now shows where you threw and how hard.
- A short arrow is a soft toss; a long, filled arrow is full power.
- The arrow still stays about two seconds after your throw, then fades.

## [1.0.6] - 2026-11-15

### Fixed
- After you throw, a clear arrow shows the exact direction the bag went.
- Lean hard right and the arrow points far right; lean hard left and it points far left.
- The arrow stays about two seconds, then fades.

## [1.0.5] - 2026-11-15

### Fixed
- Line up left or right of center and throw straight: the bag flies straight and lands to that side of the hole.
- Straight throws no longer auto-aim at the hole.

## [1.0.4] - 2026-11-15

### Changed
- The LAST SHOT popup and label are gone.
- The power meter is now a vertical bar that fills with your throw power.
- It stays on screen about two seconds after you throw, then hides.
- The CLEAR BOARD popup is gone.
- The top-right red button clears the board in Practice and keeps your stats.
- Only the bottom-right button tucks or pops the keys legend.

## [1.0.3] - 2026-11-14

### Added
- The running version (for example, `v1.0.3`) appears in the upper-right corner during play, read from the installed build so you can confirm which version is running.

## [1.0.2] - 2026-11-14

### Changed
- Full-power throws are easier: the spinner needs about 20% less speed to fill the meter (threshold 6048).

## [1.0.1] - 2026-11-14

### Changed
- Full-power throws need 35% more spinner speed.

### Removed
- The blue cooler is gone from the yard.

## [1.0.0] - 2026-11-14

### Added
- Backswing adds curl: A hooks right, C hooks left.
- A P1 red-button key legend you can tuck away or pop open, with your choice remembered.
- A left spinner legend.
- More variety among sideline beer pong players, including misses that ricochet off the rim.

### Changed
- Roll the spinner faster for more power; a long, slow roll stays soft.
- The last-shot power meter fades after three seconds.

[Unreleased]: https://github.com/Ashe-Oro/backyard-games/compare/v1.0.5...HEAD
[1.0.5]: https://github.com/Ashe-Oro/backyard-games/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/Ashe-Oro/backyard-games/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/Ashe-Oro/backyard-games/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/Ashe-Oro/backyard-games/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/Ashe-Oro/backyard-games/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/Ashe-Oro/backyard-games/releases/tag/v1.0.0
