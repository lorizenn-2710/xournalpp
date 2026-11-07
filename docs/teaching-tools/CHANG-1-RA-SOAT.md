# Rà soát và hoàn thiện CHẶNG 1 — 02/11/2026

Phạm vi theo lựa chọn của người dùng: hoàn thiện CHẶNG 1. Không triển khai chặng 2A hoặc các chặng sau.

Trạng thái: **đã hoàn thành và nghiệm thu lượt hoàn thiện CHẶNG 1**. Hướng dẫn hiện tại: [CHANG-1-HUONG-DAN.md](CHANG-1-HUONG-DAN.md). [Biên bản lần triển khai đầu](CHANG-1-NGHIEM-THU.md) và bộ minh chứng 9 trang được giữ lại để đối chiếu.

## Những lỗi tìm được và đã sửa

Bộ test cũ chạy đủ trước khi sửa: 10/10 CTest, 54 kiểm tra toán, Lua 121 PASS, tích hợp 32/32 và cả bốn nhóm kiểm tra GUI/thước/giảng bài/phím tắt đều đạt. Tuy nhiên, các ca mở rộng dưới đây tái hiện được lỗi mà bộ cũ chưa bao phủ. Log baseline: `build/math-stage1-polish-verification/baseline/`.

| Ca bổ sung | Trước rà soát | Sau sửa |
|---|---|---|
| `2^x`, `(1/2)^x`, `(x+3)^x` trên miền dương của cơ số | Lấy khoảng không bị chặn nên hết ngân sách, không tạo được hình | Dùng bao khoảng của exp(b·ln(a)); vẽ được trong ngân sách |
| `(x−0.12345)^0` | Có thể nối một nét qua điểm 0^0 bị loại | Giữ miền loại, tạo hai nét riêng |
| `ln(x³+2)` trên [−2,1] | Kiểm tra cả khoảng có thể kết luận sai là xác định | Bậc lẻ giữ cận dưới âm; không chứng nhận miền sai |
| `sqrt(0)`, `abs(0)`, `x+abs(0)`, `abs(x²)` tại 0 | Có thể trả đạo hàm NaN | Hàm hằng có đạo hàm 0; abs của biểu thức khả vi với đạo hàm 0 cũng xử lý đúng. `abs(x)`/`sqrt(x²)` tại 0 vẫn không bị gán tiếp tuyến tùy ý |
| `x^10000000000000000` tại −1 | n−1 bị làm tròn thành n, sai dấu đạo hàm | Dùng dạng tương đương ổn định số, cho −10^16 |
| Bậc ba dốc `1000000*(x−0.0078125)^3` | Trung điểm nằm trên dây cung nên bỏ sót độ cong; sai lệch tung độ quy đổi ra trang tại các điểm kiểm tra là 26.8221 pt | Thêm kiểm tra 1/4 và 3/4 trước khi nhận đoạn; sai lệch cùng phép đo 0.261934 pt, không hết ngân sách |
| Lỗ thủng khi tăng kiểm tra độ cong | Phiên bản trung gian có thể chạm 30.000 lần tính | Với thương bậc hai/bậc nhất có phần dư bằng 0 chính xác, tái sử dụng chứng nhận đường thẳng; ca mặc định còn 416 lần tính, vẫn hai nhánh và giữ điểm loại |
| `0e1000000000` khi tự sinh bảng | Có thể tạo lũy thừa 10 quá lớn trước giới hạn hệ số | Nhận hệ số 0 trước khi cấp phát/tính lũy thừa |
| Ô số chứa `exp(1)` | Kiểm tra ký tự x nên nhận nhầm là biến x | Phân biệt biến bằng cây biểu thức; chấp nhận biểu thức hằng |
| Nhãn M tại (0,0), nhãn mốc bảng | Có thể chồng O/M hoặc không thẳng vạch điểm loại | Căn theo độ rộng chữ thực, tránh giao nhau; kiểm tra trên Text thật trong tài liệu |

Kiểm chứng trước/sau được giữ ở `audit-before.log`, `audit-after.log`, `curvature-before.log`, `curvature-after.log`, `hole-budget-before.log`, `hole-budget-after.log` trong thư mục kiểm thử của lượt này. Log các lượt chưa đạt được giữ ngoài thư mục `final`; không bỏ ca cũ hay tăng ngân sách để che lỗi.

## Giao diện và thao tác bằng bút

- Bảng nhập từng ô, nút đổi dấu, thêm/bớt cột, thanh ký hiệu ±∞/mũi tên/điểm loại/lỗ và hai nút cuộn từng cột. Giữ cách nhập cả hàng bằng `|` trong mục tùy chọn.
- Chỉnh ô không làm dựng lại các widget đang gõ, nên giữ con trỏ/focus. Thay cả hàng, gọi mẫu hoặc tự sinh bảng mới đồng bộ lại lưới.
- Đã thử giới hạn 2 và 19 cột, thêm/bớt, cuộn phải/trái không đổi dữ liệu, chèn ký hiệu vào ô đang chọn, che/hiện hàng và nhớ mẫu.
- Ẩn hàm 2 khi ở chế độ bảng, chỉ hiện x₀ khi dựng tiếp tuyến. Các ô vẫn giữ giá trị khi chuyển chế độ.
- Bổ sung cách nhập dấu toán Unicode và số mũ viết trên; nhiều chữ số viết trên được hiểu là một số mũ.
- Phương trình tiếp tuyến/tiệm cận dễ đọc hơn: `y = 2x − 1`, `y = 0`, `y = x + 1`.
- Trường sai được đánh dấu cụ thể hơn; cấu hình số NaN/Infinity hoặc mode quá lớn được xử lý bằng giá trị dự phòng hữu hạn. Nhãn không đọc được dưới 4 pt bị báo lỗi trước khi chèn.

Ảnh từng bị cắt ở mép màn hình test là do cấu hình cửa sổ 1717 px được sao chép vào Xvfb 1600 px. Đã đặt kích thước 1500×1000 **chỉ trong cấu hình test riêng**, đồng thời kiểm tra kích thước cửa sổ trước mỗi lần chụp. Không sửa kích thước/cấu hình cửa sổ thật của người dùng.

## Kiểm thử và bản cài

- **79 kiểm tra toán** trong `tests/test-teaching-math.cpp`, liên kết chính core của ứng dụng. Kiểm tra độ cong đối chiếu nhiều điểm bên trong từng đoạn trả về, không chỉ kiểm tra sự tồn tại của công thức.
- CTest **10/10**, Lua **121 PASS**, tích hợp PDF/nháp **32/32**; thước, đầu vào thước, 18 kiểm tra công cụ dạy học và phím tắt được chạy lại.
- GUI Toán có **21 kiểm tra nền**, **9 ví dụ chèn**, **30 lần thêm trang đang render** và thao tác trực tiếp trên GTK. Chèn/chọn, một undo/redo cho nét và chữ, hủy xem trước và báo lỗi đều dùng tài liệu thật.
- Lưu/mở lại toàn bộ hình, chữ, trang/lớp, màu và kiểu nét được so sánh. Dung sai tọa độ 1e−4 pt; sai số lớn nhất quan sát 5e−6 pt.
- Tám lệnh cuối và mã trả về: `build/math-stage1-polish-verification/final/results.json`; log tương ứng `1.log` đến `8.log`. Log unit riêng: `build/math-stage1-polish-verification/math-oracle.log`.

Chỉ cài prefix `/home/phuc/.local`. Binary đã cài phải khớp SHA-256 với `build/xournalpp`; binary `/usr/bin/xournalpp` phải giữ hash trước lượt làm. Plugin và toolbar cá nhân không đổi trong lượt này. Backup của các tệp cần sửa, binary cá nhân và diff trước lượt làm: `build/math-stage1-polish-verification/backup/`.

## Bộ minh chứng mới

- [Tài liệu .xopp 12 trang (182 nét, 193 chữ, không có ảnh bitmap)](../../output/pdf/CHANG-1-hoan-thien.xopp)
- [PDF vector 12 trang](../../output/pdf/CHANG-1-hoan-thien.pdf)
- [Ảnh tổng quan](../../output/pdf/CHANG-1-hoan-thien-tong-quan.png)
- `output/pdf/minh-chung-hoan-thien/`: ảnh hộp thoại, thao tác bảng bằng bút, đồ thị mũ, tiếp tuyến tại O, bảng, log app và dữ liệu so sánh trước/sau lưu.

Thứ tự trang: lỗ và hai hàm; tiếp tuyến khác tỉ lệ; bảng bậc ba; 1/x; phân thức có tiệm cận xiên; ln và sqrt; sin và cos; bảng x³; bảng 1/x; hai hàm mũ; tiếp tuyến x² tại O; ca bậc ba dốc.

## Giới hạn giữ nguyên

Hình vector tĩnh, chưa có metadata sửa lại công thức của hình cũ sau mở file. Bảng tự sinh thuộc các họ đã công bố; bảng tay không phải bộ chứng minh toán học. Lấy mẫu và giá trị số có dung sai/ngân sách; không phải CAS tổng quát. Chưa thử VK640 vật lý trong lượt này; hợp đồng plugin và giao diện X11 đã kiểm tra. Các chặng sau vẫn chưa triển khai.
