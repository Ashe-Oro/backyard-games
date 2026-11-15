#!/bin/sh
# Host-only alternative when make/devkitPPC are unavailable. No persistent artifacts.
set -eu
cd "$(dirname "$0")/.."
input_test_dir=$(mktemp -d "${TMPDIR:-/tmp}/wii-input.XXXXXX")
trap 'rm -rf "$input_test_dir"' EXIT HUP INT TERM
"${HOST_CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Isource/core \
  tests/input_test.c source/core/swing.c source/core/input.c -lm -o "$input_test_dir/input-test"
"${HOST_CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Itests/fake -Isource/core \
  tests/wpad_test.c source/wii/input_wpad.c source/core/swing.c source/core/input.c -lm -o "$input_test_dir/wpad-test"
"$input_test_dir/input-test"
"$input_test_dir/wpad-test"
