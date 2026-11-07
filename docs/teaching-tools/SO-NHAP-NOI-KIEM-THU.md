# Kiểm thử sổ nháp nổi native

Kiểm thử ngày 03–04/11/2026 trên Ubuntu, GTK3 và X11/Xvfb với Intel Core i5-14400F. Các số dưới đây đến từ ứng dụng C++ và bộ kiểm tra liên kết trực tiếp với core. Kết quả nghiên cứu Python được lưu ở tài liệu nghiên cứu riêng.

## Tái lập

```sh
cmake -S /home/phuc/xpp-src -B /home/phuc/xpp-src/build -DCMAKE_INSTALL_PREFIX=/home/phuc/.local
cmake --build /home/phuc/xpp-src/build -j 4
ctest --test-dir /home/phuc/xpp-src/build --output-on-failure
cmake --install /home/phuc/xpp-src/build
/home/phuc/xpp-src/build/tests/test-floating-notebook /home/phuc/xpp-src/output/floating-notes-native/oracle-new
bash /home/phuc/xpp-src/tests/integration/check-floating-notes.sh
```

GUI dùng bản cá nhân với display, bus, cấu hình và cache riêng. Harness dùng Cua Driver với ảnh và cây trợ năng trước/sau thao tác. GAction probe đọc inventory và gọi các đường mở file, export, công cụ Toán của sản phẩm. Openbox của bộ thử nằm trong `build/floating-notes-research/wm-root`. Native oracle sinh toàn bộ fixture `.xopp` mới khi chạy. Các kịch bản bổ sung chọn qua `FLOATING_NOTES_STAGE` trong [script GUI](../../tests/integration/floating-notes-scenario.py). Kịch bản `ocr` dùng `output/pdf/pdf-formula-fixtures/unicode-text.pdf` của bộ thử PDF hiện có.

Bản sao và mốc trước lượt nằm trong `build/floating-notes-native/backup/`, `baseline.diff`, `baseline-status.txt`, `branch.txt` và `before-hashes.json`. Baseline CTest **14/14**. CMake dùng prefix `/home/phuc/.local`. Hash build, binary cài, bản hệ thống, plugin và toolbar nằm trong `build/floating-notes-native/final-manifest.json`. Các lỗi và log từng lượt được ghi riêng. [Nhật ký lỗi](SO-NHAP-NOI-NHAT-KY-LOI.md)

## Dữ liệu và số đo

Native oracle **317 kiểm tra, 0 lỗi**. Fixture có **100 tờ, 1.000 Stroke có áp lực, 100.000 điểm, 100 Text, 100 TexImage PDF và 10 ảnh gốc**. Mỗi tờ rộng 1200 pt, cao 1680 pt. Oracle so ID, nguồn, tên UTF-8, viewport và từng loại phần tử sau lưu/mở. Ca trang thường vẫn kiểm chuẩn hóa kích thước. Oracle cũng thử clone, di chuyển file, xóa trang nguồn, tệp bị phiên khác đổi, đích ghi lỗi và phân đoạn PDF qua công thức. [Log oracle](../../output/floating-notes-native/final-3/full/oracle.log)

Trong lượt `final-3/full`, lưu/mở 2 tờ mất khoảng **2,50 / 16,87 ms**. Với 100 tờ là **112,15 / 46,66 ms**, đỉnh RSS tiến trình oracle **94.616 KiB**. Đây là thời gian SaveHandler/LoadHandler trong tiến trình test, gồm chi phí khởi tạo ở lần đọc đầu. `metrics.jsonl` của GUI lưu RSS theo bước. Kho giới hạn 64 thumbnail và 64 yêu cầu chờ, tối đa 6 editor đang giữ, 64 đoạn giấy trên mỗi tờ. GUI đạt đỉnh RSS 231.036 KiB trong lượt `final-3/full`. Khoảng từ ảnh trước kéo tới ảnh xác nhận nét đầu là 2.332 ms. Nét trên bản nhân đôi của sổ 101 tờ là 2.453 ms. Hai số gồm thao tác kéo 350 ms, gửi input và chụp trạng thái của Cua/Xvfb. Chưa đo độ trễ từ đầu bút tới pixel.

## Ma trận kiểm chứng

| Ca trong prompt | Bằng chứng và phạm vi |
|---|---|
| 1. Phím thật | `full` gọi/cất và tạo tờ đúng ID. `interaction` chuyển tờ khi hộp công thức đang mở. Chưa đo giữ phím autorepeat trên phần cứng. |
| 2. Tờ giữ khi đổi thao tác | `full`, `interaction`, `tools-1` kiểm rời nét, chạm trang chính, đổi công cụ và giữ nội dung. Các suite cũ kiểm điều hướng PDF và bố cục cặp trang. |
| 3. Hai tờ, đích viết, undo | `full` ghi hai tờ độc lập, đưa đúng tờ lên trước và hoàn tác đúng đích. `tools-1` kiểm tẩy và highlighter với trang chính sạch. |
| 4. Dời/resize, hình học mực | `interaction` so tọa độ, độ dày và áp lực qua chữ ký nét native sau kéo tiêu đề, resize và zoom. Có ảnh GUI DPI 2×. |
| 5. Hand, cuộn, zoom | `interaction` dùng chu kỳ Pen/Eraser/Hand thực và so cuộn trang chính. `full` viết sau cuộn và so vị trí của tờ ẩn sau mở. |
| 6. Thêm giấy và nội dung ngoài khung | `full` tăng lên 1680 pt và lưu/mở. Oracle xuất 200 trang từ 100 tờ. Ca UI export kiểm sổ đã sửa. |
| 7. Kết thúc nét | `interaction` kiểm kéo vượt mép, nhả ngoài và viết nét tiếp. `held-stroke-1` chưa xác nhận ẩn giữa nét vì lệnh giữ nút của driver không tạo nét đang vẽ trong ảnh. Chưa kiểm proximity/mất grab vật lý. |
| 8. Tạo, cất, xóa, phục hồi | `full` giữ hai tờ, nhân bản, xóa nội dung rồi undo khôi phục cả chữ/công thức, xóa tờ rồi undo. |
| 9. Dọn nháp cũ | `full` chạy Ctrl+U khi tờ nổi active, giữ công thức và tờ nguồn. Ctrl+Shift+M giữ nội dung mọi tờ nổi. Bộ VK640 Lua kiểm hợp đồng selection và giấy nháp cũ. |
| 10. Clipboard và chuyển vector | `full` chép cụm sang trang chính, undo một bước. Copy/cut/paste giữa hai tờ giữ Stroke/Text/TexImage và giữ bản gốc. |
| 11. Công cụ Toán và nguồn PDF | `interaction` chèn đồ thị, TexImage và vectơ qua hộp thoại thật. `ocr` nhận dạng vùng PDF có lớp chữ, thử đổi/ẩn đích và chèn lại vào đúng tờ. Bộ cũ kiểm OCR ảnh quét, cắt ảnh PDF, đường thẳng và hợp lực trên trang chính. Chưa thử mọi tổ hợp đó trong tờ nổi. |
| 12. Lưu và đóng | `full` lưu tài liệu chưa đặt tên có tờ ẩn, mở và đối chiếu từng tờ. `close` kiểm tự lưu phiên của bản custom khi thoát bình thường. Dirty per-note reset sau Save As. |
| 13. Di chuyển và nguồn trang | `portable` mở bản `.xopp` chuyển thư mục và đổi tên trong profile riêng. Oracle kiểm nguồn khi chèn/xóa trang và hai Document phát hiện file đổi. Chưa chạy hai cửa sổ cùng ghi đồng thời qua UI. |
| 14. Autosave và recovery | `recovery` chờ file autosave ghi xong rồi SIGKILL PID test. `recovered` mở tệp phục hồi trong profile mới. `session-auto` chạy đường khởi động đọc session.ini. Hai ca SIGABRT kiểm tệp khẩn cấp riêng và đường Restore trên GUI. Oracle kiểm lỗi thay thế đích, giữ bản cũ. Chưa thử hết ổ thật hoặc lỗi I/O phần cứng. |
| 15. Xuất và in | `full/main.pdf` có 1 trang chính. Export riêng giữ nét vector, font nhúng và các ảnh gốc. Export giao diện tạo 202 trang. Render kiểm trực quan thấy tử và mẫu còn cùng trang sau sửa điểm chia. Chưa chạy máy in vật lý. |
| 16. Bố cục và DPI | Theme sáng/tối, khung 960×700 pixel tại 1×/2× và khung lớn được kiểm ảnh. `layout-2` kiểm F11 và Alt+dấu huyền bằng phím thật, đối chiếu ID và chữ ký nét của hai tờ trước/sau. Chưa đo chuyển giữa các màn hình vật lý. |
| 17. Dữ liệu lớn | Oracle 2/100 tờ và số đo trên. `metrics.jsonl` ghi RSS GUI và các chu kỳ cuộn kho. Chưa đo latency thiết bị. |
| 18. Kho 100 tờ | `full` mở 100 tờ, tìm và mở đúng ID, đổi tên/ghim/dirty. Các ca theme kiểm cuộn và số nút thẻ đang bind. Chưa so pixel thumbnail sau mọi tổ hợp công cụ. |
| 19. Thu về thẻ, clone, undo | `full` cất/mở, giữ scroll, clone độc lập, sửa/xóa/undo tờ và lưu lại. Oracle so ID và nội dung từng tờ. |
| 20. Ảnh kho, theme, cửa sổ nhỏ | Xem `final-3/{light,dark,dark-2x}`. Thanh tạo/cất nằm ngoài phần cuộn của kho. Thẻ dùng tên trợ năng và mở bằng chạm. |

Bằng chứng gồm [tài liệu 101 tờ sau sửa](../../output/floating-notes-native/final-3/full/evidence-100.xopp), [PDF chính](../../output/floating-notes-native/final-3/full/main.pdf), [PDF sổ từ giao diện](../../output/floating-notes-native/final-3/export/ui-notebook.pdf) và [bản autosave phục hồi](../../output/floating-notes-native/final-3/recovery/recovery-before-kill.xopp). PDF kiểm thêm bằng `pdfinfo`, `pdffonts`, `pdfimages` và Poppler render. Bản Xournal++ hệ thống xuất fixture 2 tờ thành 3 trang thường trong profile riêng. [PDF kiểm tương thích](../../output/floating-notes-native/compatibility/stock-reader.pdf)

## Kết quả bàn giao

Lượt `final-3` đạt **13/13 kịch bản GUI**, gồm 32 kiểm tra trong luồng chính, thao tác giấy, theme, OCR, xuất sổ, Save As và phục hồi. Luồng chính chạy lại trên build 42 cũng đạt 32 kiểm tra. `tools-1` kiểm tẩy và highlighter. `layout-2` kiểm fullscreen và hai cột. Hai lần dừng test bằng SIGABRT tạo hai bản khẩn cấp riêng. Build 43 được kiểm lại bằng CTest **15/15** và `emergency-final/restore-final` **3/3**, gồm giữ nguyên hai tệp khẩn cấp sau Restore. [Kết quả GUI](../../output/floating-notes-native/final-3/results.json) · [Kết quả Restore cuối](../../output/floating-notes-native/emergency-final/restore-final/result.json)

Bản cài cuối khớp SHA-256 với binary build. `/usr/bin/xournalpp` có cùng SHA-256 với mốc trước lượt. Hash, diff riêng của lượt và các đường dẫn log được ghi trong manifest. [Manifest](../../build/floating-notes-native/final-manifest.json) · [Diff của lượt](../../build/floating-notes-native/native-task.diff)

## Hồi quy

Bộ lệnh theo đúng thứ tự trong prompt. **11/11 lệnh đạt** trên build 42. Mỗi lệnh có mã thoát, thời gian và log riêng. Thay đổi C++ sau lượt này chỉ nằm ở callback Restore khi mở tệp khẩn cấp. CTest và ca Restore chạy lại trên build 43. [Kết quả hồi quy cuối](../../build/floating-notes-native/regressions-final-2/results.json)

1. `ctest --test-dir /home/phuc/xpp-src/build --output-on-failure`
2. `bash /home/phuc/xpp-src/tests/integration/run-tich-hop.sh`
3. `bash /home/phuc/xpp-src/tests/check-protractor.sh`
4. `bash /home/phuc/xpp-src/tests/check-protractor-input.sh`
5. `texlua /home/phuc/xpp-src/tests/test-vk640-erase.lua`
6. `bash /home/phuc/xpp-src/tests/integration/check-teaching-tools.sh`
7. `bash /home/phuc/xpp-src/tests/integration/check-shortcut-hints.sh`
8. `bash /home/phuc/xpp-src/tests/integration/check-math-tools.sh`
9. `bash /home/phuc/xpp-src/tests/integration/check-pdf-formula.sh`
10. `bash /home/phuc/xpp-src/tests/integration/check-pdf-region-shortcuts.sh`
11. `bash /home/phuc/xpp-src/tests/integration/check-coordinate-tools.sh`

Lượt hồi quy đầu giữ trong `build/floating-notes-native/regressions/`. Hai lỗi harness đã được sửa ở phần nhận đúng action snapshot sau khi plugin thêm đăng ký và tránh chép app.log đè lên chính nó. Assertion nghiệp vụ giữ nguyên.

**Phạm vi thiết bị.** Chưa nghiệm thu bằng VK640 vật lý, áp lực/palm rejection thực hoặc Wayland. Fixture áp lực xác nhận đường dữ liệu native. Bảng trên ghi riêng các phần chưa đủ chứng cứ.

## Bổ sung cụm phím tắt

`FLOATING_NOTES_STAGE=shortcuts` chạy phím thật trên bản cài. Sáu kiểm tra xác nhận Ctrl+dấu huyền bật/tắt ba nhãn ở cả toolbar và cụm nút, Alt+K mở/cất kho khi đang ở ô tìm, Alt+N chọn đúng nhóm sau khi tạo tờ mới, và nút Tờ mới dùng cùng đường tạo native. Bộ phím tắt toolbar cũ tiếp tục kiểm cả Ctrl+dấu huyền và Alt+dấu huyền. Luồng chính của sổ nháp chạy lại đủ 32 kiểm tra. CTest đạt 15/15, VK640 Lua đạt 124/124. [Hồi quy sổ nháp](../../output/floating-notes-shortcuts/notebook-regression/result.json) · [Ảnh bật nhãn](../../output/floating-notes-shortcuts/gui-2/005-notebook-shortcuts-on.png) · [Kết quả](../../output/floating-notes-shortcuts/gui-2/result.json)
