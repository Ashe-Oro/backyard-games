# Codex Task 11: make the menu modes self-explanatory

Owner feedback: it needs to be more obvious that 2 PLAYER is human vs human and START TOURNAMENT is against the CPU.

- Under each main menu button, add a short subtitle line in the same pixel font, smaller and dimmer, kept inside the button or right under it:
  - 2 PLAYER: "HUMAN VS HUMAN · SAME SCREEN"
  - SOLO PRACTICE: "JUST YOU · NO OPPONENT"
  - START TOURNAMENT: "YOU VS CPU · CLIMB TO CORNHOLE KING"
- Optionally add a small pixel icon on each: two people, one person, a crown or CPU. Keep it simple and readable.
- Keep the buttons the same widths, or make all three the same width, which is cleaner. There must be no text overlap at 1280x800, 844x390 (phone landscape) or 390x844. Phone landscape text must be at least 9px.
- Don't change gameplay. Keep everything from batches 9-10.
- Use short test timeouts (8s waits, 12s navigation, 150s per suite). Screenshot the menu to shots/task11-{1280,844,390}.png.
- Do not write to .git. Write a summary to codex-summary-11.md.

## Also: terminology
- In cornhole it's a "round", not an "inning". Replace every player-facing "INNING" or "inning" (e.g. "NEXT INNING" becomes "NEXT ROUND", plus labels, toasts and HUD) with "ROUND"/"round". Internal variable names can stay. Check that no visible "INNING" remains.

## Also: the first tournament opponent is too good
The owner says the 1st CPU opponent in the tournament is too strong. Fight 1 should be a gentle warm-up a new player usually beats.
- Retune fight 1 only, plus a slightly easier fight 2 if needed, so the curve ramps smoothly. A beginner should win fight 1 most of the time.
- Give the CPU much more aim and power error. Targets set by the owner: under 10% of his bags in the hole (aim for about 5-8%) and about 30% on the board, with the rest missing. It should prefer normal throws and never kobe.
- Don't change later opponents or the rematches.
- Verify with a quick headless sim if possible: CPU-only throws for fight 1, then report the in-hole and on-board rates in the summary.
