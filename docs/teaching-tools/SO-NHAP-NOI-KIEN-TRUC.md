# Sổ nháp nổi native — kiến trúc

Ngày 03/11/2026. Bản custom, prefix `/home/phuc/.local`. [Kiểm thử](SO-NHAP-NOI-KIEM-THU.md) phân biệt kết quả native với nghiên cứu/prototype trước đây.

## Quyết định lưu trữ

`Document::floatingNotes` sở hữu các `PageRef` phụ. Chúng **không nằm trong `Document::pages`**. Trang chính, sidebar, số trang, plugin `all`/`notePages`, ghép PDF–nháp cũ, in và xuất chính tiếp tục dùng collection chính. Không nhận diện nháp nổi bằng giấy lined, chỉ số hoặc basename.

Mỗi `XojPage` có `stableId` UUID. Constructor/clone tạo ID mới; reader khôi phục ID đã lưu. `FloatingNoteState` chứa tên, sourceId của trang nguồn, rect, zoom, scroll, open/pinned/deleted, created/modified/revision/zOrder. Nguồn đã xóa không xóa nháp; lệnh về nguồn tra UUID trong các trang chính. Note thuộc đối tượng Document của cửa sổ, không thuộc một file JSON hay widget. Hai tài liệu trùng basename có các bộ trang và UUID riêng.

Nội dung dùng nguyên `Layer`/`Stroke` có pressure/`Text`/`Image`/`TexImage`. Clone gọi clone native từng phần tử, có PDF công thức độc lập theo hợp đồng mutex TexImage hiện có. Không raster hóa nội dung để lưu hoặc chuyển sang trang chính.

## Schema `.xopp`

Writer ghi các trang chính trước, rồi các trang phụ bằng **serializer page/layer/element native**. Trang phụ có `scratch-version="1"`, `stable-id`, `scratch-name`, `scratch-source`, `scratch-x/y/width/height/zoom/scroll-x/scroll-y/open/pinned/deleted/created/modified/z`. Reader đọc metadata trước background/layer, tách trang phụ khỏi trang chính khi finalize. Phiên bản scratch không hỗ trợ bị từ chối, không đọc một phần rồi âm thầm ghi đè.

`LoadHandler::finalizeDocument()` bỏ chuẩn hóa kích thước cho trang phụ, vẫn giữ chuẩn hóa PDF/trang thường cũ. Oracle dùng giấy **1200×1680 pt**, cả hai chiều vượt 900, để bắt lỗi này; test riêng trang thường vẫn được chuẩn hóa.

Bản Xournal++ cũ không hiểu role: các tờ xuất hiện thành trang thường. Nét/chữ/ảnh/công thức vẫn là dữ liệu page native, nhưng **lưu lại bằng bản cũ sẽ mất metadata nổi**; không cam kết bảo toàn role/ẩn/anchor trong bản cũ. Hướng dẫn/F1 công bố giới hạn này. Giữ bản sao `.xopp` trước khi dùng bản cũ. Export PDF không phải cách lưu sổ để tiếp tục chỉnh sửa.

## View, input và đích chỉnh sửa

`FloatingNotes` thuộc `MainWindow`, đăng ký DocumentListener. `notesOverlay` nằm quanh scrolled viewport chính, sau sidebar và thanh công cụ. Mỗi khung dùng `XournalView` ở chế độ phụ, một `XojPageView`, `InputContext`, `ScrollHandling` riêng; dùng **cùng Control/ToolHandler/engine**. Không tạo thêm MainWindow/Control, không đổi currentPage của trang chính để giả lập.

`Control::getCurrentPage[No]` vẫn là trang chính. `getEditingView/getEditingPage` trả đích đang viết; `activateEditingView` được gọi trước input press của engine và khi chạm tiêu đề/nút khung. Chạm trang chính trả đích về chính. Các đường clipboard, selection, màu/độ dày, thêm nét/chữ/ảnh, công thức, đồ thị/vectơ và lớp đã được chuyển sang accessor chỉnh sửa tương ứng. API có số trang rõ ràng và các lệnh `all/notePages` vẫn giữ collection chính. Lệnh dọn nháp cũ chuyển đích undo về trang chính.

Giấy có zoom riêng, các handler lấy transform từ XournalView. Ctrl+wheel và pinch trong nháp không thay zoom đề. Hand dùng adjustments của chính view. Kéo tiêu đề/resize là vùng widget riêng; không đưa tọa độ kéo khung vào engine mực. `finishInput` chốt sequence native tại mẫu cuối, kết thúc spline/text trước stash/zoom/resize/đổi đích. Không tự ẩn bởi leave/focus/timer. Nút thân bút dùng StylusInputHandler hiện có, không có engine bút thứ hai.

`editingGeneration` tăng khi đổi đích. Clipboard request giữ weak lifetime + PageRef/lớp/generation; kết quả muộn khác đích bị bỏ. Dialog Toán giữ PageRef/lớp/zoom/generation. Nguồn PDF OCR lấy từ view chính, tách khỏi đích note; worker vẫn dùng snapshot/lifetime/generation hiện có. Không truyền widget hoặc selection pointer vào worker.

## Rendering và ngân sách

Tối đa **6 editor** được giữ; mở thêm sẽ cất khung cũ, không xóa tờ. Collection lưu được nhiều hơn sáu tờ. Một tờ bắt đầu 600×840 pt; `+ Giấy` thêm 840 pt, tối đa **64 đoạn/tờ**, sau đó cần tạo tờ khác. Không tự nối giấy mỗi motion.

Trang phụ dùng `DocumentView` native với Cairo clip trong GTK draw, cộng các native tool/selection overlays. `LayerView` lọc phần tử ngoài clip. Không cấp bitmap toàn chiều dài và không schedule RenderJob toàn trang cho giấy phụ. Pipeline buffer/render nền của trang chính giữ nguyên. Đây vẫn là engine render vector native; chỉ thay chiến lược cache view phụ.

Kho là lưới vẽ ảo: chỉ vẽ hàng giao với clip. Thumbnail render qua idle ưu tiên thấp, một ảnh/lượt, gộp thay đổi sau commit 120 ms. Pool GtkButton có tên trợ năng riêng, chỉ bind các thẻ đang nhìn thấy; nút không sử dụng không được show_all hồi sinh. Cache LRU tối đa **64 ảnh**, hàng đợi tối đa **64 yêu cầu**, khóa UUID/revision/width/theme; ảnh đại diện lấy vùng mực đầu tiên, nhãn ghi tổng số đoạn. Nội dung selection tạm tách khỏi layer cũng được vẽ vào thumbnail. Cache không phải dữ liệu tài liệu; xóa cache chỉ tạo lại ảnh. Document change hủy nguồn idle/timer và pending requests; weak token bảo vệ export callback; editor hủy idle phục hồi scroll của chính nó.

Cả thứ tự GTK overlay và GdkWindow phải được cập nhật khi đưa khung/kho lên trước. Chỉ đổi một thứ có thể làm ảnh và nơi nhận input khác nhau. Gallery phải nhận cả press/release. Đây là hợp đồng quan trọng khi chỉnh bố cục sau này.

## Undo và lifecycle

Một UndoRedoHandler native giữ các action nhưng undo/redo chọn action mới nhất phù hợp PageRef của đích. Trang chính bỏ qua action nội dung của note; note không undo nét của note khác. Action tạo/xóa/đổi tên/ghim/thêm giấy dùng snapshot metadata giữ PageRef. Lịch sử lifecycle có thể được hoàn tác từ trang chính sau khi tờ đã cất/xóa. Xóa tờ là tombstone, vẫn lưu nội dung để phục hồi; không có xóa vĩnh viễn/hàng loạt trong lượt này. Xóa nội dung dùng DeleteUndoAction native cho mọi loại phần tử.

Dirty dùng tập serial action, không dựa vào action cuối của một history đã phân tuyến. Có saved state toàn tài liệu và theo PageRef cho dấu `*` từng thẻ. Undo/redo về trạng thái đã lưu được nhận diện; lịch sử không được lưu qua lần khởi động mới. Ẩn/mở trong cùng phiên giữ lịch sử. Rect/zoom/scroll là trạng thái view, không tạo hàng trăm bước undo.

`Chép chọn → trang chính` clone native phần tử, fit đồng tỉ lệ vào phần trang chính đang thấy, giữ bản gốc, một InsertsUndoAction tại đích và chọn cả cụm. Clipboard thường vẫn dùng serializer native, có guard request riêng.

## Save/recovery và xung đột

Save/Save As chuẩn bị các selection/text/input phụ trước snapshot. SaveHandler chung ghi vào tệp tạm UUID cùng thư mục, đóng gzip rồi rename; thất bại giữ bản đích cũ và trả lỗi. SaveJob copy bản backup thay vì rename mất bản chính trước commit; chỉ update thông tin/saved state sau thành công. Trong blocking job, viewport và các khung phụ bị vô hiệu hóa, rồi focus trở lại đích chỉnh sửa.

Autosave dùng cùng serializer. Nó chụp trạng thái history trước khi chuẩn bị dữ liệu và chỉ xác nhận trạng thái đó sau khi ghi thành công. Nét mới trong lúc ghi vẫn dirty. SaveHandler đóng gzip rồi thay tệp autosave trực tiếp trong cùng thư mục. Bản phục hồi cũ vẫn ở đúng đường dẫn cho tới khi có bản thay thế.

UUID recovery đổi khi thay tài liệu. Tài liệu chưa đặt tên có autosave theo UUID trong cache. Sổ đã đặt tên có suffix UUID trong autosave cạnh tài liệu. Save As đổi nơi lưu và giữ ID các tờ. Notebook chưa đặt tên lưu phiên thành `session-UUID.xopp`. `session.ini` trỏ đúng tệp đó và reader vẫn đọc phiên cũ `session.xopp`. Thoát bình thường giữ chính sách tự lưu phiên của bản custom, gồm cả trạng thái view.

Crash handler ghi `emergencysave-UUID.xopp` theo từng tài liệu. Lúc khởi động, ứng dụng đề nghị mở bản mới nhất và vẫn nhận diện tên cũ `emergencysave.xopp`. Restore nạp vào RAM và giữ các tệp trên đĩa cho tới khi người dùng chủ động xóa. Crash giữa lúc engine đang hỏng có thể bỏ sót nét chưa commit hoặc selection đang tách khỏi layer.

Document giữ mtime khi mở/lưu. SaveJob phát hiện file bị phiên khác thay đổi và báo dùng Lưu như để giữ hai bản; không âm thầm áp dụng “lần lưu cuối thắng”. Đường tự lưu khi thoát cũng kiểm mtime: nếu có xung đột, lưu riêng session và giữ bản ngoài đĩa. Đây là phát hiện thay đổi bằng mtime, không phải hệ thống khóa phân tán hoặc merge tài liệu. Các giới hạn/ca còn chờ có trong nhật ký và bảng kiểm thử.

## Điểm nối và bàn giao

- `gui/FloatingNotes.*`: khung/kho/lifecycle, active view và cache.
- `control/FloatingNotebook.*`: xuất sổ PDF vector, phân đoạn danh nghĩa 840 pt, có thể nới đến 1050 pt để giữ công thức/chữ ngắn qua điểm chia, ghi an toàn.
- `model/{Document,XojPage}.*`, `xojfile/{SaveHandler,LoadHandler,XmlParser}.*`: nội dung và schema.
- `undo/UndoRedoHandler.*`: phân tuyến lịch sử và dirty state.
- `Control`, `InputContext`, `XournalView`, `PageView`, ClipboardHandler, LayerController, TeachingToolsDialog: đích input/chèn và lifetime.
- Lua `floatingNotes(toggle/new/library/toggle-library)`, `getFloatingNotes()` (inventory, main stroke count): UI/automation, không chứa nội dung riêng ngoài Document.

`TouchDrawingInputHandler` phải hủy subscription ToolHandler khi editor bị evict; Layout bỏ callback adjustment khi widget dispose. Hand scroll và selection autoscroll dùng nguồn GLib có owner, hủy khi finish/hủy editor; không tạo thread giữ con trỏ editor sau vòng đời. `getVisibleRect` của aux lấy giao hình học viewport trực tiếp, không phụ thuộc flag visible của scheduler trang chính. Phím Alt+N/Ctrl+Alt+N đi trước dispatch GTK3, chỉ trong main window/transient/menu của đúng document, để entry/modal công thức không nuốt phím; blocking save vẫn chặn thay notebook.

Plugin và toolbar cá nhân nằm ngoài repo; các delta tái lập ở `SO-NHAP-NOI-plugin.patch` và `SO-NHAP-NOI-toolbar.patch`. Đối chiếu bản sao trước lượt tại `build/floating-notes-native/backup/home/phuc/`; không áp patch mù lên bản plugin mới của người dùng. Lua API có khai báo ở `plugins/luapi_application.def.lua`.

Không quay lại các sửa Toán/PDF/TexImage/Vectơ có trước. Đọc diff theo backup của lượt này để phân biệt thay đổi cũ. Không reset/restore toàn repo; không tự commit/push.

Cụm phím Sổ nháp nổi dùng Alt+N, Ctrl+Alt+N và Alt+K. Native dispatch dùng cùng danh sách accelerator với các nhãn của cụm nút trong vùng giấy. `showToolbarShortcuts` bật/tắt đồng thời nhãn của toolbar và của cụm nút. Lua `library` vẫn mở kho, còn `toggle-library` đổi trạng thái mở/cất. Alt+N dựa trên tờ thực sự đang mở để tờ vừa tạo hoặc mở từ kho trở thành nhóm gần nhất.
