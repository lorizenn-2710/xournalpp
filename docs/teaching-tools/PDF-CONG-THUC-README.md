# PDF → công thức và đồ thị — bàn giao 03/11/2026

Mở rộng **CHẶNG 1** của bản cá nhân `/home/phuc/.local/bin/xournalpp`. Không triển khai chặng 2A/2B/3/4. Đọc [kết quả kiểm chứng](PDF-CONG-THUC-KIEM-THU.md) để phân biệt phạm vi đã thử và giới hạn.

## Mục lục cho người dùng và agent tiếp theo

- [Cách dùng ngay](#cách-dùng-ngay).
- [Bản sửa Ctrl+Shift+R → Alt+G và giao diện mới](PDF-CONG-THUC-GIAO-DIEN-MOI.md).
- [Setup, model, font và giấy phép](PDF-CONG-THUC-SETUP.md).
- [Kiến trúc, ownership, trạng thái và các hợp đồng cần giữ](PDF-CONG-THUC-KIEN-TRUC.md).
- [Kết quả kiểm thử, benchmark và bằng chứng](PDF-CONG-THUC-KIEM-THU.md).
- [Nhật ký lỗi, giới hạn còn mở](PDF-CONG-THUC-NHAT-KY-LOI.md).
- [Công cụ Toán chặng 1 trước mở rộng](CHANG-1-HUONG-DAN.md), [rà soát trước đây](CHANG-1-RA-SOAT.md).
- [Yêu cầu gốc của lượt này](PROMPT-PDF-SANG-DO-THI-VA-CONG-THUC.md).

## Cách dùng ngay

1. Mở lại **bản cá nhân** sau khi cài để nạp binary mới. Trình cài không đóng phiên đang dùng.
2. Bấm **Ctrl+Shift+R**, kéo khung bao trọn công thức trên PDF, rồi **Alt+G**. Dùng được với PDF có chữ và PDF scan, kể cả khung không chọn được nét/đối tượng nào. Bao gồm cả mũ, căn, tử và mẫu. Công cụ chọn chữ PDF/chọn chữ PDF theo khung vẫn dùng được; công cụ tô sáng giữ hành vi cũ.
3. Bấm **Vẽ đồ thị từ vùng chọn** ở bảng nổi, hoặc **Alt+G**. Ứng dụng tự lấy ảnh từ PDF nền và chạy OCR cục bộ. Ảnh không chứa nét/chữ/ảnh ghi chú của Xournal++.
4. Chọn một ứng viên trong danh sách. Nguồn được ghi rõ: chữ PDF hoặc OCR từng dòng, góc xoay và tỉ lệ thử. Các cách đọc khác nhau được giữ riêng, không nối mọi dòng thành một hàm.
5. Nếu các ứng viên hợp lệ chỉ có **một nghĩa AST** và không có cảnh báo OCR, ứng dụng tự đưa vào Hàm 1 và hiện công thức/đồ thị ngay. Khi có nhiều cách hiểu/cảnh báo, chọn và bấm **Dùng làm hàm 1**. Nếu cần hai đường, chọn ứng viên khác rồi **Thêm làm hàm 2**. Tối đa **hai** hàm trong một khung, như engine cũ.
6. Đối chiếu **ảnh gốc, công thức dựng đẹp và đồ thị**. Sửa phần sai trong ô ứng viên rồi bấm Dùng/Thêm lại; có thanh ký hiệu và bàn phím. Ô Hàm 1/2 cũng sửa được với cú pháp engine (`*`, `/`, `^`, ngoặc). Sửa công thức sẽ bỏ đánh dấu xác nhận.
7. Mở **Hai hàm & màu nét** hoặc **Miền vẽ & thang đo** để chỉnh; m và chế độ ở cột trái như trước. Không tự lấy hệ số làm giới hạn trục. Nếu chỉ cần công thức, bật **Chỉ chèn công thức (PDF vector)**.
8. Đánh dấu **Đã đối chiếu ảnh, công thức và đồ thị**, rồi **Chèn cả cụm**. Cụm được chọn sẵn để dời/thu phóng. Một undo/redo cho toàn bộ nét, chữ và công thức.

**Chọn vùng khác…** đóng bảng mà không chèn, chuyển sang chọn vùng PDF; kéo lại rồi bấm nút nổi/Alt+G. **Dừng nhận dạng** hủy worker; **Nhận dạng vùng PDF** chạy lại. **Hủy/Escape** đóng bảng mà không thêm phần tử/undo. Nếu trang, lớp hoặc zoom đã đổi khi bảng mở, đóng rồi mở lại bảng để xác nhận đích mới; không chèn sang trang khác một cách âm thầm.

Alt+G không có vùng PDF vẫn mở công cụ Toán bình thường. Mục PDF có thể thu gọn để dành chỗ cho tham số. Mẫu đã chèn được nhớ trong `~/.config/xournalpp/math-tools.ini`; ảnh PDF và kết quả OCR thô không được lưu vào setting này.

## Cú pháp nhận dạng và cách sửa

Nhận tiền tố `y=`, `f(x)=`, LaTeX phân số `\frac{...}{...}`, căn `\sqrt{...}`, sin/cos, ln, exp, π, lũy thừa, dấu Unicode, `|x|`, tham số m và phép nhân in thông dụng như `3x`, `mx`, `π x`. Tất cả được kiểm tra bằng AST Toán đang dùng; không chạy TeX/macro hoặc eval từ PDF.

- `1/(x+1)` khác `1/x+1`; `sqrt(x+1)` khác `sqrt(x)+1`.
- `−x²` khác `(−x)²`. `(x²−1)/(x−1)` vẫn loại x=1 và giữ lỗ (1,2).
- `1/2x`, `x2`, `√x+1`, `sin x^2` cần làm rõ bằng ngoặc, `^` hoặc `*`; ứng dụng không tự quyết định phạm vi.
- I/l/1, O/0, X/x và ký hiệu lạ không tự thay cho nhau. Phương trình ẩn `x²+y²=1`, hệ và chỉ số chưa hỗ trợ không bị đổi thành một nhánh y=f(x).
- Parse được **không chứng minh OCR đúng**. Ảnh mờ có thể sinh một công thức hợp lệ nhưng sai; luôn đối chiếu trước khi xác nhận. Không có phần trăm confidence tự bịa.

## Công thức đã chèn

Nhãn công thức chính, tiếp tuyến/tiệm cận và công thức riêng dùng PDF vector nhúng trong **TexImage**; có nguồn TeX sinh từ AST để dùng với công cụ LaTeX có sẵn của Xournal++. Double-click/chỉnh bằng công cụ LaTeX cần cấu hình LaTeX cũ của ứng dụng. **Nhận dạng, xem trước và chèn mới không cần chạy pdflatex.** Nguồn giữ cả giá trị m ở nhãn có tham số. Nhãn trục/bảng thường vẫn là Text chỉnh được.

Đồ thị vẫn là vector **tĩnh**. Sửa một công thức TexImage không tự tính lại các nét đồ thị bên cạnh. Muốn đổi hàm/miền, mở lại trình tạo và sinh cụm mới; không hứa metadata chỉnh liên kết mọi đồ thị cũ.

## Những giới hạn cần biết

OCR dành cho công thức **in** có vùng chọn rõ. Chưa nghiệm thu nhận chữ viết tay, trang nghiêng tùy ý, bố cục nhiều cột hay một bài toán dài. Có thể cần chọn vùng hẹp hơn hoặc sửa vài ký hiệu. Dòng được tách bằng khoảng trắng lớn; không phải bộ phân tích bố cục trang tổng quát. OCR giữ tối đa 12 cách đọc và 6 dòng; text layer có thể thêm tối đa 12 ứng viên. Quá nhiều cần chọn lại vùng.

Ảnh mờ/cắt mất công thức không thể được khôi phục chắc chắn. Cảnh báo chạm mép chỉ phát hiện được một số ca cắt; ảnh mất mũ/mẫu nhưng còn lề trắng vẫn phải do người dùng đối chiếu. Xem nhật ký giới hạn mở; các fixture sạch đạt không phải cam kết đúng mọi ảnh.

Giữ Ctrl+`, Alt+`, Shift+Y/Ctrl+Y, Ctrl+Shift+C, Alt+G và các công cụ cũ. Không sửa plugin/toolbar cá nhân trong lượt này. Chưa kiểm thử phần cứng VK640 vật lý; đã chạy hợp đồng plugin và kiểm thử ứng dụng trên X11 riêng.
