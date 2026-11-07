# Bộ công cụ VK640 cho Toán/Lý THPT

Bản custom tại `/home/phuc/.local/bin/xournalpp`. Mã plugin Lua VK640 nằm trong `plugins/VK640ToolCycle/` và được cài vào thư mục plugin dùng chung của prefix khi chạy `cmake --install build`; bản cài chung tắt mặc định để không nạp trùng bản trong cấu hình cá nhân. Trên máy mới, bật **một** bản VK640 trong phần quản lý plugin. Đóng và mở lại app sau khi cài để nạp mã mới. Bấm **Ctrl+`** để hiện/ẩn chữ phím tắt dưới các nút bên trái và nhóm PDF/giấy nháp trên cùng. Nút hình bàn phím cạnh nhóm chia trang có cùng tác dụng. **Alt+`** đổi bố cục 1/2 cột. **F1** vẫn mở bảng hướng dẫn đầy đủ.

## Vẽ hình

| Công cụ | Kéo thường | Giữ Shift | Giữ Ctrl |
|---|---|---|---|
| Hệ trục tọa độ | Oxy đủ 4 góc phần tư, hai đầu trục dương có mũi tên | Oxyz: gốc ở điểm bắt đầu kéo, Oz lên trên, Oy sang phải, Ox chếch xuống trái | Oxy góc phần tư I, gốc dưới trái, hai mũi tên |
| Hình chóp | Chóp tứ giác S.ABCD | Chóp tam giác S.ABC | Chóp tứ giác có SA vuông góc đáy ABCD |
| Hình hộp | Hình hộp như trước | Hình lập phương | Lăng trụ đứng tam giác ABC.A'B'C' |

Các chế độ mới dùng phím Shift/Ctrl trực tiếp, độc lập hướng kéo. Với hệ trục và chóp, Shift được ưu tiên khi giữ cả hai phím. Với hình hộp, Ctrl chọn lăng trụ; Shift thêm ràng buộc khung vuông.

Chọn cả hình rồi bấm **Ctrl+Shift+N** để đặt tên đỉnh; nếu chưa chọn, ứng dụng thử nhận diện hình vừa vẽ. Chóp tam giác nhận A,B,C,S; lăng trụ nhận A,B,C ở đáy dưới và A',B',C' ở đáy trên. Ctrl+Z hoàn tác cả nhóm nhãn trong một bước.

## Bắt điểm

Ngoài đầu mút và góc, nam châm hút vào trung điểm cạnh từ **35 pt** trở lên, tâm và bốn cực trị elip/đường tròn. Trung điểm dùng bán kính hút bằng **85%** bán kính thường; nếu cách đều, đỉnh gốc được ưu tiên. Giữ Alt tạm đảo trạng thái hút điểm như trước. Bán kính đặt trong Cài đặt → Bút viết → Snap to elements.

## Phím mới

| Phím | Tác dụng |
|---|---|
| Ctrl+` | Hiện/ẩn chữ nhỏ dưới nút công cụ, gồm cả nút thêm giấy nháp Shift+Y. |
| Alt+` | Đổi bố cục 1/2 cột. |
| Alt+C | Xoay Đỏ → Xanh dương → Xanh lá đậm → Đen. Nếu màu hiện tại nằm ngoài vòng, bắt đầu bằng Đỏ. Đổi màu bút và cả vùng chọn đang có. |
| Alt+L | Bật đoạn thẳng; bấm lại trả về bút tự do. |
| Alt+V | Bật mũi tên vectơ; bấm lại trả về bút tự do. Có thể đổi trực tiếp từ đoạn thẳng sang vectơ. |
| Alt+S | Chèn sóng sin một chu kỳ. |
| Alt+Shift+S | Chèn sóng sin hai chu kỳ. |

Sóng sin nằm trong phần trang hiện đang thấy, mặc định rộng tối đa 360 pt, biên độ tối đa 50 pt và tự thu nhỏ theo khung nhìn khi zoom/cuộn. Mỗi sóng là **một nét liền**, lấy màu/bề dày bút hiện tại, được chọn sẵn bằng công cụ chọn vùng để dời và kéo giãn. Chuột/bút kéo các tay nắm để điều chỉnh biên độ/chu kỳ hiển thị; Ctrl+Z bỏ cả nét trong một bước. Chèn nhiều sóng liên tiếp đặt tại cùng vị trí ban đầu: kéo nét vừa chèn sang vị trí cần dùng.

Nút thân bút vẫn Pen → Eraser → Hand → Pen. Shift+Y/Ctrl+Y chia đề PDF và giấy nháp, Alt+` đổi số cột; Ctrl+` hiện/ẩn chữ phím tắt dưới nút công cụ, Ctrl+Shift+C lấy ảnh vùng chọn, cùng xử lý PDF quá cỡ được giữ lại.

## Kiểm chứng

- `ctest --test-dir /home/phuc/xpp-src/build --output-on-failure` — gồm test C++ cho hình học/bắt điểm/thước và test Lua khi plugin đã được cài.
- `texlua /home/phuc/xpp-src/tests/test-vk640-erase.lua` — hợp đồng plugin, phím tắt, sóng sin, màu và chống lặp phím.
- `bash /home/phuc/xpp-src/tests/integration/run-tich-hop.sh` — 32 kiểm tra hồi quy PDF/ảnh/giấy nháp.
- `bash /home/phuc/xpp-src/tests/check-protractor.sh` — hình học thước trên ứng dụng thật.
- `bash /home/phuc/xpp-src/tests/check-protractor-input.sh` — phím dịch/xoay/phóng thước qua XTEST.
- `bash /home/phuc/xpp-src/tests/integration/check-shortcut-hints.sh` — gửi Ctrl+` và Alt+` thật, kiểm tra hiện/ẩn chữ, phục hồi giao diện và bố cục độc lập.
- `bash /home/phuc/xpp-src/tests/integration/check-teaching-tools.sh` — callback Lua thật, màu vùng chọn, sóng sin, hoàn tác/làm lại và chọn sẵn.

Các test GUI chạy trong màn hình/bus riêng, chỉ đóng đúng tiến trình test. Bộ test tương tác chọn cửa sổ đang hiển thị thay cho cửa sổ nền 10×10 của GTK, đo riêng khung thước và trả mã lỗi khi có kiểm tra thất bại.

Chỉ cài bằng `cmake --install /home/phuc/xpp-src/build`, với `CMAKE_INSTALL_PREFIX=/home/phuc/.local`. Không ghi vào `/usr/bin/xournalpp`.

## Đồ thị, tiếp tuyến và bảng — CHẶNG 1

Bấm nút **máy tính** đầu cột công cụ thứ hai hoặc **Alt+G** để mở bảng công cụ có xem trước. Có hai hàm riêng màu/bật tắt, tham số m, khung chia thang và π, tiếp tuyến tại x₀, bảng nhập tay và tự sinh giới hạn. Hình chèn vừa vùng nhìn, chọn sẵn và undo/redo cả cụm một lần. Nhớ mẫu sau khi chèn; hình là vector tĩnh.

Đọc [hướng dẫn đầy đủ và giới hạn](CHANG-1-HUONG-DAN.md), [kết quả kiểm chứng](CHANG-1-NGHIEM-THU.md). Hồi quy mới bổ sung `bash tests/integration/check-math-tools.sh` và CTest `test-teaching-math` (79 kiểm tra sau lượt hoàn thiện). Bảng có nhập từng ô bằng bút, thêm/bớt cột, nút đổi dấu và thanh ký hiệu. Xem [lượt rà soát bổ sung](CHANG-1-RA-SOAT.md). Các chặng sau chưa triển khai.
