# Sổ nháp nổi — cách dùng

Mở bản cá nhân `/home/phuc/.local/bin/xournalpp` sau khi lưu công việc trong phiên cũ. Trình cài không đóng phiên đang dùng và không thay `/usr/bin/xournalpp`.

- **Alt+N** hoặc **Nháp nổi** gọi/cất nhóm tờ đang dùng. Lần đầu tạo một tờ, lần sau mở lại đúng nhóm vừa cất.
- **Ctrl+Alt+N** hoặc **Tờ mới** tạo một tờ riêng và giữ các tờ trước.
- **Alt+K** hoặc **Kho nháp** mở/cất kho ảnh. Khi mở bằng phím, con trỏ vào ô tìm kiếm để gõ tên ngay.
- **Ctrl+\`** bật/tắt nhãn phím dưới cả ba nút. Cụm nút nằm trên thanh công cụ VK640 và ở góc dưới bên phải vùng giấy. **F1** có cùng bảng phím trong mục Sổ nháp nổi.


## Viết và sắp xếp

Viết bằng công cụ bút/tẩy/highlighter/hình/vùng chọn đang dùng. Bút thân VK640 vẫn theo vòng Pen → Eraser → Hand → Pen của bản custom. Chạm giấy hoặc tiêu đề chọn đích viết; chạm trang đề trở về trang chính. Hai tờ có thể mở cạnh nhau, kéo **thanh tiêu đề** để dời và kéo góc **◢** để đổi khung nhìn. Resize không co mực.

`− / +` chỉ zoom giấy nháp. Hand/cuộn trong tờ chỉ dời giấy của tờ đó. `+ Giấy` thêm 840 pt để viết tiếp; có thể trở lại phần đầu. Tối đa 64 đoạn trên một tờ; tạo tờ mới khi đạt giới hạn, bài cũ được giữ. Nút **▣** mở rộng tờ rồi trở lại khung nhỏ.

**⌄** cất tờ về kho, không xóa mực. Tờ vẫn giữ khi đổi trang/zoom/bố cục đề, không tự ẩn khi nhấc bút hoặc rời con trỏ. Tối đa sáu khung viết cùng lúc; mở thêm cất khung cũ, không giới hạn số tờ lưu thành sáu.

## Kho ảnh và menu tờ

Thẻ có ảnh mực thật, tên, trang nguồn, số đoạn và trạng thái mở/cất/trống/đã chọn/ghim. `*` chỉ tờ có thay đổi nội dung/metadata chưa lưu. Ảnh đại diện không phải toàn bộ nội dung tờ dài. Tìm theo tên hoặc “trang 3”; sắp theo tạo/cập nhật/tên, ghim luôn trước. Một lần chạm ảnh mở đúng tờ, không tạo bản sao. Có điều hướng mũi tên/Enter trong lưới.

Nút **⋯** có đổi tên, ghim, nhân bản, về trang nguồn, chép vùng chọn sang trang chính, xuất PDF, đổi giấy trắng/kẻ dòng/ô vuông, xóa nội dung và xóa tờ. Các lệnh xóa nằm trong menu riêng. Nhân bản có ID/nội dung độc lập. Trang nguồn đã xóa không làm mất nháp; lệnh về nguồn báo nguồn không còn.

Xóa tờ có hoàn tác và mục **Thùng rác** trong ô sắp kho để phục hồi. Xóa nội dung xóa riêng nội dung tờ ấy, có Ctrl+Z. Ctrl+U áp cho đích đang viết; Ctrl+Shift+M vẫn dọn các trang nháp cũ bên PDF và không quét collection nháp nổi.

## Undo, chuyển kết quả, lưu và xuất

**Ctrl+Z / Ctrl+Shift+Z** áp cho tờ đang hoạt động; chạm tiêu đề để chắc chắn chọn đúng tờ. History nội dung từng tờ độc lập trong cùng phiên. Action tạo/xóa/đổi tên/ghim/thêm giấy có thể được hoàn tác sau khi cất tờ. Mở lại file giữ dữ liệu nhưng không giữ toàn bộ lịch sử undo của phiên trước.

Chọn vùng cần dùng rồi **⋯ → Chép chọn → trang chính**. Ứng dụng clone nét/chữ/ảnh/công thức native, fit vào phần trang chính đang thấy, giữ bản gốc và chọn cả cụm; một undo tại đích. Copy/cut/paste thường và chèn Toán/công thức cũng dùng đích tờ đang chọn. OCR vẫn đọc vùng từ PDF chính; đích chèn và nguồn là hai khái niệm riêng.

**Ctrl+S** lưu sổ cùng `.xopp`, kể cả tờ đang cất. Save As hoặc chép `.xopp` sang thư mục khác giữ sổ; không cần JSON/cache bên cạnh. Tờ có nội dung chưa lưu làm tài liệu dirty và đóng cần xử lý lưu. Tài liệu chưa đặt tên được tự lưu phiên khi thoát theo chính sách bản custom. Khi cần lấy bản autosave, chép file `.autosave.xopp` sang tên mới kết thúc bằng `.xopp` rồi mở bản chép. Tài liệu chưa đặt tên dùng thư mục cache `xournalpp/autosaves`. Tài liệu đã đặt tên có file autosave ẩn cùng thư mục. Bản autosave gồm những nét đã vào lần ghi gần nhất. Khi ứng dụng báo có bản lưu khẩn cấp sau crash, chọn **Restore file** để mở bản mới nhất. Các tệp `emergencysave-UUID.xopp` trong thư mục cấu hình được giữ để có thể mở lại hoặc chép sang nơi khác. Khi ghi lỗi, giữ nội dung trong RAM, chọn nơi lưu khác. Nếu một phiên khác đã ghi lại cùng file, ứng dụng yêu cầu **Lưu như…** để giữ cả hai bản.

**Xuất tờ PDF…** trong menu hoặc **Xuất cả sổ PDF…** ở kho xuất vector, đủ các đoạn ngoài viewport. Xuất/in chính chỉ gồm các trang chính, không chụp khung nháp đè lên đề.

**Tương thích:** bản Xournal++ cũ hiển thị nháp thành trang thường; lưu lại bằng bản cũ làm mất thông tin nổi/ẩn/nguồn. Giữ bản sao và tiếp tục chỉnh bằng bản custom này. Không dùng PDF làm bản lưu chỉnh sửa của sổ.

## Ảnh trên bản cài

![Ba phím tắt hiện dưới thanh công cụ và cụm nút dưới vùng giấy](../../output/floating-notes-shortcuts/gui-2/005-notebook-shortcuts-on.png)

![Hai tờ nổi độc lập](../../output/floating-notes-native/final-3/full/020-after-drag.png)

![Kho 100 tờ có nội dung, theme tối](../../output/floating-notes-native/final-3/dark/004-library-theme.png)

![Kho ở cửa sổ nhỏ, DPI 2×](../../output/floating-notes-native/final-3/dark-2x/026-library-small-window.png)

## Minh chứng và giới hạn

Xem [kiểm thử](SO-NHAP-NOI-KIEM-THU.md), [kiến trúc](SO-NHAP-NOI-KIEN-TRUC.md), [nhật ký lỗi](SO-NHAP-NOI-NHAT-KY-LOI.md). Ảnh/file native nằm trong `output/floating-notes-native/`; mỗi thư mục gui-N là một lượt thật, kể cả lượt không đạt.

Chưa được coi kiểm thử X11/Cua hoặc fixture pressure là đo áp lực/palm rejection của VK640 vật lý. Phạm vi Wayland/DPI và các ca chưa kiểm đủ được công bố riêng; không lấy kết quả prototype Python làm nghiệm thu.
