# Wii input parity acceptance

Owner directive, Nov 15 2026. Input-only checks; arcade gameplay and the immutable 1.0.3 snapshot remain unchanged.

- [x] Buttons + accelerometer WPAD format only; no camera reads, cursor, optical setup/calibration or sensor-bar dependency.
- [x] D-pad LEFT/RIGHT nudges stance 0.05, clamp ±0.45, ease 70 ms; UP/DOWN retains height selection.
- [x] Straight swing from shifted stance returns zero aim with zero fine trim; no hole targeting.
- [x] Left/right lateral swings change aim and signed spin. Wrist twist at release also changes spin. Both hands tested.
- [x] B-down captures neutral grip/fine trim; B-up freezes direction/spin. Forward follow-through confirms release; back-apex invalidates the backswing peak.
- [x] Power remains retained forward swing SPEED over 80 ms; duration/distance never grant power.
- [x] Setup/pause HAND row selects LEFT/RIGHT per slot, emits save event; calibration preserves hand, strength and slot ownership.
- [x] Button help says SWING TO AIM and TWIST TO CURL; −/+ remains optional fine trim.
- [x] Input tests pass, arcade 24/24 and snapshot 48/48 pass without gameplay/snapshot edits.
- [ ] Game branch consumes throw aim/spin in launch physics, renders setup/pause HAND rows and help, and serializes slot profiles with calibration.
- [ ] P1 hardware validates signs, both hands, lateral/twist separation, real backswing/pause/forward traces, speed labels and zero false throws. Synthetic host tests do not close hardware acceptance.
