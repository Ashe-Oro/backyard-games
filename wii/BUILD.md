# Build Backyard Cornhole Wii 0.1.6

Run the commands below from the repository root after cloning this repository
with the Wii source tree present. For playing on hardware, see [INSTALL-WII.md](INSTALL-WII.md).

## 1. Install the tools

Choose native tools or the Docker recipe below. Release versions are recorded in
[TOOLCHAIN.md](TOOLCHAIN.md): devkitPPC r51 (GCC 16.2.0, binutils 2.47),
libogc 3.2.1, libfat-ogc 2.1.0, gamecube-tools 1.0.7 (`elf2dol`),
general-tools 1.4.4 (`wiiload` 0.5.3), and WIT 3.05a r8638.
Host tools need a C compiler, GNU make, Node 20+ and Python 3.11+.

### Native Linux / macOS

1. Install Git, a host C compiler, GNU make, curl and tar. On Debian/Ubuntu:

   ```sh
   sudo apt-get update
   sudo apt-get install build-essential git curl ca-certificates python3
   ```

   On macOS, install the Xcode command-line tools (`xcode-select --install`)
   and [Homebrew](https://brew.sh), then:

   ```sh
   brew install git make coreutils node@20 python@3.11
   export PATH="$(brew --prefix make)/libexec/gnubin:$(brew --prefix node@20)/bin:$(brew --prefix python@3.11)/libexec/bin:$PATH"
   ```

   `coreutils` supplies `sha256sum`, used by the Makefile. On Linux install
   Node 20+ from [Node.js](https://nodejs.org/en/download) if your distribution's
   version is older. Install Python 3.11+ if needed. Confirm:

   ```sh
   cc --version
   make --version
   node --version
   python3 --version
   sha256sum --version
   ```

2. Install [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman)
   using its Linux instructions or macOS installer, then install the Wii group:

   ```sh
   sudo dkp-pacman -Syu
   sudo dkp-pacman -S --needed wii-dev
   export DEVKITPRO=/opt/devkitpro
   export DEVKITPPC="$DEVKITPRO/devkitPPC"
   export PATH="$DEVKITPPC/bin:$DEVKITPRO/tools/bin:$PATH"
   dkp-pacman -Q devkitPPC libogc libfat-ogc gamecube-tools general-tools
   powerpc-eabi-gcc --version
   ```

   If you configured devkitPro repositories in an existing system pacman, use
   `pacman` instead of `dkp-pacman`. The `wii-dev` group supplies devkitPPC,
   libogc and libfat-ogc, including the MP3/audio libraries used here. Pacman
   repositories move forward; compare installed versions with TOOLCHAIN.md.
   For the pinned release environment, use the Docker recipe instead.

The Makefile links a vendored `libasnd.a` from official libogc 2.5.0-2 (see [third_party/asnd-libogc-2.5.0/PROVENANCE.md](third_party/asnd-libogc-2.5.0/PROVENANCE.md)) so the DOL's ASND ucode is one Dolphin 5.0-19870 HLE already knows. `make` checks that CRC after linking. Pacman still installs libogc 3.2.1 for everything else; no extra install step.


3. Install [Wiimms ISO Tools 3.05a r8638](https://wit.wiimm.de/download.html).
   Linux x86_64 commands (the archive checksum is also in
   [PROVENANCE.md](dolphin/PROVENANCE.md)):

   ```sh
   curl -fLO https://wit.wiimm.de/download/wit-v3.05a-r8638-x86_64.tar.gz
   echo 'df95a14f1b4b2bd95d3111b70c6304e91d4ed3e4c7f72b1ad5306c3616d8933f  wit-v3.05a-r8638-x86_64.tar.gz' | sha256sum -c -
   sudo mkdir -p /opt/wit
   sudo tar -xzf wit-v3.05a-r8638-x86_64.tar.gz -C /opt/wit --strip-components=1
   export PATH="/opt/wit/bin:$PATH"
   wit --version
   ```

   On macOS, download `wit-v3.05a-r8638-mac.tar.gz` from the same official page,
   extract it into `/opt/wit` in the same way, and add `/opt/wit/bin` to PATH.
   That archive supports Intel and Apple Silicon. The Linux checksum above is
   only for the Linux archive. WIT 3.01a cannot compose this disc tree.

### Docker alternative

Install Docker Engine (Linux) or Docker Desktop (macOS), then:

```sh
docker build --platform linux/amd64 -t backyard-wii-build wii/docker
docker run --rm --platform linux/amd64 -v "$PWD":/src backyard-wii-build \
  sh -c 'node --version; python3 --version; make --version'
```

The [Dockerfile](docker/Dockerfile) pins `devkitpro/devkitppc:20261102`
and checksum-verifies WIT 3.05a r8638. The recipe has **not been run on the dev
box**. Its host packages come from apt rather than version pins: confirm Node
20+ and Python 3.11+ before proceeding. On Apple Silicon this uses amd64
emulation. Run the commands in step 2 inside the mounted container, for example:

```sh
docker run --rm --platform linux/amd64 -v "$PWD":/src backyard-wii-build \
  sh -c 'make -C wii host-test && make -C wii && make -C wii iso && make -C wii release OUT=dist'
```

## 2. Test, build and package

With native tools, run these exact commands from the repository root:

```sh
make -C wii host-test
make -C wii
make -C wii iso
make -C wii release OUT=dist
```

- `host-test`: builds host executables under `wii/build/`, runs input, WPAD,
  physics, game and party tests, and checks generated files.
- Default build: writes `wii/build/boot.elf` and `wii/build/boot.dol`, then copies
  the DOL to `wii/hbc/apps/backyardcornhole/boot.dol`.
- `iso`: writes `wii/dist/BackyardCornhole-BYCEAO.iso`. The disc tree is staged in
  `wii/dolphin/iso-root/`; inspection output is `wii/build/iso-dump.txt`.
- `release`: rebuilds the ISO and stages the standalone DOL, ISO,
  `apps/backyardcornhole/`, HBC ZIP, `INSTALL-WII.md`, `BUILD.md`, the Wii
  release ZIP, `PAYLOAD-SHA256SUMS` and `SHA256SUMS` in OUT. This is the
  original-hardware bundle only; it does not include Dolphin `GameSettings`.
  For the Kinhank (3×) zip, run `make -C kinhank` after the ISO exists. See
  [../TARGETS.md](../TARGETS.md). `OUT=dist` means `wii/dist/`; an absolute
  OUT selects another directory:

  ```sh
  make -C wii release OUT=/workspace/uploads/wii-game
  ```

The HBC ZIP is `BackyardCornhole-HBC-v0.1.6.zip`; the full ZIP is
`BackyardCornhole-BYCEAO-v0.1.6-release.zip`. The ISO and HBC ZIP are always
built in `wii/dist/` first, even with a different OUT. ZIP metadata is fixed
and entries/checksums are sorted for repeatable packaging.

## 3. Verify

For `OUT=dist`:

```sh
(cd wii/dist && sha256sum -c SHA256SUMS)
wit dump wii/dist/BackyardCornhole-BYCEAO.iso
cmp wii/build/boot.dol wii/build/iso-check/sys/main.dol
```

The ISO game ID must be **BYCEAO**. `make iso` extracts `sys/main.dol` to
`wii/build/iso-check/` and fails unless its SHA-256 equals `boot.dol`.
`release` also checks the disc ID and HBC/changelog versions. After extracting
the full release ZIP, run `sha256sum -c PAYLOAD-SHA256SUMS` in that directory.
For a custom OUT, run the SHA256SUMS check there instead.

Optional Linux Dolphin capture:

```sh
make -C wii smoke
```

This also runs `release` (default OUT is `wii/dist/`). It needs Dolphin NoGUI,
Xvfb, ffmpeg, ripgrep and Python, and captures about 25 seconds using an
isolated profile. Output defaults to
`/workspace/docs/cornhole/wii-port/release/smoke/`; override with
`SMOKE_OUT=/your/capture/path make -C wii smoke`. Inspect `screenshot.png` for
the title/menu; process survival alone is not a pass. See
[packaging/README.md](packaging/README.md). This capture does not validate real
Wii hardware or swing feel.

Automated emulated-Wiimote end-to-end:

```sh
make -C wii iso
./wii/test/run-wiimote-e2e.sh wii/dist/BackyardCornhole-BYCEAO.iso
# or: make -C wii wiimote-e2e
```

This boots the ISO in Dolphin (prefer `dolphin-emu-nogui`; `dolphin-emu -b` is
fine) under Xvfb, using an isolated profile with an emulated Remote 1. The
default `E2E_MODE=dtm` path (option 1) generates a TAS movie with
`wii/test/gen_wiimote_dtm.py` and plays it with `-b -v Null -m movie.dtm` at
~60 FPS. The movie taps A on the strap and Practice menus, toggles the 1+2
overlay twice, then holds B through a backswing and forward swing before
releasing into the NORMAL/SLICK board window. A pass requires a B-up throw,
a scored callout (ON THE BOARD, HANGER! or CORNHOLE!), an FPS number and
both overlay transitions. The Null backend has no framebuffer. For Practice,
overlay and score screenshots, set `E2E_VIDEO=Vulkan` or `E2E_VIDEO=OGL`
with a working renderer. Set `E2E_INTERNAL_RESOLUTION=3` to match the Kinhank
profile. `E2E_MODE=mouse` can drive menus and the IMU with
`xdotool` on setups with suitable XInput mappings. Outputs go under
`wii/build/wiimote-e2e/` unless you set `E2E_OUT`. The run checks for DSP
warnings and checks that the DOL contains a
5.0-19870-recognized ASND CRC in the DOL. Exit `2` means the environment
blocked the harness (missing Dolphin, Xvfb failed to bind a display, and so
on), not a game bug. Point `DOLPHIN=` at a local binary or AppImage if it is
not on `PATH`. This is not a substitute for a real DolphinBar session on
Kinhank, but it is the automated gate so a human is not the first person to
press A on the strap screen or to throw a bag.
