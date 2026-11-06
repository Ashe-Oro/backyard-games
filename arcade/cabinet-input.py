#!/usr/bin/env python3
"""Read only mouse-relative and gamepad events. Never grab devices or change OS mappings."""
import json
import ctypes
import os
import signal
import select
import time
from evdev import InputDevice, list_devices, ecodes as E


def emit(data):
    print(json.dumps(data), flush=True)


def main():
    # Do not leave a reader behind if Batocera force-stops Electron.
    parent = os.getppid()
    if ctypes.CDLL(None).prctl(1, signal.SIGTERM, 0, 0, 0) != 0:
        raise RuntimeError('Cannot bind input-reader lifetime to the game')
    if parent == 1 or os.getppid() != parent:
        return
    devices, previous, scanned = {}, None, 0
    while True:
        if time.monotonic() - scanned > 2:
            scanned = time.monotonic()
            paths = set(list_devices())
            for path in list(devices):
                if path not in paths:
                    devices.pop(path)['device'].close()
            for path in sorted(paths):
                if path in devices:
                    continue
                d = InputDevice(path)
                caps = d.capabilities()
                mouse = {E.REL_X, E.REL_Y}.issubset(caps.get(E.EV_REL, []))
                keys = sorted(k for k in caps.get(E.EV_KEY, []) if E.BTN_JOYSTICK <= k < E.BTN_DIGI)
                if not mouse and not keys:
                    d.close()
                    continue
                devices[path] = dict(device=d, mouse=mouse, keys=keys, x=0, y=0, axes={})
            pads = sorted((p for p, d in devices.items() if d['keys']), key=lambda p: int(p.rsplit('event', 1)[1]))
            status = dict(kind='status', mice=[d['device'].name.strip() for d in devices.values() if d['mouse']], pads=[devices[p]['device'].name.strip() for p in pads])
            if status != previous:
                emit(status)
                previous = status
        ready, _, _ = select.select([d['device'] for d in devices.values()], [], [], .1)
        for device in ready:
            state = devices[device.path]
            try:
                events = list(device.read())
            except OSError:
                devices.pop(device.path)['device'].close()
                scanned = 0
                continue
            for e in events:
                if state['mouse'] and e.type == E.EV_REL:
                    if e.code == E.REL_X:
                        state['x'] += e.value
                    elif e.code == E.REL_Y:
                        state['y'] += e.value
                elif e.type == E.EV_SYN:
                    if e.code == E.SYN_DROPPED:
                        state['x'] = state['y'] = 0
                        emit(dict(kind='reset'))
                    elif e.code == E.SYN_REPORT and (state['x'] or state['y']):
                        emit(dict(kind='motion', x=state['x'], y=state['y'], at=e.timestamp()*1000))
                        state['x'] = state['y'] = 0
                elif state['keys'] and e.type == E.EV_KEY and e.code in state['keys'] and e.value in (0, 1):
                    emit(dict(kind='button', pad=pads.index(device.path), button=state['keys'].index(e.code), pressed=bool(e.value)))
                elif state['keys'] and e.type == E.EV_ABS and e.code in (E.ABS_X, E.ABS_Y, E.ABS_HAT0X, E.ABS_HAT0Y):
                    info = device.absinfo(e.code)
                    middle = (info.max + info.min) / 2
                    dead = max(1, (info.max - info.min) * .25)
                    value = -1 if e.value < middle - dead else 1 if e.value > middle + dead else 0
                    if value != state['axes'].get(e.code, 0):
                        state['axes'][e.code] = value
                        if value:
                            horizontal = e.code in (E.ABS_X, E.ABS_HAT0X)
                            emit(dict(kind='nav', code=('ArrowLeft' if value < 0 else 'ArrowRight') if horizontal else ('ArrowUp' if value < 0 else 'ArrowDown')))


if __name__ == '__main__':
    main()
