# Cornhole batch 7 (Ashe, Nov 3, 3:38 PM): remove complication
- Remove the STEP ←/→, AIM ←/→ and SPIN L/R buttons and the MORE toggle entirely. Aim is by pointer only (mouse/finger), as in v2.
- Remove step and spin as player controls (including keyboard shortcuts and HUD readouts). Throws use spin 0 and the centered stand position. CPU may keep its own internal variation.
- The left-side bag menu keeps only: throw height PUSH / NORMAL / KOBE, and bag side SLICK / STICKY.
- Update the Playwright tests to match (drop step/spin checks), run all suites with short timeouts, write codex-summary-7.md. .git is read-only; leave changes in the working tree.
