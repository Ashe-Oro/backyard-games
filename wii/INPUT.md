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

`InputSink` receives synchronous events with zero-based slot/channel. `INPUT_THROW` includes normalized power, release aim in radians, signed landing-curl spin, peak speed, release roll delta, clipping, release cause and release timestamp. Dispatch it once to the game's launch hook. Height and surface events carry absolute values (0 SLIDER / 1 NORMAL / 2 LOB, `ARC_KOBE`; 0 SLICK / 1 STICKY). The height picker is the only source of the arc: `SwingResult` has no pitch, so the remote's release pitch never reaches the throw. Step carries a clamped stance target; `input_stance(..., now)` supplies its 70 ms visual ease. RESET BAG re-centers the stance and discards windup; the game performs its own bag reset without consuming a throw. No event clears the practice board: CLEAR BOARD belongs to the game's pause menu.

The event callback should queue actions for the game to apply after `input_wpad_poll` returns. Avoid changing input context/player/calibration inside callbacks. Menus use discrete confirm/back, D-pad navigation and minus/plus events (minus/plus exist only in menus). The adapter requests `WPAD_FMT_BTNS_ACC`, disables the optical idle threshold and never reads camera data. There is no cursor or optical calibration.

0.1.4 mapping (Wii Bowling): D-pad LEFT/RIGHT moves stance, never aims at the hole; UP/DOWN is the LOB/NORMAL/SLIDER picker. Minus/plus do nothing in gameplay (the 0.1.3 fine trim is gone) and do not reset the swing. Power is only the forward swing speed; aim is only the sideways swing. During a qualifying forward lobe, excess-force/time-weighted grip-relative lateral acceleration sets aim. A new lobe clears the lateral accumulator. A straight swing has zero aim from any stance. Spin combines lateral swing motion and wrist twist relative to the stable grip, sampled at release. Both readings mirror for LEFT HAND. See SPEC §4.6–4.7 for equations, tunables and accelerometer limitations. No direction/spin signal changes power. B-up freezes both controls, including while follow-through confirmation is pending.

Render the exported `input_help_text` in the help screen. It lists, one per line: B HOLD, SWING BACK & FORWARD, LET GO · SWING SPEED = POWER · SWING LEFT/RIGHT TO AIM · TWIST TO CURL · D-PAD UP/DOWN LOB-NORMAL-SLIDER · D-PAD LEFT/RIGHT MOVE · A SURFACE · 2 RESET · 1 HELP · HOME PAUSE. Hold **1+2** for two seconds to toggle live accelerometer readings, B state, swing phase, report timing and the last throw's peak speed, power and aim. The overlay starts hidden.

Setup and pause expose a HAND row per player. Call `input_focus_hand(input, slot)` when that row has focus (menu context or paused); LEFT selects LEFT, RIGHT selects RIGHT. Call `input_blur_hand` when focus leaves the row. `INPUT_PROFILE_CHANGED` carries the selected slot and hand value; queue saving of that slot's `profile` fields alongside SET POWER calibration. It cancels calibration and resets the detector. Load via `input_set_profile`. Renderer and storage remain game-branch integration; no files are written by input.

While B is held or release confirmation is pending, A/D-pad/2 are ignored (minus/plus always are, in play). Help toggles without cancelling. HOME sets pause and discards the gesture. `INPUT_CONNECT_REMOTE` supplies the required remote number as `value` for `CONNECT REMOTE n`. Slot 2's disconnected state accepts A on Remote 1 for reassignment. The game calls `input_resume` only after reconnect/reassignment; it returns false until the assigned remote has supplied a valid B-up report. Resume never reuses the old gesture; the next B press grips at once.

## Power and candidate classifier

For each calibrated accelerometer report:

```
e = max(0, length(gforce) - 1)
mean_e = time-weighted trapezoidal mean over the last 80 ms
v = sqrt(0.6 * 9.80665 * mean_e)
power = clamp(retained_forward_peak_v / slot.full_speed, 0, 1)
```

The window is clipped exactly at its left boundary, with interpolation only between contiguous valid reports. A window needs 16 ms elapsed span. Power never uses stroke distance or accumulated swing duration. Duration is used only for validity and gap handling. A stroke (lobe) starts when the window exceeds 0.35 g and is valid after 40 ms; a later stroke replaces the previous stroke's peak. A continuous Bowling swing never drops below 0.35 g at the back apex, so a stroke also ends where the window falls below half its peak, and the next starts when it rises 25% and 0.15 g above that dip. The second and later strokes after the grip are forward. Extra cues: a back-down apex pause at least 15° behind grip discards the backswing peak and lateral accumulator. Apex/finish labeling requires a 40 ms two-sided calm dwell (`abs(|g|−1) < 0.10 g`, pairwise attitude variation < 5°, mean excess < 0.15 g). The next qualifying lobe after a back pause is forward. A sustained forward reversal from a behind-grip attitude can also establish direction when the player swings through the apex without pausing. A sustained calm forward-up finish at least 15° from grip can confirm a lone lobe. Acceleration sign is never treated as velocity direction.

0.1.3 grip: B grips on the very report it reads down, in any pose and while moving. There is no settle period and no stillness test (HOLD STILL is gone). The grip reference is the mean gravity of the last 100 ms of reports. Aim starts straight at the grip (0.1.4: no fine trim); neutral roll comes from the grip when the remote is not pointing straight up, otherwise from the first 60 ms of loaded swing (the Bowling remote-up start). A turn or play context that opens with B already held grips at once; after pause, menus, HOME or a button change, B must be seen up first.

The Wii game launches on B-up, and B-up while gripped always throws. A classified forward stroke uses its retained peak immediately. A lone first stroke can still wait up to 250 ms for a calm follow-through, with the measured release state frozen at B-up. Anything else (a tap, a backswing release, saturated-only motion, a gentle swing under the stroke threshold, a stroke that ended over 250 ms before B-up) is a weak throw: `0.65 * power(measured speed)`, clamped to 0.12..0.35, with the measured aim and curl. A B tap is the 0.12 lob. Holding B has no timeout.

Report timing: gaps over 25 ms and up to 100 ms are bridged with interpolated 10 ms samples; gaps up to 250 ms are accepted; longer stalls restart the motion measurement but keep the grip, so a later B-up still throws. Dropped-report counts are ignored, a report with invalid accel still carries B, backward or duplicate timestamps are clamped forward, and history overflow drops the oldest sample. Only disconnect and HOME/pause/menu changes discard the gesture.

Saturated samples cannot establish a lobe or increase its peak. Clipping can cap a previously valid, subsequently confirmed forward toss at 1.0. Back-apex/rejected clipping is discarded. The adapter's provisional raw rails are 0..1023 with a two-count margin, including gravity on every axis. `input_wpad_set_rails` accepts measured per-channel raw rails; it does not infer rails from individual swings.

This is a **candidate** C1/C2/C3 combination for P1 evaluation. Hardware must establish the pitch/roll signs, supported full-arm gesture, report interval, raw rails, and independently filmed speed ordering. Scalar `|gforce|-1` is a pendulum heuristic, not true hand velocity; radius changes remain a known source of error. The pre-P1 default is the spec's provisional 1.2 g full-power excess, not a claim that comfortable tosses already reach the board. MotionPlus is not required or enabled here.

## SET POWER and saves

Profiles belong to player slots: `full_speed`, `spin_strength` (0..1), `left_handed`. Load validated saved fields with `input_set_profile`; serialize them field by field in the game's save layer. Do not write C structs to disk. The input layer performs no storage I/O.

Enter a play context, resume if needed, and call `input_begin_calibration` for the active slot with `target_power` from the physics branch's NORMAL/SLICK board-center calculation, plus the P1 measured `full_speed_floor` and speed `clip_ceiling`. Invalid/unavailable limits return false; no physics target or measured floor is invented here. Show `TOSS EASY ×3`. Three classifier-valid, unclipped throws produce `clamp(median_peak / target_power, floor, clip_ceiling)`. Any peak more than 30% from the median, a weak (fallback) or clipped toss, or a disconnect discards the set and emits `INPUT_CAL_RETRY` (`TRY AGAIN`). `INPUT_CAL_MORE` reports progress; `INPUT_CAL_DONE` updates the active slot profile (`POWER SET!`). These tosses never emit a game throw. Save on DONE. Slot changes cancel setup; `input_cancel_calibration` exits explicitly.

## Trace hooks and timing limits

`InputWpadTraceSink` receives each drained report (0.1.3 never rejects a batch): drain time, event index, **estimated** time, raw `WPADData` (axes, extension, err/data flags), normalized sample (including clipping), active player slot and detector state. Borrowed pointers expire when the callback returns. The slot is the current thrower's slot; inactive-channel rows are acquisition diagnostics, not gestures belonging to that slot. Copy rows into your lab recorder; avoid file I/O while draining.

`WPADData` carries no timestamps. The adapter spaces each batch across the elapsed interval ending at drain time, at most 25 ms apart (after a stall the batch ends at drain time and the detector sees the hole). The first batch uses nominal 10 ms spacing. Full buffers, dropped reports, frame hitches and empty polls are counted (`dropped`, `empty_polls`) but never cancel the gesture. Check the estimated times and counters when comparing a hardware trace to a host replay.

The 1+2 overlay (hold two seconds) shows live accel, B, the swing phase (READY, GRIP, BACKSWING, FORWARD, RELEASE, SPENT), reports per second and the largest report gap over about the last two seconds (from the adapter's estimated times), the live and peak swing speed, and the last throw's peak speed and power (LOB marks a weak throw).

The game's trace writer must add build/target/independent speed labels and a header containing `SWING_VERSION`, every constant from `swing_config.h`, slot profile, calibration limits, raw rails and nominal report interval. Keep samples through release+133 ms, then call `swing_twist_relative` to sum the four largest gforce.x values within ±133 ms of `toss.release_ms`. This is trace only and cannot change curl. Emitting the full CSV and writing it under `sd:/apps/backyardcornhole/data/swing-traces/` belongs to the lab/save integration; input supplies the report and release hooks without opening files.

## Host verification

Run from the repo root:

```sh
make -C wii host-test
# Or, without make/devkitPPC:
sh wii/tests/run-input-tests.sh
```

The fake-WPAD suite replays six quantized Bowling-style motion fixtures at 100 Hz and through batched 60 Hz polling, plus the 0.1.3 remote-up swing through 60 Hz polls with frame hitches, lost reports and a stall. It also checks that a stationary B tap is one weak lob. `throw_test.c` replays the 0.1.3 traces (`tests/bowling_trace.h`): B pressed while moving, the remote-up Bowling swing released from -5 to +35 degrees, jitter and 30-60 ms gaps at 100 and 200 Hz, a gentle swing, a B tap, back-to-back throws, gap/overflow rules, aim/curl/left hand and power ordering; `throw-test settled` is the diagnostic used to isolate the 0.1.2 root causes. The fixtures use Wii Remote value ranges. Physical Wii Remote and DolphinBar timing and feel still need a hardware session. Export the modeled samples with `wii/build/wpad-test dump-realistic` after `make -C wii host-test`.

API references checked against upstream [wpad.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wpad.h), [wpad.c](https://github.com/devkitPro/libogc/blob/master/wiiuse/wpad.c) and [wiiuse.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wiiuse.h). The toolchain branch must compile against its actual pinned libogc release.
