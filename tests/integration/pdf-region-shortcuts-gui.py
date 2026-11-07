"""Production Ctrl+Shift+R + Alt+G, including empty object selections on a PDF."""
import os,time,pathlib,subprocess,gzip,xml.etree.ElementTree as ET
import pyatspi,cv2,numpy as np
from Xlib import X,XK,display
from Xlib.ext import xtest
from xkey import find_windows
root=pathlib.Path(os.environ['PDF_FORMULA_DIR'])
actions=dict(l.split('=') for l in (root/'region-actions.txt').read_text().splitlines())
def action(name):
 subprocess.run(['gdbus','call','--session','--dest',os.environ['PDF_FORMULA_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-'+actions[name],'[]','{}'],check=True,stdout=subprocess.DEVNULL);time.sleep(.4)
def walk(o,d=0):
 yield o
 if d<30:
  for c in o:
   if c:yield from walk(c,d+1)
def tree():
 result=[]
 for a in pyatspi.Registry.getDesktop(0):
  if a.get_process_id()==int(os.environ['PDF_FORMULA_PID']):result.extend(walk(a))
 return result
def find(n,role=None,timeout=6):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  for o in tree():
   if o.name==n and (role is None or o.getRoleName()==role):return o
  time.sleep(.1)
 raise AssertionError(f'Missing {n} ({role})')
def click(o):
 a=o.queryAction()
 for i in range(a.nActions):
  if a.getName(i) in ('click','press','activate'):assert a.doAction(i);time.sleep(.25);return
 raise AssertionError('No click '+o.name)
def focus_main():
 d=display.Display();wins=[];find_windows(d,d.screen().root,'xournalpp',wins);w=max(wins,key=lambda w:w.get_geometry().width*w.get_geometry().height);w.set_input_focus(X.RevertToParent,X.CurrentTime);d.sync();d.close()
def keys(*names):
 d=display.Display();codes=[d.keysym_to_keycode(XK.string_to_keysym(n)) for n in names]
 for c in codes:xtest.fake_input(d,X.KeyPress,c)
 for c in reversed(codes):xtest.fake_input(d,X.KeyRelease,c)
 d.sync();d.close();time.sleep(.4)
def shot(name):subprocess.run(['import','-window','root',str(root/name)],check=True)
def drag(bounds):
 shot('source-before-drag.png');im=cv2.imread(str(root/'source-before-drag.png'))
 mask=np.uint8((im[:,:,2]>220)&(im[:,:,1]<100)&(im[:,:,0]<100))*255
 contours,_=cv2.findContours(mask,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
 x,y,w,h=max((cv2.boundingRect(c) for c in contours if cv2.boundingRect(c)[2]>400 and cv2.boundingRect(c)[3]>400),key=lambda a:a[2]*a[3]);scale=(w-2)/595
 focus_main();d=display.Display();x1,y1,x2,y2=bounds
 xtest.fake_input(d,X.MotionNotify,x=round(x+x1*scale),y=round(y+y1*scale));xtest.fake_input(d,X.ButtonPress,1)
 for k in range(1,12):
  xtest.fake_input(d,X.MotionNotify,x=round(x+(x1+(x2-x1)*k/11)*scale),y=round(y+(y1+(y2-y1)*k/11)*scale));d.sync();time.sleep(.025)
 xtest.fake_input(d,X.ButtonRelease,1);d.sync();d.close();time.sleep(.5)
def ready(expected):
 d=display.Display();screen=d.screen();width,height=screen.width_in_pixels,screen.height_in_pixels;d.close()
 ext=find('Toán THPT · Đồ thị, tiếp tuyến, bảng').queryComponent().getExtents(pyatspi.DESKTOP_COORDS)
 assert ext.x>=0 and ext.y>=0 and ext.x+ext.width<=width and ext.y+ext.height<=height, f'Math dialog outside test screen: {ext}, {width}x{height}'
 end=time.monotonic()+18
 while time.monotonic()<end:
  e=find('pdf-formula-edit','text');raw=e.queryText().getText(0,-1)
  if raw and find('Nhận dạng vùng PDF','button').getState().contains(pyatspi.STATE_SENSITIVE):
   formula=find('math-f1','text').queryText().getText(0,-1)
   p=subprocess.run(['/home/phuc/xpp-src/build/tests/test-formula-import','--equivalent',formula,expected],capture_output=True,text=True)
   if p.returncode==0:return formula
  time.sleep(.1)
 shot('FAILED-shortcuts-no-formula.png')
 (root/'failed-accessibility.txt').write_text('\n'.join(o.getRoleName()+' '+repr(o.name) for o in tree()))
 raise AssertionError('Ctrl+Shift+R → drag → Alt+G failed to load/preview the selected PDF formula')
def scan(case,page,bounds,expected):
 action(case);time.sleep(.5);action(page);focus_main();keys('Control_L','Shift_L','r');action('tool')
 assert (root/'tool.txt').read_text()=='6','Ctrl+Shift+R did not select the actual rectangle tool'
 drag(bounds);focus_main();keys('Alt_L','g');ready(expected)
 assert not find('Đã đối chiếu ảnh, công thức và đồ thị','check box').getState().contains(pyatspi.STATE_CHECKED)
 assert not find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE)
 shot(case+'-auto-preview.png')
 click(find('Hủy','button'))
 # Retry the SAME region: opening/cancelling the math dialog must not consume it.
 focus_main();keys('Alt_L','g');ready(expected);click(find('Hủy','button'))
 print('SHORTCUT_REGION PASS',case,'real Ctrl+Shift+R/Alt+G, automatic preview, cancel/retry',flush=True)
time.sleep(1)
scan('text','page1',[55,84,330,130],'x^2-3*x+2')
scan('scan','page4',[55,84,310,160],'(x^2-1)/(x-1)')
# No stale region after changing page.
action('page1');focus_main();keys('Alt_L','g');time.sleep(.8)
assert find('pdf-formula-edit','text').queryText().getText(0,-1)==''
click(find('Hủy','button'))
print('SHORTCUT_REGION PASS stale page rejected',flush=True)

# Recognize the PDF background even when the rectangle selects a real ink stroke.
action('text');action('page1');action('ink');focus_main();keys('Control_L','Shift_L','r');drag([55,84,330,130]);focus_main();keys('Alt_L','g');ready('x^2-3*x+2');shot('ink-background-only.png');click(find('Hủy','button'))
action('stats');assert (root/'stats.txt').read_text()=='1 0 0','Recognition changed existing ink/elements'
# Cancelling did not consume the region needed by the production image-copy key.
focus_main();keys('Control_L','Shift_L','c');action('stats');assert (root/'stats.txt').read_text()=='1 0 1','PDF preview consumed the Ctrl+Shift+C rectangle'
def native(name):
 subprocess.run(['gdbus','call','--session','--dest',os.environ['PDF_FORMULA_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate',name,'[]','{}'],check=True,stdout=subprocess.DEVNULL);time.sleep(.4)
native('undo');action('stats');assert (root/'stats.txt').read_text()=='1 0 0'
native('undo');action('stats');assert (root/'stats.txt').read_text()=='0 0 0','Cancel added an undo action before the original ink'
print('SHORTCUT_REGION PASS selected ink preserved, background-only OCR, image-copy cache and undo unchanged',flush=True)
# One clean region now goes all the way to insertion without clicking Apply.
focus_main();keys('Control_L','Shift_L','r');drag([55,84,330,130]);focus_main();keys('Alt_L','g');ready('x^2-3*x+2')
click(find('Đã đối chiếu ảnh, công thức và đồ thị','check box'));assert find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE)
shot('reviewed-ready.png');click(find('Chèn cả cụm','button'));action('export');action('stats');assert int((root/'stats.txt').read_text().split()[0])>0
native('undo');action('stats');assert (root/'stats.txt').read_text()=='0 0 0'
print('SHORTCUT_REGION PASS direct automatic preview/review/insert/PDF export/one undo',flush=True)
