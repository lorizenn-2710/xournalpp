# Nhật ký lỗi và giới hạn — PDF → công thức

Ngày 03/11/2026. Mỗi trạng thái “đã sửa/đã kiểm chứng” dưới đây có log hoặc test chạy thật; không đồng nghĩa OCR đúng mọi ảnh. Giữ log không đạt trong `build/pdf-math-verification/`, không xóa.

## PDF-001 — Runtime Python không tương thích / thiếu requests

- **Thành phần, mức ảnh hưởng:** setup OCR, chặn backend.
- **Tái hiện/fixture:** import rapid-latex-ocr trong Python hệ thống 3.14; thử venv đầu tiên, sau đó import bản Python 3.12 chưa có requests.
- **Mong đợi/thực tế:** backend sẵn sàng / package khai báo Python <3.13 và import thiếu requests.
- **Nguyên nhân:** đã xác minh qua metadata/source và traceback; không phải lỗi PDF.
- **Trạng thái:** **đã sửa, kiểm chứng** bằng runtime CPython 3.12.14 riêng, full lock có requests/chardet tương thích.
- **Cách sửa/test:** setup helper + worker thật; chạy setup lại và native job tests.
- **Bằng chứng:** `backend-install.log`, `model-install.log`, `model-install-2.log`, `setup-final-3.log`, `setup-repeat.log`.

## PDF-002 — Một tỉ lệ ảnh sạch vẫn khiến model đọc sai

- **Thành phần, mức ảnh hưởng:** OCR, sai nghĩa nếu bỏ đối chiếu.
- **Tái hiện/fixture:** `trig/crop.png`, `parameter/crop.png`; model ở tỉ lệ 1×.
- **Mong đợi/thực tế:** sin/cos/π và m / chuỗi LaTeX lạ hoặc m thành r n. Phân thức 96 DPI có thể bị thêm x.
- **Nguyên nhân:** đã tái hiện phụ thuộc tỉ lệ trong model/resizer; chưa truy nguyên trọng số để sửa model.
- **Trạng thái:** **đã khắc phục các fixture sạch, giới hạn model còn mở** (xem PDF-010).
- **Cách sửa/test:** giữ cách đọc 0,5×/1× riêng, thêm 4× khi chữ nhỏ; không tự chấm confidence hoặc xóa cách đọc khác. Chọn/đối chiếu trước khi chèn.
- **Bằng chứng:** `ocr-scale-investigation.log`, `ocr-low-dpi-investigation.log`, `ocr-benchmark-pi-and-96dpi-failed.json`, `ocr-benchmark-final.json`. Benchmark cuối có ứng viên đúng, không phải mọi ứng viên đều đúng.

## PDF-003 — Mất ranh giới token sau \pi

- **Thành phần, mức ảnh hưởng:** converter, từ chối công thức đúng.
- **Tái hiện/fixture:** `y=\sin(x)+\cos(\pi x)` từ OCR thật.
- **Mong đợi/thực tế:** `sin(x)+cos(pi*x)` / tên `pix` bị từ chối.
- **Nguyên nhân:** đã xác minh: converter bỏ khoảng trắng sau command rồi nối `pi` với x.
- **Trạng thái:** **đã sửa, kiểm chứng**.
- **Cách sửa/test:** delimiter rõ sau π; regression trực tiếp trong `test-formula-import` và benchmark AST/miền thật.
- **Bằng chứng:** benchmark không đạt trước sửa và log benchmark cuối; `final/ctest-detail.log`.

## PDF-004 — Scan không có chữ nên không hiện công cụ vùng chọn

- **Thành phần, mức ảnh hưởng:** PDF selection, chặn đường vào scan.
- **Tái hiện/fixture:** kéo khung trên `formulas-scan.pdf`; finalize không có text rect.
- **Mong đợi/thực tế:** vùng scan và nút nhận dạng / nhánh cũ chỉ hiện toolbox khi có chữ.
- **Nguyên nhân:** đã xác minh từ implementation finalize/PageView.
- **Trạng thái:** **đã sửa, kiểm chứng**.
- **Cách sửa/test:** giữ feedback khung Area và hiện toolbox cho vùng hợp lệ dù không text; giữ copy ảnh cũ. GUI XTEST chọn scan → nhận dạng → chèn và hồi quy 32/32.
- **Bằng chứng:** ảnh `05-scan-source`, `06-scan-preview`, log GUI cuối và `final/2.log`.

## PDF-005 — Modal trực tiếp trong callback AT-SPI làm mất ứng dụng khỏi cây accessibility

- **Thành phần, mức ảnh hưởng:** nút nổi/GTK, thao tác kiểm thử và trợ năng bị chặn.
- **Tái hiện/fixture:** chọn vùng rồi kích hoạt nút nổi qua AT-SPI. Cửa sổ vẫn vẽ nhưng truy vấn widget timeout.
- **Mong đợi/thực tế:** trả lời action và mở bảng / lời gọi bị lồng trong gtk_dialog_run.
- **Nguyên nhân:** đã xác minh bằng ảnh sau click, cây accessibility và thử sửa idle.
- **Trạng thái:** **đã sửa, kiểm chứng**.
- **Cách sửa/test:** trả callback trước, mở modal qua idle; native button tiếp tục dùng chung showTools.
- **Bằng chứng:** `pdf-gui-2.log`, `pdf-gui-3.log`, `pdf-gui-4.log` và các lượt GUI đạt sau sửa.

## PDF-006 — Kết quả cũ sau hủy/đổi trang: phòng vệ ownership và generation

- **Thành phần, mức ảnh hưởng:** job/preview/transaction; rủi ro crash/chèn nhầm.
- **Tái hiện/fixture:** hủy worker, nhận dạng lại, đóng bảng, đổi trang trong bảng; snapshot giữ PDF sau khi ref document nguồn đã bỏ.
- **Mong đợi/thực tế:** chỉ kết quả hiện tại được áp dụng. Không ghi nhận crash trước sửa để tuyên bố đã tái hiện use-after-free.
- **Nguyên nhân/rủi ro đã xác minh:** selection chỉ có PDF page index không phân biệt hai trang tài liệu dùng cùng nền; Poppler wrapper trước đây không tự giữ parent ref; callback không được giữ widget pointer sống tùy tiện.
- **Trạng thái:** **phòng vệ đã triển khai và kiểm chứng đường thử**.
- **Cách sửa/test:** snapshot bất biến, PageRef nguồn/đích, parent ref + mutex, weak lifetime/generation, cancel đúng subprocess, guard trang/lớp/zoom; GTask preview coalesce.
- **Bằng chứng:** `test-pdf-formula`, `final/ctest-detail.log`, GUI cancel/retry và ảnh guard đổi trang. Không đồng nghĩa đã fuzz mọi interleaving của GTK.

## PDF-007 — Manifest setup tự đưa hash chính nó vào danh sách

- **Thành phần, mức ảnh hưởng:** setup tái lập, lần chạy lại có thể không ổn định.
- **Tái hiện/fixture:** tạo manifest bằng cách duyệt mọi file trong models rồi chạy lại setup.
- **Mong đợi/thực tế:** bốn asset cố định / thêm khóa xournalpp-sha256.json với hash thay đổi.
- **Nguyên nhân:** đã xác minh trong `setup-final-2.log`.
- **Trạng thái:** **đã sửa, kiểm chứng** hai lượt liên tiếp.
- **Cách sửa/test:** whitelist bốn asset, đối chiếu manifest ghim trong source, không hash manifest.
- **Bằng chứng:** `setup-final-3.log`, `setup-repeat.log`, `scripts/teaching-tools/models-sha256.json`.

## PDF-008 — Harness: hộp Save và focus trong Xvfb

- **Thành phần, mức ảnh hưởng:** test, không phải lỗi tính toán ứng dụng.
- **Tái hiện/fixture:** dùng widget “focused cuối cùng” của toàn desktop sau khi modal đóng.
- **Mong đợi/thực tế:** filename editable / role unknown hoặc mục focus khác; phím undo đôi lúc gửi trước khi Save đóng.
- **Nguyên nhân:** đã xác minh bằng ảnh chooser và cây AT-SPI. Xvfb không có window manager.
- **Trạng thái:** **đã sửa, kiểm chứng**.
- **Cách sửa/test:** scope AT-SPI theo PID test, chọn đúng Save File/filename, chờ save, focus cửa sổ test; undo/redo qua GAction thật và so XML. Không tương tác app khác hoặc bỏ assertion undo.
- **Bằng chứng:** `pdf-gui-4.log`…`pdf-gui-8.log`; các log chốt PASS cả undo/redo/copy-paste.

## PDF-009 — Thông báo vẫn đòi xác nhận dù checkbox/nút đã cho chèn

- **Thành phần, mức ảnh hưởng:** trạng thái UI, dễ gây bối rối.
- **Tái hiện/fixture:** scan preview, áp dụng rồi đánh dấu đã đối chiếu.
- **Mong đợi/thực tế:** thông báo sẵn sàng / nhãn cũ “Cần đối chiếu…” còn lại dù nút bật.
- **Nguyên nhân:** đã thấy trong ảnh GUI; updateInsertState chỉ ghi nhánh chưa review, không ghi nhánh review xong.
- **Trạng thái:** **đã sửa, kiểm chứng** trên bản release.
- **Cách sửa/test:** đồng bộ trạng thái Ready/Review và nhãn; thêm assertion accessible label sau xác nhận.
- **Bằng chứng:** ảnh trước sửa được giữ trong `PDF-CONG-THUC-before-review-label/`; `final/9-release-gui.log`, ảnh `gui/06-scan-preview.png` bản mới.

## PDF-010 — Mở: ảnh mờ vẫn có thể cho biểu thức hợp lệ nhưng sai

- **Thành phần, mức ảnh hưởng:** chất lượng OCR, cao nếu người dùng không đối chiếu.
- **Tái hiện/fixture:** `blurred.png`, nguồn đúng `1/(x+1)` nhưng một cách đọc thêm `x`.
- **Mong đợi/thực tế:** cần biết chắc phạm vi / model không cung cấp căn cứ chắc chắn để tự phân loại mọi ca.
- **Nguyên nhân:** giới hạn model đã tái hiện; không suy được ký hiệu đã mất từ ảnh.
- **Trạng thái:** **mở, có cổng bảo vệ**.
- **Cách dùng hiện tại:** bắt buộc ảnh/công thức/đồ thị và xác nhận, cho sửa/nhận dạng lại. Chữ chạm mép có cảnh báo; mất mẫu/mũ nhưng còn lề trắng có thể không bị phát hiện. Không tự chèn, không bịa confidence.
- **Hướng tiếp theo:** bộ dữ liệu khó có quyền sử dụng, detector/layout tốt hơn hoặc model khác; mọi model mới cần benchmark AST/miền và giữ cổng xác nhận.
- **Bằng chứng/test:** benchmark cuối ghi `requires-human-review`; parser test I/l/O/ẩn/phạm vi không rõ.

## PDF-011 — Mở: bố cục tổng quát và chữ viết tay

- **Thành phần, mức ảnh hưởng:** phạm vi, trung bình.
- **Tái hiện/fixture:** nhiều cột, đoạn văn dài, chữ viết tay, trang nghiêng hoặc lộn ngược không thuộc ca sạch đã đo.
- **Mong đợi/thực tế:** nhận mọi bài toán / hiện chỉ vùng công thức in; chia dòng theo khoảng trắng, thử chiều dọc 90/270 và các tỉ lệ.
- **Nguyên nhân:** chưa có parser bố cục tổng quát hoặc model viết tay.
- **Trạng thái:** **chưa triển khai**, không phải nút chết.
- **Cách dùng/hướng tiếp:** chọn khung hẹp bao riêng công thức; thêm detector sau, giữ tối đa hai hàm/khung trừ khi engine có test mở rộng.
- **Test/bằng chứng:** fixture multi-line/rotated, mã lỗi vùng quá nhiều dòng và converter từ chối ký hiệu lạ; chưa có test viết tay để cam kết.

## PDF-012 — Mở theo thiết kế: đồ thị tĩnh và đặt vị trí

- **Thành phần, mức ảnh hưởng:** chỉnh sửa sau chèn, trung bình.
- **Tái hiện:** sửa một TexImage hoặc kéo giãn cụm đã lưu.
- **Mong đợi/thực tế:** không có cam kết chỉnh liên kết / nét đồ thị không tự tính lại theo TexImage.
- **Nguyên nhân:** chặng 1 dùng Stroke/Text/TexImage tĩnh, chưa có metadata graph versioned.
- **Trạng thái:** **giới hạn công bố**, chưa triển khai chỉnh liên kết. Cụm mặc định ở giữa vùng nhìn có thể che nội dung có sẵn; có thể dời cụm đang được chọn.
- **Hướng tiếp:** thiết kế metadata, migration và hành vi khi người dùng sửa nét trước khi hứa liên kết. Không tự dò nội dung giấy trắng trong lượt này.
- **Test/bằng chứng:** save/reopen/copy/undo giữ nguyên hình; công thức riêng có nguồn nhưng không làm đổi nét.

## PDF-013 — Chờ kiểm chứng phần cứng/font cực hạn

- **Thành phần, mức ảnh hưởng:** nghiệm thu thiết bị, thấp với luồng đã đo.
- **Tái hiện:** VK640 vật lý, hệ thống không còn bất kỳ font khả dụng nào.
- **Mong đợi/thực tế:** chưa có phép đo phần cứng hoặc giả lập mất toàn bộ font trong phiên này.
- **Nguyên nhân:** chưa thực hiện, không kết luận lỗi.
- **Trạng thái:** **chờ kiểm chứng**. X11/AT-SPI/XTEST và hợp đồng Lua đều đạt; font thực tế đã nhúng trong PDF.
- **Hướng tiếp:** thử bút thật khi có thiết bị, nhiều DPI màn hình; giữ hợp đồng nút thân bút/tẩy và không chiếm phím nhập công thức.
- **Bằng chứng:** bộ hồi quy final, pdffonts và ảnh render; không ghi “đã thử bút thật”.

## PDF-014 — Vùng scan nhiều dòng bị nhầm thành công thức xoay

- **Thành phần, mức ảnh hưởng:** chọn hướng OCR, chặn vùng có nhiều công thức.
- **Tái hiện/fixture:** `multiple-scan.pdf`, vùng [55,84,330,260], hai dòng đa thức.
- **Mong đợi/thực tế:** hai ứng viên riêng / quy tắc chỉ dựa vào tỉ lệ cao/rộng đã xoay nhầm; test job bị deadline 15 s của harness dừng.
- **Nguyên nhân:** đã xác minh: nhiều dòng ngang cũng tạo vùng cao hơn rộng.
- **Trạng thái:** **đã sửa, kiểm chứng**.
- **Cách sửa/test:** phát hiện khoảng trắng lớn giữa dòng ở hướng gốc trước khi dùng aspect ratio để đề xuất xoay; giữ test PDF xoay 90° cũ. Native test kiểm cả hai AST trong text/scan; GUI chọn hai ứng viên, giữ sửa +1 khi đổi, xem trước hai đường.
- **Bằng chứng:** `ctest-multiline-failed.log`, `final/ctest-detail.log` (13 job checks PASS), `final/15-multiple-gui.log`, ảnh `gui/00-multiple-preview.png`.
- **Ghi chú harness:** lượt GUI đầu dùng phím điều hướng combobox trong Xvfb không ổn định focus; lượt chốt dùng giao diện Selection thật của AT-SPI. Không bỏ kiểm tra chọn ứng viên hoặc nới đáp án.

## PDF-015 — Ctrl+Shift+R kéo vùng rồi Alt+G không lấy hàm

- **Thành phần, mức ảnh hưởng:** đường vào vùng chữ nhật thông thường, chặn thao tác người dùng thực tế.
- **Tái hiện/fixture:** Ctrl+Shift+R → kéo [55,84,330,130] trên `unicode-text.pdf` hoặc vùng phân thức scan → Alt+G. `baseline-shortcuts.log` không nhận công thức; có ảnh/AT-SPI ở thư mục test ghi trong log.
- **Mong đợi/thực tế:** nhận vùng đã kéo / bản trước chỉ kiểm `PdfFloatingToolbox` khi dùng công cụ chọn PDF 21/22; Ctrl+Shift+R là công cụ đối tượng 6.
- **Nguyên nhân:** đã xác minh qua accelerator XML, trạng thái công cụ thật và test phím sản xuất; không phải người dùng cần copy ảnh trước.
- **Trạng thái:** **đã sửa, kiểm chứng** text và scan bằng phím thật, mở/hủy/mở lại cùng vùng; xem `shortcuts-1.log`, `shortcuts-2.log`, `gdb-shortcuts-2.log` và lượt chốt `final/10.log`.
- **Cách sửa:** `peekLastSelectionRect` đọc PageRef/khung gốc không consume cache; worker lấy text layer khi snapshot chưa có chữ, sau đó OCR thật. Không lấy bbox nét được chọn, không chèn ảnh tạm. Một nghĩa AST không cảnh báo được xem trước ngay, vẫn chưa xác nhận chèn.
- **Test bảo vệ:** `check-pdf-region-shortcuts.sh` tải plugin VK640 thật; text/scan, stale page, mực gốc không đổi, Ctrl+Shift+C và undo cũ còn nguyên. Log/backup ở `build/pdf-math-ux-verification/`.

## PDF-016 — GTK focus cũ chưa realized gây critical khi gõ shortcut sau đổi file

- **Thành phần, mức ảnh hưởng:** MainWindow key propagation, lỗi focus của widget cũ.
- **Tái hiện/fixture:** test mở PDF rồi gửi Ctrl+Shift+R. Lượt `shortcuts-1.log`/`shortcuts-2.log` vẫn hoàn tất nhưng `app.log` có `WIDGET_REALIZED_FOR_EVENT`.
- **Mong đợi/thực tế:** shortcut sạch / GTK chuyển phím vào widget focus chưa realized.
- **Nguyên nhân:** **đã xác minh bằng GDB** với `G_DEBUG=fatal-criticals`; stack `gtk_widget_event → gtk_window_propagate_key_event → MainWindow.cpp` trước khi Alt+G mở bảng. Không quy nhầm lỗi này cho OCR hoặc cho modal Lua.
- **Trạng thái:** **đã sửa, kiểm chứng** bằng các lượt GDB không còn trap, `gdb-shortcuts-2.log`, `gdb-shortcuts-3.log`.
- **Cách sửa/test:** bỏ focus chưa realized trước propagate; giữ IME/shortcut cũ. Mở modal Lua qua idle là cải thiện vòng callback riêng, không phải nguyên nhân đã chứng minh của critical này.

## PDF-017 — Render công thức đồng thời với xuất PDF có thể SIGSEGV

- **Thành phần, mức ảnh hưởng:** `TexImage`/Poppler, cao; mất phiên ứng dụng thử nghiệm.
- **Tái hiện/fixture:** shortcut text → nhận dạng → xác nhận/chèn → xuất Cairo PDF. `shortcuts-3.log`, `/tmp/xpp-pdf-formula.08mTdz/app.log` ghi crash khi xuất. Người dùng PID 994338 không bị đóng/chạm bởi harness.
- **Mong đợi/thực tế:** xuất xong và undo được / một lần SEGV, emergency save của app thử nghiệm; hai lượt GDB GUI tiếp theo không tái hiện, cho thấy phụ thuộc lịch chạy luồng.
- **Nguyên nhân:** **race nguồn Poppler đã xác minh bằng test thực và GDB**: `test-teximage-render` đồng thời vẽ màn hình và PDF vào các Cairo context riêng nhưng dùng cùng TexImage. Bản trước sửa crash trong `cairo_transform → Poppler Gfx → TexImageView::draw`; các luồng khác đang render cùng PDF. Shared lock Document cho phép nhiều reader nên không serialize CairoOutputDev của Poppler. Stack lần crash GUI đầu không có symbol dump; không tuyên bố có stack đó.
- **Trạng thái:** **đã sửa, kiểm chứng test race và GUI đầy đủ**: `final/9.log`, `final/10.log`, `small-screen.log`, `final/9-portable-release.log` đều PASS sau mutex.
- **Cách sửa:** mutex nguồn riêng mỗi TexImage, khóa suốt render và khi đo/thay nguồn/clone; clone load PDF độc lập. Đồng thời nhả ref page sau đo, GBytes giữ byte PDF độc lập khi preview tham chiếu vượt lifetime đối tượng. Không đổi định dạng file.
- **Test/bằng chứng:** `concurrent-before.log` SIGSEGV trước sửa; `concurrent-after.log` thoát bình thường; `concurrent-stress.json` 20/20 lượt, tổng 8.000 render bốn luồng; kiểm clone và retained-PDF lifetime. Test được thêm CTest. Kiểm GUI/export/undo bằng `final/9.log`, `final/10.log`; không xóa log crash cũ.

## PDF-018 — Hai cảnh báo OCR cùng lúc chỉ hiện một

- **Tái hiện:** vùng ảnh có chữ chạm mép và năm dòng tách biệt, tạo hơn 12 cách đọc. Worker trả đủ 12 ứng viên nhưng cảnh báo chạm mép ghi đè cảnh báo còn ứng viên bị cắt.
- **Cách sửa:** ghép cả hai cảnh báo vào một giá trị `notes` trên một dòng để giao thức INI và giao diện đọc được đầy đủ.
- **Kiểm chứng:** `tests/integration/check-formula-ocr-warnings.py` chạy worker với backend giả lập chỉ để kiểm luồng crop/chia dòng/giới hạn và hai cảnh báo; không dùng làm phép đo độ chính xác của model.

## PDF-019 — Dán ngay sau khi sao chép có thể không tạo phần tử

- **Tái hiện:** một lượt GUI PDF đã lưu được cụm TexImage sau redo nhưng file sau `select-all` → `copy` → `paste` vẫn chỉ có một TexImage. Lượt chạy lại đạt, cho thấy lỗi phụ thuộc thứ tự callback.
- **Nguyên nhân trong mã:** `ClipboardHandler::paste` yêu cầu danh sách định dạng clipboard bất đồng bộ nhưng chọn định dạng ngay bằng các cờ cập nhật từ callback trước đó.
- **Cách sửa:** chọn định dạng trong callback của chính yêu cầu hiện tại, giữ guard vòng đời và trang/lớp đích cho cả hai callback. Bài GUI gọi `copy` rồi `paste` ngay và so nguồn TeX/PDF trong file lưu.
