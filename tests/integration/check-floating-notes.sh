#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
D=$(mktemp -d /tmp/xpp-floating-native.XXXXXX)
export FLOATING_NOTES_ISOLATED=1 FLOATING_NOTES_DIR="$D"
export FLOATING_NOTES_OUT=${FLOATING_NOTES_OUT:-$ROOT/output/floating-notes-native/${D##*/}}
mkdir -p "$FLOATING_NOTES_OUT" "$D/config/xournalpp/plugins/Probe" "$D/data" "$D/run" "$D/cache"
chmod 700 "$D/run"
cp "$HOME/.config/xournalpp/"{settings.xml,toolbar.ini} "$D/config/xournalpp/"
cp -a "$ROOT/plugins/VK640ToolCycle" "$D/config/xournalpp/plugins/"
cp "$ROOT/tests/integration/floating-notes-probe.lua" "$D/config/xournalpp/plugins/Probe/main.lua"
cat > "$D/config/xournalpp/plugins/Probe/plugin.ini" <<'INI'
[about]
author=Native notebook tests
description=Native notebook observation
version=1
[default]
enabled=true
[plugin]
mainfile=main.lua
INI
python3 - "$D" <<'PY'
from pathlib import Path
import sys,re,os,shutil,configparser
r=Path(sys.argv[1]);p=r/'config/xournalpp/settings.xml';s=p.read_text()
for k,v in [('pluginEnabled',str(r/'config/xournalpp/plugins/VK640ToolCycle')+','+str(r/'config/xournalpp/plugins/Probe')),('mainWndWidth','1300'),('mainWndHeight','920'),('maximized','false'),('themeVariant',os.getenv('FLOATING_NOTES_THEME','forceLight')),('autosaveTimeout','1'),('autosaveEnabled','true')]:
 s,n=re.subn(r'name="'+k+r'" value="[^"]*"',f'name="{k}" value="{v}"',s);assert n==1
p.write_text(s)
if source:=os.getenv('FLOATING_NOTES_SESSION_INPUT'):
 source=Path(source); copied=p.parent/source.name
 shutil.copy2(source,copied)
 session=configparser.ConfigParser()
 session['session']={'active':'true','is_scratchpad':'true','filepath':str(copied),'page':'0','zoom':'1'}
 with (p.parent/'session.ini').open('w') as stream:session.write(stream)
for source in filter(None,os.getenv('FLOATING_NOTES_EMERGENCY_INPUT','').split(os.pathsep)):
 source=Path(source);shutil.copy2(source,p.parent/source.name)
PY
cat > "$D/launch.sh" <<'SH'
#!/bin/bash
ulimit -c 0
if [ "${FLOATING_NOTES_DEBUG:-0}" = 1 ]; then
 exec gdb --batch -ex 'handle SIGSEGV stop print nopass' -ex run -ex 'thread apply all bt' --args /home/phuc/.local/bin/xournalpp > "$FLOATING_NOTES_DIR/app.log" 2>&1
fi
exec /home/phuc/.local/bin/xournalpp "$@" > "$FLOATING_NOTES_DIR/app.log" 2>&1
SH
chmod +x "$D/launch.sh"
export XDG_CONFIG_HOME="$D/config" XDG_DATA_HOME="$D/data" XDG_RUNTIME_DIR="$D/run" XDG_CACHE_HOME="$D/cache"
export GDK_BACKEND=x11 GTK_MODULES=gail:atk-bridge
unset WAYLAND_DISPLAY NO_AT_BRIDGE
export GDK_SCALE=${FLOATING_NOTES_SCALE:-1}
# Build the real native 100-sheet fixture for this isolated acceptance run.
"$ROOT/build/tests/test-floating-notebook" "$FLOATING_NOTES_OUT" > "$FLOATING_NOTES_OUT/oracle.log" 2>&1
exec xvfb-run -a -s "-screen 0 ${FLOATING_NOTES_SCREEN:-1440x1000x24}" dbus-run-session -- /usr/bin/python3 "$ROOT/tests/integration/floating-notes-gui.py"
