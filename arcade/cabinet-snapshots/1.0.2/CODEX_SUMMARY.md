# Backyard Cornhole Arcade 1.0.2

Prepared on `release/1.0.2` by copying the live-cabinet 1.0.1 snapshot,
preserving `app -> resources-app`.

Default full-power speed is now 6048 counts/sec, down from 7560 by 20%.
Power remains linear in peak forward speed over the unchanged 80 ms window:
`clamp(peakSpeed / powerSpeed, 0, 1)`. Distance does not grant power, and the
400–10000 speed clamp is unchanged. Explicit settings such as
`settings({powerSpeed:1050})` remain accepted.

Profile loaders migrate missing/null speeds and factory speeds 1050, 1650,
2100, 3300, 4000, 5600, 6500 and 7560 to 6048. Custom calibrated speeds such
as 2000 survive in v9 and v8–v5. Legacy v4–v1 profiles migrate factory values
and floor other values at 6048; custom 9000 survives.

Validation from this snapshot directory:

- `node --test tests/*.test.mjs`: PASS, 5 test files, 0 failures.
- `node --test --test-isolation=none tests/*.test.mjs`: PASS, 45 tests,
  0 failures, 0 skipped.
- `node --check`: PASS for all five edited JS/MJS files (snapshot trackball,
  snapshot controller, two snapshot tests, and top-level arcade controller).
- `git diff --check`: PASS.
- Snapshot comparison confirms all copied files except the eight listed below
  remain byte-identical to 1.0.1; the app symlink is preserved. Earlier snapshots
  have no tracked changes.

Power tests cover 3024 = 50%, 6048 = 100%, and 9000 capped at 100%, plus the
unchanged measured-speed fixtures at 1650 and 825 with power divided by 6048.
Migration tests include both 7560 and 5600 and custom 2000 across v9–v1.
Existing button, legend, linger, spin, beer-pong and cooler-removed checks pass.
Node emitted only the existing module-type warning for bundled three.js in the
non-isolated run; no vendor or runtime files were edited.

Changed files relative to the copied snapshot:

- `resources-app/dist/site/arcade/trackball.mjs`: default and factory migration.
- `resources-app/dist/site/arcade/controller.js`: menu kicker version only.
- `resources-app/package.json`: version only.
- `tests/trackball.test.mjs` and `tests/controller.test.mjs`: power expectations
  and migration coverage.
- `READ-ME.txt`, `DEV-NOTES.txt`, and this `CODEX_SUMMARY.md`: release docs.

Outside the new snapshot, only `arcade/cabinet-snapshots/README.md` and the
requested top-level version strings changed: `arcade/package.json`, the two
root version fields in `arcade/package-lock.json`, `arcade/README.md` header,
and `arcade/controller.js` kicker.

All other gameplay remains as in 1.0.1: buttons, KEYS toggle, legends, 3000 ms
last-shot linger, spin curl, aiming, nudge step, calibration measured speed / 0.7,
and beer pong. The blue cooler was already removed and stays gone. Yard assets,
licensed/vendor files, Electron runtime and all earlier snapshots are unchanged.
No build, push, cabinet installation, /userdata edits, preview video edits,
gamelist edits, or spending occurred. Install timestamp and rollback path remain
pending. Commit and installation are left to the caller; no commit was made.
