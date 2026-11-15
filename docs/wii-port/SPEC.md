# Backyard Cornhole — Wii Port Spec

Status: draft for Ashe's review · 2026-11-15 (rev 2: confirmed Kinhank facts)
Source game: Backyard Cornhole Arcade **1.0.3** (`release/1.0.3`, commit `a07afaa`)
Companions: [IMPL_PLAN.md](IMPL_PLAN.md) · [WIIMOTE_ARCADE_MAPPING.md](WIIMOTE_ARCADE_MAPPING.md) · [REPO_LAYOUT.md](REPO_LAYOUT.md)

---

## 0. Scope and targets

| Priority | Target | What "done" means |
|---|---|---|
| **1 — Primary** | Real softmodded Nintendo Wii running unsigned code through the Homebrew Channel (HBC). Native libogc/devkitPPC + WPAD | `apps/backyardcornhole/boot.dol` launches from HBC, plays full matches at 60 fps (NTSC 480p/480i) and returns to HBC cleanly |
| **2 — Required** | **Kinhank Super Console X5 Pro** running **Dolphin Android 5.0-19870** (Vulkan, 1× native resolution) with real Wiimotes through a **MAYFLASH DolphinBar in Mode 4** (USB) | The **same game build**, packaged as an ISO (or WAD) with its own 6-character game ID and a shipped `GameSettings/<GAMEID>.ini`, runs full speed with real Wiimotes. Retro installs and tests it |
| 3 — Dev loop | Dolphin (desktop or Android) for day-to-day testing before hardware | Same binary boots; emulated Wiimote for flow, real Wiimote via DolphinBar for feel |

Confirmed device facts (from Retro, authoritative):
- The Kinhank Super Console X5 Pro is an **Android 12 TV box** (ARM, Mali GPU, exact SoC unknown). It is **not Linux**. It already runs Mario Kart Wii and Wii Sports at full speed in that Dolphin setup.
- The game is built **NTSC / 60 Hz only**. PAL runs slow on this Dolphin setup.
- Dolphin Android loads `.dol`, `.elf`, `.iso` and `.wbfs`. Builds are installed over ADB to `/storage/3050BAA350BA6EEA0/Roms/wii/`.

Out of scope: the existing Electron arcade cabinet build (packaging, launcher, trackball input, music playlist, cabinet setup). It is used **only** as the source of game logic. **The arcade cabinet is not a Wii-port test target**: no Wiimotes, and no Wii testing is planned around it. It stays the existing arcade/Electron platform (see [REPO_LAYOUT.md](REPO_LAYOUT.md)).

### 0.1 Which code is the real 1.0.3 game

The repo holds two generations of arcade code. Port **only** the 1.0.3 snapshot:

```
arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/
  cornhole/index.html     ← physics, collisions, scoring, match flow, CPU AI, rendering, HUD (1,133 lines, inline module)
  cornhole/spin.mjs       ← AIR_SPIN_RATE, CURL table, landingCurl(), slideVelocity()
  cornhole/controls.mjs   ← throwPower()/pullPower() (identity), roundWind()
  cornhole/wind-streaks.js
  arcade/trackball.mjs    ← speed-based power model (SPEED_WINDOW_MS = 80, powerSpeed 6048)
  arcade/controller.js    ← arcade menus, scoreboard, power gauge, legends, button routing
  arcade/party.mjs        ← background NPCs: keg, beer pong match scripts, chess, cook
  arcade/board-view.mjs   ← live board close-up (second camera)
  arcade/dog-celebration.mjs, arcade/legend.mjs
  yard-games/backyard.js  ← voxel scenery (fences, trees, lanterns, grill)
  yard-games/power-arrow.js ← power color ramp
```

**Do not port** top-level `arcade/trackball.mjs` / `arcade/controller.js`. They are older and still use the retired **distance-based** power (`FULL_POWER_COUNTS = 400`, `power = forwardY / 400`). That model breaks the standing rule. The snapshot `DEV-NOTES.txt` says the same thing: "Build tooling in arcade/ is older than this cabinet snapshot."

---

## 1. Recommended tech path

### 1.1 Recommendation

**Native Wii homebrew in C, using devkitPPC + libogc, raw GX for 3D, and a small custom 2D layer (or GRRLIB) for the HUD.** Build one `boot.dol`. It runs from HBC on the Wii, and the same DOL, wrapped in an ISO with a game ID, runs in Dolphin Android on the Kinhank (§1.4–1.5).

| Layer | Choice | Notes |
|---|---|---|
| Toolchain | devkitPro **devkitPPC** + **libogc** (latest is v3.2.1, Oct 2026), pinned in a Docker image (`devkitpro/devkitppc`) | Pin the exact tag. libogc's 2.x→3.x churn and the separate libogc2 fork are real risks (§8) |
| 3D | Raw **GX**: display lists, fixed-function TEV, hardware fog, 1 directional light + ambient | Three.js features map almost 1:1 onto what this game uses: Lambert, unlit, fog, nearest-filtered pixel textures |
| 2D HUD/text | Own ortho quad batcher + 8×8 bitmap font atlas. GRRLIB 4.6.x is acceptable if it builds against the pinned libogc | GRRLIB's 3D helpers are too limited for the scene; use it only for 2D or not at all |
| Input | libogc **WPAD**: `WPAD_SetDataFormat(chan, WPAD_FMT_BTNS_ACC_IR)`, `WPAD_SetVRes`, `WPAD_SetEventBufs` + `WPAD_ReadEvent` (every report, not one per frame), `WPAD_SetMotionPlus` (optional) | Header: `gc/wiiuse/wpad.h`. Details in §4 |
| Audio | libogc **ASND** (or aesnd) for short PCM/ADPCM SFX | The game ships no SFX today (§2.7) |
| Storage | libfat (`fatInitDefault`) → `sd:/apps/backyardcornhole/save.dat` | Replaces `localStorage` |
| Packaging | `.elf` → `elf2dol` → `boot.dol`, plus `meta.xml` and a 128×48 `icon.png` (HBC). For Dolphin/Kinhank, the same DOL is wrapped in an ISO with game ID `BYCEAO` plus `GameSettings/BYCEAO.ini` | §1.4–1.6 |
| Language | C11 for the platform-free **core** (physics, rules, CPU, swing detector); C for the Wii platform layer | The core also builds on the host with gcc/clang for golden tests against the JS game |

**Why this path:** it is the only path that meets **both** required targets with one game binary. The Wii can't run JavaScript or WebGL in any useful form. Dolphin runs Wii binaries, not Electron. The Kinhank is an Android box whose Wii story *is* Dolphin Android, so a real Wii build is the only thing it can play with Wiimotes. Once the game is a real `.dol`, Dolphin compatibility comes almost free.

### 1.2 Rejected alternatives

| Option | Verdict | Why |
|---|---|---|
| Keep Electron/Three.js and connect a Wiimote to a Linux/PC box over Bluetooth (Linux `hid-wiimote` exposes buttons, accelerometer, IR and MotionPlus as evdev devices; Chromium/Electron WebHID is also possible) | **Rejected.** | It never runs on a Wii, and it can't run in Dolphin: Dolphin runs Wii software, and this would be a native desktop app. It doesn't help the Kinhank either: that box is Android 12, not Linux, and its Wiimote path is Dolphin + DolphinBar |
| JS engine on Wii (QuickJS/Duktape) running the existing game code | Rejected | Three.js needs WebGL. A WebGL→GX shim is a bigger job than porting ~1,100 lines of game logic, and GC'd JS on a 729 MHz CPU with 88 MB is a poor fit |
| Wii Internet Channel / any browser | Rejected | No WebGL, no Wiimote motion access, Opera 9-era JS |
| Commercial engines with Wii export | Rejected | No legitimate homebrew export path |
| OpenGL-1.x-over-GX shims (community "OpenGX"-style layers) | Not recommended | Three.js isn't GL1 code, so nothing is reused. Raw GX display lists are simpler and faster for a static voxel scene |
| GRRLIB for everything | Rejected for 3D | Its 3D API is immediate-mode helpers (cubes, simple objects). It is fine for 2D |

### 1.3 Proposed source layout (new directory, nothing existing edited)

Location: `wii/` at the top level of the backyard-games monorepo, per [REPO_LAYOUT.md](REPO_LAYOUT.md) (a proposal; nothing is moved yet).

```
wii/
  Makefile                   devkitPro wii_rules template; also `make host-test`
  docker/                    pinned devkitpro/devkitppc image tag
  source/core/               platform-free C, host-testable
    board.c   physics.c   spin.c   rules.c   wind.c   cpu.c   swing.c   rng.c
  source/wii/                GX renderer, WPAD input, ASND audio, libfat saves, main loop
  tools/                     Node scripts (run with arcade/node_modules/three 0.170.0)
    export-scene.mjs         runs backyard.js / party.mjs / board+bag builders headless → static meshes + textures
    bake-win.mjs             builds the WIN power-window table from the C core (or the JS page) → win.bin
    golden.mjs               drives the 1.0.3 page's window.__game hooks → golden JSON vectors
  data/                      generated: scene.bin, *.tpl textures, font atlas, win.bin
  hbc/apps/backyardcornhole/ boot.dol (build output), meta.xml, icon.png
  dolphin/                   iso-root/sys/main.dol (copied at build), GameSettings/BYCEAO.ini, ADB install notes for Retro
  tests/                     host tests: core vs golden vectors
```

The scene exporter needs **no new native packages**. Every procedural texture in the 1.0.3 code uses only `ctx.fillStyle` and `ctx.fillRect` (`pixTex` in `index.html`, `pixelMap` in `backyard.js`; `party.mjs` uses solid colors). A 20-line fake 2D context that writes into a `Uint8Array` is enough. `three@0.170.0` is already in `arcade/node_modules` and builds geometry and `InstancedMesh` matrices in Node without WebGL.

### 1.4 HBC packaging

```
sd:/apps/backyardcornhole/
  boot.dol      (HBC prefers boot.elf if both exist; ship only boot.dol)
  icon.png      128×48 PNG, alpha allowed
  meta.xml
  data/         save.dat (+ save.bak), logs/, swing-traces/ (dev builds) — the ONLY write location (§2.8)
```

```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<app version="1">
  <name>Backyard Cornhole</name>
  <coder>Ashe</coder>
  <version>0.1.0</version>
  <release_date>20270101000000</release_date>
  <short_description>Toss bags. First to 21.</short_description>
  <long_description>Backyard cornhole for the Wii Remote. Swing to toss, point to aim, twist to curl. Practice, 2 Player, or take on eight tournament rivals.</long_description>
</app>
```

Assets are baked into the `.dol` (generated C arrays or a linked binary blob), so the app works even with an empty `data/` folder. Only saves and dev logs touch the SD card.

### 1.5 Dolphin / Kinhank packaging: ISO (or WAD) with a game ID

A bare `.dol` has **no game ID**, so Dolphin can't attach per-game settings to it. Dolphin Android needs per-game settings to pick **Real Wii Remote** for this game. So the Dolphin build ships as:

| Artifact | How | Notes |
|---|---|---|
| **`BackyardCornhole-BYCEAO.iso`** (primary) | WIT composes a Wii image from a **complete** extracted-disc tree (below), with `sys/main.dol` = our `boot.dol` | Proposed ID **`BYCEAO`**: `BYC` = Backyard Cornhole, `E` = USA/NTSC-U, `AO` = maker code. Check GameTDB for collisions before freezing it. The ID's region letter is disc metadata only; video mode is set by our code (below) |
| `BYCEAO.ini` → `GameSettings/BYCEAO.ini` (shipped alongside) | Plain text, below | Turns on real Wiimotes for this game only |
| WAD (**conditional**, not interchangeable) | Channel WAD with the DOL as boot content, installed into Dolphin's NAND | A WAD has a 4-char title ID, not our 6-char disc ID, so its settings filename differs. Only pursued if the ISO fails, with its own defined title ID, settings filename and P0-style proof |
| `boot.dol` (dev + fallback) | Loads directly in Dolphin Android/desktop | No game ID → needs the global Real-Wiimote fallback below. Stays usable while ISO packaging is being proven |

**ISO recipe (P0 deliverable; `main.dol` alone is not enough).** WIT's composing guide requires an extracted tree with `sys/boot.bin`, `sys/apploader.img`, `sys/main.dol` and a `files/` directory; recent WIT can generate `sys/bi2.bin`. So `wii/dolphin/iso-root/` must contain:
- `sys/boot.bin`: generated by our tool (disc header: ID `BYCEAO`, title "Backyard Cornhole", Wii magic), never copied from a retail disc.
- `sys/apploader.img`: from a **redistributable** source (WIT's documented template, or an open-source homebrew apploader). Its provenance and license are recorded in `wii/dolphin/PROVENANCE.md`. **No Nintendo retail apploader is bundled.**
- `sys/main.dol` (copied from `boot.dol` at build time), and `files/` (may be empty or hold a README).
- Region metadata set to **USA** alongside the `E` ID.
- WIT version and exact command line pinned in the Docker image and Makefile.

P0 proves it: Dolphin shows ID `BYCEAO`, region USA, the ISO boots, and the SHA-256 of the `main.dol` extracted from the ISO equals `boot.dol`.

```ini
; GameSettings/BYCEAO.ini — Backyard Cornhole (Wii port)
[Controls]
WiimoteSource0 = 2
WiimoteSource1 = 2
```

`WiimoteSource0`/`WiimoteSource1` map to Wii Remote 1/2, and `2` = **Real** (Dolphin source: `enum class WiimoteSource { None = 0, Emulated = 1, Real = 2 }`; `GameConfigLoader.cpp` maps `[Controls] WiimoteSourceN` to `WiimoteN.Source`). Remote 2 is for 2 PLAYER.

**Fallback:** if the per-game ini isn't picked up (a wrong filename/ID, or a bare `.dol`), set Dolphin's **global** Wii Remote 1 (and 2) source to **Real Wii Remote** in Controller settings. This affects every game on the box, so the per-game ini is preferred.

Video (**NTSC/60 Hz only**, on both targets): the build selects `TVNtsc480Prog` when progressive output is actually available (`VIDEO_HaveComponentCable()` and the console's progressive setting), else `TVNtsc480IntDf`. There is **no EURGB60 or PAL branch**, and a PAL-region console setting never selects 50 Hz. Three separate requirements, tested separately: video init (our code), disc region metadata (ISO), and Dolphin configuration (left at NTSC defaults).

### 1.6 Kinhank install (Retro installs and tests)

1. Copy the ISO over ADB to `/storage/3050BAA350BA6EEA0/Roms/wii/`.
2. Put `BYCEAO.ini` in the `GameSettings/` folder of **this** Dolphin Android build's user directory. The path isn't assumed from desktop Dolphin or newer Android builds: Retro records the real path for 5.0-19870 at the P0 check, and `INSTALL-KINHANK.md` is updated with it. Alternatively, set the same option in the game's per-game settings inside Dolphin Android.
3. DolphinBar in **Mode 4**, Wiimotes synced to it. The DolphinBar also acts as the IR sensor bar.
4. Dolphin settings already proven on this box: Vulkan, 1× native resolution.
5. Virtual SD: at P0, Retro confirms that this Dolphin build's virtual SD card is enabled and writable (saves, logs and traces go to `sd:/apps/backyardcornhole/data/`), and records how to copy traces off the box (the build's SD export/sync option, or `adb pull` of the SD image/folder). This is written into `INSTALL-KINHANK.md`.
6. Launch from Dolphin's game list. Confirm the game ID shows as `BYCEAO` in Properties.

Developers don't touch the Kinhank: no ADB from the dev box. Release notes include the exact `adb push` commands for Retro.

---

## 2. What ports vs what is rewritten

Legend: **Port** = translate the logic line for line into C with the same constants; **Rewrite** = same behavior, new implementation; **Drop** = not in the Wii build.

### 2.1 Physics — Port (golden-tested)

All of it is plain math in `index.html` lines 193–514 plus `spin.mjs`. Constants to carry over exactly:

| Item | 1.0.3 value |
|---|---|
| Board | 2×4 ft deck, front 3.5 in, back 12 in, `TH = asin((BACK_H-FRONT_H)/L)`, front edge at `BOARD_Z = -27 ft`, 6 in hole centered 9 in from the top |
| Bag | 6 in square, `BAG_H = 0.016`, `BAG_R = 0.074`, restitution `BAG_E = 0.25` |
| Surfaces | slick `{mu .43, ms .60, keep .32}`, sticky `{mu .62, ms .85, keep .30}`, grass `MU_GROUND = 1.1` |
| Launch | release `(rx, 0.98, -0.45)`; arcs `[24°, 35°, 58°]` = SLIDER/NORMAL/KOBE; speed `p<.4 ? 5.6+6p : 8+(p-.4)·3.2`; range normalized to 35° via `sqrt(sin 2·35° / sin 2·elev)`; yaw `baseYaw(rx) + aim + drift·0.04` |
| Wind | `roundWind()` from `controls.mjs`: shared per round, flips side each round; non-practice 6–10 mph; circuits 4–6, 6–9, 9–12, 12–15 mph; `practice:true` returns calm; accel `mph·0.025`. **Mode policy comes from the arcade controller** (§2.3) |
| Spin | `AIR_SPIN_RATE = 14 rad/s` (visual flat spin), curl slick `3.2 m/s²` decay `exp(-1.4t)`, sticky `1.5` decay `exp(-4.5t)`. Negative spin = clockwise from above = curls **right** (+X) |
| Integrator | fixed **240 Hz** step with accumulator, `dt` clamped to 0.05 |
| States | `fly → slide/board`, `ground → grass`, `hole → in`, dead grounder `→ removed` after 0.7 s |
| Landing | `keep × (KOBE .40 / SLIDER 1.35 / NORMAL 1)`; swish if `holeDist < 0.9·HOLE_R`; slide-in if `< HOLE_R − 0.012`; front-edge and side bounces |
| Bag–bag | `collide()`: airborne bag landing on a resting bag (`.12` KOBE / `.55` other), equal-mass disc impulses, rider lift on hard contact, resting bags re-woken |

Implementation notes:
- Use **double** in the core. JS numbers are doubles, the Broadway FPU does doubles in hardware, and parity with the WIN table and CPU calibration matters more than speed. A match round has ≤ 8 bags, but **practice has no bag limit** (`if (!M.practice && M.k >= 8)` skips it; bags accumulate until CLEAR BOARD), so `collide()`'s pair scan grows with practice bags (§3.2).
- **Loop order is part of the contract** (`index.html` frame loop): each 240 Hz substep steps eligible bags in insertion order (skipping `in`, `removed`, resting `board`, and non-dead `grass`), then calls `collide(bags)` once. `afterThrow` waits until **all** bags are settled, including dead grass bags waiting for removal. Contact can re-wake resting bags.
- **Per-bag flags are part of the contract:** `touched`, `dead`, `offBoard`, `prev`, `hy`, `restT`, `lp`/`lv` (board-local), `english`, `curl`, `arc`. They decide front-edge vs side impacts, grass-first fouls, fall-offs and hole transitions; equal final positions don't imply equal scoring or callouts.
- Rebuild the board transform (`deckG.matrixWorld`, `boardQ`, `HOLE_WORLD`, `G_LOCAL`) in C from the same position, rotation and `BOARD_Z`. Verify against golden values.
- `physicalPower()` / `calibratedWindow()` are **not** identity for KOBE: the lob window is remapped (`normalWidth·.78`). Port them; `sweetShift` is always 0 in 1.0.3.
- Visual-only math (tumble, yaw blending, hole shrink to 0.53, flight trail) is ported in the renderer, not the core.

### 2.2 WIN table — Port, then bake

`WIN[team+side+arc]` is built at startup by simulating 12 combinations × 401 power samples (~2M physics steps; Broadway time unmeasured until P1). **Bake it at build time** from the host build of the C core into `win.bin`, using the portable binary format in §2.9. Debug builds recompute it on the Wii once and compare with a **tolerance**: the same `lo/hi/olo/ohi/mid` within one 0.0025 power step, not bit equality, since host and PPC math libraries may differ in the last bits.

### 2.3 Scoring and match flow — Port

`lie()`, `scoreInning()` (cancellation scoring, first to 21, last scorer throws first, wash keeps the order), `afterThrow()` (knocked-in/off detection from the `M.before` snapshot), `nextTurn()`, `beginInning()`, `advance()`, practice stats, coin toss. Phases: `menu/intro/champion/wait/aim/cpu/flying/settle/summary/over`. Live `lie()` counts only `in` and non-dead `board` bags (moving slides don't count); the round is scored **once**, after all eight throws settle. The Wii keeps `exact:false`. The `exact`/bust-to-13 code is unused support, not a menu mode. Callouts carry over verbatim: **CORNHOLE! · HANGER! · ON THE BOARD · GROUNDER · OFF THE BOARD · MISS · KNOCKED IN! · KNOCKED OFF! · WASH** (copy exemption in §6).

Modes on Wii, same as the arcade menu: **PRACTICE**, **2 PLAYER**, **TOURNAMENT**.

**Practice CLEAR BOARD** (1.0.3's practice-only `#reset-bags` button, "Clear bags; keep stats"): on the Wii it's in the practice pause menu as **CLEAR BOARD**. It removes all thrown bags and keeps practice stats. It's separate from **RESET BAG** (button 2), which only re-centers the bag in hand.

**Wind policy (parity with the released arcade controller):** the arcade starts with `windy = false` and applies `host.wind(windy)` when PRACTICE or 2 PLAYER starts and on every non-career wait phase. So by default:

| Mode | Default | WIND setting ON |
|---|---|---|
| 2 PLAYER | Calm | `roundWind` non-practice (6–10 mph), new strength and flipped side each round |
| PRACTICE | Calm | Same as 2 PLAYER (matches `host.wind(true)`, which requests non-practice wind) and stays on for later throws |
| TOURNAMENT | Circuit wind (4–6 / 6–9 / 9–12 / 12–15 mph), unaffected by the setting | — |

The WIND setting (CALM / ON) persists in `save.dat`.

### 2.4 CPU opponents — Port, time-sliced

- Casual CPU (`nextTurn` CPU branch): side `sticky` 30% of the time; power `WIN.mid + wind.z·0.04 + gauss·0.055`; aim `−wind.x·0.08 + gauss·0.012`; drift `gauss·0.18` (which also adds visual spin). Cheap. (Not in the arcade menu; kept as a test mode.)
- Tournament (`planCareerShot`): reads the current deck to choose arc, side, target and goal (hole / push own bag / knock enemy / block) by rival style and level. Its candidate search then **simulates one new bag with `step()` only; it doesn't call `collide()` against blockers**. Port that single-bag predictor exactly; don't make it smarter during the parity port. Coarse grid (power .35–.85 × aim ±.06), fine grid around the best, then a fallback grid if no board/in shots were found. Hole shots are averaged when found. `CPU_RATES[level]` picks the in/board/miss **pool** to draw from (a desired selection probability, not a guaranteed live outcome); non-wild rivals pick among the 24 candidates closest in power to the best, and Tailgate Ted (wild) samples the whole pool. Sierra Spin uses `curl = .65`. All other rivals throw with zero spin.
- Cost: up to ~1,800 single-bag simulations × ≤1,920 steps. **Broadway and Kinhank times are unmeasured** and get benchmarked in P1/P4. The search runs as a resumable job that can **yield inside a simulation** (step-granular), with a per-frame budget set from those measurements (starting guess 4 ms), during the CPU's 1.3 s windup. If it hasn't finished, the windup holds. Never block a frame.
- Parity is tested with fixed seeds and identical scenarios: compare the selected arc, side, spin, power, aim, pool and RNG consumption against JS (§5.1). Statistical rate checks are secondary.
- Roster text, nicknames, taunts, circuits and records port verbatim. Portraits are 16×16 SVG pixel art in `portrait()`; bake them to textures with the exporter.

### 2.5 Rendering — Rewrite (Three.js → GX)

| Three.js in 1.0.3 | Wii/GX plan |
|---|---|
| `WebGLRenderer`, internal res up to 1920×1080, pixelRatio 1 | 640×480 EFB → NTSC 480p (or 480i) XFB, 60 Hz only (§1.5). Optional 16:9 anamorphic when the console is set to widescreen (`CONF_GetAspectRatio`) |
| `PerspectiveCamera` at `(0,3.6,8)` → `(0,0.5,-8.7)`; constructed at fov 19, but `resize()` immediately sets `fov = 18 + 6·portrait` | **Effective landscape FOV is 18°** (portrait = 0 at both 4:3 and 16:9). Use 18° via `guPerspective`/`guLookAt` with the same position and look target; check framing at both aspect ratios |
| `HemisphereLight` + `DirectionalLight` sun, `MeshLambertMaterial` | Static scenery: **pre-lit vertex colors** baked by the exporter (same light directions and colors, tuned against **fresh 1.0.3 screenshots** captured at 4:3 and 16:9; the older preview stills predate 1.0.1 and still show the removed cooler). Dynamic objects (bags, dog, NPCs, held bag): one GX directional light + ambient |
| `BasicShadowMap` 1024² sun shadows on everything | **Drop real-time shadow maps.** Bake static shadows/AO into vertex colors; draw blob-shadow quads under bags, dog and NPCs. Render-to-texture shadows are possible on GX but not worth the cost here |
| `Fog('#c4d3a7', 26, 75)` | `GX_SetFog(GX_FOG_LIN, 26, 75, …)` — direct match |
| `CanvasTexture`, NearestFilter, tiny sizes (8×8 bag, 32×64 board, 16×16 materials, 64×64 grass, 16×8 hay) | Bake to RGB565/RGB5A3 TPLs (or raw GX textures) with `GX_NEAR`. Pixel art is the best case for the Wii's texture cache |
| `InstancedMesh` voxels (backyard.js) | Exporter merges static voxels into **one display list per material** in world space, with hidden faces removed and chunked for frustum culling |
| `ExtrudeGeometry` board with hole (`curveSegments 14`) | Exported once as a mesh |
| Bag geometry: 4×1×4 segmented box, puffed | Exported once. **Material rule:** the chosen surface is the side thrown **down**. Bottom face (−Y) = the chosen surface; top and side faces = the **opposite** surface. Slick-down shows sticky on top, and vice versa (`makeBagMesh`; the held bag uses the same rule). Visuals: release yaw from the held bag, full-flight flat spin at `14 rad/s × english` (+ drift for CPU), on-deck yaw drift `spin·0.3`, visual spin decaying `×0.1^dt` while sliding. All of that is **separate from** the landing-curl scalar (`spin.mjs`), which is physics |
| Sky sphere (16×10, vertex colors, no fog) | Exported; drawn first, fog off |
| Pennant: 17×7 vertex grid animated per frame | CPU-updated vertex array, same `animatePennant()` math |
| Wind streaks: 24 `LineSegments` | `GX_LINES` with alpha, same update math (separate seed) |
| Confetti 70, board dust 14, flight trail 3, smoke 5 | Pooled unit cubes, unlit, alpha via TEV |
| Board View PIP (`board-view.mjs`: camera fov 30, near .05, far 30, at `(0, 2.5, BOARD_Z+.75)` looking at `(0, .16, BOARD_Z−.55)`; the original clears color **and depth** in the inset and restores state) | Second pass after the main scene, before the HUD: set viewport + scissor to the inset → **reset the inset's color and depth** (draw a full-inset quad with the background color, depth compare `GX_ALWAYS` and depth write at far Z; or `GX_SetCopyClear`-equivalent behavior inside the scissor) → set the PIP projection/view → draw the culled list (board, bags, ground patch) → **restore** projection, viewport, scissor, depth, blend and fog state for the HUD. **No** render-to-texture/EFB copy (§3.4). Drawn every frame (no skipped frames, since skipping makes the inset disappear) |
| Power arrow (SVG) + gauge colors (`shotPowerColor`) | 2D quads with the same 5-stop color ramp `#8fc6ee → #a7efb1 → #ffe16b → #f6a35c → #f1755f` |
| Party NPCs (`party.mjs`): keg, hose, 6 people, scripted beer pong, chess, cook, cacti | Static parts export as merged meshes. Animated parts keep **object identity and pivots** in the export manifest (§2.9) so P6 can animate them. Static poses are an interim milestone state; the release requires the animations unless Ashe descopes them (§9). Scripts (`pongCatalog`, `pongTiming`, `buildPongScript`) are pure data and port directly |
| Dog + `DogCelebration` (hops/spin/bow) | Port: hierarchical boxes, same pose math |

### 2.6 UI/HUD — Rewrite

All HUD is DOM/CSS today: scoreboard with pips, turn banner, callouts, wind panel, power gauge with 3 s last-shot linger, key legend, round summary, tournament map/intro/result/champion, menus. Rebuild it as 2D quads + bitmap text at 640×480 using the existing teal/cream/gold palette (`#173f3d`, `#fff1cf`, `#ffe16b`, red `#f1755f`, blue `#8fc6ee`).

- Font: the game uses **Press Start 2P** (OFL 1.1, Reserved Font Name "Press Start 2P", `vendor/FONT-LICENSE.txt`). An 8×8 bitmap atlas works at 1×/2×/3× on 480p. Ship the license text. The converted font must not be *called* "Press Start 2P" in credits or files; describe it as "based on Press Start 2P" with the license included.
- **Glyph repertoire** (fixed, checked by a build-time string-table scan): printable ASCII plus custom glyphs `▶ ◀ · − × ✚ ← → ↑ ↓ ↻ ↺` and Wiimote button icons (A, B, 1, 2, −, +, HOME, D-pad). Typographic characters in source strings (`’ “ ” –`, e.g. "Slidin’ Steve", "CPU’S THROW") are mapped to ASCII at export. Any other character fails the build. **480i readability:** in-play text renders at ≥ 2× (16 px glyphs); 1× is only for the dev overlay.
- **Drop** from the Wii build: CABINET SETUP, trackball calibration, diagnostics, the KEYS legend's cabinet button grid (replaced by a Wiimote legend), EXIT GAME (replaced by HOME → quit to HBC), the version IPC (compile-time version string instead), the bag drawer/touch UI, and all pointer/touch pull code (`pullAt`, `pointerdown`, etc.).

### 2.7 Audio — Rewrite (small)

1.0.3 has **no sound effects**; its only audio is a user-supplied music playlist read from disk. On Wii:
- v1: a short SFX set (bag thump on wood, grass thud, hole swish, crowd cheer, menu tick) from CC0 sources already shortlisted in `arcade/AUDIO.md` (Kenney Impact Sounds). Keep each source's license file. ~2–4 MB of 32 kHz mono.
- Wiimote rumble: short pulse on board hit, longer on CORNHOLE!. The Wiimote speaker is optional and not in v1.
- Music: **drop for v1.** Optional later: OGG from `sd:/apps/backyardcornhole/music/` via Tremor. Never bundle commercial music.

### 2.8 Saves — Rewrite

| localStorage key (1.0.3) | Wii |
|---|---|
| `backyard-cornhole-career-v1` `{fight, wins}` | `career {fight, wins}`, with the **same semantics**: valid only if `0 ≤ fight ≤ 8` and `wins ≥ fight`; saved only when a fight is won (an interrupted match restarts the current rival); NEW CAREER asks to confirm, then resets to `{0,0}` |
| `backyard-arcade-v9` (trackball profile) | **Per player slot** (RED, BLUE), not per remote: `fullPowerSpeed`, `handedness`, `spinStrength`. Remote identity isn't stable across reconnects, channel changes or pass-and-play; slots are |
| `backyard-arcade-keys-tucked` | `helpTucked` |
| (arcade WIND toggle) | `wind` (CALM/ON, §2.3) |

**One path:** `sd:/apps/backyardcornhole/data/save.dat` (+ `save.bak`). **Serialization:** explicit field-by-field encoding, **big-endian**, no raw struct dumps (no padding/ABI dependence): magic `BYCS`, `u16` format version, `u16` length, fields, then `u32` CRC-32. **Writes are crash-safe:** write `save.tmp` → flush/close → rename the previous `save.dat` to `save.bak` → rename `save.tmp` to `save.dat`. On load: `save.dat`, then `save.bak`, then defaults. Any field failing validation falls back to its default. If the SD card is missing or read-only, keep playing and show the existing copy: "Progress won't save on this device."

### 2.9 Exporter and binary formats

- **Scene export manifest** (`data/scene.bin` + `scene.json` for review): per mesh, the material id, texture id with **repeat/offset** (the board uses `repeat(1/W, 1/L)`, `offset(.5,0)`; grass repeats 24×), material groups (board deck vs skirt), **vertex winding** (CCW front), cull mode, alpha/depth-write/fog flags (the sky is inside-facing, no fog, no depth write), and whether it's static-merged or a dynamic node with a **pivot and parent** (party NPC parts, dog parts, pennant, held bag).
- Hidden-face removal runs only on static merged geometry and only removes faces that touch another static solid. It never removes faces of dynamic nodes or faces visible from the PIP camera.
- **Color:** Three.js r170 materials use sRGB textures and linear lighting; the exporter converts to the Wii's direct 8-bit color with a documented bake (lighting computed in linear, written as sRGB-encoded vertex colors), tuned against fresh **1.0.3** screenshots. No one-to-one TEV equivalence is assumed.
- **Portraits** are SVG `path` data (`M…h…v…z` rectangles only, `portrait()`); the exporter includes a small path decoder for that subset and rasterizes 16×16 RGBA, or fails loudly on any other command.
- **Binary format** for `scene.bin` and `win.bin`: magic + format version + section table; **big-endian** (Broadway's native order); explicit field widths (`f32` for geometry, `f64` for WIN bounds); 32-byte-aligned GX vertex/display-list sections; decoded with explicit readers (no struct casting) and **decode-tested on the Wii and in Dolphin** in P0/P1.
- Tool inputs are pinned: `three@0.170.0` from `arcade/package-lock.json`, the 1.0.3 snapshot's files by SHA-256 (recorded in each output's header).

---

## 3. Asset and performance budget

Two budgets apply, and both must pass:
1. **Real Wii hardware (first):** Broadway CPU 729 MHz (PPC 750CL, paired-singles), Hollywood GPU 243 MHz (fixed-function TEV, 1 MB texture cache + 2 MB embedded framebuffer), **88 MB RAM** (24 MB MEM1 + 64 MB MEM2), NTSC 480i/480p at 60 Hz.
2. **Kinhank: Dolphin Android 5.0-19870, Vulkan, 1× native, Mali GPU (§3.4).** Mario Kart Wii and Wii Sports running at full speed there prove the setup works. They are **not** a license to go heavier than those games: stay at or below their scene complexity.

### 3.1 RAM

| Item | Budget |
|---|---|
| Code + libogc + core | ≤ 4 MB |
| Static scene geometry (display lists, pre-lit) | ≤ 3 MB |
| Textures (all procedural pixel art, font atlas, portraits, icons) | ≤ 1 MB (realistically ~200 KB) |
| Dynamic buffers (bags, pennant, particles, HUD batch) | ≤ 1 MB |
| SFX | ≤ 4 MB |
| XFBs (2 × 640×480 YUV) + GX FIFO | ~2 MB |
| WIN table, CPU scratch, save | < 0.5 MB |
| **Total target** | **≤ 16 MB**, leaving more than 70 MB headroom. No streaming is needed |

### 3.2 Frame (NTSC 60 fps = 16.7 ms; no 50 Hz mode)

| Item | Budget |
|---|---|
| Main view triangles | ≤ 25k after hidden-face removal and culling |
| Board View PIP | ≤ 3k triangles (board, bags, ground patch only) |
| Draw submissions (display-list calls + matrix loads) | ≤ 400 |
| TEV stages per material | ≤ 2 (texture × vertex color; + fog) |
| Physics (240 Hz, collisions) | ≤ 1 ms at 8 bags (match); practice benchmarked up to **48 retained bags** (`collide` is O(n²) per substep); policy below |
| CPU AI search slice | Starting guess 4 ms (only during a CPU windup); set from P1/P4 measurements |

All Broadway/Kinhank timing figures in this section are **budgets to verify, not measurements**. P0/P1 adds real profiling on both targets (frame time on the Wii; Dolphin speed % on the Kinhank).

**Practice bag retention:** 1.0.3 keeps every practice bag until CLEAR BOARD. The Wii keeps that behavior as long as the 48-bag benchmark holds 60 fps on both targets. If it doesn't, the fallback is a **gameplay change that needs Ashe's approval** (§9): for example, an automatic "CLEAR BOARD?" prompt at the measured limit. Bags are never silently dropped.
| Input + swing detector | ≤ 0.3 ms |
| HUD | ≤ 1 ms CPU |

### 3.3 Scene budget and the scoped subset

The 1.0.3 yard is all low-poly boxes: fences (~250 slats), 8 trees, hedges, stones, string lanterns, grill, 6 NPCs (~20 cubes each), keg with an 18-segment hose, beer pong table and cups, chess set, hay bales, dog, sky sphere. Codex's headless build of `createBackyard` + `createParty` (snapshot, before culling) measured **258 mesh nodes, 746 mesh instances, 11,114 triangles, 72 material objects**. Board, bags, dog, HUD and inline props come on top. 72 material objects is **not** 72 shader configurations: most differ only in color, which goes into vertex colors in the bake. That leaves the triangle budget comfortable, while submission count and distinct material setups are the real constraints (§3.4).

**60 Hz / 60 fps is the acceptance contract on both targets.** If measurements in P3 say otherwise, cut scenery in this order before release: (1) wind streaks, (2) NPC detail (merge limbs), (3) far fence slats become one textured quad, (4) fewer background props. There is no PIP frame-skipping and no 30 fps fallback. A reduced-frame-rate or reduced-content edition would be a separate scope decision by Ashe.

### 3.4 Dolphin Android / Mali rules (Kinhank)

Dolphin's cost isn't the Wii's cost. These rules keep the game cheap to emulate at 1× on a Mali GPU:
- **No CPU reads of the EFB** (`GX_PeekARGB`/`GX_PeekZ`), and no EFB→RAM copies during play. No render-to-texture effects. The PIP uses a second viewport, not a texture copy.
- **One XFB copy per frame**, no mid-frame XFB tricks.
- **Few distinct TEV/blend setups** (target ≤ 12 material configurations for the whole game). Each new combination is a Dolphin shader compile and can stutter on first use; a small fixed set warms up in the first seconds of the menu.
- Draw submissions ≤ 300 in the main view (stricter than the Wii's 400), because each GX state change also costs Dolphin's CPU-side translation on an ARM core.
- No logic ops, no dual-source tricks, no dynamic texture uploads each frame except the tiny pennant and particle vertex arrays.
- Verify with Retro on the Kinhank at every milestone from P1 onward (IMPL_PLAN). Desktop Dolphin speed doesn't predict the Kinhank.

### 3.5 Scoped out

**Cannot reasonably run on Wii, scoped out:** the Three.js/WebGL runtime itself; real-time 1024² shadow maps over the whole scene (blob plus baked shadows instead); DOM/CSS UI; the music folder scanner. Everything that affects play — physics, spin/curl, scoring, CPU AI, wind, tournament — survives intact.

---

## 4. Wiimote control mapping

The research behind this mapping (cabinet control → Wiimote equivalents, Wii Sports-style precedent, sources) is in [WIIMOTE_ARCADE_MAPPING.md](WIIMOTE_ARCADE_MAPPING.md). This section is the normative summary.

### 4.1 Standing rule

> **Power comes from swing SPEED — the peak of an accelerometer-derived speed signal over a short window — and NEVER from stroke distance, swing length or swing duration.** A long, slow swing stays soft; a short, quick swing can be full power.

The *rules* mirror 1.0.3's `trackball.mjs`: peak speed over an `80 ms` window, `power = clamp(peak / fullPowerSpeed)`, linear, capped at 100%, release on deceleration, and nothing distance-like feeds power. The *measurement* differs: the cabinet measures speed directly (forward counts ÷ elapsed time), while a plain Wiimote only has an accelerometer. So the Wii signal is a **candidate speed proxy that P1 must validate** (§4.4). It isn't assumed to be "measured the same way".

### 4.2 Grip and gesture (right-handed default; left-handed mirrors the spin sign)

Hold the remote like the bag: pointer end toward the screen, buttons up, strap on.

1. **Aim:** point at the board. The reticle moves the landing line.
2. **Twist to curl** (optional): roll your wrist while aiming. A small curl arrow by the bag shows the direction.
3. **Grab:** press and hold **B**. Aim and curl lock.
4. **Toss:** swing back (optional), then swing forward underhand.
5. **Let go of B** to release the bag. If you keep holding, the bag releases itself once the swing slows (§4.4).

Releasing B with no forward swing does nothing, so the bag stays in hand. This matches 1.0.3's "a single report or tiny jitter cannot throw".

### 4.3 Full button map

| Input | In play | In menus |
|---|---|---|
| **IR pointer** | Aim (locks on B press) | Point at a button to highlight it |
| **B (hold)** | Grab → swing → let go to toss | Back |
| **Wrist roll** | Curl: twist right = curl right, twist left = curl left (locks on B press) | — |
| **A** | Flip bag surface SLICK ⇄ STICKY | Confirm |
| **D-pad ↑ / ↓** | Throw height up / down: SLIDER ⇄ NORMAL ⇄ KOBE | Move highlight |
| **D-pad ← / →** | Step left / right along the foul line (0.05 per press, clamp ±0.45, eased 70 ms — the cabinet's nudge) | Move highlight |
| **− / +** | Nudge aim left / right (for no sensor bar or a lost pointer; ±0.01 rad steps) | Adjust value |
| **1** | Show / hide the button help | — |
| **2** | Reset bag (center stance, clear aim/curl/windup; never costs a throw) | — |
| **HOME** | Pause: RESUME · CLEAR BOARD (practice only) · MAIN MENU · QUIT | Pause / back |

**Players and remotes.** RED = player slot 1, BLUE = player slot 2. Slot 1 uses Remote 1. Slot 2 uses Remote 2 if it's connected at match start; otherwise slot 2 is **pass-and-play on Remote 1**, shown on the turn banner. Profiles (SET POWER, hand, curl) belong to the **player slot**, not to a remote, so they survive reconnects and channel changes. In pass-and-play, Remote 1 uses the current thrower's slot profile.

**Disconnects:** if the remote for the current thrower disconnects, the game **pauses and discards any gesture in progress**. It shows a slot-correct prompt, e.g. `CONNECT REMOTE 2`, with `A: USE REMOTE 1` to switch slot 2 to pass-and-play. Play resumes only after the remote reconnects (or is reassigned) **and B is up**; the detector restarts in `settling`.

**Action legality** (mirrors the cabinet, where height, surface, nudge and reset all call `machine.reset`):

| Input | Ready (not holding B) | Armed (holding B: grip/backswing/forward) | Bag in flight / CPU turn / menus |
|---|---|---|---|
| A (surface), D-pad ↑/↓ (height), D-pad ←/→ (step), 2 (reset bag) | Apply, then detector → `settling` (450 ms calm before B can arm) | **Ignored** | Ignored in play; menus use A/D-pad for navigation |
| − / + (aim nudge) | Apply (aim isn't locked yet) | **Ignored** (aim is locked) | Ignored |
| 1 (help) | Toggle | Toggle (UI only, gesture unaffected) | Toggle |
| HOME | Pause | Pause **and cancel the gesture** (bag stays in hand) | Pause |
| Pointer / wrist roll | Live aim / curl preview | Locked values shown | — |

### 4.4 Swing-speed detector: candidate design, gated by P1

Status: **candidate.** The requirement (speed only, short window, peak, calibration, no distance or hold time) is fixed. The estimator and direction classifier below are the starting design that P1 must validate on plain Wiimotes on both targets (IMPL_PLAN P1 exit gate). If it fails, the estimator and/or supported gesture is revised and P1 repeats. Tuning thresholds or adding optional MotionPlus does **not** count as fixing the required plain-remote path.

**Acquisition (libogc WPAD)**
- `WPAD_SetDataFormat(chan, WPAD_FMT_BTNS_ACC_IR)`, `WPAD_SetVRes`. Persistent per-channel buffers registered with `WPAD_SetEventBufs(chan, buf, 64)`, with return codes checked. **One owner** (the main loop) drains each queue with `WPAD_ReadEvent` every frame. `WPAD_ReadPending` drains the **same** queue, so it isn't a fallback. `WPAD_DroppedEvents` is sampled every drain.
- libogc's disconnect handling clears the event-buffer registration, so buffers are **re-registered on every reconnect** before input is accepted.
- If buffered events misbehave on a target, P0 picks and documents another acquisition path (for example, a dedicated higher-rate thread with its own timing record) before P1 starts.
- Fields are used only when `err` is OK and the relevant `data_present` bits are set.
- **Signal source:** libogc's calibrated **`gforce`** (g units, per-remote calibration applied by libogc; `WPADData` exposes no `accel_calib`). Raw `accel` is kept only to detect **saturation** (an axis at its raw extreme).
- **Timestamps are estimates.** `WPADData` has none. Each drain records its own time, event count and order; sample times are reconstructed at the measured report interval and flagged as estimated. A gap (estimated spacing > 25 ms, or a dropped-event increase) while armed **cancels the gesture and re-arms**. It never interpolates a continuous swing across a gap.
- **Remote axes:** libogc/wiiuse frame, +Z out of the button face (≈ +1 g when lying flat, buttons up), +Y along the remote's length, +X across it. P0 confirms every sign with an on-screen readout before any classifier uses them.

**Speed proxy (candidate)**
- `e = max(0, |gforce| − 1 g)`. This is a scalar-gravity heuristic, **not** true motion acceleration (subtracting gravity's magnitude isn't subtracting its vector).
- Pendulum-bottom model: `v ≈ sqrt(R_EFF·g·ē)` with `R_EFF = 0.6 m`. Known error: `v_est = v·sqrt(R_EFF/r)`, so the same hand speed at radius 0.3 m vs 1.2 m differs by about 2×. One calibration can't remove changes of radius within a player's throws. So the **supported gesture** is a full-arm underhand swing from the shoulder with the elbow mostly straight; wrist flicks and shakes are outside it and must come out soft or be rejected. P1 measures how big the usable envelope really is.
- Window: samples with estimated time in the last 80 ms; **valid only if the window's elapsed span (first→last sample time) is ≥ 16 ms** (≥ 3 samples at 100 Hz) and `ē ≥ NOISE_G`. Time-weighted (trapezoid) mean.
- **Power = clamp(peak_forward_v / fullPowerSpeed, 0, 1)**, linear in the speed proxy.
- **Auto-release** in the same speed units as the cabinet: `v ≤ 0.6·peak_v` after a full window (equivalent to `ē ≤ 0.36·peak_ē`).

**Clipping**
- The rail is per raw axis, **including gravity**. At the bottom of the arc the dominant axis reads about `1 g + ē`. A full-power default of `ē ≈ 2 g` would sit right at a ±3 g rail, so that earlier figure is withdrawn.
- Default: full power set so the dominant axis stays below the **measured** rail minus a margin. Candidate `ē_full ≈ 1.2 g`; the real value comes from P1 traces.
- **Calibration** ("TOSS HARD", `fullPowerSpeed = measured_peak / 0.7`) needs `1/0.7² ≈ 2.04×` the calibration toss's `ē` to reach full power, so the result is **capped at the per-remote clip ceiling**.
- Saturation can only **cap an already validated forward toss** at full power. It never validates a toss by itself. Saturated samples in grip, backswing or rejected phases are ignored and flagged in the trace.

**Forward/backward classification: candidates, one chosen in P1** (acceleration sign alone isn't velocity direction, so it's not used as the classifier)
- **C1 — lobe timing (accelerometer only, primary candidate).** A swing with a backswing passes the bottom twice: two `ē` lobes (back, then forward) separated by a low-`ē` apex. Power comes from the **last** qualifying lobe (≥ `FORWARD_START_G`, ≥ 40 ms long) that ends within `T_REL` of the B release. A release at the back apex leaves no qualifying recent lobe → cancel.
- **C2 — follow-through confirmation.** After the forward pass, an underhand toss rises back toward the screen: the IR is reacquired, or the calm-sample gravity direction returns toward the grip attitude. Backswing-only motion ends pointing back and down. Used to confirm a lone lobe and to permit auto-release while B is held.
- **C3 — apex attitude.** At low-`ē` apex samples, `gforce` ≈ gravity, so remote pitch is readable: back-down apex vs forward-up finish labels the lobe before it.
- **C4 — MotionPlus pitch-rate sign** (optional; never required).
- **Arming:** B arms only if the last 150 ms were **stable**: `ē < 0.10 g` **and** the gravity direction changed < 5° across that span. This rejects slow rotation and motion started before B. If unstable, the bag stays in hand and the HUD shows `HOLD STILL`.
- **P1 exit gate:** zero false throws on the adversarial trace set (backswing braking, backswing-only release, motion started before B, no-backswing toss, shake, wrist-only rotation, pointer loss, packet gap, clipping in a rejected phase).

Phases:

| Phase | Enter when | Notes |
|---|---|---|
| settling | turn start, any legal button action, reset, after a throw, reconnect | Arms only after 450 ms with `ē < NOISE_G` (1.0.3 `quietMs`) |
| ready | settled | Aim/curl preview live |
| grip | B pressed **and** stability check passed | Aim and curl lock; the gravity direction is captured |
| backswing / forward lobes | per the chosen classifier (C1–C3) | Only the qualifying forward lobe feeds power |
| release | B up (primary); auto-release (`v ≤ 0.6·peak_v` + C2 confirmation); 2.5 s timeout | Throw only with a valid forward lobe; otherwise cancel quietly (bag stays) |
| spent | after a throw | Until the next turn |

Tunables (one header; every value written into each swing-trace header together with the detector version):

| Name | Start value | Notes |
|---|---|---|
| `SPEED_WINDOW_MS` | 80 | Same as 1.0.3 |
| `MIN_WINDOW_SPAN_MS` | 16 | Elapsed span, not sample count |
| `NOISE_G` / `STABLE_G` / `STABLE_DEG` | 0.15 / 0.10 / 5° | |
| `FORWARD_START_G`, lobe min length, `T_REL` | 0.35 g, 40 ms, 250 ms | C1 |
| `R_EFF_M` | 0.6 | Scales units only |
| `E_FULL_DEFAULT_G` | ≈ 1.2 (from P1) | Below the measured rail |
| `CAL_HEADROOM` | 0.7, capped at the clip ceiling | |
| `AUTO_RELEASE_SPEED_RATIO` | 0.6 | Speed units |
| `GAP_CANCEL_MS` | 25 | |
| `QUIET_MS` / `TIMEOUT_MS` | 450 / 2500 | 1.0.3 values |

**Swing trace format** (CSV with a header block): detector version, all tunables, build id, target (Wii / Dolphin desktop / Kinhank), channel, player slot, extension type. Per row: drain time, event index, estimated time + method flag, `gforce` xyz, raw accel xyz + saturation flags, `data_present`, `err`, dropped-events delta, IR valid/x/y, B state, phase, `ē`, `v`. For P1 validation, plus an independent speed label (§IMPL_PLAN P1).

Why this is speed and not distance: power comes from the peak of an 80 ms window of an instantaneous signal. Swing length and hold time are never measured or accumulated.

### 4.5 MotionPlus (optional refinement, after the plain-remote path passes P1)

If a MotionPlus or Wii Remote Plus is detected (`WPAD_SetMotionPlus(chan, 1)`, `exp.type == EXP_MOTION_PLUS`, raw `mp.rx/ry/rz`, zeroed at rest, fast-mode scale ×2000/440), the pitch rate can (a) confirm lobe direction (C4) and (b) extend range above the accelerometer rail. Gyro rate alone doesn't give hand speed when the wrist articulates, so it refines and never replaces the plain-remote path. It is never required.

### 4.6 Aim

- IR pointer x maps linearly to `aim ∈ [−0.12, +0.12]` rad (the host's `AIM_MAX`; the trackball used ±0.10), centered on the board. The reticle is drawn on the board plane, and the existing power arrow previews the line.
- Aim **locks on B press**, so swing wobble can't change it. This keeps "aim where you point" readable.
- No IR (no sensor bar, or pointing off-screen for more than 0.5 s): the reticle freezes at its last value and **−/+** nudge it. The HUD shows "AIM: − +".

### 4.7 Spin / curl via wrist roll

- Roll is computed from **calm `gforce` samples** (`atan2` of the X and Z components, relative to neutral: remote flat, buttons up). libogc's `orient`/`a_roll` is an orientation result that its own dynamics code treats as unreliable under high acceleration, so it's used only for display, never for locking. Deadzone ±10°, full spin at ±45°, linear in between, scaled by `spinStrength`.
- Sign: twist **right** (clockwise from the player's view) → **negative** spin → clockwise from above → curls **right**. This matches 1.0.3's convention (A/back-left = negative = right curl). Left-handed mode mirrors it.
- Spin **locks on B press**, as the mean roll of the stable 150 ms window that the arming check already requires (§4.4). If that window isn't stable, B doesn't arm. During the swing, accelerometer roll is corrupted by motion and is ill-conditioned when the remote points down, so it's never read there. Spin affects landing curl only, never flight curvature, as in 1.0.3.

### 4.8 Underhand feel

- The bag is visibly held at the release point (`positionHeldBag`). On B press it dips slightly; during the backswing it follows a short arc driven by the measured swing phase (cosmetic only).
- Release timing doesn't change launch angle. The throw height (SLIDER/NORMAL/KOBE) sets it, as in 1.0.3. This keeps outcomes predictable and the WIN tables valid.
- Rumble: tiny tick when the bag leaves the hand.

---

## 5. Testing strategy — Dolphin first, then hardware

### 5.1 Layers

| Layer | Where | What |
|---|---|---|
| Core golden tests | Host (`make host-test`), every change | C core vs vectors from a **read-only deterministic oracle harness** (`wii/tools/oracle/`, §5.1.1), not the page's narrow `__game` hooks alone. Tolerances: identical state classification and transition sequence; positions within 1 mm; WIN bounds within one 0.0025 step |
| Swing detector tests | Host | Replays recorded swing traces (schema in §4.4) through `swing.c`: the P1 adversarial set gives zero false throws; slow-long < 0.4 power and short-fast > 0.7 within the supported gesture; results compared with **independent** speed labels, not the estimator's own output |
| Dolphin — emulated Wiimote | Dev PC | Boot, menus, full matches, IR via mouse ("Point"), Motion Simulation "Swing" bound to a key/button for smoke-test throws, "Tilt" for roll/curl. Good for flow, **not** for power feel |
| Dolphin — real Wiimote | Dev PC (desktop Dolphin) or an Android device, Wiimotes via a DolphinBar in Mode 4 (matches the Kinhank setup) | Power feel, curl, IR via the DolphinBar's built-in sensor bar, report rate and dropped events, two remotes |
| **Kinhank (Dolphin Android 5.0-19870)** | Retro installs the ISO + `GameSettings/BYCEAO.ini` over ADB (§1.6) | Full speed at Vulkan 1×, real Wiimotes via DolphinBar Mode 4, game ID and per-game Real Wiimote setting picked up, 2 PLAYER with two remotes, saves on Dolphin's virtual SD |
| Real Wii | HBC | Frame time, video modes, SD saves, HOME/quit, long-session stability, the final feel test |

### 5.1.1 Oracle harness (why the page hooks aren't enough)

`window.__game` hooks are narrow. `_probe(power, arc, spin)` always uses team 0, slick, `RX[0]`, aim 0, drift 0, and raw launch power (bypassing the human KOBE remap), and returns only the final position, state and approximate flight time. `_place`/`_slide` make restricted fixtures. `throwWith` runs through live timers and `Math.random`. A C port could match these hooks and still fail normal play.

So the oracle follows the pattern `arcade/cabinet-snapshots/1.0.3/tests/spin.test.mjs` already uses. It **slices the pinned snapshot's `index.html` source** (constants, board transform, `newBody`/`launch`/`step`/`collide`, `lie`, `scoreInning`, `planCareerShot`, `calibratedWindow`/`physicalPower`) and evaluates it with `node:vm` against `three@0.170.0`, **without editing any game file**. The harness adds:
- deterministic fixed-step stepping in the original loop order (§2.1), with per-substep state export/import;
- full control of team, side, arc, stance (`rx`), aim, drift, spin, wind, and **both** input power and physical (KOBE-remapped) power;
- a seeded RNG injected in place of `Math.random`, and a manual clock in place of timers, recording RNG consumption;
- source SHA-256s, fixture initial state, timestep and the expected **transition sequence** (fly → slide/board, ground → grass → removed, hole → in, off) in every vector file.

Required fixtures:
- all six arc × surface combinations for both teams and several stances, with wind on and off;
- front-edge impact; grass-first stop and removal; side and back fall-off; swish vs slide-in; hanger callout;
- rider/stacked contact; simultaneous knock-in + knock-off; resting bag re-woken;
- a wash keeping throw order; a net score crossing 21; settle-then-score-once timing; throws consumed;
- `planCareerShot` decisions per rival on empty and crowded boards with fixed seeds (selected arc, side, spin, power, aim, pool, RNG draws);
- `roundWind` sequences per circuit and the §2.3 mode policy.

### 5.2 Dolphin setup notes

- Boot the `.dol`/`.elf` with File → Open, or from the game list. Keep `boot.dol` **outside** the virtual SD. Files the app reads at runtime go inside Dolphin's virtual SD (`sd.raw`; "Automatically Sync with Folder" can mirror a host folder, but it is slow with large cards). You can also install HBC into Dolphin's NAND and launch from `apps/` for a true HBC check.
- Known Dolphin limitation (issue #12939): loose ELF/DOL files can't boot from a folder-based virtual disc. Launch from a normal path.
- Real Wiimotes: the standard setup is a **MAYFLASH DolphinBar in Mode 4** (USB), the same as on the Kinhank. It also provides the IR sensor bar. Desktop Dolphin can also use the host's Bluetooth stack or Bluetooth passthrough of a compatible USB adapter, but the DolphinBar keeps dev and Kinhank behavior the same.
- Per-game Real Wiimote: boot the **ISO** (game ID `BYCEAO`) so `GameSettings/BYCEAO.ini` applies (§1.5). A bare `.dol` needs the global "Real Wii Remote" fallback.
- NTSC only: keep Dolphin's Wii settings at NTSC/60 Hz (no PAL60/EuRGB60 forcing in Dolphin).
- Motion Input from a device with a real IMU (DSU client, or SDL on a DualSense/Switch Pro) is a middle ground for testing power without a Wiimote. The mapping differs from a real remote, so final tuning still uses a real Wiimote.
- Dolphin performance is **not** a proxy for Wii performance, and desktop Dolphin is not a proxy for the Kinhank. Measure frame time on the Wii; check full speed on the Kinhank (Retro).

### 5.3 Hardware loop

- `wiiload boot.dol` to HBC over the network (devkitPro tool) for fast iteration; SD card copy as the fallback.
- On-screen dev overlay (dev builds only, hidden in release): frame time, triangles, reports/sec, dropped events, live `ē` / peak / power / roll.
- Logs and swing traces go to `sd:/apps/backyardcornhole/data/`. The same code path works in Dolphin's virtual SD.

---

## 6. Player-facing copy principles

1. **Short.** Prompts are 1–4 words; new callouts 1–2. ALL CAPS pixel font, like 1.0.3. **Exempt:** the established 1.0.3 callouts (ON THE BOARD, OFF THE BOARD, KNOCKED IN!, etc.), rival names and taunts, and the save warning. They're kept verbatim.
2. **Verb first.** "HOLD B · SWING · LET GO", "TOSS HARD", "POINT AT SCREEN".
3. **No tech words on screen.** Never show g, counts, IR, accelerometer, MotionPlus, calibration, sensor bar, Bluetooth or version numbers in play (the version can appear in the pause menu footer).
4. **Reuse the game's voice.** Keep CORNHOLE!, HANGER!, ON THE BOARD, GROUNDER, KNOCKED IN!, WASH, LET'S TOSS, CORNHOLE KING!, and the rival taunts verbatim.
5. **No coaching boxes.** 1.0.3 removed post-throw strength/direction coaching; the colored power arrow is the feedback. Keep it that way.
6. **Help is optional.** It never blocks play; button **1** toggles it, and the choice is saved.

| Situation | Copy |
|---|---|
| First throw hint (help shown) | HOLD B · SWING · LET GO |
| Help legend | B TOSS · A SURFACE · ✚ HEIGHT/STEP · 2 RESET · TWIST CURL |
| No pointer | POINT AT SCREEN / AIM: − + |
| Remote missing (slot-correct) | CONNECT REMOTE 1 / CONNECT REMOTE 2 (+ `A: USE REMOTE 1` for slot 2) |
| B pressed while moving | HOLD STILL |
| Practice pause | CLEAR BOARD |
| Low battery | LOW BATTERY |
| Power setup (settings) | SET POWER → TOSS HARD → POWER SET! |
| Strap reminder (boot) | STRAP ON! |
| Save problem | Progress won't save on this device. |
| Quit | QUIT? YES / NO |

---

## 7. Feature parity checklist (Wii v1)

| Feature (1.0.3) | Wii v1 |
|---|---|
| Practice with live stats, clear board | Yes (CLEAR BOARD in the practice pause menu; unlimited retained bags, §3.2) |
| 2 Player (same screen) | Yes (2 remotes or pass-and-play) |
| Tournament: 8 rivals, 4 circuits, wind tiers, saves, champion | Yes |
| Throw heights SLIDER/NORMAL/KOBE, SLICK/STICKY | Yes |
| Spin/curl | Yes (wrist roll) |
| Shared per-round crosswind, pennant, wind panel, streaks | Yes, with the arcade's mode policy: calm by default in Practice/2 Player, WIND setting, circuit wind in Tournament (§2.3); streaks first to cut |
| Live board close-up | Yes |
| Power gauge with 3 s last-shot linger, colored arrow | Yes |
| Dog + celebrations, confetti | Yes |
| Party NPCs (keg, beer pong scripts, chess, cook) | Static at interim milestones; **animated for release** (P6) unless Ashe descopes it (§9) |
| Music playlist, cabinet setup, trackball calibration, Electron packaging | No |

---

## 8. Risks

| # | Risk | Impact | Mitigation |
|---|---|---|---|
| 1 | **The accelerometer speed proxy isn't valid enough** (P0-class): `|a|−1g` is a heuristic; pendulum radius changes skew it ~2×; clipping at ~±3 g including gravity; estimated timestamps; forward/backward can't be read from acceleration sign | Core mechanic feels random, or false throws | Treated as a candidate, not a fact (§4.4): supported-gesture envelope, stability-gated arming, lobe-timing classifier with IR/attitude confirmation, gap cancel, clip-capped calibration; **P1 exit gate** with independent speed labels and zero false throws on adversarial traces, on Wii **and** Kinhank. Failing it means revise and repeat P1; MotionPlus doesn't count as the fix |
| 2 | **Dolphin ≠ hardware for motion**: emulated Swing is canned; DolphinBar Mode 4 timing may differ from the Wii's own Bluetooth (report rate, dropped events) | "Works in Dolphin" can hide feel bugs | Emulated motion is a smoke test only; real Wiimote via DolphinBar from P0; swing traces recorded on Wii **and** Kinhank and compared before tuning is locked |
| 3 | **GX rewrite and performance**: replacing Three.js (lighting, shadows, sRGB look) and fitting the party scene in budget; Dolphin perf isn't predictive | Visual mismatch or < 60 fps | Exporter reuses the exact JS scene code; baked lighting; cut list in §3.3; hardware frame-time checks from P1 onward |
| 4 | Physics parity drift (float vs double, matrix reconstruction, loop order, per-bag flags) | CPU calibration and WIN windows off; shots or callouts differ from arcade 1.0.3 | Doubles; deterministic oracle harness sliced from the pinned snapshot (§5.1.1) with transition fixtures; tolerant WIN check on device |
| 5 | CPU tournament search cost on 729 MHz and Kinhank (unmeasured) | Frame hitches on CPU turns | Step-granular resumable search during the windup; budgets set from P1/P4 benchmarks on both targets |
| 6 | Toolchain churn: libogc 3.x vs GRRLIB/libogc2 fork compatibility | Broken builds mid-project | Pinned Docker image; own 2D layer so GRRLIB is optional |
| 7 | WPAD event buffering behaves differently on a target, or is lost on reconnect | Lost samples | Verified in P0; re-registration on every reconnect; one queue owner; a documented alternative acquisition path chosen in P0 if needed (`WPAD_ReadPending` is the same queue, not a fallback) |
| 8 | Scope: every screen and the HUD must be rebuilt | Schedule | Phase order puts a playable Dolphin match before polish; party animation last |
| 9 | Licensing: font RFN, SFX sources | Redistribution problems | OFL text shipped, font not renamed "Press Start 2P"; CC0-only SFX with saved source pages |
| 10 | Player safety/ergonomics with a swung remote | Thrown remotes | Boot reminder "STRAP ON!"; auto-release so players don't have to fling to let go |
| 11 | **Kinhank (Dolphin Android, Mali at 1×) can't hold full speed** even though MKWii/Wii Sports do: our draw pattern may hit Dolphin costs those games avoid (shader compiles, EFB access, many state changes) | Slowdown on the second required target | §3.4 rules; stay at or below MKWii/Wii Sports complexity; Retro checks full speed at P1, P2, P3 and P6; SPEC §3.3 cut list applies to both targets |
| 12 | **ISO packaging incomplete or per-game setting not applied** (missing `boot.bin`/apploader, non-redistributable apploader, wrong ID/ini path, bare `.dol`) | Game won't boot as an ISO, or boots with an emulated remote | Complete disc tree with recorded provenance (§1.5); P0 proof (ID, region, boot, DOL hash); Retro checks the ID; bare-DOL + global-Real fallback kept working; WAD only with its own proof |
| 13 | PAL/50 Hz slips in (PAL console setting, Dolphin settings) | Slow game on the Kinhank | NTSC-only video modes in code (no EURGB60/PAL branch); USA disc metadata; tests with a PAL console setting; release check on the Kinhank |
| 14 | Practice bag accumulation (no 8-bag cap in practice) | Frame drops in long practice sessions | 48-bag benchmark on both targets; any retention change needs Ashe's approval (§3.2) |

---

## 9. Open questions for Ashe

Already answered, so not repeated here: Kinhank model, OS (Android 12), Dolphin version and settings, Wiimote connection (DolphinBar Mode 4), install path, NTSC/60 Hz, who installs and tests (Retro), and that the arcade cabinet isn't a Wii target.

**Wii and Wiimotes**
1. The real Wii's region and video cable: component (480p) or composite (480i)? TV 4:3 or 16:9? (The build outputs NTSC/60 Hz only. On a PAL-region Wii, that requires a TV that accepts NTSC/60 Hz.)
2. HBC version and SD card size? Is the Wii on Wi-Fi (for `wiiload`)?
3. Which remotes: original Wii Remote, Wii Remote Plus (built-in MotionPlus), or add-on MotionPlus? How many?
4. Two players: two remotes, or pass one?
5. Left-handed mode needed in v1?

**Kinhank**
6. Exact SoC / Mali model (still unknown). Not blocking: Retro's full-speed checks are the real test.
7. Confirm, on the box with 5.0-19870 (Retro, at the P0 check): the `GameSettings/` path, that the virtual SD is enabled and writable, and how to copy traces off.
8. OK with game ID `BYCEAO`? (Pending a GameTDB collision check.)

**Scope and feel**
9. v1 must-haves: are all three modes (Practice, 2 Player, Tournament) required for the first release? The release plan includes animated party NPCs. OK to descope to static props if time runs short?
9a. If long practice sessions can't hold 60 fps beyond the measured bag count, may the game offer an automatic "CLEAR BOARD?" prompt? (This changes 1.0.3's behavior.)
9b. Wind: keep the arcade's default (calm in Practice/2 Player unless WIND is ON), or default to wind on for the Wii?
10. Curl: OK to set it by twisting **before** you grab (recommended), rather than twisting during the swing?
11. Aim with no sensor bar: is −/+ nudging acceptable? (The DolphinBar includes a sensor bar, so this mostly matters on the real Wii.)
12. Music on Wii: drop it, or support OGG files on the SD card later?
13. Personal use only, or a public release (e.g., Open Shop Channel)? (Code location is proposed in [REPO_LAYOUT.md](REPO_LAYOUT.md).)
14. Full-power feel: should the default match the cabinet's (a full-power toss takes real effort), or be easier for couch play?

---

## 10. Decisions from review (Codex adversarial review, 2026-11-15)

Source: [REVIEW_CODEX.md](REVIEW_CODEX.md). IDs refer to that review.

**Accepted and applied in this doc**
- **I1/I2 (speed proxy, direction):** §4.4 is now a *candidate* detector with a P1 exit gate. The formula's limits (scalar gravity, radius error) are stated; there's a supported-gesture envelope, stability-gated arming, and lobe-timing / follow-through / apex-attitude classifier candidates. Acceleration sign is no longer the classifier. MotionPlus doesn't count as the fix.
- **I3:** auto-release in speed units (`v ≤ 0.6·peak_v` ≡ `ē ≤ 0.36·peak_ē`); window validity by elapsed span ≥ 16 ms, not sample count; clipping counted with gravity included, the `ē ≈ 2 g` default withdrawn, calibration capped at the clip ceiling; saturation only caps an already validated toss.
- **I4:** use libogc's calibrated `gforce` (there's no `accel_calib` in `WPADData`); `ReadPending` isn't a fallback; re-register buffers on reconnect; timestamps flagged as estimated, gaps cancel; trace schema with version and tunables; roll from calm `gforce`, not `a_roll`.
- **I5:** complete ISO tree (`boot.bin`, redistributable apploader with recorded provenance, `main.dol`, `files/`), USA region, pinned WIT, P0 proof with DOL hash. WAD is conditional and not interchangeable.
- **I6:** EURGB60 removed; NTSC-only video in code; video, disc metadata and Dolphin config are separate requirements.
- **I7:** action-legality table; armed inputs ignored; disconnect pauses and discards the gesture; slot-correct prompts; profiles per player slot.
- **G1/G3:** deterministic oracle harness sliced from the pinned snapshot (§5.1.1) with transition fixtures; loop order and per-bag flags are contract.
- **G2:** practice has no bag cap → CLEAR BOARD in the practice pause menu, 48-bag benchmark, retention change only with Ashe's approval.
- **G4:** the AI is described as a single-bag `step()`-only predictor; parity by fixed-seed choices; rate checks secondary; timings marked unmeasured; step-granular yielding.
- **G5:** effective FOV 18°; bag down-face material rule; visual spin separate from curl; fresh 1.0.3 screenshots as reference.
- **G6:** PIP clears inset color and depth and restores state; 30 Hz PIP skipping removed.
- **G7:** export manifest (repeat/offset, groups, winding, alpha/depth, pivots), sRGB/linear bake, SVG portrait decoder, big-endian versioned binaries, tolerant host/PPC parity.
- **G8:** wind defaults follow the arcade controller (calm unless WIND is ON; circuit wind in Tournament).
- **G9:** one save path, explicit big-endian serialization with crash-safe writes, career semantics preserved; 3-word legacy callouts exempt; glyph repertoire and 480i sizing; 60 fps is the contract (no 30 fps fallback); animated NPCs required for release unless descoped.

**Rejected / not adopted (mentioned for the record)**
- **R7, partly:** "the claim that all four tags are on GitHub needs a real remote check." That check *was* done on 2026-11-15 with `git ls-remote --tags origin`, which returned all four annotated tags. We still re-check remote state when the migration PR is prepared (see REPO_LAYOUT). Only the `origin/main` position was a cached ref.
- **R6, partly:** excluding README/CHANGELOG files from the deployed site. Today's site already serves them harmlessly, and they're useful as public docs. Tests, screenshots and tool output *are* excluded (REPO_LAYOUT).

---

## Sources

- libogc `wpad.h` / `wiiuse.h` (WPAD event buffers, data formats, MotionPlus, `orient_t`, `accel_t`): https://github.com/devkitPro/libogc (headers under `gc/wiiuse/`), releases https://github.com/devkitPro/libogc/releases (v3.2.1, 2026-10-15)
- GRRLIB 4.6.0 changelog: https://grrlib.github.io/GRRLIB/ · fork with libogc2 port: https://codeberg.org/extremscorner/GRRLIB
- Homebrew Channel app layout and icon: https://wiibrew.org/wiki/HBC · https://wii.hacks.guide/hbc · https://gamebrew.org/wiki/Homebrew_Channel_Wii
- Dolphin virtual SD and homebrew booting: https://dolphin-emu.org/docs/guides/virtual-sd-card-guide · loose ELF/DOL issue: https://bugs.dolphin-emu.org/issues/12939
- Dolphin real Wiimote connection: https://dolphin-emu.org/docs/guides/wii-remote-plus-rvl-cnt-01-tr-connection-guide · RetroDECK Dolphin Wii Remote guide: https://retrodeck.readthedocs.io/en/latest/wiki_emulator_guides/dolphin/dolphin-wii-remote/
- Dolphin motion input notes: https://wiki.archlinux.org/title/Dolphin_emulator · https://bugs.dolphin-emu.org/issues/11776
- Dolphin per-game Wiimote source: `Source/Core/Core/HW/Wiimote.h` (`WiimoteSource { None=0, Emulated=1, Real=2 }`) and `Source/Core/Core/ConfigLoaders/GameConfigLoader.cpp` (`[Controls] WiimoteSource0..3`), https://github.com/dolphin-emu/dolphin
- WIT image composing (required tree: `sys/boot.bin`, `sys/apploader.img`, `sys/main.dol`, `files/`; game ID): https://wit.wiimm.de/info/composing.html · https://wit.wiimm.de/wit
- libogc WPAD implementation (event buffers, `ReadPending`, disconnect clears buffers): https://raw.githubusercontent.com/devkitPro/libogc/v3.2.1/wiiuse/wpad.c · orientation under high acceleration: https://raw.githubusercontent.com/devkitPro/libogc/master/wiiuse/dynamics.c
- Codex adversarial review: `/workspace/docs/cornhole/wii-port/REVIEW_CODEX.md` (2026-11-15)
- Kinhank / Dolphin Android facts: Retro (`/tmp/wii-port-prompts/kinhank-facts.md`, 2026-11-15)
- Wiimote accelerometer (ADXL330, at least ±3 g) and MotionPlus: https://wiibrew.org/wiki/Wiimote · https://wiibrew.org/wiki/Wiimote/Extension_Controllers · 100 Hz reporting: https://arxiv.org/pdf/1309.2093
- Repo: `arcade/cabinet-snapshots/1.0.3/` (READ-ME.txt, DEV-NOTES.txt, CODEX_SUMMARY.md, sources above), `CHANGELOG.md`, `arcade/AUDIO.md`, `/workspace/docs/cornhole/spin-research.md`, reference stills in `/workspace/docs/cornhole/preview-stills/`
