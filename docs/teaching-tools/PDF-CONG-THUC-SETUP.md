# Setup và phụ thuộc — PDF → công thức

## Bản cài

- Source: `/home/phuc/xpp-src`, build `/home/phuc/xpp-src/build`.
- Prefix duy nhất: `/home/phuc/.local`; app `/home/phuc/.local/bin/xournalpp`.
- Runtime riêng: `/home/phuc/.local/share/xournalpp/formula-ocr/`.
- Worker cài cùng ứng dụng: `/home/phuc/.local/share/xournalpp/teaching-tools/formula-ocr.py`.
- Không cần CWD, thư mục build, biến môi trường của agent hoặc công cụ Codex để nhận dạng.
- Không thay `/usr/bin/xournalpp`; không cần root cho các thành phần mới trong lượt này.

Sau khi cài, mở lại bản cá nhân. Phiên đang chạy vẫn dùng binary đã nạp trước khi cài; không bị trình cài đóng.

## Tái lập

Cần `uv` trong PATH cho setup (đã có tại `~/.local/bin/uv`). Helper tải **mã/model công khai**, không gửi PDF người dùng:

```bash
cd /home/phuc/xpp-src
bash scripts/teaching-tools/setup-formula-ocr.sh /home/phuc/.local
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/home/phuc/.local
cmake --build build -j 4
cmake --install build
```

Helper cũng đã cài trong `~/.local/share/xournalpp/teaching-tools/setup-formula-ocr.sh`. File lock và manifest hash nằm cạnh helper. Script setup được chạy lại hai lần để kiểm tra idempotence; manifest chỉ hash bốn asset, không tự hash chính manifest.

Runtime dùng CPython **3.12.14** do uv quản lý (đường dẫn interpreter nền nằm trong `~/.local/share/uv/python/`); không dùng Python 3.14 hệ thống cho package này. Giữ cả uv-managed Python nếu di chuyển venv. `requirements-ocr.lock.txt` ghim toàn bộ package, trong đó:

| Thành phần | Phiên bản |
|---|---|
| rapid-latex-ocr | 0.0.9 |
| onnxruntime | 1.31.1 |
| numpy | 1.26.4 |
| tokenizers | 0.23.3 |
| Pillow | 13.0.0 |
| opencv-python | 4.11.0.86 |
| requests | 2.32.5 |
| chardet | 5.2.0 |

Không cần Tesseract để nhận dạng toán. Không cần pdflatex để nhận dạng/xem trước/chèn công thức mới. pdflatex và Poppler CLI dùng để tạo fixture/kiểm chứng; chỉnh TexImage bằng hộp LaTeX cũ vẫn dùng cấu hình LaTeX có sẵn của Xournal++.

## Model và giấy phép

Model ONNX từ release `RapidAI/RapidLaTeXOCR v0.0.0`, đặt trong `formula-ocr/lib/python3.12/site-packages/rapid_latex_ocr/models/`. Không tự tải model khi nhận dạng: worker kiểm đủ file rồi truyền đường dẫn tường minh. Thiếu model trả lỗi, không có fallback gửi tài liệu lên mạng.

| Asset | SHA-256 |
|---|---|
| encoder.onnx | `01bf5dc25539ca0cd5b1bd29296ea495977a6ba5f629dc4178277809d26e5e7d` |
| decoder.onnx | `bd695497bf1b22279b7626f5916c79226e1e244c84355f8da7edfd2d921d0072` |
| image_resizer.onnx | `e0b075c39700f64d50400f39c8fc186bbb3b5d84d31864008313f376603aca9d` |
| tokenizer.json | `1dc27b18d6a518d0d5ff3f4bb7bd98521fe80ad39e5b2a246d4109f1bb9d5019` |

Khoảng **171 MiB model**, **533 MiB venv** trong lượt đo này; interpreter uv riêng cần thêm dung lượng. Nguồn chính thức: [RapidLaTeXOCR README](https://github.com/RapidAI/RapidLaTeXOCR), [release model](https://github.com/RapidAI/RapidLaTeXOCR/releases/tag/v0.0.0), [LaTeX-OCR gốc](https://github.com/lukas-blecher/LaTeX-OCR).

Wheel 0.0.9 có tệp LICENSE **MIT** (RapidAI, 2023), repo hiện tại cũng MIT; nguồn LaTeX-OCR gốc MIT (Lukas Blecher, 2021). Metadata PyPI ghi Apache-2.0 không khớp tệp LICENSE; đã giữ nguyên bản LICENSE trong `scripts/teaching-tools/licenses/`, không suy ra một giấy phép khác cho model. Nguồn xuất xứ model là chuyển ONNX từ LaTeX-OCR theo README; release không kèm một model card/giấy phép trọng số riêng chi tiết. Khi phân phối thương mại/ra ngoài, cần kiểm tra điều khoản upstream thay vì suy rộng kết quả cài cá nhân này.

Font công thức: **Latin Modern Math**, bản package `fonts-lmodern 2.005-1build1`; font `/usr/share/texmf/fonts/opentype/public/lm-math/latinmodern-math.otf`, GUST Font License theo copyright package. Fallback STIX Two Math/DejaVu Serif; nhãn tiếng Việt dùng font hệ thống. Cairo/Pango nhúng subset font trong PDF công thức. Các tệp giấy phép có trong prefix `share/xournalpp/teaching-tools/licenses/`.

Các package hệ thống đã có: Cairo 1.18.4, Pango 1.57.0, GTK 3.24.52; thông tin máy/phụ thuộc đầy đủ trong manifest bàn giao. Không cài thêm package hệ thống bằng root.

## Kiểm tra backend độc lập

Lệnh dưới chỉ dùng **fixture công khai tự tạo**, không phải công cụ cần ngồi cạnh ứng dụng:

```bash
/home/phuc/.local/share/xournalpp/formula-ocr/bin/python \
  /home/phuc/.local/share/xournalpp/teaching-tools/formula-ocr.py \
  /home/phuc/xpp-src/output/pdf/pdf-formula-fixtures/hole/crop.png
```

`[result] code=ok` và các nhóm candidate là giao thức nội bộ version 1. Không dùng các chuỗi OCR thô làm lệnh shell/TeX.

Biến `XOURNALPP_FORMULA_MODELS` có thể trỏ tới thư mục model cục bộ khác. Mặc định không cần đặt; test dùng thư mục trống để kiểm lỗi thiếu model. Khi khắc phục lỗi không rõ nguồn, bỏ biến này trước, chạy lại setup và kiểm manifest. Đây là đường dẫn file, không phải URL/backend dịch vụ.

## Giới hạn và phục hồi

Worker CPU, không cần GPU. Một crop nested đo được khoảng **0,74 s**, peak RSS khoảng **359 MiB**; benchmark cả khởi tạo/inference thực tế được ghi riêng. Chừa khoảng 1 GiB RAM cho một tác vụ là mức dự phòng từ phép đo này, không phải bảo đảm cho mọi tài liệu. App và PDF đang mở còn dùng bộ nhớ riêng.

Giới hạn: ảnh ≤8M px, cạnh ≤4096 px, CPU 50/55 s, địa chỉ 4 GiB, deadline 60 s; tối đa 12 cách đọc OCR. Thiếu runtime/model, vùng trống, timeout hoặc lỗi parse đều cho nhận dạng lại/sửa mà không cần khởi động lại app. Font fallback thực tế đã có; chưa giả lập một hệ thống hoàn toàn không còn font.

Không log toàn bộ PDF hoặc crop vào log chẩn đoán. Crop chỉ ở thư mục tạm 0700 và bị xóa theo vòng đời job. Nếu app bị kill/crash trước cleanup có thể còn thư mục `xournalpp-formula-*` riêng; cần kiểm chính xác thư mục trước khi dọn. File `.xopp`/preview do người dùng chủ động lưu vẫn chứa nội dung tài liệu theo cơ chế Xournal++.

Thử nghiệm không dùng: `~/.local/share/xournalpp/formula-runtime/` được tạo khi khảo sát Python 3.14; app không đọc nó. Log cài thất bại ban đầu được giữ trong `build/pdf-math-verification/`, runtime dùng thật là **formula-ocr**.
