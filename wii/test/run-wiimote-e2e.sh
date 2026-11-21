#!/usr/bin/env bash
# Automated Dolphin emulated-Wiimote e2e: strap -> title -> practice -> aim -> throw -> score.
# Exit 0 = pass, 1 = game/test failure, 2 = environment blocked (missing Dolphin, Xvfb, etc.).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ISO=${1:-}
if [[ -z "$ISO" ]]; then
    for cand in \
        "${OUT:-$ROOT/dist}/BackyardCornhole-BYCEAO.iso" \
        "$ROOT/dist/BackyardCornhole-BYCEAO.iso"
    do
        if [[ -f "$cand" ]]; then ISO=$cand; break; fi
    done
fi

E2E_OUT=${E2E_OUT:-$ROOT/build/wiimote-e2e}
mkdir -p "$E2E_OUT"
E2E_OUT=$(realpath "$E2E_OUT")
# Drop prior-run evidence so a stale pass cannot be reused.
python3 - "$E2E_OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
for name in ("strap.png","title.png","aim.png","throw.png","score.png","debug.png",
             "result.txt","dolphin.log","dolphin-core.log","xvfb.log","fps.txt","wiimote-e2e.dtm"):
    (out / name).unlink(missing_ok=True)
PY

exec > >(tee "$E2E_OUT/e2e.log") 2>&1
printf 'UTC: %s\nAmerica/New_York: %s\nISO: %s\n' \
    "$(date -u +%FT%TZ)" "$(TZ=America/New_York date +%FT%T\ %Z)" "${ISO:-}"

env_fail() { printf 'ENV_BLOCK: %s\n' "$*" | tee "$E2E_OUT/result.txt"; exit 2; }
fail() { printf 'FAIL: %s\n' "$*" | tee "$E2E_OUT/result.txt"; exit 1; }

[[ -n "${ISO:-}" && -f "$ISO" ]] || env_fail "ISO missing: ${ISO:-unset}. Build with: make -C wii iso"
ISO=$(realpath "$ISO")
for tool in Xvfb ffmpeg python3 rg xdotool; do
    command -v "$tool" >/dev/null || env_fail "missing dependency: $tool"
done

if [[ -z ${DOLPHIN:-} ]]; then
    for cand in \
        "$(command -v dolphin-emu-nogui || true)" \
        "$(command -v dolphin-emu || true)" \
        /opt/dolphin/dolphin-emu-nogui \
        /opt/dolphin/dolphin-emu \
        /opt/dolphin/Dolphin_Emulator-x86_64.AppImage \
        "$HOME/Applications/Dolphin_Emulator-x86_64.AppImage"
    do
        if [[ -n "$cand" && -x "$cand" ]]; then DOLPHIN=$cand; break; fi
    done
fi
[[ -n "${DOLPHIN:-}" && -x "$DOLPHIN" ]] || env_fail \
    "Dolphin is not installed (no dolphin-emu-nogui, dolphin-emu, or DOLPHIN=). Official packages and dl.dolphin-emu.org were unavailable on this host; install a Linux Dolphin build and rerun."

printf 'DOLPHIN: %s\n' "$DOLPHIN"
"$DOLPHIN" --version || true
sha256sum "$ISO"

# Independent of emulator vintage: the DOL must carry a 5.0-19870-recognized ASND CRC.
if [[ -x "$ROOT/build/check-ucode" ]]; then
    ISO_DOL_DIR=$(mktemp -d "$E2E_OUT/iso-dol.XXXXXX")
    if command -v wit >/dev/null &&
        wit extract "$ISO" "$ISO_DOL_DIR" --psel DATA --files +/sys/main.dol --overwrite >/dev/null 2>&1; then
        "$ROOT/build/check-ucode" "$ISO_DOL_DIR/sys/main.dol" | tee "$E2E_OUT/ucode.txt"
    elif [[ -f "$ROOT/build/boot.dol" ]]; then
        "$ROOT/build/check-ucode" "$ROOT/build/boot.dol" | tee "$E2E_OUT/ucode.txt"
        printf 'NOTE: checked wii/build/boot.dol (ISO DOL extract unavailable)\n'
    else
        env_fail "check-ucode exists but no DOL could be extracted from the ISO"
    fi
    rm -rf "$ISO_DOL_DIR"
    if rg -q '5dbf8bf1' "$E2E_OUT/ucode.txt"; then
        fail 'DOL still contains unknown-to-19870 ASND ucode 0x5dbf8bf1'
    fi
    rg -q 'crc=0x(dbbeeb61|bad876ef|8d69a19b|cc2fd441|a81582e2)' "$E2E_OUT/ucode.txt" \
        || fail 'check-ucode did not report a Dolphin 5.0-19870-recognized ASND CRC'
fi

USER_DIR=$(mktemp -d "$E2E_OUT/user.XXXXXX")
mkdir -p "$USER_DIR/Config" "$USER_DIR/GameSettings" "$USER_DIR/Logs"
cat > "$USER_DIR/Config/Dolphin.ini" <<'EOF'
[Core]
CPUThread = True
EnableCheats = False
WiimoteContinuousScanning = False
[DSP]
EnableJIT = False
Backend = No audio output
[Interface]
ConfirmStop = False
UsePanicHandlers = False
OnScreenDisplayMessages = True
[Display]
RenderToMain = False
Fullscreen = False
RenderWindowWidth = 640
RenderWindowHeight = 480
[Input]
BackgroundInput = True
[Analytics]
Enabled = False
PermissionAsked = True
EOF
cat > "$USER_DIR/Config/Logger.ini" <<'EOF'
[Options]
WriteToFile = True
Verbosity = 4
[Logs]
BOOT = True
DSPHLE = True
DSPLLE = True
DSP = True
AUDIO = True
DVDINTERFACE = True
VIDEO = True
OSREPORT = True
OSREPORT_HLE = True
POWERPC = True
WII_IPC_HLE = True
WIIMOTE = True
EOF
cat > "$USER_DIR/GameSettings/BYCEAO.ini" <<'EOF'
[Controls]
WiimoteSource0 = 1
WiimoteSource1 = 0
WiimoteSource2 = 0
WiimoteSource3 = 0
EOF
# Pointer-driven emulated Wiimote. Isolated profile only; release INI keeps real remotes.
# This host has no /dev/input or /dev/uinput, and XTEST keyboard events do not reach
# Dolphin 2609a's XInput2 keyboard device. Mouse buttons on "Virtual core pointer" do.
# BackgroundInput is required: without a window manager the render window is unfocused
# and even Buttons/A = 1 is ignored.
# Movie playback supplies DesiredWiimoteState; leave MotionPlus detached so the
# serialized packets (core buttons + accel + optional IR, no extension) match.
cat > "$USER_DIR/Config/WiimoteNew.ini" <<'EOF'
[Wiimote1]
Source = 1
Device = XInput2/0/Virtual core pointer
Buttons/A = `Click 1`
Buttons/B = `Click 3`
Buttons/2 = `Click 2`
Tilt/Forward = `Cursor Y-`
Tilt/Backward = `Cursor Y+`
Swing/Up = `Cursor Y-`
Swing/Down = `Cursor Y+`
IMUAccelerometer/Forward = `Cursor Y-`
IMUAccelerometer/Backward = `Cursor Y+`
IMUAccelerometer/Up = `Cursor Y-`
IMUAccelerometer/Down = `Cursor Y+`
Shake/X = `Click 3`
Extension = None
Extension/Attach MotionPlus = 0
EOF
cat > "$USER_DIR/Config/GFX.ini" <<'EOF'
[Settings]
Backend = OGL
InternalResolution = 1
ShowFPS = True
DumpFrames = False
[Hacks]
EFBAccessEnable = True
EOF

XVFB_PID=
DOLPHIN_PID=
cleanup() {
    if [[ -n "${DOLPHIN_PID:-}" ]]; then
        kill -TERM "$DOLPHIN_PID" 2>/dev/null || true
        for _ in {1..20}; do
            kill -0 "$DOLPHIN_PID" 2>/dev/null || break
            sleep 0.1
        done
        kill -KILL "$DOLPHIN_PID" 2>/dev/null || true
        wait "$DOLPHIN_PID" 2>/dev/null || true
    fi
    if [[ -n "${XVFB_PID:-}" ]]; then
        kill "$XVFB_PID" 2>/dev/null || true
        wait "$XVFB_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# Allocate an isolated display so captures cannot include another job's window.
DISPLAY_FILE=$(mktemp "$E2E_OUT/display.XXXXXX")
Xvfb -displayfd 3 -screen 0 640x480x24 -nolisten tcp 3>"$DISPLAY_FILE" >"$E2E_OUT/xvfb.log" 2>&1 &
XVFB_PID=$!
for _ in {1..50}; do
    [[ -s "$DISPLAY_FILE" ]] && break
    kill -0 "$XVFB_PID" 2>/dev/null || break
    sleep 0.1
done
DISPLAY_NUMBER=$(cat "$DISPLAY_FILE")
rm -f "$DISPLAY_FILE"
[[ -n "$DISPLAY_NUMBER" ]] || env_fail "Xvfb display did not become ready; see $E2E_OUT/xvfb.log"
export DISPLAY=:$DISPLAY_NUMBER
xwininfo -root >/dev/null 2>&1 || env_fail "Xvfb display did not become ready; see $E2E_OUT/xvfb.log"
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QPA_PLATFORM=xcb
export SDL_VIDEODRIVER=x11
printf 'DISPLAY=%s\n' "$DISPLAY"

# Option 1 (preferred): play a generated .dtm at 60 FPS on the Null backend.
# E2E_MODE=mouse falls back to xdotool; E2E_VIDEO overrides the backend (OGL for shots).
E2E_MODE=${E2E_MODE:-dtm}
if [[ "$E2E_MODE" == "dtm" ]]; then
    VIDEO_BACKEND=${E2E_VIDEO:-Null}
else
    VIDEO_BACKEND=${E2E_VIDEO:-OGL}
fi
ARGS=(-u "$USER_DIR" -e "$ISO")
base=$(basename "$DOLPHIN")
if [[ "$base" == *nogui* ]]; then
    ARGS+=(-v "$VIDEO_BACKEND" -p x11)
else
    ARGS+=(-b -v "$VIDEO_BACKEND")
fi
PY_ARGS=(--out "$E2E_OUT" --user-dir "$USER_DIR" --display "$DISPLAY")
if [[ "$E2E_MODE" == "dtm" ]]; then
    python3 "$ROOT/test/gen_wiimote_dtm.py" -o "$E2E_OUT/wiimote-e2e.dtm"
    ARGS+=(-m "$E2E_OUT/wiimote-e2e.dtm")
    PY_ARGS+=(--movie)
fi
printf 'VIDEO_BACKEND=%s E2E_MODE=%s\n' "$VIDEO_BACKEND" "$E2E_MODE"

"$DOLPHIN" "${ARGS[@]}" >"$E2E_OUT/dolphin.log" 2>&1 &
DOLPHIN_PID=$!
sleep 0.5
kill -0 "$DOLPHIN_PID" 2>/dev/null || {
    if rg -q 'failed to bind listener|Failed to find a socket to listen on|bit out of range 0 - FD_SETSIZE' \
        "$E2E_OUT/dolphin.log" "$E2E_OUT/xvfb.log"; then
        env_fail "Dolphin/Xvfb failed to start in this environment; see $E2E_OUT/dolphin.log and xvfb.log"
    fi
    env_fail "Dolphin exited immediately; see $E2E_OUT/dolphin.log"
}

export E2E_VIDEO="$VIDEO_BACKEND"
python3 "$ROOT/test/wiimote_e2e.py" \
    "${PY_ARGS[@]}" \
    --dolphin-pid "$DOLPHIN_PID" \
    || rc=$?
rc=${rc:-0}

# Copy Dolphin's own file log if it wrote one.
if [[ -f "$USER_DIR/Logs/dolphin.log" ]]; then
    cp "$USER_DIR/Logs/dolphin.log" "$E2E_OUT/dolphin-core.log"
fi

# Always scan for the Kinhank-era unknown-ucode warning, even on a gameplay fail.
LOGS=("$E2E_OUT/dolphin.log")
[[ -f "$E2E_OUT/dolphin-core.log" ]] && LOGS+=("$E2E_OUT/dolphin-core.log")
if rg -i '5dbf8bf1|unknown ucode|unknown dsp|unrecognized ucode|this game uses an unknown' "${LOGS[@]}"; then
    fail "Dolphin log still contains a DSP ucode warning (ASND HLE regression)"
fi

if [[ "$rc" -eq 2 ]]; then
    env_fail "$(cat "$E2E_OUT/result.txt" 2>/dev/null || echo 'environment blocked during scripted play')"
fi
if [[ "$rc" -ne 0 ]]; then
    fail "$(tail -n 5 "$E2E_OUT/result.txt" 2>/dev/null || echo "scripted play failed (exit $rc)")"
fi
exit 0
