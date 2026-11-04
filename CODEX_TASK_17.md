# Codex task 17: owner feedback on live v7 (commit 10e9430)

The owner played live v7 and reported three things.

1. "The 2nd board isn't necessary." Remove the second cornhole board (the one at the player's end / the extra board visible in frame). Only the target board should be drawn. Keep the throw lanes and pitcher's boxes, and keep all scoring logic. Make sure nothing references the removed mesh.
2. "The bags look too small." Make the bags, both the held bag and the bags in flight or landed, noticeably bigger on screen, so they read clearly on a phone. Grow the visual scale only modestly (around 1.3–1.5x on screen), keeping the bag-to-hole proportion believable: a bag must still fit through the hole.
3. "The game got much, much more difficult. I think it's even more zoomed out, when we were going to make it less zoomed out." Recent framing work (batch 14 phone portrait bag framing, and possibly batch 9's static camera) seems to have pulled the camera back. Investigate what changed the framing and difficulty versus commit 251b9b9 (batch 12), using git diff. Then reframe so the target board is bigger and closer in frame (less zoomed out) on phone portrait (390x844, 456x1024) and on desktop 1280x800.
   - Throws must feel as forgiving as they did at batch 12. If any physics, launch or aim mapping changed difficulty unintentionally, restore it.
   - The held bag must sit lower-center in the foreground at the start of the aim path. Its launch point must equal the displayed position.

Keep all prior constraints:
- The camera stays static while aiming.
- Flick-only throws.
- The left bag menu (PUSH/NORMAL/KOBE, SLICK/STICKY, LEFT/RIGHT in practice) stays collapsible.
- The aim preview must not change on touch-down.
- Wording: "On the Board" and "Round".
- The CPU ladder rates and characters are unchanged.
- X Player Card meta tags and vercel.json are unchanged.

Verify:
- Serve locally and take Playwright screenshots at 390x844 and 456x1024 (touch, DPR 2), 844x390 and 1280x800 into shots/task17-*.png.
- Run the tests in tests/ and keep them passing.
- Write codex-summary-17.md covering the cause of the zoom-out/difficulty, what you changed, and test results.
- Run everything in the foreground with timeouts.

## Clarification from the owner's screenshot (shots/owner-v7-solo-two-boards.png, Solo Practice on phone)
The board to REMOVE is the NEAR one in the foreground, inside the player's pitcher's box at the bottom of the screen. The target is the FAR board. With the near board gone, the held bag should sit where that near board was drawn: lower-center, big and clear. Also note that in this screenshot the target board looks tiny and far away. Make it noticeably bigger in frame.
