#!/bin/bash
# Real Ctrl+` keyboard events against the production plugin in an isolated X11 app.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
VK640_DIR="$HERE/../../plugins/VK640ToolCycle"
D=$(mktemp -d /tmp/xpp-shortcut-hints.XXXXXX)
export HINT_TEST_DIR=$D HINT_TEST_SCRIPTS=$HERE
export APP=${APP:-/home/phuc/.local/bin/xournalpp}
# The probe is appended after the production registrations. Derive its index
# from the actual plugin, so adding native notebook actions cannot retarget it.
HINT_SNAPSHOT_INDEX=$(python3 - "$VK640_DIR/main.lua" <<'PYINDEX'
import re,sys
print(len(re.findall(r"app\.registerUi\s*\{",open(sys.argv[1],encoding="utf-8").read())))
PYINDEX
)
export HINT_SNAPSHOT_ACTION="plugins.action-$HINT_SNAPSHOT_INDEX"
mkdir -p "$D/config/xournalpp/plugins/VK640ToolCycle" "$D/data" "$D/run"
chmod 700 "$D/run"
cp "$HOME/.config/xournalpp/settings.xml" "$HOME/.config/xournalpp/toolbar.ini" "$D/config/xournalpp/"
cp "$VK640_DIR/"{main.lua,plugin.ini} "$D/config/xournalpp/plugins/VK640ToolCycle/"
cat >> "$D/config/xournalpp/plugins/VK640ToolCycle/main.lua" <<'LUA'
local productionInit = initUi
function hintSnapshot()
    print("HINT_STATE columns=" .. app.getActionState("set-columns-or-rows") ..
          " pages=" .. #app.getDocumentStructure().pages)
    io.stdout:flush()
end
function initUi()
    productionInit()
    app.registerUi{menu="Shortcut hints snapshot", callback="hintSnapshot"}
end
LUA
python3 - "$D" <<'PY'
import pathlib,re,sys
root=pathlib.Path(sys.argv[1]); p=root/'config/xournalpp/settings.xml'
s=p.read_text(); value=str(root/'config/xournalpp/plugins/VK640ToolCycle')
s,n=re.subn(r'name="pluginEnabled" value="[^"]*"',f'name="pluginEnabled" value="{value}"',s)
assert n == 1
p.write_text(s)
PY
export XDG_CONFIG_HOME="$D/config" XDG_DATA_HOME="$D/data" XDG_RUNTIME_DIR="$D/run"
export GDK_BACKEND=x11 NO_AT_BRIDGE=1
unset WAYLAND_DISPLAY
xvfb-run -a -s '-screen 0 1920x1200x24' dbus-run-session -- bash -euo pipefail -s <<'INNER'
"$APP" >"$HINT_TEST_DIR/app.log" 2>&1 &
APP_PID=$!
trap 'kill "$APP_PID" 2>/dev/null || true' EXIT
OBJECT=/com/github/xournalpp/xournalpp/window/1
BUS=''
for _ in $(seq 1 100); do
    for name in $(busctl --user list 2>/dev/null | awk '$3=="xournalpp"{print $1}'); do
        if gdbus call --session --dest "$name" --object-path "$OBJECT" --method org.gtk.Actions.DescribeAll 2>/dev/null | grep -q "$HINT_SNAPSHOT_ACTION"; then
            BUS=$name; break
        fi
    done
    [ -z "$BUS" ] || break
    sleep 0.2
done
[ -n "$BUS" ] || { cat "$HINT_TEST_DIR/app.log"; exit 1; }
export HINT_TEST_BUS=$BUS
/usr/bin/python3 - <<'PY'
import os,sys,time,subprocess
from Xlib import X,XK,display
from Xlib.ext import xtest
sys.path.insert(0,os.environ['HINT_TEST_SCRIPTS'])
from xkey import find_windows
d=display.Display(); wins=[]
find_windows(d,d.screen().root,'xournalpp',wins)
assert wins, 'no visible app window'
w=max(wins,key=lambda w:w.get_geometry().width*w.get_geometry().height)
w.set_input_focus(X.RevertToParent,X.CurrentTime);d.sync();time.sleep(.8)
root=os.environ['HINT_TEST_DIR']
def snapshot():
    subprocess.run(['gdbus','call','--session','--dest',os.environ['HINT_TEST_BUS'],
                    '--object-path','/com/github/xournalpp/xournalpp/window/1',
                    '--method','org.gtk.Actions.Activate',os.environ['HINT_SNAPSHOT_ACTION'],'[]','{}'],check=True)
    time.sleep(.2)
def capture(name):
    subprocess.run(['import','-window','root',f'{root}/{name}.png'],check=True)
def press(modifier):
    codes=[d.keysym_to_keycode(XK.string_to_keysym(k)) for k in (modifier,'grave')]
    for c in codes: xtest.fake_input(d,X.KeyPress,c)
    d.sync();time.sleep(.05)
    for c in reversed(codes): xtest.fake_input(d,X.KeyRelease,c)
    d.sync();time.sleep(.7)
snapshot();capture('01-hidden')
press('Control_L');snapshot();capture('02-visible')
press('Control_L');snapshot();capture('03-hidden-again')
press('Alt_L');snapshot()
press('Alt_L');snapshot()
# The new math shortcut must route to the native dialog and Escape must cancel it.
for key in ('Alt_L','g'):xtest.fake_input(d,X.KeyPress,d.keysym_to_keycode(XK.string_to_keysym(key)))
for key in ('g','Alt_L'):xtest.fake_input(d,X.KeyRelease,d.keysym_to_keycode(XK.string_to_keysym(key)))
d.sync();time.sleep(.8)
math_windows=[]
find_windows(d,d.screen().root,'xournalpp',math_windows)
capture('04-math-dialog')
def title(w):
    prop=w.get_full_property(d.intern_atom('_NET_WM_NAME'),d.intern_atom('UTF8_STRING'))
    return prop.value.decode('utf-8') if prop else str(w.get_wm_name())
print('Math dialog window titles:',[title(w) for w in math_windows])
math_windows=[w for w in math_windows if 'THPT' in title(w)]
assert math_windows,'Alt+G did not open the real teaching dialog'
capture('04-math-dialog')
esc=d.keysym_to_keycode(XK.string_to_keysym('Escape'))
xtest.fake_input(d,X.KeyPress,esc);xtest.fake_input(d,X.KeyRelease,esc);d.sync();time.sleep(.5)
print('PASS real Alt+G opens math dialog, Escape cancels')
PY
INNER
/usr/bin/python3 - "$D" <<'PY'
import pathlib,re,sys
import numpy as np
from PIL import Image
root=pathlib.Path(sys.argv[1]); log=(root/'app.log').read_text()
assert log.count('Plugin "VK640ToolCycle" UI initialized') == 1, 'VK640 plugin loaded more than once'
states=re.findall(r'HINT_STATE columns=(\d+) pages=(\d+)',log)
assert len(states)==5, (states,log)
assert states[0]==states[1]==states[2], 'Ctrl+grave changed layout/pages'
assert states[3][0]!=states[2][0] and states[3][1]==states[2][1], 'Alt+grave did not switch columns independently'
assert states[4]==states[0], 'Alt+grave did not restore columns'
images=[np.asarray(Image.open(root/f'{n}.png').convert('RGB')) for n in ('01-hidden','02-visible','03-hidden-again')]
# Left toolbar area: visible captions must change it, then return exactly to the compact layout.
a,b,c=[i[80:850,:200] for i in images]
changed=np.count_nonzero(np.any(a!=b,axis=2)); residual=np.count_nonzero(np.any(a!=c,axis=2))
assert changed>500, f'no visible captions after Ctrl+grave ({changed} changed pixels)'
assert residual<100, f'compact toolbar was not restored ({residual} residual pixels)'
print('PASS real Ctrl+grave toggles visible captions and restores compact toolbar')
print('PASS Ctrl+grave preserves page count and column layout')
print('PASS real Alt+grave switches/restores column layout independently')
print(f'PASS left toolbar changed={changed} pixels, restored residual={residual} pixels')
print(f'Test evidence: {root}')
PY
