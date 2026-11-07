# Nhật ký lỗi, phòng vệ và giới hạn đợt vectơ

Ngày 03/11/2026. Log tại `build/vector-verification/`; mọi lượt không đạt được giữ, không bỏ/nới test cũ.

## VEC-Q01 — Chưa đưa file toán mới vào danh sách build

- Tái hiện: thêm CoordinateMath/CoordinateFigure rồi build cây CMake có sẵn.
- Fixture/log: `build-1.log`, undefined reference tới parseVec/generate lúc link.
- Mong đợi/thực tế: core chứa implementation mới / glob nguồn CMake chưa được cấu hình lại.
- Nguyên nhân đã xác minh: `src/core/CMakeLists.txt` dùng GLOB_RECURSE không CONFIGURE_DEPENDS.
- Trạng thái: đã sửa bằng cấu hình lại CMake; build sau và test liên kết chính core đạt. Không thay glob của dự án chỉ để che lỗi quy trình.

## VEC-Q02 — Hàm gọi lại GTK và test thiếu include

- Tái hiện/log: `build-2.log`, G_CALLBACK bị macro tách tại khai báo hai biến có dấu phẩy; test dùng initializer_list max thiếu `<algorithm>`.
- Mong đợi/thực tế: biên dịch / lỗi tiền xử lý và thiếu overload.
- Nguyên nhân: được compiler chỉ rõ ở chính dòng nguồn.
- Trạng thái: đã sửa khai báo và include; build-3 trở đi thành công. Các callback giữ cùng hành vi.

## VEC-Q03 — Harness dùng binary build thiếu đường dẫn tài nguyên

- Tái hiện: gui-1 chạy `build/xournalpp` trực tiếp; ảnh cây accessibility tại thư mục minh chứng lượt đầu ghi missing ui/toolbar.ini/pagetemplates.ini.
- Mong đợi/thực tế: cửa sổ test sẵn sàng / hộp lỗi tài nguyên chặn Save.
- Nguyên nhân đã xác minh: app tìm share tương đối đường dẫn build, không giống prefix cài cá nhân.
- Trạng thái: harness chạy bản đã cài đúng `/home/phuc/.local/bin/xournalpp`. Không bấm bỏ lỗi tài nguyên rồi tính là PASS.

## VEC-Q04 — Snapshot không có selection và thiếu tên trợ năng

- Fixture/log: gui-2 snapshot gọi getStrokes('selection') khi chưa chọn; app log báo “There is no selection”. Gui-3 không tìm thấy math-mode.
- Mong đợi/thực tế: số chọn=0 và tìm được mục chế độ / exception hoặc tên chỉ là widget CSS.
- Nguyên nhân đã xác minh: hợp đồng API selection có lỗi khi rỗng; gtk_widget_set_name không đặt tên AT-SPI.
- Trạng thái: snapshot dùng pcall đúng hợp đồng; combo và preview mới có accessible name rõ ràng. Test vẫn kiểm thao tác widget thật.

## VEC-Q05 — Fixture bấm điểm bỏ sót biên của tổng vectơ

- Tái hiện: gui-4 bấm O với phép biến đổi chỉ tính A/B, nhưng hình còn u+v và các trục; B không thành (0;0).
- Mong đợi/thực tế: chọn O / bấm sai vị trí trên preview.
- Nguyên nhân đã xác minh: công thức vị trí của fixture không tính khung bao thực tế; không phải app suy tọa độ từ nét tay.
- Trạng thái: fixture chuyên biệt A=Q=O, B=(4;0), v=0, trục có biên xác định độc lập; bấm XTEST thật và assert B chính xác (0;0). Gui-5 trở đi đạt nhánh chọn điểm. Không bỏ assertion chọn điểm.

## VEC-Q06 — Clipboard sau Save không còn selection

- Fixture/log: gui-5, before-copy.xopp và after-copy.xopp giống nhau.
- Mong đợi/thực tế: nhân đôi nét và TexImage / chưa có đối tượng để copy.
- Nguyên nhân đã xác minh trong `Control::saveImpl`: ứng dụng gọi clearSelectionEndText trước Save và trong doSave. Đây là hành vi cũ, không đổi.
- Trạng thái: test chọn lại trang bằng action select-all thật trước copy/paste. Assert số Stroke và TexImage tăng đúng gấp đôi, không chỉ kiểm file tồn tại.

## VEC-Q07 — Một lượt harness không quan sát được undo sau Save

- Fixture/log: gui-6, inserted.xopp và undo.xopp giống nhau; gui-5 từng đạt. Không có stack/timeline đủ để quy chắc lỗi cho undo ứng dụng hay Save còn bận.
- Mong đợi/thực tế: một undo xóa cả cụm / snapshot lượt đó còn nguyên.
- Nguyên nhân đã xác minh: dữ liệu file còn nguyên; **chưa kết luận nguyên nhân gốc** chỉ từ lượt này.
- Trạng thái: harness đã đồng bộ file Save tồn tại và chooser đóng, gửi GAction undo/redo của cửa sổ thật. Gui-7 đạt một undo xóa hết nét/công thức, một redo phục hồi XML đầy đủ. Không thêm undo lần hai hoặc giảm điều kiện so sánh.

## VEC-N01 — Phòng vệ số học và suy biến

- Fixture: `tests/test-coordinate-math.cpp`: zero, hai điểm trùng, gần song song, số lớn/nhỏ, polar 30°/210°, tích 1e−200·1e−200.
- Mong đợi: lỗi có mã thay NaN/chia 0/hướng giả; không đo theo pixel. Không ghi rằng mọi ca đã gây crash trước sửa.
- Nguyên nhân đã xác minh về số học: lượng giác và cộng số thực có residual; tích double có thể underflow; 1−cos² mất độ phân giải gần song song.
- Trạng thái: guard và phép tính chuẩn hóa được triển khai, unit test trên implementation thật. Hợp lực trong sai số 32epsilon được coi 0 có công bố; tích mất biểu diễn báo VEC-012.
- Mã lỗi đầu vào: VEC-001 số không hữu hạn/ngoài miền; 002 đường suy biến; 003 tọa độ sai; 004 độ lớn/góc lực; 005 số lực; 006 PDF công thức; 007 vùng nhìn nhỏ; 008 mode; 009 tên/đơn vị; 010 công thức quá chật; 011 tỉ lệ hình; 012 underflow tích.

## VEC-L01 — Hình tĩnh và dữ liệu trang cũ

- Tái hiện: sửa/kéo nét của cụm đã chèn hoặc mở file cũ chứa Oxyz.
- Mong đợi/thực tế: chỉ cam kết giữ hình vector; không có quan hệ tự tính lại sau lưu/mở và không đọc ngược Oxyz cũ thành tọa độ 3D.
- Trạng thái: giới hạn công bố. Chọn điểm chỉ trong tập tọa độ đã khai báo; nam châm/chọn đối tượng cũ vẫn hoạt động.
- Hướng sau: metadata có phiên bản, định danh, clone/migration/undo và xử lý sửa nét trước khi làm liên kết chỉnh sửa.

## VEC-L02 — Phạm vi và kiểm chứng chưa có

Chưa thử VK640 vật lý hoặc mọi DPI/theme/Wayland. Không có OCR vectơ/đường thẳng, mặt phẳng nghiêng/ma sát hay bộ giải cơ học mọi bài. Tối đa 16 lực trong tính toán; hình nhiều dòng/số dài có thể bị từ chối vì khung quá chật. Không kết luận đã hoàn thành chặng 2B/4A. Nguyên hàm–tích phân là đợt tiếp theo.

## VEC-Q08 — Điểm không dùng vẫn hiện và có thể khóa chèn

- Fixture: ảnh người dùng 03/11/2026, mode Vectơ, nguồn thành phần, u=(1;2;0), v=(3;−1;2); đổi sang AB, nhập A sai rồi quay về thành phần.
- Mong đợi/thực tế trước sửa: chỉ cần u,v / A,B,Q vẫn hiện và coordinateRequest đọc cả ba trường dù không dùng.
- Nguyên nhân đã xác minh: danh sách anchors khởi tạo mọi điểm, GTK luôn hiện các hàng và request đọc A/B/Q vô điều kiện ngoài mode lực.
- Sửa: chỉ tạo/hiện/đọc điểm theo mode; picker chỉ hiện khi chọn điểm có ý nghĩa; giữ giá trị ô ẩn.
- Trạng thái: 5 ca mở rộng trước sửa trong `build/vector-polish-verification/oracle-before.log` thất bại; sau sửa 61/61, GUI theme tối kiểm nhập sai A rồi chuyển nguồn, chèn lại được.

## VEC-Q09 — Nhãn thành phần 0 và nhãn mũi tên trùng gốc

- Fixture: u.z=0 ở ảnh; hợp lực phẳng trong Oxyz; lực thẳng đứng (0;0;10).
- Mong đợi/thực tế: không có đoạn thành phần dài 0 để gắn nhãn / uz,Rz hoặc Rx,Ry vẫn được tạo, xuất hiện tại đầu vectơ/gốc và gây hiểu nhầm.
- Nguyên nhân: CoordinateFigure đẩy Path vô điều kiện, sau đó thêm tên kể cả khi hai điểm chiếu trùng nhau.
- Sửa: chỉ vẽ thành phần khác 0, chỉ thêm tên mũi tên khi độ dài hình chiếu khác 0; công thức tọa độ vẫn ghi đầy đủ số 0.
- Trạng thái: unit test trước/sau và XML tài liệu app thật sau chèn được đối chiếu; không đổi kết quả số của ảnh.

## VEC-Q10 — Chữ phụ mờ và màu widget không nhất quán ở theme tối

- Fixture: ảnh người dùng: panel trắng nhưng entry/combo/nút giữ nền tối; chữ phụ xám nhạt.
- Nguyên nhân đã xác minh: CSS chỉ đặt nền panel, chưa đặt màu entry/combo, màu nét phụ 0x8b94a2.
- Sửa: CSS giới hạn trong đúng dialog đặt màu ô nhập/nút theo panel, không đổi theme hệ thống; màu phụ 0x526174; tiêu đề preview giải thích hình chiếu Oxyz.
- Trạng thái: GUI thật chạy với GTK_THEME=Adwaita:dark, ảnh được xem trực quan; font/công thức vẫn là vector. Log và ảnh trong `build/vector-polish-verification/` và `output/pdf/VECTO-SUA-THEO-ANH/`.

## VEC-Q11 — Một tiến trình kiểm thử còn chờ sau SIGTERM

- Quan sát trong lượt sửa theo ảnh: GUI đầy đủ đạt 30 kiểm tra, .xopp/PDF đã lưu; PID test 3198400 nhận SIGTERM lúc 17:10:07 nhưng còn chờ futex sau đó. Phiên người dùng PID 3154587 không liên quan.
- Nguyên nhân: **chưa xác minh**. Thử đọc stack bằng GDB không được hệ thống cho attach; không đổi ptrace/security hoặc dùng root để vượt hạn chế.
- Trạng thái: ghi nhận riêng cho vòng đời harness/app khi kết thúc display/bus thử nghiệm; chưa kết luận đây là lỗi thoát bình thường của phiên desktop. Đã xác minh config riêng `/tmp/xpp-pdf-formula.5eehUh/config`, log PASS và file đã lưu trước khi dọn đúng PID test này. Không quét/đóng các phiên khác.
- Các kiểm tra hành vi vẽ/chèn/lưu/mở/xuất đã đạt vẫn được giữ; không suy từ đó rằng mọi đường thoát ứng dụng đều đã được nghiệm thu.

## VEC-Q12 — Trùng màn hình Xvfb giữa hai suite kiểm thử

- Fixture: `build/vector-polish-verification/final/4.log` không kết nối được :101; `/tmp/xpp-pk.HIdjDn/xvfb.log` ghi “Server is already active for display 101”.
- Nguyên nhân đã xác minh: test đầu vào thước lấy display bằng `100 + PID % 60`, có thể trùng với Xvfb do suite GUI khác chọn tự động. Không phải phép tính vectơ hoặc thao tác thước sai.
- Xử lý: kết thúc các suite GUI khác rồi chạy lại nguyên lệnh kiểm đầu vào thước, không đổi assertion/đáp án. `final/4-retry.log` đạt toàn bộ và mã 0. Log lỗi đầu giữ nguyên.
- Quy tắc chạy tiếp: không coi suite này độc lập khi chạy song song với các suite dùng Xvfb; chạy tuần tự để tránh trùng display. `final/effective-results.json` phân biệt kết quả lần đầu và lượt chạy lại.
