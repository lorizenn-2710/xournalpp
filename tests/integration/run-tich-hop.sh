#!/bin/bash
# Kiểm thử TÍCH HỢP đầu-cuối cho hai thay đổi mới của bản Xournal++ riêng (~/xpp-src):
#   A. app.selectionAsImage: mặc định phải là "copy" (GIỮ nét gốc).
#   B. app.deleteStrokes("notePages"): xoá MỰC BÚT trên mọi trang giấy nháp, KHÔNG xoá ảnh/chữ/nền/trang.
#
# Chạy: bash tests/integration/run-tich-hop.sh
# Cách làm: dựng một cấu hình Xournal++ riêng ở /tmp/xpp-kt2 (không đụng cấu hình thật của Phúc),
# chạy app THẬT trong Xvfb + bus D-Bus riêng (không hiện cửa sổ, không đụng desktop đang dùng),
# bấm lần lượt các action của plugin EraseProbe (mỗi action = 1 bước probe), lưu log từng bước,
# rồi để check-tich-hop.py đối chiếu số liệu.
set -u
D=/tmp/xpp-kt2
S=$(cd "$(dirname "$0")" && pwd)
VK_DIR="$S/../../plugins/VK640ToolCycle"
APP=$HOME/.local/bin/xournalpp

if [ ! -x "$APP" ]; then echo "LỖI: không thấy $APP"; exit 1; fi

# --- dựng instance riêng ---
rm -rf "$D"
mkdir -p "$D/xournalpp/plugins"
cp -r "$S/EraseProbe" "$D/xournalpp/plugins/"
# Plugin THẬT của Phúc cũng được nạp vào instance này (phần C gọi thẳng callback của nó).
cp -r "$VK_DIR" "$D/xournalpp/plugins/"
cp "$HOME/.config/xournalpp/settings.xml" "$D/xournalpp/settings.xml"
python3 - "$D/xournalpp/settings.xml" "$D" <<'PY'
import re, sys
p, d = sys.argv[1], sys.argv[2]
s = open(p, encoding="utf-8").read()
# THỨ TỰ ở đây quyết định số hiệu action: EraseProbe 0..29, rồi tới plugin thật 30..45.
new = ('name="pluginEnabled" value="%s/xournalpp/plugins/EraseProbe,%s/xournalpp/plugins/VK640ToolCycle"'
       % (d, d))
s2, n = re.subn(r'name="pluginEnabled" value="[^"]*"', new, s)
if n == 0:
    s2 = s.replace("</settings>", '  <property %s/>\n</settings>' % new)
open(p, "w", encoding="utf-8").write(s2)
print("pluginEnabled ->", d + "/xournalpp/plugins/{EraseProbe,VK640ToolCycle}")
PY

# --- số hiệu action của plugin thật, tính TỪ NGUỒN (không chép tay, tự đúng khi ai thêm bớt) ---
VK="$VK_DIR/main.lua"
IDX_SEL2IMG=$(python3 "$S/idx-action.py" "$S/EraseProbe/main.lua" "$VK" sel2img)
IDX_NOTEPAGES=$(python3 "$S/idx-action.py" "$S/EraseProbe/main.lua" "$VK" notepages)
export IDX_SEL2IMG IDX_NOTEPAGES
echo "số hiệu action của plugin thật: chọn->ảnh=$IDX_SEL2IMG  xoá mực giấy nháp=$IDX_NOTEPAGES"

# --- ảnh PNG thật cho probe (thuần thư viện chuẩn) ---
python3 - "$D/probe-img.png" <<'PY'
import struct, sys, zlib
w = h = 64
rows = []
for y in range(h):
    row = bytearray([0])
    for x in range(w):
        row += bytes(((x * 4) % 256, (y * 4) % 256, 160))
    rows.append(bytes(row))
raw = b"".join(rows)
def chunk(t, d):
    return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
png = (b"\x89PNG\r\n\x1a\n"
       + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
       + chunk(b"IDAT", zlib.compress(raw))
       + chunk(b"IEND", b""))
open(sys.argv[1], "wb").write(png)
print("ảnh mẫu:", sys.argv[1], len(png), "byte")
PY

echo "== chạy app trong Xvfb + bus riêng =="
xvfb-run -a -s "-screen 0 1600x1200x24" dbus-run-session -- bash "$S/run-tich-hop-trong.sh"
RC=$?

echo
echo "== đối chiếu số liệu =="
python3 "$S/check-tich-hop.py"; CRC=$?
exit $(( RC != 0 ? RC : CRC ))
