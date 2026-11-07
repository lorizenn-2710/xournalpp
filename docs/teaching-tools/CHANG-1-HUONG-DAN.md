# CHẶNG 1: đồ thị, tiếp tuyến, bảng xét dấu và biến thiên

Bổ sung 03/11/2026: [mở rộng PDF → công thức và đồ thị](PDF-CONG-THUC-README.md), gồm OCR cục bộ và công thức vector.

Cập nhật hoàn thiện 02/11/2026: [kết quả rà soát bổ sung](CHANG-1-RA-SOAT.md).

Bản cá nhân: `/home/phuc/.local/bin/xournalpp`. Mở lại ứng dụng sau khi cài để nạp binary và plugin mới; trình cài không đóng phiên đang dùng. Không thay đổi `/usr/bin/xournalpp`.

## Mở và chèn

- Bấm nút **máy tính** đầu cột công cụ thứ hai, hoặc **Alt+G**, hoặc menu plugin **Toán: đồ thị, tiếp tuyến, bảng biến thiên**.
- Chọn **Đồ thị**, **Điểm & tiếp tuyến (hàm 1)** hoặc **Bảng biến thiên / xét dấu** ở ô trên cùng.
- Chọn mẫu, nhập thông số và xem trước ở bên phải. Nút **Chèn cả cụm** chỉ hoạt động khi đầu vào hợp lệ. Hủy/Escape không chèn gì.
- Hình dùng nét và chữ vector, được chọn sẵn để kéo/thu phóng. **Ctrl+Z** bỏ cả cụm một lần; **Ctrl+Shift+Z** phục hồi cả cụm.
- Cụm nằm trong phần trang đang nhìn, chỉ thuộc trang/lớp hiện tại. Giảm zoom nếu vùng nhìn nhỏ hơn 270 × 185 pt.
- Tham số, công thức, màu, chế độ và bảng được nhớ sau khi chèn trong `~/.config/xournalpp/math-tools.ini`; chọn **Mẫu gần nhất** để gọi lại. Không ghi đè cấu hình bút.

## Đồ thị và tiếp tuyến

Hai hàm cùng hệ trục, mỗi hàm có ô bật/tắt và ô màu riêng. Ô m có nút tăng/giảm; có thể nhập số trực tiếp. Nhập miền x/y, bước chia và tên trục (ví dụ x/y, t/v, V/p). Chọn cùng tỉ lệ nếu cần; bỏ chọn để dùng hai thang khác nhau. Khi bật π, bước x tính theo bội π: `1/2` nghĩa là π/2. Mốc thưa đi khi khung chật, không đổi miền hay giá trị của trục. Các ô số chấp nhận biểu thức hằng như `pi/2`, `exp(1)` hoặc `−3`; biến x không được dùng trong các ô số.

Cú pháp: `x`, `m`, `pi` hoặc `π`, số thập phân/dạng `1e-3`, `+ - * / ^`, dấu ngoặc; `sin`, `cos`, `exp`, `ln`, `sqrt`, `abs` dùng ngoặc. Góc tính bằng **radian**. Có thể nhập hoặc dán dấu `−`, `×`, `·`, `÷` và số mũ viết trên như `x²`, `x³`, `x⁻²`; một dãy chữ số viết trên là một số mũ (`2³²` là 2^32). Dùng dấu chấm cho phần thập phân và dấu `*` để nhân: `m*x`, không viết `mx` hay `2x`. `-x^2` là `-(x^2)`; `^` kết hợp từ phải sang trái.

Mẫu hỗ trợ gồm bậc hai, bậc ba, `(a*x+b)/(c*x+d)`, `(a*x^2+b*x+c)/(m*x+n)`, sin/cos, mũ, log, căn và trị tuyệt đối. Trong ví dụ này a,b,c,d,n là hệ số cần thay bằng số; chỉ **m** là tham số có tên được nhập trực tiếp. Có thể dùng `2^x` hoặc `exp(x)` cho hàm mũ. Không thực thi mã hay lệnh từ công thức.

Ví dụ:

| Công thức / thao tác | Kết quả |
|---|---|
| `1/x`, x từ −2 đến 2 | Hai nhánh riêng; x=0 bị loại |
| `(x^2-1)/(x-1)` | Vẫn loại x=1; vòng tròn lỗ (1;2); không có tiệm cận đứng giả |
| `(x^2+1)/(x-1)` | Nét đứt cho x=1 và y=x+1 |
| `sin(x)`, `cos(m*x)`, m=2 | Hai đường cùng hệ đo; nhãn ghi m hiện tại |
| `x^2`, chế độ tiếp tuyến, x₀=1 | M(1;1), đường gióng và y=2x−1, kể cả khi hai trục khác tỉ lệ |
| `sqrt(x)`, tiếp tuyến x₀=0 | Tiếp tuyến đứng x=0 |
| `abs(x)`, tiếp tuyến x₀=0 | Báo không có tiếp tuyến duy nhất |

Tiếp tuyến áp dụng cho **hàm 1**. Đạo hàm được tính theo cây biểu thức trong tọa độ toán học. Không suy độ dốc từ màn hình. Điểm M phải nằm trong cửa sổ vẽ; miền không hợp lệ và điểm loại được báo lỗi trước khi chèn. Hình không có điểm x₀ chọn bằng chạm hay cát tuyến ở phiên bản này.

## Bảng

Chọn **Bảng biến thiên / xét dấu**, nhập hàm 1 rồi bấm **Tự sinh bảng…**. Phạm vi tự sinh là đa thức đến bậc ba, tử đến bậc hai chia mẫu bậc nhất, tại m hiện tại; mẫu có hệ số x bằng 0 được chuyển đúng sang đa thức. Các tổ hợp làm tăng bậc mẫu hoặc có mẫu lồng nhau có thể bị từ chối. Các hàm khác dùng bảng nhập tay.

Hệ số thập phân của các họ được khảo sát được giữ bằng số hữu tỉ để xác định nghiệm, bội, dấu và lỗ thủng. Nghiệm bậc hai và giá trị hiển thị vẫn là số gần đúng. Thuật toán dùng dấu hệ số đầu, nghiệm và bội của tử f′; không suy dấu cả khoảng từ vài mẫu. Ví dụ `x^3` có f′(0)=0 nhưng tăng ở cả hai bên; không đánh dấu cực trị.

Có hai cách nhập bảng:

1. **Nhập từng ô bằng bút**: hàng x nhập mốc; ô khoảng để trống. Bấm ô dấu để đổi lần lượt trống → + → − → 0 → ∥. Hàng f nhập giá trị/giới hạn hoặc mũi tên.
2. **Nhập cả hàng**: mở mục “Nhập cả hàng bằng dấu | (tùy chọn)” để nhập/dán mẫu theo định dạng cũ. Ba hàng phải có cùng số ô.

Nút **+ Cột / − Cột** thêm/bớt ngay trước cột cuối, giữ mốc cuối như +∞; giới hạn 2–19 cột. Với bảng nhiều cột, dùng nút **← / →** để cuộn từng cột. Cuộn không đổi dữ liệu. Bấm ô x/f rồi bấm **−∞, +∞, ↗, ↘, →, ∥, ○** để chèn ký hiệu vào vị trí con trỏ; phần văn bản đang chọn sẽ được thay thế. Ô đang gõ giữ nguyên focus khi xem trước cập nhật.

Ví dụ mẫu x² khi nhập cả hàng:

```
x:       −∞||0||+∞
dấu:     |−|0|+|
giá trị: +∞|↘|0|↗|+∞
```

`||` trong chuỗi đầu là hai dấu phân cách với một ô trống ở giữa. Để đánh dấu **điểm loại trong một ô**, dùng **`∥`**, khác với `0`. Bảng tự sinh điền sẵn ký hiệu này. Có thể điền nghiệm, giới hạn ±∞, lỗ ○, mũi tên ↗/↘/→ hoặc tên hàng dấu. Nội dung bảng nhập tay do người dùng chịu trách nhiệm; ứng dụng kiểm tra cấu trúc, không chứng minh các ô tự nhập.

Hai ô **Hiện dấu** và **Hiện giá trị / chiều** điều khiển hàng được chèn. Sau chèn, chỉnh từng chữ bằng công cụ Text. Muốn hiện lại đáp án đã che, gọi mẫu gần nhất, bật hàng và tạo lại bảng. Các nhãn trục/mốc được căn theo độ rộng chữ thực, tránh chồng nhãn M với O hoặc chồng giới hạn với vạch điểm loại.

## Độ chính xác và giới hạn

- Số hiển thị làm tròn khoảng 6 chữ số có nghĩa; tọa độ vẽ không lấy từ nhãn đã làm tròn. Chữ “≈” đánh dấu phương trình tiếp tuyến có hệ số không nguyên. Đồ thị là xấp xỉ bằng nét gấp khúc, không phải đường giải tích chính xác trên mọi độ phóng.
- Bộ lấy mẫu dùng khoảng để kiểm tra miền của cả đoạn trước khi nối, clipping và chia thích ứng. Kiểm tra độ cong tại giữa và hai điểm 1/4, 3/4 của đoạn, cùng bao khoảng. Ngưỡng chia thích ứng theo sai lệch tung độ quy đổi ra trang là khoảng 0.3 pt trong khung sinh; ở giới hạn độ sâu, đoạn có sai lệch đo được trên 1.2 pt bị bỏ thay vì nối sai. Đây là tiêu chí số, không phải chứng minh sai số toàn cục cho mọi biểu thức. Độ sâu chia tối đa 16 sau 128 khoảng gốc; tối đa 30.000 lần tính hoặc khoảng 180 ms mỗi hàm. Hàm quá khó bị từ chối, không chèn hình dở dang. Tên hàng/công thức dài tới mức chữ phải nhỏ hơn 4 pt cũng được báo lỗi để tránh chèn nội dung không đọc được. Với phân thức có lỗ thủng đã được đại số hữu tỉ xác nhận là đường thẳng trên miền xác định, bộ lấy mẫu dùng tính tuyến tính đó để tiết kiệm điểm; điều kiện loại ban đầu vẫn được kiểm tra trước khi nối nét. Tính toán bảng hữu tỉ giới hạn hệ số 16.384 bit và bậc được hỗ trợ.
- Khung x/y có độ rộng ít nhất 1e-9; mốc hữu hạn với trị tuyệt đối không quá 1e12. Ô m của giao diện nằm trong ±1e6. Trường hợp vượt độ phân giải số được thông báo.
- Đây là hình vector **tĩnh**. Kéo giãn hình trên giấy không đổi công thức, miền hay đơn vị toán học. `.xopp` lưu nét/chữ thông thường; không có metadata liên kết để sửa lại công thức của hình cũ. Gọi trình tạo để tạo hình mới, có thể xóa hình cũ bằng vùng chọn.
- Không phải CAS tổng quát. Tiệm cận/lỗ thủng tự đánh dấu và bảng tự sinh chỉ thuộc các họ hữu tỉ/đa thức đã nêu. Các biểu thức lồng có điểm không khả vi có thể được báo chưa xác định đạo hàm; không thay bằng độ dốc tùy ý. Biểu thức tổng quát khác vẫn qua bộ kiểm tra miền khi vẽ; có thể bị từ chối nếu không chứng nhận được ở ngân sách hiện tại.
- `.xopp` hiện ghi tọa độ với độ chính xác giới hạn của định dạng đang có. Bộ kiểm chứng dùng dung sai 1e-4 pt cho save/reopen; số nét, chữ, màu và kiểu nét phải giữ nguyên. Xuất PDF giữ vector.
- Chưa kiểm thử bút VK640 vật lý trong lượt này; các hợp đồng plugin/nút thân bút và kiểm thử ứng dụng thật trên X11 được chạy lại.

## Các phím đang có

Ctrl+` hiện/ẩn chữ phím tắt, gồm **Alt+G** dưới nút máy tính. Alt+` đổi 1/2 cột. Shift+Y/Ctrl+Y chia PDF và nháp; nút thân bút, cắt ảnh, chuẩn hóa PDF, hình học, màu và sóng sin giữ nguyên. F1 vẫn hoạt động. Không cài hay đổi bản hệ thống.

Bản vá bổ sung vào plugin/toolbar hiện có được lưu cùng hướng dẫn: `VK640-stage1-plugin.patch`, `VK640-stage1-toolbar.patch`. Hai bản vá chỉ chứa phần thêm của chặng 1; không phải bản thay thế toàn bộ cấu hình cá nhân. Build thêm phụ thuộc phát triển GMP (`libgmp-dev`, đã có trên máy), không cần root để cài ứng dụng vào prefix cá nhân.
