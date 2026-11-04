# Beer Pong: bounce shot derived from power, no toggle

Ashe (Nov 4 2026): "I don't like the toggle for bounce or not. It should be derived from the power."

Do:
1. Remove the bounce/no-bounce toggle (UI control, its label/help text, and any keyboard shortcut or saved setting for it) for human throws.
2. Derive the shot type from throw power: a softer throw (lower power band) lands on the table and bounces once toward the cups; a firmer throw is a direct arc (no bounce). Choose the power threshold so both are easy to do deliberately with an upward flick or pull-back on phone and desktop, and so a bounce shot can still sink a cup (keep the existing one-bounce rules: two-cup reward, swat-able, etc.). Power still controls distance within each band so all cups stay reachable both ways if practical; if not, keep all cups reachable via direct shots and make bounce reach at least the front/middle cups.
3. Optionally show a subtle in-flight cue (e.g. aim preview arc hits the table) so the player can tell a bounce is coming; no new buttons.
4. CPU bounce-shot behavior can stay as is.
5. Update tests (beer_pong_bounce_check.cjs, natural bounce, feedback, input, throw_check) to drop toggle assertions and assert low power => bounce, high power => direct. All Beer Pong JS tests + tests/beer_pong_throw_check.cjs must pass (see codex-summary-pong-throw.md for how to run with /usr/bin/google-chrome).
6. Screenshots shots/pong-bounce-power-*.png and codex-summary-pong-bounce.md. Don't touch other games.

## Also: swatting is too easy
Ashe (Nov 4 2026): "Also it's too easy to swat." Make the human swat harder to land: tighten the timing window and/or the hit radius around the ball so only a well-timed swipe near the ball's path succeeds (aim for roughly 30–40% success for a decent attempt, not near-automatic). Bounce shots are only swattable after the bounce as per existing rules. Keep CPU glove defense rates as they are unless they were clearly near-automatic too; if so, reduce them modestly and note it. Update swat tests (human_swat_check, swat_ai_check) to assert the tighter window: a late/far swipe misses, a well-timed close swipe hits.
