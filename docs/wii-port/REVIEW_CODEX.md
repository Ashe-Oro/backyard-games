# Adversarial review: Backyard Cornhole Wii port

Reviewed 2026-11-15 against `/workspace/tailgate-games`, HEAD `a07afaa4ada23f365babc45544cc36547446734a` (release 1.0.3), and `/tmp/wii-port-prompts/kinhank-facts.md`.

**Verdict:** the native Wii direction and choice of the released 1.0.3 snapshot are sound. The documents are not yet an executable implementation contract. The largest blockers are an unproven acceleration-to-speed mapping, an incomplete ISO recipe, a golden-test plan that exceeds the available hooks, and a release workflow that would package the obsolete arcade source.

Only this review was written. No game source, build output, tags, branches, or reviewed documents were changed. The layout remains a proposal.

## Evidence and boundaries

Paths below are relative to `/workspace/tailgate-games` unless identified as a port document. For compact citations:

- `GAME` = `arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/cornhole/index.html`.
- `SITE` = `arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/`.
- `SNAP` = `arcade/cabinet-snapshots/1.0.3/`.
- Doc references include section names and line numbers from the versions reviewed.

Checks performed:

- Read the four port docs, target facts, root changelog, build scripts, package manifests, snapshot developer/player notes, gameplay, spin, controls, trackball/controller, scenery, board-view, and relevant tests.
- Ran `node --test tests/*.test.mjs` from `SNAP`: all five test files passed, exit 0. These are snapshot tests, not proof that the proposed C port works.
- Constructed the snapshot backyard and party in Node with an in-memory canvas stub. Before culling, those two scene builders produce 258 mesh nodes, 746 mesh instances, 11,114 triangles, and 72 material objects. This excludes board, bags, dog, HUD and other inline props. Material objects are not the same thing as distinct Dolphin shader configurations.
- Inspected local tags and branch refs without fetching or modifying Git. Remote-tracking refs are cached observations, not a fresh GitHub audit.
- Checked primary libogc source and WIT composition requirements, linked below. No Wii/Dolphin hardware tests, ISO build, browser suite, packaging build, or migration were performed. Top-level `npm test` includes a test that writes `arcade/dist/site`, so it was deliberately not run against the read-only repo.

Severity: **P0** = resolve before relying on the design/release route; **P1** = likely wrong behavior or broken acceptance/migration; **P2** = incomplete contract or misleading claim.

## 1. Input mapping and target defects

### I1 — P0: the proposed signal is an acceleration heuristic, not a demonstrated swing-speed measurement

**Docs:** SPEC §§4.1, 4.4, 8 (lines 309–311, 349–378, 480); WIIMOTE_ARCADE_MAPPING §§3–4.1 (lines 67, 82, 93–95).

`e = max(0, |a| - 1g)` is not generally the magnitude of motion acceleration: subtracting a scalar gravity magnitude is different from subtracting the gravity vector. The equation `v = sqrt(R_EFF*g*e)` holds only under a particular pendulum model near its bottom. It depends on actual curvature radius, arm/wrist motion, gravity alignment, and tangential acceleration. The citations establish available sensors and reporting; they do not validate this estimator for the proposed gesture.

Even under the favorable bottom-of-pendulum assumptions, `v_est = v_actual*sqrt(R_EFF/r_actual)`. At the same actual hand speed of 2 m/s, radii of 0.3 m and 1.2 m yield estimates of approximately 2.83 and 1.41 m/s: a factor of two for the same speed. A single calibration cannot remove changes of radius within one player's throws. A sharp translation or shake can also create a high acceleration peak without the intended fast underhand toss.

This does **not** secretly integrate stroke distance, which is good. It does invalidate the unconditional claims “measured the same way,” orientation-independent speed fidelity, and T3's ±0.05 guarantee. The cabinet actually measures forward counts divided by elapsed time (`SITE/arcade/trackball.mjs:112–138`). It does not infer speed from force.

**Patch in prose:** keep speed-only power as a requirement, but label this formula a candidate accelerometer proxy. Make P1 establish its usable gesture envelope using independently classified/measured speed, changed arc radius, wrist flicks, shakes and grip angles on plain remotes. Retain a short-window peak, calibration, and no distance/hold-time contribution. If it fails, revise the estimator/gesture and repeat P1; do not declare threshold tuning or optional MotionPlus a solution to the required plain-remote path.

### I2 — P0: acceleration sign cannot supply the promised forward/backward classifier

**Docs:** SPEC §4.4 phase table (lines 361–364), §4.5; WIIMOTE_ARCADE_MAPPING §4.1 “Forward vs backward” and §5 (lines 93, 120–126).

Acceleration direction is not velocity direction. A hand braking during a backswing accelerates toward the screen while still moving backward. At an apex speed is zero, but tangential acceleration can be large; a minimum of `|a|-1g` is not a generally valid apex detector. Meanwhile the remote rotates between pointing at the screen and swinging underhand. The spec never defines a maintained body-to-world transform for its ±Y/toward-player tests. A gravity vector captured at grip does not track later attitude or establish world yaw. Acceleration-derived pitch during motion is not a reliable substitute.

Consequently the required “backswing-only never throws” result is not established by the proposed algorithm. MotionPlus can help, but cannot rescue a required accelerometer-only design by being optional. Even gyro rate does not uniquely determine hand translation speed when wrist articulation changes.

**Patch in prose:** replace the normative classifier with explicit prototype alternatives and a chosen coordinate convention. Specify the supported grip, phase hysteresis, invalid-motion cancellation and how direction confidence is obtained without a gyro. Add backswing braking, motion started before B, no-backswing tosses, pointer loss and wrist-only rotation traces. Require zero false throws for these before P1 closes. Preserve B-release cancellation when no valid forward phase exists.

### I3 — P1: release, clipping and sample-validity math disagree with the stated speed model

**Docs:** SPEC §4.4 (lines 351, 364, 373–378); WIIMOTE_ARCADE_MAPPING §§4.1, 5, 7 (lines 83–95, 124, 131, 167).

Three concrete errors:

1. The mapping releases at `ē <= 0.6*peak_ē`, whereas cabinet release is `speed <= 0.6*peak_speed`. Under the proposed square root this leaves about **77.5%** speed, not 60%. A 40% speed drop requires `ē <= 0.36*peak_ē`. SPEC's “drops 40% below its peak” leaves the quantity ambiguous.
2. At the favorable aligned bottom of a swing, `ē = 2g` means total measured load is approximately **3g**. It is not “well below” a ±3g per-axis range. Calibration to `peak/0.7` raises the required acceleration further: speed divided by 0.7 corresponds to roughly twice the excess acceleration. “Any clipping means full power” can turn a knock or rejected backswing into maximum power unless phase/validity checks take precedence.
3. Two point samples at a nominal 100 Hz are only **10 ms apart**, not ≥16 ms. Sample count alone cannot enforce the cabinet's elapsed-time requirement. Counting sample support intervals would require an explicit different definition.

**Patch in prose:** compare release in the same speed units used for power; define elapsed window duration, startup behavior, time weighting and gap handling separately from sample count. Choose measured clipping headroom for the raw axes including gravity and calibration. Saturation may cap an already validated toss; it must not itself validate a toss. Update T6/T13 and all tunable tables to agree.

### I4 — P1: the WPAD route is feasible, but the sample/calibration and recovery contract is incomplete

**Docs:** SPEC §§1.1, 4.4, 8; IMPL_PLAN Track A and P0 task 3/acceptance; WIIMOTE_ARCADE_MAPPING S5–S6.

WPAD supplies the needed buttons, acceleration, IR and optional expansion data. However, `WPADData` has no `accel_calib` member; its processed `gforce` is available. The docs need to identify the public calibration path instead of implying direct access to private remote state. `a_roll` is an orientation result, not raw acceleration. See the [WPAD header](https://raw.githubusercontent.com/devkitPro/libogc/master/gc/wiiuse/wpad.h).

`WPAD_ReadPending` drains the same event queue through `WPAD_ReadEvent`; it is not an independent acquisition fallback. The disconnect handler clears the external event buffer registration. These affect the proposed recovery plan. See [libogc v3.2.1 WPAD implementation](https://raw.githubusercontent.com/devkitPro/libogc/v3.2.1/wiiuse/wpad.c).

Assigning uniformly spaced times retrospectively at frame drain cannot recover actual jitter, missing reports, or timing across a stall. A measured average event rate is useful diagnostics, not exact timestamps. The CSV omits raw saturation, `data_present`, errors, channel identity, dropped-event deltas, tunable/version identity and the timestamp reconstruction method needed to interpret replays.

**Patch in prose:** use calibrated `gforce` for the required path, with raw `accel` retained for saturation diagnostics; gate fields on validity. Document persistent buffers per channel, return-code handling, reconnect re-registration and one owner for consuming each queue. Mark reconstructed times as estimated, record drain times/order and gaps, and cancel/re-arm rather than invent a continuous swing across a gap. Give traces a schema/header containing the detector version and tunables. Verify roll freshness while calm; do not use motion-corrupted roll as reliable pitch. The [orientation calculation](https://raw.githubusercontent.com/devkitPro/libogc/master/wiiuse/dynamics.c) explicitly treats high acceleration as unreliable.

### I5 — P0: `main.dol` plus `wit copy` is not a complete ISO recipe

**Docs:** SPEC §§1.3, 1.5 (lines 94, 130); IMPL_PLAN P0 task 9 and clean-build acceptance.

WIT requires more than `sys/main.dol`: its FST partition input requires `sys/boot.bin`, `sys/apploader.img`, and a `files/` directory. `bi2.bin` can be generated by recent WIT, but that does not eliminate the other required inputs. The proposed tree and tasks supply neither their generation nor their provenance. Therefore `make iso` is not reproducible from the described checkout. See [WIT's required composition tree and free template](https://wit.wiimm.de/info/composing.html).

**Patch in prose:** add a complete, pinned homebrew disc template or boot-header/apploader generation procedure, with provenance and redistribution terms for every bundled component. Pin exact WIT flags and version; specify USA region metadata as well as ID `BYCEAO`. Validate ID/region, booting and extracted `main.dol` SHA-256 equality in P0. Keep the documented bare-DOL/global-Real fallback usable while ISO packaging is being proven. Do not call WAD an interchangeable six-character-ID package: if retained, define its actual title identifier/settings filename and prove it separately.

### I6 — P1: EURGB60 contradicts the authoritative NTSC-only contract

**Docs:** SPEC §§1.5, 8, 9 (lines 146, 492, 501); IMPL_PLAN P0 task 8, P6 video task.

The facts require **NTSC/60 Hz only**. Repeated promises to select EURGB60 on a PAL-region Wii introduce a different output standard while simultaneously claiming NTSC-only. A game-ID region letter also does not itself make application video initialization correct.

**Patch in prose:** specify NTSC progressive or NTSC interlaced output, selected by actual progressive capability, on both targets. Remove EURGB60 branches unless the owner explicitly changes the target contract. Test that a PAL-region console setting does not select 50 Hz or an unapproved mode. Keep video initialization, disc-region metadata and Dolphin configuration as separate requirements.

### I7 — P1: buttons, calm locking and remote ownership need cancellation rules

**Docs:** SPEC §§4.2–4.3, 4.7; WIIMOTE_ARCADE_MAPPING §§3, 4.2–4.3, 5 (especially line 127); IMPL_PLAN P5/release checklist.

The mapping says aim and spin lock while calm, but quietness is only `|a|-1g` below a threshold. Slow rotation can satisfy that test. There is no stability check at B press. The mapping resets the detector for D-pad/2, while the actual cabinet resets for **height and surface** too (`SITE/arcade/controller.js:168`). The proposed A behavior could therefore change surface during an armed toss. Plus/minus aim nudge is not explicitly blocked while aim is locked.

A BLUE-remote disconnect is also ambiguous: immediately switching to Remote 1 can transfer an in-progress gesture. The generic release prompt always names Remote 1 even when Remote 2 was required. “Per remote profile” has no defined stable identity across reconnects, channel reassignment or two players passing one remote.

**Patch in prose:** specify which actions are legal in every input/game phase. Any height/surface/stance/reset change must cancel and settle, or be ignored until release; define this consistently in both docs. Require a stable recent attitude before curl lock. On disconnect, pause and discard the gesture; re-arm after reconnect and B-up, with explicit player reassignment if needed. Choose profiles by player slot or an actually accessible stable identifier, and define pass-and-play behavior. Use short slot-correct prompts such as `CONNECT REMOTE 2`.

## 2. Gameplay parity and implementation gaps

### G1 — P1: the golden-test hooks cannot provide the promised coverage unchanged

**Docs:** SPEC §5.1 (line 410); IMPL_PLAN Track C (lines 85–89); REPO_LAYOUT §2.2.

`GAME:1087–1104` exposes useful but narrow hooks:

- `_probe(power, arc, spin)` always makes **team 0, slick, RX[0], aim 0, drift 0**. It feeds power directly to `launch`, bypassing the human KOBE remap. It returns a final position/state and an approximate accumulated flight time, not all contact transitions.
- `_place` and `_slide` create slick team-0/selected-team fixtures with restricted states. There is no general state import, side/arc setter, manual fixed-step hook or direct `scoreInning` hook.
- `throwWith` runs through live phases/timers and the browser frame loop. Wind, AI choices and various cosmetic paths use `Math.random`; a seeded `R` for textures is not a seeded match.

The phrase “where reachable” for sticky tests does not satisfy six arc/surface combinations, stance, both teams and collisions. A C implementation can agree with this narrow oracle while failing normal play.

**Patch in prose:** specify a test-only harness in the new Wii tools that extracts/evaluates the pinned snapshot functions, or instruments responses in memory without editing game files. `SNAP/tests/spin.test.mjs:7–20` already demonstrates extraction. Expose deterministic steps, state import/export, side, team, stance, aim, drift, wind, spin and both input/physical power. Seed/control match randomness and timers. Record source hashes, fixture initial state, timestep and expected transitions. Include scoring and AI decisions explicitly; do not claim `_probe` alone validates them.

### G2 — P1: unlimited practice violates the performance model's “≤8 bags” assumption

**Docs:** SPEC §§2.1, 3.2, 7 (lines 182, 278, 462); IMPL_PLAN P2 practice and P6 soak.

Eight bags is a match-round limit, not a game-wide bound. `GAME:843` skips the eight-bag limit in practice. `afterThrow` keeps appending turns, and `clearBags` is only called by explicit clear/restart/round operations. `collide` scans the growing list, including checks against other bags. Practice stats continue indefinitely. The source has a practice clear button (`GAME:949`), but the Wii button map gives **2** only the distinct “Reset Bag” action and never locates **CLEAR BOARD**.

**Patch in prose:** put CLEAR BOARD in a reachable practice pause action and distinguish it from RESET BAG, preserving scores/stats behavior deliberately. Add long-practice load/memory/collision benchmarks. Either size the budget for retained practice bags or explicitly specify and approve a bounded retention policy; silently limiting to eight is a gameplay change. A 20-round match soak alone does not cover this path.

### G3 — P1: preserve actual collision/scoring order, not merely the list of physics features

**Docs:** SPEC §§2.1–2.3; IMPL_PLAN Track C/P2.

The docs correctly enumerate the main constants, curl decay, 240 Hz stepping and cancellation scoring. Missing acceptance detail matters at thresholds:

- `GAME:1031–1037` steps eligible bags in insertion order, then calls `collide` once per substep; `afterThrow` waits for **all** bags to settle, including dead grass bags awaiting removal. Resting bags may be reawakened by contact.
- `touched`, `dead`, `offBoard`, `prev`, `hy`, `restT` and board-local velocity distinguish front/side impacts, grass-first fouls, falling off and hole transitions (`GAME:421–499`). Similar final positions do not imply equal score/callout behavior.
- Live `lie()` counts only `in` and non-dead `board`, not moving slides; final scoring occurs once after all eight throws. Wash order, overshooting 21, and knocked-in/off bags of either team need fixtures (`GAME:625–631, 875–918`).

**Patch in prose:** make loop order and these flags part of the core contract. Add transition fixtures for front-edge impact, grass-first stop/removal, side/back falloff, swish versus slide-in, hanger, stacked/rider contact, simultaneous hole/knockoff, a wash, and a net score that crosses 21. Assert throws consumed and score/callout timing, not just landing within 1 mm. Preserve `exact:false`; label bust-to-13 as unused support rather than another required menu mode.

### G4 — P1: AI strategy descriptions and acceptance overstate the source behavior

**Docs:** SPEC §2.4; IMPL_PLAN P4 (lines 198, 204–205).

`planCareerShot` reads the existing deck to choose a target/style, but its candidate `assess` simulates **one new bag with `step` only** (`GAME:757–802`). It does not call `collide` against blockers. “Push/knock/block goals” must not be interpreted as a collision-aware search port. Pool selection sorts by power distance from the best target and retains up to 24 candidates; Ted samples the full pool. Early-CPU wind compensation is not reduced by a level multiplier despite the nearby comment. Casual CPU also includes longitudinal wind, lateral compensation, aim noise and drift (`GAME:852`), which the shorthand power formula does not fully describe.

`CPU_RATES` are desired pool-selection probabilities, not guaranteed final live outcomes on crowded boards. A test of 200 stochastic throws within ±5 percentage points can fail by chance or because collisions alter outcomes. The asserted ~0.3 s Broadway cost, ≤2 s windup and 4 ms slices have no device measurements. Making a whole candidate simulation indivisible can still overrun a slice.

**Patch in prose:** describe the single-bag predictor accurately and preserve its exact choices for parity. Use fixed seeds and identical scenarios to compare selected arc, side, spin, power, aim, pool classification and RNG consumption against JS. Treat statistical rate checks as secondary with justified tolerances and an explicitly empty/crowded-board setup. Benchmark all rivals/fallback grids on Wii and Kinhank; permit yielding within simulation steps and sorting if necessary. Do not add smarter collision prediction during the parity port.

### G5 — P1: camera parity is factually wrong; bag surface presentation needs an explicit rule

**Docs:** SPEC §2.5 (lines 208, 215); IMPL_PLAN Track B (line 80).

The constructor's FOV is 19°, but `resize()` immediately sets `camera.fov = 18 + 6*portrait` (`GAME:250, 258–271`). At both 4:3 and 16:9, portrait is zero, so the effective landscape FOV is **18°**. Porting 19° changes board/bag framing.

Also the selected surface is the side **down**: slick-down shows sticky on top, and vice versa (`GAME:541–546, 845`). “Two material faces, slick/sticky side shown per throw” is too easy to implement backward. The held bag's material and launch orientation must match. Visual flat spin, landing yaw and decaying spin are separate from the persistent landing-curl scalar (`GAME:1038–1052`; `SITE/cornhole/spin.mjs`).

**Patch in prose:** use the effective 18° landscape camera, preserve position/look target and test both aspect ratios. State the down/up material rule, opposite surface on the top face, release yaw, full-flight 14 rad/s spin and independent slide curl/visual-spin decay. Compare fresh **1.0.3** screenshots at matching aspect ratios; older preview stills cannot define cooler presence or current HUD behavior.

### G6 — P1: PIP lacks its required clear/depth behavior; skipping alternate draws does not retain it

**Docs:** SPEC §§2.5, 3.3 (lines 220, 285); IMPL_PLAN P3.

The original `BoardView.render()` saves render state, switches viewport/scissor, clears color **and depth**, draws, and restores state (`SITE/arcade/board-view.mjs:15–24`). Merely switching GX viewport/scissor leaves main-view depth and pixels inside the inset, so culled PIP geometry can be incorrectly occluded or reveal the main view.

The cut “PIP drops to 30 Hz updates” needs a retention design. A main view redrawn every frame overwrites the inset; omitting the second pass on alternate frames makes the inset disappear/flicker. The plan simultaneously forbids texture/EFB-copy retention.

**Patch in prose:** describe a GX-compatible inset background/depth reset and restoration of projection, viewport, scissor, depth, blend and fog state before HUD rendering. Preserve the board camera parameters from `board-view.mjs`. Remove the 30 Hz draw-skip fallback; freezing PIP object updates while still drawing is possible but does not halve its draw cost. Keep the no-EFB-read/no-render-to-texture constraint unless measured evidence justifies revising it.

### G7 — P2: export and binary portability are underspecified, although the canvas-stub approach is credible

**Docs:** SPEC §§1.3, 2.2, 2.5, 3; IMPL_PLAN Tracks B/C, P3.

The headless scene builders do work with a simple canvas stub, so that claim should remain. But a stub is not the whole exporter: the board uses texture repeat/offset and geometry material groups (`GAME:369–371`); the sky is inside-facing with no fog/depth writes; grass repeats 24 times (`SITE/yard-games/backyard.js:20–25`). Flattening static party geometry must preserve pivots/object identity needed for P6 animation, and hidden-face removal must not delete faces needed by the second camera or moving props. Portraits are SVG paths, not canvas `fillRect` callers (`GAME:720–732`); they need a separate rasterization step or a documented small path decoder.

`scene.bin`/`win.bin` also cross host-to-Wii byte order and ABI boundaries. “Dump the host C result” is not enough: define format/version, byte order, numeric representation, alignment and explicit decoding. Raw C structs introduce padding and host layout differences. Floating-point tolerances in §5.1 conflict with “assert equality” in §2.2 if this means bit equality across host/PPC math libraries.

**Patch in prose:** add an exporter/material/animation manifest and a portable binary format, with repeat/offset, vertex winding, alpha/depth, material groups and dynamic hierarchy preserved. Specify linear/sRGB color conversion and lighting bake, not an assumed one-to-one TEV match. Pin browser/tool dependencies and source hashes. Use tolerant numeric parity plus exact state classification, and test decoding on target. Keep all Broadway timing estimates labeled unmeasured until P0/P1 benchmarks.

### G8 — P1: wind defaults differ from the released controller, and practice wind settings are ambiguous

**Docs:** SPEC §§2.1, 7; IMPL_PLAN P2/P5 (lines 155, 217); WIIMOTE_ARCADE_MAPPING SETTINGS row.

`roundWind` ranges and alternating sides are correctly transcribed. The actual arcade controller adds policy: `windy=false` at startup, applies `host.wind(windy)` for casual/practice, and reapplies it on non-career wait phases (`SITE/arcade/controller.js:53, 91, 98, 255`). Thus default casual 2 PLAYER is calm in the released app; describing 6–10 mph as unconditional casual behavior omits the controller. Tournament wind follows the host's circuit generation.

P5 calls WIND “CALM/ON for practice,” while the core's `roundWind({practice:true})` always returns calm. Turning it on through `host.wind(true)` instead requests non-practice wind. These are different policies that require a decision.

**Patch in prose:** specify per-mode defaults, toggling, persistence and round regeneration. Preserve the released calm casual default if parity is intended, or label the Wii default change explicitly. Define practice wind as either always calm or an intentional override that remains active on subsequent throws. Test this through the complete controller/core flow, not only the pure `roundWind` function.

### G9 — P2: save, UI and release acceptance contain unresolved product contracts

**Docs:** SPEC §§2.3, 2.8, 3.3, 6–7; IMPL_PLAN P4–P6.

- `save.dat` is directly under the app folder in SPEC §1.1, but the HBC tree lists it under `data/`. Missing-SD fallback is sensible, but the plan does not say how Dolphin Android 5.0-19870's virtual SD is provisioned/writable or how Retro exports traces. Do not substitute instructions assumed from newer desktop Dolphin.
- A versioned struct plus CRC does not define byte order, padding, interrupted-write recovery, field validation, or per-player profile identity. The source validates `fight` in 0..8 and `wins >= fight` and saves completed fights only (`GAME:704, 716–719, 750`). Preserve those semantics.
- “Callouts verbatim” includes **ON THE BOARD** and **OFF THE BOARD**, each three words, conflicting with the two-word callout gate. The 8×8 atlas also needs glyph coverage for player text, rival names and controller symbols, rather than assuming ASCII or arbitrary Unicode will render.
- SPEC allows a 30 fps last resort while scope/P3 require 60 fps. It describes static NPCs in v1 milestones, while P6 includes full animation as a release task. These are conflicting acceptance conditions rather than approved reductions.

**Patch in prose:** choose one save/log path and explicit file serialization/recovery; document virtual-SD setup for the pinned Android build at P0/P4 and Retro's trace retrieval. Put CLEAR BOARD, new-career confirmation, pause/resume, round advance and remote-reconnect behavior in a screen/action matrix. Exempt established three-word callouts from the copy gate or replace them deliberately with short approved text. Specify glyph repertoire and 480i readability. Make 60 Hz/60 fps the acceptance contract, with scenery reductions before release; describe any 30 fps edition or static-NPC release as a separate scope decision. Add a Kinhank P4 checkpoint for AI/save changes rather than waiting until P6 to discover them.

## 3. Monorepo proposal and build/release defects

### R1 — P0: the proposed arcade tag job would package retired distance-power gameplay

**Docs:** REPO_LAYOUT §§1, 5, 6–7 (lines 40–41, 167, 180, 205, 228).

The proposal correctly recognizes that root web source feeds the arcade builder and that the released snapshot is newer. It then says tag jobs run `npm run build:linux`, while claiming CI will never publish old `build.mjs` output. These statements cannot both hold.

`arcade/package.json` maps `build:linux` to `node build-linux.mjs`; `arcade/build-linux.mjs:12` unconditionally runs `build.mjs --site`. That builder copies root web and the **top-level** arcade controller/trackball (`arcade/build.mjs:13–16`), not `SNAP/resources-app`. The top-level trackball still uses accumulated forward distance for power. Its package version being 1.0.3 does not make the output released 1.0.3 gameplay. Missing snapshot `spin.mjs`/legend integration, cooler removal and runtime version plumbing further demonstrate the split.

A byte-identical pre/post-move `dist/site` check only proves that the old build stayed old. It does not prove parity with the installed release or validate a Linux ZIP.

**Patch in prose:** remove tag publication via `build:linux` until release-source reconciliation is complete. Choose a separately reviewed reconciliation PR that either promotes the snapshot's actual runtime sources into maintained arcade source or supplies an explicit snapshot-input packager that never regenerates them with the old builder. Require full resource-manifest comparison, speed/spin/version tests and Electron smoke checks on the packaged artifact before tagging. Preserve the existing launcher, preload, trusted protocol, Python helpers, private libcups/license, Batocera support hook and media package (`build-linux.mjs:18–52`; `main.cjs:3–27, 47–59`). The layout move alone must not silently republish the cabinet application.

### R2 — P1: proposed shared constants are not currently shared across web and arcade

**Docs:** REPO_LAYOUT §§2.2, 5 (lines 112–115, 169).

The proposed drift check would reject intentional existing differences on day one:

| Value/behavior | Current web | Released arcade 1.0.3 |
|---|---|---|
| Slick friction/static friction/keep | `.30 / .5 / .42` | `.43 / .60 / .32` |
| Release Y | `1.3` | `.98` |
| Spin/curl | Inline older spin and airborne curl path; human spin suppressed at throw | `spin.mjs`, 14 rad/s, landing curl only, human spin enabled |
| Effective landscape FOV | `14°` | `18°` |

Sources: `cornhole/index.html:204, 209, 269, 416, 871`; `GAME:202, 207, 267, 417, 863`; `SITE/cornhole/spin.mjs`. FOV need not be shared gameplay data, but it reinforces that these are different platform products. The web has no cornhole `spin.mjs` to parse. The current shared `yard-games/rules.mjs` is rules for the other yard games, not an existing cornhole engine.

“Fail unless a matching changelog entry exists” is also too weak: unrelated release prose is not a precise exception specification.

**Patch in prose:** make common data a verified intersection; use explicit versioned platform overrides and separately sourced baselines for differing values. Do not change web physics to match arcade as a restructuring side effect. Start the Wii golden baseline with pinned arcade 1.0.3. Define structured drift approvals referencing exact field/platform/baseline changes, with changelog entries as documentation. If Wii tools depend on these files at P0, move shared-data PR 4 earlier or make it an explicit prerequisite; otherwise keep P0 self-contained as SPEC currently proposes.

### R3 — P1: web CI commands do not match the current tests or dependency layout

**Docs:** REPO_LAYOUT §§1, 2.1, 5, 6 (lines 25, 97, 166, 174–176, 199, 206).

`tests/run_all.py:32–37` runs only `*_check.py` with working directory equal to the web root. Numerous legacy suites, for example `tests/task12_check.py:12–14` and `tests/task6_check.py:13–17`, read `Path('index.html')` and expect the **cornhole game at `/`**. The current root is already a five-game home page. Moving tests into `web/tests` does not fix this pre-existing mismatch. Their root-relative `shots/` writes also do not become `web/tests/shots/` without an explicit code change.

The proposed `cd web && python3 -m http.server 8000 &` backgrounds the compound command; the next command cannot assume the parent shell changed directories. No readiness check, cleanup trap, Python Playwright/browser install or Node dependency resolution is supplied. `.cjs` files require `playwright-core`, installed under `arcade/node_modules`, which sibling `web/tests` does not find through normal module lookup. Some use `CHROME_PATH`, some rely on Playwright's installed Chromium.

The `*_check.mjs` glob does cover all four current `.mjs` checks. Moving their relative module imports is mostly fine; that is not the same as making the Python/browser runner valid.

**Patch in prose:** inventory runnable modern tests versus historical fixtures. Give modern browser checks one explicit server root/base URL and repair legacy routes/source paths in a separate, bounded test-maintenance patch, or document their archival status. Provision pinned Node/Python drivers and browsers; give web tooling its own dev dependencies or an explicit test-only `NODE_PATH` to the arcade install. Specify exact runner working directories, artifact paths, browser selection and an explicit `.mjs` manifest. Start/stop the server with readiness/failure handling. Keep test outputs out of deployment. Run baseline tests before moving files so migration failures can be separated from existing failures.

### R4 — P1: “two path fixes only” misses the isolation test's map-key contract

**Docs:** REPO_LAYOUT §§2, 6 PR 1 task 2 (lines 67–68, 196–199).

`arcade/tests/isolation.test.mjs` stores entries under the exact relative paths traversed. If the suggested change simply makes it visit `web/cornhole`, the resulting map contains `web/cornhole/index.html`, but assertions still call `before.get('cornhole/index.html')`, `before.get('yard-games/backyard.js')` and `before.get('yard-games/power-arrow.js')` (`:27–32`). Those lookups become undefined. Adjusting only the hash roots is insufficient.

**Patch in prose:** introduce a web-root variable for filesystem access while retaining web-relative map keys, or update every producer/consumer consistently. Continue hashing root `vercel.json` separately rather than dropping its protection. Keep root `.vercelignore` checks. Call these compatibility/test edits explicitly; do not promise that only two files ever need editing while also requiring Python screenshot and route repairs.

### R5 — P1: platform workflows lack the dependencies and triggers their own commands require

**Docs:** REPO_LAYOUT §5; SPEC §1.3; IMPL_PLAN Tracks B/C and P0.

- Wii asset/golden tools require Three.js, a Node test driver and a browser from `arcade/`; `wii.yml` provides only a devkitPPC container. It does not install `arcade` dependencies or a **host** compiler for `host-test`, or specify how it gets the oracle/browser. A PPC cross compiler is not a host test compiler.
- Wii path filters ignore changes to the pinned oracle/export source under `arcade/cabinet-snapshots/...`, its dependency lockfile and the workflow itself. If generated assets are checked in, their regeneration/validation still needs an explicit dependency policy.
- The arcade row changes directory into the snapshot and then lists `npm run build:linux`; the snapshot has no build scripts. The local example mixes persistent `cd arcade` with repo-root-relative `arcade/...` commands. These need independent working directories, not inferred shell state.
- Wii uploads an HBC ZIP and checksum list without any documented target that creates them. `make iso` alone does not define that release bundle. Tag pushes also need explicit tag triggers; changed-path descriptions alone do not define release execution.
- A path-filtered required status check can remain pending on unrelated PRs unless workflow/branch-protection design accounts for skipped platforms.

**Patch in prose:** write each job as explicit steps with a declared working directory, runtime/dependency/bootstrap stage, exact command, and expected output paths. Mark all Wii commands as proposed/new, not existing scripts. Include host tests, pinned oracle/export inputs and generated-file checks; trigger on workflow/tool dependency changes and platform tag patterns. Define a release-bundle target that actually produces the named artifacts. Use an always-running lightweight gate or equivalent branch-protection arrangement for skipped platform jobs. Keep stale arcade packaging disabled under R1.

### R6 — P1: deployment fallback and artifact exclusions need a complete migration contract

**Docs:** REPO_LAYOUT §§1–2, 6 PR 1 task 3.

The `web/` name is appropriate for a single phone/desktop static site, and preserving its internal URL tree is correct. However, current `.vercelignore` contains only **`arcade`** (plus a comment), not the `node_modules`, `dist`, `.cache` exclusions listed as current facts. The proposed web tree also includes tests, test artifacts and README/changelog files; deployment inclusion must be deliberate.

The fallback changes the Vercel project Root Directory and moves `vercel.json`, after other instructions require keeping it at root and hash-checking it there. That is a different deployment layout. Reverting the Git merge does not revert a project-setting change. Local README deployment claims are evidence of intended configuration, not proof of live settings today.

**Patch in prose:** accurately describe the existing ignore file. Choose one reviewed deployment topology and test it on a PR preview, preserving CSP and all five clean/trailing-slash routes and assets. Explicitly exclude tests, screenshots, dev dependencies and generated tools from deployed output as appropriate. If a Root Directory setting change is necessary, list its exact setting rollback and adjusted config/isolation paths; do not claim a Git revert alone rolls it back. Keep the move in a normal PR with a verified baseline and no history rewrite.

### R7 — P2: changelog/tag direction is correct, but reachability and release ownership need tightening

**Docs:** REPO_LAYOUT §§3–4, 6.

The inspected annotated tags resolve as follows:

| Existing tag | Commit | Platform | Optional additive alias |
|---|---|---|---|
| `v1.0.0` | `d3002ea76884ed5b2ba16f4abc205669f4e36e0b` | arcade | `arcade-v1.0.0` |
| `v1.0.1` | `54be030e2eff3c074be5caf29085e680d15f2ab3` | arcade | `arcade-v1.0.1` |
| `v1.0.2` | `c621a22f368cba1dfd06052fc8c37893c1b10792` | arcade | `arcade-v1.0.2` |
| `v1.0.3` | `a07afaa4ada23f365babc45544cc36547446734a` | arcade | `arcade-v1.0.3` |

Keeping all legacy tags untouched, using future `arcade-v…`, `web-v…`, `wii-v0.1.0`, and moving the existing arcade-only changelog with `git mv` are good choices. Optional aliases must point to these same commits, not the restructure commit. None were created here.

The general migration rule permits squash, while PR 0 promises original `v1.0.3` reachability from main. Squashing PR 0 would not preserve that ancestry. The current local `origin/main=c621a22` is only a cached ref. The assertion “all four are on GitHub” needs an actual remote-ref check when preparing the migration. `IMPLEMENTATION_RULES.md` is cited as authority in §3 but was not found in the supplied repo/docs; avoid relying on an unavailable policy.

**Patch in prose:** make PR 0 explicitly preserve original release commits with a normal merge, followed by `merge-base --is-ancestor` checks for legacy release tags. Recheck remote state when the PR is prepared. Keep no force-push, retagging or shared-history rewrite. Add a version/metadata check tying each platform tag to its authoritative package/Makefile version and changelog. Define one approved arcade release source before enabling its tag job; an optional alias must not retrigger a stale package build. Replace the missing-policy reference with a self-contained rule or its real location. Avoid assumptions that a particular machine can/cannot push as permanent architecture requirements.

## 4. Actionable patch order by document

These are instructions for follow-up document patches, not changes made by this review.

### SPEC.md

1. Keep §0's targets, Android 12/Mali/Vulkan 1× facts, DolphinBar Mode 4, Retro ownership, exact ADB ROM destination, ID/settings requirement and global-Real fallback. Remove the EURGB60 exception in §§1.5/8/9; leave no Batocera Wii testing path.
2. Replace §4.4's guaranteed speed/direction algorithm with the experimentally gated candidate in I1–I4. Specify the measured signal, units, coordinate assumptions, elapsed-time window, same-speed release threshold, saturation handling and invalidation rules. Keep distance and duration out of power.
3. Complete §1.5's disc template/generation requirements and §1.6's pinned-Android settings/virtual-SD instructions. Add a P0 ID/region/DOL-hash boot proof; keep WAD conditional on its own proven procedure.
4. Fix effective FOV to 18°, add down-facing bag material/visual-spin rules, and include PIP depth/background/reset/restoration in §2.5. Define exporter/binary formats and tolerant host/PPC parity.
5. Expand §5.1 to the actual deterministic harness and fixtures in G1/G3/G4. Explicitly cover physical versus input KOBE power, sticky/stance/wind, collision transitions, score-once and AI choice parity.
6. Revise §3's eight-bag assumption for practice, keep 60 fps as the required acceptance target, and remove unmeasured Broadway timing promises. Add actual profiling gates for both targets.
7. Define wind defaults, practice CLEAR BOARD, one save path/serialization contract, player profile ownership, short prompts/glyphs and three-word legacy-callout exceptions. Reconcile static/animated NPC release scope.

### IMPL_PLAN.md

1. Make P0 deliver a reproducible complete disc package, valid WPAD acquisition/reconnect handling, timestamp-quality diagnostics, Android virtual-SD setup and the actual host/tool dependencies. Mark all make targets as new deliverables.
2. Rewrite Track A and P1 acceptance around independently verified speed versus radius/grip changes, forward/backward false positives, shake/clip/gap cases, and plain-remotes on both targets. Make failure block advancement until the required path works; fix the 40%-speed-drop and ≥16 ms definitions.
3. Replace Track C's hook-only promise with the read-only deterministic source harness and full fixtures. Use source hashes and explicit RNG/time control; test human power remapping separately from raw launch.
4. In P2 add reachable CLEAR BOARD, practice load/retention policy, exact scoring/settle fixtures, wind policy and cancellation/remote-ownership behavior. In P3 fix FOV, PIP clear behavior and the invalid alternate-frame fallback.
5. In P4 compare deterministic AI choices, measure resumable work on devices and specify save persistence/recovery. Add a Retro Kinhank checkpoint for tournament AI/save behavior before P6.
6. In P5/P6 align copy exceptions and glyph coverage, settle animation scope, require full-speed performance, and define a release-bundle target producing HBC ZIP/ISO/ini/instructions/checksums. Verify the identical DOL and pinned Android path, rather than relying on current-desktop assumptions.

### REPO_LAYOUT.md

1. Keep it marked PROPOSAL ONLY. Correct the current ignore-file facts and distinguish cached Git refs from current GitHub state. Preserve all legacy tags and the no-force/no-history-rewrite policy; require a history-preserving PR 0 merge.
2. Add release-source reconciliation as a dependency of any new arcade publication. Delete the contradictory `build:linux` release step until it consumes the validated snapshot/current sources. Validate the complete Electron package and Linux support files, not just stale `dist/site` equality.
3. Replace a single shared-equality schema with verified common fields plus explicit web/arcade/Wii overrides. Pin arcade 1.0.3 oracle data and define exact drift approvals. Coordinate the shared PR dependency with P0.
4. Rewrite build/CI examples with explicit working directories, pinned tooling/browser installs, server lifecycle, accurate test manifests and outputs. Repair/isolate historical Python game-root assumptions. Preserve isolation-test map keys and root config coverage.
5. Include all actual tool/oracle/lockfile/workflow dependencies in path triggers and explicit platform-tag triggers. Make required-check behavior and release-bundle targets concrete.
6. Choose and preview-test one Vercel serving topology with deployment exclusions and setting-aware rollback. Keep per-platform changelogs and future tag scheme, add tag/version checks and the exact legacy-tag mapping above. This review does not authorize or perform that migration.

### WIIMOTE_ARCADE_MAPPING.md

1. Keep the accurate 1.0.3 cabinet baseline: 6048 counts/sec, 80 ms window, ≥16 ms and ≥8 counts, peak retention, speed power, optional backswing, negative spin/right curl, one action per press. Make inferred physical button positions visibly distinct from live-confirmed ones if repeated as hardware facts.
2. Change the research table's “Taken” conclusions to distinguish sensor/API facts, control precedents and unvalidated estimator assumptions. Wii Sports control grammar does not validate the new power equation or direction classifier.
3. Rewrite §§3–5 with the same experimental detector contract as SPEC. Correct the energy-versus-speed drop, clipping headroom and sample-time errors; specify body/world axes and conservative cancellation. Give all gameplay-changing buttons consistent cancel/ignore behavior.
4. Expand §7 tests to radius/grip changes, backswing braking, pre-B motion, shake, packet gaps, clipping in rejected phases, surface/height changes while armed, both player slots and disconnect/reconnect. Use reproducible trace metadata and separate real speed classification from the estimator's own output.
5. Keep short player copy, IR aim fallback and curl lock before motion. Document stable roll/IR handling, no retarget while armed, handedness sign tests and explicit pass-and-play profile/ownership behavior. Require plain Wiimotes to pass before optional MotionPlus refinement is considered complete.

The native port can retain this game's physics, spin/curl, scoring and tournament. The next useful step is to correct these contracts and prove P0/P1, with arcade release reconciliation kept explicit and separate from a folder migration.
