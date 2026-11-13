# Cabinet controls audit — November 6–7

The tested cabinet exposes its trackball as HID 1241:1122. Cornhole reads relative evdev counts without grabbing the device or using accelerated desktop cursor coordinates. Golden Tee '98 uses the same physical trackball through RetroArch mouse index 1 and the MAME 2003-Plus action pair. Pad 0 uses the eight-button layout below; other pads retain their previous defaults. Wiring of the two blue trackball buttons still needs physical confirmation.

Golden Tee's [official controls guide](https://livewire.itsgames.com/gt/how-to-play) describes a deliberate backswing and forward roll; its shot meter reports stroke direction and speed. Cornhole keeps its own mechanics: optional back sets spin, forward angle sets aim, accumulated forward counts after sensitivity set power, and buttons choose Slider/Normal/Kobe and Slick/Sticky. Bag rotation affects landing only.

## Findings and local changes

| Finding | Local candidate | Verification |
|---|---|---|
| Tiny downward noise accumulated while moving sideways | Require a predominantly downward pull; expire the intent window after 160 ms | Alternating slow lateral corrections with sensor noise remain placement |
| Placement jumped 1.5 mm per count | 0.8 mm/count target with 70 ms easing | Reset and repeated corrections tested; physical feel pending |
| Automatic release after only 80 counts | Release defaults to 400 counts; power is counts / 400, bounded to 0–1 | 200/400-count tests at slow and fast report intervals; hands-on comparison pending |
| No visible power feedback | Gauge beside bag uses the same converted physical power as launch; last shot persists five seconds | Forward stroke and released shot compared with launch power |
| Reset meaning and button unclear | Reset Bag returns only the ready bag to center and cancels the stroke | No score, thrown-bag or round-count changes |
| Slick slid off too readily | Landing retention .42 → .32; friction .30 → .43 | Normal, Slider and Kobe landing sweeps; Sticky unchanged |

Saved profiles retain bindings and sensitivity. Retired speed-based powerSpeed
values are ignored; release gates below 400 migrate to 400 (allowed range
400–1200). Cabinet Setup displays fixed Full-power counts: 400 and adjustable
Release counts. Calibration maps a completed reference stroke to 400 counts by
adjusting sensitivity, including any final-report overshoot. A slow 400-count
stroke reaches full power; a fast 200-count stroke previews half power.
Multiple reports at distinct timestamps are still required to launch, and each
gesture can launch only once. Physical cabinet feel remains to be playtested.

## Buttons

| P1 button | Index | In play | In menus |
|---|---:|---|---|
| Bottom 1 | 0 | Cycle Slider / Normal / Kobe | Confirm |
| Bottom 2 | 1 | Cycle Slick / Sticky | Back / menu |
| Bottom 3 | 2 | Position left, 0.10 step | No action |
| Bottom 4 | 3 | Position right, 0.10 step | Confirm |
| Top 1 | 4 | Unbound | Back / menu |
| Top 2 | 5 | Reset ready bag and stroke to center | No action |
| Top 3 | 6 | Unbound | No action |
| Top 4 | 7 | Unbound | No action |

Start/Select indices 8/9 retain pause/confirm behavior. Explicit Cabinet Setup
bindings override defaults. Other pads retain their prior maps. One action per
press; holding a button does not repeat. Position clamps to [-0.45, 0.45] with
70 ms easing. Reset leaves thrown bags and scores alone.

Optional backswing: A (back-left) sets clockwise spin and slides right;
C (back-right) sets counterclockwise spin and slides left. B (straight back)
or no backswing gives zero spin. Lean scales spin continuously to maximum at
a diagonal. Forward 1 (left) and 3 (right) still aim left and right; accumulated forward
counts set power. Spin rotates the bag and curls its landing slide, not its
flight. Sideways rolls can still place the bag. Backspin intent expires after
2.5 seconds idle or on reset/turn change. Back-to-forward can pause briefly.
