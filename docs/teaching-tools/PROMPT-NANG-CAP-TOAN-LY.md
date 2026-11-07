# Prompt triển khai Xournal++ Toán–Vật lý THPT cho VK640

Có thể sao chép toàn bộ từ phần “Vai trò và nhiệm vụ” bên dưới vào agent làm việc trên repo. Mặc định chỉ triển khai chặng 1 đến mức dùng được; các chặng khác là lộ trình, không được báo là đã làm. Để tiếp tục sau khi chặng 1 đã nghiệm thu, thay `CHẶNG_CẦN_TRIỂN_KHAI` bằng một chặng cụ thể: `2A`, `2B`, `3A`, `3B`, `4A`, `4B`, `4C` và yêu cầu đọc trạng thái thực tế trước.

**Điều chỉnh ưu tiên 03/11/2026:** không triển khai toàn bộ 2A ngay. Đợt riêng hiện tại là Vectơ – Đường thẳng – Hợp lực, một phần M08/P01; tài liệu [VECTO-DUONG-THANG-HOP-LUC.md](VECTO-DUONG-THANG-HOP-LUC.md) ghi phạm vi/giới hạn thật. Nguyên hàm–tích phân kế tiếp. Không đánh dấu hoàn thành toàn bộ 2B/4A từ đợt riêng này. Phần CHẶNG=1 bên dưới là prompt lịch sử, không thay yêu cầu ưu tiên mới.

---

## Vai trò và nhiệm vụ

Bạn là kỹ sư C++/GTK/Lua phát triển bản custom Xournal++, đồng thời phải kiểm chứng tính đúng của công cụ minh hoạ Toán–Vật lý THPT Việt Nam. Người dùng dùng bút VEIKK VK640 trên Ubuntu để học, giảng và giải bài, ưu tiên Toán 12, có nhu cầu lớp 10–11 và Vật lí THPT.

CHẶNG_CẦN_TRIỂN_KHAI = 1

Hãy trực tiếp khảo sát, triển khai, build, kiểm thử, sửa lỗi và cài bản cá nhân cho chặng được chọn. Bản nghiên cứu là căn cứ thiết kế; không dừng ở đề xuất, tạo menu rỗng hoặc ảnh minh hoạ. Không tự mở rộng sang mọi chặng trong cùng một lượt.

## 1. Bối cảnh thật và điều cần giữ

- Repo: `/home/phuc/xpp-src`, nhánh dự kiến `feature/protractor-geometry-tools`; kiểm tra nhánh thực tế, không tự đổi hay ghi đè thay đổi chưa commit.
- Build: `/home/phuc/xpp-src/build`.
- Chỉ cài bản người dùng: `/home/phuc/.local/bin/xournalpp`, với prefix `/home/phuc/.local`.
- Tuyệt đối không ghi đè `/usr/bin/xournalpp`.
- Plugin đang dùng: `/home/phuc/.config/xournalpp/plugins/VK640ToolCycle/main.lua`.
- Toolbar cá nhân: `/home/phuc/.config/xournalpp/toolbar.ini`.
- Đọc AGENTS.md áp dụng trong môi trường, cùng:
  - `/home/phuc/xpp-src/docs/teaching-tools/VK640-Toan-Ly.md`
  - `/home/phuc/xpp-src/docs/teaching-tools/NGHIEN-CUU-LO-TRINH-TOAN-LY.md`
- Nếu không có hai file trên, dùng phạm vi và yêu cầu trong prompt này; báo rõ phần tài liệu không truy cập được, không giả vờ đã đọc.

Giữ nguyên các tính năng đang dùng:
1. Nút thân bút Pen → Eraser → Hand → Pen và công cụ bút/tẩy hiện có.
2. Shift+Y/Ctrl+Y tạo nháp kẻ dòng bên cạnh các trang PDF; chức năng dọn mực nháp, ảnh và chữ vẫn đúng phạm vi.
3. **Ctrl+` hiện/ẩn chữ phím tắt dưới nút; Alt+` đổi 1/2 cột. Đây là bố trí mới đã được người dùng chấp nhận.** Không dùng ghi chú cũ để đảo chúng lại.
4. Ctrl+Shift+C lấy ảnh vùng chọn và chuẩn hoá trang PDF quá cỡ.
5. Oxy/Oxyz, các loại hình hộp/chóp/lăng trụ đã có; cạnh khuất, đặt tên đỉnh, nam châm bắt đỉnh/trung điểm/tâm elip.
6. Alt+C đổi màu; Alt+L/Alt+V bật tắt đoạn thẳng/vectơ; Alt+S/Alt+Shift+S chèn sin một/hai chu kỳ; F1 vẫn hoạt động.

Trước khi chỉnh sửa, lưu bản sao các file thực sự cần sửa và plugin/toolbar liên quan. Không reset/restore toàn repo. Không đóng tiến trình Xournal++ người dùng đang chạy để phục vụ test.

## 2. Định hướng chương trình học

Thiết kế ưu tiên tính hữu ích khi giải và giảng bài. Phần Toán phải vượt ra ngoài các hình khối: hàm số, đạo hàm, bảng biến thiên, tích phân, tọa độ, xác suất và thống kê. Các mục dưới đây là đề xuất công cụ, không phải danh sách phần mềm bắt buộc của Bộ GDĐT.

Nguồn đối chiếu:
- Chương trình Toán của Bộ, phần THPT, nhất là trang in 105–110:
  https://lophocnguvan.com/wp-content/uploads/Giao-duc-pho-thong-2018-Mon-Toan.pdf
- Tài liệu chương trình Vật lí của Bộ GDĐT/ĐHSP Hà Nội:
  https://dtbdtx.hnue.edu.vn/Portals/0/Tai%20lieu%20tim%20hieu%20chuong%20trinh%20mon%20Vat%20li_1.pdf
- Cục Quản lý chất lượng công bố đề tham khảo từ năm 2025:
  https://vqa.moet.gov.vn/vi/news/tin-tuc-su-kien/de-thi-tham-khao-ky-thi-tot-nghiep-thpt-tu-nam-2025-159.html
- Đề tham khảo Toán của Bộ, bản lưu tại Đại học CMC:
  https://cmcu.edu.vn/wp-content/uploads/2024/10/De-tham-khao-ki-thi-tot-nghiep-THPT-2025-mon-TOAN-da-nen.pdf

Dùng đề tham khảo để kiểm tra các nhóm thao tác hỗ trợ. Không suy ra từ một đề rằng mọi năm sẽ có cùng tỷ trọng/chủ đề. Nếu viện dẫn thay đổi chương trình hoặc kỳ thi mới hơn, kiểm tra nguồn chính thức và ghi rõ ngày.

## 3. Chặng 1 phải hoàn thành: giải tích và đồ thị

### A. Một cửa vào gọn cho công cụ Toán

- Thêm nút mở bảng công cụ Toán, có các mục đang hoạt động: Đồ thị, Tiếp tuyến, Bảng biến thiên/xét dấu.
- Đủ dễ bấm bằng bút. Có mẫu gần đây, nhớ các tham số nhập gần nhất, không mở chuỗi nhiều hộp thoại.
- Mỗi công cụ có xem trước; nhập sai được báo tại trường tương ứng, không làm mất nội dung trang.
- Ưu tiên cơ chế phím hiện có và chữ gợi nhớ Ctrl+`. Nếu cần phím mới, rà xung đột thực tế bằng appAccels() cùng đăng ký GAction; không chiếm phím đã dùng hay tổ hợp thông dụng khi nhập chữ.
- Chưa triển khai các chặng sau thì chỉ ghi trong tài liệu lộ trình, không tạo nút chết để làm giao diện trông nhiều chức năng.

### B. Khung tọa độ có thang đo

- Có miền x/y, gốc O, tên trục, bước chia, nhãn số nguyên/phân số hoặc bội π.
- Cho chọn hai trục cùng tỉ lệ hoặc khác tỉ lệ, luôn lưu ý phép biến đổi từ tọa độ toán học sang tọa độ trang.
- Nhãn không đè lên nhau hoặc bị cắt mất. Khi khung nhỏ, giảm mật độ nhãn thay vì sửa giá trị của trục.
- Mẫu trục x–y và t–đại lượng để dùng được cho Toán và Vật lí.
- Có thể tạo từ một khung kéo nếu phù hợp kiến trúc repo; tối thiểu chèn vừa phần trang đang thấy qua getVisiblePageRect, không nằm ngoài màn hình khi zoom/cuộn.

### C. Đồ thị nhập công thức, có xử lý miền xác định

- Hỗ trợ các mẫu: bậc hai, bậc ba, (ax+b)/(cx+d), (ax²+bx+c)/(mx+n), sin/cos, exp, ln, sqrt, abs.
- Cho ít nhất hai đường cong cùng khung; mỗi đường có màu và bật/tắt riêng.
- Nhập biểu thức theo cú pháp đã mô tả; có x, số, π, phép toán, các hàm đã hỗ trợ. Tận dụng bộ phân tích biểu thức có sẵn/phụ thuộc hiện hữu nếu thích hợp; không dùng eval/loadstring hoặc chạy lệnh hệ thống từ công thức.
- Có một tham số m dạng ô số/nút tăng giảm trong mẫu, cập nhật preview theo giá trị m hiện tại. Không yêu cầu giải mọi điều kiện tham số một cách tượng trưng.
- Lấy mẫu thích ứng và clipping trong cửa sổ; tách các nhánh và miền không xác định. Không nối xuyên tiệm cận, không vẽ NaN/Infinity, không thêm đường chéo giả giữa các nhánh.
- Với họ phân thức được hỗ trợ, giữ miền loại của biểu thức ban đầu dù rút gọn được. Lỗ thủng không được biến thành tiệm cận đứng giả.
- Có giới hạn số điểm/độ sâu/thời gian tính để biểu thức khó không làm treo GTK. Đặt sai số và giới hạn có căn cứ, kiểm tra ở nhiều mức zoom.
- Không tuyên bố có bộ khảo sát hàm tổng quát nếu chỉ hỗ trợ một số họ hàm.

### D. Tiếp tuyến và điểm đọc đồ thị

- Nhập x0 để tạo điểm M, đường gióng tới hai trục, nhãn tọa độ và tiếp tuyến trong cùng khung đồ thị.
- Dùng đạo hàm trong hệ tọa độ toán học. Với các họ mẫu cần hỗ trợ, ưu tiên đạo hàm giải tích/cây biểu thức nếu phù hợp.
- Nếu dùng đạo hàm số cho phần mở rộng, ghi rõ là xấp xỉ, có sai số/giới hạn; không suy công thức chính xác từ vài điểm ảnh.
- Điểm ngoài miền xác định, điểm không khả vi, tiếp tuyến đứng phải được xử lý rõ; abs(x) tại 0 không có tiếp tuyến duy nhất.
- Cát tuyến giữa hai giá trị x là lựa chọn bổ sung tốt nếu có thể dùng chung phần tính toán.
- Chạm trực tiếp lên đồ thị để chọn x0 có thể làm sau ô nhập số; không được suy tọa độ từ một nét vẽ tay bất kỳ mà không có khung đo đã xác lập.

### E. Bảng xét dấu và bảng biến thiên

- Chế độ nhập tay: thêm/bớt cột, mốc hữu hạn/±∞, nghiệm, điểm loại, dấu +/−/0, phân cách miền và mũi tên tăng/giảm; phân biệt 0 với không xác định.
- Bảng xét dấu có thể dùng cho f, f' hoặc một biểu thức do giáo viên đặt tên.
- Chế độ tự sinh giới hạn cho đa thức đến bậc ba và hai họ phân thức nêu trên, tại giá trị tham số m đã cho. Ghi rõ phạm vi hỗ trợ.
- Xác định miền, nghiệm đạo hàm, dấu trên các khoảng và giới hạn bằng thuật toán được kiểm chứng. Không lấy vài mẫu số để kết luận dấu trên cả khoảng.
- f'(x0)=0 không đồng nghĩa x0 là cực trị; kiểm tra đổi dấu.
- Cho sửa nội dung bảng trước khi chèn; có thể che/hiện hàng đáp án để giảng nếu không làm phình chặng.
- Sinh bảng bằng nét và chữ chỉnh được, chọn cả cụm khi vừa chèn; mọi nét cùng thuộc một bước hoàn tác.

## 4. Hành vi tài liệu và kiến trúc bắt buộc

- Tính toán toán học tách khỏi GTK và Lua, có unit test gọi chính implementation được app dùng.
- Lua dùng để đăng ký UI/gọi API/chèn và chọn hình nếu phù hợp. Repo có addStrokes, addTexts, addSplines, getVisiblePageRect, addToSelection, showToolbarShortcuts; khảo sát giới hạn trước khi dựa vào chúng.
- Nhiều nhánh đồ thị phải là nhiều nét. Một hình minh hoạ có thể gồm nhiều Stroke/Text/vùng tô; không ép thành một Stroke để rồi xuất hiện đoạn nối sai.
- Cả cụm mới chèn được chọn sẵn để dời/thu phóng, nhưng Ctrl+Z phải bỏ cả cụm một lần và Ctrl+Shift+Z khôi phục đúng.
- Nếu API chèn nét và chữ hiện tạo các undo riêng, thêm giao dịch/group undo phù hợp. Không báo đã nhóm nếu thực tế phải undo nhiều lần.
- Không biến hình thành bitmap chỉ để tránh xử lý hình học. Xuất PDF phải rõ, lưu/mở .xopp giữ hình, chữ, màu, nét đứt và vị trí.
- Chặng 1 có thể tạo hình vector tĩnh từ trình sinh. Nhớ mẫu gần nhất để tạo lại. Không cần viết lại định dạng .xopp hoặc hứa “sửa công thức của mọi đồ thị đã lưu” khi chưa có metadata/định danh.
- Phân biệt kéo giãn hình trên giấy với thay đổi miền/đơn vị/hàm số. Nếu muốn sửa tham số sau lưu/mở, thiết kế thêm metadata và kiểm thử save/load, clone, copy-paste, undo và sửa nét trước khi cam kết.
- Chỉ chèn vào đúng trang/lớp đang thao tác. Không tự xóa vùng chọn cũ mà làm mất phần tử, không thay đổi PDF nền, giấy nháp, bố cục cột hay các phím hiện có.

## 5. Bộ ca nghiệm thu tối thiểu của chặng 1

Tạo kiểm tra logic và test GUI thật cho các hành vi cần kiểm chứng; không chỉ test bản sao công thức hoặc sự tồn tại của chuỗi trong source.

1. y=1/x trên [-2,2]: hai nhánh tách, không có cạnh nối qua x=0.
2. y=(x²-1)/(x-1): x=1 vẫn bị loại; lỗ tại (1,2), không có tiệm cận đứng giả.
3. y=(x²+1)/(x-1): nhận x=1 và y=x+1 là các tiệm cận trong phần khảo sát được hỗ trợ.
4. y=x³-3x: cực đại (-1,2), cực tiểu (1,-2); dấu đạo hàm và chiều biến thiên đúng.
5. y=x³: đạo hàm bằng 0 tại 0 nhưng không có cực trị tại đó.
6. y=x² tại x0=1: tiếp tuyến y=2x-1, cả khi hai trục hiển thị khác tỉ lệ.
7. ln x: không vẽ x≤0; sqrt x có giá trị tại x=0; abs x tại 0 không bị gán đạo hàm tùy ý.
8. Mẫu sin/cos trên miền chứa π: vị trí mốc và giao trục đúng; mặc định radian, nếu thêm độ thì chuyển đổi nhất quán.
9. Hai đường cong cùng khung, m thay đổi: trục và các đường vẫn cùng hệ đo, preview không để lại rác, không thay đổi hình khác trên trang.
10. Hàm hằng, hệ số cao nhất bằng 0, khoảng x/y không hợp lệ, hệ số rất lớn/nhỏ, mẫu bằng 0: không crash hoặc sinh dữ liệu không hữu hạn; suy biến được thông báo hoặc chuyển đúng họ.
11. Chèn tại trang đang zoom/cuộn: nhìn thấy cả cụm trong vùng chèn, nhãn không tràn hoặc chồng sai.
12. Một undo/redo cho cả hình và cả bảng; hủy preview không thêm phần tử.
13. Save/reopen .xopp và xuất PDF: nội dung không mất, không chuyển thành nét nối nhầm.
14. Bật/tắt Ctrl+` vẫn hiện chữ dưới nút mới; Alt+` và Shift+Y/Ctrl+Y giữ đúng hành vi cũ.
15. Bộ lấy mẫu gặp đầu vào khó vẫn trả điều khiển cho UI trong giới hạn đã công bố.

Nếu mô tả một kết quả là “chính xác”, phải có phép tính/kiểm chứng phù hợp. Kết quả số phải ghi xấp xỉ và sai số hoặc dung sai.

## 6. Lộ trình sau chặng 1 — chỉ triển khai khi chọn đúng chặng

### Chặng 2A: tích phân, miền nghiệm và dựng hình phụ

- Tô miền giữa f,g theo a,b; phân biệt ∫(f-g) với ∫|f-g|, tách tại giao điểm.
- Minh hoạ quay quanh Ox bằng đĩa/vành khăn với bán kính ngoài/trong xác định đúng; hình biểu diễn tĩnh và công thức.
- Trục số, khoảng rỗng/đặc, vô hạn, giao/hợp; miền nửa mặt phẳng và giao các bất phương trình tuyến tính.
- Dựng song song/vuông góc/chân chiếu, chia đoạn theo tỉ số, giao điểm; dấu vuông góc/cung góc/vạch bằng nhau/nhãn.
- Oracle: giữa x và x² trên [0,1] có diện tích 1/6, quay Ox có thể tích 2π/15; x-1 trên [0,2] có tích phân 0 nhưng diện tích 1.

### Chặng 2B: mô hình Oxyz, khối và thiết diện

- Điểm, vectơ, đường tham số, mặt phẳng, cầu có tọa độ 3D thật; chân chiếu, góc, khoảng cách tính từ dữ liệu 3D.
- Trụ, nón, cầu; hình khuất, chân đường cao, pháp tuyến; lựa chọn góc nhìn và nhãn hợp lý.
- Mặt cắt đa diện lồi bởi ba điểm không thẳng hàng hoặc phương trình mặt phẳng; sắp thứ tự đa giác cắt đúng, không nối tùy ý.
- Tuyệt đối không suy tọa độ 3D, góc thật hoặc giao tuyến từ riêng ảnh chiếu 2D của nét có sẵn.
- Mẫu bài tối ưu hình học có tham số và giả thiết rõ, như tấm bìa cắt góc làm hộp.
- Oracle: hai đường (t,0,0) và (0,s,1) chéo nhau, khoảng cách 1; hình lập phương [0,1]³ cắt x+y+z=1.5 thành lục giác; mặt phẳng trùng mặt/qua cạnh/không giao được xử lý đúng.

### Chặng 3A: xác suất trực quan

- Sơ đồ cây, bảng 2×2 dùng chung dữ liệu; xác suất nhánh, xác suất đường đi, toàn phần, Bayes; che/hiện kết quả, tô đường đang xét.
- Venn hai/ba tập, tô giao/hợp/bù/hiệu; không hứa diện tích hình tỉ lệ với xác suất.
- Không tự giả định biến cố độc lập; phân biệt P(A|B) và P(B|A), kiểm tra mẫu số 0 và tổng xác suất.
- Oracle: P(B)=0.01, P(A|B)=0.9, P(A|không B)=0.05 thì P(B|A)=2/13.

### Chặng 3B: thống kê và dữ liệu thực nghiệm

- Bảng khoảng–tần số, histogram, trung bình/phương sai/độ lệch chuẩn theo quy ước phổ thông, tứ phân vị và boxplot khi dữ liệu hỗ trợ.
- Phân biệt dữ liệu gốc với ước lượng ghép nhóm; histogram khoảng không đều phải xử lý mật độ đúng.
- Cặp (x,y), biểu đồ phân tán, khớp tuyến tính và đơn vị; thanh sai số chỉ khi có dữ liệu sai số.
- Oracle: nhóm [0,10),[10,20) với tần số 2,2 cho trung bình 10, phương sai 25 theo mẫu số n và trung điểm; nhân mọi tần số cùng một hệ số không làm đổi các đại lượng này.

### Chặng 4A: cơ học, dao động và sóng

- Tổng/phân tích vectơ, sơ đồ lực và mặt phẳng nghiêng, lò xo/con lắc; hướng ma sát theo điều kiện người dùng xác định.
- Liên kết đường tròn pha với x–v–a dùng cùng A,ω,φ và cùng thời điểm. Khung hình tĩnh trước; play/pause nếu đã có cơ chế overlay phù hợp.
- Sóng truyền/sóng dừng, phân biệt ảnh theo vị trí với đồ thị theo thời gian; nút/bụng theo điều kiện biên.
- Oracle: lực 10N góc 30° có thành phần 5√3N và 5N; a=-ω²x; dây hai đầu cố định L=1 mode 2 có nút 0,0.5,1 và bụng 0.25,0.75.

### Chặng 4B: nhiệt, khí, từ trường và phóng xạ

- Các đồ thị p–V, V–T, p–T, mốc trạng thái, quá trình đẳng nhiệt/áp/tích và chu trình; đơn vị và quy ước dấu công.
- Đồ thị đun nóng/chuyển thể; trường điện/từ, ⊙/⊗, khung dây, chiều I,B,F; tính trong hệ vật lý, không theo góc hình chiếu.
- Đồ thị N,H,m theo thời gian và mốc bán rã; phân biệt lượng còn lại/đã phân rã.
- Oracle: đẳng nhiệt V tăng gấp đôi thì p giảm một nửa; dùng K khi áp dụng phương trình khí; sau 3 chu kỳ bán rã còn N0/8.

### Chặng 4C: mạch điện, giản đồ pha và mô phỏng mở rộng

- Kho linh kiện có điểm nối; phân biệt dây giao nhau với nút nối; mạch DC nối tiếp/song song khai báo đúng cấu trúc.
- RLC điều hoà, giản đồ pha, cộng hưởng là phần mở rộng/chuyên đề theo nhu cầu, có đơn vị và điều kiện áp dụng.
- Giao thoa hai nguồn, mô phỏng có thời gian chỉ làm khi kiểm chứng được biên độ, pha và điều kiện biên; hỗ trợ chèn khung hình tĩnh vào bài.

## 7. Kiểm thử hồi quy và cài đặt

Trước khi làm, ghi lại kết quả baseline thực tế. Sau khi thay đổi mã, build và chạy đầy đủ:

```
ctest --test-dir /home/phuc/xpp-src/build --output-on-failure
bash /home/phuc/xpp-src/tests/integration/run-tich-hop.sh
bash /home/phuc/xpp-src/tests/check-protractor.sh
bash /home/phuc/xpp-src/tests/check-protractor-input.sh
texlua /home/phuc/xpp-src/tests/test-vk640-erase.lua
bash /home/phuc/xpp-src/tests/integration/check-teaching-tools.sh
bash /home/phuc/xpp-src/tests/integration/check-shortcut-hints.sh
```

- Mốc gần nhất: CTest 9/9, Lua 120 PASS, tích hợp 32/32; có thể tăng số test khi thêm chức năng, không bỏ test cũ để giữ số đẹp.
- Thêm app.registerUi thì cập nhật EXPECTED trong test Lua và kiểm tra phím trùng.
- GUI test dùng màn hình/bus/config riêng; chỉ đóng đúng PID test. Không dùng pkill quét tất cả Xournal++.
- Xác minh prefix rồi mới `cmake --install /home/phuc/xpp-src/build`. Test tích hợp chạy trên bản đã cài thì phải bảo đảm đúng binary mới.
- Chỉ báo PASS sau khi thực sự chạy thành công; lưu log, ảnh GUI và file .xopp/PDF kiểm chứng của tính năng mới. Nếu thiếu môi trường/dependency hoặc lỗi có sẵn, chỉ rõ bằng chứng, không che bằng mock hay bỏ qua im lặng.

## 8. Kết quả bàn giao

1. Hoàn thành đúng chặng được chọn, có đường vào UI thật và thao tác bằng bút dễ dùng.
2. Mô tả ngắn thay đổi, cách gọi, họ hàm/tình huống hỗ trợ, giới hạn còn lại.
3. Nêu những test đã chạy và kết quả, đường dẫn log/ảnh/tài liệu kiểm chứng.
4. Cập nhật hướng dẫn sử dụng và danh sách trạng thái từng chặng: chưa làm/đang làm/đã nghiệm thu. Các chặng chưa làm không được ghi là tính năng hiện có.
5. Giữ nguyên thay đổi có trước của người dùng; không tự commit/push hay ghi đè binary hệ thống.

Bắt đầu bằng kiểm tra repo và baseline, sau đó triển khai chặng được chọn đến khi các tiêu chí nghiệm thu của chặng và bộ hồi quy đều đạt.
