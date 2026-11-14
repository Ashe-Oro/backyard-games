# Backyard Cornhole Arcade 0.2.21

Based on the SHA-256-matched live-cabinet 0.2.20 snapshot, copied with `app -> resources-app` preserved. Pending cabinet install; no push, cabinet install, purchases, licensed binary edits, or Electron runtime edits.

## Variations

- `red-blowout`: red 10–3, mostly clean red scoring, 23 turns.
- `blue-close`: blue 10–9, back-and-forth within two cups, 22 turns.
- `comeback`: coin-flipped winner, 10–6 after trailing by four cups, 27 turns.
- `blue-quick`: blue 10–2, clean scoring sprint, 20 turns (shortest).
- `red-long`: red 10–8, repeated misses and swats, 49 turns (longest).

At load, select uniformly from all five ids. After the winning impact and 5.4 seconds of celebration, select uniformly from the other four ids; comeback orientation is an independent coin flip. Racks, winner, and script reset for the next match. The match clock tracks each script's start and duration, including elapsed overshoot. Existing hit/miss/bounce/swat animation paths and 4.05/1.18/1.83/2.12/2.53-second timing constants are retained. Swats follow a table bounce and do not score. Final matches stop on the winning impact rather than adding the unused remainder of a turn.

Only party.mjs changes behavior. Package version, controller menu kicker, and READ-ME header/note are the other app/document edits. Cornhole controls and all other decorations are unchanged.

## Verification

Run from this snapshot directory:

- `node --test tests/*.test.mjs`: passes all four test files (this Node 24 runner reports file-level counts).
- `node --test --test-isolation=none tests/*.test.mjs`: **35 tests passed, 0 failed**; 31 unchanged controller/controls/trackball tests plus four new party tests.
- `node --check app/dist/site/arcade/party.mjs`, `node --check app/dist/site/arcade/controller.js`, and `node --check tests/party.test.mjs`: passed.

Party tests load the unchanged vendored Three.js ESM via a data URL, requiring neither npm packages nor WebGL. They exercise script validity and first-to-10 termination, both comeback orientations, score/length/flow distinctions, selection reachability and non-repeats, load selection, impact scoring, celebration duration, rack reset, and bounce/swat geometry flags.

## SHA-256 unchanged-file proof

Every pre-existing regular file outside the five intended edits (`party.mjs`, `package.json`, `controller.js`, `READ-ME.txt`, this summary) was hashed in both snapshots and compared byte-for-byte. All 23 files match. The only new regular file beyond the copied snapshot is `tests/party.test.mjs`. The `app` symlink remains exactly `resources-app`. Controller and package contents were additionally checked against only their exact version replacements. The 0.2.20 tree was left untouched.

Each hash below is identical for 0.2.20 and 0.2.21:

| Relative path | SHA-256 (both snapshots) |
| --- | --- |
| `resources-app/cabinet-input.py` | `63a56a4d3225c4fbfe5d1f69b1f0a292f95e4ba56b9d784b2d0c75cd34773fc1` |
| `resources-app/dist/site/arcade/arcade.css` | `2d449e5b8aac443f08909641fe9081d97b54fae62f3f5502bc7cfe3a15890bfb` |
| `resources-app/dist/site/arcade/board-view.mjs` | `4d48933e30e4adf5b46fe30cb40ec0370cf49f50a96ad9661c0966589ae1121b` |
| `resources-app/dist/site/arcade/dog-celebration.mjs` | `25664bf776e936843f9932cf1dbc0fd48448a03ac29f5f6bc00b3155b85db106` |
| `resources-app/dist/site/arcade/music-player.mjs` | `a04f0ff0f11364c55f0691e2cabfa69b5c7b65e80d173872087fd38a306d5090` |
| `resources-app/dist/site/arcade/trackball.mjs` | `c7bc2f569d9440b95cac6398da06338fdd6a113df95995338512f3d60ddc7b6b` |
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
| `tests/controller.test.mjs` | `66946e1cd91b98a9aa8cc53cdd7b71913c70e423a7b105e425e0216304340bb5` |
| `tests/controls.test.mjs` | `4982112fd2b11e8eb431ffb964a683c45bb852a75265dd2ae8eca826f9ab258b` |
| `tests/trackball.test.mjs` | `4c46a570fd0a8240075df84b066202538065de2bfdd2af3271b725b0b335178e` |
