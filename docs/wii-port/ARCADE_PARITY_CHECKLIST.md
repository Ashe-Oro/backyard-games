# Arcade 1.0.3 → Wiimote parity checklist

Owner directive, Nov 15 2026. One row per feature from the pinned 1.0.3 controls and SPEC §2. Status describes the control/screen mapping, not completed game integration. **Adapted — game branch** rows explicitly name the pending consumer; this input-only branch does not implement rendering, physics, audio or saves. No camera/pointer/sensor bar is used. Power remains swing SPEED, never distance.

| Arcade feature | Wiimote control / screen | Status / reason / integration hook |
|---|---|---|
| Forward throw | Hold B, backswing, pause, forward swing, release B | mapped — swing_feed → INPUT_THROW |
| Peak-speed power / 80 ms window | Forward lobe speed peak, per-slot calibration | mapped — duration/distance never grant power |
| Valid sample / anti-jitter gate | 16 ms window, 0.35 g lobe for 40 ms | mapped — swing_feed |
| Manual release | B-up during classified forward lobe | mapped — immediate throw; frozen aim/spin |
| Idle / speed-drop automatic release | Hold B through forward finish, speed ≤ 60% peak | adapted — sustained calm finish gates automatic release |
| Stroke-length release gate | No measured swing distance; B release / speed drop / timeout | adapted — time/speed guards replace count-distance bound |
| 2.5 s timeout | Valid confirmed lobe throws, otherwise cancel | mapped — swing_feed |
| 450 ms quiet / one throw per gesture | Settling and spent phases | mapped — two-sided calm; reset for next turn |
| Forward-left gesture 1 | Lateral left forward swing | adapted — INPUT_THROW.toss.aim |
| Forward-right gesture 3 | Lateral right forward swing | adapted — INPUT_THROW.toss.aim |
| Straight forward gesture 2 | Straight swing from stance, no auto-target | mapped — zero aim with zero trim |
| Back-left A spin / clockwise / right curl | Lateral movement and right twist at release | adapted — game branch feeds INPUT_THROW.toss.spin to existing landing physics |
| Back-right C spin / counterclockwise / left curl | Lateral movement and left twist at release | adapted — game branch feeds INPUT_THROW.toss.spin to existing landing physics |
| Straight backswing B / no spin | Neutral lateral movement and release twist | adapted — zero signed spin |
| Backswing braking / back-apex release | Back pause clears peak; B-up without new lobe cancels | mapped — physical regressions, both hands |
| Height KOBE / NORMAL / SLIDER | D-pad UP/DOWN | mapped — INPUT_HEIGHT |
| SLICK / STICKY | A in play | mapped — INPUT_SURFACE |
| Reset held bag (Top 3 / Bottom 3) | 2; center stance and clear windup | mapped — INPUT_RESET_BAG; no throw consumed |
| Nudge left | D-pad LEFT | mapped — INPUT_STANCE, step 0.05, clamp ±0.45, ease 70 ms |
| Nudge right | D-pad RIGHT | mapped — INPUT_STANCE, same constants |
| KEYS legend pop / tuck (Top 4 / Bottom 4) | 1 toggles help | adapted — INPUT_HELP; game branch renders legend and saves helpTucked |
| Pause (Start / Select) | HOME cancels gesture and opens pause | mapped — INPUT_PAUSE |
| Menu navigation | D-pad, A confirm, B back, minus/plus adjust | mapped — INPUT_NAV_*, INPUT_CONFIRM/BACK/ADJUST_* |
| Main menu | PRACTICE / 2 PLAYER / TOURNAMENT / SETTINGS | adapted — game branch menu screen consumes menu events |
| Resume | HOME pause → RESUME, A | adapted — game branch calls input_resume, requires B up after reconnect |
| Practice mode | Main menu → PRACTICE, A | adapted — game branch practice start; unlimited bags and stats preserved |
| 2 player mode | Main menu → 2 PLAYER, A | adapted — input_start_match / input_next_turn; game branch owns match |
| Versus CPU / challenge | TOURNAMENT rival selection and challenge screens | adapted — game branch career controller; casual CPU retained as test mode, not a new menu mode |
| Tournament map / circuits | D-pad selection, A confirm, B back | adapted — game branch map and circuit screens |
| Continue career | TOURNAMENT → CONTINUE, A | adapted — game branch reads saved fight/wins |
| New career confirmation | TOURNAMENT → NEW CAREER, A; confirm or B cancel | adapted — game branch confirmation and reset |
| Rival introduction / roster / taunts | A to start, B back | adapted — game branch intro screen |
| Tournament result / next rival / retry | D-pad and A, B back | adapted — game branch result panel |
| Champion screen | A continue / menu | adapted — game branch champion screen |
| Pause CLEAR BOARD | Practice pause → CLEAR BOARD, A | adapted — game branch clears thrown bags while keeping practice stats; separate from 2 |
| Pause MAIN MENU | HOME → MAIN MENU, A | adapted — game branch returns to main menu |
| EXIT GAME / quit | HOME → QUIT, A | adapted — game branch returns to Homebrew Channel |
| CABINET SETUP screen | SETTINGS screen with button navigation | adapted — game branch renders settings, consumes INPUT_ADJUST_* |
| FULL POWER adjustment | SET POWER plus manual full-speed adjustment | adapted — game branch input_set_profile per slot |
| CALIBRATE TRACKBALL / cancel | SET POWER → TOSS HARD (three valid tosses); B cancel | adapted — input_begin_calibration / input_cancel_calibration; game branch screen |
| Backswing spin strength / CURL | SETTINGS → CURL; minus/plus | adapted — game branch updates profile.spin_strength via input_set_profile |
| LEFT / RIGHT HAND | Setup/pause HAND row per slot; D-pad LEFT/RIGHT | mapped — input_focus_hand; INPUT_PROFILE_CHANGED; saved with calibration by game branch |
| WIND toggle | SETTINGS → WIND CALM/ON; A | adapted — game branch preserves calm default practice/2-player and tournament circuit policy |
| RESET SETTINGS | SETTINGS → RESET, A | adapted — game branch restores defaults and slot profiles |
| Settings BACK | B or BACK row + A | adapted — game branch returns to menu |
| Spinner sensitivity | No Wiimote screen | N/A — cabinet trackball counts only; swing full speed replaces sensitivity |
| USB / keyboard bindings | Fixed Wiimote button map | N/A — cabinet USB/keyboard hardware only |
| Cabinet raw diagnostics / capture error | Disconnect pause with CONNECT REMOTE n; A USE REMOTE 1 | adapted — INPUT_CONNECT_REMOTE, reassignment and B-up resume gate |
| Two controllers / fallback controls | Remote 1/2 or pass-and-play Remote 1 | mapped — slot-owned calibration/hand and match-start channel assignment |
| Keyboard fallback / F2 | Development-only 1+2 boot overlay | adapted — game branch debug overlay; release menus use buttons |
| Scoreboard / team colors / pips | Persistent HUD | adapted — game branch RED/BLUE score and bag pips |
| Turn banner / CPU turn | Persistent HUD, input disabled during flight/CPU | adapted — game branch input_set_context / input_next_turn |
| Power meter / five-stop color ramp | Live swing speed gauge | adapted — game branch reads detector speed and swing_power |
| LAST SHOT meter / 3 s linger | Post-throw HUD | adapted — game branch retains INPUT_THROW result for 3 s |
| Power direction arrow | Swing-aim arrow | adapted — game branch uses detector locked_aim / toss.aim |
| Wind HUD / pennant / streaks | Wind display with round direction/strength | adapted — game branch renderer and existing wind policy |
| Board View inset | HUD board camera inset | adapted — game branch second render pass |
| Held bag / height / surface feedback | Held bag and HUD selection indicators | adapted — game branch consumes INPUT_HEIGHT/SURFACE/STANCE |
| Callouts / landing results | CORNHOLE, HANGER, board, grounder, miss, knocked in/off, wash | adapted — game branch existing result callouts |
| Round scoring / summary | Cancellation scoring after eight settled throws; A continue | adapted — game branch summary screen, same scoring rules |
| Match over / winner | Winner screen; A replay/menu | adapted — game branch match-over screen |
| Practice statistics | Practice HUD / stats panel | adapted — game branch preserves stats across CLEAR BOARD |
| Career records / progression save | Tournament screens | adapted — game branch saves fight/wins after won fight |
| Saved input profiles | Per-slot full speed, curl strength, hand | adapted — INPUT_PROFILE_CHANGED; game branch serializes save.dat |
| Version label / menu kicker | Small Wii version label and menu version | adapted — game branch compile-time version replaces Electron IPC |
| MUSIC menu | Music screen with D-pad/A/B navigation | adapted — game branch follow-up required; SPEC v1 music omission needs owner reconciliation, not cabinet-only N/A |
| Music play/pause | MUSIC → PLAY/PAUSE, A | adapted — game branch audio hook pending |
| Next track | MUSIC → NEXT TRACK, A | adapted — game branch playlist hook pending |
| Choose music folder | MUSIC → folder selection, D-pad/A/B | adapted — game branch SD playlist selector pending |
| Refresh music playlist | MUSIC → REFRESH, A | adapted — game branch rescan hook pending |
| Music volume | MUSIC volume row; minus/plus | adapted — game branch volume hook pending |
| Music BACK | B or BACK + A | adapted — game branch music/menu navigation pending |

Input acceptance: host input 40/40 (33 core + 7 WPAD); arcade 24/24; pinned snapshot 48/48. Arcade gameplay and snapshots remain unchanged. P1 hardware acceptance remains open: both hands, speed labels, backswing/pause/forward traces, lateral/twist separation and zero false throws.
