# Backyard Cornhole Arcade 1.0.3

Prepared on `release/1.0.3` by copying the cabinet 1.0.2 snapshot,
preserving `app -> resources-app`.

The only feature is a small upper-right running-version label, visible during
play regardless of menu HUD rules or tucked help legends. It uses 8px muted
cream text without a box and ignores pointer events. Its 1px top offset keeps
it above the top-center scoreboard, including its compact 10px top layout.

The trusted `app:version` IPC handler returns Electron `app.getVersion()`, read
from the installed package.json (now 1.0.3). Sandboxed preload exposes
`window.arcadeApp.version()`. The controller displays `v` plus that value and
fills the menu kicker from the same result. No release number is hardcoded in
the snapshot controller. Missing, invalid or rejected lookups leave the label
empty and hidden, with a plain ARCADE kicker.

Validation from this snapshot directory:

- `node --test tests/*.test.mjs`: PASS, 5 test files, 0 failures.
- `node --test --test-isolation=none tests/*.test.mjs`: PASS, 48 tests,
  0 failures, 0 skipped.
- `node --check`: PASS for snapshot main.cjs, preload.cjs, controller.js,
  tests/controller.test.mjs, and top-level arcade/controller.js.
- `git diff --check`: PASS.

New tests cover markup, placement and modest styling, runtime label and kicker
assignment with multiple versions, absent/failed bridge behavior, the actual
preload bridge invocation, and trusted/untrusted main-process IPC callers.
Existing power, migration, button, legend, linger, spin and beer-pong tests
remain intact and pass. Node emits the existing module-type warning for bundled
three.js; vendor files were not changed. No full 3D screenshot was attempted.

Changed files relative to the copied snapshot:

- resources-app/main.cjs and preload.cjs: trusted runtime version bridge.
- resources-app/dist/site/arcade/controller.js and arcade.css: label and kicker.
- resources-app/package.json: version 1.0.3.
- tests/controller.test.mjs: three additional tests.
- READ-ME.txt, DEV-NOTES.txt and CODEX_SUMMARY.md: release documentation.

Outside this snapshot, only arcade/cabinet-snapshots/README.md and the requested
version strings changed in arcade/package.json, the two root version fields in
arcade/package-lock.json, arcade/README.md and arcade/controller.js.

All other copied files remain byte-identical to 1.0.2. Power stays 6048, the
cooler stays removed, and all other gameplay and assets remain unchanged.
Earlier snapshots, licensed/vendor files and Electron runtime are untouched.
No build, push, installation, /userdata changes, preview video edits, gamelist
edits or spending occurred. Install timestamp and rollback path remain pending.
Commit and cabinet installation are left to the caller; no commit was made.
