dofile(os.getenv('VK640_PLUGIN'))
local checks,failures=0,0
local function check(ok,s)
 checks=checks+1;if not ok then failures=failures+1 end
 print((ok and 'MATH_PASS ' or 'MATH_FAIL ')..s);io.stdout:flush()
end
local function count()
 app.clearSelection()
 return #app.getStrokes('page'),#app.getTexts('page')
end
function mathProbe()
 local ok,err=pcall(function()
  local original=app.addStrokes{strokes={{x={10,30},y={10,10},width=1,color=0x123456,tool='pen'}}}
  app.addTexts{texts={{text='Nội dung gốc được giữ',x=10,y=16,font={name='Sans',size=10}}}}
  local s0,t0=count();app.addToSelection(original)
  local success,message=app.insertMathFigure{formula='1/x',xmin=-2,xmax=2,ymin=-6,ymax=6}
  check(success,message or 'graph insertion succeeds')
  local selectedStrokes=app.getStrokes('selection');local selectedTexts=app.getTexts('selection')
  check(#selectedStrokes>2 and #selectedTexts>2,'mixed graph vectors and labels are selected together')
  check(app.getToolInfo('active').type=='selectRect' and app.getActionState('select-tool')==app.C.Tool_selectRect,'inserted group is movable immediately')
  local visible=app.getVisiblePageRect();local fits=true
  for _,s in ipairs(selectedStrokes) do for i,x in ipairs(s.x) do fits=fits and x>=visible.x and x<=visible.x+visible.width and s.y[i]>=visible.y and s.y[i]<=visible.y+visible.height end end
  for _,t in ipairs(selectedTexts) do fits=fits and t.x>=visible.x and t.y>=visible.y and t.x+t.width<=visible.x+visible.width and t.y+t.height<=visible.y+visible.height end
  check(fits,'graph and actual laid-out text fit visible zoomed/scrolled page')
  local s1,t1=count();app.activateAction('undo');local s2,t2=count();check(s2==s0 and t2==t0,'one undo removes graph strokes AND text')
  app.activateAction('redo');local s3,t3=count();check(s3==s1 and t3==t1,'one redo restores graph strokes AND text')
  local ok2,msg=app.insertMathFigure{formula='abs(x)',tangent=true,x0=0};local s4,t4=count();check(ok2==nil and msg and s4==s3 and t4==t3,'invalid tangent reports error and leaves page untouched')
  check(app.insertMathFigure{formula='x^3-3*x',mode=1},'automatic cubic variation table inserts')
  local ts,tt=count();app.activateAction('undo');local us,ut=count();check(us==s3 and ut==t3,'one undo removes entire variation table, preserving graph')
  app.activateAction('redo');local rs,rt=count();check(rs==ts and rt==tt,'one redo restores entire variation table')
  check(app.insertMathFigure{mode=2,tableX='−∞||0||+∞',tableSign='|−|0|+|',tableValue='+∞|↘|0|↗|+∞'},'manual sign table inserts with editable text')
  app.clearSelection();app.activateAction('undo');app.activateAction('undo');app.activateAction('undo');local es,et=count();check(es==s0 and et==t0,'three group undos return to exact baseline')
  for _,formula in ipairs({'2^x','sqrt(0)','abs(0)','x+abs(0)'}) do
   local inserted,message=app.insertMathFigure{formula=formula,tangent=true,x0=0,xmin=-2,xmax=2,ymin=-2,ymax=5}
   check(inserted,message or 'valid analytic tangent: '..formula)
   if inserted then app.clearSelection();app.activateAction('undo') end
  end
  check(app.insertMathFigure{formula='x^2',tangent=true,x0=0},'zero-point tangent inserts')
  local labels=app.getTexts('selection');local separated=true
  for i,a in ipairs(labels) do for j,b in ipairs(labels) do if i<j then separated=separated and not(a.x<b.x+b.width and a.x+a.width>b.x and a.y<b.y+b.height and a.y+a.height>b.y) end end end
  check(separated,'all actual text boxes, including O and M(0,0), avoid overlap')
  app.clearSelection();app.activateAction('undo')
  check(app.insertMathFigure{formula='1/x',mode=1},'pole variation table inserts')
  local paths=app.getStrokes('selection');local labels=app.getTexts('selection');local xs={}
  for _,p in ipairs(paths) do if #p.x==2 and math.abs(p.x[1]-p.x[2])<1e-8 then xs[#xs+1]=p.x[1] end end
  table.sort(xs);local center
  for i=2,#xs do if math.abs(xs[i]-xs[i-1]-4)<1e-6 then center=(xs[i]+xs[i-1])/2 end end
  local centered=false;for _,t in ipairs(labels) do if t.text=='0 (loại)' and center then centered=math.abs(t.x+t.width/2-center)<1.5 end end
  check(centered,'actual breakpoint label centered over the double exclusion rule')
  app.clearSelection();app.activateAction('undo')
  local inserted,why=app.insertMathFigure{formula='(x^2-1)/(x-1)',formula2='m*x',second=true,m=2,xmin=-2,xmax=3,ymin=-2,ymax=5}
  check(inserted,why or 'two curves and original removable hole insert')
 end)
 if not ok then check(false,tostring(err)) end
 print(string.format('MATH_RESULT %s (%d checks, %d failures)',failures==0 and 'PASS' or 'FAIL',checks,failures));io.stdout:flush()
end
local snapshotIndex=0
function mathSnapshot()
 local s,t=count();print('MATH_SNAPSHOT '..s..' '..t)
 local out=assert(io.open(os.getenv('MATH_TEST_DIR')..'/geometry-'..snapshotIndex..'.txt','w'))
 for _,stroke in ipairs(app.getStrokes('all')) do
  out:write('S ',stroke.page or 0,' ',stroke.layer or 0,' ',stroke.color,' ',string.format('%.6f',stroke.width),' ',stroke.lineStyle or 'plain','\n')
  for i,x in ipairs(stroke.x) do out:write(string.format('%.6f %.6f\n',x,stroke.y[i])) end
 end
 for _,text in ipairs(app.getTexts('all')) do out:write(string.format('T %d %d %.6f %.6f %s\n',text.page or 0,text.layer or 0,text.x,text.y,text.text)) end
 out:close();snapshotIndex=snapshotIndex+1;io.stdout:flush()
end
function mathNewPage() app.clearSelection();app.activateAction('new-page-after') end
function mathUndo() app.activateAction('undo');mathSnapshot() end
function mathRedo() app.activateAction('redo');mathSnapshot() end
local example=0
function mathExample()
 example=example+1
 local examples={
 {formula='1/x',xmin=-2,xmax=2,ymin=-6,ymax=6},
 {formula='(x^2+1)/(x-1)',xmin=-3,xmax=5,ymin=-8,ymax=8},
 {formula='ln(x)',formula2='sqrt(x)',second=true,xmin=-1,xmax=5,ymin=-3,ymax=4},
 {formula='sin(x)',formula2='cos(m*x)',second=true,m=1,piTicks=true,xtick=.5,xmin=-6.283185307179586,xmax=6.283185307179586,ymin=-2,ymax=2},
 {formula='x^3',mode=1},
 {formula='1/x',mode=1},
 {formula='2^x',formula2='(1/2)^x',second=true,xmin=-3,xmax=3,ymin=-1,ymax=6},
 {formula='x^2',tangent=true,x0=0,xmin=-3,xmax=3,ymin=-1,ymax=5},
 {formula='1000000*(x-.0078125)^3',xmin=-1,xmax=1,ymin=-1,ymax=1},
 }
 local success,message=app.insertMathFigure(examples[example]);check(success,message or 'evidence example '..example)
end
function mathView()
 app.setZoom(1.5);app.scrollToPos(80,220,false)
end
function mathExport()
 app.clearSelection();app.export{outputFile=os.getenv('MATH_TEST_DIR')..'/stage1.pdf',backend='cairo'}
 print('MATH_EXPORT requested');io.stdout:flush()
end
function mathReopen()
 app.openFile(os.getenv('MATH_TEST_DIR')..'/stage1.xopp',1,true)
 mathSnapshot()
end
function mathRenderStress()
 mathNewPage()
 local ok,message=app.insertMathFigure{formula='sin(x)',formula2='cos(m*x)',second=true,m=2}
 check(ok,message or 'page insertion during background rendering')
end
function initUi()
 app.registerUi{menu='Math probe',callback='mathProbe'}
 app.registerUi{menu='Math snapshot',callback='mathSnapshot'}
 app.registerUi{menu='Math dialog',callback='vk640MathTools'}
 app.registerUi{menu='Math view',callback='mathView'}
 app.registerUi{menu='Math export',callback='mathExport'}
 app.registerUi{menu='Math reopen',callback='mathReopen'}
 app.registerUi{menu='Math new page',callback='mathNewPage'}
 app.registerUi{menu='Math undo',callback='mathUndo'}
 app.registerUi{menu='Math redo',callback='mathRedo'}
 app.registerUi{menu='Math example',callback='mathExample'}
 app.registerUi{menu='Math render stress',callback='mathRenderStress'}
end
