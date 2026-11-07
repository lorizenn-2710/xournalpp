# Sửa luồng quét và giao diện — 03/11/2026

## Bạn đã thao tác đúng ý tưởng

Ctrl+Shift+R là chọn vùng chữ nhật thông thường. Bản trước chỉ nối nhận dạng với chọn chữ/vùng PDF chuyên biệt, nên khung bạn vừa kéo không được đọc khi bấm Alt+G. Đây là thiếu sót của bản trước.

Bản sửa đọc trực tiếp khung đã kéo trên đúng trang PDF, kể cả không chọn được đối tượng nào. PDF có chữ và PDF scan cùng đi qua luồng này. Không cần copy/chèn ảnh trước. Không lấy nét ghi chú đè trên PDF để nhận dạng, không đổi tài liệu khi mở/hủy bảng.

## Thao tác nhanh

1. Lưu công việc đang mở rồi mở lại **bản cá nhân** `/home/phuc/.local/bin/xournalpp` để nạp bản mới. Agent không đóng cửa sổ hiện tại của bạn.
2. Trên trang PDF, nhấn **Ctrl+Shift+R**, kéo bao riêng công thức, thả bút/chuột, rồi **Alt+G**.
3. Chờ nhận dạng. Bên phải có ba thẻ: **Vùng gốc**, **Công thức dùng để vẽ**, **Đồ thị xem trước**. Ảnh gốc được phóng theo nội dung; bật **Hiện toàn bộ khung đã quét** để kiểm mép vùng.
4. Một cách đọc hợp lệ duy nhất, không có cảnh báo: xem trước tự hiện. Có nhiều cách hiểu hoặc cảnh báo: chọn cách đọc và bấm **Dùng làm hàm 1**. Xem trước tự hiện vẫn chưa phải xác nhận OCR đúng.
5. Nhận sai ký hiệu: sửa ngay ô **Sửa chỗ nhận sai, rồi cập nhật hàm**, dùng thanh ký hiệu nếu cần; bấm **Dùng làm hàm 1**. Muốn hai đường: chọn ứng viên khác, **Thêm làm hàm 2**.
6. **Hai hàm & màu nét** cho sửa trực tiếp hàm/màu; **Miền vẽ & thang đo** cho chỉnh giới hạn trục/bước chia. m ở cột trái; kéo cuộn nếu màn hình thấp. Chế độ tiếp tuyến/bảng và công thức riêng vẫn có.
7. Đối chiếu cả ảnh, phạm vi phân số/căn/mũ, dấu và đồ thị; đánh dấu **Đã đối chiếu ảnh, công thức và đồ thị** → **Chèn cả cụm**. Cụm được chọn để dời; Ctrl+Z hoàn tác một lần.

**Không thấy công thức:** kiểm tra đang trên PDF, kéo xong vùng trước Alt+G và mở đúng bản mới. Chưa có vùng sẽ có chỉ dẫn trong mục **Lấy công thức từ vùng PDF**; **Chọn vùng khác…** đóng bảng/chuyển sang chọn vùng PDF, kéo lại và bấm nút nổi hoặc Alt+G. Có thể tiếp tục dùng Ctrl+Shift+R.

**Sai hoặc thiếu ký hiệu:** chọn khung hẹp hơn, bao đủ tử/mẫu/mũ/căn rồi nhận dạng lại; sửa chỗ sai. Không tự xác nhận một ảnh mờ chỉ vì đồ thị trông hợp lý. Phương trình ẩn/chữ viết tay/bài toán dài chưa thuộc phạm vi đã nghiệm thu.

**Hủy/nhận dạng lại:** không thêm phần tử hay undo. Khung vẫn dùng được cho Ctrl+Shift+C như trước. Chuyển trang làm vùng cũ không còn được lấy nhầm. Đổi trang/lớp/zoom trong bảng khiến đích chèn cũ bị vô hiệu.

## Bảo trì và bằng chứng

- Backup, log trước/sau và lỗi không đạt giữ trong `build/pdf-math-ux-verification/`. Không reset/restore/commit/push thay đổi người dùng.
- Hồi quy nguyên bộ và GUI thực: `build/pdf-math-ux-verification/final/results.json`.
- Test shortcut mới: `bash tests/integration/check-pdf-region-shortcuts.sh`; màn hình nhỏ: thêm `PDF_FORMULA_SCREEN=1280x720`.
- Test race render: `build/tests/test-teximage-render`; log trước sửa `concurrent-before.log` có SIGSEGV, sau sửa `concurrent-after.log`, 20 lượt lặp `concurrent-stress.json`.
- Kiến trúc: [PDF-CONG-THUC-KIEN-TRUC.md](PDF-CONG-THUC-KIEN-TRUC.md); nguyên nhân/trạng thái lỗi: [nhật ký](PDF-CONG-THUC-NHAT-KY-LOI.md).
- Model/runtime và chất lượng OCR không được nâng cấp trong lượt sửa giao diện. Benchmark trước vẫn là 20/20 fixture có ứng viên đúng, 18/20 đúng ứng viên đầu; không suy thành độ chính xác mọi tài liệu. Vẫn bắt buộc người dùng đối chiếu.

Ảnh/file kiểm chứng bản mới ở `output/pdf/PDF-CONG-THUC-giao-dien-moi/` sau lượt kiểm thử chốt; giữ nguyên bộ bằng chứng cũ cạnh đó. Tệp manifest ghi hash binary build/bản cài, hệ thống, plugin và toolbar. Chỉ cài vào `/home/phuc/.local`; `/usr/bin/xournalpp` được đối chiếu hash nguyên vẹn.
