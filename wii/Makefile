# Backyard Cornhole — Wii
# Run from wii/ (make -C wii ...). Needs devkitPPC + libogc (see TOOLCHAIN.md), WIT for `iso`,
# a host C compiler and Node 20+ for host tests and generators.
#
#   make              build/boot.dol (+ copy into hbc/apps/backyardcornhole/)
#   make host-test    input + physics-golden + game tests, generated-file checks (host compiler)
#   make iso          dist/BackyardCornhole-BYCEAO.iso (WIT), main.dol == boot.dol check
#   make release      dist/: HBC zip, ISO, INSTALL-WII.md, SHA256SUMS (real Wii only)

VERSION := 0.1.7
GAME_ID := BYCEAO
TITLE := Backyard Cornhole

DEVKITPRO ?= /opt/devkitpro
DEVKITPPC ?= $(DEVKITPRO)/devkitPPC
PPC := $(DEVKITPPC)/bin/powerpc-eabi-
ELF2DOL ?= $(DEVKITPRO)/tools/bin/elf2dol
WIT ?= $(firstword $(wildcard /opt/wit/bin/wit) wit)   # pinned: Wiimms ISO Tools v3.05a r8638 (3.01a cannot generate bi2.bin)
NODE ?= node
PYTHON ?= python3
OUT ?= dist

HOST_CC ?= cc
HOST_CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
INPUT_CORE := source/core/swing.c source/core/input.c
INPUT_HEADERS := source/core/swing.h source/core/swing_config.h source/core/input.h
GAME_CORE := source/core/physics.c source/core/rng.c source/core/cpu.c source/core/save.c source/core/game.c \
	source/core/arrow.c source/core/music.c source/core/wind_streaks.c source/core/party.c
CORE_HEADERS := $(wildcard source/core/*.h)

MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float
PPC_CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Wno-missing-field-initializers $(MACHDEP) \
	-I$(DEVKITPRO)/libogc/include -Isource/core -Isource/wii -Ibuild/gen -DBYC_VERSION=\"$(VERSION)\"
# libogc 3.2.1's ASND ships ucode CRC 0x5dbf8bf1, unknown to Dolphin 5.0-19870 HLE.
# Link official libogc 2.5.0-2 libasnd (CRC 0xdbbeeb61 / padded 0xbad876ef) first.
# See third_party/asnd-libogc-2.5.0/PROVENANCE.md.
ASND_LIBDIR := third_party/asnd-libogc-2.5.0/lib/wii
PPC_LDFLAGS := $(MACHDEP) -L$(ASND_LIBDIR) -L$(DEVKITPRO)/libogc/lib/wii -Wl,-Map,build/wii/boot.map
PPC_LIBS := -lfat -lmad -lasnd -lwiiuse -lbte -logc -lm
WII_SRC := $(GAME_CORE) $(INPUT_CORE) source/wii/input_wpad.c source/wii/gfx.c source/wii/scene.c \
	source/wii/hud.c source/wii/party_draw.c source/wii/music_wii.c source/wii/main.c build/gen/win_table.c
WII_OBJ := $(patsubst %.c,build/wii/%.o,$(notdir $(WII_SRC)))
vpath %.c source/core source/wii build/gen

HBC_DIR := hbc/apps/backyardcornhole
ISO_ROOT := dolphin/iso-root
ISO := dist/BackyardCornhole-$(GAME_ID).iso

.PHONY: all host-test iso release clean version check-generated icon soak smoke wiimote-e2e export-release

all: build/boot.dol
	@mkdir -p $(HBC_DIR)
	cp build/boot.dol $(HBC_DIR)/boot.dol
	@echo "boot.dol: $$(sha256sum build/boot.dol | cut -d' ' -f1)"

version:
	@echo $(VERSION)

# ---------------- generated sources ----------------
build/bake-win: tools/bake_win.c source/core/physics.c $(CORE_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Isource/core tools/bake_win.c source/core/physics.c -lm -o $@

build/gen/win_table.c: build/bake-win
	@mkdir -p build/gen
	./build/bake-win $@

check-generated:
	$(NODE) tools/gen-constants.mjs --check
	$(NODE) tools/oracle/golden.mjs --check

icon:
	$(NODE) tools/make-icon.mjs $(HBC_DIR)/icon.png

# ---------------- Wii build ----------------
build/wii/%.o: %.c $(CORE_HEADERS) $(wildcard source/wii/*.h)
	@mkdir -p build/wii
	$(PPC)gcc $(PPC_CFLAGS) -c $< -o $@

build/boot.elf: $(WII_OBJ)
	$(PPC)gcc $(PPC_LDFLAGS) $(WII_OBJ) $(PPC_LIBS) -o $@

build/check-ucode: tools/check-ucode.c
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) tools/check-ucode.c -o $@

build/boot.dol: build/boot.elf build/check-ucode
	$(ELF2DOL) $< $@
	./build/check-ucode $@

# Dev-only scripted soak build (auto-throws; logs to Dolphin OSReport). Not part of any release.
soak: $(WII_SRC)
	@mkdir -p build/soak
	$(PPC)gcc $(PPC_CFLAGS) -DBYC_SOAK $(PPC_LDFLAGS) $(WII_SRC) $(PPC_LIBS) -o build/soak/boot-soak.elf
	$(ELF2DOL) build/soak/boot-soak.elf build/soak/boot-soak.dol

# ---------------- host tests ----------------
host-test: build/input-test build/throw-test build/wpad-test build/physics-probe build/game-test build/party-test check-generated
	./build/input-test
	./build/throw-test
	./build/wpad-test
	$(NODE) tests/physics_golden.mjs build/physics-probe
	./build/game-test
	./build/party-test
	./tools/check-no-ir.sh

build/input-test: tests/input_test.c $(INPUT_CORE) $(INPUT_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Isource/core tests/input_test.c $(INPUT_CORE) -lm -o $@

build/throw-test: tests/throw_test.c tests/bowling_trace.h $(INPUT_CORE) $(INPUT_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Isource/core tests/throw_test.c $(INPUT_CORE) -lm -o $@

build/wpad-test: tests/wpad_test.c tests/bowling_trace.h tests/fake/wiiuse/wpad.h source/wii/input_wpad.c source/wii/input_wpad.h $(INPUT_CORE) $(INPUT_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Itests/fake -Isource/core tests/wpad_test.c source/wii/input_wpad.c $(INPUT_CORE) -lm -o $@

build/physics-probe: tests/physics_probe.c source/core/physics.c source/core/rng.c source/core/cpu.c $(CORE_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Isource/core tests/physics_probe.c source/core/physics.c source/core/rng.c source/core/cpu.c -lm -o $@

build/game-test: tests/game_test.c $(GAME_CORE) $(INPUT_CORE) $(CORE_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -DBYC_VERSION=\"$(VERSION)\" -Isource/core tests/game_test.c $(GAME_CORE) $(INPUT_CORE) -lm -o $@

build/party-test: tests/party_test.c source/core/party.c source/core/rng.c $(CORE_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Isource/core tests/party_test.c source/core/party.c source/core/rng.c -lm -o $@

# ---------------- Dolphin ISO (SPEC §1.5) ----------------
build/apploader.elf: dolphin/apploader/apploader.c dolphin/apploader/apploader.ld
	@mkdir -p build
	$(PPC)gcc -mcpu=750 -meabi -mhard-float -msdata=none -G 0 -Os -ffreestanding -fno-builtin -nostdlib \
		-Wall -Wextra -Wl,-T,dolphin/apploader/apploader.ld -Wl,--gc-sections dolphin/apploader/apploader.c -o $@

build/apploader.bin: build/apploader.elf
	$(PPC)objcopy -O binary $< $@

iso: build/boot.dol build/apploader.bin
	@mkdir -p dist
	$(NODE) tools/mkdisc.mjs $(ISO_ROOT) build/boot.dol build/apploader.bin \
		$$($(PPC)nm build/apploader.elf | awk '/ T apploader_entry$$/{print $$1}') $(GAME_ID) "$(TITLE)"
	rm -f $(ISO)
	$(WIT) copy $(ISO_ROOT) $(ISO) --iso --id $(GAME_ID) --name "$(TITLE)" --region USA --ios 58 --trunc --overwrite
	$(WIT) dump $(ISO) > build/iso-dump.txt
	rm -rf build/iso-check && $(WIT) extract $(ISO) build/iso-check --psel DATA --files +/sys/main.dol --overwrite >/dev/null
	@a=$$(sha256sum build/boot.dol | cut -d' ' -f1); b=$$(sha256sum build/iso-check/sys/main.dol | cut -d' ' -f1); \
		if [ "$$a" != "$$b" ]; then echo "ISO main.dol $$b != boot.dol $$a" >&2; exit 1; fi; \
		echo "ISO ok: $(ISO) ID $(GAME_ID), main.dol == boot.dol ($$a)"

# ---------------- release bundle ----------------
release: all iso
	@grep -q "<version>$(VERSION)</version>" $(HBC_DIR)/meta.xml || { echo "meta.xml version != $(VERSION)" >&2; exit 1; }
	@grep -Fq "## [$(VERSION)]" CHANGELOG.md
	rm -rf build/hbc-zip && mkdir -p build/hbc-zip/apps/backyardcornhole
	cp $(HBC_DIR)/boot.dol $(HBC_DIR)/meta.xml $(HBC_DIR)/icon.png build/hbc-zip/apps/backyardcornhole/
	cp INSTALL-WII.md build/hbc-zip/INSTALL-WII.md
	cd build/hbc-zip && rm -f ../../dist/BackyardCornhole-HBC-v$(VERSION).zip && \
		$(NODE) ../../tools/zip.mjs ../../dist/BackyardCornhole-HBC-v$(VERSION).zip apps INSTALL-WII.md
	$(PYTHON) tools/release.py --version $(VERSION) --out "$(OUT)"
	@cat "$(OUT)/SHA256SUMS"

export-release: release

smoke: release
	./packaging/smoke.sh "$(OUT)/BackyardCornhole-$(GAME_ID).iso"

wiimote-e2e: iso
	./test/run-wiimote-e2e.sh "$(ISO)"

clean:
	@rm -rf build dist $(ISO_ROOT) $(HBC_DIR)/boot.dol
