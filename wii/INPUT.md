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

`InputSink` receives synchronous events with zero-based slot/channel. `INPUT_THROW` includes normalized power, release aim in radians, signed landing-curl spin, peak speed, release roll delta, clipping, release cause and release timestamp. Dispatch it once to the game's launch hook. Height and surface events carry absolute values (0 SLIDER / 1 NORMAL / 2 KOBE, `ARC_KOBE`; 0 SLICK / 1 STICKY). The height picker is the only source of the arc: `SwingResult` has no pitch, so the remote's release pitch never reaches the throw. Step carries a clamped stance target; `input_stance(..., now)` supplies its 70 ms visual ease. RESET BAG re-centers the stance and discards windup; the game performs its own bag reset without consuming a throw. No event clears the practice board: CLEAR BOARD belongs to the game's pause menu.

The event callback should queue actions for the game to apply after `input_wpad_poll` returns. Avoid changing input context/player/calibration inside callbacks. Menus use discrete confirm/back, D-pad navigation and minus/plus events (minus/plus exist only in menus). The adapter requests `WPAD_FMT_BTNS_ACC`, disables the optical idle threshold and never reads camera data. There is no cursor or optical calibration.

D-pad LEFT/RIGHT moves stance. UP/DOWN picks SLIDER, NORMAL or KOBE. Minus/plus do nothing in gameplay. Swing speed controls power and sideways motion controls aim. During the forward swing, excess-force/time-weighted grip-relative lateral acceleration sets aim. Spin combines lateral swing motion and wrist twist relative to neutral roll. Both readings mirror for LEFT HAND. B-up locks aim and curl for the throw.

Render the exported `input_help_text` in the help screen. It lists, one per line: B HOLD, SWING BACK & FORWARD, LET GO · SWING SPEED = POWER · SWING LEFT/RIGHT TO AIM · TWIST TO CURL · D-PAD UP/DOWN KOBE-NORMAL-SLIDER · D-PAD LEFT/RIGHT MOVE · A SURFACE · 2 RESET · 1 HELP · HOME PAUSE. Hold **1+2** for two seconds to toggle live accelerometer readings, B state, swing phase, report timing and the last throw's peak speed, power and aim. The overlay starts hidden.

Setup and pause expose a HAND row per player. Call `input_focus_hand(input, slot)` when that row has focus (menu context or paused); LEFT selects LEFT, RIGHT selects RIGHT. Call `input_blur_hand` when focus leaves the row. `INPUT_PROFILE_CHANGED` carries the selected slot and hand value; queue saving of that slot's `profile` fields alongside SET POWER calibration. It cancels calibration and resets the detector. Load via `input_set_profile`. Renderer and storage remain game-branch integration; no files are written by input.

While B is held, A/D-pad/2 are ignored (minus/plus always are, in play). Help toggles without cancelling. HOME sets pause and discards the gesture. `INPUT_CONNECT_REMOTE` supplies the required remote number as `value` for `CONNECT REMOTE n`. Slot 2's disconnected state accepts A on Remote 1 for reassignment. The game calls `input_resume` only after reconnect/reassignment; it returns false until the assigned remote has supplied a valid B-up report. Resume never reuses the old gesture; the next B press grips at once.

## Swing phases and power

B-down grips immediately in any pose, including while moving. The direction reference is the normalized accelerometer average over the preceding 100 ms. The opening turn waits for B-up if B was held or its state was unknown on entering play. A B press held during bag flight or a turn change can grip when a later turn opens.

The shoulder-pivot model treats Y as tangential and the X-Z magnitude as a roll-invariant radial proxy. Each is smoothed with a 6 ms time constant. BACKSWING requires a tangential drop of 0.115 g from grip for 10 ms. When a grip starts with over 1.20 g radial load, the detector watches for recovery from braking or an increase in radial load before it uses the tangential drop. FORWARD requires the radial signal to fall at least 0.09 g from its prior high and then rise at least 0.035 g from the trough for 10 ms. That trough tracks the loss and return of centripetal loading through the turn. The recorded tangential force includes gravity and can flip sign while the arm is still going back. See [WiiBrew's accelerometer documentation](https://www.wiibrew.org/wiki/Wiimote#Accelerometer) and the [Wii Bowling measurements in Groveman, fig. 3.12](https://www.barbara-lerner.com/WiiMotility/thesis_paper.pdf#page=31).

```
e = max(0, length(gforce) - 1)
mean_e = time-weighted trapezoidal mean over up to 80 ms of classified forward reports
v = sqrt(0.6 * 9.80665 * mean_e)
power = clamp(0.55 * forward_peak_v / (slot.full_speed * type_scale), 0, 0.76)
```

The forward window needs at least 16 ms of samples. It starts at the FORWARD classification, so the backswing contributes zero to the measured peak. To qualify, the mean must reach **0.22 g** (about **1.14 m/s** on this model) for at least **24 ms**. The game releases on B-up if the peak qualified and the most recent qualifying load is at most **140 ms** old. B-up without that motion cancels and returns to READY. It sends no throw event and keeps the bag. In the game's manual release mode there is no hold timeout. The core's optional automatic finish and 2.5 s timeout also require a recent qualifying forward peak to emit a throw.

The default shared speed reference is 3.344 m/s (1.9 g excess). Both preview and release multiply that reference by **1.30 for SLIDER**, **1.20 for NORMAL**, and **1/0.90 for KOBE**. KOBE therefore produces 90% of the previous power in the linear range. The shared 0.76 cap is applied after division, and the default cap speeds are approximately 6.006, 5.544 and 5.134 m/s respectively.

Grip-relative neutral roll comes from the grip vector when its X-Z projection is large enough, or from the first 60 ms of loaded samples otherwise. Phase and speed are estimates from accelerometer reports. The 0.60 m shoulder radius is a model assumption. Physical grip, arm geometry and report cadence affect both readings.

Report gaps over 25 ms and up to 100 ms are bridged with interpolated samples. Longer gaps break the forward averaging window, and gaps over 250 ms discard motion while preserving a held grip. Invalid accelerometer reports still carry B. Duplicate or backward timestamps are advanced, and history overflow drops the oldest sample. Saturated reports contribute no measured excess and cannot qualify or raise the retained peak. A throw following clipping uses its measured forward peak, with clipping flagged for diagnostics and calibration. Raw rail defaults are 0..1023 with a two-count margin. `input_wpad_set_rails` accepts measured per-channel rails.

## SET POWER and saves

Profiles belong to player slots: `full_speed`, `spin_strength` (0..1), `left_handed`. Load validated saved fields with `input_set_profile`; serialize them field by field in the game's save layer. Do not write C structs to disk. The input layer performs no storage I/O.

Enter a play context, resume if needed, and call `input_begin_calibration` for the active slot with `target_power` from the physics branch's NORMAL/SLICK board-center calculation, plus the profile limits. The current game uses a provisional floor of `0.55 * swing_speed(0.5) / (1.20 * target_power)` and a ceiling of `swing_speed(2.0)`. A target above the cap or invalid limits return false. Show `TOSS EASY ×3`. Three classifier-valid, unclipped throws produce `clamp(0.55 * median_peak / (1.20 * target_power), floor, clip_ceiling)`. Any peak more than 30% from the median, a clipped toss, or a disconnect discards the set and emits `INPUT_CAL_RETRY` (`TRY AGAIN`). A cancelled release keeps calibration progress. `INPUT_CAL_MORE` reports progress; `INPUT_CAL_DONE` updates the active slot profile (`POWER SET!`). These tosses never emit a game throw. Save on DONE. Slot changes cancel setup; `input_cancel_calibration` exits explicitly.

## Trace hooks and timing limits

`InputWpadTraceSink` receives each drained report (0.1.3 never rejects a batch): drain time, event index, **estimated** time, raw `WPADData` (axes, extension, err/data flags), normalized sample (including clipping), active player slot and detector state. Borrowed pointers expire when the callback returns. The slot is the current thrower's slot; inactive-channel rows are acquisition diagnostics, not gestures belonging to that slot. Copy rows into your lab recorder; avoid file I/O while draining.

`WPADData` carries no timestamps. The adapter spaces each batch across the elapsed interval ending at drain time, at most 25 ms apart (after a stall the batch ends at drain time and the detector sees the hole). The first batch uses nominal 10 ms spacing. Full buffers, dropped reports, frame hitches and empty polls are counted (`dropped`, `empty_polls`) but never cancel the gesture. Check the estimated times and counters when comparing a hardware trace to a host replay.

The 1+2 overlay (hold two seconds) shows live accel, B, the swing phase (READY, GRIP, BACKSWING, FORWARD, RELEASE, SPENT), reports per second and the largest report gap over about the last two seconds (from the adapter's estimated times), the live and peak forward speed, and the last throw's peak speed and power.

The game's trace writer must add build/target/independent speed labels and a header containing `SWING_VERSION`, every constant from `swing_config.h`, slot profile, calibration limits, raw rails and nominal report interval. Keep samples through release+133 ms, then call `swing_twist_relative` to sum the four largest gforce.x values within ±133 ms of `toss.release_ms`. This is trace only and cannot change curl. Emitting the full CSV and writing it under `sd:/apps/backyardcornhole/data/swing-traces/` belongs to the lab/save integration; input supplies the report and release hooks without opening files.

## Host verification

Run from the repo root:

```sh
make -C wii host-test
# Or, without make/devkitPPC:
sh wii/tests/run-input-tests.sh
```

The fake-WPAD suite replays quantized Bowling-style fixtures at 100 Hz and through batched 60 Hz polling, with frame hitches, dropped reports and a stall. `throw_test.c` checks phase timing against five noisy shoulder-pivot traces, with bounds of 30 ms for BACKSWING and 50 ms for FORWARD. It also covers taps, tiny wiggles, backswing-only cancellations, subsequent throws, and a B press held over into the next turn. The game tests check Practice throw counts, picker preview and launch power, and first-touch distances. Physical Wii Remote and DolphinBar testing remains pending. Export the modeled WPAD samples with `wii/build/wpad-test dump-realistic`.

API references checked against upstream [wpad.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wpad.h), [wpad.c](https://github.com/devkitPro/libogc/blob/master/wiiuse/wpad.c) and [wiiuse.h](https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wiiuse.h). The toolchain branch must compile against its actual pinned libogc release.
