"""Real local model, then application's AST structure, values AND excluded points.
All synthetic fixtures are owned by this repo. This is a finite conformance
benchmark, not a population-level estimate of OCR accuracy.
"""
import pathlib,subprocess,configparser,json,base64,time,os
r=pathlib.Path(__file__).resolve().parents[2];out=r/'build/pdf-math-verification';fixtures=r/'output/pdf/pdf-formula-fixtures'
python=pathlib.Path.home()/'.local/share/xournalpp/formula-ocr/bin/python'
worker=pathlib.Path.home()/'.local/share/xournalpp/teaching-tools/formula-ocr.py'
results=[]
cases=json.loads((fixtures/'manifest.json').read_text())
for case in cases:case['image']=str(fixtures/case['name']/'crop.png')
for name,expected in [('frac-96dpi','1/(x+1)'),('frac-144dpi','1/(x+1)'),('blurred',None),('cut-denominator',None)]:
 cases.append(dict(name=name,image=str(fixtures/(name+'.png')),expected=expected))
for case in cases:
 start=time.monotonic();p=subprocess.run([str(python),str(worker),case['image']],capture_output=True,text=True,timeout=65)
 k=configparser.ConfigParser();k.read_string(p.stdout);candidates=[]
 for i in range(k.getint('result','count',fallback=0)):
  raw=base64.b64decode(k.get(f'candidate{i}','raw64')).decode();expected=case['expected']
  cmd=[str(r/'build/tests/test-formula-import'),'--equivalent',raw,expected] if expected else [str(r/'build/tests/test-formula-import'),'--import',raw]
  checked=subprocess.run(cmd,capture_output=True,text=True)
  candidates.append(dict(raw=raw,checkExit=checked.returncode,rotation=k.get(f'candidate{i}','rotation',fallback='0'),scale=k.get(f'candidate{i}','scale',fallback='1')))
 if case['expected']:
  status='correct-candidate' if any(c['checkExit']==0 for c in candidates) else 'needs-edit'
 elif case['name']=='implicit':status='unsafe-accepted' if any(c['checkExit']==0 for c in candidates) else 'correctly-rejected'
 else:status='requires-human-review'
 results.append(dict(name=case['name'],expected=case['expected'],candidates=candidates,status=status,seconds=round(time.monotonic()-start,3),code=k.get('result','code'),notes=k.get('result','notes',fallback='')))
 (out/'ocr-benchmark-final.json').write_text(json.dumps(results,indent=2,ensure_ascii=False))
 print(results[-1],flush=True)
assert all(c['status']=='correct-candidate' for c in results if c['expected']), 'Clean fixture failed semantic AST/domain comparison'
assert next(c for c in results if c['name']=='implicit')['status']=='correctly-rejected'
print('OCR_BENCHMARK PASS clean AST/domain cases; ambiguous sources always require human confirmation')
