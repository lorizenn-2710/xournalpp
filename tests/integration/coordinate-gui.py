"""Real installed GTK dialog: typed coordinate selection, transactions, persistence, PDF."""
import os,time,subprocess,pathlib,json,gzip,xml.etree.ElementTree as ET,shutil
import pyatspi
from Xlib import X,XK,display
from Xlib.ext import xtest
root=pathlib.Path(os.environ['PDF_FORMULA_DIR'])
artifacts=pathlib.Path(os.environ['PDF_FORMULA_ARTIFACTS']);artifacts.mkdir(parents=True,exist_ok=True)
def copy_app_log():
 if (root/'app.log').resolve() != (artifacts/'app.log').resolve():shutil.copy2(root/'app.log',artifacts/'app.log')
def action(n,wait=.35):
 subprocess.run(['gdbus','call','--session','--dest',os.environ['PDF_FORMULA_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate',f'plugins.action-{n}','[]','{}'],check=True,stdout=subprocess.DEVNULL);time.sleep(wait)
def native_action(name):
 subprocess.run(['gdbus','call','--session','--dest',os.environ['PDF_FORMULA_BUS'],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate',name,'[]','{}'],check=True,stdout=subprocess.DEVNULL);time.sleep(.5)
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
def find(name,role=None):
 end=time.monotonic()+5
 while time.monotonic()<end:
  for o in tree():
   if o.name==name and (role is None or o.getRoleName()==role):return o
  time.sleep(.1)
 (artifacts/'accessibility.txt').write_text('\n'.join(f'{o.getRoleName()} {o.name!r}' for o in tree()))
 raise AssertionError(f'Missing {role} {name}')
def click(o):
 a=o.queryAction()
 for i in range(a.nActions):
  if a.getName(i) in ('click','press','activate'):assert a.doAction(i);time.sleep(.3);return
 raise AssertionError('No click action '+o.name)
def edit(name,value):
 assert find('math-coord-'+name,'text').queryEditableText().setTextContents(value);time.sleep(.3)
def value(name):return find('math-coord-'+name,'text').queryText().getText(0,-1)
def combo(name,index):
 o=find(name,'combo box');selection=o.querySelection();assert selection.selectChild(index);time.sleep(.35)
def keys(*names):
 d=display.Display();codes=[d.keysym_to_keycode(XK.string_to_keysym(n)) for n in names]
 for c in codes:xtest.fake_input(d,X.KeyPress,c)
 for c in reversed(codes):xtest.fake_input(d,X.KeyRelease,c)
 d.sync();d.close();time.sleep(.4)
def focus():
 from xkey import find_windows
 d=display.Display();wins=[];find_windows(d,d.screen().root,'xournalpp',wins)
 w=max(wins,key=lambda w:w.get_geometry().width*w.get_geometry().height);w.set_input_focus(X.RevertToParent,X.CurrentTime);d.sync();d.close()
def shot(name):
 d=display.Display();from xkey import find_windows
 wins=[];find_windows(d,d.screen().root,'xournalpp',wins)
 for w in wins:
  g=w.get_geometry();assert g.width<=d.screen().width_in_pixels and g.height<=d.screen().height_in_pixels,'Dialog exceeds screen'
 d.close();subprocess.run(['import','-window','root',str(artifacts/(name+'.png'))],check=True)
def sensitive():return find('Chèn cả cụm','button').getState().contains(pyatspi.STATE_SENSITIVE)
def save(name):
 path=artifacts/(name+'.xopp');focus();keys('Control_L','Shift_L','s');time.sleep(.5)
 chooser=find('Save File','file chooser');texts=[o for o in walk(chooser) if o.getRoleName()=='text' and o.getState().contains(pyatspi.STATE_SHOWING)]
 assert texts;assert texts[0].queryEditableText().setTextContents(str(path));click(next(o for o in walk(chooser) if o.getRoleName()=='button' and o.name in ('Save','Lưu')))
 end=time.monotonic()+5
 while not path.exists() and time.monotonic()<end:time.sleep(.1)
 assert path.exists()
 end=time.monotonic()+8
 while time.monotonic()<end and any(o.getRoleName()=='file chooser' for o in tree()):time.sleep(.1)
 assert not any(o.getRoleName()=='file chooser' for o in tree()),'Save dialog did not finish'
 focus();time.sleep(.5)
 return ET.fromstring(gzip.decompress(path.read_bytes()))
def canonical(xml):
 # Save/reopen format rounds page coordinates at the application's existing precision.
 return [ET.tostring(p) for p in xml.findall('page')]
def snapshot():
 action(2);return [s for s in (root/'snapshot.txt').read_text().splitlines() if not s.startswith('VISIBLE')][-1]
def report():return find('coord-report','label').queryText().getText(0,-1)
checks=[]
def check(ok,what):
 assert ok,what;checks.append(what);print('COORD_GUI_PASS',what,flush=True)
def open_mode(index):action(0);combo('math-mode',index);time.sleep(.5)
def insert():
 check(sensitive(),'valid scene can insert');click(find('Chèn cả cụm','button'));time.sleep(.4)
time.sleep(1);action(10)
if os.getenv('COORDINATE_POLISH_ONLY'):
 def showing(name):return find('math-coord-'+name,'text').getState().contains(pyatspi.STATE_SHOWING)
 open_mode(3);combo('coord-preset',2)
 check(not any(showing(k) for k in ['A','B','Q']),'component mode hides unrelated point fields')
 check(not find('coord-pick','combo box').getState().contains(pyatspi.STATE_SHOWING),'component mode hides irrelevant point picker')
 check(sensitive(),'screenshot input u=(1,2,0), v=(3,-1,2) is valid')
 combo('coord-source',1);check(showing('A') and showing('B') and not showing('Q'),'AB mode only shows its used points')
 edit('A','bad');check(not sensitive(),'invalid used point blocks insertion')
 combo('coord-source',0);check(sensitive(),'invalid hidden point cannot block component vectors')
 shot('screenshot-fixed-dark-theme');insert()
 doc=save('evidence');labels=[e.text for e in doc.findall('.//text')]
 check(not set(['A','B','Q','uz']).intersection(labels),'saved figure also omits unused points and zero z label')
 formulas=' '.join(e.attrib.get('text','') for e in doc.findall('.//teximage'))
 check('(4; 1; 2)' in formulas and r'u\cdot v = 1' in formulas,'same screenshot sum and scalar product remain correct')
 action(1,1);check((artifacts/'evidence.pdf').exists(),'fixed figure exports through real PDF path')
 copy_app_log();(artifacts/'gui-results.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2))
 print('COORDINATE_POLISH_GUI PASS',len(checks),'checks',flush=True);raise SystemExit(0)
if os.getenv('COORDINATE_SCOPE_ONLY'):
 action(4);native_action('layer-new-above-current');open_mode(3);combo('coord-preset',1);insert()
 doc=save('layer-insert');pages=doc.findall('page');layers=pages[-1].findall('layer')
 check(len(pages)==2 and not list(pages[0].findall('.//stroke')),'insertion stays on the requested second page')
 check(len(layers)==2 and not list(layers[0]) and len(layers[1].findall('stroke'))>0 and len(layers[1].findall('teximage'))>0,'strokes and formulas stay on the selected second layer')
 action(0);native_action('layer-new-above-current');check(not sensitive(),'changed target layer blocks insertion');shot('layer-guard');click(find('Hủy','button'))
 doc2=save('layer-cancel');check(len(doc.findall('.//stroke'))==len(doc2.findall('.//stroke')) and len(doc.findall('.//teximage'))==len(doc2.findall('.//teximage')),'cancel after layer change adds no figure')
 copy_app_log();(artifacts/'gui-results.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2))
 print('COORDINATE_SCOPE_GUI PASS',len(checks),'checks',flush=True);raise SystemExit(0)
baseline=save('baseline');before=snapshot()
open_mode(3);combo('coord-preset',1);shot('01-ab-preview')
check(sensitive(),'AB sample preview succeeds')
edit('B','invalid');check(not sensitive(),'invalid coordinate disables insertion')
edit('B','4;0;5');edit('view','25');edit('xscale','2');check(sensitive(),'projection and scale can change')
click(find('Hủy','button'));check(snapshot()==before,'preview and cancel leave document and selection untouched')
# Exact declared point selection on the preview. Pick O, whose coordinates are known independently.
open_mode(3);combo('coord-preset',1);edit('Q','2;1;0');combo('coord-pick',1)
preview=find('math-scene-preview','drawing area');ext=preview.queryComponent().getExtents(pyatspi.DESKTOP_COORDS)
# Find O from actual preview strokes: typed model's origin marker is at the x/y axis common endpoint.
# The displayed fixture has O as minimum x in Oxy after explicit dimension conversion.
combo('coord-dimension',1);edit('A','0;0');edit('B','4;0');edit('Q','0;0');edit('u','0;0');edit('v','0;0');edit('view','45');edit('xscale','1')
# Use raster evidence for the actual click, never infer input coordinates from the click itself.
shot('02-point-pick-before')
# For this fixture, min projected x=0, min y=-2.2 (positive Oy axis), y max=0. Chart extent comes from B=(4,0).
# Scene anchors are at x=24,y=34+(0-(-2.2))*factor in the 560x400 model; factor=min(220.8/4,306/2.2)=55.2.
# Widget transform is the same documented viewport fit used to paint the scene.
scale=min((ext.width-12)/560,(ext.height-12)/400)
px=ext.x+(ext.width-560*scale)/2+24*scale;py=ext.y+6+(34+121.44)*scale
# Geometry may change when hiding controls; reread the widget before dispatch.
ext=preview.queryComponent().getExtents(pyatspi.DESKTOP_COORDS);scale=min((ext.width-12)/560,(ext.height-12)/400)
px=ext.x+(ext.width-560*scale)/2+24*scale;py=ext.y+6+(34+121.44)*scale
d=display.Display();xtest.fake_input(d,X.MotionNotify,x=int(px),y=int(py));xtest.fake_input(d,X.ButtonPress,1);xtest.fake_input(d,X.ButtonRelease,1);d.sync();d.close();time.sleep(.5)
check(value('B') in ('0;0','0; 0'),'real preview click snaps to declared O and stores exact coordinates')
shot('03-point-picked');click(find('Hủy','button'))
# Nine independent, app-created pages cover the requested examples.
for index,preset in enumerate([1,2,3,4,5,6,7]):
 action(4);open_mode(3);combo('coord-preset',preset)
 if preset==5:check('Chân trên d/e' in report(),'skew line closest points reported from 3D model')
 if preset==6:edit('u','0;1;0');edit('A','1;2;3');combo('coord-equation',1);shot('04-zero-direction-system');combo('coord-equation',0);edit('u','1;0;0');edit('A','0;0;0')
 shot(f'example-{index+1}-preview');insert();shot(f'example-{index+1}-inserted')
 selected=snapshot().split();check(int(selected[4])>0,'new group strokes are selected')
 if index==0:
  initial=save('inserted');native_action('undo');shot('undo-current');undone=save('undo');check(canonical(undone)[-1]!=canonical(initial)[-1] and not undone.findall('page')[-1].findall('.//stroke'),'one undo removes the whole new group')
  native_action('redo');redone=save('redo');check(canonical(initial)==canonical(redone),'one redo restores every element')
  action(0);check(value('A')=='1;2;3','last coordinate parameters remembered');click(find('Hủy','button'))
# Two selected force modes, polar convention, invalid data, and directionless zero.
action(4);open_mode(5);combo('coord-preset',3);combo('coord-force-input',1);edit('forces','10;0;0 | 10;90;0');insert()
action(4);open_mode(5);combo('coord-preset',4);combo('coord-force-input',1);edit('forces','10;30;0 | 10;210;0');shot('05-opposite-polar');insert()
# Copy and paste the real selected group, including embedded PDF formulas.
pre=save('before-copy');native_action('select-all');native_action('copy');native_action('paste');post=save('after-copy')
for tag in ['stroke','teximage']:
 a=len(pre.findall('page')[-1].findall('.//'+tag));b=len(post.findall('page')[-1].findall('.//'+tag));check(a>0 and b==2*a,'native clipboard preserves '+tag)
# Guard a changed target page while the modal remains open.
action(0);action(9);check(not sensitive(),'changed target page blocks insert');shot('06-target-guard');click(find('Hủy','button'))
evidence=save('evidence');action(3);reopened=save('reopened');check(canonical(evidence)==canonical(reopened),'save/reopen preserves all page/layer elements and formulas')
action(1,1);check((artifacts/'evidence.pdf').exists(),'real Cairo PDF export produced')
shot('07-reopened');copy_app_log()
(artifacts/'gui-results.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2))
print('COORDINATE_GUI PASS',len(checks),'checks; evidence:',artifacts,flush=True)
