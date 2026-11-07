"""Acceptance against the actual dialog through AT-SPI; no private app hooks."""
import os,time,subprocess,pathlib,json
import pyatspi
from Xlib import X,XK,display
from Xlib.ext import xtest
root=pathlib.Path(os.environ['MATH_TEST_DIR'])
def action(n):
 fn=subprocess.Popen if n==2 else subprocess.run
 fn(['gdbus','call','--session','--dest',os.environ['MATH_TEST_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate',f'plugins.action-{n}','[]','{}'],stdout=subprocess.DEVNULL,**({} if n==2 else {'check':True}))
 time.sleep(.4)
def screenshot(name):
 d=display.Display();screen=d.screen();windows=[]
 from xkey import find_windows
 find_windows(d,screen.root,'xournalpp',windows)
 for w in windows:
  geometry=w.get_geometry();assert geometry.width<=screen.width_in_pixels and geometry.height<=screen.height_in_pixels, f'Test window exceeds actual screen: {geometry.width}x{geometry.height}'
 d.close()
 subprocess.run(['import','-window','root',str(root/name)],check=True)
def walk(obj,depth=0):
 yield obj
 if depth<24:
  for child in obj:
   if child is not None:yield from walk(child,depth+1)
def tree():return list(walk(pyatspi.Registry.getDesktop(0)))
def dump():
 (root/'accessibility.txt').write_text('\n'.join(f'{x.getRoleName()} {x.name!r}' for x in tree()))
def find(name,role=None):
 for _ in range(30):
  for x in tree():
   if x.name==name and (role is None or x.getRoleName()==role):return x
  time.sleep(.1)
 dump();raise AssertionError(f'No accessible {role} {name!r}')
def click(obj):
 a=obj.queryAction()
 for i in range(a.nActions):
  if a.getName(i) in ('click','press','activate'):assert a.doAction(i);time.sleep(.3);return
 raise AssertionError(f'No click action: {obj.name}')
def edit(name,value):
 obj=find('math-'+name,'text');assert obj.queryEditableText().setTextContents(value);time.sleep(.22)
def combo(index):
 # The mode combo is the first one inside the real teaching dialog.
 dialog=next(x for x in tree() if x.getRoleName()=='dialog' and x.name.startswith('Toán THPT'))
 obj=next(x for x in walk(dialog) if x.getRoleName()=='combo box')
 obj.queryComponent().grabFocus();keys('Home')
 for _ in range(index):keys('Down')
 time.sleep(.3)
def keys(*names):
 d=display.Display();codes=[d.keysym_to_keycode(XK.string_to_keysym(n)) for n in names]
 for c in codes:xtest.fake_input(d,X.KeyPress,c)
 for c in reversed(codes):xtest.fake_input(d,X.KeyRelease,c)
 d.sync();d.close();time.sleep(.4)
if os.getenv('MATH_STRESS_ONLY'):
 for _ in range(60):action(6)
 print('MATH_GUI PASS 60 consecutive page insertions')
 raise SystemExit(0)
time.sleep(1)
action(3);action(0);time.sleep(.6);screenshot('01-selected-graph.png')
assert 'MATH_RESULT PASS' in (root/'app.log').read_text(),(root/'app.log').read_text()
action(1)
action(2);screenshot('02-dialog.png');dump()
# Cancel through the real button and verify preview did not mutate the document.
click(find('Hủy','button'));action(1)
# Preview updates, two curves, parameter, field error, and insertion via actual widgets.
assert (root/'geometry-0.txt').read_text()==(root/'geometry-1.txt').read_text(),'Cancel changed page geometry'
action(6);action(2)
edit('f1','x^2');edit('f2','m*x')
click(find('Hàm 2','check box'))
find('math-m','spin button').queryValue().currentValue=2
combo(1);edit('xmin','−3');edit('xmax','exp(1)+1');assert find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE),'Constant exp(1) was mistaken for x';edit('xmax','3');edit('ymin','-1');edit('ymax','10');edit('x0','1')
screenshot('05-tangent-preview.png')
edit('f1','sin(1000000*x)/(x-0.123456)')
assert not find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE),'Hard input did not report sampling budget'
edit('f1','x^2')
edit('x0','invalid')
assert not find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE),'Invalid input allowed insert'
edit('x0','1');assert find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE)
click(find('Chèn cả cụm','button'));screenshot('06-tangent-inserted.png')
# Last template/parameters survive closing the dialog.
action(2)
assert find('math-f1','text').queryText().getText(0,-1)=='x^2','Last formula was not remembered'
assert find('math-ymax','text').queryText().getText(0,-1)=='10','Last range was not remembered'
click(find('Hủy','button'))
# Automatic table, manual edits and row visibility use the same preview and Insert path.
action(6);action(2);edit('f1','x^3-3*x');combo(2)
click(find('Tự sinh bảng từ hàm 1 → sửa các ô bên dưới','button'))
assert '-1' in find('math-table-x','text').queryText().getText(0,-1)
# Pen controls edit cells without pipe syntax and preserve the focused entry.
old_x=find('math-table-x','text').queryText().getText(0,-1)
click(find('math-add-column','button'))
assert len(find('math-table-x','text').queryText().getText(0,-1).split('|'))==8
click(find('math-remove-column','button'))
assert find('math-table-x','text').queryText().getText(0,-1)==old_x
cell=find('math-cell-x-2','text');cell.queryComponent().grabFocus();cell.queryEditableText().setTextContents('-2');time.sleep(.3)
assert find('math-cell-x-2','text').queryText().getText(0,-1)=='-2'
assert find('math-cell-x-2','text').getState().contains(pyatspi.STATE_FOCUSED),'Typing rebuilt the table and lost pen focus'
find('math-cell-x-2','text').queryEditableText().setTextContents('-1');time.sleep(.25)
click(find('math-cell-sign-1','button'))
assert find('math-table-sign','text').queryText().getText(0,-1).split('|')[1]=='−'
value=find('math-cell-value-1','text');value.queryComponent().grabFocus();value.queryEditableText().setTextContents('');time.sleep(.25)
click(find('↘','button'))
assert find('math-cell-value-1','text').queryText().getText(0,-1)=='↘','Symbol palette did not edit focused value cell'
screenshot('10-pen-table-controls.png')
# Extra columns remain accessible by large pen-friendly scroll buttons.
for _ in range(2):click(find('math-add-column','button'))
rows_before_scroll=find('math-table-x','text').queryText().getText(0,-1)
first=find('math-cell-x-0','text').queryComponent().getExtents(pyatspi.DESKTOP_COORDS).x
click(find('math-scroll-right','button'))
shifted=find('math-cell-x-0','text').queryComponent().getExtents(pyatspi.DESKTOP_COORDS).x
assert shifted<first,'Right scroll button did not scroll table columns'
click(find('math-scroll-left','button'))
assert find('math-cell-x-0','text').queryComponent().getExtents(pyatspi.DESKTOP_COORDS).x==first
assert find('math-table-x','text').queryText().getText(0,-1)==rows_before_scroll,'Scrolling changed table data'
# Exercise both supported column-count boundaries without changing the document.
for _ in range(10):click(find('math-add-column','button'))
assert len(find('math-table-x','text').queryText().getText(0,-1).split('|'))==19
assert not find('math-add-column','button').getState().contains(pyatspi.STATE_SENSITIVE)
for _ in range(17):click(find('math-remove-column','button'))
assert len(find('math-table-x','text').queryText().getText(0,-1).split('|'))==2
assert not find('math-remove-column','button').getState().contains(pyatspi.STATE_SENSITIVE)
# Return to the certified cubic table for the evidence workbook.
click(find('Tự sinh bảng từ hàm 1 → sửa các ô bên dưới','button'))
click(find('Hiện hàng giá trị / chiều biến thiên','check box'));screenshot('07-table-hidden-preview.png')
click(find('Hiện hàng giá trị / chiều biến thiên','check box'))
edit('table-name','f′');screenshot('08-table-preview.png');click(find('Chèn cả cụm','button'))
screenshot('09-table-inserted.png');action(1);action(7);action(8)
# Separate vector examples in the saved workbook and PDF.
for i in range(9):
 action(6);action(9);screenshot(f'example-{i+1}.png')
action(1)
# Save via the application's normal keyboard action and native file chooser.
keys('Control_L','Shift_L','s');time.sleep(.6);dump();screenshot('03-save-dialog.png')
# Ctrl+L exposes the path entry in the chooser on all GTK themes.
keys('Control_L','l');time.sleep(.2)
# AT-SPI editable text targets the focused chooser entry, avoids keyboard layout assumptions.
focused=[x for x in tree() if x.getState().contains(pyatspi.STATE_FOCUSED)]
assert focused, 'No focused save location'
focused[-1].queryEditableText().setTextContents(str(root/'stage1.xopp'))
keys('Return');time.sleep(1)
# Some chooser variants need a second Save confirmation after accepting the location.
if not (root/'stage1.xopp').exists():
 for x in tree():
  if x.getRoleName()=='button' and x.name in ('Save','Lưu'):click(x);break
 time.sleep(1)
assert (root/'stage1.xopp').exists(),'Normal save did not produce .xopp'
action(4);time.sleep(1);assert (root/'stage1.pdf').exists()
action(5);time.sleep(.6);screenshot('04-reopened.png')
files=sorted(root.glob('geometry-*.txt'),key=lambda p:int(p.stem.split('-')[1]))
import re
before,after=[p.read_text().splitlines() for p in files[-2:]]
assert len(before)==len(after),'Save/reopen changed element/point count'
max_error=0
for a,b in zip(before,after):
 if a.startswith('S '):assert a==b,'Stroke page/layer/color/width/dashes changed'
 elif a.startswith('T '):
  aa=a.split(' ',5);bb=b.split(' ',5);assert aa[:3]==bb[:3] and aa[5]==bb[5],'Text or page/layer changed'
  max_error=max(max_error,abs(float(aa[3])-float(bb[3])),abs(float(aa[4])-float(bb[4])))
 else:
  aa=list(map(float,a.split()));bb=list(map(float,b.split()));assert len(aa)==len(bb)
  max_error=max(max_error,max(abs(x-y) for x,y in zip(aa,bb)))
assert max_error<1e-4,f'Save/reopen geometry error {max_error} exceeds 1e-4 pt'
print(f'MATH_GUI PASS save/reopen geometry, text, color and dashes; max error {max_error:.8g} pt')
# Make the app save the reopened document again for a structural serialization comparison.
keys('Control_L','s');time.sleep(.8)
# Keep the twelve-page native save before stress adds extra unsaved pages.
# The application's normal signal-close can save those extra test pages.
import shutil,gzip,xml.etree.ElementTree as ET
assert len(ET.fromstring(gzip.open(root/'stage1.xopp','rb').read()).findall('page'))==12
shutil.copy2(root/'stage1.xopp',root/'stage1-evidence.xopp')
for _ in range(30):action(10)
print('MATH_GUI PASS cancel/input/preview/last-template/atomic-undo/save/reopen/PDF and 30 pages during background rendering')
