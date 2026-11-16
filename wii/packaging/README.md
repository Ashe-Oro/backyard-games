# Real-game packaging

The canonical pipeline is `wii/Makefile`. From the repository root:

```sh
make -C wii release OUT=/workspace/uploads/wii-game
make -C wii smoke OUT=/workspace/uploads/wii-game
```

The compatibility Makefile here delegates to it. The old `stub/`, `package.py`
and local apploader are retained as packaging history only; no release target
uses them. The real game's metadata, icon, apploader and NTSC video behavior win.
The canonical pipeline verifies ISO main.dol equals boot.dol, checks the HBC
version, and emits deterministic ZIP metadata, HBC files, GameSettings, a full
release ZIP and SHA256SUMS. Python 3.11+, Node 20+, devkitPPC/libogc and pinned
WIT 3.05a are required (see ../TOOLCHAIN.md).

`smoke.sh` requires Dolphin NoGUI, Xvfb, ffmpeg and ripgrep, defaults to 25 seconds,
and stores an isolated profile, logs and a screenshot under
`/workspace/docs/cornhole/wii-port/release/smoke/`. Its emulated Remote 1 presses A
once to dismiss the strap prompt. Inspect the capture before declaring a pass.
The shipped GameSettings file continues to select two real Wii Remotes.
See KINHANK_INSTALL.md for the manual handoff; no install target accesses hardware.
