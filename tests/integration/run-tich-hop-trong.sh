#!/bin/bash
# Phần chạy BÊN TRONG phiên Xvfb + D-Bus riêng (do run-tich-hop.sh gọi).
# Mở app thật, chờ nó lên bus, rồi bấm lần lượt action của probe và lưu log từng bước.
set -u
D=/tmp/xpp-kt2
S=$(cd "$(dirname "$0")" && pwd)
APP=$HOME/.local/bin/xournalpp

export XDG_CONFIG_HOME="$D"
unset WAYLAND_DISPLAY
export GDK_BACKEND=x11
export NO_AT_BRIDGE=1

rm -f "$D/xournalpp/emergencysave.xopp" "$D/bus.txt"
setsid "$APP" >"$D/app.log" 2>&1 &

# Chờ tên bus: chỉ thấy app CỦA MÌNH vì đây là bus riêng của dbus-run-session.
for i in $(seq 1 120); do
    busctl --user list 2>/dev/null | awk '$3 == "xournalpp" {print $1; exit}' > "$D/bus.txt"
    [ -s "$D/bus.txt" ] && break
    sleep 0.5
done
if [ ! -s "$D/bus.txt" ]; then
    echo "LỖI: app không lên bus sau 60s"; tail -30 "$D/app.log"; exit 1
fi
echo "bus: $(cat "$D/bus.txt")"

# Chờ app sẵn sàng nhận action.
for i in $(seq 1 60); do
    XPPKT=$D bash "$S/act.sh" 0 > "$D/out-0-warm.txt" 2>&1
    grep -q 'strokes=' "$D/out-0-warm.txt" && break
    sleep 0.5
done

go() { XPPKT=$D bash "$S/act.sh" "$1" > "$D/out-$2.txt" 2>&1; }

# ---------- A. ảnh từ vùng chọn: mặc định phải là COPY ----------
go 12 12-fig          # vẽ hình (3 nét) + chọn 3 nét đó
go 29 29-sel1         # in vùng chọn hiện tại (3 nét / 0 ảnh)
go 15 15-default      # gọi KHÔNG tham số -> mặc định: nét gốc phải CÒN
go 29 29-sel2         # sau khi probe 15 truy vấn mức "page": vùng chọn đã bị bỏ (tác dụng phụ có sẵn của API)
go 18 18-geom         # kích thước ảnh vừa tạo
go 19 19-png          # đổ dữ liệu PNG của ảnh (kiểm chữ ký PNG + có mực)
go 16 16-nosel        # không có vùng chọn -> no-op
go 23 23-clear        # xoá nét/chữ trên trang
go 27 27-clearimg     # xoá ảnh trên trang
go 0  0-sach          # trang phải sạch (0 nét / 0 ảnh / 0 chữ)

# ---------- A2. đối chứng: "move" thì nét gốc PHẢI mất ----------
go 12 12-fig2         # vẽ lại 3 nét + chọn
go 13 13-move         # move -> nét gốc bị thay bằng ảnh
go 8  8-undo3         # undo -> nét trở lại
go 23 23-clear2       # dọn nét
go 0  0b-sach         # xác nhận sạch trước khi sang phần B

# ---------- B. app.deleteStrokes("notePages") ----------
go 25 25-fixture      # 1 trang thường (3 nét/1 ảnh/1 chữ) + 2 trang giấy nháp (2 nét/1 ảnh/1 chữ)
go 24 24b-pages       # liệt kê từng trang (chứng minh trang nào là giấy nháp)
go 26 26-erase        # app.deleteStrokes("notePages")
go 28 28-all1         # thống kê mức toàn tài liệu ngay sau khi xoá
go 24 24c-after       # liệt kê lại: nét biến mất, ảnh/chữ/trang còn nguyên
go 8  8b-undo         # undo -> 4 nét trở lại
go 28 28-all2         # thống kê sau undo (phải là MỘT bước undo duy nhất)
go 9  9-redo         # redo -> lại sạch nét
go 28 28-all3         # thống kê sau redo

# ---------- C. gọi callback của PLUGIN THẬT (đúng đường mà phím tắt kích hoạt) ----------
# Điều kiện tiên quyết: cả hai plugin phải đã đăng ký action (30 probe + 16 VK640 = 46).
{
    echo "SỐ ACTION PLUGIN=$(gdbus call --session --dest "$(cat "$D/bus.txt")" --object-path /com/github/xournalpp/xournalpp/window/1 \
        --method org.gtk.Actions.DescribeAll 2>/dev/null | grep -o 'plugins\.action-[0-9]*' | sort -u | wc -l)"
} > "$D/out-E0-actions.txt"

# Phần C KHÔNG giả định tài liệu sạch (phần B để lại nội dung ở nhiều trang), nên lấy MỐC trước rồi mới so.
go 28 28-allC0        # mốc toàn tài liệu
go 24 24-pagesC0      # mốc từng trang; kết thúc ở TRANG CUỐI = trang làm việc của C1

# --- C1: vùng chọn -> ảnh, bằng callback THẬT (Ctrl+Shift+C) ---
go 12 12-figC         # +3 nét trên trang đang xem và CHỌN chúng
go 29 29-selC1        # xác nhận: đang chọn 3 nét / 0 ảnh
go "${IDX_SEL2IMG:-44}" C-sel2img   # <-- CALLBACK THẬT: vk640SelectionToImage (Ctrl+Shift+C)
go 29 29-selC2        # đo vùng chọn NGAY sau đó (trước mọi stats: truy vấn mức trang xoá vùng chọn)
go 0  0-statC1        # trang đang xem sau khi plugin chạy
go 8  8-undoC         # một undo: ảnh mới biến mất, nét gốc phải còn nguyên
go 0  0-statC2
go 23 23-clearC       # dọn nét + chữ trên trang đang xem
go 27 27-clearimgC    # dọn ảnh trên trang đang xem
go 0  0-sachC

# --- C2: xoá mực giấy nháp, bằng callback THẬT (Ctrl+Shift+M) ---
go 25 25-fixtureC     # fixture (dựng trên trang đang xem + thêm 2 trang giấy nháp)
go 28 28-allC1        # mốc NGAY TRƯỚC lệnh của plugin
go "${IDX_NOTEPAGES:-41}" C-notepages   # <-- CALLBACK THẬT: vk640EraseNotePagesInk (Ctrl+Shift+M)
go 28 28-allC2        # sau lệnh: nét giảm ĐÚNG bằng số nét giấy nháp của mốc
go 24 24-pagesC       # liệt kê từng trang: nét biến mất ở đúng các trang nháp

pkill -f "^$APP$" 2>/dev/null || pkill -x xournalpp 2>/dev/null || true
sleep 1
echo "XONG"
