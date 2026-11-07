"""Select PDF with real pointer events, run installed OCR, confirm/insert/save/export."""
import os,time,subprocess,pathlib,json,gzip,xml.etree.ElementTree as ET
import pyatspi,cv2,numpy as np
from Xlib import X,XK,display
from Xlib.ext import xtest
root=pathlib.Path(os.environ['PDF_FORMULA_DIR'])
artifacts=pathlib.Path(os.environ.get('PDF_FORMULA_ARTIFACTS',str(root)))
def action(n,wait=.4):
 subprocess.Popen(['gdbus','call','--session','--dest',os.environ['PDF_FORMULA_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate',f'plugins.action-{n}','[]','{}'],stdout=subprocess.DEVNULL)
 time.sleep(wait)
def walk(o,d=0):
 yield o
 if d<25:
  for c in o:
   if c is not None:yield from walk(c,d+1)
def tree():
 result=[]
 for app in pyatspi.Registry.getDesktop(0):
  if app.get_process_id()==int(os.environ['PDF_FORMULA_PID']):result.extend(walk(app))
 return result
def find(name,role=None,timeout=6):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  for x in tree():
   if x.name==name and (role is None or x.getRoleName()==role):return x
  time.sleep(.1)
 dump();raise AssertionError(f'Missing {role} {name}')
def dump():
 lines=[]
 for x in tree():
  try:lines.append(f'{x.getRoleName()} {x.name!r}')
  except Exception:pass
 (root/'accessibility.txt').write_text('\n'.join(lines))
def click(o):
 a=o.queryAction()
 for i in range(a.nActions):
  if a.getName(i) in ('click','press','activate'):assert a.doAction(i);time.sleep(.25);return
 raise AssertionError('No click '+o.name)
def edit(name,text): assert find(name,'text').queryEditableText().setTextContents(text);time.sleep(.25)
def native_action(name):
 subprocess.run(['gdbus','call','--session','--dest',os.environ['PDF_FORMULA_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate',name,'[]','{}'],check=True,stdout=subprocess.DEVNULL);time.sleep(.4)
def keys(*names):
 d=display.Display();codes=[d.keysym_to_keycode(XK.string_to_keysym(n)) for n in names]
 for c in codes:xtest.fake_input(d,X.KeyPress,c)
 for c in reversed(codes):xtest.fake_input(d,X.KeyRelease,c)
 d.sync();d.close();time.sleep(.4)
def shot(name):
 subprocess.run(['import','-window','root',str(root/name)],check=True)
 im=cv2.imread(str(root/name));return im

def select_rect(bounds,name):
 im=shot(name)
 mask=np.uint8((im[:,:,2]>220)&(im[:,:,1]<100)&(im[:,:,0]<100))*255
 contours,_=cv2.findContours(mask,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
 rects=[cv2.boundingRect(c) for c in contours]
 rects=[r for r in rects if r[2]>400 and r[3]>400]
 if not rects:dump();raise AssertionError('No visible current PDF page border')
 x,y,w,h=max(rects,key=lambda r:r[2]*r[3]);scale=(w-2)/595
 # Every selected page must fit the display. Coordinates derive from actual page border.
 assert x+w<=im.shape[1] and y+h<=im.shape[0]
 x1,y1,x2,y2=bounds
 d=display.Display()
 # Focus only the visible test app window.
 from xkey import find_windows
 wins=[];find_windows(d,d.screen().root,'xournalpp',wins);win=max(wins,key=lambda q:q.get_geometry().width*q.get_geometry().height);win.set_input_focus(X.RevertToParent,X.CurrentTime)
 xtest.fake_input(d,X.MotionNotify,x=int(x+1+x1*scale),y=int(y+1+y1*scale));xtest.fake_input(d,X.ButtonPress,1);d.sync()
 for k in range(1,11):
  xtest.fake_input(d,X.MotionNotify,x=int(x+1+(x1+(x2-x1)*k/10)*scale),y=int(y+1+(y1+(y2-y1)*k/10)*scale));d.sync();time.sleep(.025)
 xtest.fake_input(d,X.ButtonRelease,1);d.sync();d.close();time.sleep(.6)

def ready():
 e=find('pdf-formula-edit','text',timeout=10)
 end=time.monotonic()+15
 while time.monotonic()<end:
  if e.queryText().getText(0,-1) and sensitive('Nhận dạng vùng PDF'):return e.queryText().getText(0,-1)
  time.sleep(.1)
 dump();raise AssertionError('OCR did not return candidate')
def sensitive(name):return find(name,'button').getState().contains(pyatspi.STATE_SENSITIVE)
def focus_main():
 from xkey import find_windows
 d=display.Display();wins=[];find_windows(d,d.screen().root,'xournalpp',wins)
 w=max(wins,key=lambda q:q.get_geometry().width*q.get_geometry().height);w.set_input_focus(X.RevertToParent,X.CurrentTime);d.sync();d.close()
def save(path):
 focus_main();keys('Control_L','Shift_L','s');time.sleep(.7)
 chooser=find('Save File','file chooser')
 texts=[o for o in walk(chooser) if o.getRoleName()=='text' and o.getState().contains(pyatspi.STATE_SHOWING)]
 assert texts,'No visible filename entry'
 texts[0].queryEditableText().setTextContents(str(path))
 shot('save-'+path.stem+'.png')
 click(next(o for o in walk(chooser) if o.getRoleName()=='button' and o.name in ('Save','Lưu')))
 end=time.monotonic()+5
 while time.monotonic()<end and not path.exists():time.sleep(.1)
 assert path.exists(),'Save missing'
 time.sleep(.5)

time.sleep(1)
action(0,.8);action(2);action(6)
action(9);select_rect([55,101,330,106],'00-linear-text-source.png')
click(find('Vẽ đồ thị từ vùng chọn','button'));ready()
click(find('Chọn vùng khác…','button'))
print('PDF_GUI PASS linear PDF text selection and reselect-region action',flush=True)
# Real multiline recognition, candidate switching, preserved edits and two curves.
select_rect([55,84,330,210],'00-multiple-source.png')
click(find('Vẽ đồ thị từ vùng chọn','button'));ready()
box=find('pdf-formula-candidates','combo box')
def choose_formula(power):
 for index in range(12):
  selected=box.querySelection().selectChild(index)
  if not selected:continue
  time.sleep(.15)
  raw=find('pdf-formula-edit','text').queryText().getText(0,-1)
  if (('²' if power==2 else '³') in raw) or f'^{power}' in raw or f'^{{{power}}}' in raw:return index,raw
 shot('00-multiple-failed.png');dump();raise AssertionError('No requested formula candidate')
ci,raw=choose_formula(3);edit('pdf-formula-edit',raw+'+1');click(find('Dùng làm hàm 1','button'))
qi,raw2=choose_formula(2);click(find('Thêm làm hàm 2','button'))
# Select the edited cubic again and verify its correction was retained.
assert box.querySelection().selectChild(ci)
time.sleep(.2)
assert find('pdf-formula-edit','text').queryText().getText(0,-1).endswith('+1'),'candidate edit lost on switching'
assert find('Hàm 2','check box').getState().contains(pyatspi.STATE_CHECKED)
click(find('Đã đối chiếu ảnh, công thức và đồ thị','check box'));assert sensitive('Chèn cả cụm');shot('00-multiple-preview.png')
click(find('Chọn vùng khác…','button'))
print('PDF_GUI PASS multiple candidates/preserved correction/two-curve preview',flush=True)
select_rect([55,84,330,130],'01-text-source.png')
click(find('Vẽ đồ thị từ vùng chọn','button'))
shot('01b-after-click.png');dump()
raw=ready();print('PDF_GUI text OCR:',raw,flush=True)
shot('02-text-recognition.png')
assert not sensitive('Chèn cả cụm'),'unreviewed OCR allowed insert'
click(find('Dùng làm hàm 1','button'))
assert find('math-f1','text').queryText().getText(0,-1)
# Invalid recognition can be repaired in place, without losing range or candidate.
edit('pdf-formula-edit',r'\frac{1}{x')
click(find('Dùng làm hàm 1','button'));assert not sensitive('Chèn cả cụm')
edit('pdf-formula-edit','y=x²−3x+2');click(find('Dùng làm hàm 1','button'))
click(find('Đã đối chiếu ảnh, công thức và đồ thị','check box'));assert sensitive('Chèn cả cụm')
assert any(o.getRoleName()=='label' and o.name.startswith('Đã đối chiếu · Sẵn sàng') for o in tree()),'Review state label is stale'
shot('03-text-preview-reviewed.png')
# Cancellation has no document elements/undo side effects.
click(find('Hủy','button'));action(6)
lines=(root/'snapshot.txt').read_text().splitlines();assert [s for s in lines if s.startswith('SNAP')]==['SNAP 0 0 0 pages 1','SNAP 0 0 0 pages 1'],lines
# Re-open the same selection and insert through the common path.
action(4);ready();click(find('Dùng làm hàm 1','button'));click(find('Đã đối chiếu ảnh, công thức và đồ thị','check box'));assert sensitive('Chèn cả cụm');click(find('Chèn cả cụm','button'))
shot('04-text-inserted.png')
save(artifacts/'text-evidence.xopp')
# A single undo/redo includes TexImage, strokes and labels.
native_action('undo');save(artifacts/'text-undo.xopp')
a=ET.fromstring(gzip.open(artifacts/'text-undo.xopp','rb').read());assert not a.findall('.//stroke') and not a.findall('.//teximage'),'one undo left graph/formula elements'
native_action('redo');save(artifacts/'text-redo.xopp')
a=ET.fromstring(gzip.open(artifacts/'text-redo.xopp','rb').read());assert a.findall('.//teximage') and a.findall('.//stroke'),'redo did not restore formula and graph'
print('PDF_GUI PASS text/select/recognize/edit/review/cancel/insert/one-undo-redo',flush=True)
# Native clipboard serialization includes TexImage and its source/PDF.
native_action('select-all');native_action('copy');native_action('paste');save(artifacts/'copy-paste.xopp')
copied=ET.fromstring(gzip.open(artifacts/'copy-paste.xopp','rb').read());tex=copied.findall('.//teximage')
assert len(tex)==2 and tex[0].text==tex[1].text and tex[0].get('text')==tex[1].get('text'),'clipboard lost formula data/source'
native_action('undo')
print('PDF_GUI PASS native copy/paste preserves embedded vector PDF and TeX source',flush=True)

# Separate scan PDF, with no text layer at all. Select a printed fraction with a hole.
action(1,.8);action(3)
select_rect([55,84,310,160],'05-scan-source.png')
click(find('Vẽ đồ thị từ vùng chọn','button'));raw=ready();print('PDF_GUI scan OCR:',raw,flush=True)
# Restart, cancel a real in-flight worker, then retry without restarting the app.
click(find('Nhận dạng vùng PDF','button'));click(find('Dừng nhận dạng','button'))
assert not sensitive('Chèn cả cụm')
click(find('Nhận dạng vùng PDF','button'));ready()
print('PDF_GUI PASS cancel/retry real OCR worker',flush=True)

click(find('Dùng làm hàm 1','button'));assert '/(x-1)' in find('math-f1','text').queryText().getText(0,-1).replace('(','(') or 'x-1' in find('math-f1','text').queryText().getText(0,-1)
click(find('Đã đối chiếu ảnh, công thức và đồ thị','check box'));shot('06-scan-preview.png');click(find('Chèn cả cụm','button'));shot('07-scan-inserted.png')
save(artifacts/'evidence.xopp');action(5,.8)
assert (artifacts/'evidence.pdf').exists()
before=ET.fromstring(gzip.open(artifacts/'evidence.xopp','rb').read());assert before.findall('.//teximage')
action(7,.8);shot('08-reopened.png');save(artifacts/'reopened.xopp')
after=ET.fromstring(gzip.open(artifacts/'reopened.xopp','rb').read())
for tag in ['stroke','text','teximage']:
 assert len(before.findall('.//'+tag))==len(after.findall('.//'+tag)),f'{tag} count changed'
for a,b in zip(before.findall('.//teximage'),after.findall('.//teximage')):
 assert a.attrib==b.attrib and a.text==b.text,'formula source/embedded font PDF changed on reopen'
print('PDF_GUI PASS scan/select/recognize/fraction/hole/insert/save/reopen/PDF',flush=True)
# A separate formula object shares atomic undo and the normal persistent format.
action(4);click(find('Chỉ chèn công thức (PDF vector)','check box'));time.sleep(.4)
assert sensitive('Chèn cả cụm');click(find('Chèn cả cụm','button'));save(artifacts/'formula-only.xopp')
only=ET.fromstring(gzip.open(artifacts/'formula-only.xopp','rb').read())
assert len(only.findall('.//teximage'))==len(after.findall('.//teximage'))+1
assert len(only.findall('.//stroke'))==len(after.findall('.//stroke'))
shot('09-formula-only.png')
# Page changes invalidate the preview and cannot silently redirect insertion.
action(4);action(8);time.sleep(.5);assert not sensitive('Chèn cả cụm');shot('10-page-changed.png');click(find('Hủy','button'))
print('PDF_GUI PASS standalone vector formula and page-change insertion guard',flush=True)
