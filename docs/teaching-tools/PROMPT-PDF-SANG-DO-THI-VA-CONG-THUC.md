# Prompt bàn giao: chọn công thức PDF → đồ thị và công thức trình bày đẹp

## Vai trò, phạm vi và cách làm

Bạn tiếp tục phát triển bản Xournal++ custom của Phúc trên Ubuntu, dùng bút VEIKK VK640 để học, giảng và giải Toán–Vật lý THPT. Hãy khảo sát, triển khai thật, build, kiểm thử, sửa lỗi và cài bản cá nhân; không dừng ở kế hoạch, mockup, nút rỗng hay demo chạy nhờ công cụ của riêng phiên agent.

Lượt này là **mở rộng CHẶNG 1** với hai mục tiêu:

1. Người dùng bôi/chọn vùng PDF có công thức, ứng dụng nhận dạng công thức và tạo đồ thị bằng công cụ Toán hiện có, giảm tối đa việc gõ lại.
2. Công thức trong hộp công cụ, xem trước và hình chèn được trình bày như công thức toán: phân số, căn, số mũ, ký hiệu và khoảng cách đẹp, đọc rõ khi giảng.

Hoàn thiện cả các thao tác nhỏ: trạng thái nút, chọn lại vùng, sửa nhận dạng, hủy, báo lỗi, nhớ mẫu, focus, undo/redo và lưu/mở. Tự thực hiện các việc thường lệ, có thể đảo ngược trong phạm vi. Không hỏi lại các quyết định triển khai thông thường. Chỉ hỏi khi thực sự thiếu dữ kiện hoặc cần hành động ngoài thẩm quyền như gửi tài liệu ra dịch vụ ngoài, chi phí đáng kể, thao tác phá hủy hay root ngoài helper được cho phép.

Không tự chuyển sang chặng 2A/2B/3/4. Những chặng đó chỉ giữ trạng thái lộ trình.

## 1. Đọc và kiểm tra trước khi sửa

Repo `/home/phuc/xpp-src`; build `/home/phuc/xpp-src/build`. Nhánh gần nhất `feature/protractor-geometry-tools`, nhưng phải kiểm tra nhánh/trạng thái thật. Có nhiều thay đổi chưa commit: không reset/restore, tự đổi nhánh, commit/push hoặc ghi đè thay đổi của người dùng.

Đọc đầy đủ AGENTS.md áp dụng và:

- `/home/phuc/xpp-src/docs/teaching-tools/NGHIEN-CUU-LO-TRINH-TOAN-LY.md`
- `/home/phuc/xpp-src/docs/teaching-tools/PROMPT-NANG-CAP-TOAN-LY.md`
- `/home/phuc/xpp-src/docs/teaching-tools/VK640-Toan-Ly.md`
- `/home/phuc/xpp-src/docs/teaching-tools/CHANG-1-HUONG-DAN.md`
- `/home/phuc/xpp-src/docs/teaching-tools/CHANG-1-NGHIEM-THU.md`
- `/home/phuc/xpp-src/docs/teaching-tools/CHANG-1-RA-SOAT.md`

Ghi baseline thực tế; các con số dưới đây chỉ là mốc đối chiếu, không thay kết quả chạy mới: CTest 10/10, 79 kiểm tra toán, Lua 121 PASS, tích hợp PDF/nháp 32/32. Lượt gần nhất đã chạy cả 8 nhóm kiểm thử. Chặng 1 hiện là hình vector tĩnh, chưa có metadata chỉnh lại tham số của hình cũ sau mở file.

Sao lưu đúng các tệp sẽ sửa, plugin, toolbar và binary cá nhân trước khi thay đổi. Lưu diff/hash, log build và kiểm thử theo từng lượt; không xóa bằng chứng lỗi để làm đẹp kết quả.

Chỉ cài ứng dụng với prefix `/home/phuc/.local`, binary `/home/phuc/.local/bin/xournalpp`. **Không ghi đè `/usr/bin/xournalpp`.** Kiểm tra prefix/install manifest và hash binary hệ thống trước/sau. Không đóng phiên Xournal++ của người dùng để test.

Plugin cá nhân: `/home/phuc/.config/xournalpp/plugins/VK640ToolCycle/main.lua`. Toolbar: `/home/phuc/.config/xournalpp/toolbar.ini`.

## 2. Khảo sát các điểm nối thực tế

Phần Toán hiện có nằm ở:

- `src/core/math/TeachingMath.h`, `TeachingMath.cpp`, `TeachingFigure.cpp`.
- `src/core/gui/dialog/TeachingToolsDialog.h/.cpp`.
- `src/core/plugin/luapi_application.h`, API `showMathTools`, `insertMathFigure`.
- `tests/test-teaching-math.cpp`, `tests/integration/math-probe.lua`, `math-gui-test.py`, `check-math-tools.sh`.

Phần PDF có `PdfElemSelection` với chữ được chọn, hình chữ nhật/vùng chọn và số trang nguồn; Poppler có lấy chữ và bố trí chữ theo vùng. `app.selectionAsImage()` đã hỗ trợ lấy ảnh vùng chọn, Ctrl+Shift+C giữ hành vi copy. Khảo sát cả `PdfFloatingToolbox`, các công cụ chọn chữ/chọn vùng và chuyển đổi tọa độ PDF.

**Không mặc định API lấy ảnh là API đọc không gây thay đổi:** implementation có thể chèn ảnh, hủy selection hoặc chuyển công cụ. Luồng nhận dạng cần lấy snapshot vùng PDF mà chưa thêm phần tử/undo vào tài liệu. Đừng chèn một ảnh tạm rồi xóa để làm preview.

Repo có `LatexController`, `control/latex/LatexGenerator`, `TexImage`, `TexImageView` và hộp thoại LaTeX. `TexImage` có thể giữ dữ liệu PDF và nguồn TeX. Đọc implementation, kiểm tra dependency/cách serialize/export thật rồi mới chọn cách tái sử dụng; không khẳng định mọi khả năng đã có sẵn.

## 3. Luồng sử dụng bắt buộc

Mục tiêu thao tác: **chọn công thức trong PDF → bấm “Vẽ đồ thị từ vùng chọn” → đối chiếu ảnh/công thức/đồ thị → chèn**.

- Hỗ trợ chọn chữ PDF có text layer và kéo khung bằng bút trên PDF scan/ảnh. “Bôi” được hiểu rõ là chọn chữ/vùng; không tự đổi ý nghĩa công cụ tô sáng đang dùng.
- Có đường vào gọn từ vùng chọn PDF và bảng Toán Alt+G. Ưu tiên một luồng chung, ít hộp thoại, nút đủ lớn; kiểm tra xung đột nếu thêm phím.
- Hiện vùng gốc được nhận dạng, công thức đã dựng đẹp và đồ thị xem trước cùng nhau. Người dùng xác nhận kết quả trước khi chèn; không cần gõ lại toàn bộ công thức với các ca hỗ trợ sạch.
- Nếu vùng có nhiều công thức, cho chọn một hoặc nhiều ứng viên rõ ràng. Không ghép mọi dòng thành một hàm. Giữ giới hạn số đường của engine hiện tại, hoặc mở rộng có kiểm chứng và công bố giới hạn mới.
- Có chọn/cắt lại vùng, nhận dạng lại, chọn ứng viên khác và sửa phần sai bằng bút/bàn phím/thanh ký hiệu. Giữ kết quả sửa khi thao tác hợp lý, không làm mất nội dung vì một ô chưa hợp lệ.
- Tham số m, miền x/y, thang đo, x₀ và bảng hiện có tiếp tục dùng được. Giá trị tự đề xuất phải hiện rõ và chỉnh được; không coi hệ số trong công thức là giới hạn trục.
- Thao tác nhận dạng/xem trước/hủy không đổi PDF nền, mực, ảnh, chữ hay lịch sử undo. Chỉ xác nhận chèn mới tạo phần tử, chọn sẵn cả cụm và có một undo/redo.
- Chèn đúng trang/lớp mà người dùng xác nhận, vừa vùng nhìn hiện tại. Nếu trang/lớp/zoom đổi trong lúc xử lý thì cập nhật hoặc vô hiệu hóa preview cũ một cách rõ ràng, không chèn âm thầm sang trang khác.

## 4. Lấy công thức từ PDF và ảnh

### Nguồn dữ liệu

Ưu tiên text layer và thông tin bố trí/ký tự của PDF khi đủ tin cậy. Có text layer không có nghĩa đã đọc được phân số, căn hay số mũ đúng thứ tự. Với scan, chữ lỗi mã hóa hoặc bố trí không phục hồi được, dùng crop ảnh và bộ nhận dạng công thức phù hợp.

Ảnh nguồn nên lấy trực tiếp từ vùng PDF đúng trang/rotation/crop/scale, không phụ thuộc độ phân giải screenshot hay zoom màn hình. Giữ cả hình ảnh đối chiếu. Khảo sát việc loại lớp tô sáng/chú thích khỏi ảnh nhận dạng khi cần; không sửa hay xóa chúng trên trang.

Chốt snapshot trang nguồn, vùng chọn, phép biến đổi và phiên bản yêu cầu trước khi chạy nền. Dùng trang của selection, không chỉ trang đang active khi callback trả về. Không đọc nhầm PDF/giấy nháp hoặc vùng cũ sau đổi trang.

### Backend chạy thật

- Khảo sát thư viện/công cụ đã có trên máy và repo; chọn backend có thể triển khai thật trong ứng dụng. Đọc tài liệu chính thức khi tích hợp phụ thuộc mới.
- Ưu tiên xử lý cục bộ. Không coi OCR chữ thường là đã giải quyết OCR công thức. Kiểm tra thực tế phân số nhiều tầng, căn, chỉ số và số mũ.
- Không dùng công cụ riêng của phiên Codex để làm thay bước mà app chưa làm được. Bản đã cài phải tự thực hiện luồng khi mở Xournal++ bình thường, không cần agent ngồi đọc ảnh hộ.
- Cài/khai báo runtime, model, font và phụ thuộc rõ ràng; ghi phiên bản, vị trí, giấy phép, yêu cầu CPU/RAM và cách kiểm tra hoạt động. Không phụ thuộc thư mục build/CWD tạm hay biến môi trường chỉ tồn tại trong phiên agent.
- Thiếu backend/model phải báo đúng tình trạng, có hướng khắc phục thực tế, không giả vờ nhận dạng thành công. Nếu cần dịch vụ ngoài/API tính phí, chuẩn bị lựa chọn cụ thể và xin phép trước khi gửi tài liệu hoặc phát sinh phí; tiếp tục các phần độc lập trong lúc chờ.

### Ý nghĩa toán học

Chuyển kết quả nhận dạng qua chuẩn hóa và kiểm tra ngữ nghĩa tới **AST của engine toán hiện có**. Tách biểu diễn đẹp khỏi biểu thức tính toán nhưng phải cùng một nghĩa và điều kiện xác định.

- Hỗ trợ tối thiểu các họ đang có ở chặng 1: đa thức đến bậc ba, hai họ phân thức, sin/cos, mũ, ln, sqrt, abs và m.
- Nhận tiền tố `y=`, `f(x)=`, phép nhân viết ngầm thông dụng trong công thức in nếu quy tắc không mơ hồ, dấu toán Unicode và số mũ. Không tự thay mọi chữ lạ bằng x/m.
- Phân biệt `1/(x+1)` và `1/x+1`, căn phủ toàn bộ biểu thức, −x² và (−x)², ln và chữ I/n, 1/l, 0/O, x/×, dấu trừ/gạch phân số và vị trí chỉ số.
- Giữ miền loại ban đầu dù công thức rút gọn được: `(x²−1)/(x−1)` vẫn loại x=1. Chuẩn hóa/hiển thị không được làm mất điều kiện xác định.
- Nếu cần đổi biến t thành x nội bộ, phải có quy tắc rõ và giữ nhãn biến/đơn vị hiển thị đúng. Không suy tham số, hệ số hoặc đơn vị từ hình phác tay.
- Một phương trình ẩn như `x²+y²=1`, hệ phương trình, biểu thức chưa đủ dữ kiện hoặc câu hỏi chứa nhiều công thức không mặc nhiên là y=f(x). Báo phạm vi chưa hỗ trợ hoặc cho người dùng chọn cách hiểu; không âm thầm vẽ một nhánh đoán được.
- Nhận dạng không rõ phải chỉ ra chỗ cần xem lại. Không bịa confidence/phần trăm chính xác. Kết quả parse được vẫn có thể nhận dạng sai; luôn giữ ảnh và bản công thức đối chiếu, đặc biệt với các dấu làm đổi miền/hàm.

Không cam kết OCR đúng mọi ảnh. Tiêu chí là ca sạch được nhận dạng đúng, ca mơ hồ có cách sửa nhanh và không dẫn tới chèn một đồ thị đoán sai mà người dùng không biết.

## 5. Trình bày công thức đẹp và thống nhất

- Hiển thị phân số hai tầng, căn phủ đúng phạm vi, số mũ/chỉ số, π, f′, dấu trừ, ngoặc và khoảng cách toán học. Cải thiện cả preview, nhãn công thức của đồ thị, phương trình tiếp tuyến/tiệm cận và công thức chèn riêng nếu phù hợp luồng.
- Chọn font toán phù hợp, có fallback tiếng Việt/ký hiệu; thử ở nhiều zoom và DPI. Đo baseline/bounding box bằng renderer thực để căn nhãn, không căn theo số byte hay chiều dài chuỗi ước lượng.
- Ưu tiên tái sử dụng cơ chế LaTeX/PDF vector đang có nếu phù hợp. Biên dịch/render phải bất đồng bộ, có cache và giới hạn thời gian, không làm GTK treo mỗi lần sửa một ký tự.
- Không đưa LaTeX/OCR thô vào shell hoặc cho chạy macro tùy ý. Render từ AST/markup được cho phép; tắt shell-escape, không cho nội dung công thức đọc file/chạy lệnh. Gọi subprocess bằng đối số có cấu trúc và giới hạn tài nguyên.
- Công thức chèn phải sắc khi xuất PDF, lưu/mở lại không mất font, lệch baseline hay bị cắt. Không raster hóa toàn bộ hình để né việc xử lý vector.
- Nếu dùng TexImage/đối tượng công thức riêng, lưu nguồn phù hợp để chỉnh công thức đó; kiểm tra clone/copy-paste/save/load/undo. Đừng từ đó hứa chỉnh liên kết mọi đồ thị cũ nếu chưa thiết kế và kiểm chứng metadata đồ thị.
- Tái sử dụng group undo cho nét, chữ và phần tử công thức; không để hình mới phải undo nhiều lần.

## 6. Kiến trúc và khả năng bảo trì

Tách rõ, vừa đủ cho repo: snapshot vùng PDF → trích xuất/OCR → chuẩn hóa/kiểm tra → AST/ngữ nghĩa toán chung → dựng công thức/đồ thị → xem trước → giao dịch chèn.

- Không xây parser hay bộ tính đạo hàm thứ hai trong Lua, GTK hoặc backend OCR. Không dùng eval/loadstring hay thực thi mã từ tài liệu.
- Định nghĩa dữ liệu trung gian có nguồn trang/vùng, dữ liệu thô, ứng viên, biểu thức đã kiểm tra, chỗ chưa chắc và lỗi có cấu trúc. Không bắt UI đọc chuỗi lỗi để đoán trạng thái nếu có thể dùng mã lỗi ổn định.
- Công việc nặng chạy nền; dùng cơ chế job/cancel/dispatch phù hợp repo. GTK chỉ cập nhật trên luồng UI. Kết quả cũ đến muộn sau hủy, sửa vùng, đổi tài liệu hoặc đóng hộp thoại phải bị loại; không use-after-free hay chèn trùng.
- Một trạng thái chung cho chờ vùng/đang nhận dạng/cần sửa/sẵn sàng/lỗi/hủy; các nút phản ánh đúng trạng thái. Cho chạy lại sau lỗi mà không cần khởi động app.
- Giữ API chèn, đơn vị tọa độ, kiểu phần tử, persist setting và undo nhất quán. Nếu đổi giao diện dữ liệu hoặc định dạng lưu thì có version/migration/tương thích và test tương ứng.
- Không refactor toàn repo hoặc tạo framework lớn chỉ để thêm công cụ này. Các lỗi chặng 1 đã sửa về miền xác định, bậc lẻ, 0^0, đạo hàm, độ cong, lỗ thủng, focus và đồng bộ Layout phải còn được bảo vệ bằng test.

## 7. Kiểm chứng bắt buộc

Tạo fixture tái lập được, có công thức/AST/miền/kết quả mong đợi: PDF text layer, PDF công thức vector/glyph khó trích xuất, PDF scan/ảnh; nhiều DPI, crop, trang xoay, zoom/cuộn và PDF đã chuẩn hóa kích thước. Dùng tài liệu thử tự tạo hoặc có quyền sử dụng.

Các ca tối thiểu:

1. `y=x²−3x+2`, `y=x³−3x`: nhận dạng đúng lũy thừa/dấu, đồ thị và bảng đúng.
2. `y=1/x` trên [−2,2]: không nối xuyên x=0.
3. `(x²−1)/(x−1)`: giữ lỗ (1,2); `(x²+1)/(x−1)`: đúng tiệm cận.
4. `1/(x+1)` so với `1/x+1`; `sqrt(x+1)` so với `sqrt(x)+1`: không mất phạm vi gạch phân số/căn.
5. `−x²` so với `(−x)²`, số mũ âm, π và sin/cos radian, `2^x`, `ln x`, `|x|`, m.
6. Đoạn văn có nhiều công thức: chọn đúng ứng viên; công thức ngoài khả năng không bị đổi thành hàm khác.
7. Scan mờ/cắt mất mẫu số/mũ không rõ: chỉ rõ cần sửa/nhận dạng lại; không tự chèn kết quả đoán.
8. Tiếp tuyến x² tại 1 khi hai trục khác tỉ lệ; abs tại 0; x³ tại 0 không được coi là cực trị.
9. Nhận dạng lại, sửa ứng viên, chuyển trang/tài liệu, đóng hộp thoại và hủy khi job đang chạy: không rác, crash, dữ liệu cũ hay chèn trùng.
10. Một undo/redo cho cả đồ thị/nhãn/công thức; hủy không có phần tử hoặc undo thừa; giữ tài liệu gốc.
11. Save/reopen, clone/copy-paste và PDF export của đối tượng công thức mới: đúng nguồn, font, nét, vị trí, các nhánh và bounding box.
12. Preview công thức đẹp giữ cùng ngữ nghĩa với AST và đồ thị. Đánh giá OCR bằng nghĩa toán học/miền, không chỉ so chuỗi LaTeX.
13. Thiếu backend/model/font, timeout, đầu vào quá dài và cache lỗi: thông báo được, UI vẫn dùng tiếp.

Unit test gọi implementation thật; integration nhận dạng phải chạy backend thật trên fixture. GUI test phải đi từ chọn PDF/vùng tới xem trước và chèn trong app thật. Không thay toàn bộ OCR bằng mock rồi báo đã hoàn thành. Báo rõ số ca đúng/sai/cần sửa và giới hạn, không suy kết quả vài ảnh thành độ chính xác tổng quát.

Chạy đầy đủ, không bỏ hoặc nới test cũ để PASS:

    ctest --test-dir /home/phuc/xpp-src/build --output-on-failure
    bash /home/phuc/xpp-src/tests/integration/run-tich-hop.sh
    bash /home/phuc/xpp-src/tests/check-protractor.sh
    bash /home/phuc/xpp-src/tests/check-protractor-input.sh
    texlua /home/phuc/xpp-src/tests/test-vk640-erase.lua
    bash /home/phuc/xpp-src/tests/integration/check-teaching-tools.sh
    bash /home/phuc/xpp-src/tests/integration/check-shortcut-hints.sh
    bash /home/phuc/xpp-src/tests/integration/check-math-tools.sh

Thêm test mới cho OCR, chuyển đổi công thức và renderer; có kiểm tra hiệu năng/giới hạn tài nguyên dựa trên đo thực tế. Nếu thêm app.registerUi thì cập nhật EXPECTED và rà cả accelerator/GAction. GUI dùng bus/config/display riêng, cửa sổ thật sự nằm trong màn hình test; chỉ đóng đúng tiến trình test.

## 8. Những tính năng phải bảo toàn

Ctrl+` hiện/ẩn chữ phím tắt dưới nút; Alt+` đổi 1/2 cột; Shift+Y/Ctrl+Y chia PDF và giấy nháp; Alt+G mở Toán; Ctrl+Shift+C tiếp tục copy ảnh vùng chọn. Giữ nút thân bút, tẩy, cắt ảnh, chuẩn hóa PDF, dọn nháp, hình học/bắt điểm/thước, màu/nét, sóng sin và F1. Không chiếm phím nhập công thức hay đổi hành vi cũ âm thầm.

## 9. Ghi chú cho agent sau và bàn giao

Tạo/cập nhật tài liệu trong `docs/teaching-tools/` cho kiến trúc, cách dùng, setup/backend, trạng thái và lỗi. Tên tệp có thể chọn phù hợp nhưng phải có một mục lục bàn giao dễ tìm.

Mỗi tính năng/thao tác quan trọng có: đường vào UI, điều kiện hoạt động, tác động tài liệu/undo, lỗi thường gặp và test bảo vệ. Ghi rõ vì sao ở các đoạn mã dễ sai: tọa độ PDF, lựa chọn ứng viên, miền xác định, hủy job, ownership, render, cache và giao dịch chèn; tránh bình luận lặp lại câu lệnh.

Nhật ký lỗi dùng mẫu nhất quán: ID, thành phần, mức ảnh hưởng, bước tái hiện, fixture, mong đợi/thực tế, nguyên nhân đã xác minh hay còn giả thuyết, trạng thái mở/đã sửa/chờ kiểm chứng, cách sửa, test hồi quy và đường dẫn bằng chứng. Không ghi “đã sửa” chỉ vì source đã đổi.

Ghi phiên bản/hash model, backend, font/phụ thuộc, bản build, fixture và cách chạy lại. Để agent khác hiểu những gì đã làm, chưa làm, lỗi còn mở, quyết định và giới hạn mà không phải đọc toàn bộ lịch sử chat. Phân biệt dữ liệu có thể log và crop/tài liệu riêng tư; không tự ghi log toàn bộ nội dung PDF.

Bàn giao: binary cá nhân chạy được; hướng dẫn thao tác ngắn; nguồn sửa và setup tái lập; các dependency/model đã cài; log đầy đủ; ảnh GUI; PDF đầu vào thử, .xopp và PDF xuất minh chứng; benchmark OCR và kết quả hồi quy; danh sách giới hạn/lỗi còn lại. Kiểm tra PDF bằng render trực quan.

Chỉ đánh dấu hoàn thành khi đường đi **PDF text layer và PDF scan/ảnh → nhận dạng → sửa/đối chiếu → đồ thị/công thức đẹp → lưu/mở/xuất** hoạt động và kiểm thử thật đạt. Nếu có phần bị chặn, ghi đúng phần chưa hoàn thành và bằng chứng, tiếp tục phần độc lập; không thay bằng nút chết hay yêu cầu người dùng gõ lại như tính năng chính.
