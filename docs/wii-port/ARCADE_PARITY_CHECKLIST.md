# Arcade 1.0.3 → Wiimote parity checklist

Owner directive, Nov 15 2026. One row per feature from the pinned 1.0.3 controls and SPEC §2. **Status** describes the
Wiimote path; **Game (wii/game)** says whether the game branch implements it and where. No camera/pointer/sensor bar
is used anywhere (WPAD format `BTNS_ACC` only). Power remains swing SPEED, never distance.

Legend: **mapped** = same function, Wiimote control; **adapted** = same purpose, Wii-appropriate form;
**N/A** = cabinet hardware only (reason given); **open** = not implemented yet (tracked in FOLLOWUPS.md).

| Arcade feature | Wiimote control / screen | Status | Game (wii/game) |
|---|---|---|---|
| Forward throw | Hold B, backswing, pause, forward swing, release B | mapped | done — `INPUT_THROW` → `game_input` → `game_throw` |
| Peak-speed power / 80 ms window | Forward lobe speed peak, per-slot calibration | mapped | done — `toss.power` → `physical_power` (1.0.3 KOBE remap) → `launch` |
| Valid sample / anti-jitter gate | 16 ms window, 0.35 g lobe for 40 ms | mapped | input layer |
| Manual release | B-up during classified forward lobe | mapped | done |
| Idle / speed-drop automatic release | Hold B through forward finish, speed ≤ 60% peak | adapted | done (input layer) |
| Stroke-length release gate | No measured distance; B release / speed drop / timeout | adapted | input layer |
| 2.5 s timeout | Valid confirmed lobe throws, otherwise cancel | mapped | input layer |
| 450 ms quiet / one throw per gesture | Settling and spent phases | mapped | done — `input_next_turn` per human bag, incl. each practice throw |
| Forward-left gesture 1 | Lateral left forward swing | adapted | done — `toss.aim` → launch yaw |
| Forward-right gesture 3 | Lateral right forward swing | adapted | done |
| Straight forward gesture 2 | Straight swing from stance, no auto-target | mapped | done — human yaw base `baseYaw(RX[t])` = straight down-range; host test |
| Back-left A spin / clockwise / right curl | Lateral movement and right twist at release | adapted | done — `toss.spin` → `launch(..., spin)` → `landingCurl` |
| Back-right C spin / counterclockwise / left curl | Lateral movement and left twist at release | adapted | done |
| Straight backswing B / no spin | Neutral lateral movement and twist | adapted | done (zero spin) |
| Backswing braking / back-apex release | Back pause clears peak; B-up without new lobe cancels | mapped | input layer |
| Height KOBE / NORMAL / SLIDER | D-pad UP/DOWN | mapped | done — `INPUT_HEIGHT`; status line shows height |
| SLICK / STICKY | A in play | mapped | done — `INPUT_SURFACE`; held bag shows the up face |
| Reset held bag (Top 3 / Bottom 3) | 2 | mapped | done — re-centres stance/aim/spin, no throw consumed |
| Nudge left / right | D-pad LEFT/RIGHT, 0.05 steps, ±0.45, 70 ms ease | mapped | done — `input_stance` read every frame |
| KEYS legend pop / tuck | 1 toggles help | adapted | done — help legend from `input_help_text`, one item per line; saved as `help_tucked` |
| Pause (Start / Select) | HOME cancels gesture and opens pause | mapped | done — physics and timers freeze |
| Menu navigation | D-pad, A confirm, B back, minus/plus adjust | mapped | done |
| Main menu | PRACTICE / 2 PLAYER / TOURNAMENT / SETTINGS / QUIT | adapted | done |
| Resume | HOME → RESUME, A (B up required) | adapted | done — `input_resume` retried until B is up |
| Practice mode | Main menu → PRACTICE | adapted | done — unlimited bags, IN/ON/% HIT/THROWS |
| 2 player mode | Main menu → 2 PLAYER | adapted | done — Remote 2 or pass-and-play ("PASS THE REMOTE") |
| Versus CPU / challenge | TOURNAMENT rivals | adapted | done — career planner; casual CPU kept as code path (test mode, not a menu mode, as 1.0.3) |
| Tournament map / circuits | D-pad, A, B | adapted | done — ROAD TO CORNHOLE KING ladder |
| Continue career | TOURNAMENT → CONTINUE | adapted | done |
| New career confirmation | TOURNAMENT/champion → NEW CAREER → YES/NO | adapted | done — resets to 0/0 and saves |
| Rival introduction / roster / taunts | A LET'S TOSS, B back | adapted | done — portrait, nickname, taunt, style, air |
| Tournament result / next rival / retry | NEXT UP / TRY AGAIN / CIRCUIT MAP | adapted | done — fight saved only when won |
| Champion screen | CIRCUIT MAP / NEW CAREER | adapted | done |
| Pause CLEAR BOARD | Practice pause → CLEAR BOARD | adapted | done — clears bags, keeps stats; separate from 2 |
| Pause MAIN MENU | HOME → MAIN MENU | adapted | done |
| EXIT GAME / quit | HOME → QUIT → YES (also title QUIT) | adapted | done — returns to the Homebrew Channel loader |
| CABINET SETUP screen | SETTINGS | adapted | done |
| FULL POWER adjustment | SETTINGS → RED/BLUE POWER − / + (% of default) | adapted | done — `input_set_profile`, clamped to the calibration limits |
| CALIBRATE TRACKBALL / cancel | SETTINGS → SET POWER → TOSS EASY ×3; HOME cancels | adapted | done — limits provisional until P1 traces |
| Backswing spin strength / CURL | SETTINGS → RED/BLUE CURL − / + | adapted | done |
| LEFT / RIGHT HAND | SETTINGS and pause HAND rows; D-pad LEFT/RIGHT or A | mapped | done — `input_focus_hand`, `INPUT_PROFILE_CHANGED` saved per slot |
| WIND toggle | SETTINGS → WIND CALM/ON | adapted | done — arcade policy (calm default in Practice/2 Player, circuit wind in Tournament) |
| RESET SETTINGS | SETTINGS → RESET SETTINGS | adapted | done |
| Settings BACK | B or BACK | adapted | done |
| Spinner sensitivity | — | N/A — cabinet trackball counts; swing full speed replaces it | — |
| USB / keyboard bindings | Fixed Wiimote button map | N/A — cabinet USB/keyboard hardware | — |
| Cabinet raw diagnostics / capture error | CONNECT REMOTE n; A: USE REMOTE 1 | adapted | done — play freezes; no throw is accepted while frozen |
| Two controllers / fallback controls | Remote 1/2 or pass-and-play | mapped | done |
| Keyboard fallback / F2 | Dev overlay (hold 1+2 at boot) | adapted | **open** — OSReport logging instead; overlay not built |
| Scoreboard / team colors / pips | Persistent HUD | adapted | done — TOTAL/RND, pips, ROUND n / TO 21 / FINAL, round edge |
| Turn banner / CPU turn | HUD; input disabled during flight/CPU | adapted | done |
| Power meter / five-stop color ramp | LAST SHOT gauge from release (no live fill, per IMPL_PLAN P1) | adapted | done |
| LAST SHOT meter / 3 s linger | Post-throw HUD | adapted | done |
| Power direction arrow | Shot arrow from the bag after release | adapted | done (2.4 s) |
| Wind HUD / pennant / streaks | Wind panel, pennant | adapted | done for panel + pennant; streaks **open** (first on the SPEC §3.3 cut list) |
| Board View inset | BOARD · 27 FT second viewport | adapted | done — clears colour + depth, restores state |
| Held bag / height / surface feedback | Held bag + status line | adapted | done — B-held dip |
| Callouts / landing results | CORNHOLE!, HANGER!, ON THE BOARD, GROUNDER, OFF THE BOARD, MISS, KNOCKED IN!/OFF!, WASH | adapted | done — verbatim |
| Round scoring / summary | Cancellation scoring after eight settled throws | adapted | done |
| Match over / winner | FINAL, YOU WIN!/CPU WINS/RED WINS! | adapted | done |
| Practice statistics | Practice HUD | adapted | done |
| Career records / progression save | Tournament screens + save.dat | adapted | done |
| Saved input profiles | Per-slot full speed, curl, hand | adapted | done — save.dat |
| Version label / menu kicker | "WII · 0.1.0" on the title, v0.1.0 in pause | adapted | done |
| Dog celebration / confetti / dust / trail | Scene | adapted | done |
| Party NPCs (keg, beer pong, chess, cook) | Scene | adapted | **open** — SPEC: animated for release (P6) |
| MUSIC menu, play/pause, next, folder, refresh, volume, back | MUSIC screen | adapted | **open** — SPEC §2.7 drops music for v1; owner question §9 Q12 still open |

Verification on wii/game: host input 33 + WPAD 7, physics golden 938/938, game tests 601/601; arcade 24/24; pinned
snapshot 48/48; shared drift 32/32. Dolphin 2609a: ISO and bare DOL boot; scripted soak (dev build) plays full matches.
P1 hardware acceptance remains open: both hands, speed labels, backswing/pause/forward traces, lateral/twist separation,
zero false throws, Kinhank speed.
