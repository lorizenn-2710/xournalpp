#!/usr/bin/env python3
"""Đo hình học thước đo góc (protractor) từ ảnh chụp trước/sau khi bật.

Chỉ dùng PIL + stdlib (không numpy/scipy) để chạy bằng python3 hệ thống.

Đặc tả đối chiếu — src/core/view/ProtractorView.cpp:
  - thân là NỬA đường tròn bán kính R (= độ cao thước), cạnh thẳng dài 2R ở đáy;
  - tô màu (0.94,0.94,0.99) nên hơi xanh và tối hơn nền trang;
  - vòng "cửa sổ" trong bán kính 0.42R, lỗ tâm 0.04R;
  - vạch chia mỗi `angularOffset` độ (cao >= 2cm -> 1°), vạch lớn mỗi
    `angularCaptionOffset` (cao >= 3cm -> 10°) kèm số 0,10,...,180.

Ảnh diff toàn cửa sổ lẫn toolbar/thanh trạng thái (nút vừa bấm đổi màu và nhãn
trên nút), nên KHÔNG dùng bbox ảnh diff: lấy thành phần liên thông lớn nhất làm
thân thước rồi suy tâm/bán kính từ chính nó (hàng có run dài nhất = cạnh thẳng).

In các số đo (máy đọc được) rồi tự kết luận; exit 0 nếu đạt.
"""

import math
import sys
from typing import cast

from PIL import Image, ImageChops

DOWN = 2          # giảm mẫu để BFS thuần Python chạy nhanh
INK = 200         # ngưỡng "mực" cho ảnh xám của ảnh sau


def load_mask(a_path, b_path):
    a = Image.open(a_path).convert("L")
    b = Image.open(b_path).convert("L")
    if a.size != b.size:
        raise SystemExit(f"FAIL kích thước ảnh khác nhau {a.size} vs {b.size}")
    d = ImageChops.difference(a, b)
    w, h = d.size
    small = d.resize((w // DOWN, h // DOWN), Image.Resampling.BOX)
    px = small.load()
    assert px is not None
    sw, sh = small.size
    mask = bytearray(sw * sh)
    for y in range(sh):
        row = y * sw
        for x in range(sw):
            if cast(float, px[x, y]) > 30:
                mask[row + x] = 1
    return mask, sw, sh


def largest_component(mask, sw, sh):
    """8-liên thông; trả (size, x0, y0, x1, y1) theo toạ độ ảnh đã giảm mẫu."""
    seen = bytearray(sw * sh)
    best = (0, 0, 0, 0, 0)
    for start in range(sw * sh):
        if not mask[start] or seen[start]:
            continue
        seen[start] = 1
        stack = [start]
        n = 0
        x0 = y0 = 10 ** 9
        x1 = y1 = -1
        while stack:
            i = stack.pop()
            n += 1
            y, x = divmod(i, sw)
            x0 = min(x0, x)
            x1 = max(x1, x)
            y0 = min(y0, y)
            y1 = max(y1, y)
            for dy in (-1, 0, 1):
                yy = y + dy
                if yy < 0 or yy >= sh:
                    continue
                for dx in (-1, 0, 1):
                    xx = x + dx
                    if xx < 0 or xx >= sw:
                        continue
                    j = yy * sw + xx
                    if mask[j] and not seen[j]:
                        seen[j] = 1
                        stack.append(j)
        if n > best[0]:
            best = (n, x0, y0, x1, y1)
    return best


class Shot:
    def __init__(self, path):
        self.img = Image.open(path).convert("L")
        self.w, self.h = self.img.size

    def ink(self, x, y):
        if not (0 <= x < self.w and 0 <= y < self.h):
            return False
        return cast(int, self.img.getpixel((x, y))) < INK


def main():
    ini_path, on_path = sys.argv[1], sys.argv[2]
    on = Shot(on_path)
    mask, sw, sh = load_mask(ini_path, on_path)
    n, x0, y0, x1, y1 = largest_component(mask, sw, sh)
    if n == 0:
        print("FAIL không có pixel khác biệt nào giữa hai ảnh")
        return 1
    bx0, by0, bx1, by1 = x0 * DOWN, y0 * DOWN, (x1 + 1) * DOWN, (y1 + 1) * DOWN
    print(f"thân thước (vùng diff lớn nhất): {n} pixel, bbox=({bx0},{by0})-({bx1},{by1})")

    # Cạnh thẳng = hàng có run mực liên tục dài nhất trong bbox
    best_run, base_row, base_x0, base_x1 = 0, by0, bx0, bx0
    for y in range(by0, by1):
        run = 0
        start = 0
        for x in range(bx0, bx1):
            if on.ink(x, y):
                if run == 0:
                    start = x
                run += 1
                if run > best_run:
                    best_run = run
                    base_row = y
                    base_x0 = start
                    base_x1 = x
            else:
                run = 0

    mid = (base_x0 + base_x1) / 2.0
    r_w = (base_x1 - base_x0) / 2.0

    # Đỉnh cung: hàng mực đầu tiên quanh trục giữa
    apex = -1
    for y in range(by0, base_row):
        if any(on.ink(x, y) for x in range(int(mid) - 2, int(mid) + 3)):
            apex = y
            break
    if apex < 0:
        print("FAIL không thấy đỉnh cung trên trục giữa")
        return 1
    r_h = base_row - apex
    r = (r_h + r_w) / 2.0
    print(f"cạnh thẳng: y={base_row}, x={base_x0}..{base_x1} (dài {best_run}px); "
          f"tâm=({mid:.1f},{base_row}); R_ngang={r_w:.1f} R_dọc={r_h:.1f}")

    ok = True

    def check(name, cond, detail):
        nonlocal ok
        print(f"{'PASS' if cond else 'FAIL'} {name}: {detail}")
        ok = ok and bool(cond)
        return cond

    check("thân là nửa đường tròn (bán kính ngang ≈ bán kính dọc)",
          abs(r_w - r_h) <= 0.04 * r, f"lệch {abs(r_w - r_h) / r * 100:.1f}%")

    seg = [on.ink(x, base_row) for x in range(int(mid - 0.97 * r), int(mid + 0.97 * r))]
    frac = sum(seg) / float(len(seg))
    check("cạnh thẳng liền mạch", frac >= 0.98, f"tỉ lệ mực trên cạnh={frac:.3f}")

    good = total = 0
    for k in range(4, 177, 2):
        a = math.radians(k)
        found = None
        for rr in range(int(0.96 * r), int(1.04 * r)):
            if on.ink(int(mid + rr * math.cos(a)), int(base_row - rr * math.sin(a))):
                found = rr
        total += 1
        if found is not None and abs(found - r) <= 0.02 * r:
            good += 1
    check("vành cung đều, đúng bán kính", good >= 0.9 * total,
          f"{good}/{total} góc có mép ngoài trong ±2%R")

    # Vạch chia: quét vài bán kính sát vành, lấy số run lớn nhất (vạch ngắn chỉ
    # với tới mép ngoài; ở một số bán kính lấy mẫu các vạch 1° dính nhau do làm tròn pixel)
    steps = 7200
    best_marks = 0
    for pct in (958, 965, 972, 978, 981, 984, 987, 990, 993):
        frac = pct / 1000.0
        seq = []
        for k in range(steps):
            a = math.pi * k / steps
            seq.append(on.ink(int(mid + frac * r * math.cos(a)),
                              int(base_row - frac * r * math.sin(a))))
        runs = 0
        prev = True
        for v in seq:
            if v and not prev:
                runs += 1
            prev = v
        best_marks = max(best_marks, runs)
    check("có vạch chia sát vành", 150 <= best_marks <= 215,
          f"đếm {best_marks} vạch (1°/vạch -> ~181)")

    # Số đo 0,10,...,180 ở khoảng 0.8R: vành này phải có mực (chữ), không trống
    cov = 0
    tot = 0
    for k in range(2400):
        a = math.pi * k / 2400
        cov += 1 if any(on.ink(int(mid + rr * math.cos(a)),
                               int(base_row - rr * math.sin(a)))
                        for rr in (0.72 * r, 0.76 * r, 0.80 * r, 0.84 * r)) else 0
        tot += 1
    check("có vành số đo ~0.8R", cov / tot >= 0.30,
          f"phủ {cov / tot * 100:.1f}% góc (số 0..180)")

    probe = (int(mid), int(base_row - 0.7 * r))
    bg = cast("tuple[int, int, int]", Image.open(ini_path).convert("RGB").getpixel(probe))
    fg = cast("tuple[int, int, int]", Image.open(on_path).convert("RGB").getpixel(probe))
    check("thân có tô màu", (fg[2] - fg[0]) >= 3 and sum(fg) < sum(bg) - 8,
          f"nền={bg} trong thân={fg} (xanh-đỏ={fg[2] - fg[0]})")

    print("KẾT LUẬN:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
