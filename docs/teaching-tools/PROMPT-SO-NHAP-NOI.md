# Prompt triển khai: sổ nháp nổi bằng bút, có lưu và mở lại

## Mục tiêu và phạm vi

Bạn tiếp tục phát triển Xournal++ custom của Phúc tại `/home/phuc/xpp-src`, dùng VEIKK VK640 để học/giảng Toán–Vật lý. Người dùng thiếu diện tích nháp và muốn gọi các tờ giấy nhỏ, dời đến chỗ thuận tiện, cất đi rồi viết tiếp mà không mất bài tính.

**Triển khai thật “SỔ NHÁP NỔI” trong ứng dụng**, build, kiểm thử, sửa lỗi, cài bản cá nhân. Hoàn thiện cả thao tác nhỏ, tích hợp với công cụ đã có. Không dừng ở ý tưởng, nút rỗng, cửa sổ gõ chữ, canvas bitmap tạm hoặc demo Python. Bản thử ở repo chỉ kiểm tra ý tưởng; chưa phải sản phẩm để đổi tên rồi báo xong.

Lượt này không tự chuyển sang nguyên hàm/tích phân hoặc toàn bộ chặng 2A/2B/4A. Giữ trạng thái lộ trình trung thực.

**Bổ sung trực tiếp của người dùng:** giao diện phải đẹp, hiện đại, dùng ổn; có thể chia thành rất nhiều tờ nháp, quản lý bằng các ảnh thu nhỏ nhìn được nội dung. Đây là yêu cầu bắt buộc của sản phẩm, không phải trang trí tùy chọn. Prototype cũ mới có nút tên tờ, chưa có kho ảnh thu nhỏ hoặc giao diện hoàn thiện.

## 1. Đọc trước, kiểm baseline và bảo toàn

Đọc đầy đủ AGENTS.md áp dụng và các tài liệu tại `/home/phuc/xpp-src/docs/teaching-tools/`:

- `NGHIEN-CUU-SO-NHAP-NOI.md` — quyết định tương tác, khảo sát source, phép thử và giới hạn chưa kiểm chứng.
- `NGHIEN-CUU-LO-TRINH-TOAN-LY.md`, `PROMPT-NANG-CAP-TOAN-LY.md`, `VK640-Toan-Ly.md`.
- `CHANG-1-HUONG-DAN.md`, `CHANG-1-RA-SOAT.md`.
- Toàn bộ nhóm `PDF-CONG-THUC-*.md` liên quan cách dùng, kiến trúc, giao diện mới, setup, kiểm thử và lỗi.
- `VECTO-DUONG-THANG-HOP-LUC.md`, `VECTO-KIEN-TRUC.md`, `VECTO-KIEM-THU.md`, `VECTO-NHAT-KY-LOI.md`, `VECTO-SUA-THEO-ANH.md`.

Xem prototype và log `build/floating-notes-research/`, nhưng xác minh mã nguồn/baseline hiện tại. Mốc nghiên cứu là CTest 14/14, 12 test mô hình và 21 kiểm tra GUI **prototype**; hai nhóm sau không chứng minh tính năng native đạt.

Repo có nhiều thay đổi chưa commit. Không reset/restore, ghi đè, tự commit/push hoặc làm lại các tính năng đã có. Sao lưu đúng tệp cần sửa, binary cá nhân, plugin và toolbar; lưu diff/hash/log theo lượt. Không đóng phiên Xournal++ người dùng để test.

## 2. Trải nghiệm phải dùng được ngay

### Gọi và quản lý nháp

- Có nút **Nháp nổi** và phím bật/tắt tờ/nhóm tờ gần nhất. Lần đầu tạo một tờ; bấm lại mở đúng tờ cũ, không tạo vô hạn tờ trống.
- Đề xuất **Alt+N** gọi/cất, **Ctrl+Alt+N** tạo tờ mới. Rà runtime, GAction, mnemonic menu, IME và plugin thực trước khi chốt. **Không dùng Alt+Shift+N**, plugin Export đã dùng. Nếu cần đổi ứng viên để tránh xung đột, chọn hợp lý và ghi rõ trong hướng dẫn, không hỏi lại việc thông thường.
- Thêm gợi nhớ dưới nút qua Ctrl+`, cập nhật F1 và menu; test phím thật, không chỉ gọi API thay cho shortcut.
- Có nhiều tờ, tên tự sinh/đổi tên và danh sách mở lại tờ đã cất. Cho **ít nhất hai tờ hiện đồng thời**, chọn tờ đang viết và đưa lên trước bằng chạm. Cất cả nhóm/mở lại giữ đúng danh tính và nội dung.
- Mặc định nháp thuộc tài liệu, tờ đang mở ở lại khi đổi trang/zoom/cuộn đề. Ghi nguồn trang/bài để có thể quay lại; không tự tráo nội dung theo trang đang xem. Trang nguồn bị xóa thì giữ nháp và thông báo nguồn không còn.

### Kho nháp dạng thẻ ảnh, dùng được khi có nhiều tờ

- Có **Kho nháp** dạng lưới thẻ ảnh thu nhỏ hoặc panel thích ứng, không chỉ một hàng nút tên dài vô tận. Mỗi thẻ có ảnh mực thật của tờ, tên, nguồn bài/trang nếu có và trạng thái đang mở/được chọn/chưa lưu. Tờ trống có trạng thái riêng dễ nhận.
- Luồng chính: **Tạo tờ → viết → thu về thẻ ảnh → chạm ảnh để mở và viết tiếp**. Thu gọn vẫn giữ nội dung native, zoom/scroll và lịch sử phù hợp; không biến bài nháp thành một ảnh chết. Thumbnail chỉ để xem trước, không thay thế nét, chữ, TexImage hoặc ảnh gốc trong note.
- Một tờ nháp dài/nhiều trang có ảnh đại diện phần nội dung có mực, kèm số trang/đoạn hoặc dấu cho biết còn nội dung. Không âm thầm cắt bỏ phần ngoài ảnh đại diện. Khi mở phải trở về vị trí đọc/viết gần nhất.
- Cho nhiều tờ tồn tại độc lập, giữ ít nhất hai tờ mở cạnh nhau; các tờ còn lại ở kho hoặc dải thẻ thu gọn. Không mở hàng trăm editor chỉ để hiển thị hàng trăm thumbnail. Giới hạn số cửa sổ đang mở nếu cần phải khác với số tờ có thể lưu, được giải thích rõ và không tự xóa tờ cũ.
- Có tạo mới, đổi tên, nhân bản, ghim tờ thường dùng, sắp theo thứ tự tạo/cập nhật và tìm theo tên/nguồn. Phân nhóm theo bài/trang khi có dữ liệu rõ, không cần OCR chữ viết tay để tìm. Nhân bản phải clone nội dung, có ID và lịch sử độc lập, không dùng chung bộ đệm có thể làm sửa cả hai bản.
- Chạm một lần vào thẻ mở/đưa đúng tờ lên trước; có chỉ dấu tránh tạo bản sao khi tờ đã mở. Nút menu cho đổi tên/xuất/xóa thao tác được bằng bút, không phụ thuộc hover hoặc chuột phải. Xóa hàng loạt nếu có phải chỉ rõ tờ nào và có cách phục hồi; không để động tác kéo sắp thẻ thành xóa nhầm.
- Thumbnail cập nhật sau viết/tẩy/undo/redo/dán/chèn công thức, kể cả tờ đang ẩn. Render trì hoãn/gộp công việc, cache theo note ID + revision + kích thước/theme, bỏ kết quả cũ. Khi cache lỗi/mất, tạo lại từ nội dung; không mất bài nháp.
- Lưới cuộn mượt và chỉ dựng các thẻ cần nhìn thấy. Có loading/empty/error state rõ; chọn/sắp/scroll không nhảy về đầu mỗi lần một thumbnail cập nhật. Không khóa luồng nhập bút để chụp tất cả tờ sau mỗi motion-event.

### Chất lượng giao diện bắt buộc

- Thiết kế thống nhất với bản Xournal++ custom: panel gọn, phân cấp tên tờ/nội dung/thao tác rõ, font dễ đọc, khoảng cách đều, icon vector đồng bộ, màu nhấn vừa phải; có thể dùng bo góc/bóng nhẹ khi GTK hiện tại hỗ trợ tốt. Không đổi toolkit chỉ để chạy theo vẻ ngoài.
- Ưu tiên diện tích giấy. Thanh tiêu đề gọn; thao tác thường dùng dễ chạm, thao tác ít dùng trong menu có nhãn. Vùng bấm chính khoảng 40–44 logical px khi khả thi, kiểm thực tế trên cửa sổ nhỏ và DPI cao; không làm icon nhỏ nhưng hit target cũng nhỏ.
- Thẻ ảnh có khung, khoảng cách và nhãn dễ đọc; active/selected/pinned thể hiện cả bằng dấu hoặc nhãn, không chỉ khác màu. Có tooltip, tên trợ năng, focus bàn phím và không dùng nút chỉ hiện khi hover.
- Hỗ trợ theme sáng/tối và tương phản tốt cho cả giấy, mực, chữ, viền, trạng thái vô hiệu hóa. Không để header trong suốt hoặc chữ sáng trên nền sáng như lỗi thường gặp ở demo. Thu/phóng cửa sổ không làm chữ/nút đè nhau hoặc tràn.
- Hoạt ảnh nếu dùng phải ngắn, không chặn nhập bút hoặc làm đổi vị trí vùng chạm khi đang thao tác. Tránh hiệu ứng làm đẹp gây giật, lưu chậm hoặc mất focus.
- Trước nghiệm thu, chụp và tự xem **kho nháp nhiều thẻ**, **hai tờ nổi đang viết**, **trạng thái thu gọn**, cả theme sáng/tối và cửa sổ nhỏ. Sửa lỗi bố cục qua ảnh thật; không chỉ nộp mockup đẹp hoặc ảnh thử một tờ trống.

### Vị trí, diện tích và giấy

- Nổi **trong cửa sổ ứng dụng**, không cưỡng ép always-on-top toàn desktop. Không mở modal chặn đọc/viết đề.
- Lần đầu bố trí trong vùng nhìn, ưu tiên tránh điểm đang viết/vùng chọn khi có căn cứ. Dùng vị trí bút cuối hợp lệ trong viewport; không dùng tọa độ con trỏ hệ điều hành nếu khác thiết bị. Lần sau nhớ vị trí hợp lệ, không nhảy theo từng lần chạm.
- Kéo thanh tiêu đề để dời; nút/chạm trên giấy không vô tình kéo cả tờ. Chống nhầm kéo với chạm nút.
- Resize chỉ đổi cửa sổ nhìn. Tọa độ, độ dày và hình dạng mực không bị scale để ép vừa khung. Zoom giấy là thao tác riêng, không thay zoom/tỉ lệ PDF.
- Có cuộn/Hand trong nháp; khi cần thêm chỗ, thêm trang/đoạn giấy và viết tiếp. Không yêu cầu xóa bài tính cũ để có chỗ. Nếu tự nối thêm giấy gần cuối, phải có ngưỡng/budget rõ và tránh tạo trang liên tục khi giữ bút ở mép.
- Nhớ vị trí cuộn/zoom từng tờ. Nền trắng/kẻ dòng/ô vuông dùng cùng cơ chế giấy native nếu phù hợp.
- Có mở rộng tờ đang dùng sang chế độ tập trung rồi trở lại khung nhỏ, giữ vị trí giấy. Các nút cần dễ chạm bằng bút, đọc được ở theme sáng/tối.
- Khi đổi kích thước cửa sổ, 1/2 cột, fullscreen, DPI hoặc màn hình: clamp lại khung và lưu vị trí mới; thanh tiêu đề/nút mở lại không nằm ngoài vùng chạm được. Đừng clamp theo kích thước desktop khi overlay chỉ ở viewport.

### Không làm biến mất bài tính

- **Ẩn/thu gọn không xóa mực, không thay tài liệu nền, không mất undo.** Không tự ẩn do rời con trỏ, mất focus, nhấc bút, đổi công cụ hoặc timer.
- “Tờ mới” giữ các tờ cũ. “Xóa nội dung”, “Xóa tờ” là lệnh riêng, phạm vi rõ và hoàn tác được; không đặt nút xóa sát nút cất dễ bấm nhầm.
- Nếu dùng dấu X ở tiêu đề, thể hiện rõ đó là cất tờ; lệnh xóa thật nằm riêng.
- Tài liệu chưa đặt tên, nháp đang ẩn hoặc cửa sổ đang thu nhỏ vẫn được tính là có nội dung cần lưu. Đóng app hỏi lưu đúng, không báo tài liệu sạch khi chỉ nháp thay đổi.
- Tìm hiểu phản ánh “nháp biến mất” qua ca tái hiện; phân biệt bị ẩn, chuyển trang, xóa mực bằng phím và mất khi mở lại. Không tự ghi đã sửa mất dữ liệu nếu chưa có bằng chứng.

## 3. Viết như giấy thường, tích hợp các công cụ

- Mực native vector có áp lực nếu thiết bị cung cấp; bút, tẩy, highlighter, màu, độ dày, nét, đoạn/vectơ và vùng chọn/copy/paste hoạt động đúng phạm vi.
- Nút thân bút Pen → Eraser → Hand → Pen giữ hợp đồng hiện có trong nháp và trên trang chính. Hand cuộn/di chuyển giấy, không vô tình kéo cửa sổ hoặc nền PDF.
- Chạm ngoài note trở về trang chính; vùng bị note che không nhận nét xuyên xuống. Khoảng ngoài note vẫn tương tác được, không đặt một overlay toàn màn hình bắt mọi event.
- Có active editing target rõ cho pen/eraser, selection, clipboard, undo/redo và các lệnh áp cho trang. Ctrl+Z khi viết nháp phải hoàn tác nháp, không xóa mực trang đề; đổi focus/ẩn/mở không được làm lịch sử trỏ tới đối tượng đã hủy. Ghi rõ chính sách lịch sử và triển khai nhất quán.
- Stroke bắt đầu trong note rồi vượt mép, nhả bút ngoài khung, rời proximity, hide/resize trong stroke hoặc mất grab phải kết thúc/hủy theo chính sách rõ, không kẹt pen-down, nối dài sai hoặc vẽ xuống trang nền.
- Công thức đẹp/TexImage, công cụ đồ thị, vectơ/đường thẳng/hợp lực phải chèn được vào tờ nháp đang chọn theo quy tắc đích rõ; không âm thầm chèn vào trang PDF. Dùng cùng renderer/engine và vector assets.
- Nguồn PDF OCR và đích viết là hai khái niệm riêng. Nếu nhận dạng từ PDF rồi đưa kết quả vào nháp, phải giữ snapshot nguồn đúng; note bị đóng/xóa/đổi trong lúc job chạy thì không chèn kết quả muộn nhầm nơi.
- Có **Chép vùng chọn sang trang chính** hoặc luồng copy/paste tương đương dễ dùng: giữ bản gốc, chèn vừa vùng nhìn đúng trang/lớp, chọn cả cụm, một undo ở đích. Giữ nét/chữ/ảnh/công thức vector; không raster hóa cả bài nháp chỉ để tránh xử lý clipboard.
- Dán/cắt ảnh PDF vào nháp dùng chức năng hiện có với đích rõ, không phá Ctrl+Shift+C và luồng lấy vùng PDF mới.

## 4. Mô hình dữ liệu và lưu trữ bắt buộc

Khảo sát `Document`, `PageRef`, `XojPage`, `Layer`, `XournalView`, `XojPageView`, `InputContext`, `StrokeHandler`, `EraseHandler`, selection/clipboard, `UndoRedoHandler`, `SaveHandler`, `LoadHandler`, `XmlParser`, autosave/emergency và PDF export trước khi chốt cách lưu.

- Nội dung là dữ liệu tài liệu, không phải widget hoặc Cairo surface duy nhất. Tách nội dung, ownership/ID và view (rect/zoom/scroll/z-order/hidden).
- Ưu tiên mô hình native tái sử dụng render/input/undo. Không tạo bộ bút Python/JS/Cairo đơn giản thứ hai thiếu áp lực, tẩy và serialization rồi coi là hoàn thiện.
- Note ID và nguồn trang ổn định khi thêm/xóa/sắp xếp trang; không chỉ ghi page index, PDF page number hoặc basename file. Hai tài liệu cùng tên ở hai thư mục phải tách nhau.
- Lưu nội dung nháp và metadata cần thiết cùng **.xopp**, hoặc dữ liệu nhúng trong file được reader/writer hỗ trợ thực. Chép file sang thư mục khác/mở trên profile sạch vẫn có nháp; không phụ thuộc JSON bên cạnh bị quên, đường tuyệt đối trong máy, /tmp, cache hoặc session agent.
- Nếu dùng trang phụ trong Document, phân loại bằng role/ID, không bằng giấy lined hoặc vị trí lẻ/chẵn. Không chèn xen vào cặp PDF–nháp cũ. Rà layout/sidebar/page numbering, Ctrl+U, Ctrl+Shift+M, xóa trang, API page/all/notePages và export. Lệnh dọn nháp cũ không tự quét nháp nổi.
- Rà normalization của `LoadHandler::finalizeDocument()` với trang cả hai chiều >900 pt. Không để save/reopen co nội dung mới; giữ tính năng chuẩn hóa PDF cũ bằng test chống hồi quy.
- Nếu thêm tag/attachment/version, kiểm parser/save thực, fallback bản cũ, versioning/migration. Ghi rõ ứng dụng cũ có thể giữ hay làm mất dữ liệu khi lưu lại; không âm thầm coi unknown tag là được bảo toàn.
- Save, Save As, mở lại, autosave, emergency recovery, clone, clipboard, undo và close phải tính cả note ẩn. Recovery cho tài liệu chưa đặt tên có ID riêng. Ghi an toàn bằng temp/replace và kiểm lỗi; không đánh dấu đã lưu khi thất bại. Không xóa recovery chưa có bản ghi thành công thay thế.
- Hai cửa sổ/tài liệu không được dùng lẫn notebook hay cùng recovery target. Ghi rõ xử lý cùng file mở hai nơi và Save As, không ngầm dùng “lần lưu cuối thắng” làm đảm bảo dữ liệu.
- Không ghép nhiều editor bằng cách liên tục thay currentPage của trang chính. Có bộ định tuyến đích chỉnh sửa dùng chung, lifetime/generation an toàn; không giữ widget/PageRef mất hiệu lực trong callback.

Chốt quyết định trong tài liệu kiến trúc sau khảo sát rồi tự tiến hành. Không cần hỏi người dùng duyệt từng lựa chọn kỹ thuật thường lệ.

## 5. Xuất/in và hiệu năng

- Xuất PDF/in tài liệu chính mặc định giữ trang chính, không chụp cửa sổ nháp nổi đè lên đề và không âm thầm thêm các trang ẩn vào số trang xuất.
- Có **Xuất nháp** riêng, chọn tờ hoặc toàn sổ, chia trang hợp lý, giữ vector và font nhúng. Nội dung ngoài phần đang cuộn phải được xuất đủ; khung/nút UI không được xuất.
- Không cấp phát một bitmap cực lớn cho “giấy vô hạn”. Ưu tiên trang/đoạn native và render phần cần thiết, giới hạn/budget được công bố. Không xóa tờ cũ để giảm RAM.
- Đo độ trễ viết, thời gian mở/lưu, RAM với nhiều tờ/nhiều nét; tránh I/O đồng bộ mỗi motion-event và tránh lưu snapshot cũ đè nét mới. Dirty indicator và debounce/flush phải có test.
- Có fixture tối thiểu **100 tờ có nội dung** để kiểm kho ảnh, tìm/sắp, mở lại, chuyển tờ, lưu và phục hồi; ghi rõ số nét/điểm/ảnh/công thức trong bộ đo. So với baseline ít tờ và báo số đo thực tế, không hứa số tờ vô hạn. Cache thumbnail không được khiến RAM tăng mãi khi cuộn đi/cuộn lại hoặc che nét mới bằng ảnh cũ.

## 6. Ca kiểm chứng bắt buộc trên sản phẩm thật

Viết test gọi implementation native và GUI thật, dùng fixture tự tạo, config/bus/display riêng. Không mock toàn bộ bút, lưu file hoặc clipboard. Ít nhất:

1. Phím thật gọi/cất; bấm nhanh/giữ phím không tạo hàng loạt tờ; đúng khi canvas, entry, nhập công thức hoặc menu có focus.
2. Viết, nhấc bút/rời con trỏ/đổi tool/click trang nền/đổi trang PDF/zoom/layout: note không tự mất hoặc bị xóa.
3. Hai note cùng hiện, có nội dung khác nhau; overlap/z-order; viết, tẩy, chọn và undo đúng tờ; nền không có nét xuyên.
4. Kéo tiêu đề không sinh mực; viết trên giấy không kéo khung; resize và đổi DPI không đổi tọa độ/áp lực/độ dày mực.
5. Cuộn/Hand trong nháp không cuộn PDF; chuyển tờ phục hồi zoom/scroll riêng; viết tiếp sau cuộn đúng hệ tọa độ.
6. Thêm đủ giấy qua nhiều đoạn/trang; quay lại phần đầu giữ nguyên; không giới hạn bằng cách xóa bài cũ. Save/reopen và xuất đủ phần ngoài viewport.
7. Stroke vượt biên, nhả bên ngoài, mất focus/proximity, ẩn/resize/xóa note giữa thao tác: không crash, kẹt công cụ, nối nét giả hoặc ghi nhầm trang.
8. Tờ mới giữ tờ cũ; ẩn khác xóa; xóa nội dung/xóa tờ rồi undo/redo đúng, giữ ảnh/chữ/công thức theo chính sách đã ghi.
9. Ctrl+U và Ctrl+Shift+M giữ hợp đồng cũ; không xóa nháp nổi khi lệnh nhắm giấy nháp cũ. Kiểm thiếu/có selection và note đang active.
10. Copy/cut/paste nội bộ và nháp → trang chính: giữ native assets, chọn cụm và undo một bước đúng đích, không di chuyển/xóa bản gốc khi yêu cầu copy.
11. Chèn đồ thị, công thức và vectơ vào note; OCR từ vùng PDF rồi chèn note; đổi/xóa đích trong lúc job chạy không chèn muộn hoặc crash.
12. Ctrl+S khi chỉ note thay đổi, note ẩn hoặc tài liệu chưa đặt tên; đóng có cảnh báo đúng. Save/reopen so dữ liệu từng note, không chỉ số lượng.
13. Save As, di chuyển/copy .xopp sang thư mục/profile sạch, hai tài liệu trùng basename, sắp xếp/xóa nguồn trang; ID, notebook và anchor không lẫn nhau.
14. Autosave/recovery: mô phỏng dừng process test sau nét đã commit/flush; ghi rõ khoảng có thể chưa flush. Khôi phục không thay recovery tốt bằng file lỗi. Lỗi quyền ghi/đích không hợp lệ/ghi dở phải báo thật và giữ dữ liệu trong RAM.
15. Xuất/in chính không có note đè; xuất sổ riêng đủ nội dung, đúng page count, không raster hóa mực/công thức. Render PDF và xem trực quan.
16. Theme sáng/tối, cửa sổ nhỏ, 1/2 cột, fullscreen, nhiều DPI; tiêu đề, nút cất/mở lại, cuộn và resize đều chạm được.
17. Bộ dữ liệu nhiều tờ/nhiều nét: có số đo máy thử, ngân sách và kết quả; không kết luận mượt chỉ từ một đường thẳng trong demo.
18. Kho ít nhất 100 tờ có nội dung: thumbnail đúng ID, chọn/mở/ghim/tìm/sắp đúng tờ; cập nhật ảnh sau tẩy/undo/redo/copy/công thức và sau mở lại. Đổi thứ tự khi job render còn chạy không tráo ảnh giữa các tờ.
19. Thu tờ đang viết về thẻ ảnh rồi mở lại: giữ toàn bộ dữ liệu và vị trí giấy; hai tờ cùng tên vẫn độc lập; nhân bản rồi sửa một bản không đổi bản kia. Xóa/undo tờ không bị thumbnail job hồi sinh hoặc ghi nhầm vào tờ khác.
20. Kho ở theme sáng/tối, màn hình nhỏ/DPI cao: không cắt tên hoặc che nút quan trọng, active/focus rõ, dùng bút được không cần hover. Ảnh minh chứng phải có dữ liệu thật, nhiều thẻ và nhiều tờ nổi.

Kiểm thử VK640 vật lý nếu có khả năng truy cập; nếu không, ghi rõ chỉ kiểm X11/GUI giả lập và hợp đồng plugin, không báo đã đo áp lực/palm rejection thật. Wayland cũng cần ghi đúng phạm vi đã thử.

## 7. Hồi quy và cài đặt

Đọc script/cách gọi và chạy toàn bộ bộ hiện có, không bỏ/nới assertion:

```sh
ctest --test-dir /home/phuc/xpp-src/build --output-on-failure
bash /home/phuc/xpp-src/tests/integration/run-tich-hop.sh
bash /home/phuc/xpp-src/tests/check-protractor.sh
bash /home/phuc/xpp-src/tests/check-protractor-input.sh
texlua /home/phuc/xpp-src/tests/test-vk640-erase.lua
bash /home/phuc/xpp-src/tests/integration/check-teaching-tools.sh
bash /home/phuc/xpp-src/tests/integration/check-shortcut-hints.sh
bash /home/phuc/xpp-src/tests/integration/check-math-tools.sh
bash /home/phuc/xpp-src/tests/integration/check-pdf-formula.sh
bash /home/phuc/xpp-src/tests/integration/check-pdf-region-shortcuts.sh
bash /home/phuc/xpp-src/tests/integration/check-coordinate-tools.sh
```

Thêm suite nháp nổi riêng; đăng ký test/fixture tài liệu rõ. Thêm app.registerUi phải cập nhật EXPECTED trong Lua và rà xung đột GAction/phím. Giữ log thất bại và cách sửa, không chỉ log PASS cuối.

Giữ Ctrl+`, Alt+`, Shift+Y/Ctrl+Y, Ctrl+Shift+C, Ctrl+Shift+R, Alt+G, F1, nút thân bút, tẩy/cắt ảnh/chuẩn hóa PDF, thước/bắt điểm/hình học, đồ thị/bảng, OCR/công thức đẹp và vectơ/đường thẳng/hợp lực. Kiểm bản plugin/toolbar thực, không dựa vào số action cũ viết tay.

Chỉ cài prefix `/home/phuc/.local`, binary `/home/phuc/.local/bin/xournalpp`. Tuyệt đối không ghi đè `/usr/bin/xournalpp`; kiểm hash trước/sau và build/install khớp. Không đóng tài liệu hay PID app người dùng để kiểm thử; chỉ dừng đúng process test.

## 8. Ghi chú cho các agent sau và tiêu chí bàn giao

Tạo `SO-NHAP-NOI-HUONG-DAN.md`, `SO-NHAP-NOI-KIEN-TRUC.md`, `SO-NHAP-NOI-KIEM-THU.md`, `SO-NHAP-NOI-NHAT-KY-LOI.md` hoặc tên tương đương có liên kết từ README.

- Hướng dẫn: đường vào/phím, lifecycle note, phạm vi xóa/undo, lưu/khôi phục, chuyển kết quả sang trang và xuất riêng; có hình minh họa.
- Kiến trúc: model/view/active target, ID/anchor/schema, ownership/lifetime, input routing/pressure/transform, undo, save/recovery/migration, export và điểm nối các công cụ. Ghi hợp đồng cần giữ để thêm chức năng sau không làm lệch hành vi.
- Nhật ký lỗi: ID, thành phần, mức ảnh hưởng, bước tái hiện/fixture, mong đợi/thực tế, nguyên nhân đã xác minh hay giả thuyết, trạng thái, cách sửa, test chống tái phát và đường dẫn bằng chứng. Không đánh dấu fixed chỉ vì chưa tái hiện lại.
- Kiểm thử: lệnh thật, mã trả về, số kiểm tra, phạm vi thiết bị/display và log; ảnh GUI, .xopp native có nhiều note, bản reopen và PDF xuất chính/xuất nháp riêng. Fixture người khác có thể chạy lại.
- Cập nhật lộ trình đúng tính năng đã nghiệm thu; giữ báo cáo nghiên cứu/prototype là lịch sử, không biến kết quả prototype thành số test native.

Làm đến khi luồng thực dùng được và hồi quy đạt. Nếu bị chặn, tiếp tục các phần độc lập, báo bằng chứng và phần còn thiếu. Không tuyên bố “hoàn hảo/không còn lỗi” hoặc toàn bộ chức năng đã xong khi còn thiếu lưu/mở, bút thật, routing hoặc test bắt buộc. Tự thực hiện các việc thường lệ, có thể đảo ngược trong phạm vi đã giao; chỉ hỏi khi thật sự ngoài thẩm quyền.
