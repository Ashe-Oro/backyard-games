# Backyard Cornhole — Wii
# Platform packaging remains with P0. Host input tests use the native C compiler.

VERSION := 0.1.0
GAME_ID := BYCEAO
HOST_CC ?= cc
HOST_CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
INPUT_CORE := source/core/swing.c source/core/input.c
INPUT_HEADERS := source/core/swing.h source/core/swing_config.h source/core/input.h

.PHONY: all host-test iso release clean version

version:
	@echo $(VERSION)

all iso release:
	@echo "wii: '$@' is not implemented yet (scaffold). See docs/wii-port/IMPL_PLAN.md, phase P0." >&2
	@exit 1

host-test: build/input-test build/wpad-test
	./build/input-test
	./build/wpad-test

build/input-test: tests/input_test.c $(INPUT_CORE) $(INPUT_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Isource/core tests/input_test.c $(INPUT_CORE) -lm -o $@

build/wpad-test: tests/wpad_test.c tests/fake/wiiuse/wpad.h source/wii/input_wpad.c source/wii/input_wpad.h $(INPUT_CORE) $(INPUT_HEADERS)
	@mkdir -p build
	$(HOST_CC) $(HOST_CFLAGS) -Itests/fake -Isource/core tests/wpad_test.c source/wii/input_wpad.c $(INPUT_CORE) -lm -o $@

clean:
	@rm -rf build dist dolphin/iso-root
