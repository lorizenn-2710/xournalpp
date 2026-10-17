#!/usr/bin/python3
"""Đọc các hộp thoại đang mở của app Xournal++ (pid truyền vào) qua AT-SPI."""
import sys
import pyatspi

target_pid = int(sys.argv[1])

def walk(node, depth=0):
    try:
        role = node.getRoleName()
        name = node.name or ""
        texts = []
        try:
            t = node.queryText()
            if t and t.characterCount:
                texts.append(t.getText(0, t.characterCount))
        except Exception:
            pass
        if role in ("dialog", "frame", "alert", "push button", "label"):
            print("  " * depth + f"[{role}] {name[:60]!r}")
        for txt in texts:
            print("  " * depth + f"    text: {txt[:400]!r}")
        for i in range(node.childCount):
            walk(node.getChildAtIndex(i), depth + 1)
    except Exception as e:
        print("!!", e)

desktop = pyatspi.Registry.getDesktop(0)
for app in desktop:
    try:
        if app.get_process_id() != target_pid:
            continue
        print("APP:", app.name, "pid", app.get_process_id())
        for i in range(app.childCount):
            walk(app.getChildAtIndex(i))
    except Exception as e:
        print("err", e)
