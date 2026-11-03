# Cornhole batch 8: Claude polish items Ashe approved (Nov 3, 3:49 PM)
Details for each are in /workspace/cornhole/CLAUDE_UI_REVIEW.md (items in parentheses). Do ONLY these four:
1. (#2) Solo Practice stats are shoved to the top-right because the hidden blue team block collapses #sb flex space-between. Make #sb a 3-column grid (1fr auto 1fr) or use visibility:hidden so the center stays centered. Split the practice stats into 2 lines if cramped (#20).
2. (#3, #4) Phone landscape (max-height:450px): raise text to at least 9–10px and keep touch buttons at least 36px tall. Shrink padding/gaps instead.
3. (#19) Per-player throw height in 2 Player: the PUSH/NORMAL/KOBE highlight and M.arc must re-sync to M.heights[thrower()] on every turn change, so each player's own pick is shown and used.
4. (#10) Wind widget: replace the Unicode arrow with a pixel-art arrow sprite/canvas rotated to the wind direction, and give the wind box the same thick border + offset shadow as .hb/#panel.
Run all Playwright suites with short timeouts, write codex-summary-8.md. .git is read-only; leave changes in the working tree.
