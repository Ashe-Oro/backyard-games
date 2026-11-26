# Backyard Cornhole Wii + Kinhank 0.1.12

This update starts at `3af92de`, the merged 0.1.11 release. It bounds textured bag draws in both cameras and guards the board inset against close passes.

## Blue streak report

The Round 6 TV photo shows the blue fabric pattern widening from the held bag's position toward the top of the screen. The original photo note identifies the disc as 0.1.8 on Kinhank with Dolphin at 3× and real Wii Remotes. Version 0.1.9 added a fixed scissor around the held bag. The `scene.c` and `gfx.c` render paths are identical in 0.1.9, 0.1.10 and 0.1.11.

The bag submits direct position, RGBA and texture coordinates matching its GX vertex format. It uses a fully initialized transform and no display list. The held bag uses a separate pose at the release point. A thrown bag enters the list after position and motion are initialized, rotates in flight or on the deck, shrinks during a hole drop and stops drawing once it is in the hole or removed. The main camera is fixed, and the held mesh stays at a camera depth of at least 8.65 m with a 0.10 m near plane through the full aiming dip. A host sweep across arcs, sides, powers, stance, aim and wind also kept all sampled thrown and resting poses clear of the main near plane.

An early 0.1.12 build reproduced the foreground trapezoid on desktop Dolphin 2609a Vulkan. Blue's idle bag filled the height of its scissor and returned to its normal shape once the swing armed. An instrumented repeat showed the same trapezoid with a finite modelview matrix. The paired-single and scalar matrix concatenations agreed, and the matrix placed the bag center 8.786 m in front of the camera. Lifting the scissor in a diagnostic build that used the old transform reproduced the tall column through the scoreboard. Moving the held bag's local-to-world transform to the CPU removed the trapezoid with the same replay, GX quads, texture and scissor. That comparison isolates the stretch to the per-bag GX transform path.

The close board camera can intersect a flying bag. A centered SLICK Kobe at physical launch power 0.781 crosses the old 0.05 m inset near plane during three consecutive 240 Hz physics samples, 1.408–1.417 s after release. At the first sample, the rotated corner depths range from −0.032 m to +0.052 m. Dolphin reproduced a large bag covering the upper part of the inset during Kobe flight in each of 0.1.9, 0.1.10 and 0.1.11. One trial build skipped straddling bags at 0.05 m but still rendered a full-width bag immediately after it cleared that plane.

## Rendering change

Textured bag faces submit world-space vertices with the active camera matrix. The CPU applies the same rotation, position and scale before submission.

Each bag draw uses the active camera to project a conservative bound around the mesh, intersects that bound with the active viewport and uses the result as its GX scissor. This extends the aiming protection to thrown, resting and dropping bags. Bags with invalid positions or bounds touching or crossing the near plane are skipped. The inset now uses a 0.50 m near plane to keep close flybys from filling the overlay. A close flyby can briefly disappear in the board inset. The rest of the scene draws using the restored viewport scissor.

## Verification and build

- `make -C wii host-test -j2` passed 21 input tests, 14 throw-trace groups, 13 WPAD tests, 938 physics checks, 2,537 game checks and 209,137 party checks. Generated constants, oracle vectors and the no-IR check passed. The game regression covers the near-plane intersection and the frame-filling bag after it clears the old near plane. Reverting to 0.05 m made the oversized-bag check fail.
- Dolphin 2609a under Xvfb reproduced the main-view wedge on Vulkan 1× with the old transform in diagnostic builds. The same replay showed a compact patterned bag on the final ISO. The evidence directory `/workspace/agent-logs/bag-render/` contains `before-first-0112-candidate-blue-idle.png`, `before-unscissored-blue-column.png` and `after-0112-blue-idle-vulkan.png`. Stock 0.1.9 through 0.1.11 showed compact held bags in the tested desktop scenes. OpenGL and Software Renderer reached the strap screen during the timed captures.
- The 4:3 Kobe replay at 0.703 controller power obscures the inset in `before-0111-inset-close-vulkan-4x3.png`. The same throw leaves the board visible in `after-0112-inset-close-vulkan-4x3.png`, reenters the inset and scores one point.
- `after-0112-last-shot-vulkan.png` shows LAST SHOT 88% and 1 POINT from a Normal/SLICK throw. `after-0112-debug-vulkan.png` shows LAST PEAK 3.51 G, 3.80 M/S and LAST SHOT 100% from the next throw. The 0.1.11 power tables, detector and rollout caps remain in use.
- The stock Wiimote end-to-end harness passed on the final disc on a retry. The Null replay scored a B-up throw, toggled help and the 1+2 overlay in both directions, and reported 59.9 FPS with no DSP ucode warning.
- Built with devkitPPC r51 and libogc 3.2.1 from the pinned `devkitpro/devkitppc:20261102` image and WIT 3.05a r8638. The packaged `main.dol` matches `boot.dol`. The release and payload manifests, bundled payload manifests and three ZIP integrity checks passed. All 28 files already in the two distribution directories passed their pre-build checksum manifest.
- The 0.1.12 Kinhank ISO has SHA-256 `f3afd144382d01f7cb810e8802418409377f5c0a95fac02d246d181a650f467b`. The ZIP has SHA-256 `b40d6e3b4b4da90351348c2d787b7dff3ae1da0c473c7a6568e89b75b0316b6e`. Both are in `kinhank/dist-0.1.12/`.
