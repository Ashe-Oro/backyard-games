#!/bin/sh
# 0.1.4: the pointer (IR) is never used. Gameplay and menus run on buttons + accelerometer only.
# Fails if any Wii source asks for IR data, reads it, or sets the IR virtual screen.
set -eu
cd "$(dirname "$0")/.."
hits=$(grep -rnE 'WPAD_FMT_BTNS_ACC_IR|WPAD_DATA_IR|WPAD_SetVRes|WPAD_IR|IR_Set|ir_t\b|(\.|->)ir\b|\bir\.(valid|x|y|smooth|dot)' source || true)
if [ -n "$hits" ]; then echo "IR use found:"; echo "$hits"; exit 1; fi
fmt=$(grep -rn 'WPAD_SetDataFormat' source | grep -v 'WPAD_FMT_BTNS_ACC)' || true)
if [ -n "$fmt" ]; then echo "data format other than WPAD_FMT_BTNS_ACC:"; echo "$fmt"; exit 1; fi
echo "no-IR check: ok (WPAD_FMT_BTNS_ACC only, no IR reads)"
