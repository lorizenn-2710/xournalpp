# Vectơ – đường thẳng – hợp lực

Đợt ưu tiên riêng, ngày 03/11/2026, lấy một phần M08/P01. Không phải hoàn thành chặng 2B hoặc 4A. Đợt kế tiếp là nguyên hàm–tích phân; chưa thêm chức năng đó vào giao diện.

## Mở và dùng

Mở bản cá nhân `/home/phuc/.local/bin/xournalpp` sau khi cài để nạp mã mới. Phiên cũ đang mở không bị đóng. Bấm máy tính hoặc **Alt+G**, chọn **Vectơ**, **Đường thẳng**, **Hợp lực** ở ô đầu. Đồ thị, bảng và luồng PDF → công thức vẫn ở cùng bảng.

Chọn Oxyz hoặc Oxy. Chuyển sang Oxy là lựa chọn **bỏ thành phần z**, giữ x,y; chuyển lại thêm z=0, không phục hồi z đã bỏ. Các tọa độ dùng dấu `;`, số thập phân dùng dấu chấm. Ví dụ `1;2;3`, `4;0;5`; Oxy dùng `1;2`. Có thể nhập hằng `sqrt(2)`, `pi/2`, số khoa học, nhưng không dùng biến x/m trong một tọa độ. Tên vectơ/đơn vị dùng tối đa 12 chữ/số ASCII (u, AB, F1, N, m…).

Chọn mẫu, sửa số và xem kết quả ngay. Phần tham số bên trái cuộn được; nút chèn/hủy luôn ở chân bảng. Tham số được nhớ **sau khi chèn**, trong `~/.config/xournalpp/math-tools.ini`. Hủy không ghi đè mẫu gần nhất.

- **Vectơ:** nhập u,v, hoặc dùng A,B với u=AB=B−A. Chọn tổng/hiệu/k·u/vectơ đối; quy tắc tam giác/hình bình hành. Kết quả xanh lá, u xanh dương, v đỏ; nét phụ/thành phần nét đứt. Có tọa độ, độ dài dạng căn và số gần đúng, tích vô hướng, góc, kiểm tra cùng phương/vuông góc. Vectơ 0 không có góc/hướng; không được coi là có hướng vuông góc/cùng phương với mọi vectơ.
- **Đường thẳng:** d qua A với chỉ phương u hoặc qua A,B. e qua B với chỉ phương v. Q là điểm cần kiểm tra thuộc d và dựng chân H lên d. Bảng cho quan hệ d/e, khoảng cách, chân gần nhất trên hai đường. Trong Oxy có tham số và tổng quát; Oxyz chọn tham số hoặc chính tắc kèm tọa độ cố định. Ví dụ u=(0;1;0) cho x=1, y=2+t, z=3; dạng chính tắc chỉ có (y−2)/1=t cùng x=1,z=3. Không tạo mẫu số 0. Đường vẽ trên trang là phần hữu hạn minh họa đường vô hạn.
- **Hợp lực:** lực đồng quy, đơn vị N. Thành phần: `10;0;0 | 0;10;0`. Chế độ độ lớn/góc: `10;0;0 | 10;90;0`. Oxy chỉ dùng độ lớn;phương vị. Phương vị đo từ +Ox về +Oy, góc nâng từ Oxy, đơn vị **độ**, góc nâng trong [−90,90]. R xanh lá, lực cân bằng E=−R đỏ nét đứt; các lực F1,F2,… xanh dương. Công thức cộng thành phần được lưu cùng hình. R=0 không có hướng. Với lực thẳng đứng, góc nâng ±90° nhưng phương vị không xác định. Tổng lực 0 chưa đủ kết luận cân bằng vật rắn khi chưa xét mômen.

**Trục phân tích (°)** quay hai trục vuông góc quanh Oz; u/R được phân tích theo x′,y′ và z trong Oxyz. Bật/tắt thành phần; góc nhìn Oxyz, tỉ lệ hình x/y chỉ thay hình, không đổi kết quả tính.

## Chọn điểm bằng bút

Trong Vectơ **từ hai điểm**, chỉ A,B,O hiện trên hình; trong Đường thẳng có A,B,Q,O (và H khi có). Đây là các điểm **đã có dữ liệu tọa độ**. Chế độ nhập vectơ bằng thành phần ẩn A/B/Q và bộ chọn điểm; các ô ẩn được giữ giá trị nhưng không tham gia kiểm tra đầu vào của chế độ này. Chọn đích “Chạm điểm khai báo → đặt A/B/Q” rồi chạm dấu điểm trên preview. Dùng cùng quy tắc tìm điểm gần nhất của nam châm hiện có; chạm khoảng trống không sinh tọa độ. Tọa độ được lấy nguyên từ dữ liệu, lưu đủ độ chính xác số, không đọc ngược pixel thành tọa độ 3D.

Hai điểm khác nhau trùng hình chiếu sẽ báo cần nhập tọa độ để phân biệt. Chưa nhập điểm có tọa độ bằng cách chạm một nét cũ trên trang: nét tay và Oxyz cũ không có metadata hệ đo. Sau chèn, các Stroke/TexImage thông thường vẫn tham gia chọn và bắt điểm của ứng dụng như các hình hiện có.

## Chèn, lưu và giới hạn

**Chèn cả cụm** thêm vào trang/lớp đã mở bảng, vừa vùng đang thấy; cả cụm được chọn sẵn để dời. **Ctrl+Z** bỏ cả cụm, **Ctrl+Shift+Z** phục hồi. Preview/Hủy/Escape không chèn hoặc thêm undo. Đổi trang/lớp/zoom khi bảng đang mở khóa chèn, cần mở lại bảng tại đích mới. Không dò khoảng giấy trắng tự động.

Nét và công thức PDF vector nhúng dùng cơ chế Stroke/Text/TexImage sẵn có; lưu/mở `.xopp`, copy/paste và xuất PDF giữ chúng. Đây là **hình tĩnh**: kéo giãn, sửa nét hay sửa một công thức không tính lại các đối tượng khác. Muốn đổi dữ kiện, gọi mẫu gần nhất để tạo cụm mới. Chưa có metadata quan hệ còn liên kết sau lưu/mở.

Đầu vào hữu hạn, trị tuyệt đối mỗi thành phần/số không quá 1e100; phép toán vượt miền số có lỗi rõ. Đã kiểm đến 1e−100/1e100; phép tích quá nhỏ đến mức mất biểu diễn sẽ bị từ chối, nên đổi đơn vị. Tọa độ vẽ lấy từ số đầy đủ, nhãn làm tròn khoảng 7 chữ số có nghĩa. Dung sai góc/quan hệ được định nghĩa trong [kiến trúc](VECTO-KIEN-TRUC.md); không phải đại số chính xác/CAS.

Tối đa 16 lực ở bộ tính. Công thức quá dài hoặc quá nhiều dòng so với vùng nhìn sẽ được báo chật, không chèn chữ không đọc được; giảm số lực/độ dài số hoặc tạo các cụm minh họa riêng. Cần vùng trang thấy tối thiểu khoảng 364×314 pt, có thể cần lớn hơn với hệ dài. Tỉ lệ hình trong [0.1,10].

Không bổ sung OCR vectơ/đường thẳng: parser PDF hàm một biến cũ không được dùng để đoán biểu thức tọa độ. Chưa có mặt phẳng nghiêng/ma sát, bộ giải cơ học tổng quát, mặt phẳng/cầu/thiết diện, đồ thị chuyển động hay nguyên hàm–tích phân. Chưa thử phần cứng VK640 vật lý; test dùng GTK/X11 riêng và hợp đồng Lua của bút.

Minh chứng và kết quả test: [kiểm thử](VECTO-KIEM-THU.md). Ctrl+`, Alt+`, Shift+Y/Ctrl+Y, Ctrl+Shift+C, Alt+G và các công cụ cũ giữ nguyên.

## Bản sửa theo ảnh người dùng — 03/11/2026

Các phép tính trong ảnh u=(1;2;0), v=(3;−1;2) là đúng: u+v=(4;1;2), tích vô hướng 1, góc xấp xỉ 83,13543°. Hình Oxyz là phép chiếu nên vectơ tổng có thể trông ngắn và góc nhìn thấy khác góc tính. Tiêu đề preview nay nhắc rõ điều này.

Đã bỏ điểm/ô nhập A,B,Q không dùng trong chế độ thành phần; Q chỉ hiện ở Đường thẳng. Đã bỏ nét và nhãn thành phần có giá trị 0 (uz/Rz…), không gắn tên mũi tên vào một đường chiếu co về điểm. Chữ phụ đậm màu hơn; ô nhập/nút trong bảng sáng có màu nhất quán khi hệ thống dùng theme tối. Sửa dữ liệu hợp lệ sẽ xóa dấu báo lỗi cũ; dữ liệu sai trong ô đã ẩn không khóa chèn.

[Xem bản sửa theo ảnh và bằng chứng](VECTO-SUA-THEO-ANH.md).
