# Kiến trúc PDF → công thức — 03/11/2026

## Ranh giới thành phần

```text
PdfElemSelection hoặc XournalView::peekLastSelectionRect (PageRef + khung gốc)
  → PdfFormulaSnapshot (bất biến; tham chiếu PDF có ownership)
  → PdfFormulaJob / GTask
      → Cairo crop PDF nền ở tối đa 288 DPI, ≤8M pixel
      → worker Python cục bộ / ONNX (subprocess argv, không shell)
      → RecognitionResult / FormulaCandidate
  → FormulaImport (chuyển ký pháp, từ chối mơ hồ)
  → Expression (AST, đạo hàm, miền xác định, lấy mẫu cũ)
  → Figure + FormulaRender (bố trí công thức vector từ cùng AST)
  → xem trước nền / đối chiếu / xác nhận
  → InsertsUndoAction: Stroke + Text + TexImage
```

| Thành phần | Trách nhiệm / hợp đồng |
|---|---|
| `control/tools/PdfElemSelection.*` | Giữ cả `selectionDocumentPage` và chỉ số trang PDF. Scan được giữ khung dù không có text rect. Không đổi ý nghĩa marker. |
| `gui/XournalView.*` | Đọc cache khung Ctrl+Shift+R bằng peek; không consume cache ảnh Ctrl+Shift+C. Chỉ nhận đúng PageRef hiện tại, cache bị xóa khi đổi trang/tài liệu hoặc bắt đầu thao tác mới. |
| `gui/PdfFloatingToolbox.cpp` | Nút nổi gọi cùng `showTools`. Mở modal qua idle để trả lời AT-SPI trước; không lồng `gtk_dialog_run` trong callback accessibility. |
| `math/PdfFormulaJob.*` | Snapshot → crop → worker → kết quả có mã. Không giữ `Control*`, widget hoặc pointer selection trong thread nền. |
| `scripts/teaching-tools/formula-ocr.py` | Inference thật từ model cục bộ, giao thức INI version 1; raw UTF-8 base64, dòng/góc/tỉ lệ. Không parser/đạo hàm thứ hai. |
| `math/FormulaImport.*` | Whitelist ký pháp LaTeX/Unicode, tiền tố hàm và phép nhân ngầm rõ nghĩa. Cuối cùng gọi chính `Expression`; không thực thi macro. |
| `math/TeachingMath.*` | AST cũ giữ nguyên miền gốc; thêm cây trình bày `MathNotation`, ký hiệu m/π/e và TeX sinh an toàn. `literal` số dùng đại số GMP vẫn giữ giá trị số m. |
| `math/FormulaRender.*` | Hộp có baseline thật từ Pango, phân số, căn, mũ, ngoặc, trị tuyệt đối. Cairo PDF nhúng font; cache RAM có giới hạn. |
| `math/TeachingFigure.cpp` | Thêm nhãn công thức từ biểu thức cùng nghĩa, chừa vùng tiêu đề; tùy chọn công thức riêng. Không thay sampler/đạo hàm. |
| `gui/dialog/TeachingToolsDialog.cpp` | GTK/state, ứng viên đã sửa, xác nhận, đích chèn. Tính Figure/bố trí nặng qua GTask; một preview worker đang chạy, gộp yêu cầu mới nhất. |

## Tọa độ, trang nguồn và quyền sở hữu

Selection và crop dùng **đơn vị trang đã chuẩn hóa**, không lấy pixel screenshot hoặc zoom màn hình. Với chọn chữ tuyến tính, hợp các rect chữ làm vùng chặt và thêm 3 pt. Với chọn khung, giữ toàn bộ khung để không bỏ tử/mẫu chỉ vì text mapping thiếu glyph. Cắt theo kích thước PDF, render qua `XojPdfPage::render` đã bao gồm rotation/crop/scale. Scale crop tối đa 4 px/pt, cạnh ≤4096 px và diện tích ≤8M px. PDF gốc có độ phân giải scan thấp không được tạo thêm chi tiết bằng render lớn hơn.

Không gọi `selectionAsImage()` vì API đó **chèn ảnh và thay selection/undo**. Chỉ đọc PDF nền, không chèn ảnh tạm rồi xóa. Ghi chú Xournal++ không tham gia ảnh nhận dạng. Chú thích đã nằm bên trong chính PDF có thể vẫn xuất hiện; chưa có cơ chế lọc riêng chúng.

`PopplerGlibPage` giữ ref của parent PopplerDocument để job còn dùng được sau đổi/đóng tài liệu. Render và các phép đọc chữ/links dùng cùng mutex; `selectTextLines` gọi helper region đã giữ khóa để tránh tự khóa lồng. Giữ PageRef đích và pointer lớp để **so sánh**, không dereference lớp cũ sau đổi trang. Kiểm cả trang tài liệu, trang PDF và zoom. Không suy selection thuộc trang active chỉ từ chỉ số PDF (hai trang tài liệu có thể dùng cùng PDF page).

## Bất đồng bộ và loại kết quả cũ

`PdfFormulaJob` chạy crop và communicate trong GTask; Python là subprocess độc lập. `cancel()` đặt cờ và force-exit **đúng subprocess đó**. Callback cập nhật GTK trên main context. GTask giữ shared job đến khi kết thúc; job xóa riêng `crop.png` và thư mục tạm 0700 do nó tạo.

Dialog có token lifetime yếu và số generation. Đóng/hủy/nhận dạng lại tăng generation, hủy job; callback muộn kiểm token + generation trước khi đụng widget. Preview có generation riêng, snapshot Request/khung; không đọc widget từ thread nền. Tối đa một worker preview hoạt động; yêu cầu tiếp theo được gộp, kết quả cũ bị bỏ. Parser/validator trường chạy ngắn trên UI, bố trí/cảnh chạy nền, vẽ lại dùng cache layout.

Có deadline job tối đa 60 giây và cổng phục hồi UI 60 giây. Cổng UI bỏ kết quả cũ ngay cả nếu render PDF còn đang chờ mutex; không giết thread C++ tùy tiện. Python có CPU soft/hard 50/55 giây, không gian địa chỉ 4 GiB, output file 8 MiB, OpenBLAS/ORT/OpenCV được giới hạn luồng. Test deadline rút xuống 1 ms với worker thật; không chờ đủ 60 giây và không thay OCR bằng mock.

## Trạng thái và xác nhận

`SourceState`: Waiting → Running → Review/Ready hoặc Error/Cancelled. Mã backend là enum (`Ready`, `Cancelled`, `Timeout`, `MissingBackend`, `MissingModel`, `BackendError`, `InvalidRegion`). UI không suy trạng thái từ chuỗi lỗi OCR.

Trong Running: khóa chọn/sửa/áp dụng ứng viên và chèn; nút Dừng hoạt động. Kết quả mới đi vào danh sách. Nếu chỉ có một nghĩa AST hợp lệ (`Expression::latex()`, không rút gọn) và không có cảnh báo backend, tự áp dụng vào Hàm 1 để hiện xem trước, bỏ Hàm 2 cũ ở lần áp dụng đầu. Nhiều nghĩa/có cảnh báo thì yêu cầu **Dùng làm hàm 1/2** rõ ràng. Không tự ghép dòng hoặc tự đánh dấu xác nhận. Sửa raw được giữ theo ứng viên; đổi ứng viên không làm mất nội dung đã sửa. Xác nhận chỉ có tác dụng sau khi đã áp dụng một công thức. Sửa Hàm 1/2, raw hoặc nhận dạng lại bỏ xác nhận. Trang/lớp/zoom đổi khiến nút chèn vô hiệu, kể cả callback muộn có kết quả parse tốt.

## Ngữ nghĩa, render và cache

Converter không rút gọn AST nên `(x²−1)/(x−1)` vẫn giữ mẫu x−1. Từ chối `1/2x`, chữ số ngay sau biến, căn Unicode không rõ phạm vi, biến/ký hiệu lạ, phương trình ẩn và chỉ số. Không sửa I/l/O/X theo phỏng đoán. Các cách đọc khác nhau ở hai tỉ lệ vẫn hiện riêng, kể cả cách sai bị parser từ chối. Với chữ nhỏ có thêm lượt 4×; vùng cao thử góc 90/270, ảnh đối chiếu vẫn là ảnh gốc.

`MathNotation` là phép chiếu từ chính AST, không phải parser/evaluator riêng. Hộp phân số/căn/mũ lấy metrics Pango, không đếm byte để căn công thức. Kích thước con dưới 4 pt bị báo không đọc được. Cache layout tối đa 128 khóa và PDF tối đa 64 khóa, trong RAM; lỗi không được cache. Cache PDF gồm công thức, cỡ và màu. Không biên dịch TeX từ OCR; không có shell-escape/macro/file access của công thức trong luồng mới.

Nhãn thường vẫn qua fitter Pango cũ, gồm tránh chồng chữ và khung trang. Hai công thức chính cỡ 15 pt với khoảng tiêu đề lớn hơn; tiếp tuyến/tiệm cận cũng dựng từ biểu thức đã kiểm tra. TexImage lưu PDF và TeX sinh an toàn (gồm m khi có), dùng serialize/clone/copy/paste/export sẵn có; không đổi định dạng `.xopp`.

## Giao dịch chèn và tương thích

Mọi bước có thể lỗi (parse/generate/fit/PDF render/load) xảy ra trước khi sửa tài liệu. Sau đó mới clear selection, thêm toàn bộ phần tử vào lớp đích và một `InsertsUndoAction`, đổi sang chọn vùng, chọn sẵn cả cụm. Preview/hủy/nhận dạng không gọi insert.

API `insertMathFigure` giữ mặc định Stroke/Text cho caller cũ. `pretty=true` dùng công thức TexImage; `formulaOnly=true` chỉ chèn công thức đang bật. Không thêm `app.registerUi` vào plugin sản xuất; không thay EXPECTED/phím của test cũ. Setting chỉ thêm boolean `formulaOnly`, mặc định false khi thiếu; các khóa cũ được giữ. Không có migration lưu file hoặc liên kết chỉnh lại đồ thị cũ.

Agent sau cần giữ các test miền, 0^0, đạo hàm bậc lẻ/abs, độ cong, lỗ và Layout của chặng 1. Không thay các giới hạn lấy mẫu chỉ để làm ca khó PASS. Tài liệu mốc và bản backup có trước nằm trong `build/pdf-math-verification/backup/`.

## Bổ sung sau phản hồi Ctrl+Shift+R → Alt+G

`Dialog::readSourceRegion` ưu tiên selection PDF khi đang dùng đúng công cụ, rồi fallback khung gốc đã kéo bằng công cụ chọn đối tượng. Không suy khung từ bbox của nét được chọn. `peekLastSelectionRect` không thay `takeLastSelectionRect`, không thêm ảnh/undo. Worker tự trích text layer theo vùng nếu snapshot không có text, rồi vẫn OCR và đối chiếu như cũ.

`RecognitionResult::imageContent` là bbox nét in của crop PDF gốc, chỉ để UI phóng ảnh đối chiếu. PNG đầy đủ và vùng dùng OCR không đổi; checkbox cho hiện toàn bộ khung. Không lấy nét mực Xournal++ vào nguồn. `warning` tách riêng khỏi lời giải thích ứng viên để UI không tự áp dụng khi backend cảnh báo.

Giao diện có ba thẻ vùng gốc / công thức / đồ thị. Preview công thức riêng dùng cùng AST/cache renderer ở 24 pt; worker chuẩn bị layout, GTK chỉ vẽ. Preview đồ thị bỏ tiêu đề trùng ở phần trên, hình thực chèn giữ nguyên nhãn và bố cục. Các tham số cũ nằm trong nhóm có thể mở, cột trái cuộn được. Chiều cao vùng xem trước giảm trên màn hình nhỏ. CSS giới hạn trong `#teaching-tools-dialog` và gỡ provider khi đóng.

API `showMathTools` hẹn mở qua idle để trả callback Lua/accelerator trước. `MainWindow::keyPropagate` bỏ focus GTK chưa realized của trang/entry cũ khi đổi tài liệu, trước khi chuyển sự kiện phím; giữ các shortcut nhập liệu hiện có.

`TexImage` có mutex nguồn render riêng: `TexImageView::draw` giữ khóa toàn bộ render Poppler, `getNativeSize`/`loadData`/clone cùng tuân thủ. Document dùng shared lock cho các reader nên riêng shared lock tài liệu không đủ bảo vệ Poppler CairoOutputDev. Mỗi clone có PDF/khóa riêng; GBytes sở hữu byte PDF khi preview giữ ref vượt lifetime đối tượng. `getNativeSize` nhả ref page sau khi đo. Không khóa lồng getNativeSize trong draw. Cập nhật tương lai dùng `getPdf()` đồng thời phải giữ `lockRenderSource()` cho toàn bộ truy cập, không chỉ lúc lấy pointer.

Test mới `check-pdf-region-shortcuts.sh` tải plugin sản xuất thực trong config riêng và gửi phím Ctrl+Shift+R/Alt+G bằng XTEST; không gọi thẳng API mở bảng để giả lập shortcut. `test-teximage-render` chạy 400 lượt render màn hình/xuất PDF cùng đối tượng trên 4 luồng, kiểm clone/byte lifetime. Xem hướng dẫn giao diện mới và nhật ký lỗi để tìm log trước/sau.
