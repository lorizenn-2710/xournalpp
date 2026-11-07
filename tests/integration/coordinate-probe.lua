function coordinateShow() app.showMathTools() end
function coordinateExport() app.clearSelection();app.export{outputFile=os.getenv('PDF_FORMULA_ARTIFACTS')..'/evidence.pdf',backend='cairo'} end
function coordinateSnapshot()
 local f=assert(io.open(os.getenv('PDF_FORMULA_DIR')..'/snapshot.txt','a'))
 local v=app.getVisiblePageRect()
 local ok,selected=pcall(app.getStrokes,'selection');if not ok then selected={} end
 local ok2,texts=pcall(app.getTexts,'selection');if not ok2 then texts={} end
 f:write(#app.getStrokes('all'),' ',#app.getTexts('all'),' ',#app.getImages('all'),' selection ',#selected,' ',#texts,'\n')
 if v then f:write('VISIBLE ',v.x,' ',v.y,' ',v.width,' ',v.height,'\n') end
 f:close()
end
function coordinateReopen() app.openFile(os.getenv('PDF_FORMULA_ARTIFACTS')..'/evidence.xopp',1,true) end
function coordinatePage() app.clearSelection();app.activateAction('new-page-after') end
function coordinateUndo() app.activateAction('undo') end
function coordinateRedo() app.activateAction('redo') end
function coordinateCopy() app.activateAction('select-all');app.activateAction('copy') end
function coordinatePaste() app.activateAction('paste') end
function coordinateOtherPage() app.setCurrentPage(1) end
function coordinateView() app.setZoom(1);app.scrollToPos(0,80,false) end
function initUi()
 for _,p in ipairs{{'Coordinate tools','coordinateShow'},{'Coordinate export','coordinateExport'},{'Coordinate snapshot','coordinateSnapshot'},{'Coordinate reopen','coordinateReopen'},{'Coordinate page','coordinatePage'},{'Coordinate undo','coordinateUndo'},{'Coordinate redo','coordinateRedo'},{'Coordinate copy','coordinateCopy'},{'Coordinate paste','coordinatePaste'},{'Coordinate other page','coordinateOtherPage'},{'Coordinate view','coordinateView'}} do app.registerUi{menu=p[1],callback=p[2]} end
end
