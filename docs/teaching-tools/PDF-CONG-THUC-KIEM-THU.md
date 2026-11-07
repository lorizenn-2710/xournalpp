# Kiểm chứng PDF → công thức và đồ thị — 03/11/2026

Phạm vi nghiệm thu: bản cá nhân hoạt động độc lập, PDF text layer và scan → nhận dạng thật → đối chiếu/sửa → đồ thị/công thức vector → save/reopen/export. Không nghiệm thu OCR mọi ảnh hoặc viết tay. Mục lục/cách dùng: [README](PDF-CONG-THUC-README.md).

**Cập nhật cuối:** bản sửa shortcut/giao diện đạt CTest 13/13 và các GUI ở 1600×1100, 1280×720. Xem [phần bổ sung](#bổ-sung-sau-phản-hồi-ctrlshiftr--altg) và `build/pdf-math-ux-verification/handover-manifest.json`. Các mục nghiệm thu ban đầu dưới đây được giữ làm lịch sử.

## Baseline và bảo toàn

Nhánh `feature/protractor-geometry-tools` được giữ nguyên. Không reset/restore, đổi nhánh, commit hoặc push. Backup source thực sự sửa, diff/hash trước lượt, binary cá nhân, plugin và toolbar: `build/pdf-math-verification/backup/`. Baseline chạy đủ: CTest 10/10 và cả 7 lệnh hồi quy ngoài CTest, đều mã 0; log `baseline-ctest.log` và `baseline/1.log`…`7.log`.

Bản cài dùng prefix `/home/phuc/.local`. Hash `/usr/bin/xournalpp` trước/sau vẫn là `a8568bd21b65051287f6928d14a74153b5d6ef9918b3c29754c224d45c333b37`. Binary build và cá nhân phải khớp; giá trị chốt trong `build/pdf-math-verification/handover-manifest.json`. Plugin VK640 và toolbar cá nhân không đổi. Không đóng phiên người dùng để test.

## Hồi quy

Tám lệnh bắt buộc được chạy nguyên vẹn, không bỏ/nới test cũ. Log và mã trả về ở `build/pdf-math-verification/final/1.log`…`8.log`, `results.json`.

| Nhóm | Kết quả đã kiểm chứng |
|---|---|
| CTest | 12/12; giữ 79 kiểm tra Toán cũ; thêm 162 kiểm tra notation/render và 13 kiểm tra job PDF/OCR thật |
| PDF/ảnh/nháp | 32/32 |
| Thước | PASS |
| Đầu vào thước | PASS |
| Lua VK640 | 121 PASS, 0 FAIL |
| Teaching tools | PASS, giữ hợp đồng màu/sóng sin/undo |
| Shortcut hints | PASS, gồm các phím hiện có và Alt+G |
| Math tools | PASS, gồm tham số/hai hàm/bảng/focus/save/reopen và stress thêm trang |

Chi tiết stdout của test C++ nằm trong `final/ctest-detail.log`. Những lượt trước bản chốt và các lỗi thử nghiệm vẫn được giữ ở thư mục kiểm thử, không xóa để làm đẹp kết quả.

## OCR thật và ngữ nghĩa

Fixture tự tạo bằng TeX và Pango: `output/pdf/pdf-formula-fixtures/`, generator `tests/integration/make-formula-fixtures.py`. Có PDF chữ Unicode, PDF glyph TeX, scan không text layer, phân số nhiều tầng, căn, mũ âm, π/radian, m, phương trình ẩn, nhiều DPI, ảnh mờ/cắt, trang xoay 90° và trang 10× được chuẩn hóa. Không dùng tài liệu riêng tư của người dùng.

Benchmark `tests/integration/benchmark-formula-ocr.py` gọi **worker đã cài và model thật**, rồi `test-formula-import --equivalent` so cây AST của implementation ứng dụng, giá trị và các điểm loại. Không chỉ so chuỗi LaTeX. Đây là đối chiếu những fixture hữu hạn có đáp án đã biết, không phải chứng minh hai biểu thức tổng quát tương đương bằng thử vài điểm.

Kết quả: **20/20 ca có đáp án có ít nhất một ứng viên đúng** (18 công thức ở 288 DPI, thêm phân thức ở 96/144 DPI). **18/20** đúng ngay ứng viên đầu; hai ca phải chọn cách đọc khác. Phương trình ẩn được từ chối; hai ca mờ/cắt được xếp **cần người đối chiếu**, không tính vào số ca đúng. Ảnh mờ có một cách đọc parse được nhưng thêm x sai: minh chứng vì sao parse tốt vẫn phải xác nhận ảnh. Ca cắt có cảnh báo chữ chạm mép, không thể vẽ tự động.

Trong lượt đo đã ghi: trung vị khoảng **0,770 s**, khoảng **0,674–1,644 s** cho 23 ảnh, gồm khởi tạo process/model (filesystem đã có cache). Profile nested riêng: **0,74 s**, RSS tối đa **367.060 KiB**. Đây là số đo trên máy này, không suy thành độ chính xác/tốc độ chung. JSON đầy đủ: `ocr-benchmark-final.json`; log chốt tại `final/10-ocr-benchmark.log`. Model/runtime hash không đổi.

Các test job thật kiểm: nguồn PDF text, scan có lỗ, hai công thức riêng trong PDF text và scan, chuẩn hóa 10×, xoay 90°, vùng quá nhỏ, cancel trước backend, cancel subprocess đang chạy, deadline 1 ms, vùng trống, thiếu model không tải mạng và retry. Crop tạm phải biến mất sau job. Deadline mặc định ứng dụng là 60 s; test rút ngân sách để kiểm nhánh timeout, không giả vờ đã ngồi chờ đúng 60 s trên GUI.

## GUI thật

`tests/integration/check-pdf-formula.sh` dùng Xvfb/config/bus riêng, XTEST kéo vùng thật, AT-SPI thao tác widget thật; app là `/home/phuc/.local/bin/xournalpp`. Luồng nhận dạng không mock, không lấy kết quả từ agent. Đóng đúng PID test.

Đã kiểm:

- Chọn chữ tuyến tính, chọn khung PDF text và scan; nút nổi và cùng bảng Toán.
- Chọn lại vùng, nhận dạng lại, sửa raw sai, áp dụng và bỏ/đánh lại xác nhận; không chèn trước xác nhận.
- Cancel preview không thêm phần tử; một undo/redo gồm Stroke/Text/TexImage.
- Copy/paste native giữ nguồn TeX và dữ liệu PDF nhúng; clone ở unit test.
- Dừng worker thật rồi retry; callback cũ không thay trạng thái mới.
- Lưu/mở lại: số phần tử và dữ liệu/source TexImage giữ nguyên.
- Xuất Cairo PDF, thêm công thức riêng không thêm nét đồ thị, đổi trang vô hiệu hóa chèn.
- Nhãn trạng thái xác nhận được kiểm lại sau rà soát trực quan; nút và thông báo phải đồng bộ.

Log GUI release trong `final/9-release-gui.log`; kiểm tra nhiều ứng viên/sửa giữ lại/hai hàm trong `final/15-multiple-gui.log`; các lượt đầy đủ trước đó ở `regression-before-review-label/9-pdf-gui.log` và `11-portable-gui.log`. Cấu hình test không có plugin sản xuất nên có cảnh báo toolbar item Plugin::… không tìm thấy; test không sửa plugin thật. Cảnh báo DBus/portal khi tắt phiên riêng không phải lỗi nhận dạng.

## File và kiểm tra trực quan

Bộ bàn giao `output/pdf/PDF-CONG-THUC-minh-chung/`:

- `text-evidence.xopp`, `text-undo.xopp`, `text-redo.xopp`, `copy-paste.xopp`: chữ PDF, chèn/sửa, atomic undo, clipboard.
- `evidence.xopp` và `evidence.pdf`: scan 19 trang, đồ thị phân thức có lỗ ở **trang 4**.
- `reopened.xopp`: bản mở lại đã so sánh.
- `formula-only.xopp`, `formula-only.pdf`: công thức riêng ở **trang 1**, giữ đồ thị trang 4.
- `text-graph-vector.pdf`: xuất đồ thị chữ PDF không nền để nhìn rõ hình không che chữ in có sẵn.
- `gui/`: ảnh chọn vùng, crop/công thức/đồ thị cùng nhau, chèn, mở lại và guard đổi trang.
- Ảnh render PDF `scan-export-page4.png`, `formula-only-export-page1.png`, `text-graph-vector.png`.

Các file .xopp có tham chiếu PDF tương đối do **app tự lưu**. Giữ thư mục `pdf-formula-fixtures/` cạnh `PDF-CONG-THUC-minh-chung/`; gói ZIP bàn giao giữ cấu trúc này. Không sửa XML để tạo bằng chứng chèn. Bản .xopp cuối được app thật mở lại trước khi xuất, gồm font nhúng.

PDF được render bằng Poppler và xem trực quan: phân số, mũ, vị trí, lỗ (1,2), đường cong/nhãn không bị cắt. `pdffonts` xác nhận Latin Modern Math và các font nhãn đều **embedded/subset**. Scan nền là raster có chủ đích; công thức mới và đồ thị vẫn là vector. Hình mặc định ở giữa vùng nhìn, không dò khoảng giấy trắng để tránh nội dung có sẵn; người dùng có thể dời cụm đã chọn. Vì vậy bản text minh chứng có cả xuất không nền riêng.

## Giới hạn chưa nghiệm thu

Không có đo VK640 phần cứng vật lý, OCR chữ viết tay hoặc chống mọi tình huống mất glyph nhưng còn lề trắng. Không giả lập thiếu toàn bộ font hệ thống. Chưa kiểm migration/chỉnh liên kết đồ thị cũ vì không thêm metadata đồ thị. Xem nhật ký lỗi mở và setup để tiếp tục đúng phạm vi.

## Bổ sung sau phản hồi Ctrl+Shift+R → Alt+G

Log lượt này: `build/pdf-math-ux-verification/`, backup riêng trước khi sửa. Baseline CTest 12/12, nhưng test mới `baseline-shortcuts.log` đã tái hiện đường phím người dùng không lấy vùng; vì vậy bộ cũ PASS không chứng minh workflow phím này hoạt động.

Sau sửa, tám lệnh bắt buộc đều mã 0 (`final/results.json`, `1.log`…`8.log`), cùng GUI PDF cũ (`9.log`) và shortcut sản xuất mới (`10.log`). CTest **13/13**, giữ 79 kiểm tra Toán cũ, 162 notation/render, job tăng từ 13 lên **15** kiểm tra; thêm test render đồng thời. Nhóm PDF/nháp vẫn **32/32**, Lua **121 PASS, 0 FAIL**. Sau chỉnh chiều cao giao diện nhỏ và định dạng source, chạy lại Math GUI trên binary chốt ở `final/8-release.log`; PDF portable save/reopen/export trên binary chốt ở `final/9-portable-release.log`.

`check-pdf-region-shortcuts.sh` nạp plugin VK640 thật, gửi phím thật Ctrl+Shift+R (kiểm tool enum 6), kéo khung rồi Alt+G. Không gọi trực tiếp API mở bảng để thay cho shortcut. Đã đạt text/scan, tự áp dụng một nghĩa AST, checkbox mặc định tắt/chèn khóa, hủy rồi mở lại cùng vùng, từ chối vùng trang cũ, có nét mực thật trong vùng nhưng chỉ đọc PDF nền, giữ Ctrl+Shift+C và undo ảnh/nét cũ, xác nhận/chèn/xuất và một undo cả cụm. Các assertion cũ không bị bỏ/nới để PASS.

Lặp cùng toàn bộ test shortcut ở **1280×720** (`small-screen.log`) đạt; mỗi lần mở bảng kiểm geometry nằm trọn màn hình. Bằng chứng đầy đủ 1600×1100 và 1280×720 ở `output/pdf/PDF-CONG-THUC-giao-dien-moi/`. Chưa suy ra mọi DPI/theme/Wayland hoặc bút vật lý đều đã thử.

Race render công thức được tái hiện trước sửa bằng GDB (`concurrent-before.log`, SIGSEGV trong Cairo/Poppler). Test dùng bốn luồng, context màn hình/PDF riêng nhưng chung TexImage; sau sửa đạt 20/20 lượt, **8.000 render**, cùng clone và retained-PDF lifetime. Log crash GUI không có stack được giữ, hai lượt GDB GUI trước mutex có thể PASS vì race không xảy ra ở mọi lịch chạy. Không kết luận chỉ từ một lượt GUI thành công. Nhật ký PDF-017 ghi rõ phân biệt đó.

Bản chốt dùng cùng runtime/model cục bộ trước đây. Không chạy lại benchmark OCR toàn bộ vì không đổi model/tiền xử lý/converter/sampler; job OCR và GUI thật được chạy lại. Cải thiện hình nguồn chỉ là phóng bbox để đối chiếu, không đổi ảnh đưa vào OCR. Nhiều cách hiểu/cảnh báo vẫn cần áp dụng rõ; mọi OCR đều cần người xác nhận.

PDF xuất mới được render và xem ảnh, font công thức/nhãn được kiểm bằng pdffonts. Giới hạn đặt hình tĩnh ở giữa vùng nhìn vẫn còn: hình có thể chồng nội dung PDF in sẵn, cần dời cụm hoặc dùng giấy nháp; không tuyên bố tự dò khoảng trắng. Các .xopp portable được app thật lưu thẳng vào thư mục bàn giao để đường dẫn nền tương đối đúng, không sửa XML giả bằng chứng.

Manifest chốt `build/pdf-math-ux-verification/handover-manifest.json` đối chiếu build/binary cài, hệ thống, plugin và toolbar; source diff theo baseline riêng của lượt được giữ. Không reset/restore/đổi nhánh/commit/push, không đóng phiên người dùng. Hash `/usr/bin/xournalpp`, plugin và toolbar giữ nguyên. Xem [cách dùng mới](PDF-CONG-THUC-GIAO-DIEN-MOI.md).
