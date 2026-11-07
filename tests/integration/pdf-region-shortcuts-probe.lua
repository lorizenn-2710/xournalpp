-- Load the actual production accelerators and callbacks, including Alt+G.
dofile(os.getenv('VK640_PLUGIN'))
local productionInit=initUi
local base=os.getenv('PDF_FORMULA_FIXTURES')
function regionText() app.openFile(base..'/unicode-text.pdf',1,true) end
function regionScan() app.openFile(base..'/formulas-scan.pdf',4,true) end
function regionPage1() app.setCurrentPage(1);app.scrollToPage(1);app.setZoom(1) end
function regionPage4() app.setCurrentPage(4);app.scrollToPage(4);app.setZoom(1) end
function regionTool()
 local f=assert(io.open(os.getenv('PDF_FORMULA_DIR')..'/tool.txt','w'));f:write(tostring(app.getActionState('select-tool')));f:close()
end
function regionInk() app.addStrokes{strokes={{x={65,210},y={105,105},width=12,color=0xff0000,tool='pen'}}} end
function regionStats()
 local f=assert(io.open(os.getenv('PDF_FORMULA_DIR')..'/stats.txt','w'));f:write(#app.getStrokes('all'),' ',#app.getTexts('all'),' ',#app.getImages('all'));f:close()
end
function regionExport() app.clearSelection();app.export{outputFile=os.getenv('PDF_FORMULA_DIR')..'/shortcut.pdf',backend='cairo'} end
function initUi()
 productionInit()
 local f=assert(io.open(os.getenv('PDF_FORMULA_DIR')..'/region-actions.txt','w'))
 for _,p in ipairs{{'text','regionText'},{'scan','regionScan'},{'page1','regionPage1'},{'page4','regionPage4'},{'tool','regionTool'},{'export','regionExport'},{'ink','regionInk'},{'stats','regionStats'}} do
  local a=app.registerUi{menu='Region test '..p[1],callback=p[2]};f:write(p[1],'=',a.menuId,'\n')
 end
 f:close()
end
