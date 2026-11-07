#!/usr/bin/env python3
"""Bơm phím thật vào app X11 qua XTEST (chạy được cả trên Xvfb).

Vì sao cần: harness check-protractor.sh chỉ bật/tắt action bằng gdbus nên chỉ
kiểm được phần VẼ. Các phím điều khiển thước (mũi tên = dịch, R = xoay,
S = phóng) chỉ chạy khi có sự kiện bàn phím thật. Trên Wayland, GTK từ chối
event bàn phím ảo gửi bằng XTEST; trên X11/Xvfb thì XTEST là sự kiện mức
X-server nên GTK nhận như phím thật.

Dùng: xkey.py <display> <window-class-substr> KEY[:count] ...
Ví dụ: xkey.py :77 xournalpp Left:8 R:18 S:3
"""
import sys
import time

from Xlib import X, XK, display
from Xlib.ext import xtest


def find_windows(d, win, needle, out):
    """Duyệt cây cửa sổ, gom những cửa sổ có WM_CLASS khớp needle."""
    try:
        cls = win.get_wm_class()
    except Exception:
        cls = None
    if cls and any(needle in c.lower() for c in cls):
        # GTK also creates an unmapped 10x10 leader window with the same class.
        # XSetInputFocus on that window fails with BadMatch and sends no keys.
        attrs = win.get_attributes()
        geo = win.get_geometry()
        if attrs.map_state == X.IsViewable and geo.width > 100 and geo.height > 100:
            out.append(win)
    for child in win.query_tree().children:
        find_windows(d, child, needle, out)


def main():
    if len(sys.argv) < 3:
        print("usage: xkey.py <display> <class-substr> KEY[:count] ...", file=sys.stderr)
        return 2
    disp_name, needle = sys.argv[1], sys.argv[2].lower()
    d = display.Display(disp_name)

    wins = []
    find_windows(d, d.screen().root, needle, wins)
    if not wins:
        print("KHÔNG tìm thấy cửa sổ khớp class", needle, file=sys.stderr)
        return 3

    target = max(wins, key=lambda w: w.get_geometry().width * w.get_geometry().height)
    geo = target.get_geometry()
    print(f"WINDOW id=0x{target.id:x} {geo.width}x{geo.height}")
    # Xvfb không có WM: phải tự đặt input focus, nếu không phím rơi vào root.
    target.set_input_focus(X.RevertToParent, X.CurrentTime)
    d.sync()
    time.sleep(0.2)

    focused = d.get_input_focus().focus
    if getattr(focused, "id", focused) != target.id:
        print("KHÔNG đặt được focus vào cửa sổ app", file=sys.stderr)
        return 4

    total = 0
    for spec in sys.argv[3:]:
        name, _, cnt = spec.partition(":")
        count = int(cnt) if cnt else 1
        keysym = XK.string_to_keysym(name)
        if keysym == 0:
            print("keysym lạ:", name, file=sys.stderr)
            return 5
        code = d.keysym_to_keycode(keysym)
        if code == 0:
            print("không có keycode cho", name, file=sys.stderr)
            return 6
        for _ in range(count):
            xtest.fake_input(d, X.KeyPress, code)
            d.sync()
            time.sleep(0.03)
            xtest.fake_input(d, X.KeyRelease, code)
            d.sync()
            time.sleep(0.09)
            total += 1
        print(f"SENT {name} x{count} (keycode={code})")
    print("TOTAL_KEYS", total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
