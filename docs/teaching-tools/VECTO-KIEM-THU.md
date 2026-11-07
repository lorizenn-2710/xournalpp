# Kiểm chứng đợt Vectơ – Đường thẳng – Hợp lực

Ngày 03/11/2026. Bản cài cá nhân `/home/phuc/.local/bin/xournalpp`. Phạm vi riêng M08/P01, không nghiệm thu toàn bộ 2B/4A.

## Baseline và bảo toàn

`build/vector-verification/` giữ baseline, backup, các log chưa đạt, build/install và kết quả chốt. `baseline.diff`, `baseline-status.txt`, `branch.txt` ghi nhánh `feature/protractor-geometry-tools` cùng thay đổi có trước. Không reset/restore, đổi nhánh, commit hoặc push.

Baseline CTest **13/13** và đủ 9 suite ngoài CTest đều mã 0, gồm Math GUI, PDF formula và phím chọn vùng PDF. Xem `baseline/results.json`. Các tệp sửa được sao lưu riêng trong `backup/`; binary cá nhân cũ cũng được giữ. Không thay plugin VK640/toolbar cá nhân, không đóng PID 2594684 đang dùng của người dùng.

Prefix CMake được kiểm là `/home/phuc/.local`. `handover-manifest.json` so hash binary build/cài và đối chiếu bản hệ thống, plugin, toolbar trước/sau; `/usr/bin/xournalpp` không đổi. `task-only.diff` so từng tệp với **baseline làm việc**, không nhầm thay đổi cũ thành phần của đợt này.

## Bộ tính và renderer thật

`tests/test-coordinate-math.cpp` liên kết xoj::core của ứng dụng, **56 kiểm tra, 0 lỗi**; log `coordinate-oracle-final.log` và `final/ctest-expanded.log` (**14/14 CTest**).

Đã kiểm:

- AB=(3;−2;2), chuẩn √17; u+v=(4;1;2), u·v=1; hiệu/nhân/đối.
- Vectơ 0 có chuẩn 0 và góc rỗng; cùng phương/vuông góc, biên dung sai góc.
- Hai lực 10 N vuông góc có chuẩn 10√2, phương vị 45°; E=(−10;−10;0). Hai lực đối nhau cả dạng thành phần lẫn góc 30°/210°: R=0, không hướng. Nhiều lực và lực thẳng đứng.
- Đường qua (1;2;3) chỉ phương (0;1;0) có tọa độ x,z cố định; tham số y=2+t; chính tắc không có phân số mẫu 0.
- Cắt/song song/trùng/chéo; (t;0;0) và (0;s;1) chéo nhau, khoảng cách 1; gần song song còn phân giải được có giao ở x≈−1e8.
- Chiếu (1;2;3) lên Ox cho (1;0;0), chuẩn khoảng cách √13; kiểm điểm thuộc/không thuộc đường.
- Dữ liệu đến 1e−100/1e100; chuẩn an toàn, số không hữu hạn/ngoài giới hạn, biến lạ, sai số thành phần, điểm trùng, u=0, underflow tích có lỗi.
- Dời hình học đến x=1e10 không làm residual vuông góc 1 bị coi là thuộc đường.
- Đổi tỉ lệ hai trục/góc nhìn: tọa độ trang đổi nhưng nguồn công thức toán giữ nguyên.
- Oxy có cả phương trình tham số và tổng quát. Tất cả mode/phép toán tạo nét hữu hạn trong khung; công thức thật là PDF vector.
- Nam châm trang và chọn điểm khai báo trong preview gọi cùng nearestAnchor implementation.

## GTK thật và tài liệu thật

Lệnh:

```sh
PDF_FORMULA_ARTIFACTS=/home/phuc/xpp-src/output/pdf/VECTO-DUONG-THANG-HOP-LUC/gui-7 \
  bash tests/integration/check-coordinate-tools.sh
```

Harness dùng app đã cài, config/bus/Xvfb riêng 1600×1100; AT-SPI thao tác entry/combo/nút, XTEST bấm lên preview. Chỉ đóng PID test. Không mock phép tính, UI, lưu file hay xuất PDF.

**30 kiểm tra GUI đạt**: nhập sai khóa chèn, sửa tham số/tỉ lệ/góc nhìn, preview/hủy giữ tài liệu và selection, chọn O thật đặt B=(0;0), mẫu AB/tổng/2 lực/nhiều lực/chéo/chân chiếu, chèn và chọn sẵn Stroke/Text, nhớ dữ kiện, một undo bỏ hết cả cụm, một redo khôi phục XML, clipboard gấp đôi cả Stroke lẫn TexImage, guard đổi trang khóa chèn, save/reopen so XML toàn bộ trang/lớp/phần tử/công thức, xuất Cairo PDF.

**Kiểm trang/lớp bổ sung:** 5 kiểm tra GUI thật đạt ở `build/vector-verification/gui-layer.log`, minh chứng `output/pdf/VECTO-DUONG-THANG-HOP-LUC/layer-scope/`: cụm được chèn vào trang 2/lớp 2, trang 1 và lớp 1 trống; đổi lớp khi modal mở khóa chèn; hủy không thêm hình. Tổng cộng **30 + 5 kiểm tra GUI mới**.

`gui-7/gui-results.json`, `app.log` và `build/vector-verification/gui-7.log` là bằng chứng lượt đạt. Không thấy Gtk-CRITICAL, exception plugin hoặc crash trong log app lượt chốt. Các cảnh báo thiếu nút plugin sản xuất là do config kiểm thử chỉ bật probe riêng; không sửa toolbar người dùng. Các lỗi harness trước đó được ghi riêng trong [nhật ký](VECTO-NHAT-KY-LOI.md), không xóa log không đạt.

## File bàn giao và kiểm tra hình

Thư mục `output/pdf/VECTO-DUONG-THANG-HOP-LUC/gui-7/`:

- [evidence.xopp](../../output/pdf/VECTO-DUONG-THANG-HOP-LUC/gui-7/evidence.xopp), [reopened.xopp](../../output/pdf/VECTO-DUONG-THANG-HOP-LUC/gui-7/reopened.xopp): 10 trang, 190 Stroke, 136 Text, 96 TexImage công thức nhúng.
- [evidence.pdf](../../output/pdf/VECTO-DUONG-THANG-HOP-LUC/gui-7/evidence.pdf): xuất từ tài liệu đã mở lại; trang 1 trắng baseline, trang 2 AB, 3 tổng u/v, 4 hai lực vuông góc, 5 lực đối nhau, 6 hai đường chéo, 7 chân chiếu Ox, 8 nhiều lực, 9 nhập lực bằng góc, 10 lực đối 30°/210° và bản clipboard.
- inserted/undo/redo.xopp, before-copy/after-copy.xopp: kiểm giao dịch và clipboard.
- 01-ab-preview, 02-point-pick-before, 03-point-picked, 04-zero-direction-system, 05-opposite-polar, 06-target-guard, 07-reopened và example-*.png: ảnh GUI thật.
- force-export.png, skew-export.png: render PDF bằng Poppler; đã xem trực quan cùng preview hệ phương trình có thành phần chỉ phương 0.

`pdfimages -list` không có ảnh raster; `pdffonts` báo font công thức/nhãn đều nhúng. Các mũi tên trên tên vectơ, căn, phân số và dấu hệ được dựng vector; ảnh render dùng để kiểm tra, không thay nội dung vector trong file. Log chi tiết ở pdf-info/pdf-fonts/pdf-images.log.

## Hồi quy hiện có

**Đã chạy đủ 10/10 lệnh, tất cả mã trả về 0.** CTest 14/14; PDF/nháp 32/32; thước và đầu vào thước PASS; Lua 121 PASS; teaching tools 18/18; shortcut hints PASS; Math GUI 21 kiểm tra nền cùng save/reopen/stress 30 trang PASS; PDF formula/OCR và shortcut vùng PDF PASS. Mười lệnh nguyên vẹn ở `build/vector-verification/final/results.json`, log `1.log`…`10.log`: CTest, PDF/nháp, thước, đầu vào thước, Lua VK640, teaching tools, shortcut hints, math tools, PDF formula và PDF region shortcuts. Kết quả cuối đã được chốt trong manifest; không bỏ/nới assertion cũ để PASS.

Không có kiểm thử phần cứng VK640, mọi DPI/Wayland, metadata chỉnh liên kết hoặc OCR vectơ. Hình sau chèn vẫn tĩnh. Số thực/dung sai và giới hạn bố trí được công bố trong [hướng dẫn](VECTO-DUONG-THANG-HOP-LUC.md) và [kiến trúc](VECTO-KIEN-TRUC.md).

## Rà soát theo ảnh người dùng

[Bản sửa hiển thị theo ảnh](VECTO-SUA-THEO-ANH.md) bổ sung 5 ca bắt lỗi đã tái hiện trước sửa, đưa oracle tọa độ lên 61 kiểm tra; GUI theme tối kiểm 10 thao tác/điều kiện mới. Baseline, log và manifest riêng ở `build/vector-polish-verification/`; không ghi đè bằng chứng đợt triển khai đầu.
