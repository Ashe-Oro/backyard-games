# Backyard Cornhole Arcade 0.2.23

Copied the completed 0.2.22 snapshot into 0.2.23, preserving
`app -> resources-app`. Work is on `codex/arcade-0.2.23`.

Default full-power speed rises from 4000 to 5600 counts/sec (+40%).
The 80 ms speed window and linear `min(1, speed / powerSpeed)` scaling
are unchanged; distance still grants no power. The clamp stays 400–10000.

Profile loaders migrate missing/null speed and factory values 1050, 1650,
2100, 3300, 4000 and 6500 to 5600. Custom v9–v5 speeds such as 2000
survive. Legacy v4–v1 speeds floor at 5600, with factory migration applied
first; custom 9000 survives. Explicit `settings({powerSpeed:1050})` remains
1050. Storage tests cover every version from v1 through v9.

Menu kicker, package version and player-guide header now say 0.2.23.
The player guide mentions the faster full-power roll without technical units.
Developer notes consistently describe the current default and migration rules.

Validation from this snapshot directory:

- `node --test tests/*.test.mjs`: 4/4 test files pass, zero failures.
- `node --test --test-isolation=none tests/*.test.mjs`: 39/39 tests pass,
  zero failures, skips or cancellations.
- `node --check` passes for all four edited JavaScript files:
  trackball.mjs, controller.js, trackball.test.mjs and controller.test.mjs.
- Power checkpoints cover 2800 = 50%, 5600 = 100%, and 8000 capped at 100%.
  Measured 1650/825 fixtures retain their speed assertions and divide power
  by 5600. Lower-speed release fixtures retain their gestures and now assert
  their actual fractional power.

Files edited relative to the copied snapshot:

- resources-app/dist/site/arcade/trackball.mjs
- resources-app/dist/site/arcade/controller.js (version only)
- resources-app/package.json
- tests/trackball.test.mjs
- tests/controller.test.mjs
- READ-ME.txt
- DEV-NOTES.txt
- CODEX_SUMMARY.md

The parent cabinet-snapshots/README.md adds the 0.2.23 entry with install
timestamp and rollback path pending.

Buttons, compact legend, KEYS persistence, 3000 ms last-shot linger, spin,
aiming, 0.05 nudge step, beer pong and other gameplay are unchanged.
Existing button, legend, linger, spin and beer-pong tests remain in place.
All other copied regular files remain byte-identical to 0.2.22.
Earlier snapshots, licensed binaries, Electron runtime, Ports videos,
gamelist.xml and /userdata were not modified. No spending, push, install
or commit was performed. Cabinet installation and commit are left to the caller.


0.2.23 spin follow-up (edited in place)
- Research: requested /workspace/docs/cornhole/spin-research.md could not be written
  (read-only mount). Ready copy: spin-research.md in this snapshot, with ACL/ACA
  sources and a distinction between real cut mechanics and arcade convention.
- A/C input signs flipped: A/negative X => negative angular spin => clockwise
  from above => right/+X curl. C => counterclockwise/left. Aim unchanged.
- Full-flight 14 rad/s flat yaw; release slerp removed so it cannot cancel spin.
  Slider/normal tilt reduced. Surface curl uses named constants in spin.mjs:
  slick 3.2 m/s^2, decay 1.4/s; sticky 1.5 m/s^2, decay 4.5/s.
- Live step tests: six-inch-right landing, 0.32 m short of hole, 2 m/s slide,
  C/full left spin curls in on slick normal/slider; zero spin misses. Sticky
  curls less. Bag contacts wake blockers; curl also initializes on bag landings.
- Every CPU WIN sample compared against original no-spin slide arithmetic.
  simulate remains spin-free. Power default 5600 and 80 ms window retained.
- Shared real legend CSS/markup adds bottom-left A/C spin, 1/3 aim card.
  legend-shots.html updated; spin-legends.html provides cabinet-like framing.
- Both requested test modes pass: isolated reports 5 passing test files;
  --test-isolation=none reports 43 passing tests. JS syntax checks pass.
- Screenshot requested: /workspace/docs/cornhole/legends-spin-and-keys.png.
  NOT CREATED: that directory is read-only; Chrome also aborts on forbidden
  crashpad socket setsockopt even with crash reporting disabled. No image is
  claimed. capture-spin-legend.sh is ready for caller to run outside sandbox.
- No install, commit, push, licensed binary/runtime changes or preview changes.
  Parent snapshot README install/rollback pending state left untouched.

0.2.23 beer pong follow-up (edited in place)
- Replaced blue-quick with blue-solid: blue wins 10–7 over 28 alternating
  turns at the existing 4.05-second pace, with makes and misses on both sides.
  The other four scripts and their outcomes remain unchanged.
- Named buildPongMissPath helper selects rim ricochets on six of eight round
  slots, with occasional short/long table landings in the other two slots.
  Rim paths contact the next standing cup's lip at existing bounceAt timing,
  then deflect onto the tabletop or drop beside the table. No first-path
  off-table airballs. Hits, scoring bounces, swats and timing are unchanged.
- Tests cover balanced winners, 10–7 blue finish, both comeback orientations,
  miss geometry and majority rim paths, visible rim contact/deflection without
  cup removal, random selection, impact scoring, celebration and reset.
- Validation: isolated suite passes 5/5 test files; shared-process suite passes
  45/45 tests, with zero failures. node --check passes party.mjs and
  tests/party.test.mjs. Existing spin, power, controls and legend tests pass.
- Only party.mjs, tests/party.test.mjs and this summary changed for this
  follow-up. Player guides and parent README install timestamp left unchanged.
  No install, commit, push, spending, runtime/binary or preview changes.
  Installation and commit remain with the caller.
