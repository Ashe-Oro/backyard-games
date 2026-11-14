# Backyard Cornhole Arcade 1.0.1

Prepared on `release/1.0.1` from a copy of the live-cabinet 0.2.23
snapshot, preserving `app -> resources-app`.

Default full-power speed rises from 5600 to 7560 counts/sec (+35%).
Power remains linear in peak forward speed over the unchanged 80 ms window:
`min(1, speed / powerSpeed)`. Distance does not grant power. The clamp remains
400–10000. Explicit `settings({powerSpeed:1050})` still accepts 1050.

Profile loaders migrate missing/null speed and factory speeds 1050, 1650,
2100, 3300, 4000, 5600 and 6500 to 7560. Custom v9–v5 speeds such as 2000
survive. Legacy v4–v1 profiles migrate factory values and floor other values
at 7560; custom 9000 survives.

Validation from this snapshot directory:

- `node --test tests/*.test.mjs`: 5 test files passed, 0 failed.
- `node --test --test-isolation=none tests/*.test.mjs`: 45 tests passed,
  0 failed, 0 skipped.
- `node --check` passed for all five edited JS/MJS files: snapshot
  trackball.mjs, controller.js, trackball.test.mjs, controller.test.mjs,
  and top-level arcade/controller.js.
- Power checkpoints cover 3780 = 50%, 7560 = 100%, and 9000 capped at 100%,
  using tight tolerances for floating-point fixture arithmetic. Measured
  1650/825 fixtures retain their speeds and divide power by 7560.
- Profile tests include 5600 and custom 2000 across v9–v1, including the
  controller storage loader. All existing button, legend, linger, spin,
  and beer pong tests remain.

Changed snapshot files: resources-app/dist/site/arcade/trackball.mjs and
controller.js, resources-app/package.json, tests/trackball.test.mjs and
controller.test.mjs, READ-ME.txt, DEV-NOTES.txt, and this summary.
Updated cabinet-snapshots/README.md with install timestamp and rollback path
pending. Updated only release version strings in arcade/package.json,
the two root package-lock.json version fields, README.md and controller.js.

The 0.2.23 and earlier snapshots remain untouched. All other copied files,
including vendor files, are unchanged. Buttons, legends, KEYS, 3000 ms linger,
spin curl, aiming, nudge step, calibration / 0.7 and beer pong are unchanged.
No licensed binaries, Electron runtime files, Ports videos, gamelist.xml or
/userdata files were edited. No money spent, cabinet install, push or commit.
Installation and commit are left to the caller.

## Blue cooler removal follow-up

Removed the blue cooler comment, cooler-only materials, helper and placement
call from resources-app/dist/site/yard-games/backyard.js. Kept shared metal
and cloth materials and all other props unchanged. Updated READ-ME.txt,
DEV-NOTES.txt and the parent snapshot index; pending installation and rollback
fields remain unchanged. No cooler assertions existed, so no tests were edited.

Follow-up validation:

- `node --test tests/*.test.mjs`: 5 test files passed, 0 failed.
- `node --test --test-isolation=none tests/*.test.mjs`: 45 tests passed,
  0 failed, 0 skipped. Node emitted a module-type warning for bundled Three.js.
- `node --check resources-app/dist/site/yard-games/backyard.js`: passed.
- Verified canonical backyard.js, arcade/build.mjs and all 213 protected-file
  hashes (including earlier snapshots) remain unchanged.

No build, commit, push or cabinet installation was performed.
