function floatingSnapshot()
 local f=assert(io.open(os.getenv('FLOATING_NOTES_OUT')..'/snapshot.lua','w'))
 local notes,base=app.getFloatingNotes()
 local scroll=app.getScrollPos()
 f:write('return {base=',base,',mainY=',scroll.y,',mainZoom=',app.getZoom(),',tool=',string.format('%q',app.getToolInfo('active').type),',columns=',app.getActionState('set-columns-or-rows'),',notes={')
 for _,n in ipairs(notes) do
  f:write('{')
  for k,v in pairs(n) do f:write(k,'=',type(v)=='string' and string.format('%q',v) or tostring(v),',') end
  f:write('},')
 end
 f:write('}}'); f:close()
end
function floatingExport() app.clearSelection();app.export{outputFile=os.getenv('FLOATING_NOTES_OUT')..'/main.pdf',backend='cairo'} end
function floatingReopen() app.openFile(os.getenv('FLOATING_NOTES_OUT')..'/evidence.xopp',1,true) end
function floatingFixture() app.openFile(os.getenv('FLOATING_NOTES_OUT')..'/fixture-100.xopp',1,true) end
function floatingPreparePdf() app.setZoom(1);app.scrollToPage(1);app.changeActionState('select-tool',22) end
function floatingShowMath() app.showMathTools() end
function initUi()
 for _,p in ipairs{{'Floating snapshot','floatingSnapshot'},{'Floating export','floatingExport'},{'Floating reopen','floatingReopen'},{'Floating fixture','floatingFixture'},{'Floating math','floatingShowMath'},{'Floating PDF source','floatingPreparePdf'}} do app.registerUi{menu=p[1],callback=p[2]} end
end
