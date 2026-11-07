"""Acceptance actions on the real app, via one snapshot-bound Cua connection.
Observation uses the native read-only Lua inventory, never replacement pen/save code.
"""
import json,os,pathlib,re,subprocess,time

def run(c,target,root,out,log):
    seq=0;checks=[];pid=target['pid']
    # Resolve the private bus's exact application. Probe sorts before VK640ToolCycle.
    bus=None
    for _ in range(40):
        for line in subprocess.check_output(['busctl','--user','--no-legend','list'],text=True).splitlines():
            fields=line.split()
            if len(fields)>1 and fields[1]==str(pid) and fields[0].startswith(':'):
                probe=subprocess.run(['gdbus','call','--session','--dest',fields[0],'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.DescribeAll'],text=True,capture_output=True)
                if probe.returncode==0 and 'plugins.action-0' in probe.stdout:bus=fields[0];break
        if bus:break
        time.sleep(.1)
    assert bus,'private app bus'
    def observe():
        subprocess.run(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-0','[]','{}'],check=True,stdout=subprocess.DEVNULL)
        s=(out/'snapshot.lua').read_text();notes=[]
        for body in re.findall(r'\{([^{}]*)\}',s):
            n={}
            for k,v in re.findall(r'(\w+)=("(?:[^"\\]|\\.)*"|true|false|-?[\d.]+),',body):
                n[k]=json.loads(v) if v[0]=='"' or v in ['true','false'] else float(v)
            if 'id' in n:notes.append(n)
        return {'base':int(re.search(r'base=(\d+)',s)[1]),'notes':notes,'mainY':float(re.search(r'mainY=(-?[\d.]+)',s)[1]),'mainZoom':float(re.search(r'mainZoom=(-?[\d.]+)',s)[1]),'tool':json.loads(re.search(r'tool=("[^"]*")',s)[1]),'columns':int(re.search(r'columns=(\d+)',s)[1]) if 'columns=' in s else None}
    def state(name,which=None):
        nonlocal seq
        seq+=1;which=which or target
        r=c.call('get_window_state',{**which,'screenshot_out_file':str(out/f'{seq:03}-{name}.png')})
        metrics={'step':seq,'state':name,'time':time.monotonic()}
        try:
            metrics.update({k:int(v.split()[0]) for k,v in (line.split(':',1) for line in pathlib.Path(f'/proc/{pid}/status').read_text().splitlines() if ':' in line) if k in ('VmRSS','VmHWM','VmSize')})
            with (out/'metrics.jsonl').open('a') as f:f.write(json.dumps(metrics)+'\n')
        except FileNotFoundError:pass
        (out/f'{seq:03}-{name}.json').write_text(json.dumps(r,ensure_ascii=False));return r
    def check(ok,text):
        print('PASS' if ok else 'FAIL',text,flush=True)
        assert ok,text
        checks.append(text);(out/'gui-results.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2))
    def wait(predicate,timeout=.7):
        until=time.monotonic()+timeout
        while time.monotonic()<until:
            if predicate():return True
            time.sleep(.04)
        return predicate()
    def action(name,args,expected=None,which=None):
        which=which or target;state('before-'+name,which)
        foreground=False
        try:c.call(name,{**which,**args})
        except RuntimeError as e:
            if 'background_unavailable' not in str(e):raise
            foreground=True;state('before-isolated-foreground-'+name,which)
            c.call(name,{**which,**args,'delivery_mode':'foreground'})
        state('after-'+name,which)
        if expected and not wait(expected):
            # An acknowledged synthetic input is not proof. Retry only the confirmed no-op.
            print('INPUT_NOOP',name,'retry exact action on isolated display',flush=True)
            state('before-verified-noop-retry',which)
            c.call(name,{**which,**args,'delivery_mode':'foreground'})
            state('after-verified-noop-retry',which)
            assert wait(expected,1.5),(name,args,observe())
    def key(*keys,expected=None,which=None):action('hotkey',{'keys':list(keys)},expected,which)
    def drag(x,y,xx,yy,expected=None):action('drag',{'from_x':x,'from_y':y,'to_x':xx,'to_y':yy,'duration_ms':350,'steps':24},expected)
    def button(text,index=0,expected=None,which=None):
        which=which or target;s=state('before-button',which)
        noteNames=[n['name'] for n in observe()['notes']]
        exact=text+' · '+noteNames[index] if index<len(noteNames) else text
        elements=[e for e in s['elements'] if e.get('label') in (text,exact) and e.get('role') in ['button','toggle button','menu item','check box','combo box']]
        deadline=time.monotonic()+5
        while not elements and text=='Chèn cả cụm' and time.monotonic()<deadline:
            time.sleep(.15);s=state('wait-native-preview',which)
            elements=[e for e in s['elements'] if e.get('label')==text and e.get('role')=='button']
        index=0
        assert len(elements)>index,(text,[(e.get('label'),e['role']) for e in s['elements'] if e.get('label')])
        c.call('click',{**which,'element_token':elements[index]['element_token']})
        if which!=target and text in ('Save','Chèn cả cụm','Hủy','Xuất','Restore file'):
            assert wait(lambda:all(w['window_id']!=which['window_id'] for w in windows()),5),'save dialog closes'
            state('after-save-dialog-closed')
        else:state('after-button',which)
        if expected:assert wait(expected,1.5),(text,observe())
    def windows():return c.call('list_windows',{'pid':pid}).get('windows',[])
    def modal(title):
        assert wait(lambda:any(w['window_id']!=target['window_id'] and title.lower() in w['title'].lower() for w in windows()),3),windows()
        w=next(w for w in windows() if w['window_id']!=target['window_id'] and title.lower() in w['title'].lower());return {'pid':pid,'window_id':w['window_id']}
    def enter(text,label=None,which=None):
        which=which or target;s=state('before-set-field',which)
        entries=[e for e in s['elements'] if e.get('role') in ['text','entry'] and e.get('frame') and (label is None or e.get('label')==label)]
        assert entries,(label,[(e.get('label'),e['role']) for e in s['elements'] if e.get('label')])
        c.call('set_value',{**which,'element_token':entries[0]['element_token'],'value':text})
        state('after-set-field',which)
    def save(path):
        key('ctrl','shift','s');w=modal('Save')
        shot=state('save-dialog',w)
        field=next(e for e in shot['elements'] if e.get('role')=='text' and e.get('frame'))
        c.call('set_value',{**w,'element_token':field['element_token'],'value':str(path)})
        after=state('filename-replaced',w)
        assert any(e.get('label')==str(path) for e in after['elements']), 'GTK filename readback'
        button('Save',which=w)
        assert wait(lambda:path.exists(),5),path
        state('after-save')
    try:
        c.call('set_agent_cursor_enabled',{'session':'implicit','enabled':False})
        stage=os.environ.get('FLOATING_NOTES_STAGE','full')
        if stage=='shortcuts':
            shortcuts = ('Alt+N', 'Ctrl+Alt+N', 'Alt+K')
            # Cua's text tree includes hidden GTK labels. Read their showing state
            # directly on the same isolated accessibility bus to check visibility.
            import pyatspi
            def hint_labels():
                def visible_labels(node):
                    if node.getRole() == pyatspi.ROLE_LABEL and node.name in shortcuts:
                        flags = node.getState()
                        if flags.contains(pyatspi.STATE_VISIBLE) and flags.contains(pyatspi.STATE_SHOWING):
                            yield node.name
                    for child in node:
                        if child is not None:
                            yield from visible_labels(child)
                app = next(app for app in pyatspi.Registry.getDesktop(0) if app.get_process_id()==pid)
                return list(visible_labels(app))
            def library_visible():
                return any(e.get('label')=='Cất kho nháp' and e.get('frame')
                           for e in state('library-visibility')['elements'])
            state('notebook-shortcuts-off')
            check(not hint_labels(), 'notebook captions start hidden')
            key('ctrl','`',expected=lambda:len(hint_labels())>=6)
            state('notebook-shortcuts-on')
            labels = hint_labels()
            counts = {shortcut:labels.count(shortcut) for shortcut in shortcuts}
            check(all(count>=2 for count in counts.values()), f'Ctrl+grave shows all three captions in both toolbar and dock: {counts}')
            key('alt','k',expected=library_visible)
            enter('Nháp','Tìm tờ nháp')
            key('alt','k',expected=lambda:not library_visible())
            check(not observe()['notes'], 'Alt+K opens and closes the gallery from search without making a note')
            key('ctrl','alt','n',expected=lambda:len(observe()['notes'])==1)
            key('alt','n',expected=lambda:not observe()['notes'][0]['open'])
            key('ctrl','alt','n',expected=lambda:len(observe()['notes'])==2)
            key('alt','n',expected=lambda:not any(n['open'] for n in observe()['notes']))
            key('alt','n',expected=lambda:observe()['notes'][1]['open'])
            check([n['open'] for n in observe()['notes']]==[False,True], 'new sheet becomes the recent group without reopening the previously stashed sheet')
            key('ctrl','`',expected=lambda:not hint_labels())
            state('notebook-shortcuts-restored')
            check(not hint_labels(), 'Ctrl+grave hides the three captions again')
            button('Tạo tờ mới — Ctrl+Alt+N',expected=lambda:len(observe()['notes'])==3)
            check(observe()['notes'][2]['active'], 'visible new-sheet button uses the same native creation flow')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_SHORTCUTS_PASS',len(checks),flush=True);return
        if stage=='layout':
            check(wait(lambda:len(observe()['notes'])==2,5),'native document opens both test notes for layout checks')
            current=observe();fingerprint=[(n['id'],n['inkSignature'],n['open']) for n in current['notes']]
            frame=lambda:next(w for w in windows() if w['window_id']==target['window_id'])
            initial_width=frame()['width']
            key('f11',expected=lambda:frame()['width']>initial_width)
            state('fullscreen-two-papers')
            key('f11',expected=lambda:frame()['width']==initial_width)
            key('alt','`',expected=lambda:observe()['columns']!=current['columns'])
            state('paired-layout-with-notes')
            key('alt','`',expected=lambda:observe()['columns']==current['columns'])
            check([(n['id'],n['inkSignature'],n['open']) for n in observe()['notes']]==fingerprint,'fullscreen and one/two columns preserve both open notes and their exact ink')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_LAYOUT_PASS',len(checks),flush=True);return
        if stage=='visual':
            subprocess.run(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-3','[]','{}'],check=True,stdout=subprocess.DEVNULL)
            check(wait(lambda:len(observe()['notes'])==100,5),'100 native notes loaded for theme inspection')
            state('two-native-papers')
            button('Kho nháp — tìm và mở lại các tờ');time.sleep(.6)
            shot=state('library-theme')
            for direction in ['down','down','up','down','up','up']:
                action('scroll',{'direction':direction,'amount':8,'x':shot['screenshot_width']*.82,'y':shot['screenshot_height']*.45})
            shot=state('library-after-scroll-cycles')
            cards=[e for e in shot['elements'] if e.get('role')=='button' and ' · ' in e.get('label','') and e.get('label','').startswith('Nháp ')]
            check(1<len(cards)<30,'visible cards expose bounded native accessible buttons')
            action('set_window_frame',{'x':15,'y':20,'width':960,'height':700})
            time.sleep(.2);state('library-small-window')
            button('Cất kho nháp');state('papers-small-window')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_VISUAL_PASS',len(checks),flush=True);return
        if stage=='held-stroke':
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            action('click',{'x':20,'y':60},expected=lambda:observe()['tool']=='pen')
            action('mouse_button_down',{'x':400,'y':220})
            try:
                action('mouse_drag',{'x':580,'y':280,'duration_ms':350,'steps':24})
                check(observe()['notes'][0]['strokes']==0,'native in-progress stroke stays uncommitted before hide')
                key('alt','n',expected=lambda:not observe()['notes'][0]['open'])
                check(observe()['notes'][0]['strokes']==1,'hide commits the last valid native stroke while the pointer is held')
            finally:
                action('mouse_button_up',{'x':760,'y':665})
            check(observe()['base']==0,'release after mid-stroke hide does not mark the main page')
            key('alt','n',expected=lambda:observe()['notes'][0]['open'])
            drag(430,340,580,395,lambda:observe()['notes'][0]['strokes']==2)
            check(observe()['base']==0,'next stroke after hide and release is independent')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_HELD_STROKE_PASS',len(checks),flush=True);return
        if stage=='tools':
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            action('click',{'x':20,'y':60},expected=lambda:observe()['tool']=='pen')
            drag(400,220,580,280,lambda:observe()['notes'][0]['strokes']==1)
            ink=observe()['notes'][0]['inkSignature']
            key('tab',expected=lambda:observe()['tool']=='eraser')
            drag(400,220,580,280,lambda:observe()['notes'][0]['inkSignature']!=ink)
            key('ctrl','z',expected=lambda:observe()['notes'][0]['inkSignature']==ink)
            check(observe()['base']==0,'native eraser affects only the active note and undo restores its geometry')
            action('click',{'x':137,'y':60},expected=lambda:observe()['tool']=='highlighter')
            drag(430,430,590,450,lambda:observe()['notes'][0]['strokes']==2)
            check(observe()['base']==0,'native highlighter writes on the note without ink behind it')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_TOOLS_PASS',len(checks),flush=True);return
        if stage=='interaction':
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            action('click',{'x':20,'y':60})
            drag(400,220,580,280,lambda:observe()['notes'][0]['strokes']==1)
            signature=observe()['notes'][0]['inkSignature']
            old=observe()['notes'][0]
            drag(440,125,470,180,lambda:observe()['notes'][0]['y']>old['y'])
            check(observe()['notes'][0]['inkSignature']==signature,'title drag moves only frame, preserving pressure/coordinates/width')
            n=observe()['notes'][0]
            edgeX=267+n['x']+n['width']-20;edgeY=80+n['y']+n['height']-20
            drag(edgeX,edgeY,edgeX+100,edgeY+70,lambda:observe()['notes'][0]['width']>n['width'])
            check(observe()['notes'][0]['inkSignature']==signature,'resize changes only viewport geometry')
            button('Phóng to giấy nháp',0,lambda:observe()['notes'][0]['zoom']>.75)
            check(observe()['notes'][0]['inkSignature']==signature,'note zoom preserves native pressure and coordinates')
            n=observe()['notes'][0]
            insideX=267+n['x']+100;insideY=80+n['y']+130
            drag(insideX,insideY,267+n['x']+n['width']+150,insideY+180,lambda:observe()['notes'][0]['strokes']==2)
            check(observe()['base']==0,'stroke released beyond note edge stays on its source paper')
            drag(insideX,insideY+25,insideX+140,insideY+60,lambda:observe()['notes'][0]['strokes']==3)
            check(observe()['base']==0,'next stroke after outside release is independent and does not stick')
            mainBefore=observe()['mainY'];inkBefore=observe()['notes'][0]['inkSignature']
            key('tab',expected=lambda:observe()['tool']=='eraser')
            key('tab',expected=lambda:observe()['tool']=='hand')
            n=observe()['notes'][0];previousScroll=n['scrollY']
            drag(insideX,insideY+170,insideX,insideY+30,lambda:observe()['notes'][0]['scrollY']>previousScroll)
            check(observe()['mainY']==mainBefore and observe()['notes'][0]['inkSignature']==inkBefore,'real Hand gesture scrolls only note paper and adds no ink')
            key('tab',expected=lambda:observe()['tool']=='pen')
            check(observe()['notes'][0]['open'],'real Pen/Eraser/Hand cycle keeps the floating sheet open')
            # Open the real teaching dialog with the native Lua entry point; all editing uses its widgets.
            subprocess.Popen(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-4','[]','{}'],stdout=subprocess.DEVNULL)
            w=modal('Toán THPT');enter('x^2', 'math-f1',which=w)
            button('Chèn cả cụm',which=w)
            check(observe()['notes'][0]['strokes']>3 and observe()['base']==0,'real graph dialog inserts native vector group into active floating note')
            key('ctrl','z',expected=lambda:observe()['notes'][0]['strokes']==3)
            subprocess.Popen(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-4','[]','{}'],stdout=subprocess.DEVNULL)
            w=modal('Toán THPT');enter('(x^2-1)/(x-1)', 'math-f1',which=w)
            button('Chỉ chèn công thức (PDF vector)',which=w)
            button('Chèn cả cụm',which=w)
            check(observe()['notes'][0]['tex']==1 and observe()['base']==0,'pretty formula dialog inserts native TexImage into floating paper')
            key('ctrl','z',expected=lambda:observe()['notes'][0]['tex']==0)
            subprocess.Popen(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-4','[]','{}'],stdout=subprocess.DEVNULL)
            w=modal('Toán THPT');button('math-mode',which=w);button('Vectơ',which=w)
            button('Chèn cả cụm',which=w)
            check(observe()['notes'][0]['strokes']>3 and observe()['notes'][0]['tex']>0 and observe()['base']==0,'coordinate vector dialog inserts native ink and equations into floating paper')
            key('ctrl','z',expected=lambda:observe()['notes'][0]['strokes']==3 and observe()['notes'][0]['tex']==0)
            # Formula entry focus must not swallow the global notebook shortcuts.
            subprocess.Popen(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-4','[]','{}'],stdout=subprocess.DEVNULL)
            w=modal('Toán THPT');button('math-mode',which=w);button('Đồ thị',which=w);enter('sin(x)', 'math-f1',which=w)
            key('alt','n',expected=lambda:not observe()['notes'][0]['open'],which=w)
            key('alt','n',expected=lambda:observe()['notes'][0]['open'],which=w)
            button('Hủy',which=w)
            check(observe()['notes'][0]['strokes']==3,'formula entry hide/reopen/cancel preserves committed notebook ink')
            save(out/'interaction.xopp')
            state('interaction-final')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_INTERACTION_PASS',len(checks),flush=True);return
        if stage=='ocr':
            subprocess.run(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-5','[]','{}'],check=True,stdout=subprocess.DEVNULL)
            s=state('pdf-source-before')
            import cv2,numpy as np
            im=cv2.imread(s['screenshot_file_path']);mask=np.uint8((im[:,:,2]>220)&(im[:,:,1]<100)&(im[:,:,0]<100))*255
            contours,_=cv2.findContours(mask,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
            rects=[cv2.boundingRect(x) for x in contours];rects=[r for r in rects if r[2]>400 and r[3]>400]
            assert rects,'native PDF page border in the fresh Cua image'
            x,y,width,height=max(rects,key=lambda r:r[2]*r[3]);scale=(width-2)/595
            drag(x+1+55*scale,y+1+84*scale,x+1+330*scale,y+1+130*scale)
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            subprocess.Popen(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-4','[]','{}'],stdout=subprocess.DEVNULL)
            w=modal('Toán THPT')
            key('alt','n',expected=lambda:not observe()['notes'][0]['open'],which=w)
            time.sleep(.8);state('recognition-target-hidden',w)
            key('alt','n',expected=lambda:observe()['notes'][0]['open'],which=w)
            check(observe()['notes'][0]['strokes']==0 and observe()['notes'][0]['tex']==0 and observe()['base']==0,'recognition callback after hidden/changed target adds no late content')
            button('Hủy',which=w)
            subprocess.Popen(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-4','[]','{}'],stdout=subprocess.DEVNULL)
            w=modal('Toán THPT');deadline=time.monotonic()+60
            while True:
                s=state('native-recognition-result',w)
                if any(e.get('label')=='Dùng làm hàm 1' and e.get('role')=='button' for e in s['elements']):break
                assert time.monotonic()<deadline,'real local PDF recognition did not produce an actionable candidate'
                time.sleep(.3)
            button('Dùng làm hàm 1',which=w)
            button('Đã đối chiếu ảnh, công thức và đồ thị',which=w)
            button('Chèn cả cụm',which=w)
            check(observe()['notes'][0]['strokes']>0 and observe()['notes'][0]['tex']>0 and observe()['base']==0,'real PDF source recognition inserts native graph/formula only into the confirmed note')
            key('ctrl','z',expected=lambda:observe()['notes'][0]['strokes']==0 and observe()['notes'][0]['tex']==0)
            check(observe()['base']==0,'one note undo after PDF recognition preserves source document')
            key('ctrl','shift','z',expected=lambda:observe()['notes'][0]['strokes']>0)
            save(out/'ocr-to-note.xopp')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_OCR_PASS',len(checks),flush=True);return
        if stage=='export':
            check(len(observe()['notes'])==101,'native saved notebook loaded for UI export')
            button('Kho nháp — tìm và mở lại các tờ')
            button('Xuất tất cả tờ chưa xóa thành PDF vector')
            w=modal('Xuất nháp PDF vector');enter(str(out/'ui-notebook.pdf'),which=w)
            button('Xuất',which=w)
            check(wait(lambda:(out/'ui-notebook.pdf').exists(),5),'native notebook export chooser writes a real PDF')
            info=subprocess.check_output(['pdfinfo',str(out/'ui-notebook.pdf')],text=True)
            check(int(re.search(r'Pages:\s+(\d+)',info)[1])==202,'UI export includes all 202 segments, including hidden/offscreen paper')
            (out/'pdfinfo.txt').write_text(info)
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_EXPORT_PASS',len(checks),flush=True);return
        if stage=='portable':
            check(len(observe()['notes'])>=100,'fresh profile opens the relocated embedded notebook')
            check(observe()['notes'][72]['strokes']==10 and observe()['notes'][72]['tex']==1,'relocated file preserves per-note pressure ink and formula')
            state('portable-reopened')
            print('NATIVE_PORTABLE_PASS',len(checks),flush=True)
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2));return
        if stage=='recovered':
            check(len(observe()['notes'])==1 and observe()['notes'][0]['strokes']==1,'clean profile reopens committed recovered native note')
            check(not observe()['notes'][0]['open'],'recovery preserves the hidden sheet state')
            state('recovered-native-note')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_RECOVERED_PASS',len(checks),flush=True);return
        if stage=='close':
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            action('click',{'x':20,'y':60})
            drag(400,220,580,280,lambda:observe()['notes'][0]['strokes']==1)
            button('Cất tờ về kho — giữ nguyên bài nháp',0,lambda:not observe()['notes'][0]['open'])
            state('before-normal-quit')
            try:c.call('hotkey',{**target,'keys':['ctrl','q']})
            except RuntimeError as e:
                if 'background_unavailable' not in str(e):raise
                state('before-normal-quit-isolated-foreground')
                c.call('hotkey',{**target,'keys':['ctrl','q'],'delivery_mode':'foreground'})
            check(wait(lambda:not windows(),7),'normal quit closes the isolated unnamed notebook successfully')
            import configparser,gzip,xml.etree.ElementTree as ET,shutil
            ini=configparser.ConfigParser();ini.read(log/'config/xournalpp/session.ini')
            session=pathlib.Path(ini['session']['filepath'])
            check(session.name.startswith('session-') and session.exists(),'normal quit stores notebook in its independent session file')
            xml=ET.fromstring(gzip.open(session,'rb').read());notes=[p for p in xml.findall('page') if p.get('scratch-version')=='1']
            check(len(notes)==1 and len(notes[0].findall('./layer/stroke'))==1,'normal quit retains hidden native note ink')
            shutil.copy2(session,out/'quit-session.xopp')
            (out/'result.json').write_text(json.dumps({'checks':checks,'source':str(session)},ensure_ascii=False,indent=2))
            print('NATIVE_CLOSE_PASS',len(checks),flush=True);return
        if stage=='emergency':
            import gzip,signal,shutil,xml.etree.ElementTree as ET
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            action('click',{'x':20,'y':60})
            drag(400,220,580,280,lambda:observe()['notes'][0]['strokes']==1)
            button('Cất tờ về kho — giữ nguyên bài nháp',0,lambda:not observe()['notes'][0]['open'])
            before=observe()
            assert str(log).encode() in pathlib.Path(f'/proc/{pid}/environ').read_bytes()
            os.kill(pid,signal.SIGABRT)
            candidates=lambda:list((log/'config/xournalpp').glob('emergencysave-*.xopp'))
            check(wait(lambda:len(candidates())==1,7),'native crash handler commits a document-specific emergency file')
            source=candidates()[0];xml=ET.fromstring(gzip.open(source,'rb').read())
            notes=[p for p in xml.findall('page') if p.get('scratch-version')=='1']
            check(len(notes)==1 and notes[0].get('stable-id')==before['notes'][0]['id'] and len(notes[0].findall('./layer/stroke'))==1,'emergency file contains the exact hidden native sheet and committed stroke')
            artifact=out/source.name;shutil.copy2(source,artifact)
            (out/'result.json').write_text(json.dumps({'checks':checks,'source':str(artifact),'snapshot':before},ensure_ascii=False,indent=2))
            print('NATIVE_EMERGENCY_PASS',len(checks),flush=True);return
        if stage=='emergency-restore':
            import gzip,xml.etree.ElementTree as ET
            files=list((log/'config/xournalpp').glob('emergencysave-*.xopp'))
            check(len(files)==2,'two independently crashed documents have separate recovery files')
            most_recent=max(files,key=lambda p:p.stat().st_mtime_ns)
            expected=next(p.get('stable-id') for p in ET.fromstring(gzip.open(most_recent,'rb').read()).findall('page') if p.get('scratch-version')=='1')
            if any(window['window_id']!=target['window_id'] for window in windows()):
                w=modal('');button('Restore file',which=w)
            else:
                # GtkMessageDialog has no title. On X11 the driver omits this transient
                # from list_windows. Capture only the isolated Xvfb display in this case.
                assert os.environ.get('FLOATING_NOTES_ISOLATED')=='1' and os.environ.get('DISPLAY') not in (None,':0',':1')
                c.call('get_desktop_state',{'screenshot_out_file':str(out/'recovery-dialog-desktop.png')})
                coords=os.environ.get('FLOATING_NOTES_RESTORE_POINT')
                assert coords,'recovery button requires a point from the isolated desktop capture'
                x,y=map(int,coords.split(','))
                c.call('click',{'target':{'kind':'desktop','display_id':'primary'},'x':x,'y':y})
                c.call('get_desktop_state',{'screenshot_out_file':str(out/'recovery-dialog-after.png')})
            check(wait(lambda:len(observe()['notes'])==1 and observe()['notes'][0]['id']==expected,5),'startup recovery opens the most recent exact note')
            check(all(p.exists() for p in files),'restore retains both emergency files until an explicit deletion')
            state('emergency-restored')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
            print('NATIVE_EMERGENCY_RESTORE_PASS',len(checks),flush=True);return
        if stage=='recovery':
            check(not observe()['notes'],'recovery test starts as unnamed empty document')
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            action('click',{'x':20,'y':60})
            drag(400,220,580,280,lambda:observe()['notes'][0]['strokes']==1)
            button('Cất tờ về kho — giữ nguyên bài nháp',0,lambda:not observe()['notes'][0]['open'])
            expected=observe();start=time.monotonic()
            candidates=lambda:list((log/'cache').rglob('*.autosave.xopp'))
            check(wait(lambda:bool(candidates()),75),'native one-minute autosave commits hidden unnamed note')
            import gzip,xml.etree.ElementTree as ET,shutil
            recovery=candidates()[0];xml=ET.fromstring(gzip.open(recovery,'rb').read())
            notes=[p for p in xml.findall('page') if p.get('scratch-version')=='1']
            check(len(notes)==1 and len(notes[0].findall('./layer/stroke'))==1,'autosave contains committed native stroke and metadata')
            shutil.copy2(recovery,out/'recovery-before-kill.xopp')
            # Only this exact isolated process is stopped; SIGKILL bypasses session-on-quit.
            assert pathlib.Path(f'/proc/{pid}/environ').read_bytes().find(str(log).encode())>=0
            os.kill(pid,9)
            check(recovery.exists(),'SIGKILL preserves the last good autosave')
            (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':expected,'autosave_wait_seconds':time.monotonic()-start,'source':str(recovery)},ensure_ascii=False,indent=2))
            print('NATIVE_RECOVERY_PASS',len(checks),flush=True);return
        if os.environ.get('FLOATING_NOTES_STAGE')!='extended':
            check(not observe()['notes'],'clean isolated native document')
            key('alt','n',expected=lambda:len(observe()['notes'])==1)
            a=observe()['notes'][0]['id'];check(len(observe()['notes'])==1,'first real Alt+N creates exactly one sheet')
            action('click',{'x':20,'y':60}) # Pen control, grounded in the captured production toolbar.
            drag(400,220,580,280,lambda:observe()['notes'][0]['strokes']==1)
            check(observe()['base']==0,'native pen gesture cannot pass through floating paper')
            key('alt','n',expected=lambda:not observe()['notes'][0]['open'])
            check(observe()['notes'][0]['strokes']==1,'real hide shortcut preserves vector ink')
            key('alt','n',expected=lambda:observe()['notes'][0]['open'])
            check(observe()['notes'][0]['id']==a,'real reopen shortcut restores the same note ID')
            key('ctrl','alt','n',expected=lambda:len(observe()['notes'])==2)
            # Second sheet is placed beside the first in a viewport wide enough for two frames.
            drag(880,235,1050,295,lambda:observe()['notes'][1]['strokes']==1)
            check([n['strokes'] for n in observe()['notes']]==[1,1],'two simultaneous native editors hold different ink')
            key('ctrl','z',expected=lambda:observe()['notes'][1]['strokes']==0)
            check(observe()['notes'][0]['strokes']==1,'undo is scoped to the second note')
            key('ctrl','shift','z',expected=lambda:observe()['notes'][1]['strokes']==1)
            drag(760,665,925,715,lambda:observe()['base']==1)
            key('ctrl','z',expected=lambda:observe()['base']==0)
            check([n['strokes'] for n in observe()['notes']]==[1,1],'main-page undo keeps both note histories')
            action('click',{'x':440,'y':125},lambda:observe()['notes'][0]['active'])
            key('ctrl','z',expected=lambda:observe()['notes'][0]['strokes']==0)
            key('ctrl','shift','z',expected=lambda:observe()['notes'][0]['strokes']==1)
            check(observe()['notes'][1]['strokes']==1,'tap header activates the correct note history')
            button('Thêm 840 pt giấy, không co mực',0,lambda:observe()['notes'][0]['paperHeight']==1680)
            check(observe()['notes'][0]['strokes']==1,'add paper preserves vector coordinates and content')
            action('scroll',{'direction':'down','amount':5,'x':500,'y':350},lambda:observe()['notes'][0]['scrollY']>0)
            firstscroll=observe()['notes'][0]['scrollY']
            drag(410,300,580,350,lambda:observe()['notes'][0]['strokes']==2)
            check(observe()['base']==0,'writing after scroll remains on native note paper')
            button('Cất tờ về kho — giữ nguyên bài nháp',0,lambda:not observe()['notes'][0]['open'])
            check(observe()['notes'][0]['strokes']==2,'stash does not delete long-paper content')
            button('Kho nháp — tìm và mở lại các tờ')
            time.sleep(.4);state('library-content')
            check(len(observe()['notes'])==2,'thumbnail library does not duplicate sheets')
            button('Cất kho nháp')
            save(out/'evidence.xopp')
            check((out/'evidence.xopp').stat().st_size>300,'Ctrl+S/Save As produces native document with hidden ink')
            # Read-only export observation calls the production export action, no replacement renderer.
            subprocess.run(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-1','[]','{}'],check=True,stdout=subprocess.DEVNULL)
            assert wait(lambda:(out/'main.pdf').exists(),5)
            # Reopen through the application, preserving the native serialized data.
            subprocess.run(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-2','[]','{}'],check=True,stdout=subprocess.DEVNULL)
            assert wait(lambda:len(observe()['notes'])==2 and observe()['notes'][0]['strokes']==2,5)
            b=observe();check([n['strokes'] for n in b['notes']]==[2,1] and b['notes'][0]['id']==a,'native save/reopen preserves every note identity and ink count')
            check(abs(b['notes'][0]['scrollY']-firstscroll)<1,'hidden note scroll persists through save/reopen')
            state('reopened')
        # Load the 100-note fixture made by the C++ implementation, not the old Python prototype.
        subprocess.run(['gdbus','call','--session','--dest',bus,'--object-path','/com/github/xournalpp/xournalpp/window/1','--method','org.gtk.Actions.Activate','plugins.action-3','[]','{}'],check=True,stdout=subprocess.DEVNULL)
        check(wait(lambda:len(observe()['notes'])==100,5),'application opens all 100 native fixture notes')
        check(sum(n['strokes'] for n in observe()['notes'])==1000,'100-sheet fixture retains 1000 pressure strokes')
        button('Kho nháp — tìm và mở lại các tờ');time.sleep(.8);state('library-100')
        enter('Nháp 73','Tìm tờ nháp');time.sleep(.3);state('search-73')
        s=state('before-card')
        card=next(e for e in s['elements'] if e.get('role')=='button' and e.get('label','').startswith('Nháp 73 ·'))
        c.call('click',{**target,'element_token':card['element_token']});state('after-card')
        assert wait(lambda:observe()['notes'][72]['open'],2),'native card opens paper'
        check(observe()['notes'][72]['active'],'thumbnail search opens the correct stable note ID')
        state('100-note-opened')
        # Pen-accessible menu workflows use the real native model and undo actions.
        button('Thao tác tờ nháp',72)
        enter('Nháp 73 ưu tiên','Tên tờ nháp')
        button('Đổi tên tờ nháp',expected=lambda:observe()['notes'][72]['name']=='Nháp 73 ưu tiên')
        button('Thao tác tờ nháp',72)
        button('Ghim tờ thường dùng',expected=lambda:observe()['notes'][72]['pinned'])
        check(observe()['notes'][72]['dirty'] and not observe()['notes'][0]['dirty'],'rename/pin dirty state is per note')
        key('ctrl','a',expected=lambda:observe()['notes'][72].get('selected',False))
        button('Thao tác tờ nháp',72)
        button('Copy vector vùng đang chọn sang trang chính',expected=lambda:observe()['base']==10)
        check(observe()['notes'][72]['strokes']==10,'copy to main preserves source pressure strokes and assets')
        key('ctrl','z',expected=lambda:observe()['base']==0)
        check(observe()['notes'][72]['strokes']==10,'one destination undo removes the whole vector group')
        key('ctrl','shift','z',expected=lambda:observe()['base']==10)
        # Bring the note back from its visible title; no pen mark for activation.
        action('click',{'x':430,'y':101},lambda:observe()['notes'][72]['active'])
        button('Thao tác tờ nháp',72)
        button('Nhân bản nội dung vector sang tờ độc lập',expected=lambda:len(observe()['notes'])==101)
        clone=observe()['notes'][100]
        check(clone['id']!=observe()['notes'][72]['id'] and clone['strokes']==10 and clone['tex']==1,'native duplicate has independent ID and full vector assets')
        action('click',{'x':20,'y':60})
        drag(410,245,570,300,lambda:observe()['notes'][100]['strokes']==11)
        check(observe()['notes'][72]['strokes']==10,'editing cloned sheet leaves original unchanged')
        button('Thao tác tờ nháp',100)
        button('Xóa toàn bộ nội dung tờ này — Ctrl+Z khôi phục',expected=lambda:observe()['notes'][100]['strokes']==0)
        key('ctrl','z',expected=lambda:observe()['notes'][100]['strokes']==11)
        check(observe()['notes'][100]['tex']==1 and observe()['notes'][100]['texts']==1,'clear-content undo restores TexImage/Text and all ink')
        button('Thao tác tờ nháp',100)
        button('Xóa riêng tờ này — Ctrl+Z khôi phục',expected=lambda:observe()['notes'][100]['deleted'])
        key('ctrl','z',expected=lambda:not observe()['notes'][100]['deleted'])
        check(observe()['notes'][100]['strokes']==11,'delete sheet and undo retain its entire native content')
        state('edited-notebook')
        save(out/'evidence-100.xopp')
        check(not any(n['dirty'] for n in observe()['notes']),'successful Save As resets per-note dirty indicators')

        button('Thao tác tờ nháp',72)
        key('escape')
        key('ctrl','a',expected=lambda:observe()['notes'][72]['selected'])
        key('ctrl','c')
        key('ctrl','alt','n',expected=lambda:len(observe()['notes'])==102)
        key('ctrl','v',expected=lambda:observe()['notes'][101]['strokes']==10)
        check(observe()['notes'][101]['tex']==1 and observe()['notes'][101]['texts']==1,'real internal clipboard preserves native formula/text/ink in a different note')
        check(observe()['notes'][72]['strokes']==10 and observe()['base']==10,'clipboard copy preserves source note and main page')
        key('ctrl','x',expected=lambda:observe()['notes'][101]['strokes']==0)
        key('ctrl','z',expected=lambda:observe()['notes'][101]['strokes']==10)
        check(observe()['notes'][101]['tex']==1,'cut undo restores the whole native mixed selection')
        key('escape')
        key('ctrl','u',expected=lambda:observe()['notes'][101]['strokes']==0)
        check(observe()['notes'][101]['tex']==1 and observe()['notes'][72]['strokes']==10,'real Ctrl+U clears only active note ink and preserves native formula/source')
        key('ctrl','z',expected=lambda:observe()['notes'][101]['strokes']==10)
        before=[n['strokes'] for n in observe()['notes']]
        key('ctrl','shift','m')
        check([n['strokes'] for n in observe()['notes']]==before,'real legacy notebook cleanup excludes every floating sheet')
        save(out/'clipboard-native.xopp')
        (out/'result.json').write_text(json.dumps({'checks':checks,'snapshot':observe()},ensure_ascii=False,indent=2))
        print('NATIVE_GUI_PASS',len(checks),flush=True)
    except Exception:
        state('failure');(out/'failure-state.json').write_text(json.dumps(observe(),ensure_ascii=False,indent=2));raise
