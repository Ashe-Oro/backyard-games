# Backyard Cornhole Arcade 1.0.4

Copied from the verified 1.0.3 snapshot with `app -> resources-app` preserved.
Version stays 1.0.4. Cabinet installation is pending.

## Changes

- Kept one vertical POWER meter, filled upward from the bottom during the swing.
  The same bar holds thrown power through release + 2000 ms, then hides.
  No player-facing LAST SHOT label or second meter. Live preview takes priority;
  menus hide the meter. Colors and meter accessibility values are preserved.
- Power remains peak forward speed over 80 ms, full-power threshold 6048.
  Calibration and migrations are preserved. The version label stays upper-right.
- Removed the #reset-bags CLEAR BOARD box, its CSS and every DOM reference.
- Pad 0 index 6 (top 4) emits ClearBoard during play; menus do nothing.
  The controller calls window.__arcadeHost.clearBoard(), which clears bag meshes
  and refreshes the HUD only in Practice on a human aim turn, keeping stats.
  Controller guards also block paused play, CPU turns and active swings.
  Two-player and tournament presses are harmless no-ops.
- Only Pad 0 index 7 (bottom 4) emits ToggleKeys. Native and polled inputs retain
  rising edges; on-screen KEYS and the remembered tucked state are preserved.
- Eight-button legend: HEIGHT / SURFACE / RESET / CLEAR above
  LEFT / RIGHT / RESET / KEYS. Removed the extra on-legend RESET BAG button.
  Top/bottom 3 still reset the unthrown bag. Both screenshot fixtures match.
- Updated player guide, developer notes, snapshot index and root changelog.
  Existing first-pass top-level version edits are preserved.

## Validation

From this snapshot directory:

- `node --test tests/*.test.mjs`: PASS; 5 test files passed, 0 failed.
- `node --test --test-isolation=none tests/*.test.mjs`: PASS; 51 tests passed,
  0 failed, 0 skipped.
- `node --check` passed for all changed JS/MJS files and the changed HTML inline
  scripts (excluding import maps), plus the existing top-level controller edit.
- Native and polled top-4 coverage exercises the keyboard handler, real host
  clearBoard method, real clearBags and human-turn guard; verifies mesh removal,
  HUD refresh, unchanged stats, one action per press and guarded no-ops.
- Map tests cover play/menu routing; KEYS tests use only index 7 and retain
  persistence and reset-bag checks. Legend tests verify all eight labels, both
  fixtures and removal of both extra button boxes.
- Existing meter tests retain exact +2000/+2001 boundaries, live/held power,
  colors, aria values, upward fill, no removed label, and upper-right version.
  Power, 80 ms window, spin, beer pong and reset-bag tests pass.
- Compared the snapshot against 1.0.3: only the files below differ. Vendor files
  are byte-identical. No visual cabinet or Electron playtest was performed.
- Node emits the existing module-type warning for bundled three.js; no vendor
  or package module-type changes were made.

## Changed snapshot files (combined first pass and follow-up)

- CODEX_SUMMARY.md
- DEV-NOTES.txt
- READ-ME.txt
- legend-shots.html
- resources-app/dist/site/arcade/arcade.css
- resources-app/dist/site/arcade/controller.js
- resources-app/dist/site/arcade/legend.css
- resources-app/dist/site/arcade/legend.mjs
- resources-app/dist/site/arcade/trackball.mjs
- resources-app/dist/site/cornhole/index.html
- resources-app/package.json
- spin-legends.html
- tests/controller.test.mjs
- tests/trackball.test.mjs

Root CHANGELOG.md and arcade/cabinet-snapshots/README.md also describe the change.
Earlier snapshots, licensed/vendor assets and Electron runtime are untouched.
No rebuild, commit, tag, push, bundle, install or purchase was performed.
Commit, annotated tag, bundle and cabinet installation remain with the caller.
