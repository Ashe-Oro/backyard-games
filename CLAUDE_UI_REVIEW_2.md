# Backyard Cornhole — UI/UX polish review #2

Scope: current `index.html` on `master` (HEAD `8286004`, "Codex batch 11: menu mode subtitles"). All findings below are grounded in fresh screenshots captured this session at 1280×800, 844×390 (phone landscape) and 390×844 (phone portrait), in `shots/review2/`. Every item was verified against the live page via `window.__game` test hooks, not guessed from code alone — where a screenshot looked ambiguous I re-captured at 2–4x device-scale to confirm pixel-level detail before writing it up.

The owner's constraints are respected throughout: nothing below proposes a new button, a bottom hint, a moving camera while aiming, or any wording other than "On the Board." Several items below are in fact about *removing* clutter, not adding it.

## Top 10

### 1. Fixed-pixel text outline makes small UI text misread as different words — `index.html:33-34,67`
- **What's wrong:** `.ol{text-shadow:2px 0 …, -2px 0 …, 0 2px …, 0 -2px …, 2px 2px …, …}` draws an 8-direction, flat **2px** outline around all text in that class — and `#hud` and `#panel` both use it while their own font-size is `clamp(7px,1.9vmin,14px)`. On a portrait phone (390×844), that clamp floors out at **7.4px** (the existing `@media(max-height:450px)` safety floor only fires for *short* viewports, so it never kicks in here). At 7–8px, a flat 2px outlines on every side is nearly as wide as the glyph strokes themselves, so adjacent letters visually fuse.
- **Proof:** `shots/review2/02-opponent-card-390x844.png` (and the 4x zoom `zoom-card-text-390-4x.png`) shows "Backyard Circuit" rendering as **"Backuard Cjrruit"** — a real, reproducible character misread, not a copy typo (the source string at `index.html:770` is spelled correctly). Same bug, different glyph: the round-end recap at `index.html:956` prints `${pts[scorer]} − ${pts[1-scorer]} = ${net}`, which renders in `shots/review2/06-round-end-390x844.png` as **"8 _ 4 = 4"** — the minus sign gets smeared into a blob.
- **Fix:** two one-line CSS edits. (a) Switch `.ol`'s shadow offsets from `px` to `em` (e.g. `.14em` instead of `2px`, which preserves the current look at the 14px ceiling but shrinks proportionally at small sizes) at `index.html:33`. (b) Raise the `#hud`/`#panel` clamp floor from `7px` to `~10px` at `index.html:34,67`. Either alone helps; both together fixes it everywhere, including the already-noted 450px-height breakpoint.
- **Effort:** S

### 2. "NORMAL" button label gets clipped on portrait phones — `index.html:100,119`
- **What's wrong:** `@media(pointer:coarse),(max-width:600px){#bag-menu{width:min(60vw,320px)}#bag-menu .hb{min-height:44px;font-size:12px;padding:8px 3px}}` sizes the three THROW HEIGHT buttons (PUSH/NORMAL/KOBE) at 12px with only 3px side padding inside a panel capped at `min(60vw,320px)`. "NORMAL" is the longest label and doesn't fit.
- **Proof:** `shots/review2/03-aiming-practice-390x844.png` and the zoomed crop `zoom-bagmenu.png` show the button text clipped to **"NORMA"** — the final L runs off the edge of the button. Reproduces on every aim screen at this width (also visible in `04-`, `05-`, `06-round-end-390x844.png`).
- **Fix:** give the 3-across THROW HEIGHT row its own narrower font-size at this breakpoint (e.g. 9–10px, matching what `@media(max-height:450px)` already does for the landscape case), or drop the horizontal padding to 2px. This is the single most "looks cheap" screenshot in the whole set — it's the first thing a new phone player sees.
- **Effort:** S

### 3. Aim trajectory preview looks like scattered rendering glitches, not a landing guide — `index.html:670-676,1172-1181`
- **What's wrong:** While `M.phase==='aim'`, 48 small yellow boxes (`trajectory`) and 14 white planes (`aimDots`) continuously trace the *entire* predicted parabola, including the airborne apex — not just the ground track. They're scaled by camera distance (`d.scale.setScalar(camera.position.distanceTo(d.position)*.025)` at `index.html:1181`), so the far ones balloon up, and because they follow real arc height, several sit well above the fence line, floating in front of the trees and trash cans.
- **Proof:** `shots/review2/zoom-dashes.png` (a cropped, 2x-scale shot of a completely idle aim screen) shows ~8 pale, inconsistently-sized rectangles scattered across the fence, a tree trunk, and a storage bin — nothing about their placement reads as "this is where your bag will land." It's present in literally every aim-phase screenshot captured this session, at all three viewports (`03-aiming-practice-*.png`, `04-mid-flight-*.png` while the banner is still fading, `07-turn-change-*.png`).
- **Fix:** clip both arrays to the part of the arc that's near the ground (e.g. only draw when `position.y < ~0.25`), so the preview reads as the clean dotted line already visible directly in front of the thrower (seen correctly in the first few dashes of `03-aiming-practice-1280.png`) instead of trailing off into the scenery.
- **Effort:** M

### 4. Thrown bag has no trail, dust, or release cue — looks like it teleports — `index.html:1150` area (no `trail`/particle code found)
- **What's wrong:** There's no motion trail, squash/stretch, or release puff anywhere in the flight code — the bag is a single static mesh moving frame-to-frame with nothing connecting "where it left your hand" to "where it is now."
- **Proof:** `shots/review2/04-mid-flight-1280.png` — the bag is a small red rectangle alone near the fence, with no visual thread back to the thrower. Compare to the satisfying payoff already present on a make (`shots/review2/07-turn-change-1280.png`, confetti burst on CORNHOLE!) — the throw itself has none of that juice.
- **Fix:** 2–3 fading afterimage copies of the bag mesh during `M.phase==='flying'` (cheap: clone + reduce opacity + don't update), or a small dust-puff sprite at the release point. Reuses assets that already exist (`basic()`, the bag mesh, the confetti pattern).
- **Effort:** S/M

### 5. Zero audio in the entire game
- **What's wrong:** Confirmed via full-file search — no `AudioContext`, `<audio>`, or `new Audio()` anywhere. Every throw, landing, "CORNHOLE!", and round win is silent.
- **Why it matters:** this was flagged in the prior review and is still true. For an arcade toss game, a toss-whoosh, board-thud, and score chime are the highest-leverage "juice" per hour of work, and right now even the camera-shake-free, text-only feedback has no audio layer at all to back it up.
- **Fix:** 3–4 short WebAudio-generated blips (release, board-thud, score chime, win fanfare) — no asset pipeline required.
- **Effort:** M

### 6. "PITCHER'S BOX" adds a third control group beyond the stated left-menu scope — `index.html:121`
- **What's wrong:** You asked for PUSH/NORMAL/KOBE and SLICK/STICKY only in the left menu. Solo Practice currently renders a third group, `PITCHER'S BOX · LEFT/RIGHT` (`shots/review2/03-aiming-practice-1280.png`), stacked below the other two.
- **Why it matters:** it's the direct reason the portrait panel runs long enough to crowd the screen — on 390×844 the bag-menu panel occupies roughly the bottom third of the viewport (`shots/review2/06-round-end-390x844.png`), squeezing the actual play area. Cutting it (or moving it behind a toggle you don't need day-to-day) also softens item #2.
- **Fix:** confirm with yourself whether pitcher's-box-side selection earns its place; if not, delete the row — that's a net simplification, not new work.
- **Effort:** S (deletion)

### 7. Pale sky clouds drift directly behind the score HUD, killing contrast — `index.html` cloud/HUD z-order
- **What's wrong:** The 3D cloud sprites pass through the same screen region as the top HUD (names, score, round banner) with no darkening treatment behind the HUD text.
- **Proof:** `shots/review2/06-round-end-390x844.png` and `07-turn-change-390x844.png` — a large pale cloud sits right behind "ROUND 1 / FIRST TO 21" and the RED/BLUE score numbers, washing out the already-light text.
- **Fix:** either keep clouds below a fixed screen-space height (push their world Y / spawn range down so they stay below the fence line), or add a very subtle `linear-gradient(rgba(0,0,0,.25), transparent)` band under `#sb` so HUD text keeps contrast regardless of sky color behind it.
- **Effort:** S

### 8. Score number has no feedback when it updates — `index.html:40`
- **What's wrong:** `.ts{font-size:2.6em;margin-top:.3em;color:#ffe45e}` — no transition, no pulse. The result message (`ON THE BOARD`, `CORNHOLE!`) pops in with a nice scale-in (`#msg.on` at `index.html:58`), but the score digit underneath it just jump-cuts to the new value a beat later with zero connective animation.
- **Fix:** a 2-line CSS `@keyframes` scale-pulse on `.ts` triggered when the score updates ties the celebratory message to the number it's actually about.
- **Effort:** S

### 9. "On the Board" wording is inconsistent between the two places it appears — `index.html:739` vs `index.html:929`
- **What's wrong:** You were explicit that the wording should be "On the Board" everywhere. The per-throw result message already says it correctly (`say('ON THE BOARD', '1 POINT')`, confirmed in `shots/review2/07-turn-change-390x844.png`), but the Solo Practice stats line at `index.html:739` prints the shortened `${n.on} ON BOARD` (missing "THE"), visible in every practice screenshot, e.g. `shots/review2/03-aiming-practice-1280.png` ("0 IN · 0 ON BOARD").
- **Fix:** change `ON BOARD` → `ON THE BOARD` in the practice stats template string. One-word fix, but it's the exact phrase you called out, so worth closing the gap.
- **Effort:** S

### 10. A 1-point board hit gets no particle feedback, only a make does — `index.html:923,929`
- **What's wrong:** `burst(...)` (confetti) only fires for `b.state === 'in'` (the hole) and for knock-ins. A bag landing cleanly "ON THE BOARD" — a real, common scoring outcome — gets text only.
- **Proof:** compare `shots/review2/05-after-landing-1280.png` (board hit, no particles) against `shots/review2/07-turn-change-1280.png` (hole make, full confetti burst) — the feedback weight between a 1-point and 3-point score is very lopsided, which makes routine board hits feel like a non-event.
- **Fix:** a smaller/plainer particle puff (reuse `burst()` with a lower count, e.g. 12–15 pieces instead of 70) at the bag's landing spot for board hits, distinct from the full hole-burst.
- **Effort:** S

## Quick wins
- `.ol` text-shadow: px → em (item 1) — one line, fixes legibility everywhere at once.
- `#hud`/`#panel` font clamp floor: `7px` → `10px` (item 1).
- `ON BOARD` → `ON THE BOARD` in the practice stats template (item 9).
- Score-number pulse `@keyframes` on `.ts` (item 8).
- Confirm/cut the `PITCHER'S BOX` row (item 6) — also relieves phone crowding.
- Lower confetti count for board hits vs. hole makes (item 10) — reuses existing `burst()`.

## What's already fixed since the last review
Spot-checked against `CLAUDE_UI_REVIEW.md` so this doesn't re-litigate solved problems:
- The "RED RED"/"BLUE BLUE" duplicate name bug (old #1) is gone — `#t0`/`#t1` markup no longer hardcodes the team word; confirmed in every HUD screenshot this session.
- The Solo Practice `#sb` flex-centering bug (old #2) is gone — the stats line sits centered correctly now.
- The wind widget (old #10) now uses a pixel-bordered box with the same thick border + shadow treatment as other panels, and a small pixelated canvas arrow instead of a Unicode glyph — consistent with the rest of the UI.
- The opponent-card two-button hierarchy (old #23) already differentiates primary ("LET'S TOSS", wide yellow `.go`) from secondary ("CIRCUIT MAP", smaller `.hb`) clearly at all three viewports tested.
