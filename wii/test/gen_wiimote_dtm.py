#!/usr/bin/env python3
"""Write a Dolphin 2609+ .dtm that plays Backyard Cornhole through a scored practice toss.

Wiimote packets follow SerializedWiimoteState from DesiredWiimoteState.cpp
(length-prefixed, no extension, no MotionPlus). Accelerometer units match
Dolphin's emulated rest pose (0g=0x200, 1g=0x268) and the game's swing_feed
sample() helper: g = ((1+e) sin r cos p, (1+e) sin p, (1+e) cos r cos p).
The movie grips at rest, moves the remote into a backswing, swings through
the bottom, and releases B while moving forward. The host suite covers
additional graded and misfire traces at native sensor/frame cadence.
"""
from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path

GAME_ID = b"BYCEAO"
MAGIC = b"DTM\x1a"
UPDATE_HZ = 200
ZERO_G = 0x80 << 2  # 0x200
ONE_G = 0x9A << 2   # 0x268
G_UNITS = ONE_G - ZERO_G  # 0x68 per 1 g
DEFAULT_ACCEL = (ZERO_G, ZERO_G, ONE_G)

BUTTON_A = 1 << 0
BUTTON_B = 1 << 1
BUTTON_ONE = 1 << 4
BUTTON_TWO = 1 << 5


def g_to_raw(gx: float, gy: float, gz: float) -> tuple[int, int, int]:
    def conv(v: float) -> int:
        return max(0, min(0x3FF, int(round(ZERO_G + v * G_UNITS))))

    return conv(gx), conv(gy), conv(gz)


def sample_g(energy: float, pitch_deg: float, roll_deg: float = 0.0) -> tuple[float, float, float]:
    """Same construction as wii/tests/input_test.c sample()."""
    p = pitch_deg * math.pi / 180.0
    r = roll_deg * math.pi / 180.0
    s = 1.0 + energy
    return (s * math.sin(r) * math.cos(p), s * math.sin(p), s * math.cos(r) * math.cos(p))


def pack_state(buttons: int = 0, accel: tuple[int, int, int] | None = None) -> bytes:
    if accel is None:
        accel = DEFAULT_ACCEL
    has_buttons = 1 if buttons else 0
    has_accel = 0 if accel == DEFAULT_ACCEL else 1
    flags = has_buttons | (has_accel << 1)
    data = bytearray([flags])
    if has_buttons:
        data.append(buttons & 0x7F)
        data.append(0)  # d-pad
    if has_accel:
        ax, ay, az = accel
        accel_low = (ax & 0b11) | (((ay >> 1) & 1) << 2) | (((az >> 1) & 1) << 3)
        if has_buttons:
            data[-1] |= accel_low << 4
        else:
            data.append(accel_low << 4)
        data.append((ax >> 2) & 0xFF)
        data.append((ay >> 2) & 0xFF)
        data.append((az >> 2) & 0xFF)
    return bytes([len(data)]) + bytes(data)


def hold(packets: list[bytes], seconds: float, buttons: int = 0,
         energy: float = 0.0, pitch: float = 0.0) -> None:
    n = max(1, int(round(seconds * UPDATE_HZ)))
    if energy == 0.0 and pitch == 0.0:
        pkt = pack_state(buttons, None if buttons == 0 else DEFAULT_ACCEL)
    else:
        pkt = pack_state(buttons, g_to_raw(*sample_g(energy, pitch)))
    packets.extend([pkt] * n)


def pack_header(input_count: int, author: bytes = b"BYC e2e") -> bytes:
    buf = bytearray(256)
    buf[0:4] = MAGIC
    buf[4:10] = GAME_ID
    buf[10] = 1  # bWii
    buf[11] = 1 << 4  # Wiimote 1
    buf[12] = 0  # from boot, not a savestate

    def put_u64(off: int, v: int) -> None:
        struct.pack_into("<Q", buf, off, v)

    def put_u32(off: int, v: int) -> None:
        struct.pack_into("<I", buf, off, v)

    # Generous bounds so CheckInputEnd does not stop on tickCount before inputs are used.
    put_u64(13, 60 * 60 * 2)          # frameCount: two minutes at 60 (the movie is ~64 s)
    put_u64(21, input_count)
    put_u64(29, 0)                    # lagCount
    put_u64(37, 0)                    # uniqueID
    put_u32(45, 0)                    # numRerecords
    buf[49:81] = author[:32].ljust(32, b"\0")
    buf[81:97] = b"Null".ljust(16, b"\0")
    # md5 left zero: Movie.cpp CheckMD5 skips an all-zero hash
    # recordingStartTime, config flags left 0 (bSaveConfig=false → use current settings)
    buf[162] = 1  # bWidescreen, informational
    put_u32(229, 0)
    put_u32(233, 0)
    put_u64(237, 729_000_000 * 600)  # tickCount: 10 minutes of Wii timebase
    assert len(buf) == 256
    return bytes(buf)


def build_movie() -> bytes:
    pkts: list[bytes] = []
    # Cover fake-BS2 + WPAD bring-up. 200 Hz, so 8 s is 1600 reports.
    hold(pkts, 8.0)
    # Strap: tap A (200 ms down, 800 ms up).
    hold(pkts, 0.20, BUTTON_A)
    hold(pkts, 0.80)
    # Title: PRACTICE is the first item; tap A.
    hold(pkts, 0.20, BUTTON_A)
    hold(pkts, 0.80)
    # begin_inning(900 ms) + "THROWS FIRST" banner; wait until PH_AIM.
    hold(pkts, 4.00)
    # 0.1.4: a fresh save shows the help panel; 1 tucks it, 1 again brings it back
    # (the new control list stays up for the rest of the run, for the help.png shot).
    hold(pkts, 0.20, BUTTON_ONE)
    hold(pkts, 3.00)
    hold(pkts, 0.20, BUTTON_ONE)
    hold(pkts, 0.60)
    for _ in range(2):
        hold(pkts, 2.6, BUTTON_ONE | BUTTON_TWO)
        hold(pkts, 0.4)

    # The NORMAL/SLICK hole window is 0.655–0.742 normalized power.
    # Several nearby values of e = 1.2 * power^2 cover quantization.
    def toss(energy: float) -> None:
        hold(pkts, 0.55)
        hold(pkts, 0.08, BUTTON_B)
        for pitch in (-10, -22, -33, -39):
            hold(pkts, 0.045, BUTTON_B, energy=0.10, pitch=pitch)
        hold(pkts, 0.13, BUTTON_B, pitch=-39)
        for pitch in (-31, -20, -9):
            hold(pkts, 0.025, BUTTON_B, energy=0.24, pitch=pitch)
        hold(pkts, 0.18, BUTTON_B, energy=energy)
        hold(pkts, 0.025, 0, energy=0.30, pitch=12)
        hold(pkts, 0.10, 0, pitch=25)
        # Flight ~2.2 s, GROUNDER/score callout, return to PH_AIM (~0.75 s).
        hold(pkts, 4.00)

    for energy in (0.50, 0.56, 0.59, 0.62, 0.68, 0.74):
        toss(energy)
    hold(pkts, 3.00)
    return b"".join(pkts)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args()
    body = build_movie()
    # inputCount is the number of PlayWiimote packets, not bytes
    count = 0
    i = 0
    while i < len(body):
        n = body[i]
        i += 1 + n
        count += 1
    path = Path(args.output)
    path.write_bytes(pack_header(count) + body)
    print(f"wrote {path} ({path.stat().st_size} bytes, {count} inputs, {count / UPDATE_HZ:.2f}s)")


if __name__ == "__main__":
    main()
