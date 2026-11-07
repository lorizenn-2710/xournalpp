#!/usr/bin/python3
"""All GUI input goes through one Cua MCP connection on an isolated X11 display.

Run under xvfb-run + dbus-run-session. Never run on the user's real display.
"""
import copy
import json
import os
import pathlib
import select
import subprocess
import time

ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = ROOT / "output/floating-notes-research-2026-11-03"
LOG = ROOT / "build/floating-notes-research"
STATE = OUT / "prototype-state.json"
assert os.environ.get("FLOATING_NOTES_ISOLATED") == "1", "isolated display required"
assert os.environ.get("DISPLAY") not in (None, ":0", ":1"), "refuse normal desktop"


class Cua:
    def __init__(self):
        self.stderr = (LOG / "cua-isolated.log").open("w")
        self.process = subprocess.Popen(["cua-driver", "mcp", "--direct"], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=self.stderr, text=True, bufsize=1)
        self.seq = 0
        self.transcript = (LOG / "cua-transcript.jsonl").open("w")
        self.rpc("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                                "clientInfo": {"name": "floating-notes-research", "version": "1"}})
        self.process.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
        self.process.stdin.flush()

    def rpc(self, method, params):
        self.seq += 1
        seq = self.seq
        request = {"jsonrpc": "2.0", "id": seq, "method": method, "params": params}
        self.transcript.write(json.dumps({"request": request}, ensure_ascii=False) + "\n")
        self.transcript.flush()
        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()
        deadline = time.monotonic() + 40
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.process.stdout], [], [], max(0, deadline - time.monotonic()))
            if not ready:
                break
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError("Cua ended")
            response = json.loads(line)
            self.transcript.write(json.dumps({"response": response}, ensure_ascii=False) + "\n")
            self.transcript.flush()
            if response.get("id") == seq:
                if "error" in response:
                    raise RuntimeError(response["error"])
                return response.get("result", {})
        raise TimeoutError(method)

    def call(self, name, args):
        result = self.rpc("tools/call", {"name": name, "arguments": args})
        if result.get("isError"):
            raise RuntimeError(result)
        if "structuredContent" in result:
            return result["structuredContent"]
        for block in result.get("content", []):
            if block.get("type") == "text":
                try:
                    return json.loads(block["text"])
                except json.JSONDecodeError:
                    pass
        return result


def main():
    # Display infrastructure, not an alternative input route. Cua remains the only input driver.
    wmroot = LOG / "wm-root"
    wm_env = dict(os.environ, LD_LIBRARY_PATH=str(wmroot / "usr/lib/x86_64-linux-gnu"),
                  XDG_DATA_DIRS=str(wmroot / "usr/share") + ":/usr/local/share:/usr/share")
    wm = subprocess.Popen([str(wmroot / "usr/bin/openbox"), "--sm-disable", "--config-file",
                           str(wmroot / "etc/xdg/openbox/rc.xml")], env=wm_env,
                          stdout=(LOG / "wm.log").open("w"), stderr=subprocess.STDOUT)
    from Xlib import display
    d = display.Display()
    until = time.monotonic() + 5
    while not d.screen().root.get_full_property(d.intern_atom("_NET_SUPPORTING_WM_CHECK"), 0):
        if time.monotonic() > until:
            raise RuntimeError("test window manager unavailable")
        time.sleep(.05)
    d.close()
    c = Cua()
    result = c.call("launch_app", {"name": str(ROOT / "scripts/teaching-tools/research-floating-notes/launch.sh")})
    pid = result["pid"]
    windows = result.get("windows", [])
    if not windows:
        windows = c.call("list_windows", {"pid": pid})["windows"]
    assert windows, result
    wid = windows[0]["window_id"]
    target = {"pid": pid, "window_id": wid}
    captures = 0
    passed = []

    def state(label):
        nonlocal captures
        captures += 1
        shot = OUT / f"gui-{captures:02d}-{label}.png"
        data = c.call("get_window_state", {**target, "screenshot_out_file": str(shot)})
        (LOG / f"gui-{captures:02d}-{label}.json").write_text(json.dumps(data, ensure_ascii=False, indent=2))
        return data

    def book():
        return json.loads(STATE.read_text())

    def active(data):
        return next(n for n in data["notes"] if n["id"] == data["active"])

    def check(condition, text):
        assert condition, text
        passed.append(text)
        print("PASS", text, flush=True)

    def button(label):
        before = state("before-button")
        element = next(e for e in before["elements"] if e["label"] == label and e["role"] == "button")
        c.call("click", {**target, "element_token": element["element_token"]})
        if label == "Đóng thử nghiệm":
            remaining = c.call("list_windows", {"pid": pid})
            check(not remaining.get("windows"), "close only the research window")
            return remaining
        return state("after-button")

    def drag(start, end):
        state("before-drag")
        args = {**target, "from_x": start[0], "from_y": start[1], "to_x": end[0],
                "to_y": end[1], "duration_ms": 450, "steps": 24}
        try:
            c.call("drag", args)
        except RuntimeError as error:
            if "background_unavailable" not in str(error):
                raise
            print("BACKGROUND_UNAVAILABLE: foreground retry on isolated Xvfb only", flush=True)
            state("before-isolated-drag")
            c.call("drag", {**args, "delivery_mode": "foreground"})
        return state("after-drag")

    def hotkey(keys):
        state("before-hotkey")
        try:
            c.call("hotkey", {**target, "keys": keys})
        except RuntimeError as error:
            if "background_unavailable" not in str(error):
                raise
            state("before-isolated-hotkey")
            c.call("hotkey", {**target, "keys": keys, "delivery_mode": "foreground"})
        state("after-hotkey")

    state("initial")
    initial = book()
    old = len(active(initial)["strokes"])
    drag((650, 300), (820, 360))
    if len(active(book())["strokes"]) == old:
        print("BACKGROUND_NOOP: retry foreground only on isolated Xvfb", flush=True)
        state("before-isolated-retry")
        c.call("drag", {**target, "from_x": 650, "from_y": 300, "to_x": 820, "to_y": 360,
                        "duration_ms": 450, "steps": 24, "delivery_mode": "foreground"})
        state("after-isolated-retry")
    check(len(active(book())["strokes"]) == old + 1, "real canvas gesture creates one stroke")
    ink = copy.deepcopy(active(book())["strokes"])
    first_id = book()["active"]
    check(book()["base_strokes"] == initial["base_strokes"], "note gesture does not reach underlying page")
    button("Ẩn")
    check(not book()["visible"] and active(book())["strokes"] == ink, "hide keeps every ink point")
    button("Ẩn/hiện nháp • Alt+N")
    check(book()["visible"] and active(book())["strokes"] == ink, "reopen restores previous note")
    hotkey(["alt", "n"])
    check(not book()["visible"] and active(book())["strokes"] == ink, "Alt+N hides without removing ink")
    hotkey(["alt", "n"])
    check(book()["visible"] and active(book())["strokes"] == ink, "Alt+N restores same note")
    before = book()["page"]
    button("Trang đề kế")
    check(book()["page"] == before + 1 and book()["active"] == first_id and active(book())["strokes"] == ink,
          "changing background page keeps active note")
    button("Tờ mới")
    check(book()["active"] != first_id and active(book())["strokes"] == [], "new sheet preserves old sheet")
    button("Nháp 1")
    check(book()["active"] == first_id and active(book())["strokes"] == ink, "note list restores exact old sheet")
    button("Hoàn tác nháp")
    check(len(active(book())["strokes"]) == old, "undo targets note ink only")
    button("Làm lại")
    check(active(book())["strokes"] == ink and book()["base_strokes"] == initial["base_strokes"],
          "redo restores note without touching page")
    state("before-scroll")
    args = {**target, "direction": "down", "amount": 8, "x": 760, "y": 400}
    try:
        c.call("scroll", args)
    except RuntimeError as error:
        if "background_unavailable" not in str(error):
            raise
        state("before-isolated-scroll")
        c.call("scroll", {**args, "delivery_mode": "foreground"})
    state("after-scroll")
    check(active(book())["scroll"] > 0 and active(book())["strokes"] == ink,
          "scroll reaches more paper without moving stored ink")
    scroll = active(book())["scroll"]
    button("Nháp 2")
    button("Nháp 1")
    check(abs(active(book())["scroll"] - scroll) < 1,
          "switching notes restores independent scroll position")
    drag((650, 300), (820, 345))
    latest = active(book())["strokes"]
    check(len(latest) == len(ink) + 1 and min(p[1] for p in latest[-1]) >= scroll,
          "writing after scroll uses paper coordinates beyond viewport")
    ink = copy.deepcopy(latest)
    height = active(book())["height"]
    button("Thêm giấy")
    check(active(book())["height"] == height + 1800 and active(book())["strokes"] == ink,
          "more paper does not scale old ink")
    # Drawing-area pixels are grounded in the screenshot already captured above.
    drag((740, 176), (540, 245))
    check(book()["panel"][0] < 600 and active(book())["strokes"] == ink, "header drag moves panel without drawing")
    button("Cửa sổ nhỏ")
    s = state("small-window")
    x, y, w, h = book()["panel"]
    check(x >= 8 and y >= 8 and x + w <= s["screenshot_width"] - 8,
          "smaller window keeps note controls reachable")
    button("Tải lại bản lưu thử")
    check(active(book())["strokes"] == ink and len(book()["notes"]) == 2, "reload restores ink and both notes")
    state("final")
    before_close = copy.deepcopy(book())
    button("Đóng thử nghiệm")
    result = c.call("launch_app", {"name": str(ROOT / "scripts/teaching-tools/research-floating-notes/launch.sh")})
    pid = result["pid"]
    windows = result.get("windows", []) or c.call("list_windows", {"pid": pid})["windows"]
    wid = windows[0]["window_id"]
    target = {"pid": pid, "window_id": wid}
    state("restarted")
    check(book() == before_close, "new process restores saved prototype notebook exactly")
    button("Đóng thử nghiệm")
    (LOG / "gui-results.json").write_text(json.dumps({"passed": passed, "count": len(passed),
        "scope": "GTK prototype only, isolated X11; not Xournal++ stylus/save/export acceptance"}, ensure_ascii=False, indent=2))
    c.process.stdin.close()
    c.process.wait(timeout=10)
    wm.terminate()
    wm.wait(timeout=5)


if __name__ == "__main__":
    main()
