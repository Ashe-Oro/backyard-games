# Backyard Cornhole Arcade 0.2.19

Changed: `app/dist/site/arcade/trackball.mjs`, `app/dist/site/arcade/controller.js`, `app/package.json`, `tests/trackball.test.mjs`, `tests/controller.test.mjs`, `READ-ME.txt`, and this summary.

Power now uses peak sensitivity-adjusted forward hardware counts/sec over an 80 ms sliding window, with interpolated window boundaries. The opening report is excluded; samples require at least 16 ms and 8 measured forward counts. Power is linear and capped: 825 counts/sec = 50%, 1650 counts/sec = 100%, regardless of stroke distance. The old 0.2.17 full-power default was 1050 counts/sec; the new default is 1650 counts/sec (approximately 1.57×, from 1050 × 400/255). The rejected 0.2.18 model used 400 accumulated counts for full power.

A valid peak releases on leaving the forward cone, 160 ms idle, a speed drop of at least 40% after a full window, the release gate, or forward timeout. Stroke length remains 20–1200 counts, default 400, solely as an auto-release upper bound. Short fast flicks can release below the gate. Single-report, malformed-input, jitter, residual-motion and one-shot protections remain. Aiming, optional backswing spin, P1 buttons, nudges and Reset Bag retain their 0.2.18 behavior.

Cabinet Setup restores Full-power speed ±200 counts/sec, bounded to 400–10000. Calibration uses peak stroke.speed / 0.7, clamped through settings(), without changing sensitivity. The controller handles idle/timeout throws for both play and calibration. Package and visible menu/README versions are 0.2.19.

Storage is backyard-arcade-v9. Current v9 settings load directly. v8–v5 migrate missing speed and factory 1050/2100/6500 to 1650 counts/sec, preserving other custom speeds. Factory gates 220/48/80 become 400; spin 0 becomes 1; custom gates, nonzero spin, bindings and sensitivity survive. v4–v1 use those migrations plus a minimum 1650 counts/sec to avoid legacy hair-trigger settings. Reset restores the new defaults.

Validation: npm test passed all three test files. Direct execution also passed all 29 individual tests (22 trackball, 6 controller, 1 controls); controller syntax check passed. Tests cover speed/distance independence, late acceleration, irregular timestamps, release paths, render cadence, migrations, calibration, mappings and safety. Checksums confirm index.html and main.cjs, preload.cjs, cabinet-input.py, release-exit-keys.py and music-library.cjs are unchanged. controls.mjs and its test were not edited. No server, Playwright, commit, push, purchase or deployment was performed.
