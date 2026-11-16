# Backyard Cornhole Arcade 1.0.7

Copied `1.0.6/` to `1.0.7/`, preserving `app -> resources-app`.
Snapshot package version is 1.0.7; the label/kicker still read the runtime
version through `app.getVersion()`. Install timestamp and rollback path: pending.

## Root cause and fix

Two oversized indicators competed for attention: the separate 160 px vertical
POWER meter (110 px on short screens) and the fixed 9 m world-space direction
shaft. The shaft's length did not communicate power.

Removed the meter markup, CSS, frame-loop updates, aria wiring, unused color
import and POWER_LINGER_MS. Diagnostic lastShot data remains available.
One compact arrow starts at the bag/stance release x and release height.
`showDirectionArrow()` projects a unit horizontal launch bearing through the
camera, then normalizes it to the power-dependent screen-space length. This
preserves the projected bearing without keeping the oversized 9 m shaft.
`showShotArrow()` still uses `Math.atan2(b.v.x, -b.v.z)`, including launch drift;
preview uses `shot.aim` unchanged, already scaled by the Trackball aim rule.
Zero aim follows world -Z from the nudged stance. No hole targeting or raw
spinner angle is introduced.

The shaft is `max(24, clamp(power, 0, 1) * 110)` px long. Soft throws have a short
visible shaft; full power fills a faint 110 px track to its endpoint. The
cool-to-hot color reinforces power. Shaft/outline widths are 3/6 px rather
than 6/10 px. No player labels were added. The arrow appears in forward preview
and after release, holds for `ARROW_LINGER_MS=2000`, then fades over 0.4 seconds.
`finishStroke()` retains accepted throws and hides cancelled or rejected strokes.
Explicit reset, nudge, pause and menu hiding remain intact.

Speed-based power, peak over 80 ms, full-power 6048, placement clamps, CLEAR/KEYS,
upper-right runtime version, spin/curl, and 1.0.5 parallel straight flight remain
unchanged. Aim remains `clamp(angle * .10, -.10, .10)` in `trackball.mjs`.

## Validation

From this snapshot directory:

- `node --test tests/*.test.mjs`: PASS, 5 test files, 0 failures.
- `node --test --test-isolation=none tests/*.test.mjs`: PASS, 59 individual tests,
  0 failures, 0 skipped.
- Obsolete meter-specific tests were replaced with absence checks. All other
  existing regression coverage remains: power/window, runtime version, CLEAR/KEYS,
  nudged straight landings, spin, beer pong, reset, clamps and stroke cleanup.
- Arrow tests execute production helper, projection, preview, launch and SVG
  rendering code. They cover soft/mid/full lengths, constant full-power track,
  matched shaft/outline endpoints, full fill, color, straight direction across
  powers, mirrored extremes versus weak leans, actual yaw with drift and nudged
  stance, real scaled Trackball strokes, preview/release agreement and the timer.
  Full-power shafts stay at 110 px across landscape and portrait fixtures.
  The old 5%-of-screen assertion was replaced with compact lateral separation
  checks: capped leans exceed weak leans by over 15 px at full power.
- `node --check`: PASS for top-level `arcade/controller.js`, snapshot controller,
  power-arrow helper, both edited test files and extracted Cornhole inline module.
- `git diff --check` and edited-source whitespace checks: PASS.
- Existing bundled Three.js module-type warning remains; vendor untouched.
- No cabinet visual playtest or screenshots performed.

## Files changed

Relative to 1.0.6, only these files in the new copy differ:

- `resources-app/dist/site/arcade/controller.js`
- `resources-app/dist/site/arcade/arcade.css`
- `resources-app/dist/site/cornhole/index.html`
- `resources-app/dist/site/yard-games/power-arrow.js`
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
unchanged. Earlier snapshots, web, root changelog index and earlier changelog
entries remain untouched. Work stayed in `/workspace/cornhole-1.0.7` on
`release/1.0.7`. Pre-existing untracked task notes/log were left alone.
No rebuild, commit, tag, push, bundle, cabinet install or purchase performed.
Commit, tag, bundle, cabinet install and screenshots remain with the caller.
