# Bug: Beer Pong ball can no longer be thrown

Ashe reports (Nov 4 2026): after a recent update, the player can no longer throw the ball in Beer Pong (/beer-pong). It used to work.

Suspects: the most recent Beer Pong PRs — #25 (88435b6 "Add human swats and CPU bounce shots"), #23 (893848d "Fix automatic rebounds and lengthen the table"), #22 (599ea3f one-bounce shots + CPU glove defense), and #24 (a15f24a copy tightening across all games, which touched beer-pong too). Use `git log -p -- beer-pong` / `git bisect` to find the commit that broke throwing.

Do:
1. Reproduce in a headless browser (Playwright/puppeteer is fine; see tests/beer_pong_input_check.cjs) on desktop (mouse flick) and phone-size touch (swipe). Confirm the ball does not launch.
2. Find the root cause and the commit that introduced it.
3. Fix it with the smallest change that restores throwing, without removing the swat/bounce features.
4. Add or extend a regression test that fails before the fix and passes after (a human flick/swipe launches the ball and it travels toward the cups).
5. Run all tests in tests/ (beer pong + home + any others that run) and make sure they pass.
6. Write codex-summary-pong-throw.md: root cause, breaking commit, fix, tests run. Save before/after screenshots to shots/pong-throw-*.png if possible.
Do not touch other games except for shared code the fix needs.
