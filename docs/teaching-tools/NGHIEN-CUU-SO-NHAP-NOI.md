# Nghiên cứu sổ nháp nổi — 03/11/2026

**Trạng thái: đã khảo sát mã nguồn, dựng và thử prototype GTK3 riêng; chưa tích hợp tính năng vào Xournal++, chưa cài bản ứng dụng mới.** Tài liệu này là đầu vào cho [prompt triển khai](PROMPT-SO-NHAP-NOI.md), không phải biên bản nghiệm thu sản phẩm.

**Bổ sung yêu cầu sau nghiên cứu:** người dùng muốn giao diện đẹp, hiện đại, ổn định và nhiều tờ được quản lý như các ảnh nhỏ. Prompt đã bổ sung kho nháp dạng lưới thumbnail, thu tờ về thẻ/mở lại, ghim/tìm/sắp/nhân bản, render cache theo ID/revision và fixture 100 tờ. Phần này **chưa có trong prototype và chưa được kiểm thử**; số 12 test mô hình/21 kiểm tra GUI bên dưới không bao gồm kho thumbnail hay nghiệm thu giao diện sản phẩm.

## Nhu cầu và quyết định tương tác

Người dùng thiếu diện tích viết khi vừa đọc đề PDF vừa tính bằng bút, muốn gọi các tờ nháp nhỏ, dời đến chỗ thuận tiện, cất đi rồi viết tiếp. Câu “tính xong thì biến mất” là mô tả trải nghiệm; chưa có ca tái hiện chứng minh ứng dụng tự xóa mực. Không được ghi thành một lỗi mất dữ liệu đã tìm ra nguyên nhân.

Đề xuất: **sổ nháp nổi bên trong cửa sổ Xournal++**, dùng giấy và nét vector thật. Có nhiều tờ độc lập; bản sản phẩm cần mở được ít nhất hai tờ đồng thời, có danh sách để tìm lại tờ đã ẩn. Tờ đang dùng mặc định ở lại khi đổi trang đề. Mỗi tờ có nguồn trang/bài để quay lại, không tự đổi nội dung theo số trang đang xem.

- Một phím gọi/cất nháp gần nhất, một nút cùng tác dụng cho bút. Lần đầu chưa có tờ thì tạo một tờ; các lần sau không tạo thêm âm thầm.
- Kéo **thanh tiêu đề** để dời. Viết trên giấy không kéo cả tờ; Hand kéo giấy bên trong; cuộn không cuộn nhầm PDF nền.
- Khung nhỏ là cửa sổ nhìn vào nội dung: cuộn và thêm trang/đoạn giấy để viết tiếp; đổi kích thước khung không đổi tọa độ mực. Bản sản phẩm phải có zoom giấy riêng nếu cần, không ràng vào zoom PDF.
- “Ẩn/thu gọn” không xóa dữ liệu. “Tờ mới” giữ tờ cũ. Xóa nội dung/xóa tờ là lệnh riêng có hoàn tác và phạm vi rõ.
- Khi thiếu chỗ trên màn hình, thu khung hoặc chuyển chế độ rộng/tập trung có đường quay lại; không vứt tiêu đề/nút mở lại ra ngoài màn hình.
- Dán kết quả đã chọn vào trang chính là **copy vector một cụm**, giữ bản nháp và có một undo ở đích. Xuất riêng nháp thành PDF là lệnh riêng; cửa sổ nổi không tự in đè lên PDF đề.

| Phương án | Đánh giá |
|---|---|
| Chỉ thêm trang giấy thường bên cạnh PDF | Dùng lại chức năng cũ nhưng vẫn chia diện tích màn hình; không đáp ứng gọi/cất ở vị trí tùy ý. Giữ nguyên làm lựa chọn hiện có. |
| Cửa sổ ghi chú của hệ điều hành/app thứ hai | Có thể nổi nhưng không dùng chung bút/tẩy, vùng chọn, undo, công thức và dữ liệu tài liệu; không chọn làm luồng chính. |
| Vẽ tạm lên overlay rồi xóa/ẩn buffer | Dễ làm demo nhưng thiếu tài liệu, save/reopen và phục hồi; không chấp nhận cho sản phẩm. |
| Sổ giấy vector thật + các view nổi trong app | Phù hợp nhất; cần làm đúng mô hình dữ liệu, bộ định tuyến đầu vào và lưu file, không chỉ thêm widget. |

## Những gì đã xác minh trong repo

1. Plugin thật `~/.config/xournalpp/plugins/VK640ToolCycle/main.lua`, `ensureNotePages()` khoảng dòng 282–376: Shift+Y/Ctrl+Y tạo trang thường sau PDF và bật hai cột. Không có lệnh tự xóa mực trong callback này. Có lệnh xóa mực riêng Ctrl+Shift+M, xóa mực trang Ctrl+U và lệnh xóa hẳn trang ở menu.
2. `src/core/gui/FloatingToolbox.cpp`: overlay hiện có dùng pass-through, và `handleLeaveFloatingToolbox()` gọi `hide()` khi rời bảng. Chỉ tái sử dụng kiến thức bố trí/clamp; không dùng nguyên lifecycle này cho giấy viết tồn tại lâu.
3. `XournalView.cpp` tạo `InputContext` và widget Xournal; `PageView.cpp` dựng StrokeHandler/EraseHandler/selection gắn với Control, PageRef, zoom và undo. Chưa có bằng chứng chỉ thêm một GtkDrawingArea là có đủ hành vi bút của ứng dụng.
4. `src/core/plugin/luapi_application.h`, nhánh `notePages`: nhận diện trang không PDF có giấy lined. Tờ nháp nổi có giấy lined nếu dùng chung danh sách trang rất dễ lọt vào lệnh xóa nháp cũ. Cần ID/role rõ, không dựa vào màu giấy hoặc chỉ số lẻ/chẵn.
5. `LoadHandler::finalizeDocument()`: trang không PDF có **cả hai chiều > 900 pt** có thể bị chuẩn hóa và scale các phần tử. Không phải mọi trang dài đều bị co; một giấy rộng 600 pt và dài 3600 pt không thỏa điều kiện này. Cần test đúng ngưỡng và loại giấy mới trước khi dùng trang rất lớn.
6. `SaveHandler::prepareSave()` hiện duyệt các trang trong Document; `AutosaveJob` gọi cùng SaveHandler. Dữ liệu chỉ sống trong controller/widget ngoài Document sẽ không tự được lưu. Lưu recovery không thay cho lưu chính thức.
7. `XmlParser` hiện cảnh báo/bỏ qua tag không nhận diện; `XmlTags.h` chưa có loại sổ nháp. Không được thêm một node JSON rồi cho rằng mọi bản đọc/lưu lại đều giữ node đó. Repo đã có cả xử lý gzip và ZIP/attachment; phải khảo sát thực tế trước khi chọn cách đóng gói.
8. Các phần Toán, PDF OCR, công thức vector, vectơ/đường thẳng/hợp lực đã có. Không được biến việc thêm nháp thành quay lui các nâng cấp này.

## Phím tắt: đề xuất, chưa đăng ký vào app

Ưu tiên thử **Alt+N** cho gọi/cất nháp, **Ctrl+Alt+N** cho tờ mới. Đã rà tĩnh các bảng accelerator XML, ActionProperties và plugin VK640; chưa thấy đăng ký đúng hai tổ hợp này. Đây không phải chứng minh không xung đột runtime/mnemonic/IME hoặc mọi plugin đang bật.

**Alt+Shift+N đã có trong plugin Export** cho xuất PNG, không dùng tổ hợp này. Agent triển khai cần chuẩn hóa thứ tự modifier, kiểm đăng ký runtime, các menu mnemonic và focus khi nhập chữ/công thức. Không dùng chữ thường không modifier hoặc nút thân bút làm phím gọi nháp mới. Nút trên toolbar phải hiện gợi nhớ qua Ctrl+` như cơ chế hiện có.

Trong prototype đã gửi **Alt+N thật** để ẩn/mở lại tờ cũ. Chưa thử Alt+N trong ứng dụng Xournal++ sản phẩm.

## Thiết kế dữ liệu khuyến nghị

Các điều kiện cần chốt trước khi sửa UI:

- Sổ thuộc tài liệu, mỗi note và trang nguồn có ID ổn định; không lấy tên file/basename hoặc chỉ số trang làm toàn bộ định danh.
- Nội dung dùng mô hình phần tử native như PageRef/Layer/Stroke/Text/Image/TexImage; vị trí khung trên màn hình là trạng thái view tách biệt.
- Ưu tiên lưu nội dung và danh mục nháp trong chính .xopp, hoặc payload nhúng có version được app đọc/ghi đầy đủ. Không chỉ lưu JSON/mực trong /tmp, cache hoặc settings.
- Hai hướng cần khảo sát: collection trang phụ native được serialize riêng; hoặc trang native có role/ID và view/export filter. Hướng thứ hai cần rà mọi đường dùng chỉ số trang, xuất/in, sidebar, cặp PDF–nháp và API all/notePages. Không tự chọn nhanh chỉ vì thêm trang dễ hơn.
- Nếu dùng extension/payload mới, có chiến lược tương thích và cảnh báo rõ khi xuất định dạng cũ có thể mất nháp. Save As, autosave, emergency save, clone, undo, clipboard phải có hợp đồng cụ thể.
- Trước khi có lần Save đầu, có recovery ID cho tài liệu chưa đặt tên. Ctrl+S và cảnh báo đóng tài liệu phải tính cả nháp ẩn. Lỗi ghi file không được báo “đã lưu”.
- Nhiều tờ/canvas là **nhiều view**, không đồng nghĩa tạo nhiều MainWindow/Control tùy tiện hoặc tạm đổi currentPage rồi đổi lại để giả lập.
- Xác định active editing target cho input, undo, clipboard và chèn công thức/hình. Giữ nguồn PDF OCR riêng với đích chèn, kiểm generation/lifetime nếu job hoàn thành sau khi đổi/ẩn/xóa note.

## Thử nghiệm đã thực hiện

### Baseline ứng dụng hiện có

`ctest --test-dir build --output-on-failure`: **14/14 PASS**, 9,42 giây. Bản log tại `build/floating-notes-research/baseline-ctest.log` (sao từ LastTest.log của lượt vừa chạy). Không chạy lại toàn bộ suite GUI sản phẩm trong lượt nghiên cứu vì chưa sửa mã sản phẩm. Không được gọi đây là nghiệm thu tính năng nháp nổi.

### Prototype độc lập

Nguồn: `scripts/teaching-tools/research-floating-notes/`.

- `model.py`: mô hình trạng thái tối giản, mực dạng điểm và file JSON thử nghiệm.
- `test_model.py`: **12/12 test PASS**, gồm 1000 bố trí ngẫu nhiên với seed cố định, owner khác nhau, hide/show, nhiều tờ, undo, thêm giấy không scale, roundtrip và tệp tạm dở dang không ghi đè bản đã commit.
- `prototype.py`: GTK3/Cairo; nền đề **giả lập**, không đọc PDF thật; hai tờ trong danh sách nhưng chỉ hiện một tờ tại một thời điểm. Không có pressure/eraser/native clipboard/save .xopp.
- `gui_probe.py`: một kết nối Cua MCP, fresh snapshot trước và sau các action. Chạy Xvfb, bus và window manager riêng. Cua là đường duy nhất gửi input; Python Xlib trong harness chỉ đọc cờ sẵn sàng của WM.
- **21 kiểm tra GUI PASS**, gồm hai kiểm tra đóng cửa sổ thử ở hai lượt process. Đã thử: kéo thực tạo nét; không ghi xuống nền; nút ẩn/mở; Alt+N thật hai lần; đổi trang đề vẫn giữ note; tờ mới và quay lại tờ cũ; undo/redo riêng; cuộn/nhớ vị trí cuộn từng tờ; viết tiếp sau cuộn bằng tọa độ giấy; thêm giấy giữ mực; kéo tiêu đề; thu nhỏ cửa sổ; tải lại state; đóng và mở process mới giữ chính xác dữ liệu.

Log: `build/floating-notes-research/gui-probe-5.log`, `gui-results.json`, `cua-transcript.jsonl`, `model-tests.log`. Ảnh: `output/floating-notes-research-2026-11-03/gui-*.png`. Bản cuối mở lại: `gui-47-restarted.png`. JSON chỉ là bằng chứng prototype; **không dùng làm định dạng sản phẩm**.

Lệnh test mô hình:

```sh
/usr/bin/python3 scripts/teaching-tools/research-floating-notes/test_model.py
```

Lệnh GUI đã chạy từ repo (cần bộ WM thử đã giải nén trong build):

```sh
xvfb-run -a -s '-screen 0 1440x1000x24' dbus-run-session -- \
  env -u WAYLAND_DISPLAY GDK_BACKEND=x11 FLOATING_NOTES_ISOLATED=1 \
  /usr/bin/python3 scripts/teaching-tools/research-floating-notes/gui_probe.py
```

Harness đọc state ở đường cố định; để chạy lại từ đầu, chuyển state thử trước đó sang tên backup riêng, không xóa file người dùng. Gói openbox/libobrender32/libobt2/libimlib2t64 lấy bằng apt-get download rồi dpkg-deb -x **trong build**, không cài root hoặc đổi WM desktop. Version/hash trong manifest nghiên cứu. Không chạy harness trên DISPLAY :0/:1; nó có kiểm từ chối. Không dùng foreground trên desktop thật: lần background ở desktop không tạo nét, không được tính đạt; foreground retry chỉ diễn ra trong Xvfb riêng sau refusal.

## Lỗi thử nghiệm và rủi ro còn mở

| ID | Bằng chứng, xử lý, trạng thái |
|---|---|
| NR-001 | Prototype ban đầu import Gdk không ghim 3.0 nên nạp nhầm Gdk 4.0 và không mở. Đã ghim version, log lỗi giữ tại prototype-launch-failed.log. Đây là lỗi prototype, không phải lỗi Xournal++. |
| NR-002 | GUI lượt 3: khung đã clamp trên màn hình sau resize nhưng file state còn vị trí cũ. Đã lưu trạng thái sau layout thay đổi qua idle; lượt 4 và 5 kiểm lại đạt. Giữ gui-probe-3.log và state-failed-3.json. |
| NR-003 | Background kéo trên display thật không tạo nét; Xvfb không có WM thì Cua không xác nhận foreground, từ chối gửi input. Đã dùng WM riêng cho môi trường thử, không sửa cấu hình bảo mật/input của người dùng. Đây là giới hạn hạ tầng kiểm thử. |
| R-001 | Chưa tích hợp engine bút, áp lực, tẩy, palm rejection, clipboard, shape, TexImage, undo và save/reopen native. Bắt buộc nghiệm thu ở sản phẩm, không suy từ prototype. |
| R-002 | Chưa có nhiều note hiện đồng thời, drag vượt mép khi đang viết, resize trong stroke, Wayland/DPI thực và thử VK640 vật lý. Chưa kết luận đạt. |
| R-003 | Chưa có .xopp thật chứa nháp, autosave/emergency recovery, Save As/di chuyển file, PDF export/in hoặc lỗi ổ đầy. JSON roundtrip không chứng minh các đường này. |
| R-004 | notePages/normalization/unknown tags/active target là rủi ro xác minh từ kiến trúc, chưa phải ca mất dữ liệu của chức năng mới vì chức năng chưa tồn tại. |

## Nguồn đối chiếu

GTK cho phép bố trí con overlay và tính vị trí tùy chỉnh; input pass-through cần đặt theo vùng tương tác. Đây là cơ sở dùng overlay cho **view**, không phải cơ chế lưu mực: [GtkOverlay](https://docs.gtk.org/gtk3/class.Overlay.html), [set_overlay_pass_through](https://docs.gtk.org/gtk3/method.Overlay.set_overlay_pass_through.html).

Xournal++ phân biệt file tài liệu đã lưu và autosave, với vị trí recovery cho tài liệu chưa đặt tên: [File Locations](https://xournalpp.github.io/guide/file-locations/). Hành vi bản custom phải căn cứ thêm SaveHandler/AutosaveJob trong repo, không chỉ dựa vào hướng dẫn upstream.
