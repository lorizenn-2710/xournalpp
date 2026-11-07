# Xournal++ — hướng dẫn tiếng Việt

[English](README.md) · [Dự án chính chủ](https://github.com/xournalpp/xournalpp)

Xournal++ là ứng dụng ghi chú bằng bút, viết trên nền giấy và chú thích PDF, được xây dựng bằng C++ và GTK3. Ứng dụng chạy trên Linux, Windows và macOS. Đây là **fork** của dự án chính chủ, không phải kênh phát hành chính thức của nhóm Xournal++.

> **Trạng thái cài đặt:** Hiện chưa có bộ cài Windows/Linux được kiểm thử để tái tạo đầy đủ bản ứng dụng tùy chỉnh đang dùng trên máy phát triển. Bộ cài chính chủ chỉ chứa Xournal++ gốc. Bản tùy chỉnh đã được build và kiểm thử trên Linux; Windows chưa được xác minh.

![Giao diện Xournal++ trên Linux](readme/main.png)

## Tính năng

- Viết và vẽ bằng chuột hoặc bút cảm ứng có lực nhấn; dùng nhiều loại giấy và lớp.
- Mở PDF để ghi chú, đánh dấu, chọn văn bản; xuất trang hoặc tài liệu thành PDF, PNG, SVG.
- Công cụ hình học, thước, compa, hình dạng, văn bản và công thức LaTeX.
- Hoàn tác, tự lưu, sao lưu, tùy biến thanh công cụ và mở rộng bằng plugin Lua.
- Nhánh [`feature/protractor-geometry-tools`](https://github.com/lorizenn-2710/xournalpp/tree/feature/protractor-geometry-tools) của fork này bổ sung công cụ dạy Toán–Vật lý, nhập công thức từ vùng PDF và sổ nháp nổi. Xem [tài liệu tính năng](https://github.com/lorizenn-2710/xournalpp/blob/feature/protractor-geometry-tools/docs/teaching-tools/README.md) trên nhánh đó.

## Cài bản phát hành chính chủ

Các cách cài trong mục này lấy **bản Xournal++ chính chủ**. Chúng không tự bao gồm tính năng riêng trong nhánh tùy chỉnh của fork.

### Ubuntu và Debian

Trên bản phân phối có gói `xournalpp` trong kho hệ thống:

```sh
sudo apt update
sudo apt install xournalpp
```

Bạn cũng có thể tải đúng gói cho phiên bản hệ điều hành của mình từ [trang phát hành chính chủ](https://github.com/xournalpp/xournalpp/releases). Danh sách PPA và các lựa chọn khác nằm trong [README tiếng Anh](README.md#ubuntu-and-derivatives).

### Fedora, openSUSE, Arch Linux và Solus

```sh
sudo dnf install xournalpp       # Fedora
sudo zypper in xournalpp         # openSUSE Tumbleweed
sudo pacman -S xournalpp         # Arch Linux
sudo eopkg it xournalpp          # Solus
```

Chạy **một** lệnh tương ứng với bản phân phối đang dùng. Thông tin gói và các bản phát triển được liên kết trong [hướng dẫn chính chủ](README.md#installing).

### Flatpak

Cài từ [Flathub](https://flathub.org/apps/com.github.xournalpp.xournalpp):

```sh
flatpak install flathub com.github.xournalpp.xournalpp
flatpak run com.github.xournalpp.xournalpp
```

Nếu cần LaTeX trong bản Flatpak, xem phần [Flatpak của README chính chủ](README.md#flatpak) để cài phần mở rộng TeX Live.

### Windows và macOS

Tải bộ cài phù hợp từ [Releases chính chủ](https://github.com/xournalpp/xournalpp/releases). Trên Windows còn có Microsoft Store và `winget install -e --id Xournal++.Xournal++`. Trên macOS, xem [hướng dẫn khắc phục chặn ứng dụng](README.md#macos) nếu hệ thống từ chối mở bản tải về.

### Android và iOS

Xournal++ Mobile là **dự án riêng**, không phải bản desktop trong kho này. Xem [mục Mobile của dự án chính chủ](README.md#mobile--web-app) để biết trạng thái và liên kết hiện hành.

## Cài bản tùy chỉnh từ mã nguồn

Đây là cách build mã C++ chứa các công cụ Toán–Vật lý và sổ nháp nổi của fork. Các lệnh dưới đây dành cho Ubuntu/Debian; với hệ khác, xem [phụ thuộc theo hệ điều hành](readme/LinuxBuild.md). Bản tùy chỉnh chưa có gói phát hành cài sẵn trên trang Releases.

1. Cài công cụ biên dịch và thư viện:

   ```sh
   sudo apt update
   sudo apt install git cmake ninja-build g++ libgtk-3-dev libpoppler-glib-dev \
     portaudio19-dev libsndfile-dev texlive libxml2-dev liblua5.4-dev \
     libzip-dev librsvg2-dev gettext libgtksourceview-4-dev help2man libqpdf-dev
   ```

2. Lấy đúng nhánh tùy chỉnh, biên dịch và cài trong thư mục người dùng:

   ```sh
   git clone --branch feature/protractor-geometry-tools \
     https://github.com/lorizenn-2710/xournalpp.git
   cd xournalpp
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
     -DCMAKE_INSTALL_PREFIX="$HOME/.local"
   cmake --build build --parallel
   cmake --install build
   "$HOME/.local/bin/xournalpp"
   ```

3. Kiểm tra mã sau khi build:

   ```sh
   ctest --test-dir build --output-on-failure
   ```

Để nhận dạng công thức từ ảnh scan, cần cài thêm runtime và model theo [hướng dẫn OCR](https://github.com/lorizenn-2710/xournalpp/blob/feature/protractor-geometry-tools/docs/teaching-tools/PDF-CONG-THUC-SETUP.md). Mã plugin Lua VK640 hiện được cài cùng ứng dụng từ nhánh tùy chỉnh. Plugin cài chung được để **tắt mặc định** để tránh nạp trùng với bản cá nhân; trên máy mới, bật một bản VK640 trong phần quản lý plugin của Xournal++. Bố cục thanh công cụ và các thiết lập cá nhân trên máy phát triển chưa được đóng gói, nên bản cài mới có thể trông khác. Các bước cài/kiểm thử độc lập của dự án gốc có trong [Compile.md](readme/Compile.md).

## Tài liệu và đóng góp

- [Hướng dẫn sử dụng chính chủ](https://xournalpp.github.io/guide/overview/)
- [Hướng dẫn các công cụ riêng của fork](https://github.com/lorizenn-2710/xournalpp/blob/feature/protractor-geometry-tools/docs/teaching-tools/README.md)
- [Cách đóng góp cho Xournal++](CONTRIBUTING.md)
- [Giấy phép GPL-2.0-or-later](LICENSE)

Nếu báo lỗi của tính năng riêng trong fork, hãy nêu nhánh đang dùng và các bước tái hiện trong [Issues của fork](https://github.com/lorizenn-2710/xournalpp/issues). Với lỗi của bản phát hành chính chủ, dùng [Issues chính chủ](https://github.com/xournalpp/xournalpp/issues).
