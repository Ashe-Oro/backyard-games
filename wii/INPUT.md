# Wiimote input integration

`swing.c` and `input.c` are C11 with libm and no platform dependencies; `input_wpad.c` includes libogc's `<wiiuse/wpad.h>`. The game build links those three files. Never add `tests/fake/` to that build's include path.

## Game hooks

Keep `Input` and `InputWpad` alive for the whole application, preferably static rather than on the small Wii thread stack:

```c
static Input input;
static InputWpad wpad;

/* In startup, after your callback/user object exists: */
input_init(&input, on_input_event, game);
int status = input_wpad_init(&wpad, &input, on_trace_row, trace_writer);
/* Check status; on_trace_row may be NULL. */

/* Once per frame, using your monotonic clock: */
input_wpad_poll(&wpad, ticks_to_millisecs(gettime()));
```

The adapter owns both queues. It registers persistent 64-report buffers and drains each connected channel once through the `WPAD_ReadPending` callback, which supplies the held buttons paired with each accelerometer sample. `WPAD_ScanPads` already drains that queue and returns the coalesced frame-level button state, so calling it separately would discard per-report motion and can hide an edge from a second drain. The adapter checks I/O errors, tracks the read-and-clear dropped-event count, and re-registers on reconnect. See the [libogc 3.2.1 WPAD implementation](https://github.com/devkitPro/libogc/blob/v3.2.1/wiiuse/wpad.c).

Call `input_start_match` after polling connection state. It fixes slot 1 to Remote 1 and slot 2 to Remote 2 if connected then, otherwise Remote 1. A late Remote 2 connection does not silently change pass-and-play. Call `input_next_turn(slot)` with **zero-based** slot 0 or 1 when a new human bag becomes available, including consecutive practice throws. The detector is spent after a throw. The game controls `INPUT_PLAY`, `INPUT_MENU` and `INPUT_DISABLED` through `input_set_context`; disabled suppresses throw controls during flight/CPU turns but keeps HOME/help. The caller still polls both remotes in every context.

`InputSink` receives synchronous events with zero-based slot/channel. `INPUT_THROW` includes normalized power, release aim in radians, signed landing-curl spin, peak speed, release roll delta, clipping, release cause and release timestamp. Dispatch it once to the game's launch hook. Use the B-up timestamp for the release stance when a result arrives after the lobe crest. Height and surface events carry absolute values (0 SLIDER / 1 NORMAL / 2 KOBE, `ARC_KOBE`; 0 SLICK / 1 STICKY). The height picker is the only source of the arc: `SwingResult` has no pitch, so the remote's release pitch never reaches the throw. Step carries a clamped stance target; `input_stance(..., now)` supplies its 70 ms visual ease. RESET BAG re-centers the stance and discards windup; the game performs its own bag reset without consuming a throw. No event clears the practice board: CLEAR BOARD belongs to the game's pause menu.

The event callback should queue actions for the game to apply after `input_wpad_poll` returns. Avoid changing input context/player/calibration inside callbacks. Menus use discrete confirm/back, D-pad navigation and minus/plus events (minus/plus exist only in menus). The adapter requests `WPAD_FMT_BTNS_ACC`, disables the optical idle threshold and never reads camera data. There is no cursor or optical calibration.

D-pad LEFT/RIGHT moves stance. UP/DOWN picks SLIDER, NORMAL or KOBE. Minus/plus do nothing in gameplay. Swing speed controls power and sideways motion controls aim. During the forward stroke, the load-weighted mean of lateral X relative to radial load sets aim. The same sideways motion aims in the same world direction for either hand. Curl uses release roll with a correction for lateral motion. LEFT HAND mirrors curl. B-up locks both controls.

Render the exported `input_help_text` in the help screen. It lists, one per line: B HOLD, SWING BACK & FORWARD, LET GO · SWING SPEED = POWER · SWING LEFT/RIGHT TO AIM · TWIST TO CURL · D-PAD UP/DOWN KOBE-NORMAL-SLIDER · D-PAD LEFT/RIGHT MOVE · A SURFACE · 2 RESET · 1 HELP · HOME PAUSE. Hold **1+2** for two seconds to toggle swing diagnostics. The overlay starts hidden.

Setup and pause expose a HAND row per player. Call `input_focus_hand(input, slot)` when that row has focus (menu context or paused); LEFT selects LEFT, RIGHT selects RIGHT. Call `input_blur_hand` when focus leaves the row. `INPUT_PROFILE_CHANGED` carries the selected slot and hand value; queue saving of that slot's `profile` fields alongside SET POWER calibration. It cancels calibration and resets the detector. Load via `input_set_profile`. Renderer and storage remain game-branch integration; no files are written by input.

While B is held, A/D-pad/2 are ignored (minus/plus always are, in play). Help toggles without cancelling. HOME sets pause and discards the gesture. `INPUT_CONNECT_REMOTE` supplies the required remote number as `value` for `CONNECT REMOTE n`. Slot 2's disconnected state accepts A on Remote 1 for reassignment. The game calls `input_resume` only after reconnect/reassignment; it returns false until the assigned remote has supplied a valid B-up report. Resume never reuses the old gesture; the next B press grips at once.

## Swing phases and power

The adapter passes through raw libogc `gforce`. The remote reads +1 g upward at rest. **+X** points to the remote's left, **+Y** toward its expansion port, and **+Z** out of the button face. Upright tip-up reads **(0, −1, 0)**. Flat at the TV with buttons up reads **(0, 0, +1)**. Static tip elevation is `atan2(−Y, hypot(X, Z))`. The pitch display trusts it after 30 ms within 0.10 g of 1 g.

B-down grips immediately, including while moving. The opening turn waits for B-up if B was held or its state was unknown on entering play. A press held during bag flight or a turn change can grip when a later turn opens.

| Phase | Transition |
| --- | --- |
| READY → GRIP | B-down grips the bag and records the address vector. |
| GRIP → BACKSWING | Load changes by more than 0.25 g for 20 ms or swing-plane load opens a lobe at 1.20 g. |
| BACKSWING → TURN | The back-pass lobe lasts at least 90 ms, reaches 1.30 g, then drops by at least 0.15 g or 15% of its excess, whichever is larger. The detector learns a radial axis from the top of this lobe. |
| TURN → FORWARD | Swing-plane load rises at least 0.15 g from its minimum for two reports without a backward pull. The radial load at that minimum must be below 2.20 g to rule out a clipped backswing peak. Only this stroke can set throw power. |
| FORWARD → PAST BOTTOM | The forward **radial** lobe crests. Its weighted center estimates when the remote passed the bottom. |
| FORWARD → RESOLVE | B-up before the crest waits up to 160 ms for confirmation. The bottom estimate has 10 ms of release tolerance, reduced to zero when the grip rotates more than 120° and directions across the back-lobe top span more than 15°. |
| PAST BOTTOM / RESOLVE → DONE | Eligible B-up emits one throw with the original B-up timestamp. The release window closes after 600 ms or a follow-through turnaround, detected from a radial dip below 1.20 g and a reversal. |

The learned plane axis is `n = (ny, nz)`. At the bottom it is near +Z for a tip-forward grip and near +Y for an in-line grip. Its perpendicular gives signed tangential load. The address vector determines the forward sign when it has a usable tangential projection. The turnaround drive supplies the sign when that projection is ambiguous. For a free swing, gravity and pendulum acceleration cancel along the tangent. Positive tangential load shows forward drive. The release gate uses the radial crest to distinguish forward drive before the bottom from a valid release.

```
radial = Y*ny + Z*nz
e = max(0, radial - 1)
v = sqrt(0.60 * 9.80665 * mean_40ms(e))
power = clamp(0.55 * forward_peak_v / (slot.full_speed * type_scale), 0, 0.76)
```

Power uses the peak of the 40 ms average through B-up. A forward peak must reach **0.30 g** excess (about **1.33 m/s** with the 0.60 m shoulder radius). A sample near the rail triggers a fit to the unclipped lobe shoulders from the turnaround through B-up. The default rail estimate is 3.55 g. With insufficient shoulders, power uses the measured peak. `input_wpad_set_rails` accepts measured raw rails per remote. Its raw defaults span 0–1023 counts with a two-count margin.

The default speed reference is **2.45 m/s**. SLIDER, NORMAL and KOBE multiply it by **1.30**, **1.20** and **1/0.90**, respectively. The shared 0.76 cap is reachable at **4.40**, **4.06** and **3.76 m/s**, respectively. The NORMAL target accounts for the Wii Remote's approximately 3.6 g radial rail and the clipped-peak fit.

Every rejected release returns to READY and keeps the bag. The diagnostics show **NO SWING**, **LET GO ON BACKSWING**, **LET GO BEFORE BOTTOM**, **FORWARD TOO WEAK**, **LET GO TOO LATE** or **REPORT GAP**. Reports spaced more than 25 ms apart and up to 100 ms apart can be bridged with interpolated acceleration. Longer gaps break continuity. A gap over 250 ms discards an active stroke. Invalid accelerometer reports can still carry button edges. Duplicate or backward timestamps are advanced and history overflow drops the oldest report.

## SET POWER and saves

Profiles belong to player slots: `full_speed`, `spin_strength` (0..1), `left_handed`. Load validated saved fields with `input_set_profile`; serialize them field by field in the game's save layer. Save version 4 keeps the existing field layout and migrates earlier factory references to 2.45 m/s. Custom SET POWER values carry over. The input layer performs no storage I/O.

Enter a play context, resume if needed, and call `input_begin_calibration` for the active slot with `target_power` from the physics branch's NORMAL/SLICK board-center calculation, plus the profile limits. The current game uses a provisional floor of `0.55 * swing_speed(0.5) / (1.20 * target_power)` and a ceiling of `swing_speed(2.0)`. A target above the cap or invalid limits return false. Show `TOSS EASY ×3`. Three classifier-valid, unclipped throws produce `clamp(0.55 * median_peak / (1.20 * target_power), floor, clip_ceiling)`. Any peak more than 30% from the median, a clipped toss, or a disconnect discards the set and emits `INPUT_CAL_RETRY` (`TRY AGAIN`). A cancelled release keeps calibration progress. `INPUT_CAL_MORE` reports progress; `INPUT_CAL_DONE` updates the active slot profile (`POWER SET!`). These tosses never emit a game throw. Save on DONE. Slot changes cancel setup; `input_cancel_calibration` exits explicitly.

## Trace hooks and timing limits

`InputWpadTraceSink` receives each drained report (0.1.3 never rejects a batch): drain time, event index, **estimated** time, raw `WPADData` (axes, extension, err/data flags), normalized sample (including clipping), active player slot and detector state. Borrowed pointers expire when the callback returns. The slot is the current thrower's slot; inactive-channel rows are acquisition diagnostics, not gestures belonging to that slot. Copy rows into your lab recorder; avoid file I/O while draining.

`WPADData` carries no timestamps. The adapter spaces each batch across the elapsed interval ending at drain time, at most 25 ms apart (after a stall the batch ends at drain time and the detector sees the hole). The first batch uses nominal 10 ms spacing. The adapter counts dropped reports and empty polls. It feeds reports in order and retains the next available B edge. The detector handles gaps using the estimated timestamps. Check the estimated times and counters when comparing a hardware trace to a host replay.

The live **1+2** overlay shows raw X/Y/Z and signed Y/Z, pitch with STATIC/MOVING, phase and B, reject reason, signed tangent and radial load, lateral load, estimated arm angle and speed, peak and fitted speed, bottom timing, last throw, reject counts, report rate and learned axis/sign source. The arm estimate is for diagnostics.

With the overlay up and B released, **A** opens the calibration page. Close the overlay with **1+2** to restore **A** for SURFACE. Use **left/right** to select a pose, **A** to capture a 300 ms average and **2** to return to live diagnostics. The capture starts after the A-press report and requires readings within 0.10 g of 1 g. Capture tip up, flat at the TV, roll right 90° with the right side down, tip down, and tip-up leaned 30° toward the TV. The page shows the vector, magnitude, signed dominant axis and whether the reading matches within 0.15 g. In the 30° pose, negative Z means the face points toward the TV and positive Z means it points toward the player. A flat turn to the right leaves gravity on +Z. A right roll moves it onto +X.

While the overlay is on, the Wii frontend buffers active-remote CSV reports in RAM and flushes only in READY or DONE. Each overlay session starts with a header. The active file at `sd:/apps/backyardcornhole/data/swing-trace.csv` rotates at 1 MiB and keeps one previous file, `swing-trace.1.csv`. Without writable SD it sends `BYC-SWING` rows to OSReport. The rows include report and drain times, raw counts and signed g, report validity, clipping, phase, radial and tangential load, excess, B and reject reason. Closing the overlay queues the remaining rows for the next idle flush. A full RAM buffer skips new reports and logs their count.

## Host verification

Run from the repo root:

```sh
make -C wii host-test
# Or, without make/devkitPPC:
sh wii/tests/run-input-tests.sh
```

The host suites exercise physical shoulder-pivot traces with noisy, quantized samples, native 100 Hz input and 60 Hz batched polling. They cover static seeds, both grips and rolls, release timing and rejects, power isolation, clipping, held-B transitions and report gaps. The game tests also check picker preview, launch power, first-touch distances and save migration. Physical Wii Remote and DolphinBar testing remains pending.

API references checked against upstream [wpad.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wpad.h), [wpad.c](https://github.com/devkitPro/libogc/blob/master/wiiuse/wpad.c) and [wiiuse.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wiiuse.h). The toolchain branch must compile against its actual pinned libogc release.
