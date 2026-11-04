# Beer Pong: simplify the swat hand

Ashe (Nov 4 2026, after trying the game): "The hand looks too weird. It should just swipe from left and right."

The swat/glove hand (beer-pong/glove.js and related code in beer-pong/game.js / style.css, used for CPU glove defense and human swats from PRs #22 and #25) currently animates in a way that looks odd.

Do:
1. Look at how the hand currently renders and animates (take screenshots of a CPU swat and a human swat mid-animation: shots/pong-hand-before-*.png).
2. Change it so the hand simply enters from the LEFT or RIGHT edge of the table/screen, does a quick horizontal swipe across the ball's path, and exits back out the same side (or across). Pick the side nearest the ball (or alternate) so it reads naturally. No weird rotations, pops, scaling from the center, or appearing out of nowhere. Keep it clean and readable on phone and desktop.
3. Keep swat gameplay rules, timing windows, and hit/miss logic the same unless the new path requires a minimal timing tweak; keep the bounce-shot and upward-flick-throw behavior intact.
4. Update/extend the swat tests (tests/beer_pong_human_swat_check.cjs, tests/beer_pong_swat_ai_check.mjs, tests/beer_pong_bounce_check.cjs 'hand animation') so they pass and assert the hand starts off-screen-ish at a side edge and moves horizontally.
5. Run all Beer Pong JS tests plus tests/beer_pong_throw_check.cjs (see codex-summary-pong-throw.md for how to run with /usr/bin/google-chrome and a static server). All must pass.
6. Save after screenshots shots/pong-hand-after-*.png and write codex-summary-pong-hand.md.
Do not touch other games.
