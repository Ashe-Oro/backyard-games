# Wiimote input integration

This branch supplies input only. It does not create a main loop, render, simulate a bag, save profiles or package a DOL. `swing.c` and `input.c` are C11 with libm and no platform dependencies; `input_wpad.c` includes libogc's `<wiiuse/wpad.h>`. The game build links those three files. Never add `tests/fake/` to that build's include path.

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

Do not also call `WPAD_ScanPads`, `WPAD_ReadPending` or `WPAD_ReadEvent`: the adapter owns both queues. It registers persistent 64-report buffers, drains both channels, and checks format, registration, probe, read and dropped-event return values. It re-registers on reconnect. `WPAD_DroppedEvents` is a read-and-clear count, not a cumulative total. Counters and the latest error are exposed in `InputWpad`.

Call `input_start_match` after polling connection state. It fixes slot 1 to Remote 1 and slot 2 to Remote 2 if connected then, otherwise Remote 1. A late Remote 2 connection does not silently change pass-and-play. Call `input_next_turn(slot)` with **zero-based** slot 0 or 1 when a new human bag becomes available, including consecutive practice throws. The detector is spent after a throw. The game controls `INPUT_PLAY`, `INPUT_MENU` and `INPUT_DISABLED` through `input_set_context`; disabled suppresses throw controls during flight/CPU turns but keeps HOME/help. The caller still polls both remotes in every context.

`InputSink` receives synchronous events with zero-based slot/channel. `INPUT_THROW` includes normalized power, release aim in radians, signed landing-curl spin, peak speed, release roll delta, clipping, release cause and release timestamp. Dispatch it once to the game's launch hook. Height and surface events carry absolute values (0 SLIDER / 1 NORMAL / 2 KOBE; 0 SLICK / 1 STICKY). Step carries a clamped stance target; `input_stance(..., now)` supplies its 70 ms visual ease. RESET BAG re-centers stance/aim and discards windup; the game performs its own bag reset without consuming a throw. No event clears the practice board: CLEAR BOARD belongs to the game's pause menu.

The event callback should queue actions for the game to apply after `input_wpad_poll` returns. Avoid changing input context/player/calibration inside callbacks. Menus use discrete confirm/back, D-pad navigation and minus/plus events. The adapter requests `WPAD_FMT_BTNS_ACC`, disables the optical idle threshold and never reads camera data. There is no cursor or optical calibration.

D-pad LEFT/RIGHT moves stance, never aims at the hole; UP/DOWN retains height selection. Minus/plus adjusts optional fine trim by 0.01 rad (clamp ±0.12), captured at B-down. During a qualifying forward lobe, excess-force/time-weighted grip-relative lateral acceleration sets aim. A new lobe clears the lateral accumulator. A straight swing with zero trim has zero aim from any stance. Spin combines lateral swing motion and wrist twist relative to the stable grip, sampled at release. Both readings mirror for LEFT HAND. See SPEC §4.6–4.7 for equations, tunables and accelerometer limitations. No direction/spin signal changes power. B-up freezes both controls, including while follow-through confirmation is pending; auto-release uses the current report.

Render the exported `input_help_text` in the help screen. It includes SWING LEFT/RIGHT TO AIM, TWIST TO CURL and D-PAD HEIGHT/STEP.

Setup and pause expose a HAND row per player. Call `input_focus_hand(input, slot)` when that row has focus (menu context or paused); LEFT selects LEFT, RIGHT selects RIGHT. Call `input_blur_hand` when focus leaves the row. `INPUT_PROFILE_CHANGED` carries the selected slot and hand value; queue saving of that slot's `profile` fields alongside SET POWER calibration. It cancels calibration and resets settling. Load via `input_set_profile`. Renderer and storage remain game-branch integration; no files are written by input.

While B is held or release confirmation is pending, A/D-pad/2/minus/plus are ignored. Help toggles without cancelling. HOME sets pause and discards the gesture. `INPUT_CONNECT_REMOTE` supplies the required remote number as `value` for `CONNECT REMOTE n`. Slot 2's disconnected state accepts A on Remote 1 for reassignment. The game calls `input_resume` only after reconnect/reassignment; it returns false until the assigned remote has supplied a valid B-up report. Resume restarts 450 ms settling; it never reuses the old gesture.

## Power and candidate classifier

For each calibrated accelerometer report:

```
e = max(0, length(gforce) - 1)
mean_e = time-weighted trapezoidal mean over the last 80 ms
v = sqrt(0.6 * 9.80665 * mean_e)
power = clamp(retained_forward_peak_v / slot.full_speed, 0, 1)
```

The window is clipped exactly at its left boundary, with interpolation only between contiguous valid reports. A window needs 16 ms elapsed span. Power never uses stroke distance or accumulated swing duration. Duration is used only for validity, report-loss checks, settling and timeout. The current lobe must exceed 0.35 g for 40 ms; a later lobe replaces the previous lobe's peak. A back-down apex pause at least 15° behind grip discards the backswing peak and lateral accumulator. Apex/finish labeling requires a 40 ms two-sided calm dwell (`abs(|g|−1) < 0.10 g`, pairwise attitude variation < 5°, mean excess < 0.15 g). The next qualifying lobe after the back pause is forward; B-up throws immediately without waiting for a static finish. A sustained calm forward-up finish at least 15° from grip can confirm a lone lobe. Acceleration sign is never treated as velocity direction.

The calm grip window spans 150 ms, with less than 0.10 g absolute deviation from 1 g and less than 5° pairwise attitude variation. Fine trim and neutral roll are captured at B down. Direction and spin follow the swing until release. Right twist gives negative spin/right landing curl; left hand mirrors aim and spin. A lone forward lobe is supported, so no backswing is required. Manual release of a classified forward lobe retains the peak and throws at B-up; an unclassified lone lobe can wait up to 250 ms for sustained calm follow-through confirmation; it never adds motion after B up to the peak. Release must be within 250 ms of the qualifying lobe. Hold-B auto-release requires a full window, at most 60% of peak speed and follow-through confirmation. Timeout is 2.5 s. Invalid samples, non-monotonic timestamps, gaps over 25 ms, queue loss and disconnect discard the gesture and require a fresh B press after settling.

Saturated samples cannot establish a lobe or increase its peak. Clipping can cap a previously valid, subsequently confirmed forward toss at 1.0. Back-apex/rejected clipping is discarded. The adapter's provisional raw rails are 0..1023 with a two-count margin, including gravity on every axis. `input_wpad_set_rails` accepts measured per-channel raw rails; it does not infer rails from individual swings.

This is a **candidate** C1/C2/C3 combination for P1 evaluation. Hardware must establish the pitch/roll signs, supported full-arm gesture, report interval, raw rails, and independently filmed speed ordering. Scalar `|gforce|-1` is a pendulum heuristic, not true hand velocity; radius changes remain a known source of error. The pre-P1 default is the spec's provisional 1.2 g full-power excess, not a claim that comfortable tosses already reach the board. MotionPlus is not required or enabled here.

## SET POWER and saves

Profiles belong to player slots: `full_speed`, `spin_strength` (0..1), `left_handed`. Load validated saved fields with `input_set_profile`; serialize them field by field in the game's save layer. Do not write C structs to disk. The input layer performs no storage I/O.

Enter a play context, resume if needed, and call `input_begin_calibration` for the active slot with `target_power` from the physics branch's NORMAL/SLICK board-center calculation, plus the P1 measured `full_speed_floor` and speed `clip_ceiling`. Invalid/unavailable limits return false; no physics target or measured floor is invented here. Show `TOSS EASY ×3`. Three classifier-valid, unclipped throws produce `clamp(median_peak / target_power, floor, clip_ceiling)`. Any peak more than 30% from the median, a rejected/clipped toss, or report loss discards the set and emits `INPUT_CAL_RETRY` (`TRY AGAIN`). `INPUT_CAL_MORE` reports progress; `INPUT_CAL_DONE` updates the active slot profile (`POWER SET!`). These tosses never emit a game throw. Save on DONE. Slot changes cancel setup; `input_cancel_calibration` exits explicitly.

## Trace hooks and timing limits

`InputWpadTraceSink` receives each drained report, including rejected batches: drain time, event index, **estimated** time, raw `WPADData` (axes, extension, err/data flags), normalized sample (including clipping), active player slot and detector state. Borrowed pointers expire when the callback returns. The slot is the current thrower's slot; inactive-channel rows are acquisition diagnostics, not gestures belonging to that slot. Copy rows into your lab recorder; avoid file I/O while draining.

There are no report timestamps in `WPADData`. The current estimate anchors the batch at drain time and spaces reports by nominal 10 ms. Overlap, full buffers, stalls and detected gaps cause cancellation rather than interpolation through unknown motion. P0 must measure rates on each target before this method is accepted. The adapter exposes diagnostics to replace the timing strategy if measurements require it.

The game's trace writer must add build/target/independent speed labels and a header containing `SWING_VERSION`, every constant from `swing_config.h`, slot profile, calibration limits, raw rails and nominal report interval. Keep samples through release+133 ms, then call `swing_twist_relative` to sum the four largest gforce.x values within ±133 ms of `toss.release_ms`. This is trace only and cannot change curl. Emitting the full CSV and writing it under `sd:/apps/backyardcornhole/data/swing-traces/` belongs to the lab/save integration; input supplies the report and release hooks without opening files.

## Host verification

Run from the repo root:

```sh
make -C wii host-test
# Or, without make/devkitPPC:
sh wii/tests/run-input-tests.sh
```

The synthetic detector/button/calibration suite has 25 cases; the fake-WPAD suite has 7. These tests do not establish hardware feel, Bluetooth behavior, a real libogc link, or the P1 exit gate. Toolchain, rendering, physics, game flow, menus, persistence, rumble and packaging are separate integration work.

API references checked against upstream [wpad.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wpad.h), [wpad.c](https://github.com/devkitPro/libogc/blob/master/wiiuse/wpad.c) and [wiiuse.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wiiuse.h). The toolchain branch must compile against its actual pinned libogc release.
