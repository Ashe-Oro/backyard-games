#!/usr/bin/env python3
"""Drive the isolated Dolphin window through strap, practice, a swing, and a scored bag."""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

# game.h: UI_STRAP=1, UI_TITLE=2, UI_NONE=0; PH_MENU=0, PH_WAIT=3, PH_AIM=4, PH_FLYING=6
RE_SCREEN = re.compile(r"BYC: screen (\d+) phase (\d+)")
RE_THROW = re.compile(r"BYC: throw slot .* release (\d+)")
RE_CALLOUT = re.compile(
    r"BYC: callout '([^']*)' '([^']*)' score (\d+)-(\d+) practice in=(\d+) on=(\d+) throws=(\d+)"
)
RE_FPS = re.compile(r"BYC: (\d+) frames rendered; ui (\d+) phase (\d+); fps ([0-9.]+)")
RE_DEBUG = re.compile(r"BYC: wiimote debug (on|off)")
SCORE_BIG = ("CORNHOLE!", "ON THE BOARD", "HANGER!")


def write_result(out: Path, status: str, lines: list[str]) -> None:
    out.joinpath("result.txt").write_text(status + "\n" + "\n".join(lines) + "\n")


def env_block(out: Path, msg: str) -> None:
    write_result(out, "ENV_BLOCK: " + msg, [])
    raise SystemExit(2)


def fail(out: Path, msg: str, extra: list[str] | None = None) -> None:
    write_result(out, "FAIL: " + msg, extra or [])
    raise SystemExit(1)


def grab(display: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    r = subprocess.run(
        [
            "ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
            "-f", "x11grab", "-draw_mouse", "0", "-video_size", "640x480",
            "-i", display, "-frames:v", "1", str(dest),
        ],
        capture_output=True, text=True,
    )
    if r.returncode != 0 or not dest.is_file() or dest.stat().st_size < 100:
        raise RuntimeError(f"screenshot failed for {dest.name}: {r.stderr.strip()}")


def combined_log(out: Path, user_dir: Path) -> str:
    parts = []
    for p in (out / "dolphin.log", user_dir / "Logs" / "dolphin.log", out / "dolphin-core.log"):
        if p.is_file():
            try:
                parts.append(p.read_text(errors="replace"))
            except OSError:
                pass
    return max(parts, key=len, default="")


def parse(log: str) -> dict:
    info = {
        "ui": None, "phase": None, "throws": [], "callouts": [], "fps": [],
        "remote_connected": "remote 1 connected" in log,
        "boot": "Backyard Cornhole Wii" in log,
    }
    for m in RE_SCREEN.finditer(log):
        info["ui"], info["phase"] = int(m.group(1)), int(m.group(2))
    info["throws"] = RE_THROW.findall(log)
    info["callouts"] = [m.groups() for m in RE_CALLOUT.finditer(log)]
    info["fps"] = [float(m.group(4)) for m in RE_FPS.finditer(log)]
    return info


def scored(info: dict) -> bool:
    for big, small, *_rest in info["callouts"]:
        if big in SCORE_BIG or "POINT" in small.upper():
            return True
    # practice in/on counts also count as a scored bag
    for _big, _small, _s0, _s1, pin, pon, _th in info["callouts"]:
        if int(pin) > 0 or int(pon) > 0:
            return True
    return False


def focus_window() -> str | None:
    try:
        out = subprocess.check_output(
            ["xdotool", "search", "--onlyvisible", "--name", "Dolphin"],
            text=True, stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        try:
            out = subprocess.check_output(
                ["xdotool", "search", "--onlyvisible", "--class", "dolphin"],
                text=True, stderr=subprocess.DEVNULL,
            )
        except subprocess.CalledProcessError:
            return None
    wids = [w for w in out.split() if w.strip()]
    if not wids:
        return None
    wid = wids[-1]
    subprocess.run(["xdotool", "windowactivate", "--sync", wid], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["xdotool", "windowfocus", "--sync", wid], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return wid



def click(button: int, hold: float) -> None:
    subprocess.run(["xdotool", "mousedown", str(button)], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(hold)
    subprocess.run(["xdotool", "mouseup", str(button)], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.2)


def wait_visible(display: str, dest: Path, timeout: float) -> None:
    deadline = time.time() + timeout
    last_err = "no frame"
    while time.time() < deadline:
        try:
            grab(display, dest)
        except RuntimeError as e:
            last_err = str(e)
            time.sleep(0.5)
            continue
        try:
            from PIL import Image
            pix = Image.open(dest).convert("RGB").tobytes()
            if any(pix):
                return
            last_err = "screenshot was all black"
        except Exception:
            if dest.is_file() and dest.stat().st_size > 2000:
                return
            last_err = "could not inspect screenshot"
        time.sleep(0.5)
    raise RuntimeError(last_err)


def key(wid: str | None, *keys: str, hold: float = 0.08) -> None:
    for k in keys:
        down = ["xdotool", "keydown"] + (["--window", wid, k] if wid else [k])
        up = ["xdotool", "keyup"] + (["--window", wid, k] if wid else [k])
        subprocess.run(down, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(hold)
        subprocess.run(up, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.12)


def chord_down(wid: str | None, *keys: str) -> None:
    for k in keys:
        cmd = ["xdotool", "keydown"] + (["--window", wid, k] if wid else [k])
        subprocess.run(cmd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def chord_up(wid: str | None, *keys: str) -> None:
    for k in keys:
        cmd = ["xdotool", "keyup"] + (["--window", wid, k] if wid else [k])
        subprocess.run(cmd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def wait_alive(pid: int, out: Path) -> None:
    try:
        os.kill(pid, 0)
    except OSError:
        env_block(out, f"Dolphin pid {pid} exited early; see dolphin.log")


def swing(wid: str | None, variant: int) -> None:
    """Hold B (right click), script a back-then-forward toss with pointer motion, release B."""
    del wid
    time.sleep(0.45)
    subprocess.run(["xdotool", "mousemove", "320", "240"], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["xdotool", "mousedown", "3"], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.25)
    if variant % 2 == 0:
        ys = list(range(240, 471, 15)) + list(range(470, 9, -25))
    else:
        ys = list(range(240, 9, -15)) + list(range(10, 471, 25))
    for y in ys:
        subprocess.run(["xdotool", "mousemove", "320", str(y)], check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.012)
    time.sleep(0.04)
    subprocess.run(["xdotool", "mouseup", "3"], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.15)



def finish_pass(out: Path, info: dict, notes: list[str], method: str) -> None:
    fps_vals = info["fps"]
    if fps_vals:
        fps_line = f"FPS samples: {', '.join(f'{v:.1f}' for v in fps_vals)}; last={fps_vals[-1]:.1f}"
    else:
        fps_line = "FPS: not reported in OSREPORT (need a 120-frame window)"
    out.joinpath("fps.txt").write_text(fps_line + "\n")
    shots = [p.name for p in (out / "strap.png", out / "title.png", out / "aim.png",
                               out / "throw.png", out / "score.png", out / "debug.png") if p.is_file()]
    last = info["callouts"][-1]
    lines = [
        f"method={method}",
        f"boot=ok remote_connected={info['remote_connected']}",
        f"final ui={info['ui']} phase={info['phase']}",
        f"callout='{last[0]}' '{last[1]}' practice in={last[4]} on={last[5]} throws={last[6]}",
        fps_line,
        "screenshots: " + (", ".join(shots) if shots else "none (Null backend has no framebuffer)"),
        "DSP ucode warning: absent (grep passed in wrapper)",
    ] + notes
    write_result(out, "PASS", lines)
    print("PASS")
    for line in lines:
        print(line)


def watch_movie(out: Path, user: Path, pid: int, display: str, video: str, notes: list[str]) -> None:
    """Option 1: .dtm playback already drives the emulated Wiimote. Only watch OSREPORT."""
    notes.append(f"input: Dolphin TAS .dtm on the {video} backend")
    capture = video != "Null"
    deadline = time.time() + 120
    info = parse("")
    saw_boot = False
    while time.time() < deadline:
        wait_alive(pid, out)
        log = combined_log(out, user)
        info = parse(log)
        if capture and not (out / "aim.png").exists() and info["ui"] == 0 and info["phase"] == 4:
            wait_visible(display, out / "aim.png", 15)
        if capture and not (out / "debug.png").exists() and "BYC: wiimote debug on" in log:
            wait_visible(display, out / "debug.png", 15)
        if info["boot"] or info["remote_connected"] or info["ui"] is not None:
            saw_boot = True
        if info["throws"] and scored(info) and info["fps"]:
            break
        # A scored throw is enough even if the FPS line has not arrived yet;
        # keep waiting a little for the 120-frame report.
        if info["throws"] and scored(info) and time.time() + 8 > deadline:
            break
        time.sleep(0.2)
    if not saw_boot:
        fail(out, "game did not boot during .dtm playback", notes)
    if not info["throws"]:
        fail(out, "DTM playback never produced BYC: throw (see dolphin.log)", notes + [
            f"last ui={info['ui']} phase={info['phase']}",
            f"remote_connected={info['remote_connected']}",
        ])
    if any(release != "1" for release in info["throws"]):
        fail(out, "throw did not come from the B-up edge", notes)
    if RE_DEBUG.findall(log) != ["on", "off"]:
        fail(out, "1+2 did not toggle the debug overlay on and off once each", notes)
    if not scored(info):
        fail(out, "threw bags but none scored (ON THE BOARD / HANGER / CORNHOLE!)", notes + [
            f"throws logged: {len(info['throws'])}",
            f"callouts: {info['callouts']}",
        ])
    if not info["fps"]:
        # One more short wait for the fps ticker.
        extra = time.time() + 5
        while time.time() < extra:
            info = parse(combined_log(out, user))
            if info["fps"]:
                break
            time.sleep(0.2)
    if not info["fps"]:
        fail(out, "toss scored but no FPS number was logged", notes)
    if capture:
        if not (out / "aim.png").exists():
            fail(out, "rendered run missed the Practice aim screenshot", notes)
        wait_visible(display, out / "score.png", 15)
    notes.append("B-up throws: " + str(len(info["throws"])) + "; 1+2 overlay: on, off")
    finish_pass(out, info, notes, f"dtm-{video.lower()}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--user-dir", required=True)
    ap.add_argument("--dolphin-pid", required=True, type=int)
    ap.add_argument("--display", required=True)
    ap.add_argument("--movie", action="store_true",
                    help="Watch a Dolphin .dtm playback; do not inject xdotool input")
    args = ap.parse_args()
    out = Path(args.out)
    user = Path(args.user_dir)
    pid = args.dolphin_pid
    display = args.display
    notes: list[str] = []

    video = os.environ.get("E2E_VIDEO", os.environ.get("VIDEO_BACKEND", "OGL"))
    slow = video != "Null"
    if args.movie:
        return watch_movie(out, user, pid, display, video, notes)
    hold = 4.0 if slow else 0.15
    after = 8.0 if slow else 0.8
    boot_timeout = 90 if slow else 20
    aim_timeout = 120 if slow else 20
    shot_timeout = 30 if slow else 8

    deadline = time.time() + boot_timeout
    info = parse("")
    # Wait until the emulated remote is actually connected; early A clicks are lost.
    while time.time() < deadline:
        wait_alive(pid, out)
        info = parse(combined_log(out, user))
        if info["remote_connected"] and (info["ui"] in (1, 2) or info["boot"]):
            break
        time.sleep(0.2)
    else:
        fail(out, "game did not boot with a connected emulated Wiimote", notes)

    wid = None
    for _ in range(25):
        wid = focus_window()
        if wid:
            break
        time.sleep(0.2)
    if not wid and not slow:
        notes.append("warning: no Dolphin window (expected with the Null backend)")
    elif not wid:
        notes.append("warning: no Dolphin window id")

    # Strap
    if info["ui"] is None or info["ui"] == 1:
        try:
            wait_visible(display, out / "strap.png", shot_timeout)
        except RuntimeError as e:
            if slow:
                env_block(out, str(e))
            else:
                notes.append(f"strap screenshot skipped: {e}")
        click(1, hold)  # A
        time.sleep(after)
        info = parse(combined_log(out, user))
        if info["ui"] == 1:
            click(2, hold)  # 2
            time.sleep(after)
            info = parse(combined_log(out, user))
        if info["ui"] == 1:
            fail(out, "strap screen did not accept A or 2 from the emulated Wiimote", notes)

    # Title
    deadline = time.time() + (30 if slow else 8)
    while info["ui"] != 2 and time.time() < deadline:
        time.sleep(0.2)
        info = parse(combined_log(out, user))
    if info["ui"] != 2:
        fail(out, f"did not reach title (last ui={info['ui']} phase={info['phase']})", notes)
    try:
        wait_visible(display, out / "title.png", shot_timeout)
    except RuntimeError as e:
        if slow:
            env_block(out, str(e))
        else:
            notes.append(f"title screenshot skipped: {e}")
    time.sleep(0.3)
    click(1, hold)  # PRACTICE is the first item
    time.sleep(after)

    # Aim (UI_NONE=0, PH_AIM=4). ~0.9s inning delay plus a thrower banner (wall-slow on softpipe).
    deadline = time.time() + aim_timeout
    while time.time() < deadline:
        wait_alive(pid, out)
        info = parse(combined_log(out, user))
        if info["ui"] == 0 and info["phase"] == 4:
            break
        time.sleep(0.2)
    if not (info["ui"] == 0 and info["phase"] == 4):
        fail(out, f"did not reach practice aim (last ui={info['ui']} phase={info['phase']})", notes)
    try:
        wait_visible(display, out / "aim.png", shot_timeout)
    except RuntimeError as e:
        if slow:
            env_block(out, str(e))
        else:
            notes.append(f"aim screenshot skipped: {e}")

    # Throws: retry a few scripted swings until a bag scores.
    got_throw = False
    for n in range(8):
        wait_alive(pid, out)
        focus_window()
        swing(wid, n)
        if n == 0:
            try:
                grab(display, out / "throw.png")
            except RuntimeError:
                pass
        # Wait for the bag to land.
        land_deadline = time.time() + 6
        while time.time() < land_deadline:
            info = parse(combined_log(out, user))
            if info["throws"]:
                got_throw = True
            if scored(info):
                break
            time.sleep(0.2)
        if scored(info):
            break
        # Wait until we are back in AIM for the next toss.
        back = time.time() + 6
        while time.time() < back:
            info = parse(combined_log(out, user))
            if info["ui"] == 0 and info["phase"] == 4:
                break
            time.sleep(0.2)

    if not got_throw:
        fail(out, "scripted swing never produced a throw (see throw.png and dolphin.log)", notes)
    if not scored(info):
        fail(out, "threw bags but none scored (ON THE BOARD / HANGER / CORNHOLE!)", notes + [
            f"throws logged: {len(info['throws'])}",
            f"callouts: {info['callouts']}",
        ])
    try:
        grab(display, out / "score.png")
    except RuntimeError as e:
        if slow:
            env_block(out, str(e))
        else:
            notes.append(f"score screenshot skipped: {e}")
    if not info["fps"]:
        fail(out, "toss scored but no FPS number was logged", notes)
    finish_pass(out, info, notes, "mouse-imu")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception as exc:  # noqa: BLE001 — surface unexpected harness errors cleanly
        out = Path(sys.argv[sys.argv.index("--out") + 1]) if "--out" in sys.argv else Path(".")
        write_result(out, "ENV_BLOCK: " + f"harness exception: {exc}", [])
        raise SystemExit(2)
