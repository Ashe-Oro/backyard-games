# Codex Task 12: final tournament ladder numbers

The owner updated the targets after batch 11 started. Re-tune the CPU opponents to these approximate per-bag rates (in the hole / on the board). Verify with a headless CPU-only sim of 200+ throws each, and put a table of the measured rates in codex-summary-12.md.
1. Slidin' Steve: under 10% (about 5-8%), about 30%. Never kobe.
2. Big Bag Barb: about 15%, about 35%.
3. Kobe Kowalski: about 25%, about 35%.
4. Sticky Vicky: about 35%, about 35%.
5. Tailgate Ted: about 45%, about 30%, high variance.
6. NEW CHARACTER (replaces Slidin' Steve II): about 55%, about 30%.
7. NEW CHARACTER (replaces Big Bag Barb II): about 67%, about 25%.
8. Reign Supreme: about 80%, about 15%. The owner set this one explicitly.
Change only CPU tuning; keep the styles. Use short test timeouts. Do not write to .git.

## No reused characters, so every opponent has a unique avatar
The owner says not to reuse characters. Replace the rematches "Slidin' Steve II" and "Big Bag Barb II" with two brand-new opponents. Give each one an original name, a nickname, a hometown, a record, a taunt, a style (pick distinct play styles, e.g. a "spinner" who curls bags and a "bulldozer" who line-drives to clear blockers, or similar, implemented through existing CPU params) and a unique color.
- All 8 opponents must have visually distinct pixel portraits. Today the portrait() function picks hair by level%4, so faces repeat. Make each portrait unique: a different hair style and color, skin tone, an accessory (cap, visor, sunglasses, headband, beard or crown), and clothing color. No two should look alike at a glance. Reign Supreme keeps a crown.
- Update any tournament text that mentions "rematch" or "II" so it fits the new roster. Keep the career save key working: existing saves index by fight number, which is fine.
- Screenshot all 8 opponent intro cards to shots/task12-opp-{1..8}.png and check that they're distinct.

## Held bag misaligned with the aim path (owner, phone portrait)
See shots/owner-phone-bag-offset.png (phone portrait, about 456x1024 CSS). The held red bag sits well to the right of the pitcher's box/lane markers and of the path toward the board, so it doesn't look like it will travel down the aim line.
- The held bag's resting position must line up with the thrower's current standing spot and aim line, so that a straight (aim 0) throw visibly starts from the bag and heads at the board. Check this at 1280x800, 844x390 and 456x1024 portrait, since a portrait aspect may be pushing it right.
- The bag's launch point must match its displayed position, with no jump at release.
- Keep the camera static.

## Solo Practice: choose a left or right side
When starting SOLO PRACTICE, let the player pick which pitcher's box to throw from: LEFT or RIGHT. Use a simple two-button choice in the same pixel style, or a toggle in the left bag menu, whichever is simpler and cleaner. Remember the choice in localStorage. The thrower position, held bag and aim line all use that side's box. Keep the default as it is now.

Screenshots: shots/task12-bag-{1280,844,456}.png and shots/task12-practice-left.png, shots/task12-practice-right.png.

## Left bag menu is too small on mobile (owner)
In phone portrait (shots/owner-phone-bag-offset.png, about 456x1024 CSS), the lower-left menu buttons (PUSH/NORMAL/KOBE, SLICK/STICKY, RESET BAGS, MENU) are too small to tap.
- On touch/coarse-pointer or narrow screens, make every button at least 44px tall with a label font of at least 12px, and make the panel use more width: up to about 60% of the screen in portrait, or a full-width bottom-left dock that does not overlap the power meter on the right.
- In phone landscape (844x390), use at least 36px buttons and a 10px font without covering the board.
- Desktop stays as is.
- Use the existing @media (pointer:coarse) or width breakpoints. Screenshot shots/task12-menu-{456x1024,844x390}.png.
