#!/bin/bash
# Kiểm chứng TƯƠNG TÁC của thước đo độ (protractor): bật thước dưới Xvfb rồi
# bơm phím THẬT bằng XTEST (xkey.py) và đo hình học của mực trên màn hình:
#   - Left x8 (mỗi phím 0.25 cm)  -> thước dịch sang trái, đo dx (px)
#   - R x18   (mỗi phím 5°)       -> xoay 90°: bbox phải đổi 2R×R thành R×2R
#   - S x3    (mỗi phím ×1.1)     -> diện tích mực ×1.331² ≈ 1.77
# Chỉ đo trên mask = |ảnh_có_thước − ảnh_tắt_thước| nên không lẫn toolbar.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
D=/tmp/xpp-pk
rm -rf "$D"; mkdir -p "$D/run" "$D/cfg/xournalpp" "$D/data" "$D/shot"
chmod 700 "$D/run"
# Mượn settings.xml + toolbar.ini của máy (disableAudio=true): thiếu nó app SIGABRT
# dưới Xvfb vì không có thiết bị âm thanh -> không có cửa sổ nào để bơm phím.
for f in settings.xml toolbar.ini; do
    [ -f "$HOME/.config/xournalpp/$f" ] && cp "$HOME/.config/xournalpp/$f" "$D/cfg/xournalpp/$f"
done
export XDG_RUNTIME_DIR=$D/run XDG_CONFIG_HOME=$D/cfg XDG_DATA_HOME=$D/data
# Mỗi lần chạy một display riêng: Xvfb còn sót của lần trước làm Xvfb mới chết
# (Fatal server error: already active) rồi mọi bước sau lặng lẽ sai.
DISP=":$(( 100 + ($$ % 60) ))"
export DISPLAY=$DISP
APP=${APP:-/home/phuc/xpp-src/build/xournalpp-wrapper}
TW=${TW:-$HERE}
Xvfb $DISP -screen 0 1500x1100x24 >"$D/xvfb.log" 2>&1 &
XVFB=$!
trap 'kill $XVFB 2>/dev/null' EXIT
sleep 1.5

APP="$APP" TW="$TW" D="$D" dbus-run-session -- bash -s <<'EOS'
set -u
FIG=$(mktemp -d)
"$APP" >"$D/app.log" 2>&1 &
APPPID=$!
sleep 2
WINOBJ=/com/github/xournalpp/xournalpp/window/1
NM=""
for i in $(seq 1 160); do
    for nm in $(busctl --user list 2>/dev/null | awk '$3=="xournalpp"{print $1}'); do
        if gdbus call --session --dest "$nm" --object-path "$WINOBJ" \
            --method org.freedesktop.DBus.Properties.Get \
            com.github.xournalpp.xournalpp.Window visible >/dev/null 2>&1; then
            NM=$nm; break
        fi
    done
    [ -n "$NM" ] && break
    sleep 0.5
done
echo "BUS=${NM:-KHONG_THAY}"
grab() { import -display "$DISPLAY" -window root "$D/shot/$1.png" 2>/dev/null; }

grab 00-off
gdbus call --session --dest "$NM" --object-path "$WINOBJ" \
    --method com.github.xournalpp.xournalpp.Window.activateAction protractor \
    >>"$D/act.log" 2>&1
sleep 1.2
grab 01-on

/usr/bin/python3 "$TW/xkey.py" "$DISPLAY" xournalpp Left:8 || echo "XKEY_MOVE_FAIL"
sleep 0.8; grab 02-move
/usr/bin/python3 "$TW/xkey.py" "$DISPLAY" xournalpp R:18 || echo "XKEY_ROT_FAIL"
sleep 0.8; grab 03-rot
/usr/bin/python3 "$TW/xkey.py" "$DISPLAY" xournalpp S:3 || echo "XKEY_SCALE_FAIL"
sleep 0.8; grab 04-scale

kill $APPPID 2>/dev/null
wait $APPPID 2>/dev/null
EOS

/usr/bin/python3 - "$D" <<'PY'
import sys, glob
import numpy as np
from PIL import Image

D = sys.argv[1]
def load(n):
    return np.asarray(Image.open(f"{D}/shot/{n}.png").convert("L"), dtype=np.int16)

off = load("00-off")
def mask(n):
    a = load(n)
    return np.abs(a - off) > 40

def stats(m):
    ys, xs = np.nonzero(m)
    if len(xs) == 0:
        return None
    x0, x1, y0, y1 = xs.min(), xs.max(), ys.min(), ys.max()
    return dict(area=int(m.sum()), w=int(x1-x0+1), h=int(y1-y0+1),
                cx=float(xs.mean()), cy=float(ys.mean()))

S = {n: stats(mask(n)) for n in ("01-on", "02-move", "03-rot", "04-scale")}
for n, s in S.items():
    print(f"{n}: " + (f"area={s['area']} bbox={s['w']}x{s['h']} "
                      f"centroid=({s['cx']:.1f},{s['cy']:.1f})" if s else "KHÔNG có mực"))

fails = []
def check(cond, name, extra=""):
    global fails
    if not cond:
        fails.append(name)
    print(f"{'PASS' if cond else 'FAIL'} {name} {extra}".rstrip())

b, mv, rot, scl = S["01-on"], S["02-move"], S["03-rot"], S["04-scale"]
check(b is not None and b["area"] > 500,
      "thước có mực khi bật", f"area={b and b['area']}")

if mv and b:
    dx, dy = mv["cx"] - b["cx"], mv["cy"] - b["cy"]
    check(dx < -8 and abs(dy) < 6, "Left x8 dịch sang trái",
          f"dx={dx:.1f}px dy={dy:.1f}px (8 phím × 0.25 cm = 2 cm)")
    check(abs(mv["area"] - b["area"]) / b["area"] < 0.06,
          "dịch chuyển không đổi kích thước",
          f"area {b['area']} -> {mv['area']}")

if rot and mv:
    r_now, r_before = rot["h"] / rot["w"], mv["h"] / mv["w"]
    check(r_now > 1.4 and r_before < 0.75, "R x18 xoay 90° (bbox đổi chiều)",
          f"h/w {r_before:.2f} -> {r_now:.2f}; bbox {mv['w']}x{mv['h']} -> {rot['w']}x{rot['h']}")

if scl and rot:
    ratio = scl["area"] / rot["area"]
    check(1.45 <= ratio <= 2.10, "S x3 phóng ×1.331 (diện tích ×1.77)",
          f"tỷ lệ diện tích = {ratio:.3f} (lý thuyết 1.772)")
    lin = scl["h"] / rot["h"]
    check(1.20 <= lin <= 1.45, "S x3 phóng tuyến tính ×1.331",
          f"cạnh dài {rot['h']}px -> {scl['h']}px = ×{lin:.3f}")

print("KẾT LUẬN:", "FAIL " + ",".join(fails) if fails else "PASS")
PY
