# Codex Task 13: two approved polish items (from CLAUDE_UI_REVIEW_2.md)

1. Blurry or fused small text on phones (review item #1). The `.ol` class uses a fixed 2px 8-direction text-shadow outline. At the `#hud`/`#panel` clamp floor of 7px on portrait phones, letters fuse: "Backyard Circuit" reads as "Backuard Cjrruit" and the minus sign in the round recap smears.
   - Change the `.ol` shadow offsets from px to em (about .12–.14em) so the outline scales with the font.
   - Raise the `#hud`/`#panel` font clamp floor from 7px to about 10px.
   - Verify at 390x844 and 456x1024 (deviceScaleFactor 2) that the opponent card, HUD and round-end recap are crisp, with no overlap.
2. Bag flight trail and a board-hit puff (review items #4 and #10).
   - While a bag is flying, show 2–3 short fading afterimages of the bag, or a small pixel dust trail. Keep it subtle and cheap: no per-frame allocations, reuse pooled meshes.
   - When a bag lands on the board and scores (ON THE BOARD), play a small puff using burst() with about 12–15 plain pieces at the landing spot. Keep the big confetti for in-the-hole only.
Don't change anything else: no new buttons, camera static, wording "On the Board" and "Round", keep all batch 9–12 behavior. Use short test timeouts. Save screenshots to shots/task13-*.png, including a mid-flight shot and a phone text shot. Don't write to .git. Write a summary to codex-summary-13.md.
