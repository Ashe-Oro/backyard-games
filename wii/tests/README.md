Host input tests: `make -C wii host-test` or `sh wii/tests/run-input-tests.sh` from the repo root. The shell runner also works from another directory and deletes its temporary executables.

`input_test.c` exercises deterministic synthetic accelerometer samples, the exact time-weighted speed window, retained forward peaks, B-release latency, back-apex rejection, arming, clipping, swing direction, release twist, both hands, straight throws from shifted stance, setup/pause hand selection and release freezing, menu/play buttons, two remotes, pass-and-play, disconnect/reassignment and comfortable-toss calibration. `wpad_test.c` links the actual adapter against a test-owned API double in `fake/`, covering queue draining, registration errors, reconnect, error/data flags, dropped counts and estimated timestamp gaps. The fake header never belongs in a Wii build's include path.

Use a C11 host compiler with libm; neither devkitPPC nor a renderer is needed. Assertions must remain enabled. The fake-WPAD tests verify our adapter contract, not the Bluetooth stack or a real libogc link. Recorded real-remote traces, independent video speed labels and the blocking P1 hardware gates remain required. Physics golden-vector tests are Track C work.

Game tests (wii/game): `make -C wii host-test` also runs
- `physics_golden.mjs` + `physics_probe.c`: the C physics/wind/RNG/CPU core against
  `golden/arcade-1.0.3-physics.json`, produced by the read-only oracle `tools/oracle/golden.mjs` from the pinned
  1.0.3 snapshot (WIN table within one 0.0025 step, 66 shots and 8 multi-bag scenes with identical states and
  transition sequences and positions within 1 mm, roundWind sequences, mulberry32, 24 seeded planCareerShot
  decisions including RNG draws).
- `game_test.c`: menus, a 20-round 2 PLAYER soak, 200 practice throws with CLEAR BOARD, career vs the first
  and last rivals, scoring (cancellation, wash keeps order, crossing 21, moving slides don't count), wind policy,
  straight-from-stance throws and the KOBE remap, pause freezing, disconnect/resume, hand/settings/SET POWER,
  saves (round trip, CRC failure, invalid fields).
- `check-generated`: `cornhole_constants.h` matches `shared/cornhole`, and the golden vectors match the oracle.
