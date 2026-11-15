# Backyard Cornhole Arcade 1.0.6

Copied `1.0.5/` to `1.0.6/`, preserving `app -> resources-app`.
Snapshot package version is 1.0.6; the label/kicker still read the runtime
version through `app.getVersion()`. Install timestamp and rollback path: pending.

## Root cause and fix

In `resources-app/dist/site/arcade/controller.js`, `frame()` ran
`handleStroke(machine.tick(now), now)` and then called `host.hideArrow()` when
the machine left forward. An accepted throw had just called `showShotArrow()`,
so `hideArrow()` -> `cancelPull()` removed its `shown` class immediately.
`feed()` also hid feedback on forward-to-ready transitions. Both now route
results through `finishStroke()`: an accepted throw retains its feedback;
cancellation or rejection still hides it. Nudge, reset, menu, pause and
height/surface changes retain their existing explicit hiding.

In `resources-app/dist/site/cornhole/index.html`, `showShotArrow()` already
used `Math.atan2(b.v.x, -b.v.z)`, the real launch yaw including scaled aim and
drift. It and `host.preview()` drew a short screen-space vector using
`powerArrowLength()`. The shared `yard-games/power-arrow.js` caps that length
at 145 px: a 0.10 rad lean moved the tip only about 14 px sideways.

The new `showDirectionArrow()` projects a nine-metre horizontal bearing from
the release position at hand height through the scene camera. Preview uses
`shot.aim` as supplied (already 1/3 scaled); release uses actual velocity yaw
and the launched bag's x. Length is independent of power. The long, thick,
outlined shaft and pointed head retain power coloring; the short track and
limit dot are hidden. No new player text. Hard leans visibly spread right or
left; zero aim follows world -Z from the nudged stance, parallel to the board.
There is no hole targeting, raw spinner yaw or artificial yaw amplification.

`showAim()` now holds for `ARROW_LINGER_MS=2000`, matching the vertical meter's
`POWER_LINGER_MS`, then uses the existing 0.4-second opacity fade. Previously
its timeout was 2400 ms. `cancelPull()` clears the timer and hides immediately.

Power math, 80 ms speed window, full-power 6048, nudge step/clamps, 1/3 aim,
spin/curl and the 1.0.5 straight-path fix are unchanged. The shared arrow helper,
website and all earlier snapshots are unchanged.

## Validation

From this snapshot directory:

- `node --test tests/*.test.mjs`: PASS, 5 test files, 0 failures.
- `node --test --test-isolation=none tests/*.test.mjs`: PASS, 59 individual
  tests, 0 failures, 0 skipped (54 inherited plus 5 new).
- New production-source harness tests cover actual launch velocity yaw with
  drift and nudged release x; mirrored leans; straight aim; capped ±0.10 aim
  versus weak ±0.025 aim; real Trackball strokes through preview and launch;
  input-driven and idle-frame throws, cancellation and rejection; and the
  2000 ms timer, fade rule and explicit cancellation cleanup.
- Projected capped tips exceed 5% of screen width laterally at 1280x720,
  1920x1080 and 720x1280, and spread about four times farther than weak leans.
  Soft throws retain the same long guide. No visual cabinet playtest performed.
- `node --check`: PASS for top-level `arcade/controller.js`, snapshot
  `resources-app/dist/site/arcade/controller.js`, both edited test files,
  and the extracted Cornhole HTML inline module.
- `git diff --check`: PASS; copied source/test whitespace also checked.
- Existing bundled Three.js module-type warning remains; vendor untouched.

## Files changed

Relative to 1.0.5, only these files in the new copy differ:

- `resources-app/dist/site/arcade/controller.js`
- `resources-app/dist/site/cornhole/index.html`
- `resources-app/package.json`
- `tests/controller.test.mjs`
- `tests/spin.test.mjs`
- `READ-ME.txt`
- `DEV-NOTES.txt`
- `CODEX_SUMMARY.md`

Outside the snapshot:

- `arcade/CHANGELOG.md`
- `arcade/cabinet-snapshots/README.md`
- `arcade/package.json`
- `arcade/package-lock.json` (only the two root version fields)
- `arcade/README.md` (header only)
- `arcade/controller.js` (fallback kicker version only)

All other copied files are byte-identical, including vendor assets; symlink
unchanged. Earlier snapshots and changelog entries remain untouched. Root
`CHANGELOG.md` remains an index and needed no edit.
Work stayed in `/workspace/cornhole-1.0.6` on `release/1.0.6`.
No rebuild, commit, tag, push, bundle, cabinet install or purchase performed.
Commit, tag, bundle and install remain with the caller.
