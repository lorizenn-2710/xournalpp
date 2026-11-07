# Nghiệm thu CHẶNG 1 — 02/11/2026

Biên bản này ghi lượt triển khai đầu (54 kiểm tra). Bản hiện tại có lượt [rà soát và hoàn thiện bổ sung](CHANG-1-RA-SOAT.md) với 79 kiểm tra toán và bộ minh chứng mới; giữ biên bản cũ để đối chiếu.

Phạm vi M01–M03 và M04 nhập tay/tự sinh giới hạn. Các chặng 2A, 2B, 3A, 3B, 4A, 4B, 4C chưa triển khai. Hướng dẫn và giới hạn: [CHANG-1-HUONG-DAN.md](CHANG-1-HUONG-DAN.md).

## Baseline và bảo toàn

Nhánh giữ nguyên `feature/protractor-geometry-tools`. Không reset, restore, commit hay push. Trước khi sửa đã lưu bản sao các tệp bị sửa, plugin, toolbar, binary cá nhân và diff các thay đổi có trước tại `build/math-stage1-verification/backup/`. Tệp bổ sung sau khi khảo sát cũng được sao lưu trước khi sửa.

Baseline chạy thật: CTest 9/9; Lua 120 PASS; tích hợp 32/32; thước, đầu vào thước, công cụ dạy học (18 kiểm tra) và phím gợi nhớ đều đạt. Log từng lệnh trong `build/math-stage1-verification/baseline/1.log` đến `7.log`.

Giữ các phím Ctrl+`, Alt+`, Shift+Y/Ctrl+Y, công cụ hình học, nút thân bút, cắt ảnh và chuẩn hóa PDF. Chỉ thêm Alt+G và một nút máy tính. Bảng hợp đồng Lua tăng từ 22 lên 23 mục đăng ký; `appAccels()` rà phím của XML và ActionProperties, không có xung đột mới. Kiểm tra GAction/phím thật mở đúng hộp thoại; Escape hủy. Ctrl+` hiện nhãn và phục hồi bố cục toolbar với sai khác ảnh bằng 0 pixel trong vùng đối chiếu.

## Các thay đổi chính

- `src/core/math/TeachingMath.*`: parser giới hạn, đạo hàm cây biểu thức, kiểm tra miền bằng khoảng, lấy mẫu/clipping, khảo sát đa thức/phân thức bằng số hữu tỉ GMP, bảng tự sinh và kiểm tra bảng tay.
- `src/core/math/TeachingFigure.cpp`: cùng một trình sinh cảnh vector cho xem trước và chèn; nhánh tách riêng, tiếp tuyến, lỗ thủng, tiệm cận, trục/π và bảng.
- `src/core/gui/dialog/TeachingToolsDialog.*`: GTK, trường nhập lớn, báo lỗi, xem trước, lưu mẫu gần nhất, chèn Stroke/Text vào đúng lớp và một InsertsUndoAction; chuyển công cụ qua GAction để kéo được ngay.
- Lua API: `showMathTools()` và `insertMathFigure(options)`; phần tính toán không nằm trong Lua/GTK.
- `Layout::computePrecalculated()` cập nhật bộ ánh xạ, kích thước và tổng tích lũy dưới mutex mà phía đọc đang dùng. Kiểm thử mở rộng từng gặp assertion truy cập vector khi thêm trang; khảo sát mã chỉ ra RenderJob đọc vị trí ở luồng nền trong khi bên ghi chưa giữ mutex. Sau sửa chạy lại thêm 30 trang có đồ thị trong lúc render, cùng lượt stress riêng 60 trang liên tiếp, không còn lỗi quan sát được.

## Kiểm chứng toán học và hành vi

`tests/test-teaching-math.cpp` liên kết chính thư viện core được ứng dụng dùng, gồm **54 kiểm tra**, không sao chép lại thuật toán để tự chấm. Log đầy đủ: `build/math-stage1-verification/math-oracle.log`.

| Ca theo tài liệu | Kiểm chứng |
|---|---|
| 1/x trên [−2,2] | Đúng hai nhánh; mọi cạnh đều ở một phía x=0; clipping đúng. Thêm cực ngoài lưới ở ba kích thước khung. |
| (x²−1)/(x−1) | Giữ x=1 bị loại; lỗ (1,2), không có TC đứng; các nét không vượt lỗ. |
| (x²+1)/(x−1) | TC x=1 và y=x+1; minh chứng PDF trang 5. |
| x³−3x | Cực đại (−1,2), cực tiểu (1,−2), dấu +/−/+ và bảng tương ứng. |
| x³ | f′(0)=0 nhưng dấu +/+; không nhận cực trị; PDF trang 8. |
| Tiếp tuyến x² tại 1 | Giá trị và đạo hàm giải tích tạo y=2x−1 ở khung khác tỉ lệ; thao tác nhập và chèn thật qua GTK. |
| ln/sqrt/abs | Không lấy x≤0 cho ln; sqrt có (0,0) và tiếp tuyến đứng; abs(0) báo không có tiếp tuyến duy nhất. |
| sin/cos và π | Parser pi/π, radian, nhãn π của cảnh thật; hai đường trong một khung, PDF trang 7. |
| Hai hàm và m | Kiểm tra cảnh sinh, sửa hai hàm/m trong GTK, xem trước, bật đường thứ hai, nhớ mẫu sau chèn. |
| Suy biến/hệ số | Hàm hằng, bậc cao nhất bằng 0, mẫu hằng, mẫu 0, hệ số 1e±200, miền/bước không hợp lệ; không sinh NaN ra hình. |
| Zoom/cuộn và nội dung cũ | App thật ở zoom 1.5 và vị trí cuộn khác 0; toàn bộ nét/chữ thực sau layout nằm trong vùng nhìn. Nét/chữ gốc và vùng chọn cũ được giữ. |
| Undo/redo/cancel | Một undo/redo cho cả nét và chữ của hình/bảng; hủy hộp thoại giữ nguyên chữ và toàn bộ tọa độ; dữ liệu sai không chèn. |
| Save/reopen và PDF | 9 trang vector; số phần tử/điểm, trang/lớp, chữ, màu, kiểu nét giữ nguyên; sai số tọa độ lớn nhất quan sát 5e−6 pt, dưới dung sai 1e−4 pt. |
| Các phím cũ | Hồi quy đầy đủ và phím Ctrl+`, Alt+` thật; thêm Alt+G mở hộp thoại thật. |
| Biểu thức khó | Ngân sách 30.000/180 ms, độ sâu 16; test kiểm giới hạn và phản hồi dưới 2 giây; nhập hàm dao động cao trong GTK bị từ chối rồi sửa lại dùng tiếp. |

Bổ sung: hệ số thập phân `(x²−0.01)/(x−0.1)` có lỗ đúng, phần dư thập phân nhỏ khác 0 vẫn là cực. Kiểm tra không bỏ điều kiện xác định trong mẫu lồng nhau/lũy thừa 0; tính đạo hàm thương tránh bình phương mẫu bị underflow. Hệ số hữu tỉ tăng theo hàm mũ bị chặn bởi giới hạn bit.

## Bộ hồi quy

Chạy sau build/cài vào prefix cá nhân; mọi lệnh phải trả mã 0. Kết quả cuối cùng và mã trả về được ghi trong `build/math-stage1-verification/final/results.json`.

| Lệnh | Kết quả |
|---|---|
| `ctest --test-dir build --output-on-failure` | 10/10 |
| `bash tests/integration/run-tich-hop.sh` | 32/32 |
| `bash tests/check-protractor.sh` | PASS |
| `bash tests/check-protractor-input.sh` | PASS |
| `texlua tests/test-vk640-erase.lua` | 121 PASS, 0 FAIL |
| `bash tests/integration/check-teaching-tools.sh` | 18 kiểm tra PASS |
| `bash tests/integration/check-shortcut-hints.sh` | PASS, gồm Alt+G/Escape |
| `bash tests/integration/check-math-tools.sh` | PASS: 13 kiểm tra nền, 6 ví dụ, 30 lần thêm trang đang render và thao tác GTK thực |

Log lần lượt là `final/1.log` đến `final/8.log`. Mỗi phiên GUI dùng màn hình, bus và cấu hình riêng; chỉ đóng PID test, không đóng Xournal++ của người dùng. Việc kết thúc PID test có thể ghi cảnh báo signal 15/DBus đóng; điều này không được tính là lỗi tính năng. Log thử nghiệm thất bại trong quá trình sửa vẫn được giữ ngoài thư mục `final`, không bị xóa để che kết quả.

## Bản cài và minh chứng

`CMAKE_INSTALL_PREFIX=/home/phuc/.local`. Toàn bộ đường dẫn trong install manifest nằm dưới prefix này. SHA-256 của `/usr/bin/xournalpp` trước và sau giống nhau: `a8568bd21b65051287f6928d14a74153b5d6ef9918b3c29754c224d45c333b37`.

- `output/pdf/CHANG-1-kiem-chung.xopp`: bản lưu gốc của ứng dụng trước lượt stress, mở lại được, 9 trang; 134 nét, 149 chữ, 0 ảnh.
- `output/pdf/CHANG-1-kiem-chung.pdf`: xuất trực tiếp bằng Cairo của ứng dụng, giữ vector.
- `output/pdf/CHANG-1-tong-quan.png`: ảnh render cả 9 trang PDF.
- `output/pdf/minh-chung-gui/`: hộp thoại, tiếp tuyến, bảng che/hiện, hình chọn sẵn, lưu/mở lại, log app và dữ liệu tọa độ đối chiếu.
- `build/math-stage1-verification/`: backup, build/install/configure, baseline, unit, hồi quy và các lượt chẩn đoán.

PDF được render bằng Poppler để kiểm tra bố cục và chữ; các giới hạn trái/phải của bảng phân thức được đặt hai phía vạch điểm loại. Tài liệu chứa Stroke/Text, không dùng ảnh bitmap thay đồ thị. Phần preview PNG tiêu chuẩn trong file `.xopp` vẫn là cơ chế sẵn có của Xournal++.

Chưa có kiểm thử thao tác trên bút VK640 vật lý; không tuyên bố đã đo độ trễ/phần cứng. Hình là vector tĩnh, chưa có sửa tham số gắn với hình đã lưu hay chạm đồ thị chọn x₀; các giới hạn được công bố trong hướng dẫn.
