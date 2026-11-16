# Wii 0.1.0 release validation — Ashe

Date: 2026-11-16T12:57:01.701084+00:00
Branch: `wii/game`; approved game tip `513a596`; packaging merge `315cdf2`.
Game source is unchanged from the approved tip. No push, rebase, tag change or hardware access.

## Status: fresh Dolphin smoke BLOCKED

A 25-second isolated Dolphin NoGUI smoke was requested against the real release
ISO. Xvfb cannot bind its local display socket in this managed sandbox and exits
before Dolphin starts. `xvfb.log` records `failed to bind listener` and
`Failed to find a socket to listen on`. `smoke.log` and `result.txt` record failure.
A separate direct NoGUI headless/software-renderer attempt also aborted before
rendering: `dolphin-headless.log` records `bit out of range 0 - FD_SETSIZE on fd_set`.
Dolphin installed version: 2503. This does not establish a game boot failure.

**No new screenshot was captured. The 20+ second boot/render requirement remains
unverified. No prior capture is substituted as release smoke evidence.**
Expected capture after a successful rerun:
`/workspace/docs/cornhole/wii-port/release/smoke/screenshot.png`.
Existing historical game captures are under `../../pr/dolphin-wii-game/`; they
are earlier evidence, not this release test.

ISO: `/workspace/uploads/wii-game/BackyardCornhole-BYCEAO.iso` (262,307,840 bytes)
SHA-256: `0007d5625ca7181ebb2a49e2e2cbc4c59427b9c4c54b885ddcf7e66d76de492e`
DOL SHA-256: `79bf64a272e663b6175845682207b5c7415b7e1e4e57856f54951db43dcf256d`

Rerun in an environment that permits a local X display:

```sh
cd /workspace/tailgate-games
make -C wii smoke OUT=/workspace/uploads/wii-game
```

The smoke script uses a temporary profile, software OpenGL and an emulated
Remote 1 A mapping intended to dismiss the strap prompt; that mapping is not
validated here. The release INI retains real Wii Remotes 1 and 2 (`= 2`). A
surviving process alone is not a visual pass: inspect the screenshot and log.

## Passed validation

- Forced full release rebuild with devkitPPC r51/libogc 3.2.1 and WIT 3.05a: clean, no compiler warnings.
- `make -C wii host-test`: 33 input + 7 WPAD tests; 938 physics golden checks; 2,256 game checks; 209,137 party checks. Generated constants and golden vectors current.
- Shared drift/guard suite: 32/32 pass, zero skips.
- Pinned arcade 1.0.3 snapshot: 48/48 pass, zero skips.
- WIT reports BYCEAO, NTSC/USA, IOS 58. Game uses NTSC-only 480p/480i modes.
- ISO extracted `sys/main.dol`, standalone DOL, HBC folder DOL and zipped HBC DOL are byte-identical.
- Both ZIPs pass CRC checks; both SHA256 manifests verify.
- Two consecutive release builds have byte-identical ISOs and checksum manifests.
  `dolphin/generated-ticket.hex` retains an original WIT-generated ticket so the
  title key does not change on each compose. No retail disc input is used.

Node 24's default test reporter groups test files; reruns with
`--test-isolation=none` expose the 32 and 48 individual test totals. Logs in `../`.
The additional top-level arcade suite was attempted: four of six files passed;
exit-cleanup/isolation files hit sandbox `spawnSync ... EPERM` for Python/Node
subprocesses. Those are extra checks, separate from the passing required suites;
no arcade code was changed.

## Artifacts

Build from repository root with one command:
`make -C wii release OUT=/workspace/uploads/wii-game`.

| Artifact under `/workspace/uploads/wii-game/` | Bytes |
|---|---:|
| `BYCEAO.ini` | 157 |
| `BackyardCornhole-BYCEAO-v0.1.0-release.zip` | 2,385,069 |
| `BackyardCornhole-BYCEAO.iso` | 262,307,840 |
| `BackyardCornhole-HBC-v0.1.0.zip` | 774,384 |
| `GameSettings/BYCEAO.ini` | 157 |
| `INSTALL-KINHANK.md` | 2,871 |
| `KINHANK_INSTALL.md` | 2,871 |
| `PAYLOAD-SHA256SUMS` | 895 |
| `SHA256SUMS` | 1,089 |
| `SHA256SUMS.boot` | 75 |
| `apps/backyardcornhole/boot.dol` | 772,928 |
| `apps/backyardcornhole/icon.png` | 512 |
| `apps/backyardcornhole/meta.xml` | 514 |
| `boot.dol` | 772,928 |

`SHA256SUMS` hashes the release ZIP and payload files. The ZIP includes
`PAYLOAD-SHA256SUMS` to verify its contents without a circular archive hash.
`SHA256SUMS.boot` is a pre-existing matching standalone DOL checksum.

## Remaining checks

Fresh Dolphin title/menu screenshot after 20+ seconds; real-Wiimote feel and
calibration; full-speed Android/Kinhank and Wii hardware validation. The cabinet
and Kinhank were not accessed. Recommend `wii-v0.1.0` after the fresh smoke passes;
no tag was created.
