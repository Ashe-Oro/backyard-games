"""Release only Cornhole's virtual exit chord before returning to Batocera."""
import evdev
from evdev import ecodes as E


def release_exit_keys():
    for device_path in evdev.list_devices():
        device = None
        try:
            device = evdev.InputDevice(device_path)
            if device.name != "batocera hotkeys":
                continue
            held = set(device.active_keys())
            released = []
            for key in (E.KEY_F4, E.KEY_LEFTALT):
                if key in held:
                    device.write(E.EV_KEY, key, 0)
                    released.append(key)
            if released:
                device.write(E.EV_SYN, E.SYN_REPORT, 0)
                print("Cornhole exit: released virtual shortcut keys", released)
        except OSError as error:
            print("Cornhole exit cleanup:", error)
        finally:
            if device is not None:
                device.close()


if __name__ == "__main__":
    release_exit_keys()
