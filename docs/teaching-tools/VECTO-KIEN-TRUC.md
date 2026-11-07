# Kiến trúc tọa độ dùng chung Toán/Lý

Ngày 03/11/2026. Phạm vi M08/P01 ưu tiên riêng.

`CoordinateMath.{h,cpp}` chứa Vec, Line, phép cộng/trừ/nhân, chuẩn hypot, tích vô hướng, góc atan2(|u×v|,u·v), quan hệ đường, chân chiếu và hợp lực. Không có GTK, Lua, Cairo hoặc tọa độ màn hình trong phép tính. Cả Toán và Lý dùng chính các hàm này. `parseVec` tách kiểu tọa độ theo số thành phần và dấu `;`; chỉ dùng Expression cho **hằng số trong từng thành phần**, không đưa chuỗi vectơ/phương trình vào parser hàm một biến.

`CoordinateFigure.cpp` tạo Figure và Scene từ tọa độ thật, sau cùng mới chiếu sang Oxy/Oxyz và fit khung. Oxyz có quy ước Ox xuống trái, Oy sang phải, Oz lên trên tương thích công cụ cũ; góc nhìn/tỉ lệ có thể đổi mà không chạm phép tính. Scene giữ anchor → Vec của điểm đã khai báo. Các đường vô hạn được minh họa bằng đoạn hữu hạn, không suy giao điểm từ nét chiếu.

`MathNotation` được mở rộng các node ghép hàng, xếp hệ và mũi tên trên tên vectơ. Các node này do bộ dựng tọa độ tạo trực tiếp từ dữ liệu đã kiểm tra; không dùng parser scalar để đoán vectơ. `FormulaRender` dùng lại metrics Pango, căn/phân số và Cairo PDF của luồng công thức cũ. Nhãn typed giữ nguồn TeX sinh từ cây trình bày, được nhúng TexImage; không thực thi TeX trong preview/chèn.

`TeachingToolsDialog` giữ ba chế độ cũ ở chỉ số 0–2, thêm Vectơ/Đường thẳng/Hợp lực 3–5. Dữ liệu được xác minh và dựng đầy đủ trước `insertFigure`. Hàm này dùng đúng giao dịch chèn cũ: Stroke/Text/TexImage → một InsertsUndoAction → chọn cả cụm. Kiểm PageRef, lớp và zoom từ lúc mở bảng. Callback preview đồ thị cũ có generation/lifetime giữ nguyên; chuyển sang tọa độ loại kết quả cũ, hủy OCR đang chạy. Chọn vùng PDF mới ưu tiên đưa về luồng công thức cũ.

Bắt điểm preview gọi `ElementSnapper::nearestAnchor`, cùng implementation mà nam châm trang dùng; không sửa quy tắc latching, tolerance hoặc thứ tự ưu tiên đỉnh/trung điểm cũ. Tọa độ được lấy từ anchor đã khai báo và ghi 17 chữ số, không lấy từ pixel. Trùng hình chiếu giữa hai tọa độ khác nhau bị từ chối chọn. Nét cũ không được âm thầm hiệu chuẩn.

## Dung sai và số học

- Vec kiểm số hữu hạn và |thành phần|≤1e100. Norm dùng hypot tránh overflow/underflow không cần thiết. Dot/triple product tích lũy long double, trả double; tích khác 0 nhưng mất biểu diễn bị lỗi VEC-012.
- Vectơ không được nhận bằng chuẩn bằng 0. Góc trả optional rỗng khi một vectơ 0; parallel/perpendicular trả false cho vectơ không có hướng.
- Cùng phương: |û×v̂|≤1e−10. Vuông góc: |û·v̂|≤1e−10. Đường gần song song trong ngưỡng này được phân loại song song/trùng trong dung sai, không hứa giao điểm rất xa chính xác.
- Dung sai residual vị trí là `1e−10·|A−B| + 32·epsilon_double·max(|A|,|B|)`. Dùng cho khoảng cách từ điểm tới đường và so hai đường. Không dùng một ngưỡng điểm ảnh hoặc một sai số tuyệt đối cố định khiến dữ liệu nhỏ bị coi trùng hết.
- Chân chiếu: A + û·((Q−A)·û). Hai đường không song song dùng tích hỗn tạp với |û×v̂|², tránh phép trừ 1−cos² thiếu ổn định gần song song. Khoảng cách 3D dùng |(B−A)·n̂|.
- Lực tổng cộng từng thành phần bằng long double. R có độ lớn ≤32·epsilon_double·tổng độ lớn lực được quy về 0 trong sai số làm tròn; hướng rỗng, E=0. Đây là dung sai số, không kết luận vật rắn cân bằng. Các giá trị lượng giác gần 0 trong 4epsilon được chuẩn hóa để góc trục 90/180° không sinh hướng giả.
- Phân tích theo hệ trực chuẩn quay quanh Oz: Fx′=Fx cosα+Fy sinα, Fy′=−Fx sinα+Fy cosα; Fz giữ nguyên. Hình không đo lực/góc theo điểm ảnh.

## Lưu và mở rộng

Không đổi định dạng .xopp hoặc API Lua đăng ký UI. Mô hình là đầu vào trình sinh; hình được lưu ở dạng vector tĩnh, không lưu metadata quan hệ. Đây là ranh giới rõ cho tích hợp sau này: đồ thị chuyển động/tích phân có thể dùng Vec/Forces mà không kéo theo GTK/chiếu. Muốn chỉnh liên kết sau lưu phải thiết kế định danh, phiên bản, clone, migration, hành vi sửa nét và undo trước khi cam kết.
