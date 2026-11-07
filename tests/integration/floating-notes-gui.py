#!/usr/bin/python3
import os,sys,time,json,pathlib,subprocess,select,traceback
ROOT=pathlib.Path(__file__).resolve().parents[2]
LOG=pathlib.Path(os.environ['FLOATING_NOTES_DIR'])
OUT=pathlib.Path(os.environ['FLOATING_NOTES_OUT'])
assert os.environ.get('FLOATING_NOTES_ISOLATED')=='1'
assert os.environ.get('DISPLAY') not in (None,':0',':1')
class Cua:
    def __init__(self):
        self.stderr = (LOG / "cua-isolated.log").open("w")
        self.process = subprocess.Popen(["cua-driver", "mcp", "--direct"], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=self.stderr, text=True, bufsize=1, start_new_session=True)
        self.seq = 0
        self.transcript = (LOG / "cua-transcript.jsonl").open("w")
        self.rpc("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                                "clientInfo": {"name": "floating-notes-research", "version": "1"}})
        self.process.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
        self.process.stdin.flush()

    def rpc(self, method, params):
        self.seq += 1
        seq = self.seq
        request = {"jsonrpc": "2.0", "id": seq, "method": method, "params": params}
        self.transcript.write(json.dumps({"request": request}, ensure_ascii=False) + "\n")
        self.transcript.flush()
        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()
        deadline = time.monotonic() + 40
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.process.stdout], [], [], max(0, deadline - time.monotonic()))
            if not ready:
                break
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError("Cua ended")
            response = json.loads(line)
            self.transcript.write(json.dumps({"response": response}, ensure_ascii=False) + "\n")
            self.transcript.flush()
            if response.get("id") == seq:
                if "error" in response:
                    raise RuntimeError(response["error"])
                return response.get("result", {})
        raise TimeoutError(method)

    def call(self, name, args):
        result = self.rpc("tools/call", {"name": name, "arguments": args})
        if result.get("isError"):
            raise RuntimeError(result)
        if "structuredContent" in result:
            return result["structuredContent"]
        for block in result.get("content", []):
            if block.get("type") == "text":
                try:
                    return json.loads(block["text"])
                except json.JSONDecodeError:
                    pass
        return result


wmroot=ROOT/'build/floating-notes-research/wm-root'
wm_env=dict(os.environ,LD_LIBRARY_PATH=str(wmroot/'usr/lib/x86_64-linux-gnu'),XDG_DATA_DIRS=str(wmroot/'usr/share')+':/usr/local/share:/usr/share')
wm=subprocess.Popen([str(wmroot/'usr/bin/openbox'),'--sm-disable','--config-file',str(wmroot/'etc/xdg/openbox/rc.xml')],env=wm_env,stdout=(LOG/'wm.log').open('w'),stderr=subprocess.STDOUT,start_new_session=True)
time.sleep(.5)
c=Cua()
pid=None;identity=None
try:
 r=c.call('launch_app',{'name':str(LOG/'launch.sh'),'additional_arguments':[os.environ['FLOATING_NOTES_INPUT']] if os.environ.get('FLOATING_NOTES_INPUT') else []})
 pid=r['pid'];windows=r.get('windows',[])
 if os.environ.get('FLOATING_NOTES_DEBUG')=='1':
  for _ in range(100):
   children=pathlib.Path(f'/proc/{pid}/task/{pid}/children').read_text().split()
   apps=[int(p) for p in children if pathlib.Path(f'/proc/{p}/exe').resolve().name=='xournalpp']
   if apps:pid=apps[0];windows=[];break
   time.sleep(.1)
  assert apps,'gdb child app'

 for _ in range(40):
  if windows:break
  time.sleep(.2);windows=c.call('list_windows',{'pid':pid}).get('windows',[])
 assert windows,r
 identity=pathlib.Path(f'/proc/{pid}/stat').read_text().split(') ',1)[1].split()[19]
 assert str(LOG).encode() in pathlib.Path(f'/proc/{pid}/environ').read_bytes(),'isolated app identity'
 wid=max(windows,key=lambda w:w['width']*w['height'])['window_id'];target={'pid':pid,'window_id':wid}
 (OUT/'target.json').write_text(json.dumps(target))
 print('TARGET',target,'CONTROL',str(LOG),flush=True)
 import importlib.util
 spec=importlib.util.spec_from_file_location('native_scenario',ROOT/'tests/integration/floating-notes-scenario.py')
 scenario=importlib.util.module_from_spec(spec);spec.loader.exec_module(scenario)
 scenario.run(c,target,ROOT,OUT,LOG)

finally:
 try:
  if pid is None or identity is None:raise ProcessLookupError
  stat=pathlib.Path(f'/proc/{pid}/stat').read_text().split(') ',1)[1].split()
  if stat[0]=='Z' or stat[19]!=identity:raise ProcessLookupError
  os.kill(pid,15)
  for _ in range(30):
   try:os.kill(pid,0)
   except ProcessLookupError:break
   time.sleep(.1)
  else:os.kill(pid,9)
 except (ProcessLookupError,FileNotFoundError):pass
 c.process.terminate();wm.terminate()
 import shutil
 for name in ["app.log","cua-isolated.log","cua-transcript.jsonl","wm.log"]:
  if (LOG/name).exists():shutil.copy2(LOG/name,OUT/name)
