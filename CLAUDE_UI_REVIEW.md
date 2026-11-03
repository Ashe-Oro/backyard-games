# Backyard Cornhole — UI/UX polish review

Scope: current `index.html` (matches HEAD @ 42dd814, "batch 5: restore v2 camera, hide HUD on menus") plus `shots/live-v4.png` (captured after that fix). Most other screenshots in `shots/` (desktop.png, card.png, mobile.png, career-intro.png, career-champion.png, task3-*, task5-*, wind-*, career-trajectory-*, career-flight.png) were taken **before** batch 5 landed and show the old menu/HUD overlap bug that's already fixed — I did not re-flag that. Findings below are grounded in the current source and `live-v4.png`.

The owner wants this kept simple — every fix here is either a bug (wrong behavior vs. what the code/HUD clearly intends) or a one-line CSS/copy tweak. Nothing proposes new systems.

## Top 10

1. **"RED RED" / "BLUE BLUE" duplicated name tag in 2P and Solo Practice** (`index.html:97,99,675`)
   - Problem: `#t0`'s markup hardcodes `RED <span id="n0">...</span>`, and `teamName()` returns `TEAM[t]` (literally `'RED'`/`'BLUE'`) whenever `M.vs !== 'cpu'`. In 2-Player and Solo Practice the span re-prints the same word the markup already hardcoded, producing "RED RED" / "BLUE BLUE" — confirmed live in `shots/live-v4.png` top-left.
   - Why it matters: reads like a broken string substitution on the very first thing players see; undermines trust in the rest of the HUD.
   - Fix: drop the hardcoded "RED"/"BLUE" words from the markup (the `.red`/`.blue` CSS classes already color the name) and just show the dynamic label — "YOU", rival name, or nothing for 2P. Simplest: change `teamName()` to return `''` for the 2P/practice case and let the team-colored name area read "YOU"/opponent name only, or player-neutral labels.

2. **`#sb`'s flex `space-between` breaks when the blue team block is hidden (Solo Practice)** (`index.html:36,708`)
   - Problem: `renderHud()` sets `ui.t[1].hidden = M.practice`. `#sb` is `display:flex; justify-content:space-between` across 3 children (red team / mid / blue team). Removing the blue team from flow leaves only 2 children, so `space-between` shoves the `.mid` block (INNING / practice stats line) all the way to the right edge instead of centering it — visible in `live-v4.png` as "SOLO PRACTICE · 0 IN · 0 On the Board · 0% HIT · 9 THROWS" crammed into the top-right corner, nearly touching the score pips.
   - Why it matters: this is the actual "HUD text overlapping" symptom players are seeing right now in Solo Practice — the fix notes focused on the menu/title case, but this flex-collapse is a second, separate cause of the same visual bug class.
   - Fix: use `visibility:hidden` instead of the `hidden` attribute on `ui.t[1]` (preserves its flex slot so `.mid` stays centered), or switch `#sb` to a 3-column grid (`grid-template-columns:1fr auto 1fr`) so the center column never moves regardless of what's hidden.

3. **Landscape-phone breakpoint drops to 6px text** (`index.html:86`)
   - Problem: `@media(max-height:450px)` sets `#hint{font-size:6px}` and `#touch-controls button{font-size:6px}`. Phone landscape heights (iPhone SE ~375px, iPhone 14 ~390px, most Androids) are well under 450px, so this isn't a rare edge case — it's the default state for anyone playing in landscape, which this game effectively requires (wide board view).
   - Why it matters: 6px in a blocky pixel font is below legible size on a phone; the control hint and touch buttons become decorative noise rather than instructions.
   - Fix: raise the floor to ~9–10px minimum at this breakpoint; if space is tight, cut words from the hint string (see #6) rather than shrinking type further.

4. **Touch-control buttons shrink to 26px tall in the same landscape breakpoint** (`index.html:86`)
   - Problem: `#touch-controls button{min-height:26px}` under `max-height:450px` — well below the ~40–44px touch-target minimum, on exactly the devices (landscape phones) most likely to be using these buttons.
   - Why it matters: STEP/AIM/SPIN taps will miss or fat-finger adjacent buttons during actual play.
   - Fix: keep `min-height` at 36px+ even in the compact breakpoint; shrink padding/gap instead of the tap target.

5. **Hint text runs into the "PWR" meter label at the bottom on narrow/portrait viewports** (`index.html:56,60`)
   - Problem: `.mlbl` (the "PWR" label) sits immediately left of the meter bar; `#hint` spans full width ending near the same spot. On narrow viewports they collide with no gap — visible in `shots/mobile.png`, `shots/wind-390.png`, `shots/task5-held-390.png` as "...PICK PUSH / NORMAL / KOBEPWR" running together.
   - Why it matters: looks like a layout bug (because it is one) and makes the last instruction word unreadable.
   - Fix: add `padding-right` to `#hint` sized to clear the meter + PWR label width on narrow screens (there's already a per-width media query at `:560px` — extend it), or reserve a fixed right-margin equal to meter width + label.

6. **The MOUSE/SPINNER control-toggle button is permanently `hidden`** (`index.html:108`)
   - Problem: `<button id="mode" hidden>MOUSE</button>` — nothing in the JS ever removes the `hidden` attribute (confirmed via full-file search). `toggleCtrl()` still works from the `m` key, but there is no visible way to discover, see the current state of, or toggle it with mouse/touch.
   - Why it matters: it's dead UI — a button that exists in the DOM and has a click handler wired up but can never be seen. Either finish wiring it (show it when a mouse/pointer is detected) or delete it; right now it's just inert code.
   - Fix (keep-it-simple route): delete the button and the `m` binding if the drag-to-aim scheme is the only one meant to ship. If the spinner mode is meant to stay, show the button on pointer-fine devices via `matchMedia('(pointer:fine)')` at startup.

7. **"SLICK SIDE DOWN" button labels current state, not the action it performs** (`index.html:711`)
   - Problem: the button text is literally the side that's *currently* face-down (`'SLICK SIDE DOWN'` or `'STICKY SIDE DOWN'`), but clicking it *flips* to the other side. It reads like a status readout, not a button.
   - Why it matters: first-time players are as likely to think pressing it will keep the slick side down as flip away from it — the label doesn't describe the result of the click.
   - Fix: label it by the action, e.g. "FLIP TO STICKY" / "FLIP TO SLICK" (swap which word shows based on state), so the button always names what pressing it does.

8. **Long career rival names risk colliding with the center inning readout on narrow widths** (`roster` array + `#sb .team{min-width:9em}`)
   - Problem: names like "Big Bag Barb II" get " BLUE" appended (`teamName()` + hardcoded suffix) and wrap to 2 lines on narrow viewports (`shots/task3-390.png`). The `.team` block has no `max-width`/truncation, so very long name+suffix combos can approach the centered `INNING n / FIRST TO 21` text at small widths.
   - Why it matters: it's not catastrophic today, but it's the same text-crowding failure mode as items 1–2, just one tier down in severity, and it'll get worse with longer custom names later.
   - Fix: cap `.team` name with `text-overflow:ellipsis` + `max-width`, or drop the redundant hardcoded "BLUE"/"RED" suffix per item 1 (the color already encodes team) so only the rival's own name has to fit.

9. **Zero audio feedback anywhere in the game** (confirmed: no `AudioContext`/audio element/sound call in the file)
   - Problem: toss, bounce, "CORNHOLE!"/"SWISH!"/"KNOCKED IN!" moments, and the championship win all rely purely on text pop-ins and confetti — there is no sound at all.
   - Why it matters: for an arcade-feeling toss game, a satisfying thunk/swish/cheer sound is one of the highest-leverage "juice" additions per unit of effort, and right now even a flick-release has no audio cue.
   - Fix (small, scoped): 3–4 short WebAudio-generated blips (toss release, bag-land-thud, score chime, win fanfare) — no asset pipeline needed, stays within "keep it simple."

10. **Pixel-art inconsistency: the wind arrow glyph and widget chrome don't match the rest of the UI** (`index.html:83`)
    - Problem: `#wind-arrow` renders a plain Unicode `↑` in `font:36px/1 sans-serif` — a smooth, anti-aliased system glyph sitting inside an otherwise fully pixelated scene (canvas is `image-rendering:pixelated`, UI font is "Press Start 2P"). The wind box itself also uses a thin flat `2px solid` border, while every other panel/button in the game uses the thick blocky border + offset drop-shadow (`.hb`, `#panel`).
    - Why it matters: it's the one UI element that visibly breaks the retro-pixel look, and it's also the newest element (added in batch 4), so it reads as bolted-on rather than designed-in.
    - Fix: draw the arrow as a small rotated pixel sprite (same technique already used for board/bag textures via `pixTex`) instead of a text glyph, and give `#wind` the same border+shadow treatment as `.hb`/`#panel` for visual consistency.

## Additional items (11–25)

11. **`#hud.menu` hiding relies on class name, not a verified-safe allowlist.** (`index.html:35,686`) The rule `#hud.menu > :not(#title){display:none!important}` hides everything *except* `#title` when phase is menu/intro/champion — but `#panel` lives outside `#hud` entirely (`index.html:110`), so it's unaffected either way. This is fine today, but if a future HUD element gets added outside `#hud`, this hiding mechanism silently won't apply to it. Worth a one-line comment in the code noting the invariant ("anything that must hide on menu screens must live inside `#hud`").

12. **`#title.gone{opacity:0}` leaves the title's layout box in place but invisible, not removed.** (`index.html:64`) It no longer intercepts clicks (`#hud{pointer-events:none}`) so this isn't a functional bug, but it means screen readers / tab order still encounter an invisible "BACKYARD CORNHOLE" heading during gameplay. Low priority; add `aria-hidden` toggling alongside the `gone` class if accessibility is a goal.

13. **Viewport has `user-scalable=no`** (`index.html:5`) combined with the 6–7px minimum font sizes (items 3, and the base `clamp(7px,1.9vmin,14px)` at `index.html:34`) means a low-vision player has no way to compensate for small text — pinch-zoom is explicitly disabled. Consider allowing zoom, or at minimum raising the absolute font floor.

14. **"MORE" button label doesn't hint at what it reveals** (`index.html:108`, handler at `929`). It expands the STEP/AIM/SPIN keyboard-remap panel — a reasonable progressive-disclosure pattern, but "MORE" is generic. Rename to "CONTROLS" or "KEYS" so the purpose is clear without tapping it.

15. **Hint text describes a two-step gesture as two separate instructions.** (`index.html:718`) "Flick up to throw · Point to aim" reads as two separate actions, but the actual mechanic is one continuous press→drag(aim)→flick(release) gesture. First-time players may try to "point" without pressing down (works for mouse-hover per `index.html:970`, but not for touch, which has no hover). Consider a touch-specific hint string ("Drag to aim, flick to release") vs. the desktop one.

16. **"KOBE" as a throw-height label is undocumented flavor text.** (`#height-choice` buttons, `index.html:107`) PUSH/NORMAL are self-explanatory; "KOBE" (high lob, named for the fadeaway) requires outside knowledge of the reference to understand it means "high arc." Low priority given the game's playful tone, but a one-word subtitle or distinct icon per button would remove the ambiguity without breaking the joke.

17. **Wind widget can visually overlap the opponent's dog sprite / cooler prop.** (visible in `live-v4.png`, `wind-1280.png`) It's a fixed 2D-positioned HUD box over a 3D scene, so at some camera angles it sits directly in front of background props. Not text-on-text, but still a minor z-order/clutter issue. Lower priority than items 1–5 since nothing becomes unreadable.

18. **`#conditions`/`#setup` status line is fully dead code.** (`index.html:89,710`) `$('setup').textContent` is still being computed every frame (`CLASSIC · SPIN 0 · STEP 0.00`) but `#setup{display:none}` means it's never shown. Harmless today, but it's a confusing read for anyone editing this file later ("why does this update something invisible?"). If the status line is permanently retired, delete the `textContent` write and the backing DOM node; if it's meant to return, wire up the display instead of leaving it off.

19. **Height-choice buttons (PUSH/NORMAL/KOBE) don't indicate whose turn's arc they're setting when height differs per player.** (`M.heights[thrower()]=M.arc` at `index.html:932`) Each player can have an independent default arc, but the UI only shows one active/yellow-highlighted button (the shared `#height-choice` row) with no per-player memory indicator — so switching from Red's turn to Blue's turn, the highlighted button silently represents a different stored value. Minor but worth a glance: confirm `renderHud()` re-syncs the highlighted button to `M.heights[thrower()]` on turn change (it currently keys off `M.arc`, which is shared global state, not the per-player array) — worth double-checking this doesn't desync in 2P.

20. **Practice-mode goal/stats line is dense and tiny at `.75em`.** (`index.html:83,709`) "0 IN · 0 On the Board · 0% HIT · 9 THROWS" packs 4 stats into one small centered line — readable at 1280px, cramped below ~480px even before the item-2 flex bug is fixed. Consider 2 lines (accuracy stat separate from the throw tally) once item 2 is fixed and the block is centered again.

21. **No empty/first-load state difference between "just opened the game" and "returned from a match."** Both land on the same 3-button main menu with the title card. Not a bug, just worth confirming this matches the intended simple flow — no onboarding/tutorial step is currently shown to brand-new players beyond the persistent bottom hint, which is consistent with "keep it simple" but worth a conscious choice rather than a default.

22. **Confetti `burst()` only fires at the hole.** (`index.html:864,868`) "Knocked in" (opponent's bag pushed into the hole) reuses the same hole-position burst, which is correct, but a cancellation-scored "on the board" point gets no particle feedback at all — only text. Minor asymmetry in feedback weight between a 3-point make and a 1-point board hit; could add a smaller/plainer particle puff for board hits if more "juice" is wanted, though this is explicitly optional given the simplicity goal.

23. **Career "NEXT UP" card and the in-match HUD both use the purple `#panel` chrome with a `.go` yellow CTA**, which is good for consistency — but the "LET'S TOSS" / "CIRCUIT MAP" buttons are stacked with no visual hierarchy beyond color (same width, same spacing). On the 390px shot (`task5-intro-390.png`) the secondary "CIRCUIT MAP" button is barely distinguishable from the primary CTA at a glance. Consider a smaller/plain style for the secondary action, consistent with how `.hb` already differs from `.go` elsewhere.

24. **`#panel{max-width:94vw}` combined with long taunt strings can force a lot of line wraps on 390px width** (`task5-intro-390.png` shows 6+ short wrapped lines in the card body). Not broken, just dense; shortening taunt copy (already short) or increasing line-height slightly at narrow widths would reduce the "wall of text" feel the owner is trying to avoid elsewhere.

25. **Score pips (`.pips i`) and the big score number (`.ts`) both update on every `renderHud()` call, with no transition/animation between old and new score.** Given confetti and the `#msg` pop-in already provide "juice" for the throw result, the actual score-counter update itself is an instant, silent jump cut, 1 beat after the celebratory message. A brief scale-pulse on the score number when it changes (CSS-only, a few lines) would tie the message and the number together instead of them feeling like two separate events.

## Notes on what's already fixed / not re-flagged

- The literal bug described in `CLAUDE_REVIEW_NOTES.md` — HUD text overlapping the title on the menu screen — is fixed by the `#hud.menu` CSS rule + `renderHud()`'s class toggle added in commit `42dd814`. I verified this is live in the current `index.html` (matches working tree, no uncommitted diff).
- The "RED RED" duplicate-name bug mentioned in the same notes file is **not** fixed — see item 1 above, with the root cause and a concrete one-line-ish fix.
