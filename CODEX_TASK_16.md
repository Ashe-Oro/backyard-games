# Codex Task 16

## Collapsible left bag menu (owner request)
The owner wants to dismiss the left bag menu (THROW HEIGHT, BAG SIDE DOWN, PITCHER'S BOX, RESET BAGS/MENU) after choosing, then pull it back out to change something.
- Add a collapse/expand behavior: a small pixel-styled tab or handle on the panel's edge (e.g. a "◀" chevron or a "BAG" tab) collapses the panel off-screen to the left, leaving only a compact tab. The tab shows the current picks in tiny text, e.g. "NORMAL · SLICK". Tapping the tab slides the panel back out. Use a quick slide animation of about 150ms.
- This is the panel's own handle, not a new control button. Keep it minimal. It needs a touch target of at least 44px on phones and 32px on desktop.
- Remember the collapsed or expanded state in localStorage. The default is expanded on first visit.
- When collapsed, taps and flicks on the game area must work normally. The tab must not swallow throw gestures that start elsewhere.
- Screenshot shots/task15-menu-{expanded,collapsed}-456x1024.png and 1280.

Keep all earlier batch behavior, including batch 15's consistent aim preview. Use short test timeouts. Don't write to .git. Write a summary to codex-summary-16.md.
