local base=os.getenv('PDF_FORMULA_FIXTURES')
function pdfOpenText() app.openFile(base..'/unicode-text.pdf',1,true) end
function pdfOpenScan() app.openFile(base..'/formulas-scan.pdf',4,true) end
function pdfPrepare()
 app.setZoom(1);app.scrollToPage(1);app.changeActionState('select-tool',22)
end
function pdfPrepareScan()
 app.setCurrentPage(4);app.scrollToPage(4);app.setZoom(1);app.changeActionState('select-tool',22)
end
function pdfPrepareLinear() app.changeActionState('select-tool',21) end
function pdfShow() app.showMathTools() end
function pdfExport()
 app.clearSelection();app.export{outputFile=os.getenv('PDF_FORMULA_ARTIFACTS')..'/evidence.pdf',backend='cairo'}
end
function pdfSnapshot()
 local f=assert(io.open(os.getenv('PDF_FORMULA_DIR')..'/snapshot.txt','a'))
 local d=app.getDocumentStructure();local v=app.getVisiblePageRect()
 f:write('SNAP ',#app.getStrokes('all'),' ',#app.getTexts('all'),' ',#app.getImages('all'),' pages ',#d.pages,'\n')
 if v then f:write('VISIBLE ',v.x,' ',v.y,' ',v.width,' ',v.height,'\n') end
 f:close()
end
function pdfReopen() app.openFile(os.getenv('PDF_FORMULA_ARTIFACTS')..'/evidence.xopp',1,true) end
function pdfChangePage() app.setCurrentPage(5) end
function initUi()
 for _,p in ipairs{{'Open text','pdfOpenText'},{'Open scan','pdfOpenScan'},{'Prepare','pdfPrepare'},{'Prepare scan','pdfPrepareScan'},{'Show math','pdfShow'},{'Export','pdfExport'},{'Snapshot','pdfSnapshot'},{'Reopen','pdfReopen'},{'Change page','pdfChangePage'},{'Linear selection','pdfPrepareLinear'}} do app.registerUi{menu=p[1],callback=p[2]} end
end
