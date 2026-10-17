# Kiểm thử TÍCH HỢP trong app thật (không phải unit test)

Dùng khi cần chứng minh một hàm Lua API mới hoạt động trên TÀI LIỆU THẬT, chứ không chỉ trên
bản giả: dựng một instance Xournal++ RIÊNG (cấu hình riêng, không đụng tài liệu của user), rồi
gọi hàm qua D-Bus và đọc kết quả từ stdout của app.

## Vì sao làm cách này
- Bơm phím vào app bằng công cụ ảo KHÔNG đáng tin (GTK từ chối sự kiện bàn phím ảo).
  Gọi thẳng GAction qua D-Bus thì chạy đúng callback, không phụ thuộc tầng nhập liệu.
- `io.open` KHÔNG chạy trong Lua của app ⇒ mọi kết quả in bằng `print()` + `io.stdout:flush()`
  ra stdout của app, rồi đọc từ file log.
- Số hiệu action: action của plugin đặt tên `plugins.action-<N>`, N theo THỨ TỰ ĐĂNG KÝ
  (plugin nạp trước lấy số nhỏ). Muốn chắc thì đếm số action hoặc gọi thử và xem log.

## Quy trình
```bash
# 1. dựng cấu hình test (tách câu lệnh này RA KHỎI lệnh mở app — gộp chung sẽ hỏng)
D=/tmp/xpp-kt2
mkdir -p $D/xournalpp/plugins
cp -r tests/integration/EraseProbe $D/xournalpp/plugins/
cp ~/.config/xournalpp/settings.xml $D/xournalpp/settings.xml
python3 - <<'PY'
import re
p = "/tmp/xpp-kt2/xournalpp/settings.xml"
s = open(p, encoding="utf-8").read()
s = re.sub(r'name="pluginEnabled" value="[^"]*"',
           'name="pluginEnabled" value="/tmp/xpp-kt2/xournalpp/plugins/EraseProbe"', s)
open(p, "w", encoding="utf-8").write(s)
PY

# 2. mở app test, log ra file
XDG_CONFIG_HOME=$D setsid ~/.local/bin/xournalpp > $D/app.log 2>&1 &

# 3. tìm bus D-Bus của instance (theo pid), ghi vào bus.txt
PID=$(pgrep -f "^$HOME/.local/bin/xournalpp$" | head -1)
busctl --user list | awk -v p=$PID '$2==p && $3=="xournalpp" {print $1; exit}' > $D/bus.txt

# 4. gọi hàm và đọc kết quả
XPPKT=$D ./act.sh 0            # action-0 = probeStats → in ra số nét/ảnh/chữ/trang
```

## Bộ kiểm TỰ ĐỘNG (dùng cái này, đừng gõ tay từng lệnh)

```bash
bash tests/integration/run-tich-hop.sh          # dựng instance + chạy hết phép đo + TỰ CHẤM điểm
bash tests/integration/run-tich-hop-trong.sh    # như trên nhưng chạy trong phiên đăng nhập hiện tại (không Xvfb)
python3 tests/integration/check-tich-hop.py     # chỉ chấm lại trên log cũ (không cần mở app)
```

- `run-tich-hop.sh` in ra bảng PASS/FAIL và dòng `TỔNG: n/N mục PASS`; exit code 0 chỉ khi đủ N.
- Mỗi phép đo là một action của plugin `EraseProbe` (`go <N> <nhãn>` trong script = gọi
  `plugins.action-N`), log lưu ở `$D/out-<nhãn>.txt`, `$D/app.log` là log chung.
- **Phần C gọi callback của CHÍNH PLUGIN THẬT** (`VK640ToolCycle`) — đúng đoạn code mà phím tắt
  kích hoạt, chỉ khác là kích bằng D-Bus thay vì bàn phím. `run-tich-hop.sh` nạp cả hai plugin
  (`pluginEnabled` để **EraseProbe TRƯỚC**) nên số hiệu action = 30..45; số hiệu cụ thể do
  `idx-action.py` tính TỪ NGUỒN (`44` ⇒ `vk640SelectionToImage`, `41` ⇒ `vk640EraseNotePagesInk`),
  không ghi cứng: thêm/bớt một mục menu là số đổi, script tự tính lại.
  `out-E0-actions.txt` ghi số action đã đăng ký (phải = 46) làm điều kiện tiên quyết.
- `check-tich-hop.py` đọc lại các `out-*.txt`, tìm dòng `[PROBE]` gần NHẤT sau nhãn, nên KHÔNG cần
  chạy app lại khi chỉ sửa phép chấm.
- Nhãn nên đặt `<số>-<ý>` (vd `19-png`) để `findall` không khớp lẫn giữa các lần gọi cùng một action.

## Bẫy đã gặp (đều đã trả giá)
- **Các phần A/B/C dùng CHUNG một tài liệu**, nên phần sau thừa hưởng nội dung của phần trước (phần B
  để lại ảnh/chữ trên các trang nháp, và fixture đặt nền `plain` lên TRANG ĐANG XEM — không phải trang 1).
  Phép kiểm phải lấy MỐC bằng một phép đo ngay trước đó (`28-allC0`, `24-pagesC0`) rồi so hiệu số; so
  với số cứng sẽ báo FAIL giả và làm người đọc tưởng tính năng hỏng.
- Cấu hình PHẢI ở `$XDG_CONFIG_HOME/xournalpp/settings.xml`; đặt sai chỗ ⇒ app tự sinh cấu hình
  mặc định và KHÔNG nạp plugin (dấu hiệu: `plugins.action` = 0).
- Chép cấu hình và mở app phải là HAI câu lệnh riêng (gộp chung thì lần chạy đầu dùng cấu hình mặc định).
- Tắt app test bằng `kill` để lại `emergencysave.xopp` ⇒ lần mở sau HIỆN MODAL "Tệp tin khôi phục",
  modal này nuốt nhập liệu và làm mọi phép đo sau đó sai. Xoá file đó trước khi mở lại.
- Object path của action là `/com/github/xournalpp/xournalpp/window/1`, bus là tên duy nhất
  `:1.x` (tên `com.github.xournalpp.xournalpp` KHÔNG phục vụ `org.gtk.Actions`).
- Chép binary ra /tmp để test ⇒ app không tìm thấy tài nguyên UI (`findResourcePath()` chỉ nhìn
  cạnh file thực thi) ⇒ nhóm action rỗng. Luôn dùng bản cài trong `~/.local/bin`.
- Đọc hộp thoại: `/usr/bin/python3` (có `pyatspi`), KHÔNG phải python của venv.
- Truy vấn mức TRANG của `app.getStrokes/getImages` (`"page"`, `"layer"`, `"notePages"`) **xoá luôn vùng
  chọn hiện tại** (tác dụng phụ có sẵn trong nguồn: để chữ trong vùng chọn được nhận diện). Nên muốn
  đo trạng thái vùng chọn thì phải đo NGAY sau hành động, TRƯỚC mọi lệnh `stats()`.
- Đếm "mực" trong PNG: khối `IDAT` là dữ liệu **đã lọc theo từng dòng** (5 kiểu None/Sub/Up/Average/
  Paeth) và Xournal++ lưu nền trong suốt dưới dạng RGB(0,0,0) + alpha=0. Bỏ qua bộ lọc, hoặc không xét
  alpha ⇒ "100% điểm khác trắng" (dấu hiệu giải mã sai, không phải ảnh đặc).
- Tỷ lệ mực hợp lý là ~2..70%: bằng 0% = ảnh trắng, bằng đúng tổng số điểm = giải mã sai. Đặt cận cả
  hai đầu, đừng chỉ kiểm `> 0`.
