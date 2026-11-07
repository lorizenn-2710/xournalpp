# Sửa hiển thị theo ảnh người dùng — 03/11/2026

Ảnh người dùng cho u=(1;2;0), v=(3;−1;2). Các phép tính trong ảnh **đúng**: u+v=(4;1;2), u·v=1, góc ≈83,13543°, độ dài lần lượt √5, √14, √21. Hình chiếu Oxyz có thể làm w trông ngắn và góc biểu kiến khác góc thật.

Lỗi cần sửa nằm ở trình bày và đọc dữ liệu không dùng:

- A,B,Q vẫn hiện khi đang nhập u,v bằng thành phần, làm rối hình; dữ liệu sai của các điểm này còn có thể khóa chèn dù không dùng.
- Thành phần bằng 0 vẫn có nhãn như uz/Rz ở đầu vectơ, gây hiểu nhầm về phương/thành phần.
- Chữ phụ quá nhạt; panel sáng nhưng entry/combo/nút lấy nền tối theo theme hệ thống.

Đã sửa danh sách điểm, hàng nhập và kiểm tra dữ liệu theo chế độ. Vectơ thành phần chỉ cần u,v; vectơ từ AB hiện A,B; Q chỉ hiện cho đường thẳng. Giá trị ô ẩn được giữ để đổi chế độ lại, không xóa dữ liệu. Bộ chọn điểm chỉ hiện khi có điểm cần chọn. Nét thành phần 0 và tên mũi tên co về một điểm không còn được vẽ; công thức vẫn giữ số 0. Dấu báo lỗi cũ được xóa khi đầu vào đã sửa.

Màu chữ/nét phụ đổi từ #8b94a2 sang #526174. CSS chỉ áp dụng trong bảng công cụ, làm ô nhập/nút phù hợp panel sáng cả khi app dùng theme tối; không đổi cấu hình theme hệ thống. Tiêu đề preview nhắc rõ hình chiếu Oxyz và vị trí đọc góc/độ dài thật.

## Bằng chứng trước/sau

- [Ảnh người dùng](../../output/pdf/VECTO-SUA-THEO-ANH/before-user.png).
- [GUI sau sửa, app thật với theme tối](../../output/pdf/VECTO-SUA-THEO-ANH/dark-1/screenshot-fixed-dark-theme.png).
- [Tệp .xopp](../../output/pdf/VECTO-SUA-THEO-ANH/dark-1/evidence.xopp), [PDF vector](../../output/pdf/VECTO-SUA-THEO-ANH/dark-1/evidence.pdf), [ảnh render PDF](../../output/pdf/VECTO-SUA-THEO-ANH/dark-1/export-render.png).

`build/vector-polish-verification/` giữ backup, baseline, log build/install, test trước/sau và bộ hồi quy. Năm ca mới đều thất bại trước sửa (`oracle-before.log`: 61 kiểm tra, 5 lỗi); sau sửa đạt **61/61** (`oracle-after.log`). Các ca kiểm điểm thừa, uz/Rz=0, Rx/Ry=0 khi lực đứng và giữ A/B đúng ở mode hai điểm.

GUI mới chạy GTK thật với `GTK_THEME=Adwaita:dark`, đạt **10 kiểm tra**: field/picker theo mode, nhập A sai rồi đổi sang thành phần vẫn chèn được, XML lưu không có A/B/Q/uz thừa, u+v và u·v giữ đúng, xuất PDF. Các kiểm tra này nằm trong `tests/integration/coordinate-gui.py`, bật bằng `COORDINATE_POLISH_ONLY=1`; không mock hình hoặc công thức. Ảnh GUI và PDF render đã được xem trực quan, PDF không có ảnh raster và font được nhúng.

Bộ GUI đầy đủ đạt 30 kiểm tra; CTest 14/14 và cả 10 bộ hồi quy hiện có được chạy lại, đều đạt trên binary đã cài. Lượt đầu test đầu vào thước bị trùng display Xvfb; chạy lại riêng nguyên suite đã đạt, log cũ vẫn được giữ. Kết quả phân biệt rõ trong `handover-manifest.json`, `final/results.json` và `final/effective-results.json`. Các log không đạt trước sửa được giữ. Không bỏ/nới test cũ. Một tiến trình test còn chờ sau SIGTERM đã được dọn đúng PID sau khi ghi đủ bằng chứng; nguyên nhân chưa xác minh, xem VEC-Q11 trong nhật ký.

Bản cài chỉ ở `/home/phuc/.local`. Giữ phiên đang dùng; bản mới được nạp khi người dùng lưu việc và mở lại app. Không thay binary hệ thống, plugin VK640 hay toolbar cá nhân; không commit/push. Phạm vi và giới hạn hình tĩnh, chưa thử bút vật lý giữ như [hướng dẫn](VECTO-DUONG-THANG-HOP-LUC.md).
