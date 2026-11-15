# Backyard Cornhole Arcade 1.0.5

Copied from the verified live 1.0.4 snapshot with `app -> resources-app`
preserved. Package version is 1.0.5; the upper-right label and menu kicker
continue to read the installed runtime version. Cabinet installation is pending.

## Root cause and fix

In `resources-app/dist/site/cornhole/index.html`, `baseYaw(rx)` computed
`Math.atan2(HOLE_WORLD.x - rx, -(HOLE_WORLD.z - REL_Z))`: the yaw from the
bag's actual release x toward the hole. `launch()` added that angle to `aim`
and `drift * 0.04`. A shifted stance therefore launched toward the hole even
when spinner aim was exactly zero. Held-bag rotation and pointer/spinner
previews also used that hole-facing angle.

Traced input through `trackball.mjs` (centered forward motion gives zero aim),
controller `placementTarget` / `host.position`, `releaseX`, `__arcadeHost.throw`,
and `throwBag` into `launch`. Placement changes the release x; the unwanted
lateral velocity originated in `baseYaw`, not the spinner or placement code.
Flight `step()` integrates velocity, gravity and wind without target attraction.
Its hole capture/settling behavior applies only after a bag reaches the hole.

Removed `baseYaw` and all calls. `launch()` now uses `aim + drift * 0.04`,
so zero aim follows world -Z, parallel to the board's long axis, from the
actual release x. Held-bag rotation and previews use the unassisted aim too.
There is no compensating counter-aim. Spinner 1/3 formulas and backswing
spin/curl helpers are unchanged.

Power remains speed-based, peak over 80 ms, full-power threshold 6048.
Placement remains 0.05 m per nudge, clamped to ±0.45 m. Vertical POWER fill,
2000 ms linger, no LAST SHOT, upper-right runtime version, top-4 Practice CLEAR,
bottom-4-only KEYS, reset-bag and beer-pong behavior are preserved.

## Validation

From this snapshot directory:

- `node --test tests/*.test.mjs`: PASS; 5 test files, 0 failures.
- `node --test --test-isolation=none tests/*.test.mjs`: PASS; 54 individual
  tests, 0 failures, 0 skipped (51 existing plus 3 new).
- New tests extract production `launch()` and `step()` into the existing
  Three.js physics harness and simulate through landing and rest. No wind,
  65% physical power, normal arc, slick side. World x is in metres.
- Straight releases at -0.20, 0 and +0.20 m land and finish within 0.001 m of
  their release x. -0.20 m corresponds to four completed LEFT nudges.
- Both forward aim signs retain a lateral offset with a -0.10 m stance;
  the smaller stance keeps both aimed landings on the deck. The shift relative
  to the matching centered aimed throw remains -0.10 m within 0.001 m.
- Straight A/C shots from -0.20 m land at that x before curling right/left.
- Negative control: running the new straight regression against the unchanged
  1.0.4 HTML fails as expected: release -0.20 m lands at -0.008608213 m.
  The fixed 1.0.5 HTML was restored before the final full test runs.
- `node --check`: PASS for edited top-level `arcade/controller.js`, snapshot
  `tests/spin.test.mjs`, and the changed HTML inline module (extracted to /tmp;
  import map excluded). `git diff --check` passes.
- Existing bundled Three.js module-type warning remains; no vendor changes.
  No cabinet or Electron visual playtest was performed.

## Files changed

Relative to 1.0.4, only these files in the copied snapshot differ:

- `resources-app/dist/site/cornhole/index.html`
- `resources-app/package.json`
- `tests/spin.test.mjs`
- `READ-ME.txt`
- `DEV-NOTES.txt`
- `CODEX_SUMMARY.md`

Outside the new snapshot:

- `CHANGELOG.md`
- `arcade/cabinet-snapshots/README.md`
- `arcade/package.json`
- `arcade/package-lock.json` (only the two root version fields)
- `arcade/README.md` (header only)
- `arcade/controller.js` (kicker version only)

The full new snapshot is copied, including unchanged assets. Its symlink matches
1.0.4. Vendor files and all other copied files are byte-identical to 1.0.4.
Earlier snapshots and earlier changelog entries are unchanged. Work stayed in
`/workspace/cornhole-1.0.5` on `release/1.0.5`.
No rebuild, commit, tag, push, bundle, cabinet install or purchase was performed.
Commit, annotated tag, bundle and installation remain with the caller.
