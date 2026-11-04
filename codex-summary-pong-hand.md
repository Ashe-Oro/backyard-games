# Beer Pong hand swipe

Replaced the CPU and human hand's animated world-space movement/rotation with a shared 280 ms horizontal swipe. Each hand starts beyond the nearest viewport edge, moves to the projected impact point, then retreats beyond the same edge. Depth, screen height, pose and scale stay fixed during the swipe. Projection/unprojection keeps the motion horizontal on desktop and portrait/landscape phones. The existing glove mesh is reused.

CPU contact simulation, attention, reaction windows, difficulty, human hit/miss checks and cooldowns are unchanged. Bounce rewards and upward-flick throwing are unchanged. CPU visual animation is independent of the physical defender, so preserving its gameplay does not require a timing adjustment.

Extended the AI test with both-edge trajectory checks (offscreen start/end, impact, constant height/depth and monotonic entry). Browser bounce/human swat tests inspect the rendered hand's projected position, fixed rotation and scale. The initial browser assertion sampled too late under concurrent browser load; it now checks the actual trajectory's start and subsequent rendered motion. The pure trajectory check uses a tolerance for floating-point interpolation.

Screenshots inspected: `shots/pong-hand-before-cpu.png`, `shots/pong-hand-before-human.png`, `shots/pong-hand-after-cpu.png`, `shots/pong-hand-after-human.png`, `shots/pong-hand-after-human-390.png`, `shots/pong-hand-after-human-844.png`.

Validation uses a static server (`python3 -m http.server 8012`) and the existing temporary `/tmp/pong-chrome.cjs` Playwright adapter to launch `/usr/bin/google-chrome` with `--no-sandbox`:

```sh
for test in tests/beer_pong*check.cjs tests/beer_pong*check.mjs; do
  NODE_OPTIONS=--require=/tmp/pong-chrome.cjs node "$test"
done
```

The input regression initially sampled a stale ball projection immediately after switching to the blue camera. Its origin helper now waits two animation frames (as the existing position test does); the full input suite passes on rerun. No throw implementation changed.

All 11 Beer Pong JavaScript suites pass: bounce, main game, feedback, human swat, input (after the synchronization fix), natural bounce, position, re-rack, CPU re-rack, throw, and swat AI. Dedicated screenshot runs of bounce and human swat also pass. `git diff --check` passes. No other games were edited.
