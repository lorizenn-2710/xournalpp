--[[
    Probe kiểm thử app.deleteStrokes() chạy TRONG Xournal++.

    Vì sao cần: hàm mới phải được chứng minh bằng tài liệu THẬT, không phải bằng suy luận.
    io.open KHÔNG chạy trong Lua của app -> ghi kết quả bằng print() ra stdout của app.

    Mỗi hàm là một GAction riêng để gọi được qua D-Bus mà không cần bơm phím:
        gdbus call --session --dest <bus> --object-path /com/github/xournalpp/xournalpp \
            --method org.gtk.Actions.Activate "plugins.action-3" "[]" "{}"
    Thứ tự đăng ký dưới đây quyết định số hiệu action.
]]

local function stats(tag)
    local strokes = app.getStrokes("page")
    local images  = app.getImages("page")
    local texts   = app.getTexts("page")
    local struct  = app.getDocumentStructure()
    local nStroke, nImage, nText, nPage = 0, 0, 0, 0
    if strokes then nStroke = #strokes end
    if images  then nImage  = #images  end
    if texts   then nText   = #texts   end
    if struct and struct.pages then nPage = #struct.pages end
    print(string.format("[PROBE] %s | strokes=%d images=%d texts=%d pages=%d",
                        tag or "-", nStroke, nImage, nText, nPage))
    io.stdout:flush()
    return nStroke, nImage, nText, nPage
end

-- %d = 3 nét bút, %d ... để dễ đọc
function probeStats()
    stats("stats")
end

function probeAddStrokes()
    app.addStrokes({
        strokes = {
            { x = { 100, 200, 300 }, y = { 100, 140, 100 }, width = 2.0, color = 0x000000,
              tool = "pen", lineStyle = "plain" },
            { x = { 100, 200, 300 }, y = { 200, 240, 200 }, width = 2.0, color = 0xff0000,
              tool = "pen", lineStyle = "dash" },
            { x = { 100, 200, 300 }, y = { 300, 340, 300 }, width = 2.0, color = 0x0000ff,
              tool = "pen", lineStyle = "plain" },
        },
        allowUndoRedoAction = "grouped",
    })
    stats("after-add-strokes")
end

function probeAddImage()
    local refs = app.addImages({
        images = { { path = "/tmp/xpp-kt2/probe-img.png", x = 420, y = 420, maxWidth = 220 } },
        allowUndoRedoAction = "grouped",
    })
    local ok = refs and refs[1] and type(refs[1]) ~= "string"
    print("[PROBE] addImages -> " .. (ok and "OK" or tostring(refs and refs[1])))
    io.stdout:flush()
    stats("after-add-image")
end

function probeAddText()
    app.addTexts({
        texts = { { text = "DE BAI — khong duoc xoa", x = 80, y = 520, fontSize = 12 } },
        allowUndoRedoAction = "grouped",
    })
    stats("after-add-text")
end

function probeErasePage()
    local n = app.deleteStrokes("page")
    print("[PROBE] app.deleteStrokes(\"page\") -> " .. tostring(n))
    io.stdout:flush()
    stats("after-erase-page")
end

function probeEraseAll()
    local n = app.deleteStrokes("all")
    print("[PROBE] app.deleteStrokes(\"all\") -> " .. tostring(n))
    io.stdout:flush()
    stats("after-erase-all")
end

function probeEraseSelection()
    local ok, n = pcall(app.deleteStrokes, "selection")
    print("[PROBE] app.deleteStrokes(\"selection\") -> ok=" .. tostring(ok) .. " n=" .. tostring(n))
    io.stdout:flush()
    stats("after-erase-selection")
end

function probeEraseBadArg()
    local ok, e = pcall(app.deleteStrokes, "khong-co-loai-nay")
    print("[PROBE] app.deleteStrokes(\"khong-co-loai-nay\") -> ok=" .. tostring(ok)
          .. " err=" .. tostring(e))
    io.stdout:flush()
end

function probeUndo()
    app.activateAction("undo")
    stats("after-undo")
end

function probeRedo()
    app.activateAction("redo")
    stats("after-redo")
end

function probeUndoTwice()
    app.activateAction("undo")
    app.activateAction("undo")
    stats("after-undo-twice")
end

-- Thêm một lớp mới ở TRÊN lớp hiện tại và chuyển sang lớp đó (không vẽ gì lên).
-- Dùng để chứng minh việc xoá nét đi TÌM ĐÚNG LỚP: nét nằm ở lớp 1 trong khi lớp đang chọn là
-- lớp 2 — bản code chỉ nhìn "lớp đang chọn" sẽ không tìm thấy nét nào và bỏ sót sạch.
function probeAddLayer()
    app.activateAction("layer-new-above-current")
    stats("after-add-layer")
end

-- ===================== kiểm thử app.selectionAsImage() =====================
-- LƯU Ý: getStrokes("page")/getImages("page") sẽ HUỶ vùng chọn đang có (để trả các phần tử về lớp),
-- nên khi cần đo TRONG lúc đang có vùng chọn thì phải dùng "selection" (không huỷ).

local function selStats(tag)
    local okS, s = pcall(app.getStrokes, "selection")
    local okI, i = pcall(app.getImages, "selection")
    print(string.format("[PROBE] %s | ĐANG CHỌN: strokes=%s images=%s",
                        tag or "-", okS and #s or ("(không có vùng chọn)"), okI and #i or "-"))
    io.stdout:flush()
end

-- Vẽ 1 "hình" (3 nét) + dán 1 ảnh, rồi CHỌN 3 nét vừa vẽ (ảnh để ngoài vùng chọn).
function probeFigAndSelect()
    local refs = app.addStrokes({
        strokes = {
            { x = { 150, 400, 400, 150 }, y = { 150, 150, 320, 320 }, width = 2.0, color = 0x000000,
              tool = "pen", lineStyle = "plain" },
            { x = { 150, 400 }, y = { 150, 320 }, width = 2.0, color = 0x000000, tool = "pen" },
            { x = { 400, 150 }, y = { 150, 320 }, width = 2.0, color = 0x000000, tool = "pen" },
        },
        allowUndoRedoAction = "grouped",
    })
    app.addToSelection(refs)
    print("[PROBE] đã vẽ 3 nét và CHỌN chúng (1 ảnh nằm ngoài vùng chọn)")
    io.stdout:flush()
    selStats("fig-and-select")
end

function probeSelAsImageMove()
    local ok, n = pcall(app.selectionAsImage, { mode = "move" })
    print("[PROBE] selectionAsImage{move} -> ok=" .. tostring(ok) .. " n=" .. tostring(n))
    io.stdout:flush()
    selStats("after-move (ảnh mới phải đang được chọn)")
    stats("after-move (trang)")
end

function probeSelAsImageCopy()
    local ok, n = pcall(app.selectionAsImage, { mode = "copy" })
    print("[PROBE] selectionAsImage{copy} -> ok=" .. tostring(ok) .. " n=" .. tostring(n))
    io.stdout:flush()
    selStats("after-copy")
    stats("after-copy (trang)")
end

function probeSelAsImagePlain()
    -- gọi không có tham số: mode mặc định phải là "copy" (GIỮ nét gốc) — đã sửa theo yêu cầu của Phúc:
    -- "không phải là cắt lấy đi mà là copy". Sau lệnh này: số nét TRÊN TRANG phải NGUYÊN 3, số ảnh +1.
    local ok, n = pcall(app.selectionAsImage)
    print("[PROBE] selectionAsImage() (không tham số) -> ok=" .. tostring(ok) .. " n=" .. tostring(n))
    io.stdout:flush()
    selStats("after-plain")
    stats("after-plain (trang)")
end

function probeSelAsImageNoSelection()
    app.clearSelection()
    local ok, n = pcall(app.selectionAsImage, { mode = "move" })
    print("[PROBE] selectionAsImage (mode=\"move\") khi KHÔNG có vùng chọn -> ok=" .. tostring(ok) .. " n=" .. tostring(n))
    io.stdout:flush()
end

function probeSelectAll()
    -- chọn mọi nét trên trang (dùng addToSelection với ref của getStrokes)
    local strokes = app.getStrokes("page")
    if not strokes or #strokes == 0 then
        print("[PROBE] trang không có nét nào để chọn")
        io.stdout:flush()
        return
    end
    local refs = {}
    for i = 1, #strokes do refs[i] = strokes[i].ref end
    app.addToSelection(refs)
    print("[PROBE] đã chọn " .. #refs .. " nét trên trang")
    io.stdout:flush()
    selStats("select-all")
end

function probeImageGeom()
    local imgs = app.getImages("page")
    print("[PROBE] số ảnh trên trang: " .. (#imgs or 0))
    for i = 1, #imgs do
        local im = imgs[i]
        print(string.format("[PROBE]   ảnh %d: x=%.3f y=%.3f w=%.3f h=%.3f | pixel %dx%d",
                            i, im.x, im.y, im.width, im.height, im.imageWidth, im.imageHeight))
    end
    io.stdout:flush()
end

-- Đổ dữ liệu PNG của ảnh đầu tiên ra log dạng hex, để phía ngoài giải mã và SOI TỪNG ĐIỂM ẢNH
-- (chứng minh ảnh tạo ra có chứa nét vẽ thật, không phải khung trắng).
function probeDumpImageHex()
    local imgs = app.getImages("page")
    if not imgs or #imgs == 0 then
        print("[PROBE] không có ảnh để đổ")
        io.stdout:flush()
        return
    end
    local data = imgs[1].data
    local hex = (data:gsub(".", function(c) return string.format("%02x", string.byte(c)) end))
    print("[PROBE] PNG bytes=" .. #data .. " PNGMAGIC=" ..
          (data:sub(1, 8) == "\137PNG\13\10\26\10" and "OK" or "SAI"))
    print("PNGHEX-BEGIN")
    print(hex)
    print("PNGHEX-END")
    io.stdout:flush()
end

-- ===================== kiểm thử app.labelSelectionVertices() =====================
function probeLabelVertices()
    local ok, n = pcall(app.labelSelectionVertices, { fontSize = 16 })
    print("[PROBE] labelSelectionVertices -> ok=" .. tostring(ok) .. " số chữ = " .. tostring(n))
    io.stdout:flush()
    local texts = app.getTexts("page")
    print("[PROBE] số chữ trên trang: " .. (#texts or 0))
    for i = 1, #texts do
        local t = texts[i]
        print(string.format("[PROBE]   chữ %d: '%s' tại (%.2f, %.2f) cỡ %.1f",
                            i, t.text, t.x, t.y, t.font and t.font.size or -1))
    end
    io.stdout:flush()
end

function probeAddFakeBox()
    -- Hình hộp dựng sẵn ĐÚNG dạng mà công cụ vẽ phát ra (đường bao 17 điểm đi vòng Euler + 3 nét đứt),
    -- để kiểm thử phần đặt tên mà không cần bơm chuột. Hình học thật đã được kiểm ở
    -- tests/test-solid-labels.cpp (liên kết module SolidGeometry thật).
    -- Mặt trước: BA(200,560) BB(380,560) BCC(380,500) BDD(200,500); vector sâu v = (80,-120)
    local outline = {
        x = { 200, 280, 460, 460, 280, 280, 200, 380, 460, 380, 380, 460, 380, 200, 280, 200, 200 },
        y = { 560, 440, 440, 380, 380, 440, 560, 560, 440, 560, 500, 380, 500, 500, 380, 500, 560 },
    }
    app.addStrokes({
        strokes = {
            { x = outline.x, y = outline.y, width = 2.0, color = 0x000000, tool = "pen", lineStyle = "plain" },
            -- 3 cạnh khuất, đều gặp ở góc khuất BDD2 (280,380)
            { x = { 200, 280 }, y = { 500, 380 }, width = 2.0, color = 0x000000, tool = "pen", lineStyle = "dash" },
            { x = { 460, 280 }, y = { 380, 380 }, width = 2.0, color = 0x000000, tool = "pen", lineStyle = "dash" },
            { x = { 280, 280 }, y = { 380, 440 }, width = 2.0, color = 0x000000, tool = "pen", lineStyle = "dash" },
        },
        allowUndoRedoAction = "grouped",
    })
    print("[PROBE] đã thêm 1 hình hộp (đường bao + 3 nét đứt)")
    io.stdout:flush()
    stats("after-fake-box")
end

function probeAddFakePyramid()
    -- Đáy: P1(200,560) P2(400,560) P3(480,440) P4(280,440); đỉnh chóp APEX(340,240)
    local outline = {
        x = { 200, 400, 480, 280, 200, 280, 340, 480, 340, 400, 340, 200, 340 },
        y = { 560, 560, 440, 440, 560, 440, 240, 440, 240, 560, 240, 560, 240 },
    }
    app.addStrokes({
        strokes = {
            { x = outline.x, y = outline.y, width = 2.0, color = 0x000000, tool = "pen", lineStyle = "plain" },
            -- cạnh đáy xa (khuất): P3-P4
            { x = { 480, 280 }, y = { 440, 440 }, width = 2.0, color = 0x000000, tool = "pen", lineStyle = "dash" },
        },
        allowUndoRedoAction = "grouped",
    })
    print("[PROBE] đã thêm 1 hình chóp")
    io.stdout:flush()
    stats("after-fake-pyramid")
end

function probeClearPage()
    local n = app.deleteStrokes("page")
    -- xoá cả chữ để trang sạch
    local texts = app.getTexts("page")
    local refs = {}
    for i = 1, #texts do refs[i] = texts[i].ref end
    if #refs > 0 then
        app.addToSelection(refs)
        app.activateAction("delete")
        app.clearSelection()
    end
    print("[PROBE] đã xoá " .. tostring(n) .. " nét và " .. #refs .. " chữ")
    io.stdout:flush()
    stats("after-clear-page")
end

-- ===================== kiểm thử app.deleteStrokes("notePages") =====================
-- "Trang giấy nháp" = trang KHÔNG có nền PDF (pdfBackgroundPageNo == 0) và nền giấy là "lined".
-- Fixture: trang 1 là trang THƯỜNG (nền plain): 3 nét + 1 ảnh + 1 chữ; rồi tới 2 trang GIẤY NHÁP (lined),
-- mỗi trang 2 nét + 1 ảnh + 1 chữ. Nếu hàm xoá nhầm ảnh/chữ/trang thì các con số dưới đây lộ ra ngay.

local function statsAll(tag)
    local sAll = app.getStrokes("all")
    local iAll = app.getImages("all")
    local tAll = app.getTexts("all")
    local okN, sN = pcall(app.getStrokes, "notePages")
    local struct = app.getDocumentStructure()
    print(string.format("[PROBE] %s | strokes(all)=%d strokes(notePages)=%s images(all)=%d texts(all)=%d pages=%d",
                        tag or "-", sAll and #sAll or 0, okN and tostring(#sN) or "(hàm lỗi)",
                        iAll and #iAll or 0, tAll and #tAll or 0, struct and #struct.pages or 0))
    io.stdout:flush()
end

-- In ra từng trang: kiểu nền giấy, có nền PDF hay không, số nét/ảnh/chữ của trang đó.
function probeDocPages()
    local struct = app.getDocumentStructure()
    if not struct or not struct.pages then
        print("[PROBE] không đọc được cấu trúc tài liệu")
        io.stdout:flush()
        return
    end
    print("[PROBE] tài liệu có " .. #struct.pages .. " trang; trang đang xem = " .. tostring(struct.currentPage))
    for i = 1, #struct.pages do
        local p = struct.pages[i]
        app.setCurrentPage(i)
        local s = app.getStrokes("page")
        local im = app.getImages("page")
        local tx = app.getTexts("page")
        print(string.format("[PROBE]   trang %d: nen='%s' pdfBackgroundPageNo=%s | net=%d anh=%d chu=%d",
                            i, tostring(p.pageTypeFormat), tostring(p.pdfBackgroundPageNo),
                            s and #s or 0, im and #im or 0, tx and #tx or 0))
    end
    io.stdout:flush()
    statsAll("doc-pages")
end

function probeNoteFixture()
    app.changeCurrentPageBackground("plain")
    app.addStrokes({ strokes = {
        { x = { 100, 300 }, y = { 100, 100 }, width = 2.0, color = 0x000000, tool = "pen" },
        { x = { 100, 300 }, y = { 140, 140 }, width = 2.0, color = 0x000000, tool = "pen" },
        { x = { 100, 300 }, y = { 180, 180 }, width = 2.0, color = 0x000000, tool = "pen" },
    }, allowUndoRedoAction = "grouped" })
    app.addImages({ images = { { path = "/tmp/xpp-kt2/probe-img.png", x = 420, y = 420, maxWidth = 220 } },
                    allowUndoRedoAction = "grouped" })
    app.addTexts({ texts = { { text = "CHU tren trang thuong", x = 80, y = 620, fontSize = 12 } },
                   allowUndoRedoAction = "grouped" })
    for k = 1, 2 do
        local n = #app.getDocumentStructure().pages
        app.setCurrentPage(n)                    -- tới trang cuối rồi chèn trang mới SAU nó
        app.activateAction("new-page-after")
        app.setCurrentPage(n + 1)
        app.changeCurrentPageBackground("lined") -- giấy kẻ dòng -> thành trang giấy nháp
        app.addStrokes({ strokes = {
            { x = { 120, 380 }, y = { 200, 200 }, width = 2.0, color = 0xff0000, tool = "pen" },
            { x = { 120, 380 }, y = { 260, 260 }, width = 2.0, color = 0x0000ff, tool = "pen" },
        }, allowUndoRedoAction = "grouped" })
        app.addImages({ images = { { path = "/tmp/xpp-kt2/probe-img.png", x = 440, y = 440, maxWidth = 180 } },
                        allowUndoRedoAction = "grouped" })
        app.addTexts({ texts = { { text = "CHU tren giay nhap", x = 80, y = 640, fontSize = 12 } },
                       allowUndoRedoAction = "grouped" })
    end
    app.setCurrentPage(1)
    print("[PROBE] đã dựng fixture: 1 trang THƯỜNG (3 nét/1 ảnh/1 chữ) + 2 trang GIẤY NHÁP (2 nét/1 ảnh/1 chữ mỗi trang)")
    io.stdout:flush()
    statsAll("after-note-fixture")
end

function probeEraseNotePages()
    local ok, n = pcall(app.deleteStrokes, "notePages")
    print("[PROBE] app.deleteStrokes(\"notePages\") -> ok=" .. tostring(ok) .. " n=" .. tostring(n))
    io.stdout:flush()
    statsAll("after-erase-note-pages")
end

-- Xoá HẾT ảnh trên TRANG đang xem (không có hàm Lua API riêng: chọn hết ảnh rồi gọi action delete).
function probeEraseAllImages()
    local imgs = app.getImages("page")
    local refs = {}
    for i = 1, #imgs do refs[i] = imgs[i].ref end
    if #refs > 0 then
        app.addToSelection(refs)
        app.activateAction("delete")
        app.clearSelection()
    end
    print("[PROBE] đã xoá " .. #refs .. " ảnh trên trang")
    io.stdout:flush()
    statsAll("after-erase-all-images")
end

-- Chỉ in thống kê MỨC TOÀN TÀI LIỆU (để phía ngoài kiểm sau undo/redo).
function probeStatsAll()
    statsAll("snapshot-toan-tai-lieu")
end

-- Chỉ in vùng chọn HIỆN TẠI (không đụng gì khác).
function probeSelSnapshot()
    selStats("snapshot-vung-chon")
end

function initUi()
    app.registerUi{ menu = "PROBE 0: stats",            callback = "probeStats" }
    app.registerUi{ menu = "PROBE 1: them 3 net but",   callback = "probeAddStrokes" }
    app.registerUi{ menu = "PROBE 2: them anh",         callback = "probeAddImage" }
    app.registerUi{ menu = "PROBE 3: them chu",         callback = "probeAddText" }
    app.registerUi{ menu = "PROBE 4: xoa net (page)",   callback = "probeErasePage" }
    app.registerUi{ menu = "PROBE 5: xoa net (all)",    callback = "probeEraseAll" }
    app.registerUi{ menu = "PROBE 6: xoa net (select)", callback = "probeEraseSelection" }
    app.registerUi{ menu = "PROBE 7: xoa net (arg sai)",callback = "probeEraseBadArg" }
    app.registerUi{ menu = "PROBE 8: undo",             callback = "probeUndo" }
    app.registerUi{ menu = "PROBE 9: redo",             callback = "probeRedo" }
    app.registerUi{ menu = "PROBE 10: undo x2",         callback = "probeUndoTwice" }
    app.registerUi{ menu = "PROBE 11: them lop moi",    callback = "probeAddLayer" }
    app.registerUi{ menu = "PROBE 12: ve hinh + chon",  callback = "probeFigAndSelect" }
    app.registerUi{ menu = "PROBE 13: chon -> anh (move)", callback = "probeSelAsImageMove" }
    app.registerUi{ menu = "PROBE 14: chon -> anh (copy)", callback = "probeSelAsImageCopy" }
    app.registerUi{ menu = "PROBE 15: chon -> anh (mac dinh)", callback = "probeSelAsImagePlain" }
    app.registerUi{ menu = "PROBE 16: chon -> anh (khong co vung chon)", callback = "probeSelAsImageNoSelection" }
    app.registerUi{ menu = "PROBE 17: chon het net tren trang", callback = "probeSelectAll" }
    app.registerUi{ menu = "PROBE 18: kich thuoc anh tren trang", callback = "probeImageGeom" }
    app.registerUi{ menu = "PROBE 19: do du lieu PNG cua anh", callback = "probeDumpImageHex" }
    app.registerUi{ menu = "PROBE 20: ghi ten dinh", callback = "probeLabelVertices" }
    app.registerUi{ menu = "PROBE 21: ve hinh hop gia", callback = "probeAddFakeBox" }
    app.registerUi{ menu = "PROBE 22: ve hinh chop gia", callback = "probeAddFakePyramid" }
    app.registerUi{ menu = "PROBE 23: xoa trang", callback = "probeClearPage" }
    app.registerUi{ menu = "PROBE 24: liet ke tung trang", callback = "probeDocPages" }
    app.registerUi{ menu = "PROBE 25: dung fixture giay nhap", callback = "probeNoteFixture" }
    app.registerUi{ menu = "PROBE 26: xoa muc giay nhap", callback = "probeEraseNotePages" }
    app.registerUi{ menu = "PROBE 27: xoa het anh tren trang", callback = "probeEraseAllImages" }
    app.registerUi{ menu = "PROBE 28: thong ke toan tai lieu", callback = "probeStatsAll" }
    app.registerUi{ menu = "PROBE 29: in vung chon hien tai", callback = "probeSelSnapshot" }
end
