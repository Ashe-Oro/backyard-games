# Cabinet controls audit — November 6–7

The tested cabinet exposes its trackball as HID 1241:1122. Cornhole reads relative evdev counts without grabbing the device or using accelerated desktop cursor coordinates. Golden Tee '98 uses the same physical trackball through RetroArch mouse index 1 and the MAME 2003-Plus action pair. Raw cabinet buttons 3/4 are the existing B/A pair; Cornhole retains that mapping on either player panel. Wiring of the two blue trackball buttons still needs physical confirmation.

Golden Tee's [official controls guide](https://livewire.itsgames.com/gt/how-to-play) describes a deliberate backswing and forward roll; its shot meter reports stroke direction and speed. Cornhole keeps its own mechanics: back sets release direction, forward speed sets power, and buttons choose Slider/Normal/Kobe and Slick/Sticky. Bag rotation affects landing only.

## Findings and local changes

| Finding | Local candidate | Verification |
|---|---|---|
| Tiny downward noise accumulated while moving sideways | Require a predominantly downward pull; expire the intent window after 160 ms | Alternating slow lateral corrections with sensor noise remain placement |
| Placement jumped 1.5 mm per count | 0.8 mm/count target with 70 ms easing | Reset and repeated corrections tested; physical feel pending |
| Automatic release after only 80 counts | Fresh-install candidate uses 220 counts and 4,000 counts/sec for full power | Deterministic short/normal/hard stroke tests; hands-on comparison pending |
| No visible power feedback | Gauge beside bag uses the same converted physical power as launch; last shot persists five seconds | Forward stroke and released shot compared with launch power |
| Reset meaning and button unclear | Reset Bag returns only the ready bag to center and cancels the stroke | No score, thrown-bag or round-count changes |
| Slick slid off too readily | Landing retention .42 → .32; friction .30 → .43 | Normal, Slider and Kobe landing sweeps; Sticky unchanged |

Existing v4 calibration is preserved. New defaults do not overwrite a player's tuned speed. Use Cabinet Setup → Reset Settings to try the fresh candidate, or calibrate a comfortable sustained stroke. The physical effort of a Golden Tee drive cannot be inferred from software counts alone; the 220-count profile remains a local playtest candidate.

## Buttons

- Existing B / raw 3: select in menus; cycle throw height in play.
- Existing A / raw 4: back in menus; flip surface in play.
- Raw 2 or 5: Reset Bag in play. Remappable; physical label/layout confirmation pending.
- Start: select in menus; pause in play. Select: pause.
- Keyboard fallback: H height, F surface, R reset, Enter select, Escape pause.

No golf hooks/slices, club selection, automatic follow-through steering, or global cabinet button remaps were added. These changes are isolated from the web game.
