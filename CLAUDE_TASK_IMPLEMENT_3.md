Implement ALL 8 prioritized recommendations from /workspace/cornhole/CLAUDE_UI_REVIEW_3.md (your own phone-portrait review) in /workspace/cornhole/index.html. The owner approved every one.

Also fix this still-open owner bug, which is part of rec #1. In live v7 (shots/live-v7-phone.png, live-v7-phone-collapsed.png, live-v7.png), the HELD red bag still isn't in the lower-center foreground. On phone portrait it floats mid-field about 62% of the way down, just right of center. On desktop it sits mid-right by the dog and cooler. The owner wants the held bag at the start of the aim path, clearly in the lower-center foreground, before the toss (for both the LEFT and RIGHT pitcher's box, with only a modest offset). The launch point must equal the displayed bag position.

Hard constraints from the owner:
- Keep it SIMPLE. No new buttons and no bottom hint.
- The camera stays STATIC while aiming (no panning with aim).
- Flick-only throws.
- The left bag menu keeps PUSH/NORMAL/KOBE, SLICK/STICKY and LEFT/RIGHT, and stays collapsible (batch 16).
- The aim preview must not change on touch-down (batch 15).
- Wording: "On the Board" (never "Woody") and "Round" (never "inning").
- Keep the CPU ladder, characters and X Player Card meta tags unchanged.
- Desktop 1280x800 should still look good. Don't make the board huge or hide the bag (an earlier over-zoom was rejected).

Process:
- Serve with `python3 -m http.server 8012` and verify with Playwright (`PLAYWRIGHT_BROWSERS_PATH=/tmp/pw-browsers /tmp/cornhole-venv/bin/python`, short timeouts of 8s waits and 12s navigation). Screenshot 456x1024 and 390x844 (touch, DPR 2), 844x390, and 1280x800 into shots/claude-impl3-*.png and look at them.
- Run the existing tests in tests/ (with a 150s cap per suite) and keep them passing. Update a test only when it asserts something these changes intentionally alter.
- Commit your work with git in /workspace/cornhole (the repo already exists). Use clear commit messages: one or a few commits, e.g. "Claude: phone UI polish (review 3)".
- Write /workspace/cornhole/claude-impl3-summary.md: what you changed for each recommendation, the held-bag fix, test results, and the screenshot paths. Stop the server when done.


IMPORTANT: You are running non-interactively. Never background a command or schedule a wakeup or resume, because the session ends and your work is lost. Run every test and server step in the foreground with a timeout (start the http server with `&` only inside the same Bash command that runs the tests, then kill it). Make the index.html edits FIRST, then verify.
