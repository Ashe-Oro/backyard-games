# Cornhole batch 7

Completed CODEX_TASK_7.md.

- Removed STEP and AIM arrow buttons, SPIN buttons, MORE toggle, their handlers, and unused panel styles.
- Removed Arrow Left/Right, Q/E, and A/D control bindings. Aim comes from mouse/finger movement. Existing pointer throw modes, flicks, and wheel throws remain available.
- Human throws launch from the centered stand position with spin/English zero, including sideways flicks. CPU internal variation remains available.
- Removed step/spin HUD readouts. The bag options are THROW HEIGHT (PUSH / NORMAL / KOBE) and BAG SIDE DOWN (SLICK / STICKY); menu and practice reset actions remain.
- Updated existing Playwright suites to remove obsolete control expectations and spin probes. Added assertions for absent controls/readouts, inactive removed shortcuts, and centered zero-spin human launches with sideways input.

## Validation

All six Playwright suites passed:

- career_check.py
- playwright_check.py
- task3_check.py
- task5_check.py
- task6_check.py
- wind_flag_check.py

Coverage includes career persistence and champion progression, physics/scoring, mouse/touch aiming and throws, wheel throws, iframe play, modes, practice/reset, per-player height, camera framing/following, responsive panel layout, wind widget and animated flag. Updated layout checks run at 1280×800, 480×360, 390×844, and 844×390. Inspected the refreshed phone screenshot. Suites refreshed their screenshot artifacts.

Command (with the local server running on port 8000):

```sh
PLAYWRIGHT_BROWSERS_PATH=/tmp/pw-browsers /tmp/cornhole-venv/bin/python tests/run_all.py
```

The runner uses 8-second default waits, 12-second navigation timeouts, and a 150-second limit per suite. Initial attempts with system Python/default browser cache failed due to missing dependencies; the existing environment above resolved this. Final run: all suites passed. Full output: `/tmp/b7-tests.log`.

`git diff --check` passed. `CLAUDE_UI_REVIEW.md` was not edited. No writes to `.git` and no commits; changes remain in the working tree.
