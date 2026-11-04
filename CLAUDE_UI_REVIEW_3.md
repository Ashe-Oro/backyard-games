# Backyard Cornhole — Phone Portrait UI Review

## Overall verdict

This is a likeable, cohesive little scene — the pixel-art backyard, the string lights, the outlined retro font, and the yellow "selected" highlight all read as one consistent visual language, and the control panel is clean and easy to understand at a glance. The two things holding portrait back right now are both things you've already flagged as being fixed: the bag floating off to the side instead of where you're aiming, and the aim line jumping on touch-down — once those land, the core "aim and flick" moment will feel much more trustworthy. Beyond that, the main opportunity is simply tightening up how much of the tall phone screen is empty grass versus doing useful work.

## Layout and composition on a phone

Portrait is a very tall, narrow canvas (roughly 1:2.25), and right now the game only really uses the top third and the bottom third of it. The board sits small and distant near the top of the screen, the control panel and power meter sit in the bottom third, and in between is a long stretch of flat, empty grass — in the live screenshot that gap is well over 40% of total screen height doing nothing but separating the two "real" parts of the UI. On a phone, where every pixel of vertical space is precious, that's the single biggest layout issue: the game feels like it's being viewed through a tall letterbox rather than a composition that was framed for portrait.

The panel itself is reasonably sized for what it holds — it's left-anchored, doesn't block the board, and the bag options stay out of the way until you actually need them, which is exactly the "simple, menu-based" approach you want. But because it only occupies the left ~65% of the width, the bottom-right corner ends up with just the lone power meter bar, isolated and disconnected from everything else, floating in its own patch of grass. The bottom edge of the screen reads as two separate, unrelated UI elements rather than one control strip.

## Visual hierarchy and HUD readability

The scoreboard (name, score, pips) in the top corners is clean, compact, and doesn't fight for attention — good use of the dead corners of the screen. The center stat line ("0 IN · 0 ON BOARD / 0% HIT · 0 THROWS") is dense but the outline treatment keeps it legible even over the sky gradient.

The weaker spot is "RED'S THROW" sitting mid-screen directly over the busiest part of the background — tree canopies, string lights, and bright highlight pixels. The text-shadow outline helps, but it's still competing with high-contrast art behind it rather than sitting on a calmer part of the scene (sky or grass). It also doesn't have a fixed "home" the way the scoreboard does — it just floats over whatever happens to be behind it, which makes it feel a bit like an afterthought rather than a designed element.

## Control panel design

The panel itself is a strong piece of UI: consistent button sizing, a single accent color (yellow) for "this option is selected," and good grouping into labeled rows (THROW HEIGHT / BAG SIDE DOWN / PITCHER'S BOX). That's exactly the kind of simple, scannable menu you asked for, and it doesn't need new buttons or structure — it needs tightening, not expanding.

Two small inconsistencies stand out: the RESET BAGS / MENU row at the bottom sits at the same visual weight (same button style, same size) as the actual gameplay choices above it, so a first-time player's eye doesn't immediately know which three rows matter during aiming and which one row is "utility." And because the panel height is tall relative to the phone's width, it quietly eats a big chunk of the court view right when you're trying to aim — exactly the moment you'd want the most visibility.

## Power meter

The vertical power bar is functionally fine but visually orphaned. It uses a different "material" than the panel (thin outlined bar vs. solid filled box) and sits in an empty corner with no relationship to the control panel beside it, even though both are "the controls you look at while aiming." On a wide landscape screen that separation probably reads fine; in portrait, where the eye is already jumping between a distant board, a mid-screen throw label, and a bottom-left panel, one more disconnected element in the opposite corner adds to the scattered feeling of the bottom third.

## Color and pixel-art consistency

This is the strongest part of the design. The warm backyard palette (greens, the peach sky gradient, the mauve/purple panel) is consistent throughout, the yellow highlight color is used exclusively and clearly for "selected/active," and the pixel font with dark outline keeps every piece of text readable over a busy 3D scene. Nothing here clashes — the HUD, the panel, and the 3D art all clearly belong to the same game.

## What would make it feel polished

Once the bag position and aim-preview stability are fixed, the remaining polish is almost entirely about using the portrait screen more intentionally: close the dead grass gap so the board feels closer and more "in reach," give the bottom third a sense of being one control strip instead of two separate floating pieces, and give the throw-announcement text a calmer, more consistent place to live. None of that requires new buttons, new hints, or any change to how aiming/flicking works — it's framing and spacing, not new functionality.

## Prioritized recommendations

1. **Tighten the portrait camera framing so the board sits closer and the grass gap shrinks.** Why: right now close to half the screen is empty grass between the board and the panel — on a phone that space is too valuable to leave idle, and a closer board makes aiming feel more immediate. Effort: **M** (camera/FOV tuning, needs testing across a few phone aspect ratios).
2. **Visually tie the power meter to the control panel instead of leaving it isolated in the opposite corner.** Why: both are "things you check while aiming," but right now they read as two unrelated UI fragments at the bottom of the screen. Effort: **S** (styling/positioning only, no new elements).
3. **Give "RED'S THROW" a consistent, calmer backdrop** (e.g. sit it over sky/grass rather than tree canopy, or give it a subtle scrim like the panel has). Why: it currently competes with the busiest part of the background art for legibility. Effort: **S**.
4. **Slightly reduce the panel's vertical footprint** (tighter row spacing, not fewer options). Why: it currently claims a large slice of the one moment — aiming — where court visibility matters most; the options themselves don't need to change, just the breathing room between them. Effort: **S**.
5. **De-emphasize RESET BAGS / MENU relative to the three gameplay rows above them** (smaller or quieter style). Why: right now all four rows carry equal visual weight, which blurs the line between "choices for this throw" and "utility actions." Effort: **S**.
6. **Add light variation to the empty grass lane** (a subtle shadow gradient or sparse detail), especially if the camera gap can't be fully closed. Why: a long flat green stretch reads as "nothing happening" more than a backyard should. Effort: **S/M**.
7. **Double-check tap-target comfort on the smaller panel rows** (PITCHER'S BOX, RESET BAGS, MENU) against the larger THROW HEIGHT/BAG SIDE buttons above them. Why: in the screenshots these sit visually tighter, and thumb accuracy matters most on the row players touch mid-game. Effort: **S**.
8. **Consider a very subtle horizon/vignette treatment pulling the eye toward the board** rather than letting the sky and grass carry equal visual weight. Why: reinforces where attention should go without adding any new UI. Effort: **M**.
