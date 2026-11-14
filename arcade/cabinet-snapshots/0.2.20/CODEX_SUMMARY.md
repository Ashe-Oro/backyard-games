# Backyard Cornhole Arcade 0.2.20

Copied the verified 0.2.19 cabinet snapshot; the older arcade source tree was not used.
Local snapshot only: cabinet installation pending; no push, cabinet access, purchases,
licensed binary edits, or Electron runtime edits.

## Changes

- P1 play indices 0–9: height, surface, reset, left, right, reset, reset,
  reset, menu, menu. Menus, other pads, explicit bindings, and keyboard fallback stay intact.
- Physical top row: 0 height / 1 surface / 2 reset / 6 reset.
  Physical bottom row: 3 left / 4 right / 5 reset / 7 reset.
  Ashe confirmed indices 0, 1, 2, 3, 5. Top 4 = 6, bottom 2 = 4 and bottom 4 = 7
  are inferred from the Twin USB Y X L1 L2 / B A R1 R2 grid and unused-button
  observations; top 4 was not directly observed.
- Default and Reset Settings full-power speed: 3300 counts/sec. Measured 1650
  stays 1650 and produces 50% power; 3300 produces 100%. The linear 80 ms
  window, distance gate, spin, aiming and backswing logic are unchanged.
- Profile-only speed migration maps missing speed and factory values
  1050/1650/2100/6500 to 3300 for v9–v1. v9 retains custom gate/spin choices;
  v9–v5 retain custom speeds. Legacy v4–v1 retain the requested 3300 floor.
  Explicit settings({powerSpeed:1050}) still works for setup/calibration.
- Version updated in package, menu kicker and READ-ME. Physical help corrected.
  Added relative app -> resources-app symlink so snapshot test imports resolve.

## Validation

All 29 tests passed: controller 6, controls 1, trackball 22. Ran each file with
`node tests/<name>.test.mjs` and the three-file `node --test tests/*.test.mjs`
runner. Both edited JavaScript files passed `node --check` (Node v24.22.1).
Tests cover native/polled rising edges, menus/other pads, placement/reset,
v1–v9 factory and custom speed loading, calibration, measured speed versus
power, linear scaling and unchanged gesture behavior.

## SHA-256 comparison against 0.2.19

Every original file was compared by SHA-256. Only these files differ:

- `CODEX_SUMMARY.md`
- `READ-ME.txt`
- `resources-app/dist/site/arcade/controller.js`
- `resources-app/dist/site/arcade/trackball.mjs`
- `resources-app/package.json`
- `tests/controller.test.mjs`
- `tests/trackball.test.mjs`

All 21 remaining original files match byte-for-byte, including every
protected file, vendor asset and binary. Matching hashes (same in both snapshots):

| Relative path | SHA-256 in both snapshots |
| --- | --- |
| `resources-app/cabinet-input.py` | `63a56a4d3225c4fbfe5d1f69b1f0a292f95e4ba56b9d784b2d0c75cd34773fc1` |
| `resources-app/dist/site/arcade/arcade.css` | `2d449e5b8aac443f08909641fe9081d97b54fae62f3f5502bc7cfe3a15890bfb` |
| `resources-app/dist/site/arcade/board-view.mjs` | `4d48933e30e4adf5b46fe30cb40ec0370cf49f50a96ad9661c0966589ae1121b` |
| `resources-app/dist/site/arcade/dog-celebration.mjs` | `25664bf776e936843f9932cf1dbc0fd48448a03ac29f5f6bc00b3155b85db106` |
| `resources-app/dist/site/arcade/music-player.mjs` | `a04f0ff0f11364c55f0691e2cabfa69b5c7b65e80d173872087fd38a306d5090` |
| `resources-app/dist/site/arcade/party.mjs` | `a720daaf325265fa011a9ad1c6433b56596d04653b0aad67b4901d4aa25d8921` |
| `resources-app/dist/site/cornhole/controls.mjs` | `09f16072dafde87c34d33209ecec91a8374536207be1cf10f9de7d7c84a68d44` |
| `resources-app/dist/site/cornhole/index.html` | `e3b4f2b173d640681329c91e8eced7f0c6db0966426560c69fe9d94f6aab099a` |
| `resources-app/dist/site/cornhole/wind-streaks.js` | `f1a9337ce61841f8aa8a48dc2192852a5cdcb9002648887df16329afd6530159` |
| `resources-app/dist/site/vendor/FONT-LICENSE.txt` | `98e451fe911e5ee4a1a45359535dc30ea7df28b8d9a9cd1f741778efab1387ee` |
| `resources-app/dist/site/vendor/THREE-LICENSE.txt` | `4c40a1ef62450b857c3b2aaf294936304cd552d965fbcd9d32d4c5bcf4ba4454` |
| `resources-app/dist/site/vendor/pixel.woff2` | `afec86997fdaf54af1f59358fa2c1e2a0f1d04146edad18e5cd141d0384a7548` |
| `resources-app/dist/site/vendor/three.module.min.js` | `08fd7545d13d2c7fb65ab691530a802dafefd638596501854f267d0fb13c39e7` |
| `resources-app/dist/site/yard-games/backyard.js` | `571e992d9cef08bad14ab0ed251449674c3d30e685026a764df77d85a72520bd` |
| `resources-app/dist/site/yard-games/power-arrow.js` | `8f9823ede2ca92e4267107df12a901f976ef87622a827c391d02c9a16c58cb1c` |
| `resources-app/dist/site/yard-games/ui.css` | `da638648b160bd258c80a930852b553ba35fe72bd347afcfabe2a6dee40ec019` |
| `resources-app/main.cjs` | `fcc0582037c678bc4c0b681c05d4b082ad1138a60487e8a6425d1861b6fbbe6c` |
| `resources-app/music-library.cjs` | `1dfc9903de08740758e984c1ef88efbfd6cc89fd55b283298ce23ca1a99beef9` |
| `resources-app/preload.cjs` | `7398ab11693dd209a86d060a385650f3ded3b7cb057dde3a5b255a60e18f87ff` |
| `resources-app/release-exit-keys.py` | `a1f94c7c9160f781c4312028425493f0ba639b67a0e243e5e4b3778520436e99` |
| `tests/controls.test.mjs` | `4982112fd2b11e8eb431ffb964a683c45bb852a75265dd2ae8eca826f9ab258b` |
