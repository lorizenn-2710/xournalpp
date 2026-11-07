--[[
 Kiểm thử logic plugin VK640 (không cần chạy app), đối chiếu với nguồn trong repo:
   plugins/VK640ToolCycle/main.lua

 Nội dung:
 * 22 mục registerUi — callback + accelerator (mục menu-only thì KHÔNG có phím tắt)
 * Ctrl+U        -> vk640EraseInk          : xoá HẾT nét bút trên TRANG ĐANG XEM
 * Ctrl+Shift+M  -> vk640EraseNotePagesInk : xoá MỰC BÚT trên MỌI trang giấy nháp
                   (gọi app.deleteStrokes("notePages"); KHÔNG xoá trang, KHÔNG xoá ảnh/chữ/nền)
 * menu (không phím tắt) -> vk640DeleteAllNotePages (+ vk640DeleteAllNotePagesAnswer)
                   : xoá HẲN các TRANG giấy nháp, có hỏi lại trước
 * Ctrl+Shift+C  -> vk640SelectionToImage  : app.selectionAsImage{ mode = "copy" } — GIỮ nét gốc
 * Ctrl+Shift+N  -> vk640LabelVertices     : app.labelSelectionVertices{ fontSize = 16 }

 Chạy: texlua tests/test-vk640-erase.lua
 Cách làm: thay app.* bằng bản giả, thay đồng hồ /proc/uptime bằng đồng hồ điều khiển được
 (plugin chống auto-repeat bằng /proc/uptime, không có đồng hồ giả thì các lần gọi liền nhau
 bị chốt chặn nuốt mất và ta sẽ tưởng logic sai).

 GHI CHÚ cho người đọc kết quả: có hai chỗ plugin IM LẶNG khi thành công, trong khi bản mô tả
 nhiệm vụ nói "thành công thì hiện hộp thoại" (vk640EraseNotePagesInk và vk640SelectionToImage).
 Test dưới đây bám theo HÀNH VI THẬT của main.lua; các check đó được ghi rõ "(KHÁC mô tả)".
 Không tự sửa main.lua.
]]

local PLUGIN = os.getenv("VK640_PLUGIN")
 or "plugins/VK640ToolCycle/main.lua"
local SRC_DIR = os.getenv("VK640_SRC") or "."

local pass, fail = 0, 0
local function check(cond, name, extra)
 if cond then
 pass = pass + 1
 print(string.format(" %-62s PASS", name))
 else
 fail = fail + 1
 print(string.format(" %-62s FAIL %s", name, tostring(extra or "")))
 end
end

-- ---------------------------------------------------------------- đồng hồ giả
local fakeUp = 100.0
local realOpen = io.open
io.open = function(path, mode)
 if path == "/proc/uptime" then
 return {
 read = function() return string.format("%.2f 0.00\n", fakeUp) end,
 close = function() end,
 }
 end
 return realOpen(path, mode)
end

-- ---------------------------------------------------------------- app giả
local registered, dialogs, actions, doc, currentPage, strokesOnPage
local lastDeleteArg, lastImageMode, lastLabelOpts, imageResult, labelResult

local function newApp(withDelete, withFigures)
 registered, dialogs, actions = {}, {}, {}
 currentPage = 1
 strokesOnPage = 0
 lastDeleteArg = nil
 lastImageMode = nil
 lastLabelOpts = nil
 imageResult = 1
 labelResult = 8
 app = {
 C = { Tool_pen = 1, Tool_eraser = 2, Tool_hand = 13, Tool_selectRect = 6 },
 registerUi = function(t) registered[#registered + 1] = t end,
 openDialog = function(msg, buttons, cb, isError)
 dialogs[#dialogs + 1] = { msg = msg, buttons = buttons, cb = cb, err = isError }
 end,
 getDocumentStructure = function() return { pages = doc } end,
 setCurrentPage = function(i) currentPage = i end,
 scrollToPage = function() end,
 getActionState = function() return 1 end,
 changeActionState = function() end,
 activateAction = function(name)
 actions[#actions + 1] = name
 if name == "delete-page" then table.remove(doc, currentPage) end
 end,
 }
 if withDelete then
 -- app.deleteStrokes(where) -> số nét đã xoá; ghi lại tham số cuối vào lastDeleteArg
 app.deleteStrokes = function(where)
 lastDeleteArg = where
 return strokesOnPage
 end
 end
 if withFigures then
 app.selectionAsImage = function(opts)
 lastImageMode = opts and opts.mode or "?"
 return imageResult
 end
 app.labelSelectionVertices = function(opts)
 lastLabelOpts = opts
 return labelResult
 end
 end
 dofile(PLUGIN)
 initUi() -- app gọi hàm này lúc nạp plugin; ở đây gọi tay để kiểm tra bảng đăng ký
end

local function docOf(pdfN, linedN, plainN)
 local d = {}
 for _ = 1, pdfN do d[#d + 1] = { pdfBackgroundPageNo = 1, pageTypeFormat = "pdf" } end
 for _ = 1, linedN do d[#d + 1] = { pdfBackgroundPageNo = 0, pageTypeFormat = "lined" } end
 for _ = 1, plainN do d[#d + 1] = { pdfBackgroundPageNo = 0, pageTypeFormat = "plain" } end
 return d
end

local function findByCallback(cb)
 for _, r in ipairs(registered) do
 if r.callback == cb then return r end
 end
end

-- ================================================================ 1. đăng ký UI
-- Bảng dưới đây là HỢP ĐỒNG giữa plugin và test: mỗi dòng là một app.registerUi{}
-- theo đúng thứ tự trong initUi(). accelerator = nil nghĩa là mục MENU-ONLY.
print("== 1. Đăng ký UI: callback + accelerator ==")
local EXPECTED = {
 { cb = "vk640CycleTool", accel = "Tab" },
 { cb = "vk640CycleToolCtrl", accel = "<Control>Tab" },
 { cb = "vk640CycleToolF8", accel = "F8" },
 { cb = "vk640ToggleColumns", accel = "<Alt>grave" },
 { cb = "vk640SplitPdfNoteShift", accel = "<Shift>y" },
 { cb = "vk640SplitPdfNote", accel = "<Control>y" },
 { cb = "vk640ToggleDashed", accel = "<Control>j" },
 { cb = "vk640CycleLineStyle", accel = "<Control>k" },
 { cb = "vk640Thicker", accel = "<Control>b" },
 { cb = "vk640Thinner", accel = "<Control><Shift>b" },
 { cb = "vk640EraseInk", accel = "<Control>u" },
 { cb = "vk640EraseNotePagesInk", accel = "<Control><Shift>m" },
 { cb = "vk640DeleteAllNotePages", accel = nil }, -- MENU-ONLY
 { cb = "vk640LabelVertices", accel = "<Control><Shift>n" },
 { cb = "vk640SelectionToImage", accel = "<Control><Shift>c" },
 { cb = "vk640ShortcutHelp", accel = "F1" },
 { cb = "vk640CycleColor", accel = "<Alt>c" },
 { cb = "vk640ToggleLine", accel = "<Alt>l" },
 { cb = "vk640ToggleArrow", accel = "<Alt>v" },
 { cb = "vk640SineOne", accel = "<Alt>s" },
 { cb = "vk640SineTwo", accel = "<Alt><Shift>s" },
 { cb = "vk640ToggleShortcutHints", accel = "<Control>grave" },
 { cb = "vk640MathTools", accel = "<Alt>g" },
 { cb = "vk640FloatingToggle", accel = "<Alt>n" },
 { cb = "vk640FloatingNew", accel = "<Control><Alt>n" },
 { cb = "vk640FloatingLibrary", accel = "<Alt>k" },
}
doc = docOf(1, 1, 0)
newApp(true)
check(#registered == #EXPECTED,
 string.format("đủ %d mục đăng ký (không mất, không thừa)", #EXPECTED), #registered)
for i, e in ipairs(EXPECTED) do
 local r = registered[i]
 local what = string.format("[%2d] %-22s accel=%s", i, e.cb, tostring(e.accel))
 if not r then
 check(false, what, "không có mục này")
 else
 check(r.callback == e.cb and r.accelerator == e.accel, what,
 string.format("nhận: cb=%s accel=%s", tostring(r.callback), tostring(r.accelerator)))
 end
end
local seen, dup = {}, {}
for _, r in ipairs(registered) do
 if seen[r.callback] then dup[#dup + 1] = r.callback end
 seen[r.callback] = true
end
check(#dup == 0, "không callback nào đăng ký hai lần", table.concat(dup, ","))
-- Xournal++ gọi callback bằng lua_getglobal(name) -> hàm BẮT BUỘC toàn cục
local notGlobal = {}
for _, r in ipairs(registered) do
 if type(_G[r.callback]) ~= "function" then notGlobal[#notGlobal + 1] = r.callback end
end
check(#notGlobal == 0,
 "mọi callback là hàm TOÀN CỤC (lua_getglobal mới gọi được)", table.concat(notGlobal, ","))
check(type(_G.vk640DeleteAllNotePagesAnswer) == "function",
 "hàm nhận trả lời vk640DeleteAllNotePagesAnswer là hàm toàn cục", "")
check(findByCallback("vk640DeleteAllNotePages").accelerator == nil,
 'mục menu "xoa HAN cac trang giay nhap" KHÔNG gắn phím tắt',
 tostring(findByCallback("vk640DeleteAllNotePages").accelerator))

-- ================================================================ 2. Ctrl+U: xoá nét
print("== 2. Ctrl+U — xoá hết nét bút trên trang đang xem ==")
strokesOnPage = 5
fakeUp = fakeUp + 10
vk640EraseInk()
check(lastDeleteArg == "page", 'gọi app.deleteStrokes("page") — đúng trang đang xem', lastDeleteArg)
check(#dialogs == 0, "xoá được 5 nét thì KHÔNG hiện hộp thoại (không vướng tay khi giảng)", #dialogs)
check(#actions == 0, "không đụng tới trang tài liệu (không xoá trang nào)", #actions)

strokesOnPage = 0
fakeUp = fakeUp + 10
vk640EraseInk()
check(#dialogs == 1 and dialogs[1].msg:find("chưa có nét bút nào") ~= nil and dialogs[1].err == false,
 "trang trống -> báo một câu, biết là phím vẫn ăn", #dialogs > 0 and dialogs[1].msg)

strokesOnPage = 3
fakeUp = fakeUp + 10
local n0 = #dialogs
vk640EraseInk()
check(#dialogs == n0, "xoá được thì không báo gì thêm", #dialogs - n0)

-- ================================================================ 3. chốt auto-repeat
print("== 3. chốt chống auto-repeat (giữ phím) ==")
strokesOnPage = 4
fakeUp = fakeUp + 10
local calls = 0
app.deleteStrokes = function(where) calls = calls + 1; return strokesOnPage end
vk640EraseInk()
vk640EraseInk()
vk640EraseInk()
check(calls == 1, "bấm giữ phím: chỉ xoá MỘT lần", calls)
fakeUp = fakeUp + 1
vk640EraseInk()
check(calls == 2, "nhả rồi bấm lại (sau 1 giây): chạy lại", calls)

-- ================================================================ 4. app cũ chưa có API
print("== 4. bản app chưa có app.deleteStrokes() ==")
doc = docOf(1, 1, 0)
newApp(false)
fakeUp = fakeUp + 10
vk640EraseInk()
check(#dialogs == 1 and dialogs[1].err == true and dialogs[1].msg:find("KHÔNG có hàm") ~= nil,
 "Ctrl+U: báo RÕ là bản app thiếu hàm, không im lặng hỏng",
 #dialogs > 0 and tostring(dialogs[1].msg))

-- Ctrl+U KHÔNG bọc pcall quanh app.deleteStrokes("page") -> lỗi từ hàm API lan ra ngoài.
-- (Có chủ ý: nhánh "page" là API cũ, luôn tồn tại; chỉ nhánh "notePages" mới cần pcall.)
app.deleteStrokes = function() error("boom") end
fakeUp = fakeUp + 10
local okPage, errPage = pcall(vk640EraseInk)
check(okPage == false and tostring(errPage):find("boom") ~= nil,
 "Ctrl+U: lỗi từ app.deleteStrokes lan ra ngoài (không pcall — đúng thiết kế)",
 tostring(okPage) .. "/" .. tostring(errPage))

-- ================================================================ 5. Ctrl+Shift+M: xoá MỰC giấy nháp
print("== 5. Ctrl+Shift+M — xoá MỰC BÚT trên MỌI trang giấy nháp ==")
do
 -- (a) app cũ thiếu hàm (chú ý: lời nhắn của nhánh này KHÁC lời nhắn của Ctrl+U —
 -- Ctrl+U: "KHÔNG có hàm app.deleteStrokes()", nhánh này: "không có app.deleteStrokes()")
 doc = docOf(3, 3, 1)
 newApp(false)
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 check(#dialogs == 1 and dialogs[1].err == true
 and dialogs[1].msg:find("app.deleteStrokes()") ~= nil,
 "thiếu app.deleteStrokes() -> hộp thoại LỖI, nói rõ phải cài lại bản build riêng",
 #dialogs > 0 and dialogs[1].msg)

 -- (b) không có trang giấy nháp nào -> không gọi xoá
 doc = docOf(2, 0, 1)
 newApp(true)
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 check(#dialogs == 1 and dialogs[1].err == false and dialogs[1].msg:find("Chưa có trang giấy nháp nào") ~= nil,
  "không có trang nháp -> chỉ báo, KHÔNG hỏi xoá", #dialogs > 0 and dialogs[1].msg)
 check(lastDeleteArg == nil, "không có trang nháp -> KHÔNG gọi app.deleteStrokes", tostring(lastDeleteArg))

 -- (c) có trang nháp + xoá được nét: gọi đúng "notePages", KHÔNG đụng vào trang
 doc = docOf(3, 3, 1) -- 3 PDF + 3 nháp + 1 trắng
 newApp(true)
 strokesOnPage = 12
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 check(lastDeleteArg == "notePages", 'gọi app.deleteStrokes("notePages")', tostring(lastDeleteArg))
 check(#actions == 0, "KHÔNG xoá trang nào (không gọi delete-page)", #actions)
 check(currentPage == 1, "không đổi trang đang xem (màn hình không nhảy)", currentPage)
 check(#doc == 7, "tài liệu còn nguyên 7 trang (trang + ảnh/chữ/nền giữ hết)", #doc)
 -- LỆCH MÔ TẢ: bản mô tả nói "thành công thì hiện hộp thoại thông báo số nét đã xoá";
 -- main.lua im lặng khi n > 0 (xem khối comment ngay trên dòng "n > 0: KHÔNG báo gì").
 check(#dialogs == 0, "xoá được 12 nét: KHÔNG hộp thoại (plugin im lặng — KHÁC mô tả)", #dialogs)

 -- (d) có trang nháp nhưng chưa có nét nào
 doc = docOf(3, 3, 1)
 newApp(true)
 strokesOnPage = 0
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 check(#dialogs == 1 and dialogs[1].err == false
  and dialogs[1].msg:find("chưa có nét bút nào trên đó") ~= nil,
  "có 3 trang nháp nhưng 0 nét -> một câu cho biết phím vẫn ăn", #dialogs > 0 and dialogs[1].msg)
 check(lastDeleteArg == "notePages", 'vẫn gọi app.deleteStrokes("notePages")', tostring(lastDeleteArg))

 -- (e) app cũ hơn nguồn: getElementsFromHelper không biết "notePages" -> ném lỗi Lua
 doc = docOf(3, 3, 1)
 newApp(true)
 app.deleteStrokes = function() error("unknown mode: notePages") end
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 check(#dialogs == 1 and dialogs[1].err == true
  and dialogs[1].msg:find("notePages") ~= nil
  and dialogs[1].msg:find("unknown mode") ~= nil,
  "app.deleteStrokes lỗi -> hộp thoại LỖI kèm lý do", #dialogs > 0 and dialogs[1].msg)

 -- (f) app.deleteStrokes trả nil + chuỗi lỗi
 doc = docOf(3, 3, 1)
 newApp(true)
 app.deleteStrokes = function() return nil, "getElementsFromHelper: bad mode" end
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 check(#dialogs == 1 and dialogs[1].err == true and dialogs[1].msg:find("bad mode") ~= nil,
  "app.deleteStrokes trả nil -> hộp thoại LỖI, không im lặng", #dialogs > 0 and dialogs[1].msg)

 -- (g) chốt chống auto-repeat riêng của Ctrl+Shift+M
 doc = docOf(3, 3, 1)
 newApp(true)
 local nCall = 0
 app.deleteStrokes = function(where) nCall = nCall + 1; lastDeleteArg = where; return 5 end
 fakeUp = fakeUp + 10
 vk640EraseNotePagesInk()
 vk640EraseNotePagesInk()
 vk640EraseNotePagesInk()
 check(nCall == 1, "giữ phím: chỉ xoá MỘT lần", nCall)
 fakeUp = fakeUp + 1
 vk640EraseNotePagesInk()
 check(nCall == 2, "nhả rồi bấm lại (sau 1 giây): chạy lại", nCall)
end

-- ================================================================ 6. mục MENU: xoá hẳn TRANG nháp
print("== 6. MỤC MENU (không phím tắt) — xoá HẲN các TRANG giấy nháp, có hỏi lại ==")
do
 -- không có trang nháp
 doc = docOf(2, 0, 1)
 newApp(true)
 fakeUp = fakeUp + 10
 local nNhap0, nPdf0 = vk640DeleteAllNotePages()
 check(#actions == 0 and #dialogs == 1
  and dialogs[1].msg:find("Không có trang giấy nháp nào để xoá") ~= nil,
  "không có trang nháp -> chỉ báo, không hỏi xoá", #dialogs > 0 and dialogs[1].msg)
 check(nNhap0 == 0 and nPdf0 == 0, "trả về (0, 0) khi không có trang nháp", nNhap0 .. "/" .. nPdf0)

 -- có trang nháp -> HỎI trước, chưa xoá gì
 doc = docOf(3, 3, 1) -- 3 PDF + 3 nháp + 1 trắng
 newApp(true)
 fakeUp = fakeUp + 10
 local nNhap, nPdf = vk640DeleteAllNotePages()
 check(nNhap == 3 and nPdf == 3, "trả về (số trang nháp, số trang PDF) = (3, 3)", nNhap .. "/" .. nPdf)
 check(#actions == 0, "bấm mục menu thì CHƯA xoá gì — phải hỏi trước", #actions)
 check(#dialogs == 1 and dialogs[1].buttons[1] == "Xoá hết" and dialogs[1].buttons[2] == "Huỷ"
  and dialogs[1].err == false,
  'hộp thoại hỏi, nút 1 = "Xoá hết", nút 2 = "Huỷ"',
  #dialogs > 0 and table.concat(dialogs[1].buttons, "|"))
 check(dialogs[1].cb == "vk640DeleteAllNotePagesAnswer",
  "trả lời qua callback vk640DeleteAllNotePagesAnswer", dialogs[1].cb)

 -- trả lời "Huỷ" -> không xoá
 vk640DeleteAllNotePagesAnswer(2)
 check(#actions == 0 and #doc == 7, "chọn Huỷ -> không xoá trang nào", #actions .. "/" .. #doc)

 -- trả lời "Xoá hết" -> xoá đúng 3 trang nháp, giữ 3 PDF + 1 trắng
 fakeUp = fakeUp + 10
 vk640DeleteAllNotePages()
 fakeUp = fakeUp + 10
 vk640DeleteAllNotePagesAnswer(1)
 check(#actions == 3, "chọn Xoá hết -> đúng 3 lần delete-page", #actions)
 check(#doc == 4, "còn lại 4 trang (3 PDF + 1 trang trắng)", #doc)
 local pdfLeft, linedLeft, plainLeft = 0, 0, 0
 for _, p in ipairs(doc) do
  if p.pdfBackgroundPageNo ~= 0 then pdfLeft = pdfLeft + 1
  elseif p.pageTypeFormat == "lined" then linedLeft = linedLeft + 1
  else plainLeft = plainLeft + 1 end
 end
 check(pdfLeft == 3 and linedLeft == 0 and plainLeft == 1,
  "giữ nguyên 3 PDF + 1 trắng, xoá sạch 3 trang nháp",
  string.format("pdf=%d lined=%d plain=%d", pdfLeft, linedLeft, plainLeft))
 check(currentPage == 1, "đứng lại trang 1 sau khi xoá", currentPage)
 check(#dialogs >= 2 and dialogs[#dialogs].msg:find("Đã xoá 3 trang giấy nháp") ~= nil,
 "báo lại đã xoá 3 trang", #dialogs > 0 and dialogs[#dialogs].msg)
 -- CẢNH BÁO ĐÃ ĐO ĐƯỢC (không assert vì đây là lỗi HIỂN THỊ của main.lua, không có trong mô tả):
 -- dòng "Còn lại %d trang PDF và %d trang khác" tính total(SAU khi xoá) - soPdf - soNhap, tức
 -- trừ số trang nháp HAI lần, nên đếm thiếu hoặc ra số âm:
 -- doc(3 PDF + 3 nháp + 1 trắng) -> "Còn lại 3 trang PDF và -2 trang khác" (đúng phải là 1)
 -- doc(0 PDF + 2 nháp + 0 trắng) -> "Còn lại 0 trang PDF và -2 trang khác" (đúng phải là 0)
 -- doc(2 PDF + 2 nháp + 3 trắng) -> "Còn lại 2 trang PDF và 1 trang khác" (đúng phải là 3)
 -- Công thức đúng phải là total - soPdf (total tính SAU khi xoá = PDF + trang khác).
 check(dialogs[#dialogs].err == false, "báo cáo là hộp thoại thường, không phải hộp thoại lỗi",
 tostring(dialogs[#dialogs].err))

 -- ca biên: tài liệu CHỈ có giấy nháp
 doc = docOf(0, 2, 0)
 newApp(true)
 fakeUp = fakeUp + 10
 vk640DeleteAllNotePages()
 fakeUp = fakeUp + 10
 vk640DeleteAllNotePagesAnswer(1)
 check(#actions == 2, "xoá cả 2 trang nháp khi tài liệu chỉ có nháp", #actions)
 -- LƯU Ý: #doc == 0 là hành vi của bản app GIẢ (activateAction("delete-page") xoá vô điều kiện).
 -- App thật có thể TỪ CHỐI xoá trang cuối cùng; nhánh C++ đó test này không kiểm chứng được.
 -- Chính main.lua cũng ghi "tài liệu luôn phải còn ít nhất 1 trang" — vòng lặp trong
 -- vk640DeleteAllNotePagesAnswer KHÔNG tự bảo đảm điều đó.
 check(#doc == 0, "tài liệu không còn trang nào (theo app giả)", #doc)
 check(#dialogs >= 1, "vẫn có báo cáo sau khi xoá", #dialogs)

 -- chốt chống auto-repeat của hàm hỏi
 doc = docOf(2, 2, 0)
 newApp(true)
 fakeUp = fakeUp + 10
 vk640DeleteAllNotePages()
 vk640DeleteAllNotePages()
 check(#dialogs == 1, "bấm mục menu liên tiếp: chỉ HỎI một lần", #dialogs)
end

-- ================================================================ 7. trùng phím với app
-- Phím của plugin gắn ở cấp ỨNG DỤNG (gtk_application_set_accels_for_action). Khi hai GAction
-- cùng một phím thì bên ĐĂNG KÝ TRƯỚC thắng, mà menu của app đăng ký trước plugin -> plugin im
-- lặng không chạy, KHÔNG có cảnh báo nào. Đó đúng là lỗi người dùng từng gặp với Ctrl+D /
-- Ctrl+Shift+D. Nên phép kiểm này đọc bảng phím THẬT của app từ cây nguồn rồi đối chiếu:
-- * ui/*.xml                                         -> phím của menu
-- * src/core/control/actions/ActionProperties.h       -> phím đặt trong MÃ NGUỒN
-- (thiếu nguồn thứ hai thì sẽ bỏ sót đúng phím Redo <Ctrl>Y — xem check ngay dưới)
print("== 7. Đối chiếu phím với bảng phím của app ==")

-- GTK so khớp keyval ở dạng CHỮ THƯỜNG (đó là lý do app viết Redo là "<Ctrl>Y" mà vẫn bắt được
-- Ctrl+y); modifier mới là thứ phân biệt hoa/thường. Hạ chữ thường khoá cho giống GTK.
local function normalizeAccel(raw)
 local a = raw:gsub("&lt;", "<"):gsub("&gt;", ">"):gsub("&amp;", "&"):gsub("Ctrl", "Control")
 local ctrl = a:find("<Control>") ~= nil
 local shift = a:find("<Shift>") ~= nil
 local alt = a:find("<Alt>") ~= nil
 local meta = a:find("<Meta>") ~= nil
 local key = (a:match("([^<>]+)$") or ""):lower()
 return (ctrl and "<Control>" or "") .. (shift and "<Shift>" or "")
 .. (alt and "<Alt>" or "") .. (meta and "<Meta>" or "") .. key
end

local function appAccels()
 local set, found = {}, false
 local f = io.popen("grep -rho 'accel\">[^<]*' " .. SRC_DIR .. "/ui/*.xml")
 if f then
 found = true
 for line in f:lines() do
  local a = line:gsub('^accel">', "")
  if a ~= "" then set[normalizeAccel(a)] = true end
 end
 f:close()
 end
 -- phím đặt bằng mã nguồn (Undo/Redo/Cut/Copy/Paste, phóng to/thu nhỏ...)
 local g = io.popen("grep -h 'accelerators\\[\\]' " .. SRC_DIR
 .. "/src/core/control/actions/ActionProperties.h")
 if g then
 found = true
 for line in g:lines() do
  for s in line:gmatch('"([^"]+)"') do
  if s:find("[<>]") then set[normalizeAccel(s)] = true end
  end
 end
 g:close()
 end
 if not found then return nil end
 return set
end

local appSet = appAccels()
if appSet == nil then
 print(" (bỏ qua: không đọc được cây nguồn " .. SRC_DIR .. ")")
else
 -- Các phím CỐ Ý trùng, đã ghi rõ trong bảng phím tắt trong app (F1):
 local allowed = {
 ["<Control>y"] = "Redo của app (<Ctrl>Y trong ActionProperties.h) — plugin cố ý chiếm, đã cảnh báo trong bảng phím tắt F1",
 ["<Shift>y"] = "app không dùng, nhưng gõ chữ Y HOA cũng kích hoạt — đã cảnh báo trong bảng phím tắt",
 }
 local clash = {}
 for _, r in ipairs(registered) do
 local a = r.accelerator
 if a then
  local norm = normalizeAccel(a)
  if appSet[norm] and not allowed[norm] then
  clash[#clash + 1] = a .. " (" .. r.callback .. ")"
  end
 end
 end
 check(#clash == 0, "không phím nào của plugin bị app đăng ký trước (nếu trùng là phím chết)",
 table.concat(clash, ", "))
 -- hai phím "cố ý trùng" phải THẬT SỰ là phím của app, nếu không thì bảng allowed đã mục
 check(appSet["<Control>y"] == true,
 "Ctrl+Y ĐÚNG LÀ phím Redo của app (trùng có chủ ý, đã ghi trong bảng phím tắt F1)", "")
 check(appSet["<Shift>y"] == nil,
 "Shift+Y vẫn chưa bị menu nào của app chiếm", "")
 -- phím xoá nét phải thật sự TRỐNG
 check(appSet["<Control>u"] == nil, "Ctrl+U (xoá nét) không menu nào của app dùng", "")
 check(appSet["<Control><Shift>m"] == nil, "Ctrl+Shift+M (xoá MỰC giấy nháp) không menu nào dùng", "")
 check(appSet["<Control><Shift>u"] ~= nil,
 "Ctrl+Shift+U LÀ của app (công cụ Link) — lý do phải tránh phím đó", "")
 check(appSet["<Control><Shift>c"] == nil and appSet["<Control><Shift>n"] == nil,
 "Ctrl+Shift+C và Ctrl+Shift+N không menu nào của app dùng", "")
end

-- ================================================================ 8. Ctrl+Shift+C: lấy ảnh
print("== 8. Ctrl+Shift+C — quét vùng chọn thành ẢNH (copy, giữ nét gốc) ==")
do
 doc = docOf(1, 0, 0)
 newApp(true, true)
 local uiImg = findByCallback("vk640SelectionToImage")
 check(uiImg ~= nil and uiImg.accelerator == "<Control><Shift>c",
 "Ctrl+Shift+C -> vk640SelectionToImage", uiImg and uiImg.accelerator)

 fakeUp = fakeUp + 10
 vk640SelectionToImage()
 check(lastImageMode == "copy", 'gọi với mode = "copy" (GIỮ nét gốc, không cắt đi)', lastImageMode)

 -- LỆCH MÔ TẢ: mô tả nói "thành công hiện hộp thoại"; main.lua im lặng khi lấy được ảnh.
 local n1 = #dialogs
 fakeUp = fakeUp + 10
 vk640SelectionToImage()
 check(#dialogs == n1, "lấy ảnh được: KHÔNG hộp thoại (plugin im lặng — KHÁC mô tả)", #dialogs - n1)

 -- không có vùng chọn -> báo rõ cách làm
 imageResult = 0
 fakeUp = fakeUp + 10
 vk640SelectionToImage()
 check(#dialogs == n1 + 1 and dialogs[#dialogs].err == false
  and dialogs[#dialogs].msg:find("Chưa có vùng nào") ~= nil,
  "không có vùng chọn -> hướng dẫn cách chọn", #dialogs > 0 and dialogs[#dialogs].msg)
 imageResult = 1

 -- hàm lỗi -> báo lỗi thật, không im
 fakeUp = fakeUp + 10
 app.selectionAsImage = function() error("hết bộ nhớ") end
 vk640SelectionToImage()
 check(dialogs[#dialogs].err == true and dialogs[#dialogs].msg:find("hết bộ nhớ") ~= nil,
 "hàm lỗi -> hiện hộp thoại báo lỗi kèm lý do", dialogs[#dialogs].msg)

 -- bản app cũ chưa có hàm
 doc = docOf(1, 0, 0)
 newApp(false)
 fakeUp = fakeUp + 10
 vk640SelectionToImage()
 check(dialogs[#dialogs].err == true and dialogs[#dialogs].msg:find("KHÔNG có hàm") ~= nil,
 "app thiếu hàm -> nói rõ là thiếu, không hỏng im lặng", dialogs[#dialogs].msg)
end

-- ================================================================ 9. Ctrl+Shift+N: ghi tên đỉnh
print("== 9. Ctrl+Shift+N — ghi tên đỉnh cho hình đang chọn ==")
do
 doc = docOf(1, 0, 0)
 newApp(true, true)
 local uiLab = findByCallback("vk640LabelVertices")
 check(uiLab ~= nil and uiLab.accelerator == "<Control><Shift>n",
 "Ctrl+Shift+N -> vk640LabelVertices", uiLab and uiLab.accelerator)

 fakeUp = fakeUp + 10
 vk640LabelVertices()
 check(lastLabelOpts ~= nil and lastLabelOpts.fontSize == 16, "gọi kèm cỡ chữ 16",
 lastLabelOpts and lastLabelOpts.fontSize)
 check(#dialogs == 0, "ghi được 8 tên thì im lặng", #dialogs)

 labelResult = 0
 fakeUp = fakeUp + 10
 vk640LabelVertices()
 check(dialogs[#dialogs].err == false and dialogs[#dialogs].msg:find("Không nhận ra hình nào") ~= nil,
 "không nhận ra hình -> nói rõ, không đoán bừa", dialogs[#dialogs].msg)

 labelResult = 5
 fakeUp = fakeUp + 10
 local nL = #dialogs
 vk640LabelVertices()
 check(#dialogs == nL, "nhận ra hình -> không hỏi gì thêm", #dialogs - nL)

 doc = docOf(1, 0, 0)
 newApp(false)
 fakeUp = fakeUp + 10
 vk640LabelVertices()
 check(dialogs[#dialogs].err == true and dialogs[#dialogs].msg:find("KHÔNG có hàm") ~= nil,
 "app thiếu hàm -> nói rõ là thiếu", dialogs[#dialogs].msg)
end

-- New teaching shortcuts: test state changes, debounce, actual waveform samples and selection.
do
 doc = {{ pageWidth = 600, pageHeight = 800 }}
 newApp(true, true)
 local state = { ["select-tool"] = app.C.Tool_pen }
 local penColor = 0x123456
 local colorCalls, lastColor = 0, nil
 local batches, selected, cleared = {}, nil, 0
 app.getDocumentStructure = function() return { pages = doc, currentPage = 1 } end
 app.getToolInfo = function() return { color = penColor } end
 app.getActionState = function(name) return state[name] end
 app.changeActionState = function(name, value)
  if name == "tool-draw-line" or name == "tool-draw-arrow" then
   state["tool-draw-line"], state["tool-draw-arrow"] = false, false
  end
  state[name] = value
 end
 app.changeToolColor = function(opts)
  colorCalls, lastColor = colorCalls + 1, opts
  penColor = opts.color
 end
 app.clearSelection = function() cleared = cleared + 1 end
 app.addStrokes = function(opts) batches[#batches+1] = opts; return { "wave-ref" } end
 app.addToSelection = function(refs) selected = refs end
 for _, color in ipairs({0xff0000, 0x0000ff, 0x008000, 0x000000, 0xff0000}) do
  fakeUp = fakeUp + 1
  vk640CycleColor()
  check(penColor == color and lastColor.tool == "pen" and lastColor.selection == true,
        "Alt+C: đúng thứ tự màu, áp cả vùng chọn")
 end
 local n = colorCalls
 vk640CycleColor()
 check(colorCalls == n, "Alt+C chống auto-repeat")
 fakeUp = fakeUp + 1
 vk640ToggleLine()
 check(state["tool-draw-line"] == true and state["select-tool"] == app.C.Tool_pen,
       "Alt+L bật đoạn thẳng bằng bút")
 vk640ToggleLine()
 check(state["tool-draw-line"] == true, "Alt+L chống auto-repeat")
 fakeUp = fakeUp + 1
 vk640ToggleLine()
 check(state["tool-draw-line"] == false, "Alt+L lần hai trả về nét tự do")
 fakeUp = fakeUp + 1
 vk640ToggleLine()
 vk640ToggleArrow()
 check(state["tool-draw-arrow"] == true and state["tool-draw-line"] == false,
       "Alt+V chuyển trực tiếp đoạn thẳng sang mũi tên")
 fakeUp = fakeUp + 1
 vk640ToggleArrow()
 check(state["tool-draw-arrow"] == false, "Alt+V lần hai trả về nét tự do")
 for cycles, callback in ipairs({vk640SineOne, vk640SineTwo}) do
  fakeUp = fakeUp + 1
  callback()
  local batch = batches[#batches]
  local wave = batch.strokes[1]
  check(#batch.strokes == 1 and #wave.x == 128*cycles+1 and #wave.x == #wave.y,
        "sóng sin: đúng một nét mượt 128 đoạn/chu kỳ")
  local exact, fits, monotonic = true, true, true
  for i = 1, #wave.x do
   local expected = 320 - 50*math.sin(2*math.pi*cycles*(wave.x[i]-120)/360)
   exact = exact and math.abs(wave.y[i] - expected) < 1e-8
   fits = fits and wave.x[i] >= 0 and wave.x[i] <= 600 and wave.y[i] >= 0 and wave.y[i] <= 800
   if i > 1 then monotonic = monotonic and wave.x[i] > wave.x[i-1] end
  end
  check(exact and monotonic, "sóng sin: mọi mẫu đúng A sin(omega x), x tăng đều")
  check(fits and wave.fill == -1 and wave.tool == "pen", "sóng sin: vừa trang, không tô kín")
  check(selected[1] == "wave-ref" and cleared == cycles and batch.allowUndoRedoAction == "grouped"
        and state["select-tool"] == app.C.Tool_selectRect,
        "sóng sin: chọn riêng nét mới, một bước hoàn tác")
  local count = #batches
  callback()
  check(#batches == count, "sóng sin chống chèn lặp khi giữ phím")
 end
 doc = {{ pageWidth = 100, pageHeight = 100 }}
 fakeUp = fakeUp + 1
 vk640SineTwo()
 local wave = batches[#batches].strokes[1]
 check(wave.x[1] == 15 and wave.x[#wave.x] == 85,
       "sóng sin tự co vừa trang nhỏ")
 doc = {{ pageWidth = 600, pageHeight = 800 }}
 app.getVisiblePageRect = function() return { x = 200, y = 400, width = 100, height = 150 } end
 fakeUp = fakeUp + 1
 vk640SineOne()
 wave = batches[#batches].strokes[1]
 local fitsViewport = true
 for i = 1, #wave.x do
  fitsViewport = fitsViewport and wave.x[i] >= 200 and wave.x[i] <= 300 and wave.y[i] >= 400 and wave.y[i] <= 550
 end
 check(fitsViewport and wave.x[1] == 215 and wave.x[#wave.x] == 285,
       "sóng sin luôn vừa vùng đang nhìn khi zoom/cuộn sâu trong trang")
 app.getVisiblePageRect = function() return nil end
 fakeUp = fakeUp + 1
 vk640SineOne()
 wave = batches[#batches].strokes[1]
 check(wave.x[1] == 120 and wave.x[#wave.x] == 480,
       "không có vùng nhìn: quay về khung trang an toàn")
 doc = {}
 fakeUp = fakeUp + 1
 local count = #batches
 vk640SineOne()
 check(#batches == count, "không có trang: không chèn sóng")
 vk640ShortcutHelp()
 local help = dialogs[#dialogs].msg
 check(help:find("Alt + C", 1, true) and help:find("Alt + S", 1, true) and help:find("Oxyz", 1, true),
       "F1 có phím màu, sóng sin và hệ trục 3D")
 local seenKeys, unique = {}, true
 for _, ui in ipairs(registered) do
  if ui.accelerator then
   local key = normalizeAccel(ui.accelerator)
   unique = unique and not seenKeys[key]
   seenKeys[key] = true
  end
 end
 check(unique, "các phím plugin không trùng nhau sau chuẩn hoá")
end

do
 doc = docOf(1, 1, 0)
 newApp(true, true)
 local visible, calls, columnChanges = false, 0, 0
 app.showToolbarShortcuts = function() visible = not visible; calls = calls + 1; return visible end
 app.changeActionState = function(name, value)
  if name == "set-columns-or-rows" then columnChanges = columnChanges + 1 end
 end
 fakeUp = fakeUp + 1
 vk640ToggleShortcutHints()
 check(visible and calls == 1 and columnChanges == 0,
       "Ctrl+`: hiện chữ phím tắt, không thay bố cục/trang")
 vk640ToggleShortcutHints()
 check(calls == 1, "Ctrl+`: chống auto-repeat")
 fakeUp = fakeUp + 1
 vk640ToggleShortcutHints()
 check(not visible and calls == 2, "Ctrl+` lần hai: ẩn chữ, thanh công cụ gọn lại")
 fakeUp = fakeUp + 1
 vk640ToggleColumns()
 check(columnChanges == 1 and calls == 2, "Alt+`: vẫn đổi cột độc lập với chữ phím tắt")
 app.showToolbarShortcuts = nil
 fakeUp = fakeUp + 1
 vk640ToggleShortcutHints()
 check(dialogs[#dialogs].err == true, "bản cũ thiếu API: hướng dẫn mở lại bản custom")
 vk640ShortcutHelp()
 check(dialogs[#dialogs].msg:find("Ctrl + `", 1, true) and dialogs[#dialogs].msg:find("Alt + `", 1, true),
       "F1 giải thích cả phím hiện chữ và phím đổi cột mới")
end

-- ================================================================ 10. bảng đăng ký đã xác nhận
print("== 10. BẢNG ĐĂNG KÝ UI ĐÃ XÁC NHẬN (đúng như main.lua) ==")
doc = docOf(1, 1, 0)
newApp(true)
print(string.format("   tổng số mục app.registerUi: %d", #registered))
print(string.format("   %-3s %-26s %-20s %s", "#", "callback", "accelerator", "toolbarId"))
for i, r in ipairs(registered) do
 print(string.format("   %-3d %-26s %-20s %s", i, r.callback, tostring(r.accelerator),
 tostring(r.toolbarId)))
end
check(#registered == #EXPECTED, string.format("bảng đăng ký vẫn đủ %d mục", #EXPECTED), #registered)

print(string.format("\nKẾT QUẢ: %d PASS, %d FAIL", pass, fail))
os.exit(fail == 0 and 0 or 1)
