# Codex Task 10: KOBE power sweet spot is too hard

Owner feedback (see shots/owner-kobe-meter.png): "while i appreciate that a kobe requires more accuracy, this is a bit much." With KOBE selected, the power meter's target (dashed box / white line) is too narrow to hit reliably.

- Make the KOBE sweet-spot tolerance only modestly tighter than NORMAL. It should be roughly 75-80% of NORMAL's window, not a sliver. PUSH and NORMAL stay as they are.
- If the meter shows a thin white line inside the dashed box as the real target, make sure the visual box matches the actual scoring tolerance, so what you see is what counts.
- Keep the tournament's later-round shift and shrink, but apply it on top of the new KOBE width. Clamp it so the KOBE window never goes below about 60% of the base NORMAL window.
- Keep SLICK/STICKY and all other behavior unchanged. Keep the camera static (batch 9).
- Run the Playwright suites with short timeouts (8s waits, 12s navigation, 150s per suite). Screenshot the meter in PUSH, NORMAL and KOBE to shots/task10-*.png.
- Do not write to .git. Write a summary to codex-summary-10.md.
