#!/usr/bin/env bash
# Isolated desktop smoke only: no ADB, hardware pairing, or user-profile writes.
set -euo pipefail
ISO=${1:-/workspace/uploads/wii-stub/BackyardCornhole-BYCEAO.iso}
SMOKE_OUT=${SMOKE_OUT:-/workspace/docs/cornhole/wii-port/pr/smoke}
SMOKE_SECONDS=${SMOKE_SECONDS:-10}
mkdir -p "$SMOKE_OUT"
SMOKE_OUT=$(realpath "$SMOKE_OUT")
# Avoid treating a prior run's successful capture as evidence for this run.
python3 - "$SMOKE_OUT" <<'PY'
from pathlib import Path
import sys
for name in ('screenshot.png', 'dolphin-core.log', 'result.txt'):
    (Path(sys.argv[1]) / name).unlink(missing_ok=True)
PY
exec > >(tee "$SMOKE_OUT/smoke.log") 2>&1
printf 'UTC: %s\nISO: %s\n' "$(date -u +%FT%TZ)" "$ISO"
fail() { printf 'FAIL: %s\n' "$*" | tee "$SMOKE_OUT/result.txt"; exit 1; }
[[ -f "$ISO" ]] || fail "ISO missing: $ISO"
[[ "$SMOKE_SECONDS" =~ ^[1-9][0-9]*$ ]] || fail 'SMOKE_SECONDS must be a positive integer'
for tool in Xvfb ffmpeg python3 rg; do
    command -v "$tool" >/dev/null || fail "missing dependency: $tool"
done
if [[ -z ${DOLPHIN:-} ]]; then
    DOLPHIN=$(command -v dolphin-emu-nogui || command -v dolphin-emu || true)
fi
[[ -n "$DOLPHIN" ]] || fail 'Dolphin not installed; install dolphin-emu'
"$DOLPHIN" --version
USER_DIR=$(mktemp -d "$SMOKE_OUT/user.XXXXXX")
mkdir -p "$USER_DIR/Config" "$USER_DIR/GameSettings" "$USER_DIR/Logs"
cat > "$USER_DIR/Config/Dolphin.ini" <<'EOF'
[Core]
CPUThread = True
EnableCheats = False
WiimoteContinuousScanning = False
[DSP]
Backend = No audio output
[Interface]
ConfirmStop = False
UsePanicHandlers = False
[Display]
RenderToMain = False
RenderWindowWidth = 640
RenderWindowHeight = 480
EOF
cat > "$USER_DIR/Config/Logger.ini" <<'EOF'
[Options]
WriteToFile = True
Verbosity = 3
[Logs]
BOOT = True
DVDINTERFACE = True
VIDEO = True
OSREPORT = True
POWERPC = True
EOF
cat > "$USER_DIR/GameSettings/BYCEAO.ini" <<'EOF'
[Controls]
WiimoteSource0 = 0
WiimoteSource1 = 0
WiimoteSource2 = 0
WiimoteSource3 = 0
EOF
cat > "$USER_DIR/Config/GFX.ini" <<'EOF'
[Settings]
InternalResolution = 1
ShowFPS = True
EOF
XVFB_PID=
DOLPHIN_PID=
cleanup() {
    if [[ -n "$DOLPHIN_PID" ]]; then
        kill -TERM "$DOLPHIN_PID" 2>/dev/null || true
        for _ in {1..20}; do
            kill -0 "$DOLPHIN_PID" 2>/dev/null || break
            sleep 0.1
        done
        kill -KILL "$DOLPHIN_PID" 2>/dev/null || true
        wait "$DOLPHIN_PID" 2>/dev/null || true
    fi
    if [[ -n "$XVFB_PID" ]]; then
        kill "$XVFB_PID" 2>/dev/null || true
        wait "$XVFB_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT
# -displayfd chooses a free display; no dependency on xauth or an existing desktop.
DISPLAY_FILE=$(mktemp "$SMOKE_OUT/display.XXXXXX")
Xvfb -displayfd 3 -screen 0 640x480x24 -nolisten tcp 3>"$DISPLAY_FILE" >"$SMOKE_OUT/xvfb.log" 2>&1 &
XVFB_PID=$!
for _ in {1..50}; do
    [[ -s "$DISPLAY_FILE" ]] && break
    kill -0 "$XVFB_PID" 2>/dev/null || fail 'Xvfb exited; see xvfb.log'
    sleep 0.1
done
[[ -s "$DISPLAY_FILE" ]] || fail 'Xvfb display did not become ready'
export DISPLAY=":$(cat "$DISPLAY_FILE")"
rm "$DISPLAY_FILE"
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QPA_PLATFORM=xcb
ARGS=(-u "$USER_DIR" -v OGL -e "$(realpath "$ISO")")
if [[ $(basename "$DOLPHIN") == *nogui* ]]; then
    ARGS+=(-p x11)
else
    ARGS+=(-b)
fi
"$DOLPHIN" "${ARGS[@]}" >"$SMOKE_OUT/dolphin.log" 2>&1 &
DOLPHIN_PID=$!
for ((i = 0; i < SMOKE_SECONDS; i++)); do
    sleep 1
    kill -0 "$DOLPHIN_PID" 2>/dev/null || fail 'Dolphin exited before capture; see dolphin.log'
done
ffmpeg -hide_banner -loglevel error -y -f x11grab -draw_mouse 0 -video_size 640x480 \
    -i "$DISPLAY" -frames:v 1 "$SMOKE_OUT/screenshot.png" || fail 'screenshot capture failed'
cleanup
DOLPHIN_PID=
XVFB_PID=
if [[ -f "$USER_DIR/Logs/dolphin.log" ]]; then
    cp "$USER_DIR/Logs/dolphin.log" "$SMOKE_OUT/dolphin-core.log"
fi
if rg -i 'invalid apploader|failed to (boot|load|initialize)|panic|fatal|invalid read|invalid write' \
    "$SMOKE_OUT/dolphin.log" "$USER_DIR/Logs" 2>/dev/null; then
    fail 'known boot/runtime failure in log; inspect screenshot and logs'
fi
printf 'CAPTURED: Dolphin survived %s seconds; screenshot.png requires visual inspection.\nProfile: %s\n' \
    "$SMOKE_SECONDS" "$USER_DIR" | tee "$SMOKE_OUT/result.txt"
