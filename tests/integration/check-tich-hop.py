#!/usr/bin/env python3
"""Đối chiếu log kiểm thử tích hợp trong /tmp/xpp-kt2/out-*.txt với các con số MONG ĐỢI.

Mỗi mục là một câu khẳng định có thể sai được; in PASS/FAIL kèm số đọc được từ log thật.
Thoát mã 1 nếu có mục FAIL.
"""
import os
import re
import struct
import sys
import zlib

D = "/tmp/xpp-kt2"
kq = []


def doc(ten):
    p = os.path.join(D, "out-%s.txt" % ten)
    if not os.path.exists(p):
        return None
    return open(p, encoding="utf-8", errors="replace").read()


def _last(ten, mau):
    t = doc(ten)
    if t is None:
        return None
    m = re.findall(mau, t)
    return tuple(int(x) for x in m[-1]) if m else None


def page_stats(ten):
    """Số liệu mức TRANG: 'strokes=N images=N texts=N pages=N' (bỏ qua dòng '(all)')."""
    return _last(ten, r"\[PROBE\][^\n]*?strokes=(\d+) images=(\d+) texts=(\d+) pages=(\d+)")


def all_stats(ten):
    """Số liệu toàn tài liệu: 'strokes(all)=N strokes(notePages)=N images(all)=N texts(all)=N pages=N'."""
    return _last(
        ten,
        r"strokes\(all\)=(\d+) strokes\(notePages\)=(\d+) images\(all\)=(\d+) texts\(all\)=(\d+) pages=(\d+)",
    )


def sel_stats(ten):
    """Vùng chọn hiện tại: 'ĐANG CHỌN: strokes=N images=N'."""
    return _last(ten, r"ĐANG CHỌN: strokes=(\d+) images=(\d+)")


def dong(ten, mau):
    """Dòng đầu tiên khớp mẫu, để in ra làm bằng chứng."""
    for ln in (doc(ten) or "").splitlines():
        if re.search(mau, ln):
            return ln.strip()
    return "(không thấy dòng khớp)"


def kiem(mo_ta, ok, doc_duoc):
    kq.append((mo_ta, bool(ok), doc_duoc))


def _khu_loc(surow, rong, cao, chan):
    """Khử bộ lọc từng dòng của PNG (None/Sub/Up/Average/Paeth) -> danh sách dòng điểm ảnh thật."""
    bpp = chan
    stride = rong * chan
    prev = bytearray(stride)
    dong_anh = []
    pos = 0
    for _ in range(cao):
        ft = surow[pos]
        pos += 1
        line = bytearray(surow[pos : pos + stride])
        pos += stride
        if ft == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif ft == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ft == 3:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif ft == 4:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                b = prev[i]
                c = prev[i - bpp] if i >= bpp else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        elif ft != 0:
            return None
        prev = line
        dong_anh.append(line)
    return dong_anh


def giai_png_tu_hex(ten):
    """Lấy khối hex PNG ở probe 19 rồi giải mã -> (rộng, cao, số điểm CÓ MỰC).

    Phải khử bộ lọc từng dòng và để ý kênh alpha: nền trong suốt của Xournal++ thường lưu là
    RGB(0,0,0) alpha=0, nếu không xét alpha thì "đếm mực" sẽ ra 100% (sai hẳn).
    """
    t = doc(ten)
    if not t:
        return None
    m = re.search(r"PNGHEX-BEGIN\n([0-9a-fA-F]+)", t)
    if not m:
        return None
    raw = bytes.fromhex(m.group(1).strip())
    if raw[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    pos, w, h, chan, idat = 8, None, None, 0, b""
    while pos + 8 <= len(raw):
        ln = struct.unpack(">I", raw[pos : pos + 4])[0]
        typ = raw[pos + 4 : pos + 8]
        data = raw[pos + 8 : pos + 8 + ln]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", data[:8])
            bitdepth, colortype = data[8], data[9]
            chan = {0: 1, 2: 3, 4: 2, 6: 4}.get(colortype, 0)
            if bitdepth != 8 or not chan:
                return None
        elif typ == b"IDAT":
            idat += data
        pos += 12 + ln
    if not idat or not w:
        return None
    rong, cao = int(w), int(h or 0)
    dong_anh = _khu_loc(zlib.decompress(idat), rong, cao, chan)
    if dong_anh is None:
        return None
    co_muc = 0
    for line in dong_anh:
        for x in range(rong):
            o = x * chan
            if chan == 4 and line[o + 3] < 128:
                continue  # trong suốt = nền, không phải mực
            if chan == 2 and line[o + 1] < 128:
                continue  # xám + alpha: alpha thấp = nền
            if chan >= 3:
                r, g, b = line[o], line[o + 1], line[o + 2]
            else:
                r = g = b = line[o]
            if min(r, g, b) < 200:  # nền Xournal++ gần trắng; nét bút là đen
                co_muc += 1
    return rong, cao, co_muc


print("ĐỐI CHIẾU LOG KIỂM THỬ TÍCH HỢP (số liệu đọc lại từ /tmp/xpp-kt2/out-*.txt)")
print("-" * 78)

# ================= A. app.selectionAsImage: mặc định phải là COPY =================
s = sel_stats("29-sel1")
kiem("A1 vẽ hình xong: đang chọn 3 nét, 0 ảnh", s == (3, 0), "vùng chọn=%s" % (s,))

ps = page_stats("15-default")
kiem("A2 gọi KHÔNG tham số (mặc định phải là copy): nét gốc 3 -> VẪN 3, ảnh 0 -> 1", ps == (3, 1, 0, 1), "trang=%s" % (ps,))
kiem("A2b log báo ok=true (không lỗi)", "ok=true" in (doc("15-default") or ""), dong("15-default", r"ok="))

s = sel_stats("15-default")
kiem("A2c sau copy: ẢNH MỚI đang được chọn, nét gốc đã bỏ chọn (nét không bị xoá nhầm)", s == (0, 1), "vùng chọn=%s" % (s,))

g = re.search(r"w=([0-9.]+) h=([0-9.]+)", doc("18-geom") or "")
if g:
    w, h = float(g.group(1)), float(g.group(2))
    kiem(
        "A3 ảnh đúng cỡ hình đã vẽ (hộp 250x170 điểm + nét 2 -> 252x172, sai số ±6)",
        246 <= w <= 258 and 166 <= h <= 178,
        "w=%.1f h=%.1f" % (w, h),
    )
else:
    kiem("A3 ảnh đúng cỡ hình đã vẽ", False, "không đọc được w=/h= trong out-18-geom.txt")

kiem("A4 dữ liệu ảnh là PNG thật (chữ ký PNG)", "PNGMAGIC=OK" in (doc("19-png") or ""), dong("19-png", r"PNG bytes"))
png = giai_png_tu_hex("19-png")
if png:
    w2, h2, ink = png
    tong = w2 * h2
    ty_le = 100.0 * ink / tong if tong else 0.0
    # Ảnh trắng trơn -> 0% (FAIL); ảnh đặc đen hoặc giải mã sai stride -> ~100% (FAIL).
    kiem(
        "A5 ảnh có MỰC THẬT: tỷ lệ điểm khác trắng nằm trong khoảng hợp lý 2%..70%",
        2.0 <= ty_le <= 70.0,
        "%dx%d điểm, khác trắng=%d (%.2f%%)" % (w2, h2, ink, ty_le),
    )
else:
    kiem("A5 ảnh có mực thật", False, "không giải mã được PNG từ out-19-png.txt")

kiem("A6 không có vùng chọn -> không lỗi, không tạo gì (ok=true n=0)", "ok=true n=0" in (doc("16-nosel") or ""), dong("16-nosel", r"ok="))

ps = page_stats("0-sach")
kiem("A7 dọn xong: trang sạch 0 nét / 0 ảnh / 0 chữ", ps == (0, 0, 0, 1), "trang=%s" % (ps,))

# ================= A2. đối chứng: MOVE =================
ps = page_stats("13-move")
kiem("A8 đối chứng: MOVE thì nét gốc MẤT (0 nét, 1 ảnh) — khác hẳn copy", ps == (0, 1, 0, 1), "trang=%s" % (ps,))
ps = page_stats("8-undo3")
kiem("A9 undo sau move: nét trở lại (3 nét, 0 ảnh)", ps == (3, 0, 0, 1), "trang=%s" % (ps,))
ps = page_stats("0b-sach")
kiem("A10 sạch trước khi sang phần B", ps == (0, 0, 0, 1), "trang=%s" % (ps,))

# ================= B. app.deleteStrokes("notePages") =================
a = all_stats("25-fixture")
kiem("B1 fixture: 3 trang | nét toàn tài liệu 7 | nét trang nháp 4 | ảnh 3 | chữ 3", a == (7, 4, 3, 3, 3), "toàn tài liệu=%s" % (a,))

t24 = doc("24b-pages") or ""
mong = [
    "trang 1: nen='plain' pdfBackgroundPageNo=0 | net=3 anh=1 chu=1",
    "trang 2: nen='lined' pdfBackgroundPageNo=0 | net=2 anh=1 chu=1",
    "trang 3: nen='lined' pdfBackgroundPageNo=0 | net=2 anh=1 chu=1",
]
thieu = [d for d in mong if d not in t24]
kiem("B2 nhận diện đúng trang giấy nháp: trang 1 nền plain; trang 2-3 nền lined và KHÔNG có nền PDF", not thieu, "thiếu: %s" % (thieu or "không thiếu dòng nào"))

kiem('B3 deleteStrokes("notePages") xoá đúng 4 nét (ok=true n=4)', "ok=true n=4" in (doc("26-erase") or ""), dong("26-erase", r"n=4"))
a = all_stats("28-all1")
kiem("B4 sau khi xoá: nét trang nháp 0, nét toàn tài liệu còn 3 (trang thường nguyên vẹn)", a == (3, 0, 3, 3, 3), "toàn tài liệu=%s" % (a,))
kiem("B4b KHÔNG mất ảnh (vẫn 3) và KHÔNG mất chữ (vẫn 3)", a is not None and a[2] == 3 and a[3] == 3, "ảnh=%s chữ=%s" % (a[2] if a else "?", a[3] if a else "?"))
kiem("B4c KHÔNG mất trang (vẫn 3 trang)", a is not None and a[4] == 3, "trang=%s" % (a[4] if a else "?"))

t24c = doc("24c-after") or ""
mong = [
    "trang 1: nen='plain' pdfBackgroundPageNo=0 | net=3 anh=1 chu=1",
    "trang 2: nen='lined' pdfBackgroundPageNo=0 | net=0 anh=1 chu=1",
    "trang 3: nen='lined' pdfBackgroundPageNo=0 | net=0 anh=1 chu=1",
]
thieu = [d for d in mong if d not in t24c]
kiem("B5 liệt kê lại TỪNG TRANG: 2 trang nháp sạch nét nhưng vẫn còn 1 ảnh + 1 chữ, trang 1 không bị đụng", not thieu, "thiếu: %s" % (thieu or "không thiếu dòng nào"))

a = all_stats("28-all2")
kiem("B6 MỘT lần undo phục hồi đủ 4 nét (nét toàn tài liệu 3 -> 7, nét trang nháp 0 -> 4)", a == (7, 4, 3, 3, 3), "toàn tài liệu=%s" % (a,))
a = all_stats("28-all3")
kiem("B7 redo xoá lại đúng 4 nét đó (nét toàn tài liệu 3, nét trang nháp 0)", a == (3, 0, 3, 3, 3), "toàn tài liệu=%s" % (a,))

# ================= C. gọi callback của PLUGIN THẬT (đúng đường mà phím tắt kích hoạt) =================
# Phần B để lại nội dung trên nhiều trang nên C lấy MỐC từ log rồi so, không dùng số cứng.


def liet_ke_trang(ten):
    """[(số trang, kiểu nền, pdfBackgroundPageNo, nét, ảnh, chữ), ...] từ log của probe 24."""
    ra = []
    for ln in (doc(ten) or "").splitlines():
        m = re.search(
            r"trang (\d+): nen='([^']*)' pdfBackgroundPageNo=(\d+) \| net=(\d+) anh=(\d+) chu=(\d+)", ln
        )
        if m:
            ra.append(tuple(int(x) if i != 1 else x for i, x in enumerate(m.groups())))
    return ra


na = re.search(r"SỐ ACTION PLUGIN=(\d+)", doc("E0-actions") or "")
n_act = int(na.group(1)) if na else -1
kiem("C1 instance nạp ĐỦ hai plugin (>=46 action plugin: 30 probe + 16 VK640)", n_act >= 46, "số action=%d" % n_act)

base = liet_ke_trang("24-pagesC0")
tl = base[-1] if base else None
kiem(
    "C2 đọc được mốc trước phần C (%d trang, trang làm việc của C1 = trang cuối)" % len(base),
    len(base) == 3 and tl is not None,
    "mốc=%s" % (base,),
)
if tl is None:
    tl = (0, "plain", 0, 0, 0, 0)
_, _, _, net0, anh0, chu0 = tl


def gdbus_ok(ten):
    return "gdbus: ()" in (doc(ten) or "")


s = sel_stats("29-selC1")
kiem("C3 trước khi gọi plugin: đang chọn 3 nét / 0 ảnh", s == (3, 0), "vùng chọn=%s" % (s,))
kiem(
    "C4 vk640SelectionToImage (phím Ctrl+Shift+C) chạy trong APP THẬT không lỗi",
    gdbus_ok("C-sel2img"),
    dong("C-sel2img", r"gdbus:"),
)
s = sel_stats("29-selC2")
ps = page_stats("0-statC1")
kiem(
    "C5 qua PLUGIN THẬT mặc định vẫn là COPY: ảnh mới đang được chọn, nét gốc +3 KHÔNG bị ăn, ảnh +1",
    s == (0, 1) and ps[:3] == (net0 + 3, anh0 + 1, chu0),
    "vùng chọn=%s trang=%s | mốc trang %d: net=%d anh=%d chu=%d" % (s, ps, tl[0], net0, anh0, chu0),
)
ps = page_stats("0-statC2")
kiem(
    "C6 một undo sau đó: ảnh mới biến mất, nét gốc +3 VẪN CÒN (chưa từng bị đụng)",
    ps[:3] == (net0 + 3, anh0, chu0),
    "trang=%s | mốc trang %d: net=%d anh=%d chu=%d" % (ps, tl[0], net0, anh0, chu0),
)
ps = page_stats("0-sachC")
kiem("C7 dọn sạch trang làm việc trước phần C2", ps[:3] == (0, 0, 0), "trang=%s" % (ps,))

def all5(ten):
    """Như all_stats nhưng luôn trả bộ 5 số (để phép so không vỡ khi thiếu log)."""
    v = all_stats(ten)
    return v if v else (-1, -1, -1, -1, -1)


moc = all5("28-allC1")
kiem(
    "C8 fixture mới: nét giấy nháp = 4 (và cả ảnh/chữ/trang đều > 0 để phép kiểm sau có nghĩa)",
    moc[1] == 4 and moc[2] > 0 and moc[3] > 0,
    "toàn tài liệu=%s" % (moc,),
)
kiem(
    "C9 vk640EraseNotePagesInk (phím Ctrl+Shift+M) chạy trong APP THẬT không lỗi",
    gdbus_ok("C-notepages"),
    dong("C-notepages", r"gdbus:"),
)
sau = all5("28-allC2")
mong = (moc[0] - moc[1], 0, moc[2], moc[3], moc[4])
kiem(
    "C10 qua PLUGIN THẬT xoá ĐÚNG %d nét giấy nháp; ảnh %d / chữ %d / trang %d nguyên vẹn"
    % (moc[1], moc[2], moc[3], moc[4]),
    sau == mong,
    "trước=%s sau=%s mong đợi=%s" % (moc, sau, mong),
)
moi = liet_ke_trang("24-pagesC")
con_net = [(t[0], t[3]) for t in moi if t[1] == "lined" and t[2] == 0 and t[3] != 0]
so_nhap = sum(1 for t in moi if t[1] == "lined" and t[2] == 0)
giu = [(t[0], t[4], t[5]) for t in moi if t[1] == "lined" and t[2] == 0]
tr1 = [t for t in moi if t[0] == 1]
kiem(
    "C11 liệt kê TỪNG TRANG sau đó: cả %d trang nháp sạch nét, vẫn giữ ảnh/chữ; trang thường không bị đụng"
    % so_nhap,
    not con_net and so_nhap >= 2 and all(a >= 1 and c >= 1 for _, a, c in giu) and tr1 and tr1[0][3] >= 3,
    "còn nét ở trang nháp: %s | các trang nháp (trang, ảnh, chữ): %s | trang 1: %s" % (con_net, giu, tr1),
)

# ================= kết luận =================
print("-" * 78)
for mo_ta, ok, doc_duoc in kq:
    print("%s  %s\n          đọc được: %s" % ("PASS" if ok else "FAIL", mo_ta, doc_duoc))
so_fail = sum(1 for _, ok, _ in kq if not ok)
print("-" * 78)
print("TỔNG: %d/%d mục PASS%s" % (len(kq) - so_fail, len(kq), "" if so_fail == 0 else "  <-- CÓ MỤC FAIL"))
sys.exit(1 if so_fail else 0)
