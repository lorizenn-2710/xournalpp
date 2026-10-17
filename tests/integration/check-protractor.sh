#!/bin/bash
# Kiểm chứng thước đo độ (protractor) ở mức runtime: mở app thật dưới Xvfb,
# bật/tắt action "protractor" trên object window, chụp màn hình, rồi kiểm:
#   1. action "protractor" có mặt trong bảng action của window,
#   2. bật -> số điểm mực tăng, tắt -> quay lại đúng như ban đầu (residual = 0),
#   3. hình học (nửa đường tròn đúng bán kính, cạnh thẳng liền mạch, ~181 vạch
#      chia 1°, vành số đo, thân có tô màu) -> đo bằng protractor_shape.py.
# Dùng: APP=<binary> bash tests/integration/check-protractor.sh
# Ghi chú: chạy với XDG_CONFIG_HOME/XDG_DATA_HOME riêng để không vướng hộp
# thoại "Recovery file detected" (emergencysave.xopp trong config thật che canvas).
# Nhưng config mới tinh thì app abort ngay khi khởi động: PortAudio ném
# "Unanticipated host error" (disableAudio=false + không có thiết bị âm thanh
# dưới Xvfb) -> SIGABRT. Vì vậy chỉ mượn settings.xml + toolbar.ini (chứa
# disableAudio=true), không mượn emergencysave.xopp.
set -u
D=/tmp/xpp-pr
rm -rf "$D"; mkdir -p "$D/run" "$D/cfg/xournalpp" "$D/data"; chmod 700 "$D/run"
for f in settings.xml toolbar.ini; do
    [ -f "$HOME/.config/xournalpp/$f" ] && cp "$HOME/.config/xournalpp/$f" "$D/cfg/xournalpp/$f"
done
export XDG_RUNTIME_DIR=$D/run
export XDG_CONFIG_HOME=$D/cfg
export XDG_DATA_HOME=$D/data
APP=${APP:-/home/phuc/xpp-src/build/xournalpp-wrapper}
: >"$D/act.err"; : >"$D/shot.err"
Xvfb :77 -screen 0 1500x1100x24 >"$D/xvfb.log" 2>&1 &
XVFB=$!
trap 'kill $XVFB 2>/dev/null' EXIT
sleep 1.5
export DISPLAY=:77

APP="$APP" dbus-run-session -- bash -s <<'EOF'
set -u
D=/tmp/xpp-pr
WINOBJ=/com/github/xournalpp/xournalpp/window/1
"$APP" >"$D/app.log" 2>&1 &
sleep 1

# Tên bus: GApplication của Xournal++ không đăng ký well-known name, chỉ có
# unique name (:1.x) -> phải dò theo PID thay vì grep chuỗi "xournalpp".
# busctl in kèm cột PID/PROCESS nên lọc theo process name là rẻ nhất.
# (Lưu ý: gdbus GetConnectionUnixProcessID trả "uint32 1234" -> nếu parse bằng
#  grep -o '[0-9]\+' phải lấy khớp CUỐI, không thì ra "32" của chữ uint32.)
N=""
for i in $(seq 1 160); do
    for nm in $(busctl --user list 2>/dev/null | awk '$3=="xournalpp"{print $1}'); do
        if gdbus call --session --dest "$nm" --object-path "$WINOBJ" \
             --method org.gtk.Actions.DescribeAll 2>/dev/null | grep -q "'protractor'"; then
            N=$nm; break
        fi
    done
    [ -n "$N" ] && break
    sleep 0.25
done
echo "BUS=${N:-NONE}"
[ -n "$N" ] || { echo "FAIL: không tìm được object window có action protractor"; pkill -f xournalpp; exit 1; }

act() {
    gdbus call --session --dest "$N" --object-path "$WINOBJ" \
        --method org.gtk.Actions.Activate "$1" '[]' '{}' >>"$D/act.err" 2>&1
}
state() {  # trạng thái bật/tắt của action, theo DescribeAll
    gdbus call --session --dest "$N" --object-path "$WINOBJ" \
        --method org.gtk.Actions.Describe "$1" 2>/dev/null | tr ',' '\n' | sed -n '1p'
}

import -window root "$D/00-initial.png" 2>>"$D/shot.err"
sleep 0.4
act protractor; echo "ACT_ON_RC=$?"; sleep 1.2
import -window root "$D/01-on.png" 2>>"$D/shot.err"
sleep 0.3
act protractor; echo "ACT_OFF_RC=$?"; sleep 1.0
import -window root "$D/02-off.png" 2>>"$D/shot.err"
pkill -f xournalpp 2>/dev/null
EOF

python3 - "$D" <<'PY'
import sys, math
from PIL import Image, ImageChops, ImageStat

D = sys.argv[1]
on  = Image.open(f"{D}/01-on.png").convert("L")
ini = Image.open(f"{D}/00-initial.png").convert("L")
off = Image.open(f"{D}/02-off.png").convert("L")
ink = lambda im: sum(im.histogram()[:200])

ok = True
def check(name, cond, extra=""):
    global ok
    ok &= bool(cond)
    print(f"{'PASS' if cond else 'FAIL'} {name}{(' ' + extra) if extra else ''}")

i0, i1, i2 = ink(ini), ink(on), ink(off)
print(f"INK initial={i0} on={i1} off={i2}")
check("bật protractor có vẽ thêm", i1 - i0 > 2000, f"delta={i1 - i0}")
check("tắt protractor xoá sạch", i2 == i0, f"residual_ink={abs(i2 - i0)}")
res = sum(1 for p in ImageChops.difference(ini, off).get_flattened_data() if p > 40)
check("ảnh sau khi tắt giống hệt ban đầu", res == 0, f"pixel_khác={res}")


sys.exit(0 if ok else 1)
PY
# Hình học chi tiết (nửa đường tròn đúng R, cạnh thẳng liền mạch, ~181 vạch 1°, vành số đo,
# thân có tô màu) — đo bằng script riêng trên chính hai ảnh chụp này.
py_rc=$?
GEO="$(cd "$(dirname "$0")" && pwd)/protractor_shape.py"
python3 "$GEO" "$D/00-initial.png" "$D/01-on.png"
geo_rc=$?
if [ "$py_rc" -eq 0 ] && [ "$geo_rc" -eq 0 ]; then
    echo "KẾT LUẬN: PASS"
    exit 0
fi
echo "KẾT LUẬN: FAIL (ink=$py_rc hình_học=$geo_rc)"
exit 1
