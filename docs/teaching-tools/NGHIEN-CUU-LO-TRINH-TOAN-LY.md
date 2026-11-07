# Đề xuất công cụ Toán–Vật lý THPT cho Xournal++ và VK640

Ngày nghiên cứu: 02/11/2026. Mục đích: chọn công cụ minh hoạ và dựng hình phục vụ giảng/giải bài bằng bút, ưu tiên Toán 12; có phần dùng cho lớp 10–11. Đây là tài liệu nghiên cứu và lộ trình. Cập nhật 02/11/2026: CHẶNG 1 (M01–M03, M04 nhập tay và tự sinh giới hạn) đã triển khai và nghiệm thu trên bản cá nhân, có [rà soát bổ sung chặng 1](CHANG-1-RA-SOAT.md); các mục còn lại theo trạng thái lộ trình bên dưới; từ 03/11 có đợt ưu tiên riêng một phần M08/P01. Xem [hướng dẫn chặng 1](CHANG-1-HUONG-DAN.md) và [biên bản kiểm chứng](CHANG-1-NGHIEM-THU.md).

Ngày 04/11/2026, đợt riêng **Sổ nháp nổi** đã có bản native trong prefix cá nhân. Phạm vi là khung viết, kho ảnh, lưu/mở `.xopp`, chuyển nội dung vector và xuất sổ. Các mục 2A, 2B, 4A giữ trạng thái trong bảng lộ trình. Phạm vi nghiệm thu và ca thiết bị còn thiếu nằm trong tài liệu kiểm thử. [Hướng dẫn Sổ nháp nổi](SO-NHAP-NOI-HUONG-DAN.md) · [Kiểm thử](SO-NHAP-NOI-KIEM-THU.md)

## Cơ sở và giới hạn của nghiên cứu

Đã đối chiếu chương trình môn Toán GDPT 2018, tài liệu tìm hiểu chương trình Vật lí của Bộ GDĐT/ĐHSP Hà Nội, đề tham khảo Toán–Vật lí do Bộ công bố cho kỳ thi từ năm 2025, và mã nguồn/bản hướng dẫn của bản custom đang có. Đề tham khảo được dùng để kiểm tra loại thao tác cần hỗ trợ, không dùng một đề để suy ra tỷ trọng mọi kỳ thi hoặc dự báo đề sau này.

- Toán: cần mở rộng ra ba mạch đại số–giải tích, hình học–đo lường, thống kê–xác suất. Bản custom hiện tại tập trung vào vẽ hình nên bỏ trống nhiều thao tác về hàm số, bảng, miền và dữ liệu.
- Vật lí: ngoài dao động, sóng và điện cần có nhiệt, khí lí tưởng, từ trường, hạt nhân. Cần phân biệt nội dung phổ thông với chuyên đề lựa chọn; công cụ RLC nâng cao nằm trong nhóm mở rộng.
- Các nhóm chức năng, thứ tự ưu tiên và cách tương tác bên dưới là đề xuất thiết kế của tác giả bản nghiên cứu, không phải yêu cầu phần mềm do Bộ GDĐT quy định.

## Nguồn đã đọc

1. **Chương trình môn Toán của Bộ GDĐT**, bản PDF của Bộ được lưu lại trên lophocnguvan.com. Đặc biệt trang in 105–110: hàm số/đạo hàm, tích phân, Oxyz, thống kê ghép nhóm, xác suất có điều kiện; trang in 98–104 cho nền tảng lớp 11 và chuyên đề.
   [PDF chương trình Toán](https://lophocnguvan.com/wp-content/uploads/Giao-duc-pho-thong-2018-Mon-Toan.pdf)
2. **Bộ GDĐT – Trường ĐHSP Hà Nội: Tài liệu tìm hiểu chương trình môn Vật lí (2019)**, trang in 8–10 về phân bố nội dung; trang 15 trở đi có ví dụ dạy khí lí tưởng.
   [Tài liệu trên website ĐHSP Hà Nội](https://dtbdtx.hnue.edu.vn/Portals/0/Tai%20lieu%20tim%20hieu%20chuong%20trinh%20mon%20Vat%20li_1.pdf)
3. **Cục Quản lý chất lượng – Bộ GDĐT: Đề thi tham khảo kỳ thi tốt nghiệp THPT từ năm 2025**, công bố 18/10/2024. Dùng làm điểm xuất phát xác định tài liệu của Bộ.
   [Trang công bố chính thức](https://vqa.moet.gov.vn/vi/news/tin-tuc-su-kien/de-thi-tham-khao-ky-thi-tot-nghiep-thpt-tu-nam-2025-159.html)
4. **Đề tham khảo Toán của Bộ**, bản PDF kèm đáp án lưu trên website Đại học CMC. Đã xem các nhóm câu về đồ thị, số liệu ghép nhóm, tọa độ, chuyển động, xác suất và tối ưu.
   [PDF đề Toán](https://cmcu.edu.vn/wp-content/uploads/2024/10/De-tham-khao-ki-thi-tot-nghiep-THPT-2025-mon-TOAN-da-nen.pdf)
5. **Đề tham khảo Vật lí của Bộ**, bản PDF lưu trên Báo Giáo dục và Thời đại. Đã xem các nhóm câu về nhiệt/khí, cảm ứng điện từ, phóng xạ và đọc số liệu.
   [PDF đề Vật lí](https://cdn.giaoducthoidai.vn/files/54e690acdcedeca283e63e944004f0bf354ee36cfcbe196365fe6f6d9d020b6d6e34fb0c3abbc7bd294a23a683a6d93f/03-vatli-2715-3045.pdf)
6. **NXBGD Việt Nam: Thông báo sử dụng SGK năm học 2026–2027**. Tham khảo bối cảnh cập nhật sách, không thay thế việc kiểm tra chương trình từng môn.
   [Thông báo của NXBGD](https://nxbgd.vn/bai-viet/nha-xuat-ban-giao-duc-viet-nam-thong-bao-ve-viec-su-dung-sach-giao-khoa-nam-hoc-2026-2027)

## Những gì đã có — không làm lại

Oxy/Oxyz có mũi tên; hình hộp, chóp tứ giác, chóp tam giác, chóp có SA vuông góc đáy, lăng trụ tam giác; cạnh khuất nét đứt; đặt tên đỉnh; bắt đỉnh/trung điểm/tâm elip; sóng sin tĩnh 1/2 chu kỳ; màu/kiểu nét; dụng cụ hình học; chia đề PDF và nháp; cắt ảnh; chữ gợi nhớ dưới nút công cụ.

Một hình vẽ Oxyz hiện có là hình biểu diễn 2D. Nó chưa đồng nghĩa với mô hình tọa độ 3D có thể tính giao tuyến, chân đường vuông góc hoặc thiết diện. Phải kiểm tra mã thực tế trước khi dựa vào nó để tính toán.

## Danh mục chức năng đề xuất

### Nhóm A — Giải tích và đồ thị: giá trị sử dụng cao nhất, nên làm trước

**M01. Khung đồ thị có thang đo và nhãn.** Nhãn x,y,t,v,p,V; gốc O; chia số nguyên, phân số hoặc bội π; chế độ hai trục cùng tỉ lệ khi cần đo hình. Chỉ ẩn chữ/tick khi chật, giữ nguyên giá trị toán học. Giúp dựng nhanh đồ thị hàm, chuyển động và quá trình khí.

**M02. Vẽ hàm số từ công thức.** Một hoặc nhiều hàm trong một hệ trục, miền x và cửa sổ y rõ ràng; mẫu bậc hai, bậc ba, phân thức bậc nhất/bậc nhất và bậc hai/bậc nhất, mũ, log, sin/cos. Cho tham số m thay đổi bằng ô số/nút tăng giảm. Ưu tiên thư viện biểu thức có sẵn trong repo nếu phù hợp. Lấy mẫu có giới hạn, tách miền xác định, không nối qua cực hoặc lỗ thủng.

**M03. Tiếp tuyến, cát tuyến và điểm đọc đồ thị.** Chọn x0 hoặc chạm vào đường cong trong khung đã tạo để đặt điểm, đường gióng, tọa độ và tiếp tuyến. Không suy đạo hàm từ độ dốc màn hình khi hai trục khác tỉ lệ. Tiếp tuyến đứng/điểm không khả vi phải được phân biệt với độ dốc hữu hạn.

**M04. Bảng xét dấu và bảng biến thiên.** Bản nhập tay có ô x, dấu f'/f, nghiệm, điểm loại, giới hạn và mũi tên tăng/giảm. Tự sinh cho các họ hàm có thuật toán đã kiểm chứng; ví dụ đa thức đến bậc ba và hai họ phân thức ở M02. Hàm ngoài phạm vi hỗ trợ vẫn nhập tay được, không suy dấu toàn khoảng từ vài mẫu số. Có nút che/hiện từng hàng để giảng.

**M05. Miền tích phân, diện tích và khối tròn xoay.** Chọn a,b, f,g; tô nhạt miền, vẽ đường gióng và hiển thị công thức. Phân biệt tích phân có dấu với diện tích; tách tại giao điểm. Giai đoạn đầu chỉ hỗ trợ quay quanh Ox cho miền đã xác định được bán kính ngoài/trong, kèm thiết diện đĩa/vành khăn và hình biểu diễn tĩnh. Phần 3D tương tác làm sau.

**M06. Miền nghiệm và trục số.** Đầu mút rỗng/đặc, khoảng vô hạn, giao/hợp khoảng; nửa mặt phẳng và giao các bất phương trình tuyến tính hai ẩn trong một cửa sổ hữu hạn. Hữu ích cho bất phương trình, miền xác định, bài toán tối ưu và điều kiện tham số.

### Nhóm B — Hình học có dựng và đo đúng

**M07. Dựng hình phụ bằng bút.** Chọn một điểm và một đường để dựng song song/vuông góc/chân chiếu; trung điểm và chia đoạn theo tỉ số; giao hai đường; ký hiệu vuông góc, cung góc, vạch bằng nhau, dấu song song; nhãn độ dài/góc. Giữ phân biệt đoạn, đường thẳng và tia. Cho phép nhập giá trị đo hoặc chỉ ghi nhãn, không coi hình phác tay là dữ kiện chính xác.

**M08. Oxyz có dữ liệu tọa độ.** Nhập/chọn điểm, vectơ, đường tham số, mặt phẳng và mặt cầu; dựng pháp tuyến, hình chiếu, đoạn khoảng cách và góc. Tính bằng tọa độ gốc 3D rồi mới chiếu ra giấy. Hai đường chéo nhau có thể trông cắt nhau trên màn hình: không được kết luận giao nhau từ hình biểu diễn.

**M09. Khối và thiết diện.** Thêm trụ, nón, cầu với cạnh/đường khuất đúng. Với đa diện lồi có mô hình 3D, dựng mặt cắt từ ba điểm không thẳng hàng hoặc phương trình mặt phẳng. Thiết diện là đa giác được sắp thứ tự trong mặt cắt, không nối các giao điểm tùy ý. Xử lý mặt cắt đi qua đỉnh, trùng cạnh/mặt, hoặc không cắt khối. Nhóm này khó hơn rõ rệt so với thêm vài nét vẽ mẫu.

**M10. Mô hình bài tối ưu.** Các mẫu hình có tham số: cắt bốn góc tấm bìa làm hộp, hàng rào, bể chứa. Hình và đồ thị đại lượng mục tiêu dùng cùng tham số. Đây là thư viện mẫu có giả thiết rõ, không phải bộ giải mọi bài toán thực tế.

### Nhóm C — Xác suất và thống kê: bổ sung quan trọng cho Toán

**M11. Sơ đồ cây và bảng 2×2.** Vẽ nhánh biến cố, bổ sung biến cố đối, xác suất có điều kiện, xác suất đường đi và tổng xác suất. Đổi cùng bộ dữ liệu giữa sơ đồ cây và bảng 2×2. Giáo viên chọn nhánh được tô sáng; có thể che các kết quả để hỏi học sinh. Kiểm tra tổng nhánh và mẫu số, không tự giả định độc lập.

**M12. Venn và phép toán biến cố.** Hai/ba tập, tô giao/hợp/phần bù/hiệu; nhãn biến cố. Diện tích vòng tròn là hình minh hoạ, không mặc nhiên tỉ lệ với xác suất. Đặt cạnh M11 để phân biệt P(A|B) với P(B|A).

**M13. Thống kê ghép nhóm.** Nhập bảng khoảng–tần số; sinh biểu đồ, bảng trung điểm, trung bình, phương sai theo quy ước phổ thông và độ lệch chuẩn; nội suy tứ phân vị có nêu cách làm. Nếu khoảng có độ rộng khác nhau, histogram dùng mật độ phù hợp. Dữ liệu ghép nhóm cho giá trị ước lượng dựa trên trung điểm, không tái tạo được bộ dữ liệu gốc.

**M14. Bảng số liệu và hồi quy tuyến tính đơn giản.** Nhập cặp (x,y), biểu đồ phân tán, đường khớp tuyến tính và độ dốc. Dùng được cho Toán thống kê và thí nghiệm Vật lí; đơn vị hai trục tách biệt. Nếu có thanh sai số, nhập sai số đo thực sự, không tạo ngẫu nhiên. Không biến tương quan thành kết luận nhân quả.

### Nhóm D — Vật lý

**P01. Vectơ và sơ đồ lực.** Tổng/hiệu, hình bình hành, phân tích thành phần theo hai trục, mặt phẳng nghiêng, vật treo, lò xo. Có trục chọn được, góc, nhãn P,N,T,Fms. Chỉ tính khi nhập đủ dữ kiện; hướng ma sát theo chuyển động/xu hướng trượt tương đối, không cố định một phía.

**P02. Dao động liên kết đường tròn pha và x–v–a.** Nhập A,T hoặc ω,φ, đơn vị; cùng một thời điểm điều khiển ba đồ thị và hình chiếu. Bản đầu có khung hình tĩnh chọn pha; play/pause và tua thời gian là mở rộng sau. Ghi rõ quy ước x=A cos(ωt+φ), v=-Aω sin(ωt+φ), a=-ω²x.

**P03. Sóng truyền và sóng dừng.** Phân biệt u(x,t0) với u(x0,t). Chọn biên cố định/cố định hoặc cố định/tự do; đánh dấu nút/bụng và bậc dao động theo đúng điều kiện biên. Tham số và pha thực, không chỉ kéo méo nét sin có sẵn. Giao thoa hai nguồn chỉ mở rộng sau khi xử lý được chênh pha và biên độ từng nguồn.

**P04. Nhiệt–khí.** Mẫu p–V, V–T, p–T; ba quá trình đẳng nhiệt/đẳng áp/đẳng tích, điểm trạng thái và mũi tên tiến trình. Chuyển °C sang K đúng; lượng khí được giữ cố định khi dùng pV/T hằng số. Diện tích có dấu dưới p–V biểu diễn công do khí thực hiện theo quy ước đã chọn; làm rõ dấu và loại quá trình. Thêm đồ thị đun nóng/chuyển thể dạng từng đoạn.

**P05. Điện trường, từ trường và cảm ứng.** Điện tích điểm, vectơ E, tổng vectơ tại điểm; điện trường đều, bản tụ; kí hiệu ⊙/⊗ cho vectơ vuông góc trang; dây dẫn/khung dây và hướng I,B,F. Hình dây sức là minh hoạ có mô hình, chiều đi đúng; không tính tích có hướng bằng góc biểu kiến trên hình chiếu.

**P06. Phóng xạ.** Đường N(t), H(t), m(t), mốc bán rã và lượng đã phân rã; nhãn đơn vị và ký hiệu hạt nhân. Không lẫn số hạt còn lại với số hạt đã phân rã. Nếu vẽ hạt nhân con thì nêu giả thiết về phân nhánh và tính bền của hạt nhân con.

**P07. Mạch điện và giản đồ pha — nhóm mở rộng.** Kho ký hiệu nguồn, R,L,C, công tắc, ampe kế/vôn kế; bắt đầu dây vào cổng, chấm nối khác giao cắt. Tính mạch DC nối tiếp/song song đơn giản theo cấu trúc khai báo. RLC điều hòa, phasor và cộng hưởng là phần lựa chọn nâng cao, có điều kiện áp dụng, không đưa tất cả vào màn hình mặc định.

## Ưu tiên riêng từ 03/11/2026

Đợt **Vectơ – Đường thẳng – Hợp lực** lấy phần tọa độ vectơ/đường của M08 và lực đồng quy của P01, tách khỏi các chặng đầy đủ. Xem [hướng dẫn và giới hạn](VECTO-DUONG-THANG-HOP-LUC.md), [kiến trúc](VECTO-KIEN-TRUC.md), [kiểm chứng](VECTO-KIEM-THU.md). Nguyên hàm–tích phân là đợt kế tiếp; chưa triển khai toàn bộ 2A. Mặt phẳng/cầu/khối/thiết diện, dao động và sóng chưa được hoàn thành bởi đợt này.

## Lộ trình triển khai đề xuất

| Chặng | Phạm vi | Lý do | Trạng thái |
|---|---|---|---|
| 1 | M01–M03, M04 nhập tay và tự sinh giới hạn | Mở rộng phần Toán rõ nhất; dùng lại cho đồ thị Vật lí; chưa cần mô hình 3D | Đã nghiệm thu 02/11/2026 |
| 2A | M05–M07 | Tích phân, miền nghiệm và dựng hình phụ thường xuyên dùng khi giải bài | Chưa triển khai |
| 2B | M08–M10 | Cần dữ liệu hình học thật, mô hình chiếu và kiểm thử nhiều suy biến | Một phần M08 trong đợt ưu tiên vectơ; chưa hoàn thành chặng |
| 3A | M11–M12 | Xác suất có điều kiện nhìn bằng cây/bảng; giá trị dạy học lớn | Chưa triển khai |
| 3B | M13–M14 | Thống kê và dữ liệu dùng chung hai môn | Chưa triển khai |
| 4A | P01–P03 | Nâng từ hình mẫu sang hình có tham số và trạng thái | Lực đồng quy P01 trong đợt ưu tiên; chưa hoàn thành chặng |
| 4B | P04–P06 | Bổ sung những mảng Vật lí 12 còn thiếu | Chưa triển khai |
| 4C | P07 và phần mô phỏng nâng cao | Làm sau các công cụ cốt lõi, theo nhu cầu thực tế | Chưa triển khai |

Chặng 1 đã có sản phẩm vector tĩnh dùng được, với phạm vi và giới hạn công bố trong hướng dẫn. Trạng thái này không bao gồm các chặng còn lại; mỗi chặng phải có nghiệm thu riêng.

## Những lỗi phải đưa vào tiêu chí nghiệm thu

| Tình huống tự tạo để kiểm chứng | Kết quả phải đúng |
|---|---|
| y=1/x trên [-2,2] | Tách hai nhánh; không có đoạn nối xuyên qua x=0 |
| y=(x²-1)/(x-1) | x=1 bị loại, có thể đánh dấu lỗ (1,2); không dựng tiệm cận đứng giả |
| y=(x²+1)/(x-1) | Tiệm cận x=1 và y=x+1 |
| y=x³-3x | Cực đại (-1,2), cực tiểu (1,-2); bảng tăng/giảm tương ứng |
| y=x³ | f'(0)=0 nhưng không phải cực trị |
| Tiếp tuyến y=x² tại x=1, hai trục khác tỉ lệ | Phương trình y=2x-1 vẫn đúng |
| f=ln x và f=√x | Không vẽ miền x≤0 cho ln; √x có điểm tại 0 |
| Diện tích giữa x và x² trên [0,1] | 1/6 |
| f=x-1 trên [0,2] | Tích phân có dấu 0; diện tích 1 |
| Quay miền giữa x và x² trên [0,1] quanh Ox | Thể tích 2π/15 |
| Hai đường (t,0,0) và (0,s,1) | Chéo nhau, khoảng cách 1, dù hình chiếu có thể cắt nhau |
| Chiếu điểm (1,2,3) lên z=0 | Chân chiếu (1,2,0), khoảng cách 3 |
| Cầu tâm (1,2,3), R=2, cắt z=3 / z=5 / z=6 | Đường tròn R=2 / một điểm / không giao |
| Lập phương [0,1]³ cắt x+y+z=1.5 | Sáu đỉnh nằm trên mặt cắt, sắp thành lục giác |
| Cây với P(B)=0.01, P(A|B)=0.9, P(A|không B)=0.05 | P(B|A)=2/13; không đổi chỗ điều kiện |
| Ghép nhóm [0,10),[10,20), tần số 2,2 | Theo trung điểm: trung bình 10, phương sai 25, độ lệch chuẩn 5 |
| Nhân mọi tần số ghép nhóm với cùng số nguyên dương | Trung bình và phương sai theo mẫu số n không đổi |
| Lực 10N tạo góc 30° với Ox | Fx=5√3N, Fy=5N theo chiều dương đã chọn |
| u=Acos(ωt+φ) và các đồ thị x–v–a liên kết | Pha, dấu và đơn vị dùng cùng quy ước, a=-ω²x |
| Sóng dừng cố định hai đầu L=1, mode 2 | Nút 0,0.5,1; bụng 0.25,0.75 |
| Khí đẳng nhiệt từ V đến 2V | Áp suất giảm một nửa, không tính bằng nhiệt độ Celsius |
| Phóng xạ sau ba chu kỳ bán rã | N=N0/8; số đã phân rã=7N0/8 |

## Nguyên tắc thiết kế riêng cho VK640

- Một bảng “Toán/Lý” gọn với mục Giải tích, Hình học, Xác suất, Dữ liệu, Vật lý; có tìm theo tên bài toán: “tiếp tuyến”, “Bayes”, “thiết diện”, “sóng dừng”.
- Nút đủ dễ chạm bằng bút; ít hộp thoại lồng nhau; nhớ tham số gần nhất. Chức năng thông thường nên tạo được hình sau chọn mẫu → kéo khung → xác nhận tham số.
- Ctrl+` tiếp tục hiện/ẩn chữ gợi nhớ dưới nút; Alt+` tiếp tục đổi cột. Không bắt người dùng học thêm hàng chục tổ hợp phím. Thêm công cụ mới vào cùng cơ chế nhãn.
- Hình chèn nằm trong khung nhìn hiện tại, có xem trước, Escape hủy, Ctrl+Z một bước. Không đưa kết quả lên trang PDF khác hoặc ngoài vùng đang nhìn.
- Hình/bảng phải xuất ra PDF và mở lại được. Ưu tiên nét, chữ và vùng tô vector; một cụm hình được chọn cùng nhau khi vừa chèn. “Một cụm” không đồng nghĩa “một Stroke”.
- Phải tách “kéo giãn hình minh hoạ” khỏi “đổi tham số toán học”. Khi cần sửa công thức, quay lại trình tạo và sinh hình mới; chỉ cam kết sửa liên kết sau lưu/mở nếu đã có định danh và lưu thông số thật.
- Không gắn quan hệ hình học chính xác với hình phác tay bằng nhận đoán âm thầm. Chỉ có quan hệ khi người dùng chọn/nhập hoặc phần mềm dựng rõ ràng.

## Gợi ý kỹ thuật sau khi đã xem repo

Tận dụng Lua để đăng ký UI, gọi action và chèn/đặt vùng chọn. Repo đã có addStrokes, addTexts, addSplines, getVisiblePageRect, addToSelection và showToolbarShortcuts; nhưng cần kiểm tra khả năng vùng tô/nhóm undo và giới hạn của từng API trước khi dùng.

Tính toán tọa độ, tách miền, dựng hình và lấy mẫu nên nằm ở lớp C++ thuần được test trực tiếp. Không ép mọi hình thành một nét: nhiều nhánh đồ thị phải là nhiều nét để không có đoạn nối giả. Trình bày GTK tách khỏi logic toán.

Mô hình 3D là chặng riêng. Không cần viết lại định dạng tài liệu từ đầu chỉ để có đồ thị tĩnh. Nếu thêm metadata cho sửa tham số sau khi mở lại, phải khảo sát save/load/clone/undo/copy-paste và tương thích trước; không để metadata mất đồng bộ khi người dùng sửa nét thông thường.

Các bước build/install và bộ test hồi quy giữ nguyên như prompt triển khai đi kèm. Không được báo “tất cả đã xong” khi mới tạo menu, ảnh chụp hoặc mockup.
