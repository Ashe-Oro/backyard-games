# Cornhole 0.2.18 port

Ported onto the supplied live-cabinet 0.2.17 files; reference sources were used for behavior only.

Files changed:
- `app/dist/site/arcade/trackball.mjs`: count-based power, backswing spin, migration, P1 action helper.
- `app/dist/site/arcade/controller.js`: v8 loading/saving, P1 routing and native debounce, placement nudges, calibration, setup readout, legend and version kicker.
- `app/dist/site/arcade/arcade.css`: wrapping legend styles.
- `app/dist/site/cornhole/controls.mjs`: linear human power conversion.
- `app/package.json`: version 0.2.18.
- `READ-ME.txt`: controls, power/spin, migration, calibration and profile/install guidance.
- `tests/trackball.test.mjs`: corrected cabinet import and adapted/expanded behavior checks.
- `tests/controller.test.mjs`: headless native/polled input, debounce and placement checks.
- `tests/controls.test.mjs`: linear power conversion checks.
- `CODEX_SUMMARY.md`: this report.

Constants and behavior:
- Old power: forward speed / `powerSpeed` (factory 1050 counts/sec, previously 2100). New: accumulated sensitivity-adjusted forward counts / named `FULL_POWER_COUNTS = 400`, clamped to [0,1]. 200 counts = 50%; 400 = 100%.
- Default release gate: 220 → 400; allowed gate range: [20,400] → [20,1200]. Custom shorter gates still release at their configured counts. Distinct timestamped reports remain required before release.
- Default spin: 0 → 1. Placement uses named `PLACEMENT_STEP = 0.1`, bounded to [-0.45,0.45].
- Cabinet `throwPower` / `pullPower`: tanh GAIN=4 → clamped identity. CPU launch paths remain unchanged.

Final P1 / Pad 0 map (bottom row first; explicit saved bindings retain precedence):

| Physical button | Index | Play | Menu |
| --- | --- | --- | --- |
| Bottom 1 | 0 | Cycle height: Kobe / Slider / Normal | Confirm |
| Bottom 2 | 1 | Slick / Sticky | Back |
| Bottom 3 | 2 | Nudge position left | Existing reset-key alias |
| Bottom 4 | 3 | Nudge position right | Confirm alias |
| Top 1 | 4 | Unused | Back alias |
| Top 2 | 5 | Reset Bag | Existing reset-key alias |
| Top 3 | 6 | Unused | Unused |
| Top 4 | 7 | Unused | Unused |
| Select | 8 | Pause/menu | Back/menu |
| Start | 9 | Pause/menu | Confirm |

Other pads retain their original tables. Native held presses are ignored until release; polled pads retain rising-edge activation. Nudges cancel windup and hide the preview without throwing. Reset Bag retains its exact 0.2.17 implementation and host semantics: center the unthrown bag, clear windup/aim/spin, preserve throws, bags and scores.

Spin and aim:
- Optional back-left A gives positive clockwise landing spin/curl right; back-right C gives negative counterclockwise spin/curl left.
- `clamp(-backX / backY,-1,1) * config.spin` scales spin with lean. Straight back B and no backswing give zero spin. Stale backswing expires after `timeoutMs` (default 2500).
- Forward X still aims left/right independently. Existing landing english/curl, CPU spin and airborne physics are unchanged.

Profiles:
- Read v8 through `settings`; otherwise v7/v6/v5 through `previousSettings`, or v4–v1 through `legacySettings`; save to `backyard-arcade-v8`.
- Only old factory gates 220/48/80 become 400. Other valid calibrated gates, sensitivity and bindings survive. Old spin 0 becomes 1; nonzero custom spin survives. Current v8 profiles can explicitly choose spin 0.
- The 0.2.16→0.2.17 factory-speed history (2100/6500 → 1050, v6 → v7) remains documented; v8 supersedes speed-based power and drops obsolete `powerSpeed` from normalized settings. Existing storage entries are not deleted. Preserve the cabinet user profile during installation.

Intentional retirement: speed-based power and adjustable Full-power speed / speed calibration. Setup now shows fixed Full-power counts: 400; Calibrate Trackball scales sensitivity so its measured stroke represents 400 counts, within the existing [0.25,4] sensitivity limits. As in the reference, calibration completes at a 400-count gate, so the reference stroke must reach at least 400 current counts. Sensitivity, Stroke length, Wind, Backswing spin, bindings, Reset Settings, F2 and the power gauge remain.

Validation: `node --test tests/*.test.mjs` passed all three test files. `node --test --test-isolation=none tests/*.test.mjs` reported 19 tests passed, zero failed. Controller module syntax check passed. SHA-256 checks confirm `index.html`, `main.cjs`, `preload.cjs`, `cabinet-input.py`, `release-exit-keys.py` and `music-library.cjs` are unchanged; party, board view and physics remain intact. No other 0.2.17 behavior was intentionally retired.

No server, Playwright, build, installation, commit or push was run. Physical cabinet validation remains unperformed.
