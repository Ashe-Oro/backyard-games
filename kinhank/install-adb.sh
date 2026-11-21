#!/bin/sh
# Install the Kinhank bundle on the Kinhank over ADB. Retro runs this;
# developers never touch the Kinhank.
#
# A file pushed with adb is owned by shell:shell and defaults to -rw-rw----.
# Dolphin (the app uid) then cannot read GameSettings/BYCEAO.ini and silently
# falls back to global settings. chmod 666 after the copy is required.
# Do not change Dolphin's global GFX.ini; Ashe keeps that at 1x for other games.
set -eu

SERIAL="${ADB_SERIAL:-192.168.4.59:5555}"
ISO_DEST="${ISO_DEST:-/storage/3050BAA350BA6EEA0/Roms/wii/BackyardCornhole-BYCEAO.iso}"
INI_DEST="${INI_DEST:-/storage/emulated/0/Android/data/org.dolphinemu.dolphinemu/files/GameSettings/BYCEAO.ini}"

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -f "$here/GameSettings/BYCEAO.ini" ]; then
  bundle=$here
elif [ -f "$here/../GameSettings/BYCEAO.ini" ]; then
  bundle=$(CDPATH= cd -- "$here/.." && pwd)
else
  bundle=$PWD
fi

iso=$bundle/BackyardCornhole-BYCEAO.iso
ini=$bundle/GameSettings/BYCEAO.ini
if [ ! -f "$iso" ]; then
  echo "ISO not found at $iso (run make -C kinhank, or cd into the unzipped bundle)" >&2
  exit 1
fi
if [ ! -f "$ini" ]; then
  echo "GameSettings/BYCEAO.ini not found at $ini" >&2
  exit 1
fi
if ! grep -qx "InternalResolution = 3" "$ini" || \
   ! grep -qx "WiimoteSource0 = 2" "$ini" || \
   ! grep -qx "WiimoteSource1 = 2" "$ini"; then
  echo "Refusing to install: $ini is not the 3x / real-Wiimote Kinhank ini" >&2
  exit 1
fi

adb connect "$SERIAL" >/dev/null
adb -s "$SERIAL" push "$iso" "$ISO_DEST"
adb -s "$SERIAL" shell mkdir -p "$(dirname "$INI_DEST")"
adb -s "$SERIAL" push "$ini" "$INI_DEST"
# World-readable so Dolphin's app uid can load per-game settings.
adb -s "$SERIAL" shell chmod 666 "$INI_DEST"

listing=$(adb -s "$SERIAL" shell ls -l "$INI_DEST")
printf '%s\n' "$listing"
# Expect a world-read bit, e.g. -rw-rw-rw- after chmod 666, not -rw-rw----.
mode=${listing%% *}
others=$(printf '%s' "$mode" | cut -c8-10)
case $others in
  r??) ;;
  *)
    echo "Read-back failed: $INI_DEST is not world-readable ($listing)" >&2
    echo "Dolphin will ignore this ini and fall back to global settings." >&2
    exit 1
    ;;
esac
echo "Installed ISO and world-readable GameSettings ini."
echo "Leave Dolphin's global GFX.ini at 1x; 3x comes only from this per-game ini."
