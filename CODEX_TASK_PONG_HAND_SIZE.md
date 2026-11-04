# Beer Pong: smaller swat hand

Ashe (Nov 4 2026, after the side-swipe change): "The hand is still too big."

Shrink the swat hand (human swat hand and CPU glove hand, beer-pong/hand-swipe.mjs, glove.js, game.js, style.css) by roughly 40-50% on both phone and desktop so it reads as a hand near the ball rather than covering the table. Keep the side-swipe motion from left/right, keep swat timing/hit rules exactly as they are (the hit radius is already tight; do not make it larger or smaller). Make sure it stays clearly visible on a 390x844 phone. Update any tests that assert hand size/position; run all Beer Pong JS tests (incl. throw_check and tournament check if present). Screenshots shots/pong-hand-small-*.png (desktop + 390x844), summary codex-summary-pong-hand-size.md. Don't touch other games.
