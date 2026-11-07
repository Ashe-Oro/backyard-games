#!/bin/sh
# Batocera configgen gameStop hook: context changes can precede key release.
# Restore only hotkeygen's virtual keyboard before EmulationStation resumes.
[ "$1" = gameStop ] || exit 0
python3 - <<'PY'
import evdev
from evdev import ecodes as E
for device_path in evdev.list_devices():
    device = None
    try:
        device = evdev.InputDevice(device_path)
        if device.name != "batocera hotkeys":
            continue
        held = device.active_keys()
        for key in held:
            device.write(E.EV_KEY, key, 0)
        if held:
            device.write(E.EV_SYN, E.SYN_REPORT, 0)
            print("Game exit: released stranded virtual hotkeys", held)
    except OSError as error:
        print("Virtual hotkey cleanup:", error)
    finally:
        if device is not None:
            device.close()
PY
