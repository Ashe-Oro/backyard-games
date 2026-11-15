# Backyard Cornhole — Wii Port Implementation Plan

Status: draft for Ashe's review · 2026-11-15 (rev 2: confirmed Kinhank facts)
Spec: [SPEC.md](SPEC.md) · Input research: [WIIMOTE_ARCADE_MAPPING.md](WIIMOTE_ARCADE_MAPPING.md) · Repo: [REPO_LAYOUT.md](REPO_LAYOUT.md)
Source game: 1.0.3 snapshot at `arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/`

Targets (SPEC §0): **(1)** real softmodded Wii via HBC (`boot.dol`); **(2)** the same build in **Dolphin Android 5.0-19870 on the Kinhank Super Console X5 Pro** (Android 12, Mali, Vulkan 1×), real Wiimotes via a **DolphinBar in Mode 4**, packaged as an ISO with game ID `BYCEAO` + `GameSettings/BYCEAO.ini`; **(3)** Dolphin desktop/Android for the daily dev loop. The arcade cabinet is **not** a Wii test target.

Ground rules for every phase:
- Port from the **1.0.3 snapshot only**. The top-level `arcade/trackball.mjs` and `arcade/controller.js` are the retired distance-power code.
- **Power = peak swing speed over an 80 ms window. Never distance.** Every phase that touches input re-runs the swing tests (slow-long stays soft, short-fast can be full).
- One game binary: the `boot.dol` for HBC is byte-identical to the `sys/main.dol` inside the Dolphin ISO. Nothing ships that only works in one place.
- **NTSC / 60 Hz only** (PAL runs slow in the Kinhank's Dolphin setup).
- No existing game source is edited. New work lives in `wii/` ([REPO_LAYOUT.md](REPO_LAYOUT.md)).
- Developers never touch the Kinhank (no ADB from the dev box). **Retro installs and tests** the ISO + ini there, at the checkpoints marked 🟢K below.
- Player-facing text follows SPEC §6.

---

## Overview

| Phase | Name | Proves / delivers | Size |
|---|---|---|---|
| **P0** | Toolchain + "hello Wiimote" + packaging | Build pipeline, HBC boot, Dolphin boot, **ISO + game ID + ini**, full-rate Wiimote samples | S |
| **P1** | **Toss Lab — first milestone** | One board, one bag, a real swing-speed toss with ported physics, on Dolphin, a real Wii **and the Kinhank** | M |
| P2 | Match loop — **Dolphin-playable game** | 8 bags, collisions, scoring, Practice / 2 Player / casual CPU, HUD basics | M |
| P3 | The Yard | Baked backyard scene, lighting, fog, blob shadows, dog, PIP; 60 fps on Wii, full speed on the Kinhank | L |
| P4 | Tournament + saves | 8 rivals, time-sliced CPU AI, career screens, SD saves | M |
| P5 | Menus, help, audio, feel pass | Complete UI, power setup, SFX, rumble, left-handed mode, final tuning | M |
| P6 | Party life + release | Animated NPCs, video polish, HBC package + Kinhank ISO/ini package, release checklists | M |

Sizes are relative (S ≈ a few focused days, M ≈ 1–2 weeks, L ≈ 2–3 weeks of one developer). They are not commitments.

Dolphin playability arrives at **P1** (a single toss) and **P2** (a full match). Wii hardware checks start at P0. **Kinhank checkpoints (🟢K, done by Retro):** P0 (boot + real Wiimotes picked up), P1 (Toss Lab feel + full speed), P2 (full 2-remote match), P3 (full speed with the whole yard), P4 (tournament AI windups + career save), P6 (release candidate).

---

## Dependencies

```
            ┌──────────── Track C: physics core (host only) ───────────┐
            │                                                           ▼
P0 ──┬──► Track A: gesture lab (Wii/Dolphin) ───────────────────────► P1 Toss Lab ──► P2 Match ──┬──► P3 Yard ──┐
     └──► Track B: GX render + scene exporter ──────────────────────►                           │              ├──► P5 ──► P6
                                                                                                └──► P4 Tourn. ┘
🟢K Kinhank checkpoints hang off P0, P1, P2, P3, P4 and P6. Each needs only the ISO + ini from that phase plus Retro's time.
```

| Phase | Hard dependencies | Can run in parallel with |
|---|---|---|
| P0 | none | Track C |
| Track A (gesture lab) | P0 | B, C |
| Track B (GX render + exporter) | P0 (render); exporter needs nothing | A, C |
| Track C (physics core + golden tests) | none (host only) | P0, A, B |
| P1 | A + B + C | — |
| P2 | P1 | Track B work on yard export |
| P3 | P2 (match loop to stress-test), Track B exporter | P4 |
| P4 | P2 | P3 |
| P5 | P3 + P4 | — |
| P6 | P5 | — |

---

## Things to prototype independently (before or during P0)

These three tracks are separable and merge at P1.

### Track A — Wiimote toss gesture lab (a `.dol`; needs P0)
A bare-bones app with no 3D. It shows:
- live `|gforce|`, `e`, the 80 ms windowed `ē`, `v`, peak, and power bar (using the 1.0.3 color ramp)
- raw-axis saturation flags, `data_present`/`err`, reports/sec, dropped-event deltas, and estimated-timestamp gaps
- the detected phase and lobe markers for **each classifier candidate C1–C3** in parallel (SPEC §4.4), so one recorded swing evaluates all of them
- stability-check state at B press, calm roll, and the resulting curl arrow; IR reticle and validity
- **A** writes the last swing as a trace with the SPEC §4.4 **schema** (header: detector version, tunables, build, target, channel, slot, extension; rows: drain time, event index, estimated time + method, `gforce`, raw accel + saturation, `data_present`, `err`, dropped delta, IR, B, phase, `ē`, `v`) to `sd:/apps/backyardcornhole/data/swing-traces/`

**Independent speed labels** (the estimator can't validate itself): each P1 validation swing is filmed at ≥ 120 fps (a phone in slow-mo is enough) with a visible marker on the hand or remote, next to a scale reference. A small offline script measures peak hand speed and joins it to the trace by swing number. A MotionPlus remote can serve as a *secondary* lab reference only.

Done when: traces from Dolphin (real Wiimote via DolphinBar), the real Wii and the Kinhank are saved with the full schema, and `swing.c` replays them on the host to the same power values and classifications.

### Track B — GX bag/board render (a `.dol`) + scene exporter (Node)
- `tools/export-scene.mjs`: imports `three` from `arcade/node_modules` (0.170.0, pinned by `arcade/package-lock.json`), installs a tiny fake `document.createElement('canvas')` whose 2D context supports only `fillStyle`/`fillRect` (all that 1.0.3 textures use; Codex's review confirmed the backyard and party builders construct headlessly this way). It then builds the board (`makeBoard`/`boardTexture`), bag (`bagGeo`/`bagTexture`) and, later, `createBackyard()` and `createParty()`. The board/bag builders are inline in `index.html`, so slice them from the pinned snapshot by source range, the same way the oracle harness does, rather than editing the game. Portraits use a separate small SVG-rect path decoder. Output follows the **SPEC §2.9 manifest and big-endian binary format** (repeat/offset, material groups, winding, alpha/depth/fog flags, dynamic pivots), with snapshot SHA-256s in the header.
- Wii side: load `scene.bin` with explicit readers (a decode test runs on the Wii and in Dolphin), build display lists, draw board + bag + ground + sky color with the 1.0.3 camera at the **effective 18° FOV**, `(0,3.6,8)` → `(0,0.5,-8.7)`, fog 26–75, nearest-filtered textures, and the bag down-face material rule.

Done when: a side-by-side of the Wii frame and **fresh 1.0.3 screenshots at the same aspect ratio** (4:3 and 16:9; the older preview stills predate 1.0.1) shows the board, hole, sunburst art and bag framing match (scenery excluded), at 60 fps on hardware.

### Track C — physics core port (host only, no Wii needed)
- `source/core/board.c` (board transform, `HOLE_WORLD`, `G_LOCAL`), `physics.c` (`newBody`, `launch`, `step`, `collide`), `spin.c` (`landingCurl`, `slideVelocity`), `wind.c` (`roundWind` with an injectable RNG), `rules.c` (`lie`, `scoreInning`, knocked-in/off detection), power mapping (`physicalPower`, `calibratedWindow` incl. the KOBE remap).
- `tools/oracle/` (**read-only deterministic harness**, SPEC §5.1.1): slices the pinned snapshot's `index.html` (constants, board transform, `newBody`/`launch`/`step`/`collide`, `lie`/`scoreInning`, `calibratedWindow`/`physicalPower`, `planCareerShot`) and runs it under `node:vm` with `three@0.170.0`, following `arcade/cabinet-snapshots/1.0.3/tests/spin.test.mjs`. It adds fixed-step stepping in the original loop order, state import/export, full control of team/side/arc/stance/aim/drift/spin/wind and input-vs-physical power, a seeded RNG replacing `Math.random`, and a manual clock. **No game file is edited.** `window.__game` hooks (`_probe`, `win`) remain only as a cross-check that the harness agrees with the live page.
- `tools/golden.mjs` writes `tests/golden/*.json` from the harness: the six arc × surface combinations for both teams and several stances, wind on/off, and all transition fixtures in SPEC §5.1.1 (front edge, grass-first removal, side/back fall-off, swish vs slide-in, hanger, rider contact, knock-in + knock-off, wash, crossing 21, score-once timing). Each file records source SHA-256s, initial state, timestep and the expected transition sequence.
- `make host-test` (**new target**, host gcc/clang, not the PPC cross compiler): the C core reproduces the golden data (identical state classification and transitions; positions within 1 mm; WIN bounds within one 0.0025 step).

Done when: all golden tests pass, the harness matches the live page's `_probe`/`win` on their narrow domain, and `tools/bake-win.mjs` emits a big-endian `win.bin` (SPEC §2.9) matching the oracle's WIN within tolerance.

---

## P0 — Toolchain + "hello Wiimote"

**Goal:** prove we can build, package, boot and read the Wiimote at full rate on the Wii, in desktop Dolphin, and (via Retro) on the Kinhank.

All `make` targets named here are **new deliverables** of this plan, not existing scripts.

Tasks
1. `wii/docker/`: pinned `devkitpro/devkitppc` image plus pinned WIT and a **host** C compiler, Node 20+ and `npm ci` in `arcade/` (for `three` and the oracle). `make` builds `boot.elf` → `boot.dol`. Record the libogc version (target v3.2.x).
2. A minimal app: GX init, clear to the sky color `#c4d3a7`, draw one textured quad, 8×8 bitmap text, frame-time counter.
3. WPAD acquisition per SPEC §4.4: `WPAD_Init`, `WPAD_SetDataFormat(…BTNS_ACC_IR)`, `WPAD_SetVRes(640,480)`, persistent per-channel `WPAD_SetEventBufs(chan, buf, 64)` with return codes checked, one owner draining `WPAD_ReadEvent` each frame, **re-registration on reconnect**, `err`/`data_present` gating. Show reports/sec, dropped-event deltas, estimated-timestamp gaps, buttons, IR dot, calibrated **`gforce`**, and raw-accel saturation flags. On-screen axis readout confirms the sign conventions. Try `WPAD_SetMotionPlus(chan,1)` and report detection. If buffering misbehaves on any target, choose and document an alternative acquisition path here (`WPAD_ReadPending` drains the same queue, so it doesn't count).
4. HOME → return to HBC (`exit(0)` / `SYS_ResetSystem` path as appropriate for HBC).
5. `hbc/apps/backyardcornhole/` with `meta.xml` and a placeholder 128×48 `icon.png`.
6. Dolphin: boot via File → Open; also install into Dolphin's virtual SD `apps/` and launch through HBC in Dolphin's NAND.
7. Real Wiimote in Dolphin: a DolphinBar in Mode 4 (same as the Kinhank), which also supplies IR.
8. **Video: NTSC/60 Hz only.** `TVNtsc480Prog` when progressive output is actually available, else `TVNtsc480IntDf`. No EURGB60/PAL branch. A PAL console setting must not select 50 Hz.
9. **Dolphin package (SPEC §1.5):** `make iso` builds the **complete** extracted tree in `wii/dolphin/iso-root/`: generated `sys/boot.bin` (ID `BYCEAO`, USA region, title), a redistributable `sys/apploader.img` with `PROVENANCE.md` (source, license; no retail apploader), `sys/main.dol` = `boot.dol`, and `files/`. It then composes `BackyardCornhole-BYCEAO.iso` with the pinned WIT version and exact flags, next to `wii/dolphin/GameSettings/BYCEAO.ini` (`[Controls] WiimoteSource0 = 2`, `WiimoteSource1 = 2`). Check `BYCEAO` against GameTDB for collisions. WAD isn't built unless the ISO fails (then: define its title ID and settings filename, and prove it separately).
10. `wii/dolphin/INSTALL-KINHANK.md` for Retro: `adb push` the ISO to `/storage/3050BAA350BA6EEA0/Roms/wii/`, the ini into this Dolphin build's `GameSettings/` (path to be recorded by Retro), DolphinBar Mode 4, the bare-DOL + global "Real Wii Remote" fallback, and how to enable the virtual SD and copy traces off (filled in from Retro's P0 findings).
11. `make host-test` skeleton (host compiler), and the save/trace path `sd:/apps/backyardcornhole/data/` created on first run.

Acceptance
- [ ] Clean build from the Docker image on a fresh checkout, using one command (`make && make iso`).
- [ ] ISO proof: Dolphin shows ID `BYCEAO` and region USA; the ISO boots; the SHA-256 of the `main.dol` extracted from the ISO (`wit extract`) equals `boot.dol`; `PROVENANCE.md` covers every bundled disc component.
- [ ] Bare `boot.dol` + global Real-Wiimote fallback also works (kept usable while the ISO is proven).
- [ ] NTSC check: with the console (or Dolphin) set to a PAL region/50 Hz preference, the game still outputs NTSC 60 Hz.
- [ ] Boots in Dolphin (emulated Wiimote): IR dot follows the mouse, buttons register, accel changes with Motion Simulation.
- [ ] Boots in Dolphin with a **real** Wiimote via DolphinBar Mode 4: IR dot tracks, reports/sec shown.
- [ ] Desktop Dolphin shows the ISO's game ID as `BYCEAO`, and the per-game ini switches Wii Remote 1 to Real with no global change.
- [ ] Boots from HBC on the real Wii; HOME returns to HBC.
- [ ] 🟢K Retro: the ISO boots on the Kinhank (Dolphin Android 5.0-19870, Vulkan 1×), shows the `BYCEAO` ID, and real Wiimotes via DolphinBar Mode 4 drive the IR dot and buttons. If the ini isn't applied, the global fallback is tried and the result recorded. Retro records the real `GameSettings/` path, confirms the virtual SD is writable (a test file appears under `data/`), and documents trace retrieval.
- [ ] Measured report rate, dropped-event counts and timestamp-gap statistics recorded for: Dolphin emulated, Dolphin real (DolphinBar), Wii hardware, Kinhank. Disconnect + reconnect re-registers buffers with no stale events. If acquisition misbehaves anywhere, the alternative path is chosen and documented **before P1**.
- [ ] Profiling hooks are in: frame time on the Wii, and Dolphin's speed % recorded by Retro on the Kinhank. All later timing budgets are set from these numbers, not assumed.

---

## P1 — Toss Lab (first milestone: proves the path)

**Goal:** the smallest thing that proves the whole stack. One player, one bag, one board on a flat green ground with sky and fog. Swing the Wiimote → the bag flies with the ported 1.0.3 physics → lands, slides, curls → the game calls the result. Not a game yet: no turns, no score, no CPU, no scenery.

Includes
- Track C core linked into the Wii build (240 Hz fixed step, double precision).
- Track B board + bag rendering, held bag at the release point, flight with flat spin (`AIR_SPIN_RATE`), hole shrink, board dust and hole confetti.
- Track A detector (SPEC §4.4 candidate) driving `launch()`: power from the peak windowed speed proxy, aim from IR and curl from calm roll (both lock at a **stable** B press), auto-release at `v ≤ 0.6·peak_v` with follow-through confirmation; action-legality rules (armed inputs ignored).
- Throw height (D-pad ↑/↓) and SLICK/STICKY (A) to exercise all six WIN combinations; stance step (D-pad ←/→); reset bag (2).
- Power gauge with the 1.0.3 color ramp and the 3 s last-shot linger; aim arrow.
- Callouts: CORNHOLE! / HANGER! / ON THE BOARD / GROUNDER / OFF THE BOARD / MISS.
- Each throw auto-clears after 2 s (practice-style). Dev overlay toggle. Swing-trace capture still available.

Acceptance — must pass in **Dolphin (real Wiimote)**, on the **real Wii** and on the **Kinhank (🟢K, Retro)**; emulated-Wiimote Dolphin must at least complete throws.
**Speed-proxy validation (plain Wii Remotes; MotionPlus off or absent):**
- [ ] **Speed, not distance:** within the supported gesture, 10 slow, long, wide swings average power < 0.4 and 10 short, quick swings average > 0.7 (default threshold). Measured against **independent video speed labels**: pairwise ordering of filmed peak hand speed vs estimated speed agrees ≥ 90% across ≥ 30 labeled swings from ≥ 3 players.
- [ ] **Envelope documented:** the same filmed speed with deliberately shorter vs longer arm radius, and with the remote held at different grip angles, is reported as the estimate's spread. The supported gesture and on-screen coaching art are defined from it. Swings outside the envelope (wrist flicks, shakes) come out soft or are rejected, never full power.
- [ ] **Zero false throws** on the adversarial trace set: backswing braking, backswing-only release at the back, motion started before B (arming refused), no-backswing toss (allowed), shake, wrist-only rotation, pointer loss mid-gesture, packet gap (cancels), clipping in a rejected phase (ignored), B tap.
- [ ] Release and window math as specified: the auto-release fires at a 40% **speed** drop; window validity uses elapsed span ≥ 16 ms; replays on the host reproduce on-device decisions.
- [ ] Clipping: the per-remote rail is measured; full-power default and calibration cap sit below it with margin; saturation never validates a toss.

**Gameplay checks:**
- [ ] Holding B through the swing auto-releases; letting go with no valid forward lobe cancels quietly.
- [ ] Pointing left/right of the hole moves the landing left/right; aim doesn't change during the swing; −/+ and buttons are ignored while armed.
- [ ] Twist right + SLICK → visible right curl after landing; twist left → left curl; neutral → straight. B during unstable motion shows `HOLD STILL` and doesn't arm.
- [ ] All three heights and both surfaces match arcade 1.0.3: the Wii build's core reproduces the oracle fixtures for those shots (state, transitions, landing).
- [ ] A skilled tester lands CORNHOLE! at least 1 in 5 tosses on NORMAL/SLICK after 5 minutes of practice. This is a feel sanity check, not a balance target.
- [ ] Hardware: steady 60 fps with the dev overlay open; no dropped Wiimote events during throws.
- [ ] 🟢K Kinhank: full speed (Dolphin's speed indicator at 100%) through 20 tosses; the speed/distance check repeated with a real Wiimote via DolphinBar; swing traces saved from Dolphin's virtual SD for comparison with the Wii's.
- [ ] Power default and calibration (`measured / 0.7`, capped at the clip ceiling) chosen from recorded traces (Wii + Kinhank); values and the chosen classifier (C1–C3) written into SPEC §4.4. If the Wii and the Kinhank need noticeably different thresholds, record it. The per-slot "SET POWER" step (P5) absorbs the difference.
- [ ] Benchmarks recorded on both targets: one worst-case single-bag simulation, a full `planCareerShot` worst case, and WIN recompute time. These set the AI slice budget.

**Exit gate (blocking):** if the plain-remote path fails the validation or false-throw criteria, **P2 doesn't start**. Revise the estimator, the classifier or the supported gesture, and repeat P1. Tuning thresholds alone, or adding optional MotionPlus, doesn't pass this gate.

---

## P2 — Match loop (Dolphin-playable game)

**Goal:** a complete, playable cornhole match in Dolphin with a Wiimote, on a plain scene.

Tasks
- Port `collide()` (blocks, push-ins, knock-offs, rider lift, re-waking), 8-bag rounds, `lie()` live recount, `scoreInning()` (cancellation, first to 21, last scorer throws first, scored once after settle), `afterThrow()` knocked-in/off callouts, coin toss, `M.phase` flow, settle detection. Loop order and per-bag flags as in SPEC §2.1.
- Modes: PRACTICE (stats: IN / ON BOARD / % HIT / THROWS; **CLEAR BOARD in the practice pause menu**, separate from RESET BAG; unlimited retained bags as in 1.0.3), 2 PLAYER (slot 2 on Remote 2 or pass-and-play), and a casual vs-CPU match using the `nextTurn` CPU branch (handy for testing, even though the arcade menu doesn't list it).
- **Wind policy per SPEC §2.3:** calm by default in Practice and 2 Player; the WIND setting (CALM/ON) switches to `roundWind` non-practice wind, regenerated each round with the side flipped, and stays on for later practice throws. Wind panel arrow + MPH, basic pennant.
- **Input rules per SPEC §4.3:** action-legality table, disconnect → pause + discard gesture + slot-correct `CONNECT REMOTE n` (with `A: USE REMOTE 1` for slot 2), resume only after reconnect/reassign + B up, profiles per slot.
- HUD v1: scoreboard (RED/BLUE, TOTAL/ROUND, pips, ROUND n, TO 21, round edge "RED +2"/EVEN), turn banner, round summary panel (`… IN · … ON`, cancellation line), FINAL screen.
- Load `win.bin`; debug builds recompute and compare within tolerance.
- Pause on HOME: RESUME · CLEAR BOARD (practice) · MAIN MENU · QUIT.

Acceptance
- [ ] Full 2 PLAYER match to 21 in Dolphin with two real Wiimotes, and with one Wiimote passed between players.
- [ ] Oracle transition fixtures (SPEC §5.1.1) reproduce on the Wii build's core: front edge, grass-first removal, side/back fall-off, swish vs slide-in, hanger, rider contact, knock-in + knock-off, re-woken bag, with identical callouts and throws consumed.
- [ ] Scoring fixtures: 6–4 resolves once to +2 after all eight bags settle; a wash keeps throw order; a net score crossing 21 ends the match; moving slides don't count in the live round total.
- [ ] Wind: default calm in 2 Player and Practice; with WIND ON it flips sides each round and matches the oracle's flight for the same seed.
- [ ] Disconnect mid-gesture (each slot): pause, no throw, correct prompt, clean resume.
- [ ] **Practice load:** 48 retained bags hold 60 fps on the Wii and full speed on the Kinhank (🟢K); memory stays flat. If not, record the limit and take the retention question to Ashe (SPEC §9 Q9a). Nothing is capped silently.
- [ ] No soft-lock across 20 consecutive rounds (scripted auto-throw soak in Dolphin), plus a 200-throw practice soak with periodic CLEAR BOARD.
- [ ] Hardware: a full match at 60 fps on the plain scene.
- [ ] 🟢K Kinhank: a full 2 PLAYER match with two real Wiimotes (`WiimoteSource0/1 = 2`) at full speed.

---

## P3 — The Yard

**Goal:** the 1.0.3 look within **both** budgets: Wii hardware and Dolphin Android/Mali at 1× (SPEC §3, §3.4).

Tasks
- Exporter: `createBackyard(scene, {farZ:-13, nearZ:4, propZ:-7})` and `createParty(scene)` (keg, table, people, chess set, cacti), hay bales, smoke puffs, string lanterns. Codex's headless count before culling: 258 mesh nodes, 746 instances, 11,114 triangles, 72 material objects (board, bags, dog and HUD extra). Static parts merge by material; **animated parts keep object identity and pivots** in the manifest for P6. Hidden-face removal only on static merged geometry (SPEC §2.9).
- Lighting bake into vertex colors (hemisphere `#d6ecff`/`#7a8f3a` + sun `#ffe0b0` from `(-7,12,5)` toward `(0,0,-5)`), linear lighting written as sRGB, plus baked contact shadows. Tune against **fresh 1.0.3 screenshots** at 4:3 and 16:9 (the older preview stills predate 1.0.1 and still show the removed cooler). Collapse color-only material differences into vertex colors to stay ≤ 12 TEV/blend configurations.
- Sky sphere (vertex colors, inside-facing, no fog, no depth write), clouds, fog.
- Dynamic: dog + `DogCelebration` (hops/spin/bow) and head tracking, blob shadows for bags/dog/NPCs, pennant animation (`animatePennant`), wind streaks (24 lines), confetti/dust/trail pools, flight trail.
- Board View PIP per SPEC §2.5: board-view camera (fov 30, near .05, far 30, `(0,2.5,BOARD_Z+.75)` → `(0,.16,BOARD_Z−.55)`), viewport + scissor, **inset color and depth reset**, culled draw list, then **restore** projection/viewport/scissor/depth/blend/fog before the HUD. Drawn every frame.
- Dev overlay: triangles, draw calls, GPU/CPU ms.

Acceptance
- [ ] Hardware: ≥ 60 fps steady during the heaviest moment (CPU windup + flight + confetti + PIP) at 480p, measured over a full match. If not, apply the SPEC §3.3 scenery cut list in order and record what was cut. There's no 30 fps or PIP-skip fallback.
- [ ] PIP correctness: main-view geometry never shows through the inset, and inset geometry is never hidden by main-view depth; HUD state is intact after the PIP pass.
- [ ] Main view ≤ 25k triangles, ≤ 300 submissions (the Dolphin-friendly limit, SPEC §3.4); PIP ≤ 3k triangles; ≤ 12 distinct TEV/blend configurations; no EFB peeks or copies besides the one XFB copy.
- [ ] 🟢K Kinhank: full speed through a whole match with the full yard at Vulkan 1×, including the heaviest moment, with no repeated shader-compile stutter after the first minute. If not, apply the SPEC §3.3 cut list (it applies to both targets) and record what was cut.
- [ ] Ashe signs off on a side-by-side with fresh 1.0.3 screenshots ("reads as the same backyard").
- [ ] Desktop Dolphin renders identically (no missing textures or fog issues) with both the OpenGL and Vulkan backends.

---

## P4 — Tournament + saves

**Goal:** the full 8-rival tournament with persistent progress.

Tasks
- Port the `roster` (names, nicknames, homes, records, taunts, styles, colors, circuits), `circuits`, career map / intro / result / champion screens, NEW CAREER (with confirmation).
- Port `planCareerShot()` **exactly as the single-bag, `step()`-only predictor it is** (no collision-aware improvements): deck-based arc/side/target/goal choice, coarse → fine → fallback grids, hole-shot averaging, `CPU_RATES` pool pick, closest-24 selection (Ted: whole pool), Sierra Spin `curl = .65`. It runs as a **step-granular resumable job** with a per-frame budget set from the P1 benchmarks; the windup holds if unfinished.
- Rival bag color per opponent (`TEAM_MATS[1]` recolor) and baked 16×16 portraits (SVG-rect decoder).
- Circuit wind tiers (4–6, 6–9, 9–12, 12–15 mph); tournament wind unaffected by the WIND setting.
- `save.dat` per SPEC §2.8: one path `sd:/apps/backyardcornhole/data/`, explicit big-endian serialization, CRC, crash-safe tmp/rename/bak writes, field validation, career semantics (`0 ≤ fight ≤ 8`, `wins ≥ fight`, saved only on a win), per-slot profiles, WIND, help tucked. Missing or read-only SD → "Progress won't save on this device."

Acceptance
- [ ] **Decision parity:** for every rival, on empty and crowded fixture boards, with fixed seeds, the C port selects the same arc, side, spin, goal, pool, power and aim (within tolerance) and consumes the same RNG draws as the oracle (SPEC §5.1.1).
- [ ] Secondary statistical check: pool-selection frequencies over ≥ 1,000 seeded empty-board throws per rival match `CPU_RATES` within a binomial 99% interval. Live in/on outcomes on crowded boards aren't expected to match `CPU_RATES`, because collisions alter them.
- [ ] Worst-case CPU search (fallback grid, crowded board) never drops a frame on the Wii; the windup length is measured and recorded for every rival on both targets.
- [ ] Beat all 8 rivals, power off, relaunch: progress and the crown persist on the real Wii's SD and in desktop Dolphin's virtual SD.
- [ ] Save robustness: corrupt `save.dat` → loads `save.bak`; both corrupt → fresh defaults, no crash; power loss during a write (simulated by killing the app between steps) never leaves both files invalid.
- [ ] 🟢K **Kinhank P4 checkpoint (Retro):** a tournament match against a late rival (crowded board, fallback grid) at full speed with no hitch during CPU windups; the career save persists across a Dolphin restart on the virtual SD.

---

## P5 — Menus, help, audio, feel pass

**Goal:** a complete, friendly product loop.

Tasks
- Main menu: PRACTICE · 2 PLAYER · TOURNAMENT · SETTINGS. Navigation by pointer + A, or D-pad + A; B backs out.
- Settings (short labels): SET POWER per player slot (TOSS HARD → POWER SET!, capped at the clip ceiling), CURL (strength), HAND (RIGHT/LEFT), WIND (CALM/ON: Practice and 2 Player, SPEC §2.3), RESET.
- Help legend (button 1, remembered), first-throw hint "HOLD B · SWING · LET GO", "STRAP ON!" boot card, `HOLD STILL`, slot-correct `CONNECT REMOTE 1/2`, low battery, no-pointer "AIM: − +".
- **Screen/action matrix** documented and tested: every screen (menus, pause, practice pause with CLEAR BOARD, round summary/advance, career map/intro/result/champion, NEW CAREER confirm, reconnect prompt) × every button.
- Glyph-coverage build check (SPEC §2.6 repertoire; typographic characters mapped to ASCII) and 480i readability at ≥ 2× text.
- SFX via ASND (CC0 set from `arcade/AUDIO.md` candidates, licenses saved alongside), ducking on cheers; rumble on release/board/hole.
- Feel pass: final swing tunables from P1–P4 traces, auto-release timing, held-bag dip/arc animation.
- Copy review against SPEC §6: no technical words on any player screen.

Acceptance
- [ ] A new player (not a developer) can start a match and land a bag within 60 seconds without being told the controls.
- [ ] Left-handed mode mirrors curl correctly (twist toward the body vs away tested both hands).
- [ ] Every new prompt is ≤ 4 words and every new callout ≤ 2. Exempt (verbatim from 1.0.3): established callouts such as ON THE BOARD / OFF THE BOARD / KNOCKED IN!, rival names and taunts, and the save warning. A scan of the string table finds none of these words: g, count, IR, sensor, accel, calibrat, Bluetooth, MotionPlus. Every string renders with the shipped glyph set.
- [ ] Audio plays on hardware (TV speakers) without crackle across a full tournament match.

---

## P6 — Party life + release

**Goal:** finish parity polish and ship to HBC and the Kinhank.

Tasks
- Port `party.mjs` animations (**required for release** unless Ashe descopes them, SPEC §9 Q9): keg pours (pump/tap/stream), the five scripted beer pong matches (`pongCatalog`, `pongTiming`, `buildPongScript`, `selectNext` non-repeat), chess opening with lifted pieces, cook flipping food and the dog burger toss. All of it pauses with the game clock and uses the pivots kept by the P3 export.
- Video: NTSC 480p/480i at 60 Hz only (no PAL/EURGB60), 16:9 anamorphic when the console is set to widescreen, safe-area margins for the HUD.
- Final `icon.png` (128×48, from `arcade/media/backyard-cornhole-cover.png` art), `meta.xml` with version/date, license files (font OFL, SFX CC0, apploader provenance; Three.js MIT is not shipped since no Three.js code is in the `.dol`).
- **`make release` (new target)** produces, in `wii/dist/`: `BackyardCornhole-Wii-HBC-vX.Y.Z.zip` (`apps/backyardcornhole/{boot.dol,meta.xml,icon.png}`), `BackyardCornhole-BYCEAO-vX.Y.Z.iso`, `BYCEAO.ini`, `INSTALL-KINHANK.md` (with Retro's recorded 5.0-19870 paths), licenses, and `SHA256SUMS`. It verifies that the `main.dol` extracted from the ISO is byte-identical to `boot.dol`, and that `VERSION` in the Makefile, `meta.xml`, the in-game footer, `wii/CHANGELOG.md` and the tag (`wii-vX.Y.Z`, REPO_LAYOUT.md) all agree.
- Release checklists (below).

Acceptance
- [ ] Wii hardware release checklist passes.
- [ ] 🟢K Kinhank release checklist passes (Retro).
- [ ] 2-hour soak on the real Wii (tournament + practice loops) with no crash, leak or frame-rate decay; memory high-water mark logged ≤ 16 MB budget.
- [ ] Same game binary passes the desktop Dolphin checklist.

---

## How to verify: Dolphin vs hardware

| Check | Dolphin desktop (emulated remote) | Dolphin desktop (real remote, DolphinBar) | **Kinhank — Dolphin Android (Retro)** | Real Wii |
|---|---|---|---|---|
| Boots | `.dol` directly / via HBC | ISO (`BYCEAO`) | ISO (`BYCEAO`) from `/storage/3050BAA350BA6EEA0/Roms/wii/` | ✔ HBC `boot.dol` |
| Per-game Real Wiimote ini | n/a | ✔ verify | ✔ verify (fallback: global source = Real) | n/a |
| Menus, flow, scoring, soak scripts | ✔ primary | ✔ | spot-check | spot-check |
| IR aim | Mouse "Point" | DolphinBar IR | DolphinBar IR | Sensor bar |
| Swing power feel | Smoke only (Motion Simulation "Swing"; DSU/SDL Motion Input as a middle ground) | ✔ primary for tuning | ✔ feel sign-off | ✔ final sign-off |
| Wrist-roll curl | "Tilt" mapping | ✔ | ✔ | ✔ |
| Report rate / dropped events | Record, don't trust | Record | Record | ✔ authoritative |
| Performance | ✗ not representative | ✗ | ✔ **full speed at Vulkan 1×** | ✔ frame time, the budget's source of truth |
| SD saves | Virtual SD (`sd.raw` or synced folder) | same | Dolphin Android virtual SD | Real SD |
| Video | NTSC only | NTSC only | NTSC only (PAL runs slow here) | 480p/480i, 4:3/16:9 |
| HOME → back to loader / menu | ✔ | ✔ | ✔ (back to Dolphin) | ✔ (back to HBC) |

Per-phase routine:
1. `make host-test` (core + swing trace replays) must pass.
2. Dolphin, emulated remote: scripted soak + manual flow pass.
3. Dolphin, real remote via DolphinBar: feel checks for anything touching input.
4. Real Wii via `wiiload` (or SD copy): frame time overlay, phase acceptance items, then a short manual play.
5. 🟢K checkpoints: hand Retro the ISO + ini + `INSTALL-KINHANK.md` + a short checklist. Retro reports speed, controls and any issues.
6. Attach to the phase notes: reports/sec, dropped events, frame-time max/avg, memory high-water, Kinhank speed result, and any tunables changed.

### Hardware release checklist (P6)
- [ ] Launch from HBC with a fresh SD (no `data/`): runs, creates its save on first progress.
- [ ] Launch with a read-only/missing SD: plays, shows the save warning.
- [ ] 1 remote and 2 remotes; remote disconnect + reconnect mid-turn for **each** slot (game pauses with the slot-correct `CONNECT REMOTE n`; the gesture is discarded; slot 2 can switch to Remote 1).
- [ ] PAL console setting → still NTSC 60 Hz output.
- [ ] Long practice session at the measured bag limit holds 60 fps.
- [ ] Remote without a sensor bar: −/+ aim works, the prompt shows.
- [ ] Wii Remote Plus and a plain Wii Remote both play (MotionPlus optional path never required).
- [ ] 480p component and 480i composite; 4:3 and 16:9 settings.
- [ ] HOME → QUIT returns to HBC; power button on the console shuts down cleanly.
- [ ] Full tournament completion and persistence.

### Kinhank release checklist (P6, Retro)
- [ ] `adb push` of the ISO to `/storage/3050BAA350BA6EEA0/Roms/wii/`; `BYCEAO.ini` in Dolphin's `GameSettings/`; the game shows in the list with ID `BYCEAO`.
- [ ] DolphinBar Mode 4: Remote 1 and Remote 2 come up as real remotes for this game only (another game still uses its own setting). The fallback (global source = Real) is documented as tested or not needed.
- [ ] Full speed at Vulkan 1× through a full tournament match, including the yard, PIP, confetti and CPU windups.
- [ ] Swing power, aim and curl feel match the Wii (same SET POWER flow works).
- [ ] Saves persist across Dolphin restarts (virtual SD).
- [ ] NTSC/60 Hz confirmed; no PAL setting needed.

---

## Suggested first week (concrete)

1. Day 1–2: P0 Docker build + hello-Wiimote `.dol` in Dolphin (emulated) and on HBC, plus `make iso` → complete `BYCEAO` ISO (boot.bin + provenance-recorded apploader) + ini. In parallel, start Track C: the read-only oracle harness slicing the 1.0.3 snapshot, then board transform + `launch`/`step` in C against its fixtures.
2. Day 3–4: real Wiimote in Dolphin via DolphinBar; hand the P0 ISO to Retro for the first 🟢K boot check; Track A gesture lab recording traces on Dolphin and hardware; Track B exporter for board + bag.
3. Day 5+: film labeled validation swings and record the adversarial trace set; merge into the P1 Toss Lab; run the blocking P1 gate on the Wii and (via Retro) the Kinhank before anything else is built on top.

---

## Decisions from review (Codex, 2026-11-15)

Source: [REVIEW_CODEX.md](REVIEW_CODEX.md). The full accept/reject list is in [SPEC.md §10](SPEC.md). Changes to this plan:

- **P0:** complete ISO tree with an apploader of recorded provenance, an ID/region/DOL-hash proof (I5); NTSC-only video and a PAL-setting check (I6); WPAD acquisition with gforce, reconnect re-registration, one queue owner and timestamp-gap diagnostics (I4); Retro records this Dolphin build's GameSettings path, virtual-SD writability and trace retrieval (G9); host compiler + Node deps in the image, and all `make` targets marked as new (R5); profiling hooks on both targets (G4).
- **Track A / P1:** the detector is a candidate; independent video speed labels, the radius/grip envelope, zero false throws on adversarial traces, speed-unit auto-release, the 16 ms elapsed-span window and clip-capped calibration are now blocking gates. Failing means revise and repeat P1; MotionPlus or threshold tuning doesn't pass it (I1–I3).
- **Track B:** effective 18° FOV, bag down-face rule, export manifest and big-endian binaries, fresh 1.0.3 screenshots (G5, G7).
- **Track C:** a read-only deterministic oracle harness replaces the hook-only golden plan, with transition fixtures (G1, G3).
- **P2:** CLEAR BOARD, the 48-bag practice benchmark with no silent cap, the arcade wind policy, action legality and disconnect rules (G2, G8, I7).
- **P3:** PIP inset clear/restore; the 30 Hz PIP and 30 fps fallbacks are removed (G6, G9).
- **P4:** fixed-seed AI decision parity is primary and rate statistics secondary; a single-bag predictor ported as-is; step-granular yielding; crash-safe saves; new Kinhank P4 checkpoint (G4, G9).
- **P5/P6:** legacy callouts exempt from the copy gate; glyph repertoire; screen/action matrix; animations required for release unless descoped; `make release` bundle with version/tag consistency checks (G9, R5, R7).
- **Not adopted:** nothing in this plan's scope was rejected outright. The two partial rejections (the remote-tag check, and README/CHANGELOG deployment) concern SPEC §10 and REPO_LAYOUT.
