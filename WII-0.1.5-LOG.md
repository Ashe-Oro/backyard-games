# Backyard Cornhole Wii 0.1.5

## Implementation

Started from `f3c69e4` on `wii/power-round-0.1.5`. The release implementation is `da75e5bb99900400e94b54be942f882f73964795`.

Wii Remote power now uses `clamp(0.75 * peak_speed / full_speed, 0, 1)`. Applying the gain before saturation preserves the linear response and extends the range of measured speeds that produce distinct power. The old 1.2 g reference gives 0.750, and about 2.13 g excess reaches 1.000 with the default profile. A confirmed, clipped forward stroke still reports 1.000. Weak throws keep their classification and use scaled bounds of 0.090–0.2625. Existing profile settings load as saved and use the new gain. SET POWER includes the same gain in its median-to-target calculation so a new calibration can still target the NORMAL/SLICK center of the hole within the profile limits.

The faceting came from geometry and lighting. In `party_draw.c`, `cyl8()` overlapped two boxes rotated 45° apart, producing a scalloped outline. The box renderer lit each whole face with a single normal. The pong ball was an unlit cube, and the keg guest, friend, and serving cups were pairs of boxes. The round meshes use 32 radial segments and a 32-by-12 sphere. Their analytic radial and spherical normals produce interpolated vertex lighting. Cup rims are rings, with the original body and rim colors. The boxy people and the party poses, timing, and placement use the original code.

## Throw measurements

Calm, centered throws from the same scripted Wii Remote reports, with the NORMAL picker. Distances are to first landing at 240 Hz and are measured from release as `BYC_REL_Z - land_z`.

| Forward excess | 0.1.4 power | 0.1.5 power | 0.1.4 landing | 0.1.5 landing |
| --- | ---: | ---: | ---: | ---: |
| 0.45 g | 0.612 | 0.459 | 8.21 m | 7.58 m |
| 0.65 g | 0.736 | 0.552 | 8.77 m | 7.94 m |
| 0.90 g | 0.866 | 0.650 | 9.85 m | 8.38 m |

The board front is at `BYC_BOARD_Z = −8.2296 m` (27 ft) and release is at `BYC_REL_Z = −0.45 m`. With `BYC_L = 1.2192 m` and the 0.17802 rad tilt, the deck projects 1.200 m downrange and spans 7.7796–8.9795 m from release. The 0.65 and 0.90 g throws touch the deck. A 1.20 g NORMAL throw touches down at 8.84 m (0.750) and a 1.56 g KOBE throw at 8.86 m (0.855). The 2.20 g NORMAL throw reaches 1.000 on an unclipped host trace, clears the deck and touches down at 10.63 m. These measurements use the existing launch physics. The host tests also confirm strict distance ordering for the reference NORMAL throws, sideways aim and its left-handed mirror, gameplay −/+ no-op, and picker-only height.

## Build and emulator checks

- `make -j1 -C wii host-test` passed 36 input tests, 12 throw-trace tests, 12 WPAD tests, 938/938 physics golden checks, 2303/2303 game checks and 209137/209137 party checks. The generated-file checks and `wii/tools/check-no-ir.sh` passed.
- `make -j1 -C wii release` and `make -j1 -C kinhank` passed using the specified devkitPro and WIT. WIT extracted an exact copy of the built DOL from the ISO. The Kinhank ZIP passed its member CRC checks and contains the exact delivered ISO and `InternalResolution = 3` profile. `sha256sum -c BackyardCornhole-BYCEAO.iso.sha256` passed.
- Dolphin 2503 with Vulkan passed the TAS replay at both 1× and 3×. One B-up throw at 0.658 power scored a cornhole. Help and the 1+2 overlay each toggled twice, and the DSP warning scan passed. The 3× `strap.png` and `aim.png` visibly show v0.1.5.
- The 3× `fps.txt` reports 59.9 fps in emulated time. Across matching Practice frames 720–1320, the median wall-clock software-rendered rate was 13.81 fps for 0.1.4 and 15.05 fps for 0.1.5, measured over 120-frame intervals from Dolphin's log timestamps. The 0.1.5 run at 1× measured 59.94 wall-clock fps in that window. The 0.1.4 comparison reached Practice and hit the harness wall-clock timeout before its first toss. Actual 60 fps at 3× and Android performance remain unverified.
- The before close-ups use the exact reference 0.1.4 ISO at `/workspace/scratch/wii-0.1.4/BackyardCornhole-BYCEAO.iso`. Its extracted DOL hash matched a clean build from `f3c69e4`. The after shots use the delivered 0.1.5 ISO. Isolated Dolphin profiles used temporary in-memory look-at and overlay patches, with the same seed and party phase for each pair. The matched 3× Vulkan captures were cropped from 1920×1080 render windows. The keg shots show the shell, bands, pump and serving cup. The pong shots show both racks and the airborne ball against the dark fence post.

## Summary

Artifacts are in `/workspace/scratch/wii-0.1.5/`. The ISO SHA-256 is `7841d3e828c7570286f97ded2e611b12b529334e3a62dfd0a68dc064a2268f71`. The package is `BackyardCornhole-Kinhank-v0.1.5.zip`. Verification outputs include `host-test.log`, `wii-release.log`, `kinhank-build.log`, `dolphin-e2e/` and `dolphin-baseline-3x/`.

The cogged keg and cup silhouettes and cube ball came from box geometry. Round meshes and interpolated lighting correct both the silhouettes and shading. The 0.75 gain changes reference powers from 0.612 / 0.736 / 0.866 to 0.459 / 0.552 / 0.650. Landings move from 8.21 / 8.77 / 9.85 m to 7.58 / 7.94 / 8.38 m against the 7.7796–8.9795 m deck span. Local release commit `da75e5bb99900400e94b54be942f882f73964795`.

Host verification passed 36 input tests, 12 throw traces, 12 WPAD tests, 938 physics checks, 2303 game checks and 209137 party checks. The 3× Dolphin replay passed with a cornhole at 59.9 fps in emulated time and about 15 fps in wall time on the software renderer.

The close-up pairs are `keg-before.png` / `keg-after.png` and `pong-before.png` / `pong-after.png`.
