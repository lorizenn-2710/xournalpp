--[[
    Plugin VK640 cho Xournal++ — bút VEIKK VK640
    (bản TOÁN/LÝ: xem thêm phần hình học trong toolbar.ini và snap nối điểm trong mã nguồn)

    PHÍM TẮT
      Tab        : Bút -> Tẩy -> Kéo trang (Hand) -> Bút ...
      Shift + Y  : BẬT/TẮT "PDF + Giấy nháp" (giấy nháp hiện NGAY BÊN CẠNH PDF)
      Ctrl + Y   : giống Shift+Y (giữ thêm cho quen tay)
      Alt + `    : đổi bố cục 1 trang <-> 2 trang cạnh nhau
                   (phím ` nằm ngay bên trái số 1, phía trên Tab)
      Ctrl + U   : XOÁ HẾT nét bút trên trang đang xem (giữ ảnh + nền giấy; Ctrl+Z lấy lại)
      Ctrl + Shift + M : XOÁ HẾT nét bút trên MỌI trang giấy nháp (giữ ảnh + chữ; Ctrl+Z lấy lại)
                   (mu​ốn xoá hẳn cả TRANG nháp: dùng menu plugin "VK640: xoa HAN cac trang giay nhap")
      Ctrl + Shift + C : COPY vùng đã chọn thành 1 ẢNH (nét gốc VẪN CÒN; ảnh dời/phóng to/xoá đư​ợc)
      Ctrl + Shift + N : ghi tên đỉnh (A, B, C, D / A', ... / S) cho hình đang chọn
      Ctrl + J   : nét đứt  <->  nét liền
      Ctrl + K   : xoay vòng 4 kiểu nét: liền -> đứt -> đứt-chấm -> chấm -> liền
      Ctrl + B   : tăng bề dày nét (mảnh -> rất dày, rồi quay lại)
      Ctrl + Shift + B : giảm bề dày nét

    VÌ SAO CÓ PHÍM NÉT ĐỨT: trên bảng vẽ, bấm nút thanh công cụ bằng bút chậm và dễ trượt tay;
    một phím trên bảng vẽ (hoặc Ctrl+J trên bàn phím) nhanh hơn nhiều khi đang giảng bài.

    VÌ SAO KHÔNG DÙNG Ctrl+D: Xournal++ đã gán Ctrl+D cho menu "Trang mới phía sau" và
    Ctrl+Shift+D cho "Công cụ mặc định". Với hai GAction cùng một phím, GTK cho cái ĐĂNG KÝ
    TRƯỚC thắng (đã đo bằng một chương trình GTK nhỏ: đổi thứ tự đăng ký thì đổi luôn bên thắng),
    mà menu của app đăng ký trước plugin — nên Ctrl+D và Ctrl+Shift+D rơi hết vào menu và plugin
    KHÔNG BAO GIỜ chạy. Ctrl+J và Ctrl+K thì không menu nào của app dùng.

    CHỨC NĂNG "PDF + GIẤY NHÁP" (Shift+Y / Ctrl+Y)
      Bấm MỘT LẦN: chèn một trang GIẤY KẺ DÒNG ngay sau MỖI trang PDF còn thiếu,
      rồi bật chế độ 2 trang cạnh nhau -> mỗi trang đề luôn có giấy nháp bên cạnh.

      Vì sao làm vậy (bài học từ các bản trước):
        * Bản cũ chỉ tạo MỘT trang nháp đi theo con trỏ. Trang nháp là một trang
          thật của tài liệu nên nó KHÔNG đi theo màn hình: tạo lúc ở trang 4 thì
          nằm mãi ở vị trí 5; sang trang 1 thì thấy cặp (1,2) = 2 trang đề và
          tưởng "gộp thành 2 bản".
        * Bản này KHÔNG BAO GIỜ xoá và KHÔNG BAO GIỜ dời -> không còn gì để lệch.
      Bấm lại: chỉ điền vào những trang PDF còn thiếu (không nhân bản, không xoá).
      Muốn về 1 trang: Alt+` (đổi bố cục), giấy nháp vẫn còn nguyên trong file.

    GHI CHÚ KỸ THUẬT (đã kiểm chứng bằng thực nghiệm)
    * Xournal++ gọi callback bằng lua_getglobal(name) nên hàm callback BẮT BUỘC
      là hàm toàn cục — "local function" sẽ im lặng không chạy.
    * Accelerator của plugin gắn ở cấp ỨNG DỤNG
      (gtk_application_set_accels_for_action), nên một phím TRẦN như "y" sẽ
      NUỐT luôn việc gõ chữ y trong công cụ Text và trong ô nhập tên file.
      ĐÁNH ĐỔI CỦA Shift+Y: gõ chữ "Y" HOA bằng bàn phím sẽ kích hoạt chức năng
      này thay vì ra chữ Y. Muốn tránh thì dùng Ctrl+Y.
    * Giữ phím gây auto-repeat rất nhanh -> có chốt chặn 0.3-0.4 giây dựa trên
      /proc/uptime cho từng phím.
    * Các phím ĐÃ bị Xournal++ chiếm: F5, F9, F10, F11, F12, Delete,
      Ctrl+1..8, Ctrl+Page_Up/Down, Ctrl+Home/End.
    * VỀ CHUYỆN "GHI ĐÈ" PHÍM CỦA APP — hiểu cho đúng: plugin KHÔNG ghi đè được phím của
      menu. Đã đo bằng một chương trình GTK nhỏ: khi hai GAction cùng gắn một phím thì GTK
      cho cái ĐĂNG KÝ TRƯỚC thắng (đổi thứ tự đăng ký thì đổi luôn bên thắng), mà menu của
      app đăng ký trước plugin. Vì vậy Ctrl+D và Ctrl+Shift+D (menu "Trang mới phía sau" /
      "Công cụ mặc định") rơi hết vào app. Các phím plugin đang dùng — Ctrl+Y, Ctrl+J,
      Ctrl+K, Ctrl+B, Ctrl+Shift+B, Ctrl+`, Ctrl+Tab, F1, F8 — đã rà lại: KHÔNG menu nào
      của app dùng.
    * KHÔNG dùng "new-page-after" mà không kiểm tra: nó sao chép nền PDF sang
      trang mới (PageBackgroundChangeController::copyBackgroundFromOtherPage),
      tức là trang mới cũng là PDF chứ không phải giấy trắng.
    * app.openDialog PHẢI có bảng nút ở tham số 2:
      openDialog(message, {"nút"}, callback, isError). Thiếu -> Lua báo
      "bad argument #2 to 'openDialog' (table expected, got nil)" và callback chết.
]]

local C = app.C

local PDF_FALLBACK_W, PDF_FALLBACK_H = 595.275591, 841.889764  -- A4

-- ---------------------------------------------------------------------------
-- tiện ích
-- ---------------------------------------------------------------------------

local function uptime()
    local f = io.open("/proc/uptime")
    if not f then return nil end
    local line = f:read("*l")
    f:close()
    return tonumber(line:match("^(%S+)"))
end

-- chốt chống auto-repeat, mỗi phím một mốc riêng
local lastPress = {}
local function debounced(key, seconds)
    local t = uptime()
    if not t then return false end
    local prev = lastPress[key]
    if prev and (t - prev) < (seconds or 0.3) then
        return true
    end
    lastPress[key] = t
    return false
end

-- ---------------------------------------------------------------------------
-- 1. Tab: vòng xoay công cụ
-- ---------------------------------------------------------------------------

local cycle = { C.Tool_pen, C.Tool_eraser, C.Tool_hand }

-- Một chốt chống lặp RIÊNG cho mỗi phím: Tab, Ctrl+Tab và F8 cùng làm một việc, nhưng nếu dùng
-- chung một khoá thì bấm liền hai phím khác nhau sẽ bị nuốt mất lần thứ hai.
local function cycleTool(debounceKey)
    if debounced(debounceKey, 0.3) then return end

    local cur = app.getActionState("select-tool")
    local idx = 0
    for i = 1, #cycle do
        if cycle[i] == cur then
            idx = i
            break
        end
    end
    -- công cụ ngoài vòng (text, chọn vùng...) -> quay về bút
    app.changeActionState("select-tool", cycle[(idx % #cycle) + 1])
end

function vk640CycleTool()       cycleTool("tab") end
function vk640CycleToolCtrl()   cycleTool("ctrlTab") end
function vk640CycleToolF8()     cycleTool("f8") end

-- ---------------------------------------------------------------------------
-- 2. Alt+`: đổi bố cục 1 trang / 2 trang
-- ---------------------------------------------------------------------------

function vk640ToggleShortcutHints()
    if debounced("shortcutHints", 0.3) then return end
    if type(app.showToolbarShortcuts) ~= "function" then
        app.openDialog("Cần mở lại bản Xournal++ custom mới để hiện phím tắt dưới nút công cụ.",
                       {"Đã hiểu"}, "", true)
        return
    end
    app.showToolbarShortcuts()
end

function vk640ToggleColumns()
    if debounced("cols", 0.3) then return end

    local n = app.getActionState("set-columns-or-rows")
    if n == nil or n <= 0 then n = 1 end
    app.changeActionState("set-columns-or-rows", n == 1 and 2 or 1)
end

-- ---------------------------------------------------------------------------
-- 2b. Ctrl+J / Ctrl+K: kiểu nét (liền / đứt / đứt-chấm / chấm)
-- ---------------------------------------------------------------------------
--
-- VÌ SAO CẦN: trên bảng vẽ, bấm nút thanh công cụ bằng bút chậm và dễ trượt tay. Một phím
-- nhanh hơn nhiều khi đang giảng bài. Đây là các phím của app (đã kiểm tra không menu nào dùng):
--   Ctrl+J            : đổi qua lại nét liền <-> nét đứt  (dùng nhiều nhất)
--   Ctrl+K            : xoay vòng đủ 4 kiểu: liền -> đứt -> đứt-chấm -> chấm -> liền
--
-- GHI CHÚ: app lưu kiểu nét trong "tool-pen-line-style" dưới dạng chuỗi: "none" (liền),
-- "dash", "dashdot", "dot". Đọc bằng app.getActionState, đổi bằng app.changeActionState.

-- TÊN TRẠNG THÁI THẬT của app (đo bằng app.getActionState sau mỗi lần đổi):
--   nét liền -> "plain"   (KHÔNG phải "none" — "none" chỉ là giá trị MẶC ĐỊNH ban đầu;
--                          đặt "none" thì app cũng chuẩn hoá lại thành "plain")
--   nét đứt -> "dash" | đứt-chấm -> "dashdot" | chấm -> "dot"
-- Dùng sai tên "none" làm cả hai phím nét im lặng không chạy: so sánh `cur == "none"` không
-- bao giờ đúng, và vòng xoay tính ra index 1 = "none" -> app lại chuẩn hoá thành "plain".
local SOLID_STYLE = "plain"
local LINE_STYLES = { SOLID_STYLE, "dash", "dashdot", "dot" }

local function currentLineStyle()
    local s = app.getActionState("tool-pen-line-style")
    if s == nil or s == "none" then return SOLID_STYLE end
    return s
end

-- Kiểu nét của những nét ĐANG ĐƯ​ỢC CHỌN (nếu có vùng chọn).
--
-- VÌ SAO CẦN RIÊNG HÀM NÀY: app.getActionState("tool-pen-line-style") trả về kiểu nét của nét MỚI SẼ VẼ.
-- Khi giáo viên khoanh một hình ĐÃ VẼ rồi bấm Ctrl+J, thứ cần đổi là kiểu nét CỦA HÌNH ĐÓ, không phải
-- của nét sắp vẽ. app.getStrokes("selection") đ​ọc đư​ợc từ​ng nét kèm trường "lineStyle", nên quyết
-- định ở đây dựa trên đ​úng hình đang chọn chứ không đo​án theo trạ​ng thái bút.
-- Trả về: bảng đếm {kiểu = số nét} và tổng số nét; nil khi không có vùng chọn hoặc vùng chọn không có nét.
local function selectionStyleCounts()
    if type(app.getStrokes) ~= "function" then return nil end
    local ok, strokes = pcall(app.getStrokes, "selection")
    if not ok or type(strokes) ~= "table" or #strokes == 0 then return nil end
    local counts, total = {}, 0
    for _, s in ipairs(strokes) do
        local st = s and s.lineStyle
        if type(st) ~= "string" or st == "" then st = SOLID_STYLE end
        counts[st] = (counts[st] or 0) + 1
        total = total + 1
    end
    return counts, total
end

-- Đưa kiểu nét tới app. Đây chính là Control::setLineStyle() của bản build riêng:
--   * đang có vùng chọn  -> áp cho VÙNG CHỌN (một bước hoàn tác, Ctrl+Z lấy lại);
--   * không có vùng chọn -> đặt cho BÚT sắp vẽ (hành vi cũ).
-- Nhờ vậy cùng một phím dùng đư​ợc cho cả hai tình huống.
local function applyLineStyle(style)
    app.changeActionState("tool-pen-line-style", style)
end

-- Ctrl+J: đổi qua lại liền <-> đứt.
--   * Có vùng chọn (hình đã vẽ): đang ĐỨT HẾT thì về LIỀN, còn lại thì thành ĐỨT.
--   * Không có vùng chọn: đổi kiểu nét của BÚT như trước.
function vk640ToggleDashed()
    if debounced("dashed", 0.3) then return end
    local counts, total = selectionStyleCounts()
    if counts then
        -- "đứt hết" = mọi nét đều là "dash"; lẫn đứt-chấm/chấm coi như chưa đứt đều -> đưa về "dash"
        if (counts["dash"] or 0) == total then
            applyLineStyle(SOLID_STYLE)
        else
            applyLineStyle("dash")
        end
        return
    end
    local cur = currentLineStyle()
    applyLineStyle(cur == SOLID_STYLE and "dash" or SOLID_STYLE)
end

-- Ctrl+K: xoay vòng đủ 4 kiểu (có vùng chọn thì xoay kiểu nét của chính hình đó).
function vk640CycleLineStyle()
    if debounced("linestyle", 0.3) then return end
    local cur = currentLineStyle()
    local counts = selectionStyleCounts()
    if counts then
        -- lấy kiểu đang có NHIỀU NÉT NHẤT trong vùng chọn làm mốc xoay, để vòng xoay bám theo hình
        -- chứ không theo trạ​ng thái bút
        local best, bestN = cur, 0
        for st, n in pairs(counts) do
            if n > bestN then
                best, bestN = st, n
            end
        end
        cur = best
    end
    local idx = 0
    for i = 1, #LINE_STYLES do
        if LINE_STYLES[i] == cur then
            idx = i
            break
        end
    end
    applyLineStyle(LINE_STYLES[(idx % #LINE_STYLES) + 1])
end

-- ---------------------------------------------------------------------------
-- 2c. Ctrl+B / Ctrl+Shift+B: bề dày nét
-- ---------------------------------------------------------------------------
-- Bề dày lưu trong "tool-pen-size" theo chỉ số 0..4: rất mảnh, mảnh, trung bình, dày, rất dày.
-- Trên bảng vẽ, đổi bề dày bằng nút thanh công cụ cũng chậm; Ctrl+B nhanh hơn.

local MIN_SIZE, MAX_SIZE = 0, 4

function vk640Thicker()
    if debounced("thicker", 0.25) then return end
    local n = app.getActionState("tool-pen-size")
    if n == nil then return end
    if n >= MAX_SIZE then n = MIN_SIZE - 1 end   -- hết cỡ thì quay lại mảnh nhất
    app.changeActionState("tool-pen-size", n + 1)
end

function vk640Thinner()
    if debounced("thinner", 0.25) then return end
    local n = app.getActionState("tool-pen-size")
    if n == nil then return end
    if n <= MIN_SIZE then n = MAX_SIZE + 1 end   -- hết cỡ thì vòng lên dày nhất
    app.changeActionState("tool-pen-size", n - 1)
end

-- ---------------------------------------------------------------------------
-- 3. Shift+Y / Ctrl+Y: mỗi trang PDF một trang nháp cố định bên cạnh
-- ---------------------------------------------------------------------------
--
-- Cách nhận biết "trang này đã có nháp bên cạnh": trang NGAY SAU nó không có
-- nền PDF (pdfBackgroundPageNo == 0).
--
-- VÌ SAO PHẢI TÌM TỪ CUỐI VỀ ĐẦU: chèn một trang vào giữa làm mọi trang phía
-- sau dịch xuống 1 bậc. Duyệt từ cuối về đầu thì các trang chưa xét vẫn giữ
-- nguyên số thứ tự.

local function ensureNotePages()
    local struct = app.getDocumentStructure()
    local userPage = struct.currentPage

    -- đếm số trang PDF còn thiếu nháp
    local pdfCount, missing = 0, 0
    for i = 1, #struct.pages do
        if struct.pages[i].pdfBackgroundPageNo ~= 0 then
            pdfCount = pdfCount + 1
            local nxt = struct.pages[i + 1]
            if not nxt or nxt.pdfBackgroundPageNo ~= 0 then
                missing = missing + 1
            end
        end
    end

    if pdfCount == 0 then
        -- Tài liệu không có trang PDF (ví dụ trang ghi chú thường):
        -- Chèn thêm một trang nháp kẻ dòng ngay sau trang đầu cặp nếu cặp 2 cột chưa đủ 2 trang, rồi bật 2 cột
        local pairStart = (userPage % 2 == 0) and (userPage - 1) or userPage
        if pairStart < 1 then pairStart = 1 end
        local cur = struct.pages[pairStart] or struct.pages[userPage]
        local w, h = PDF_FALLBACK_W, PDF_FALLBACK_H
        if cur and cur.pageWidth and cur.pageHeight then
            w, h = cur.pageWidth, cur.pageHeight
        end
        if not struct.pages[pairStart + 1] then
            app.changeActionState("set-columns-or-rows", 1)
            app.setCurrentPage(pairStart)
            app.activateAction("new-page-after")
            app.setCurrentPage(pairStart + 1)
            app.changeCurrentPageBackground("lined")
            app.setPageSize(w, h, false)
        end
        app.changeActionState("set-columns-or-rows", 2)
        app.setCurrentPage(pairStart)
        app.scrollToPage(pairStart, false)
        return
    end

    if missing > 0 then
        -- kích thước: theo trang người dùng đang xem, không có thì A4
        local w, h = PDF_FALLBACK_W, PDF_FALLBACK_H
        local cur = struct.pages[userPage]
        if cur and cur.pageWidth and cur.pageHeight then
            w, h = cur.pageWidth, cur.pageHeight
        end

        -- đang bật 2 cột thì tắt tạm: ở chế độ 2 cột setCurrentPage(trang chẵn)
        -- bị app kéo về đầu cặp, lệnh đổi nền giấy sẽ đánh vào SAI trang.
        app.changeActionState("set-columns-or-rows", 1)

        local total = #app.getDocumentStructure().pages
        for i = total, 1, -1 do
            local pages = app.getDocumentStructure().pages
            local p = pages[i]
            if p and p.pdfBackgroundPageNo ~= 0 then
                local nxt = pages[i + 1]
                if not nxt or nxt.pdfBackgroundPageNo ~= 0 then
                    -- tới trang PDF rồi chèn trang trắng ngay sau nó.
                    -- activateAction KHÔNG truyền state -> insertNewPage(..., automatedInsertion=false)
                    -- -> clearSelectionEndText() nên không bị đổi sang công cụ chọn vùng.
                    app.setCurrentPage(i)
                    app.activateAction("new-page-after")
                    -- trang mới sao chép nền PDF của trang trước -> đổi thành giấy kẻ dòng
                    app.setCurrentPage(i + 1)
                    app.changeCurrentPageBackground("lined")
                    local pw = (p.pageWidth and p.pageWidth > 0) and p.pageWidth or w
                    local ph = (p.pageHeight and p.pageHeight > 0) and p.pageHeight or h
                    app.setPageSize(pw, ph, false)
                    if i < userPage then
                        userPage = userPage + 1
                    end
                end
            end
        end
    end

    -- bật 2 trang cạnh nhau và đứng ở đầu cặp chứa trang người dùng đang xem
    app.changeActionState("set-columns-or-rows", 2)
    local total = #app.getDocumentStructure().pages
    local back = userPage
    if back > total then back = total end
    if back < 1 then back = 1 end
    if back % 2 == 0 then back = back - 1 end   -- cặp hiển thị là (lẻ, chẵn)
    app.setCurrentPage(back)
    app.scrollToPage(back, false)
end

function vk640SplitPdfNote()
    if debounced("split", 0.4) then return end
    ensureNotePages()
end

-- Shift+Y: cùng chức năng, chốt riêng để bấm liền 2 phím khác nhau không bị nuốt
function vk640SplitPdfNoteShift()
    if debounced("splitShift", 0.4) then return end
    ensureNotePages()
end

-- ---------------------------------------------------------------------------
-- 4. Bảng phím tắt: F1, hoặc bấm nút trên thanh công cụ
-- ---------------------------------------------------------------------------
--
-- VÌ SAO CẦN: các phím tắt ở đây do plugin tự thêm, KHÔNG có trong menu Trợ giúp của app, nên
-- không có chỗ nào tra được. Trên bảng vẽ lại càng khó nhớ. Bấm nút là ra bảng.

function vk640FloatingToggle() app.floatingNotes("toggle") end
function vk640FloatingNew() app.floatingNotes("new") end
function vk640FloatingLibrary() app.floatingNotes("toggle-library") end

local SHORTCUT_HELP = [[
PHÍM TẮT — BẢNG VẼ VK640

── SỔ NHÁP NỔI (native, lưu trong .xopp) ──
  Alt + N          Gọi/cất nhóm nháp gần nhất, giữ nguyên mực
  Ctrl + Alt + N   Tạo tờ nháp mới
  Alt + K          Mở/cất Kho nháp; mở kho là có thể gõ tên để tìm
  Ctrl + `         Hiện/ẩn phím tắt dưới ba nút Nháp nổi, Tờ mới, Kho nháp
  Kéo tiêu đề      Dời khung; −/+ zoom giấy; + Giấy viết tiếp
  Ctrl + Z         Hoàn tác trên tờ đang viết
  Nút ⋯            Đổi tên, ghim, nhân bản, chép chọn sang trang chính, xuất PDF
  Cất tờ ≠ xóa     Xóa riêng trong ⋯, có Ctrl+Z / Thùng rác
  Bản Xournal++ cũ mở nháp như trang thường, lưu lại sẽ mất metadata nổi.

── BÚT & CÔNG CỤ ─────────────────────────
  Tab              Bút → Tẩy → Kéo trang → Bút
  Ctrl + `         Hiện/ẩn chữ phím tắt ngay dưới nút công cụ
  Ctrl + Tab       (giống Tab, dùng khi Tab bị nuốt)
  F8               (dự phòng cho vòng xoay công cụ)
  Nút thân bút     Bấm 1 lần là DÍNH, xoay Bút → Tẩy → Tay
  Key 1 trên bảng  Xoay Bút → Tẩy → Kéo trang

── KIỂU NÉT & BỀ DÀY ─────────────────────
  Ctrl + J         Không chọn gì: nét liền ⇄ nét đứt cho bút
                   ĐANG CHỌN hình: đổi nét liền ⇄ đứt cho CHÍNH hình đó
  Ctrl + K         Xoay vòng kiểu nét (liền → đứt → đứt-chấm → chấm)
                   đang chọn hình thì xoay kiểu nét của hình đó
  Ctrl + B         Dày hơn (hết cỡ thì vòng lại mảnh)
  Ctrl + Shift + B Mảnh hơn (hết cỡ thì vòng lên dày)
  Alt + C          Đỏ → Xanh dương → Xanh lá đậm → Đen (cả vùng chọn)
  Alt + L          Bật/tắt đoạn thẳng; tắt trở về bút tự do
  Alt + V          Bật/tắt mũi tên vectơ; tắt trở về bút tự do
  Alt + S          Chèn sóng sin 1 chu kỳ, chọn sẵn để kéo giãn
  Alt + Shift + S  Chèn sóng sin 2 chu kỳ, Ctrl+Z để bỏ
  (hoặc bấm 4 nút kiểu nét trên thanh công cụ)

── LẤY ẢNH TỪ VÙNG ĐÃ CHỌN (COPY) ────────
  Ctrl + Shift + C Quét vùng đang chọn thành MỘT ẢNH:
                   ảnh kéo để dời, núm để phóng to/thu nhỏ,
                   xoay và xoá như ảnh dán vào từ tệp
                   NÉT GỐC VẪN CÒN (copy, không cắt đi)
                   mu​ốn dán sang chỗ khác: Ctrl+C rồi Ctrl+V
                   Ctrl+Z bỏ ảnh vừa tạo

── GHI TÊN ĐỈNH HÌNH KHỐI ────────────────
  Ctrl + Shift + N Chọn cả hình rồi bấm: tự ghi tên đỉnh
                   - Hình hộp: A, B, C, D (mặt trước) và A', B', C', D' (mặt sau)
                   - Hình chóp: A, B, C, D (đáy) và S (đỉnh chóp)
                   - Chóp tam giác: S.ABC; lăng trụ tam giác: ABC.A'B'C'
                   - Tam giác / tứ giác: A, B, C (, D)
                   (chỉ nhận hình do công cụ vẽ; Ctrl+Z xoá cả loạt tên)

── PDF + GIẤY NHÁP ───────────────────────
  Shift + Y        Mỗi trang PDF một trang giấy kẻ dòng bên cạnh
  Ctrl + Y         (giống Shift+Y)
  Ctrl + U         XOÁ HẾT nét bút trên trang đang xem
                   (giấy sạch để giảng tiếp; ảnh, chữ, công thức
                    và nền giấy kẻ dòng vẫn còn nguyên; Ctrl+Z lấy lại)
  Ctrl + Shift + M XOÁ MỰC trên TOÀN BỘ trang giấy nháp (1 lần bấm)
                   (trang, ảnh dán, chữ viết tay vẫn còn; Ctrl+Z trả lại
                    cả loạt trong MỘT bước — nên không hỏi lại)
                   Mu​ốn xoá hẳn cả TRANG nháp: menu plugin
                   "VK640: xoa HAN cac trang giay nhap"
  Alt + `          Đổi 1 trang ⇄ 2 trang cạnh nhau

── VẼ HÌNH (bấm nút trên thanh công cụ) ──
  Vẽ Đường thẳng   Giữ Shift = đường thẳng theo góc 45°
  Vẽ Tam giác      Giữ Shift = tam giác ĐỀU
  Vẽ hình chữ nhật Giữ Shift = hình vuông
  Vẽ hình hộp      Shift = lập phương; Ctrl = lăng trụ đứng tam giác
  Vẽ hình chóp     Thường = chóp tứ giác; Shift = chóp tam giác S.ABC
                   Ctrl = chóp có SA vuông góc đáy ABCD
  Hệ trục          Thường = Oxy bốn góc; Shift = Oxyz; Ctrl = góc I
  Vẽ Elip          Giữ Shift = hình tròn
  Ê ke / Com-pa    Kéo để di chuyển, 2 ngón để xoay/phóng to

── NỐI ĐIỂM (tự động) ────────────────────
  Hút đầu mút, góc, trung điểm cạnh dài và tâm/4 cực trị elip
  Đưa bút tới gần điểm cần nối của nét đã vẽ
  → tự hút vào điểm đó (hiện vòng tròn xanh)
  Giữ Alt          Tạm tắt hút điểm
  Bán kính hút     Cài đặt → Bút viết → Snap to elements

── HAY DÙNG KHÁC ─────────────────────────
  Ctrl + Z         Hoàn tác
  Ctrl + Shift + Z Làm lại  ⚠ Ctrl+Y đã bị plugin dùng, xem ghi chú dưới
  Ctrl + S         Lưu
  Ctrl + O         Mở file
  Ctrl + P         In / Xuất PDF (menu Tệp → Xuất PDF)
  Ctrl + E         Xuất file
  Thêm/Chỉnh sửa TeX: gõ công thức LaTeX (∫, vectơ, đồ thị)

⚠ LƯU Ý: Ctrl+Y do plugin dùng cho "chia PDF + giấy nháp"
  nên KHÔNG còn là "Làm lại" như thông thường. Muốn Làm lại
  thì dùng Ctrl+Shift+Z hoặc nút ↷ trên thanh công cụ.

⚠ LƯU Ý: Shift+Y làm chức năng chia PDF, nên gõ chữ "Y" HOA
  bằng bàn phím sẽ kích hoạt chức năng này. Dùng Ctrl+Y nếu ngại.

Bấm F1 lúc nào cũng mở lại bảng này.
]]

function vk640ShortcutHelp()
    app.openDialog(SHORTCUT_HELP, { "Đóng" }, "", false)
end


-- ---------------------------------------------------------------------------
-- 4b. Ctrl+U: XOÁ HẾT NÉT BÚT TRÊN TRANG ĐANG XEM
-- ---------------------------------------------------------------------------
--
-- VÌ SAO CẦN: giảng xong một trang, muốn tờ giấy sạch để viết tiếp mà KHÔNG mất đề bài và
-- không mất hình/ảnh đã dán. Tẩy thì phải tẩy từng nét, xoá vùng chọn thì phải kéo chọn cả
-- trang — một phím nhanh hơn nhiều khi đang cầm bút.
--
-- CHỈ XOÁ NÉT BÚT (nét do bút vẽ ra). Ảnh, chữ, công thức LaTeX và nền giấy (kẻ dòng /
-- trang PDF) giữ nguyên — đúng những thứ cần giữ lại khi dạy.
--
-- Toàn bộ việc xoá nằm trong MỘT bước hoàn tác, nên bấm nhầm thì Ctrl+Z lấy lại hết.
--
-- Gọi app.deleteStrokes() — hàm này do bản build riêng thêm vào Lua API (không có trong
-- Xournal++ gốc). Bản app chưa có hàm đó thì nói rõ, chứ không im lặng mà hỏng.

function vk640EraseInk()
    if debounced("eraseInk", 0.5) then return end

    if type(app.deleteStrokes) ~= "function" then
        app.openDialog("Bản Xournal++ đang chạy KHÔNG có hàm app.deleteStrokes().\n"
                       .. "Cần cài lại bản build riêng (nguồn ở ~/xpp-src) rồi MỞ LẠI app.",
                       {"Đã hiểu"}, "", true)
        return
    end

    local n = app.deleteStrokes("page")
    -- Thành công thì KHÔNG hiện hộp thoại: đang giảng bài, giấy sạch là thấy ngay, hỏi thêm
    -- chỉ làm vướng tay. Chỉ khi không có nét nào để xoá mới nói một câu, để biết phím vẫn ăn.
    if n == 0 then
        app.openDialog("Trang này chưa có nét bút nào để xoá.", {"Đã hiểu"}, "", false)
    end
end


-- ---------------------------------------------------------------------------
-- 4c. Ctrl+Shift+C: LẤY ẢNH VÙNG ĐÃ CHỌN
-- ---------------------------------------------------------------------------
--
-- VÌ SAO CẦN: nét vẽ là các nét RỜI. Công cụ chọn cho di chuyển/phóng to, nhưng vẫn là một bó nét:
-- sửa một nét là "nhóm" tan, và không thể xoá/dời cả hình như một vật. Quét vùng đã chọn thành ẢNH
-- biến hình thành MỘT vật giống hệt ảnh dán từ tệp — dời, phóng to/thu nhỏ, xoay, xoá đều theo khối.
--
-- CÁCH DÙNG: khoanh vùng bằng công cụ CHỌN VÙNG rồi bấm Ctrl+Shift+C.
--   * Nét gốc VẪN CÒN NGUYÊN (đây là COPY, không phải cắt lấy đi) — đúng như yêu cầu: "bôi vào đoạn
--     ảnh trong file PDF rồi copy dán qua chỗ khác". Mu​ốn ảnh THAY nét (cắt) thì gọi mode = "move".
--   * Ảnh mới đư​ợc TỰ ĐỘNG CHỌN SẴN: kéo/núm chỉnh ngay; mu​ốn dán sang chỗ khác thì Ctrl+C rồi Ctrl+V.
--   * Cả thao tác là MỘT bước hoàn tác: Ctrl+Z bỏ ảnh (nét gốc chưa hề bị đụng tới).
--   * Ảnh vẽ ở 300 dpi nên phóng to vẫn nét, không vỡ như chụp màn hình.
--
-- KHÔNG dùng Ctrl+Shift+U (app giữ cho công cụ Link). Ctrl+Shift+C đã rà: app không dùng, GNOME
-- không đặt, fcitx5 không chiếm (trigger của fcitx5 là Super+space và Shift_L).

function vk640SelectionToImage()
    if debounced("selImg", 0.5) then return end

    if type(app.selectionAsImage) ~= "function" then
        app.openDialog("Bản Xournal++ đang chạy KHÔNG có hàm app.selectionAsImage().\n"
                       .. "Cần cài lại bản build riêng (nguồn ở ~/xpp-src) rồi MỞ LẠI app.",
                       {"Đã hiểu"}, "", true)
        return
    end

    -- mode = "copy": nét gốc giữ nguyên, ảnh đư​ợc chồng lên đúng chỗ (mặc định của hàm cũng là "copy").
    local ok, n = pcall(app.selectionAsImage, { mode = "copy" })
    if not ok then
        app.openDialog("Không lấy được ảnh: " .. tostring(n), {"Đã hiểu"}, "", true)
        return
    end
    if n == 0 then
        app.openDialog("Chưa có vùng nào đang được chọn.\n"
                       .. "Bấm nút CHỌN VÙNG trên thanh công cụ, khoanh vùng muốn lấy ảnh rồi bấm lại "
                       .. "Ctrl+Shift+C.",
                       {"Đã hiểu"}, "", false)
    end
    -- Thành công thì không hỏi gì thêm: ảnh đã hiện ra và đang được chọn sẵn.
end

-- ---------------------------------------------------------------------------
-- 4d. Ctrl+Shift+N: GHI TÊN ĐỈNH CHO HÌNH ĐANG CHỌN
-- ---------------------------------------------------------------------------
--
-- VÌ SAO CẦN: khi giảng phải gọi tên đỉnh ("so sánh AB với A'B'"), mà tự đặt 4-8 chữ cái bằng tay
-- thì mất vài phút mỗi hình và hay lệch. Công cụ vẽ đã biết CHÍNH XÁC vị trí các đỉnh nên cứ đặt
-- chữ vào đó.
--
-- CÁCH DÙNG: vẽ xong hình (hoặc khoanh chọn hình bằng công cụ CHỌN VÙNG) rồi bấm Ctrl+Shift+N.
--   * Hình hộp  -> A, B, C, D ở mặt TRƯỚC; A', B', C', D' ở mặt sau (đúng cặp đỉnh nối với nhau).
--   * Hình chóp -> A, B, C, D ở đáy; S ở đỉnh chóp.
--   * Tam giác / tứ giác -> A, B, C (, D).
--   * Hình lạ (đường tròn, nét ngoằn ngoèo...) -> KHÔNG ghi gì và báo rõ, vì ghi sai còn tệ hơn.
--   * Cả loạt tên là MỘT bước hoàn tác (Ctrl+Z xoá hết một lần).
-- Không dùng Ctrl+Shift+L (app đã giữ), Ctrl+Shift+N đã rà: không menu nào của app dùng.

function vk640LabelVertices()
    if debounced("labelVerts", 0.5) then return end

    if type(app.labelSelectionVertices) ~= "function" then
        app.openDialog("Bản Xournal++ đang chạy KHÔNG có hàm app.labelSelectionVertices().\n"
                       .. "Cần cài lại bản build riêng (nguồn ở ~/xpp-src) rồi MỞ LẠI app.",
                       {"Đã hiểu"}, "", true)
        return
    end

    local ok, n = pcall(app.labelSelectionVertices, { fontSize = 16 })
    if not ok then
        app.openDialog("Không ghi được tên đỉnh: " .. tostring(n), {"Đã hiểu"}, "", true)
        return
    end
    if n == 0 then
        app.openDialog("Không nhận ra hình nào trong vùng đang chọn (hoặc hình vừa vẽ).\n"
                       .. "Hãy vẽ một hình (hình hộp / hình chóp / lăng trụ tam giác / tam giác / tứ giác) hoặc dùng công cụ "
                       .. "CHỌN VÙNG khoanh cả hình rồi bấm lại Ctrl+Shift+N.",
                       {"Đã hiểu"}, "", false)
    end
    -- Ghi được thì không báo gì: chữ hiện ra ngay trên hình.
end

-- ---------------------------------------------------------------------------
-- 5. Ctrl+Shift+M: XOÁ MỰC TRÊN TOÀN BỘ TRANG GIẤY NHÁP
-- ---------------------------------------------------------------------------
--
-- Đối xứ​ng với Shift+Y / Ctrl+Y ở trên: lệnh đó TẠO một trang giấy kẻ dòng sau mỗi trang PDF,
-- lệnh này DỌN MỰC trên tất cả các trang nháp đó trong một lần bấm.
--
-- ⚠ Ý NGHĨA ĐÃ ĐƯ​ỢC SỬA (phản hồi trực tiếp của Phúc): "xoá giấy nháp" = XOÁ NÉT BÚT ĐÃ VẼ,
-- KHÔNG phải xoá trang. Bản trư​ớc gọi app.activateAction("delete-page") — sai ý, vì nó gỡ cả
-- trang (mất luôn ảnh/chữ đã dán trên đó) và đổi số trang của tài liệu. Giờ chỉ xoá NÉT BÚT,
-- đ​úng như Ctrl+U nhưng áp cho MỌI trang nháp thay vì trang đang xem.
--
-- "Trang giấy nháp" = MỌI trang không có nền PDF (pdfBackgroundPageNo == 0) mà nền là giấy kẻ
-- dòng (pageTypeFormat == "lined"). Trang trắng của b​ạn và mọi trang có nền PDF đều không bị đụng.
--
-- Cài đặt: app.deleteStrokes("notePages") — nhánh mới thêm trong bản build riêng
-- (~/xpp-src, hàm getElementsFromHelper). Ưu điểm so với việc tự đi từ​ng trang bằng Lua:
--   * CẢ LOẠT nét (mọi trang, mọi lớp) nằm trong MỘT bước hoàn tác -> Ctrl+Z trả lại hết một lần;
--   * không phải đổi trang đang xem, nên màn hình không nhảy qua nhảy lại;
--   * không mở hộp thoại xác nhận: xoá mực lấy lại đư​ợc bằng Ctrl+Z, còn giáo viên đang cầm bút.

-- (Hàm đư​ợc đặt SAU notePageCounts vì nó dùng hàm đó — xem bên dư​ới.)

local function linedPageTypeOf(p)
    -- pageTypeFormat: "lined" | "plain" | "pdf" | "image" | "custom"
    local fmt = p and p.pageTypeFormat
    if fmt == nil then return nil end
    return tostring(fmt)
end

--- @return number soTrangNhap, number soTrangPdf
local function notePageCounts()
    local pages = app.getDocumentStructure().pages
    local soNhap, soPdf = 0, 0
    for i = 1, #pages do
        local p = pages[i]
        if p.pdfBackgroundPageNo ~= 0 then
            soPdf = soPdf + 1
        elseif linedPageTypeOf(p) == "lined" then
            soNhap = soNhap + 1
        end
    end
    return soNhap, soPdf
end

-- Ctrl+Shift+M: xoá MỰC trên mọi trang giấy nháp (giữ nguyên trang, giữ ảnh/chữ đã dán).
function vk640EraseNotePagesInk()
    if debounced("eraseNotes", 0.5) then return end

    if type(app.deleteStrokes) ~= "function" then
        app.openDialog("Bản Xournal++ đang chạy không có app.deleteStrokes().\n"
                       .. "Cần bản build riêng (nguồn ở ~/xpp-src), rồi MỞ LẠI app.",
                       {"Đã hiểu"}, "", true)
        return
    end

    local soNhap = notePageCounts()
    if soNhap == 0 then
        app.openDialog("Chưa có trang giấy nháp nào để xoá mực.\n"
                       .. "Tạo bằng Ctrl+Y (mỗi trang PDF kèm một trang kẻ dòng) rồi bấm lại.",
                       {"Đã hiểu"}, "", false)
        return
    end

    -- pcall: bản app CŨ hơn nguồn ~/xpp-src thì getElementsFromHelper không biết kiểu "notePages"
    -- và app.deleteStrokes NÉM LỖI Lua; bắt lại để báo rõ, chứ không im lặng mà hỏng.
    local ok, n, err = pcall(app.deleteStrokes, "notePages")
    if not ok then
        app.openDialog("Bản app đang chạy chưa biết kiểu \"notePages\" (cũ hơn nguồn ~/xpp-src).\n"
                       .. "Cần cài lại bản build riêng rồi mở lại app.\n\nChi tiết: " .. tostring(n),
                       {"Đã hiểu"}, "", true)
        return
    end
    if n == nil then
        app.openDialog("Không xoá đư​ợc mực giấy nháp: " .. tostring(err or "lỗi không rõ"),
                       {"Đã hiểu"}, "", true)
        return
    end
    if n == 0 then
        -- Chỉ nói một câu khi không có nét nào để xoá, để giáo viên biết là phím vẫn ăn.
        app.openDialog(string.format("Có %d trang giấy nháp như​ng chưa có nét bút nào trên đó.", soNhap),
                       {"Đã hiểu"}, "", false)
    end
    -- n > 0: KHÔNG báo gì. Giấy sạch là thấy ngay, và Ctrl+Z trả lại cả loạt trong MỘT bước.
end

-- (Chỉ nằm trong MENU plugin, KHÔNG gắn phím tắt.) XOÁ HẲN cả loạt TRANG giấy nháp.
-- Dùng khi dạy xong không mu​ốn mang giấy nháp theo file, hoặc tay kéo nhầm làm dư trang.
--
-- Bước 1: HỎI trước khi xoá. Xoá cả loạt TRANG khó lấy lại bằng tay (khác hẳn xoá mực), nên
-- KHÔNG xoá ngay. Trư​ớc đây lệnh này nằm ở Ctrl+Shift+M nhưng đã trả phím đó về đúng nghĩa
-- "xoá mực" theo phản hồi của Phúc.
function vk640DeleteAllNotePages()
    if debounced("removeNotes", 0.5) then return 0, 0 end

    local soNhap, soPdf = notePageCounts()
    if soNhap == 0 then
        app.openDialog("Không có trang giấy nháp nào để xoá.\n"
                       .. "Bạn đang có " .. soPdf .. " trang PDF và không có trang giấy kẻ dòng nào cả.\n"
                       .. "(Tạo lại bằng Ctrl+Y hoặc Shift+Y.)",
                       {"Đã hiểu"}, "", false)
        return 0, 0
    end

    app.openDialog(string.format("Xoá %d trang giấy nháp?\n"
                       .. "Giữ nguyên %d trang PDF và mọi trang trắng.\n"
                       .. "Lỡ tay bấm thì chọn Huỷ.", soNhap, soPdf),
                   {"Xoá hết", "Huỷ"}, "vk640DeleteAllNotePagesAnswer", false)
    return soNhap, soPdf
end

-- Bước 2: người dùng đã bấm nút trong hộp thoại (1 = Xoá hết, 2 = Huỷ).
function vk640DeleteAllNotePagesAnswer(button)
    if button ~= 1 then return end

    local soNhap, soPdf = notePageCounts()

    -- Xoá TỪ CUỐI VỀ ĐẦU: xoá trang i làm mọi trang phía sau lùi xuống 1 bậc, nên duyệt
    -- từ cuối về đầu giữ cho mọi chỉ số chưa xét vẫn đúng.
    local pages = app.getDocumentStructure().pages
    for i = #pages, 1, -1 do
        local cur = app.getDocumentStructure().pages
        local p = cur[i]
        if p and p.pdfBackgroundPageNo == 0 and linedPageTypeOf(p) == "lined" then
            app.setCurrentPage(i)
            app.activateAction("delete-page")
        end
    end

    -- Đứng lại trang đầu tiên còn lại; tài liệu luôn phải còn ít nhất 1 trang
    local total = #app.getDocumentStructure().pages
    if total >= 1 then
        app.setCurrentPage(1)
        app.scrollToPage(1, false)
    end

    app.openDialog(string.format("Đã xoá %d trang giấy nháp.\n"
                       .. "Còn lại %d trang PDF và %d trang khác.\n"
                       .. "Tạo lại thì bấm Ctrl+Y / Shift+Y. Lỡ tay thì Ctrl+Z lấy lại được.",
                       soNhap, soPdf, math.max(0, total - soPdf)),
                   {"Đã hiểu"}, "", false)
end

-- Công cụ dạy Toán/Lý: mỗi phím có chốt chống lặp riêng.
local TEACHING_COLORS = { 0xff0000, 0x0000ff, 0x008000, 0x000000 }

function vk640CycleColor()
    if debounced("color", 0.3) then return end
    local color = app.getToolInfo("pen").color
    local index = 0
    for i, value in ipairs(TEACHING_COLORS) do
        if value == color then index = i; break end
    end
    -- Giữ nguyên vùng chọn để đổi màu cả hình; không đổi công cụ trước khi tô.
    app.changeToolColor{ tool = "pen", color = TEACHING_COLORS[index % #TEACHING_COLORS + 1], selection = true }
end

local function toggleDrawing(action, key)
    if debounced(key, 0.3) then return end
    local enabled = app.getActionState(action) == true
    app.changeActionState("select-tool", C.Tool_pen)
    app.changeActionState(action, not enabled)
end

function vk640ToggleLine() toggleDrawing("tool-draw-line", "line") end
function vk640ToggleArrow() toggleDrawing("tool-draw-arrow", "arrow") end

local function insertSine(cycles)
    if debounced("sine" .. cycles, 0.4) then return end
    local structure = app.getDocumentStructure()
    local page = structure.pages[structure.currentPage]
    if not page then return end
    local visible = type(app.getVisiblePageRect) == "function" and app.getVisiblePageRect() or nil
    local frame = visible or { x = 0, y = 0, width = page.pageWidth, height = page.pageHeight }
    local width = math.min(360, frame.width * 0.7)
    local amplitude = math.min(50, frame.height * 0.12)
    local left = frame.x + (frame.width - width) / 2
    local middle = frame.y + frame.height * 0.4
    local x, y = {}, {}
    -- 128 segments/cycle keep the waveform smooth when enlarged on the board.
    local samples = 128 * cycles
    for i = 0, samples do
        x[i + 1] = left + width * i / samples
        y[i + 1] = middle - amplitude * math.sin(2 * math.pi * cycles * i / samples)
    end
    app.clearSelection()
    local refs = app.addStrokes{ strokes = {{ x = x, y = y, tool = "pen", fill = -1, lineStyle = "plain" }},
                                 allowUndoRedoAction = "grouped" }
    app.changeActionState("select-tool", C.Tool_selectRect)
    app.addToSelection(refs)
end

function vk640SineOne() insertSine(1) end
function vk640SineTwo() insertSine(2) end

-- ---------------------------------------------------------------------------
-- đăng ký
-- ---------------------------------------------------------------------------

function vk640MathTools()
    app.showMathTools()
end

function initUi()
    -- Tab trần (giữ theo yêu cầu); Ctrl+Tab là bản CÓ MODIFIER vì nhóm phím
    -- có Ctrl/Shift chạy ổn định hơn hẳn Tab trần trên máy này; F8 là dự phòng.
    app.registerUi{
        menu = "VK640: Tab doi But - Tay - Keo trang",
        callback = "vk640CycleTool",
        accelerator = "Tab",
    }
    app.registerUi{
        menu = "VK640: Ctrl+Tab doi But - Tay - Keo trang",
        callback = "vk640CycleToolCtrl",
        accelerator = "<Control>Tab",
    }
    app.registerUi{
        menu = "VK640: F8 doi But - Tay - Keo trang (du phong)",
        callback = "vk640CycleToolF8",
        accelerator = "F8",
    }
    app.registerUi{
        menu = "VK640: Alt+` doi 1 trang / 2 trang",
        callback = "vk640ToggleColumns",
        accelerator = "<Alt>grave",
        toolbarId = "TOGGLE_COLUMNS",
        iconName = "xopp-vk640-columns",
    }
    -- Shift+Y = phím chính user yêu cầu; Ctrl+Y giữ thêm cho quen tay
    app.registerUi{
        menu = "VK640: Shift+Y chia doi PDF + giay nhap",
        callback = "vk640SplitPdfNoteShift",
        accelerator = "<Shift>y",
        toolbarId = "SPLIT_PDF_NOTE",
        iconName = "xopp-vk640-split",
    }
    app.registerUi{
        menu = "VK640: Ctrl+Y chia doi PDF + giay nhap",
        callback = "vk640SplitPdfNote",
        accelerator = "<Control>y",
    }
    -- Nét đứt: Ctrl+J đổi qua lại liền <-> đứt (dùng nhiều nhất); Ctrl+K xoay vòng 4 kiểu
    app.registerUi{
        -- Ctrl+J chứ không phải Ctrl+D: app đã chiếm Ctrl+D (Trang mới phía sau) và
        -- Ctrl+Shift+D (Công cụ mặc định); GTK cho action đăng ký TRƯỚC thắng nên phím đó
        -- không bao giờ tới plugin.
        menu = "VK640: Ctrl+J net dut / net lien",
        callback = "vk640ToggleDashed",
        accelerator = "<Control>j",
        toolbarId = "TOGGLE_DASHED",
        iconName = "xopp-vk640-dashed",
    }
    app.registerUi{
        menu = "VK640: Ctrl+K xoay 4 kieu net",
        callback = "vk640CycleLineStyle",
        accelerator = "<Control>k",
    }
    -- Bề dày nét: Ctrl+B dày hơn, Ctrl+Shift+B mảnh hơn
    app.registerUi{
        menu = "VK640: Ctrl+B net day hon",
        callback = "vk640Thicker",
        accelerator = "<Control>b",
    }
    app.registerUi{
        menu = "VK640: Ctrl+Shift+B net manh hon",
        callback = "vk640Thinner",
        accelerator = "<Control><Shift>b",
    }
    -- Ctrl+U: XOÁ HẾT nét bút trên trang đang xem (giữ ảnh, giữ nền giấy). Đã rà: không menu
    -- nào của app dùng phím này; cả việc xoá là MỘT bước hoàn tác nên Ctrl+Z lấy lại được.
    app.registerUi{
        menu = "VK640: Ctrl+U xoa het net but tren trang",
        callback = "vk640EraseInk",
        accelerator = "<Control>u",
        toolbarId = "ERASE_INK",
        iconName = "xopp-vk640-erase-ink",
    }
    -- Ctrl+Shift+M: XOÁ MỰC trên TOÀN BỘ trang giấy nháp (đối xứ​ng với Shift+Y ở trên).
    -- Nghĩa là xoá NÉT BÚT đã vẽ — trang, ảnh đã dán và chữ viết tay khác còn nguyên.
    -- (Bản trư​ớc gọi "delete-page": hiểu sai, đã sửa theo phản hồi trực tiếp của Phúc.)
    -- KHÔNG dùng Ctrl+Shift+U: app đã gán phím đó cho công cụ Link (menu Tools), mà với hai
    -- GAction cùng một phím thì bên ĐĂNG KÝ TRƯỚC thắng — menu của app đăng ký trước plugin.
    -- Cũng không dùng Ctrl+Shift+U vì GTK/IBus coi đó là phím nhập mã Unicode.
    -- Đã rà cả cây nguồn: Ctrl+Shift+M không menu/công cụ nào của app dùng, GNOME cũng không.
    app.registerUi{
        menu = "VK640: Ctrl+Shift+M xoa MUC tren toan bo trang giay nhap",
        callback = "vk640EraseNotePagesInk",
        accelerator = "<Control><Shift>m",
        toolbarId = "ERASE_NOTE_INK",
        iconName = "xopp-vk640-clear-notes",
    }
    -- Xoá HẲN cả loạt TRANG giấy nháp: CHỈ trong menu, KHÔNG gắn phím tắt.
    -- Ít dùng hơn, mà lỡ tay thì khó lấy lại — khác hẳn xoá mực, xoá mực chỉ cần Ctrl+Z.
    app.registerUi{
        menu = "VK640: xoa HAN cac trang giay nhap (khong co phim tat)",
        callback = "vk640DeleteAllNotePages",
    }
    -- Ctrl+Shift+N: ghi tên đỉnh (A, B, C, D / A'... / S) cho hình đang chọn
    app.registerUi{
        menu = "VK640: Ctrl+Shift+N ghi ten dinh hinh dang chon",
        toolbarId = "LABEL_VERTICES",
        iconName = "xopp-vk640-labels",
        callback = "vk640LabelVertices",
        accelerator = "<Control><Shift>n",
    }
    -- Ctrl+Shift+C: quét vùng đang chọn thành ảnh (di chuyển/phóng to/xoá như ảnh dán)
    app.registerUi{
        menu = "VK640: Ctrl+Shift+C lay anh tu vung chon",
        toolbarId = "SELECTION_IMAGE",
        iconName = "xopp-vk640-image",
        callback = "vk640SelectionToImage",
        accelerator = "<Control><Shift>c",
    }
    -- Bảng phím tắt: F1, hoặc bấm nút trên thanh công cụ (toolbarId -> Plugin::SHORTCUT_HELP)
    app.registerUi{
        menu = "VK640: xem bang phim tat",
        callback = "vk640ShortcutHelp",
        accelerator = "F1",
        toolbarId = "SHORTCUT_HELP",
        iconName = "xopp-vk640-help",
    }
    app.registerUi{ menu = "VK640: Alt+C doi mau Do - Xanh duong - Xanh la - Den", callback = "vk640CycleColor", accelerator = "<Alt>c" }
    app.registerUi{ menu = "VK640: Alt+L doan thang / but tu do", callback = "vk640ToggleLine", accelerator = "<Alt>l" }
    app.registerUi{ menu = "VK640: Alt+V mui ten vecto / but tu do", callback = "vk640ToggleArrow", accelerator = "<Alt>v" }
    app.registerUi{ menu = "VK640: Alt+S ve song sin 1 chu ky", callback = "vk640SineOne", accelerator = "<Alt>s" }
    app.registerUi{ menu = "VK640: Alt+Shift+S ve song sin 2 chu ky", callback = "vk640SineTwo", accelerator = "<Alt><Shift>s" }
    app.registerUi{
        menu = "VK640: Ctrl+` hien/an phim tat duoi nut cong cu",
        callback = "vk640ToggleShortcutHints",
        accelerator = "<Control>grave",
        toolbarId = "SHORTCUT_HINTS",
        iconName = "input-keyboard",
    }
    app.registerUi{
        menu = "Toán: đồ thị, tiếp tuyến, bảng biến thiên",
        callback = "vk640MathTools",
        toolbarId = "MATH_TOOLS",
        iconName = "accessories-calculator",
        accelerator = "<Alt>g",
    }

    app.registerUi{menu="Nháp nổi: Gọi/cất", callback="vk640FloatingToggle", accelerator="<Alt>n", toolbarId="FLOATING_NOTES", iconName="accessories-text-editor"}
    app.registerUi{menu="Nháp nổi: Tờ mới", callback="vk640FloatingNew", accelerator="<Control><Alt>n", toolbarId="FLOATING_NEW", iconName="document-new-symbolic"}
    app.registerUi{menu="Nháp nổi: Mở/cất kho", callback="vk640FloatingLibrary", accelerator="<Alt>k", toolbarId="FLOATING_LIBRARY", iconName="view-grid-symbolic"}

end
