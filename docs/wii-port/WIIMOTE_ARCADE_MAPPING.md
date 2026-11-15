# Arcade Spinner + Buttons → Wiimote: Translation Research

Status: research + design guidance for the Wii input implementation · 2026-11-15 (rev 2: Codex review reconciled)
Normative spec: [SPEC.md §4](SPEC.md) (if this doc and SPEC disagree, SPEC wins and this doc gets fixed)
Source of the cabinet controls: Backyard Cornhole Arcade **1.0.3**, `arcade/cabinet-snapshots/1.0.3/` (`READ-ME.txt`, `DEV-NOTES.txt`, `resources-app/dist/site/arcade/trackball.mjs`, `controller.js`, `legend.mjs`)

> **Standing rule:** toss power comes from swing **SPEED**, the peak of an accelerometer-derived speed signal over a short window. **Never** stroke distance, swing length or swing duration. A long slow swing stays soft; a short quick swing can be full power.
>
> The **rule** is fixed. The **estimator** that turns plain-Wiimote acceleration into speed is a *candidate* that IMPL_PLAN P1 must validate (SPEC §4.4). Nothing in §2's precedents validates the power equation or the direction classifier.

---

## 1. What the cabinet does today (1.0.3)

### 1.1 Spinner (trackball) gesture — `Trackball` in `trackball.mjs`

| Aspect | 1.0.3 behavior | Code/notes |
|---|---|---|
| Throw | Roll **forward** | A sample counts as forward if `-dy > 0 && -dy >= |dx|·0.65` |
| **Power** | **Peak forward speed** (counts/sec, sensitivity-adjusted) over a sliding **80 ms** window, `power = clamp(peak / powerSpeed, 0, 1)`, linear | `SPEED_WINDOW_MS = 80`; default `powerSpeed = 6048`; `3024` → 50% |
| Valid sample | Window ≥ **16 ms** and ≥ **8** forward counts; the opening report is excluded (unknown duration) | `MIN_SAMPLE_MS`, `MIN_FORWARD_COUNTS` |
| Release | Forward motion ends (direction change), **160 ms** idle, or windowed speed drops **≥ 40%** below peak after a full window; peak retained | `finish()`; `speed <= peakSpeed·0.6` |
| Stroke length | `releaseGate` 400 counts (20–1200) is **only an auto-release upper bound**, never power | "Distance does not grant power: a long slow roll stays weak" |
| Timeout | 2.5 s forward or backswing timeout | `timeoutMs` |
| **Aim** | Angle of the forward roll: `aim = clamp(atan2(fx, fy)·0.10, ±0.10)` rad; host clamps to `AIM_MAX = 0.12` | Forward-left (1) aims left, forward-right (3) aims right |
| **Spin** | Optional **backswing**: back-left (A) → negative spin = clockwise from above = **curls right**; back-right (C) → positive = curls left; straight back (B) or none → 0. Lean scales it: `spin = clamp(backX/backY, ±1)` | Spin affects landing curl only, never flight curvature (`spin.mjs`) |
| Anti-misfire | **450 ms quiet** after each turn before input arms; one throw per gesture; single report or tiny jitter can't throw; sideways rolls ignored | `quietMs`, `phase` `settling/ready/backswing/forward/spent` |
| Calibration | "Roll forward hard, like a full-power throw": `powerSpeed = measured_peak / 0.7` | Leaves 30% headroom |
| Feedback | Live power gauge + 5-stop color ramp; last-shot gauge lingers **3 s**; power arrow shows aim | `LAST_SHOT_MS = 3000`, `shotPowerColor` |

### 1.2 P1 buttons (Twin USB grid, Pad 0)

Physical positions: per `DEV-NOTES.txt`, indices **0, 1, 2, 3, 5 were confirmed by live presses**; **4, 6, 7 are inferred** from the standard Twin USB layout (Top 4 was never observed directly). Marked ✓ / ~ below.

| Physical | Index | In play | In menus |
|---|---:|---|---|
| Top 1 ✓ | 0 | Cycle height **KOBE / SLIDER / NORMAL** | Confirm |
| Top 2 ✓ | 1 | Flip **SLICK / STICKY** | Back |
| Top 3 ✓ | 2 | **Reset bag** (center, clear windup; no throw used) | (reset alias) |
| Top 4 ~ | 6 | **KEYS** legend tuck/pop (remembered) | KEYS |
| Bottom 1 ✓ | 3 | **Nudge left** 0.05 (clamp ±0.45, 70 ms ease) | Confirm |
| Bottom 2 ~ | 4 | **Nudge right** 0.05 | Back |
| Bottom 3 ✓ | 5 | Reset bag | (reset alias) |
| Bottom 4 ~ | 7 | KEYS | KEYS |
| Start/Select | 8/9 | Pause / pause | Back / confirm |

One action per press; holding never repeats. **Height, surface, nudge and reset all call `machine.reset()`** (`controller.js`): they cancel any windup and put the spinner back into `settling`. Nudges never throw. Keyboard fallback: H height, F surface, R reset, Enter, Escape, F2 setup.

---

## 2. Research: precedent and platform facts (what was taken from each source)

Each row is tagged by **kind**: **[API/sensor fact]** (what the hardware/library provides), **[control precedent]** (how other games map gestures; validates the *grammar*, not our math), or **[assumption to validate]** (used by our candidate detector, unproven until P1).

| # | Source | What it says (short) | What we took / rejected |
|---|---|---|---|
| S1 [control precedent] | Wii Sports Bowling | Hold B, backswing, forward swing, release B; direction and twist during release | Owner-selected gesture grammar; aim/spin derive from swing motion. See SPEC §4.6–4.7 for the candidate math and limits |
| S2 [control precedent, unverified] | Wii Sports Resort | Automatic/manual release and wrist twist | Optional automatic release retained; twist is sampled at release |
| S3 [API/sensor fact] | WiiBrew Wiimote | Calibrated acceleration, raw per-axis rails including gravity | Buttons + acceleration only (`WPAD_FMT_BTNS_ACC`); raw axes flag clipping |
| S4 [API/sensor fact] | **WiiBrew — Wii MotionPlus**: https://wiibrew.org/wiki/Wiimote/Extension_Controllers/Wii_Motion_Plus | 14-bit yaw/roll/pitch rates; slow mode ≈ **13.768 units per deg/s**, fast mode scaled by **2000/440**; slow/fast bits are 1 when rotating slowly; zero ≈ 0x1F7F (8063) "calibrate at startup"; activation by register writes | **Taken (optional path):** if MotionPlus is present, zero it at rest, apply the fast-mode scale, and use pitch rate to confirm swing direction and extend range above the accelerometer rail. Gyro rate alone isn't hand speed when the wrist articulates. **Never required, and never the fix for a failing plain-remote path.** |
| S5 [API/sensor fact] | libogc WPAD | Buffered reports, dropped counts, reconnect registration; no timestamps | One queue consumer per channel; calibrated gforce, raw rails, reconstructed timestamps; no optical data |
| S6 [API/sensor fact] | Hand-held accelerometer study, arXiv 1309.2093: https://arxiv.org/pdf/1309.2093 | Wiimote reports over Bluetooth at **100 Hz**; ADXL330, 300 mV/g | **Taken:** nominal 10 ms spacing as a starting assumption; an 80 ms window ≈ 8 samples. The 16 ms minimum is an **elapsed span** (first→last sample), i.e. ≥ 3 samples at 100 Hz, not "2 samples". Real rates are measured per target in P0 |
| S7 [API/sensor fact] | **Dolphin source**: `Core/HW/Wiimote.h`, `Core/ConfigLoaders/GameConfigLoader.cpp`: https://github.com/dolphin-emu/dolphin | `WiimoteSource { None=0, Emulated=1, Real=2 }`; `[Controls] WiimoteSource0..3` in a game's ini maps to Wii Remote 1–4 source | **Taken:** ship `GameSettings/BYCEAO.ini` with `WiimoteSource0 = 2`, `WiimoteSource1 = 2` (SPEC §1.5) |
| S8 [API/sensor fact] | Dolphin motion simulation | Swing and Tilt | Smoke tests only; real remotes required for feel and direction/twist acceptance |
| S9 [control precedent] | Repo: `/workspace/docs/cornhole/spin-research.md` (ACL / ACA shot guides) | Flat spin, slick vs sticky, airmail/slider/push; the arcade convention A → clockwise/right curl, C → counterclockwise/left | **Taken:** the sign convention, carried over exactly: twist right ≙ A ≙ curl right |
| S10 [target fact] | Existing deployment notes | Hardware deployment remains outside this input change | No deployment or device settings changed; game consumes buttons and acceleration only |

---

## 3. Translation principles

1. **Same verbs, same outcomes.** Every cabinet action gets exactly one Wiimote action, and the game logic behind it (`host.throw({power, aim, spin})`, height, surface, nudge, reset) stays identical. Only the input layer changes.
2. **Same power rules, different measurement.** Peak of an 80 ms windowed speed signal, linear to a calibratable full-power value, capped at 100%, peak retained to release, 40% *speed*-drop auto-release. Distance, duration and arc size never feed power. The cabinet measures speed directly (counts ÷ time); a plain Wiimote can only *estimate* it from acceleration, so the estimator is validated in P1, not assumed.
3. **Capture neutral grip, then read the swing.** Stable B-down captures neutral attitude and fine trim. Lateral forward-swing motion sets direction; lateral motion and release twist set spin, frozen at release. Both mirror for LEFT HAND.
4. **Backswing stays optional.** It's never required, and backswing samples never count toward power.
5. **Same anti-misfire guarantees, plus conservative cancellation.** A quiet period after each turn and after any legal button action; one throw per gesture; jitter or a B tap can't throw; residual motion can't fire the next bag; packet gaps, disconnects and HOME cancel the gesture rather than guess.
6. **Buttons: one action per press, no auto-repeat**, as on the cabinet. Any button that changes the throw (height, surface, step, reset) is **ignored while B is held** and otherwise resets the detector to `settling`, mirroring `machine.reset()`.
7. **Feel the toss, read the screen.** Gesture for the throw; buttons for discrete choices; short on-screen words only (SPEC §6).

---

## 4. The mapping

### 4.1 Gesture (candidate detector; SPEC §4.4 is normative)

| Cabinet (spinner) | Wiimote | Implementation |
|---|---|---|
| Forward roll **speed** → power | **Forward underhand swing speed** (estimated) → power | `e = max(0, |gforce| − 1 g)` (a scalar-gravity heuristic, not true motion acceleration); `ē` = time-weighted 80 ms mean; pendulum-bottom proxy `v = sqrt(R_EFF·g·ē)` (`R_EFF` 0.6 m); `power = clamp(peak_forward_v / fullPowerSpeed)`. **Known limit:** `v_est = v·sqrt(R_EFF/r)`, so arm-radius changes skew it (~2× between 0.3 m and 1.2 m). Hence a **supported gesture** (full-arm underhand from the shoulder, elbow mostly straight), measured in P1 against video speed labels |
| Valid sample: ≥ 16 ms window, ≥ 8 counts | Window **elapsed span ≥ 16 ms** (≥ 3 samples at 100 Hz) and `ē ≥ 0.15 g` | Span, not sample count |
| Release on roll end / 160 ms idle / 40% drop | **Release B** (primary) / auto-release at `v ≤ 0.6·peak_v` (≡ `ē ≤ 0.36·peak_ē`) after a full window, with follow-through confirmation / 2.5 s timeout | Same *speed* ratio as the cabinet |
| `releaseGate` (stroke length bound) | No equivalent needed | Swing length is never measured |
| Forward roll **angle** → aim | Lateral forward-swing acceleration → aim | SPEC §4.6: grip-relative, lobe-local proxy; straight from stance, no auto-target; −/+ fine trim |
| Backswing A / C / B → spin | Lateral swing + wrist twist at release → spin | SPEC §4.7: signed landing curl, mirrored for LEFT HAND |
| Sideways-only roll → ignored | No confirmed forward lobe → cancel | Lateral input changes direction only within a valid toss |
| 450 ms quiet after each turn | 450 ms with `ē < 0.15 g` before B can arm, after each turn **and** after any legal button action | Same constant |
| One throw per gesture | Throw once, then `spent` until next turn | Same state machine shape |
| Calibrate: roll hard → `powerSpeed = peak/0.7` | SETTINGS → **SET POWER** → "TOSS HARD" → `fullPowerSpeed = min(peak/0.7, clip ceiling)` | Reaching full power then needs ≈ 2.04× the calibration toss's `ē` (since `ē ∝ v²`), so the result is capped below the measured rail. Saved **per player slot** |

**Forward vs backward (acceleration sign alone isn't velocity direction):** a hand braking during the backswing accelerates toward the screen while still moving backward, and the remote rotates through the swing. So the sign-of-acceleration rule from rev 1 is withdrawn. P1 records every swing through these candidates in parallel and picks one:
- **C1 lobe timing (primary candidate):** a swing with a backswing passes the bottom twice, giving two `ē` lobes separated by a low-`ē` apex. Power comes from the **last** qualifying lobe (≥ 0.35 g, ≥ 40 ms) that ends within `T_REL` (≈ 250 ms) of the B release. A release at the back apex leaves no recent lobe → cancel.
- **C2 follow-through:** after the forward pass, the remote rises back toward the screen: calm gravity direction at least 15° forward-up from the grip. Confirms a lone lobe and gates auto-release.
- **C3 apex pause:** 40 ms of two-sided calm (`abs(|g|−1) < 0.10 g`, pairwise attitude variation < 5°, mean excess < 0.15 g) at least 15° behind grip discards the back lobe. The next qualifying lobe is forward; B-up throws immediately with frozen peak/aim/spin, without static follow-through. C2 also requires this sustained calm dwell. Settling and grip stability use two-sided deviation, so sub-1 g motion is never assumed calm.
- **C4 MotionPlus pitch-rate sign:** optional, never required.

**Clipping:** the rail is per raw axis **including gravity**. At the bottom the dominant axis reads ≈ `1 g + ē`, so rev 1's "`ē ≈ 2 g` is well below ±3 g" was wrong and is withdrawn. The full-power default (candidate `ē ≈ 1.2 g`) comes from measured rails. Saturation can only **cap an already validated forward toss** at full power. It never validates a toss, and saturated samples in grip, backswing or rejected phases are ignored and flagged.

**Timing:** `WPADData` has no timestamps. Sample times are reconstructed estimates (flagged in traces). A gap > 25 ms or a dropped-event increase while armed **cancels and re-arms**; nothing is interpolated across it.

### 4.2 Buttons

| Cabinet | Wiimote | Why this button |
|---|---|---|
| Top 1 — cycle height | **D-pad ↑ / ↓** — height up/down (SLIDER ⇄ NORMAL ⇄ KOBE) | Up = higher arc reads naturally; direct up/down instead of a cycle avoids overshoot |
| Top 2 — SLICK/STICKY | **A** — flip surface | Big thumb button, one press = one flip |
| Top 3 / Bottom 3 — reset bag | **2** — reset bag | Out of the way of the grip, so it's hard to hit by accident |
| Top 4 / Bottom 4 — KEYS legend | **1** — help on/off (remembered) | Same "tuck/pop" behavior |
| Bottom 1 / 2 — nudge ±0.05 | **D-pad ← / →** — step left/right on the foul line (0.05, clamp ±0.45, 70 ms ease) | Wii Sports precedent: D-pad positions before the throw (S1) |
| Start/Select — pause/confirm | **HOME** — pause (RESUME · CLEAR BOARD in practice · MAIN MENU · QUIT); **A** confirm, **B** back in menus | Wii convention; HOME returns to HBC from QUIT. Practice **CLEAR BOARD** (1.0.3's "clear bags; keep stats") is separate from RESET BAG |
| CABINET SETUP | SETTINGS (SET POWER, CURL, HAND, WIND, RESET) | Cabinet-only items (sensitivity, bindings, stroke length, diagnostics) are dropped. WIND follows the arcade policy (calm by default in Practice/2 Player) |
| (no cabinet equivalent) | **− / +** — fine aim trim | 0.01 rad per press, captured at grip |
| Keyboard fallback, F2 | Dev builds only: hold **1+2** at boot → dev overlay | Never in release builds |

**Legality while armed** (SPEC §4.3): while B is held, A, D-pad, 2 and −/+ are **ignored** (discrete settings are frozen; swing direction and release twist remain live). 1 toggles help (UI only). HOME pauses and **cancels** the gesture. When not armed, A/D-pad/2 apply and return the detector to `settling`, matching the cabinet's `machine.reset()` on height, surface, nudge and reset.

### 4.3 Player slots, pass-and-play and disconnects

- RED = player slot 1 on Remote 1. BLUE = slot 2 on Remote 2 if connected at match start; otherwise slot 2 is pass-and-play on Remote 1. In Dolphin, both remotes come from the per-game ini (`WiimoteSource0/1 = 2`).
- SET POWER, hand and curl strength belong to the **player slot** (remote identity isn't stable across reconnects or channel changes). In pass-and-play, Remote 1 uses the current thrower's slot profile.
- Disconnect of the current thrower's remote: **pause, discard the gesture**, show `CONNECT REMOTE n` (slot-correct), with `A: USE REMOTE 1` for slot 2. Resume only after reconnect/reassign **and B up**; the detector restarts in `settling`.

---

## 5. Gesture state machine (reference for the implementation)

```
settling ──(ē<NOISE for 450 ms)──► ready
ready ──(B down AND last 150 ms stable: ē<0.10g, Δattitude<5°)──► grip  [capture fine trim + neutral grip; read swing until release]
ready ──(B down but unstable)──► ready, show HOLD STILL (no arm)
grip ──(lobes detected per chosen classifier C1–C3)──► forward lobe tracked: peak_forward_v (80 ms, span ≥16 ms)
grip/forward ──(B up)──► valid forward lobe? THROW → spent : cancel → ready (bag stays)
forward ──(v ≤ 0.6·peak_v after full window AND C2 confirms)──► THROW → spent
armed ──(2.5 s timeout)──► valid lobe? THROW : cancel
armed ──(gap >25 ms | dropped events | disconnect | HOME)──► cancel → settling (pause for HOME/disconnect)
ready ──(A / D-pad / 2 / −+)──► apply → settling      armed ──(A / D-pad / 2 / −+)──► ignored
spent ──(next turn)──► settling
```

Tunables (one header, all recorded in every swing trace with the detector version): `SPEED_WINDOW_MS 80`, `MIN_WINDOW_SPAN_MS 16`, `NOISE_G 0.15`, `STABLE_G 0.10`, `STABLE_DEG 5`, `FORWARD_START_G 0.35`, `LOBE_MIN_MS 40`, `T_REL_MS 250`, `QUIET_MS 450`, `TIMEOUT_MS 2500`, `AUTO_RELEASE_SPEED_RATIO 0.6`, `GAP_CANCEL_MS 25`, `R_EFF_M 0.6`, `E_FULL_DEFAULT_G ≈1.2 (from P1)`, `CAL_HEADROOM 0.7 (capped at clip ceiling)`, `ROLL_DEADZONE_DEG 10`, `ROLL_FULL_DEG 45`, `AIM_MAX_RAD 0.12`, `AIM_NUDGE_RAD 0.01`, `STEP 0.05`, `STEP_CLAMP 0.45`.

---

## 6. Emulator bindings for day-to-day dev (Dolphin emulated Wii Remote)

Suggested, for flow testing only (S8):

| Wii input | Bind to | Purpose |
|---|---|---|
| B | Right mouse | Grip / release |
| Motion Simulation → Swing (forward/back) | A key or stick | Fires a canned swing, so a throw happens. **Power from this isn't representative** |
| Motion Simulation → Tilt left/right | Q / E | Wrist-twist curl |
| A, 1, 2, −, +, HOME, D-pad | Keys | Buttons |

Real feel always comes from real Wiimotes via the DolphinBar in Mode 4 (desktop and Kinhank) and from the real Wii.

---

## 7. Input test cases (traceable to IMPL_PLAN P1 / P2 / P5)

Speed is judged against **independent labels** (≥ 120 fps video of a hand marker, SPEC/IMPL_PLAN P1), never against the estimator's own output. Every trace carries the SPEC §4.4 header (detector version, tunables, target, slot, channel).

| ID | Do | Expect |
|---|---|---|
| T1 | 10 slow, long, wide swings (supported gesture) | Average power < 0.4 |
| T2 | 10 short, quick swings (supported gesture) | Average power > 0.7 |
| T3 | ≥ 30 labeled swings, ≥ 3 players | Estimated vs filmed speed ordering agrees ≥ 90% pairwise |
| T4 | Same filmed speed, shorter vs longer arm radius; different grip angles | Spread measured and reported; the supported envelope is defined from it (no fixed ±0.05 promise) |
| T5 | Backswing only, release B at the back | No throw |
| T6 | Backswing with hard braking, then release | No throw from the braking lobe |
| T7 | Motion already under way when B is pressed | Doesn't arm (`HOLD STILL`) |
| T8 | No backswing, single forward toss | Throws (backswing optional) |
| T9 | Tap B, no swing; shake; wrist-only rotation | No throw (or soft, never full power) |
| T10 | Hold B through the swing | Auto-release at a 40% *speed* drop with follow-through confirmation |
| T11 | Packet gap / dropped events mid-swing (forced) | Gesture cancelled, re-arms cleanly |
| T12 | Swing hard enough to clip | Validated toss → full power, flagged; clipping in a rejected phase → ignored |
| T13 | Point left/right of the hole, swing with wobble | Landing left/right; aim unchanged by the wobble |
| T14 | Twist right + SLICK; twist left; neutral | Curl right; curl left; straight |
| T15 | Left-handed mode, twist toward/away from the body | Mirrored correctly |
| T16 | Straight / left / right swing, then −/+ before grip and while armed | Straight launches from stance without targeting; yaw changes aim; trim ignored while armed |
| T17 | Press A / D-pad / 2 while holding B | Ignored; the gesture continues |
| T18 | Press A / D-pad / 2 while not holding B | Applied; B can't arm for 450 ms of calm |
| T19 | Throw, then shake the remote during the next turn's first 450 ms | No throw fires |
| T20 | Two remotes in Dolphin (ISO + ini) and on the Wii; pass-and-play with one | Each slot controls its own team; slot profiles kept |
| T21 | Disconnect the current thrower's remote mid-gesture (each slot) | Pause, gesture discarded, `CONNECT REMOTE n`, clean resume after reconnect + B up; `A: USE REMOTE 1` works for slot 2 |

Every test runs on the real Wii and (via Retro) the Kinhank before P1 sign-off (T20/T21 by P2). T1–T12 are also replayed on the host from recorded traces. Plain Wii Remotes must pass all of them before any MotionPlus refinement counts as done.

---

## 8. Deliberately not copied

- **Release timing alone as spin:** elapsed time never supplies curl. The owner-required lateral motion and wrist twist at B-up do supply spin, mirrored by hand; this supersedes the earlier rejection of tilt-at-release spin. Accelerometer roll under load remains a proxy to validate on hardware.
- **Earlier pointing design:** superseded by the owner directive. Swing direction is required; validate the candidate lateral proxy on real remotes.
- **Distance-based anything:** the retired 0.2.x / top-level `arcade/trackball.mjs` model (`power = forwardY / 400`) is never ported.

---

## 9. Decisions from review (Codex, 2026-11-15)

Source: [REVIEW_CODEX.md](REVIEW_CODEX.md); the overall accept/reject list is in [SPEC.md §10](SPEC.md).

**Accepted and applied here**
- **I1:** the estimator is labeled a candidate; "measured the same way" removed; the radius error is stated; a supported gesture plus validation against independent speed labels replaces the T3 ±0.05 guarantee.
- **I2:** the acceleration-sign forward/backward rule is withdrawn; C1–C3 classifier candidates (C4 optional MotionPlus); stability-gated arming; braking, pre-B motion, shake and wrist-only tests added.
- **I3:** auto-release in speed units (`ē ≤ 0.36·peak_ē`); 16 ms as elapsed span; clipping counted with gravity included, the `ē ≈ 2 g` default withdrawn, calibration capped; saturation never validates a toss.
- **I4:** `gforce` instead of a nonexistent `accel_calib`; `ReadPending` isn't a fallback; reconnect re-registration; estimated timestamps with gap cancellation; calm-sample roll.
- **I7:** armed-input legality, `machine.reset()`-equivalent behavior for height/surface/step/reset, −/+ blocked while armed, slot-based profiles, slot-correct disconnect prompts.
- Review §4 item 1: inferred vs live-confirmed cabinet buttons are marked. Item 2: research rows tagged by kind (API fact / control precedent / assumption).
- G2/G8 cross-references: practice CLEAR BOARD and the WIND policy are reflected in the button table.

**Rejected / not adopted:** none for this doc. The review's partial points that weren't adopted (the remote-tag verification claim, README deployment) concern SPEC §10 and REPO_LAYOUT.

Owner directive (Nov 15): no camera, sensor bar, optical cursor or optical calibration. Setup and pause expose per-slot LEFT/RIGHT HAND, saved with SET POWER. Both aim and spin mirror; see SPEC §4.6–4.7 and [ARCADE_PARITY_CHECKLIST.md](ARCADE_PARITY_CHECKLIST.md).
