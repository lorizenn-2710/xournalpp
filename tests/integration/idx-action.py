#!/usr/bin/env python3
"""In ra số hiệu action ("plugins.action-N") của hai hàm trong plugin THẬT (VK640ToolCycle).

Cách tính: Xournal++ đánh số action bằng một bộ đếm DÙNG CHUNG cho mọi plugin, theo thứ tự
plugin được nạp. Ở harness này `pluginEnabled` để EraseProbe TRƯỚC, nên:
    action của plugin thật = (số registerUi của EraseProbe) + (vị trí registerUi trong plugin thật)

    python3 idx-action.py <erseprobe/main.lua> <vk640/main.lua> sel2img|notepages|list

Không ghi cứng con số vào script chạy: ai thêm bớt một menu là số hiệu đổi, tính lại từ nguồn là đúng.
"""
import re
import sys

ep, vk, what = sys.argv[1], sys.argv[2], (sys.argv[3] if len(sys.argv) > 3 else "list")

so_erpr = len(re.findall(r"registerUi\s*\{", open(ep, encoding="utf-8").read()))
ten = re.findall(r"registerUi\s*\{[^}]*callback\s*=\s*\"([^\"]+)\"", open(vk, encoding="utf-8").read())

CAN = {"sel2img": "vk640SelectionToImage", "notepages": "vk640EraseNotePagesInk"}

if what == "list":
    for i, t in enumerate(ten):
        print(so_erpr + i, t)
    raise SystemExit(0)

ham = CAN[what]
if ham not in ten:
    sys.exit("LỖI: không thấy callback %r trong %s (đã đổi tên hàm?)" % (ham, vk))
print(so_erpr + ten.index(ham))
