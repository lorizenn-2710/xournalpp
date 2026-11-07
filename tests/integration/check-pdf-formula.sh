#!/bin/bash
# Full installed application, own display/config/bus. Never close the user's app.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
D=$(mktemp -d /tmp/xpp-pdf-formula.XXXXXX)
export PDF_FORMULA_DIR=$D PDF_FORMULA_SCRIPTS=$HERE PDF_FORMULA_FIXTURES=$ROOT/output/pdf/pdf-formula-fixtures
export APP=${APP:-$HOME/.local/bin/xournalpp}
export PDF_FORMULA_SCREEN=${PDF_FORMULA_SCREEN:-1600x1100}
export PDF_FORMULA_ARTIFACTS=${PDF_FORMULA_ARTIFACTS:-$D}
mkdir -p "$PDF_FORMULA_ARTIFACTS"
mkdir -p "$D/config/xournalpp/plugins/PdfFormulaProbe" "$D/data" "$D/run"
chmod 700 "$D/run"
cp "$HOME/.config/xournalpp/"{settings.xml,toolbar.ini} "$D/config/xournalpp/"
cp "${PDF_FORMULA_PROBE:-$HERE/pdf-formula-probe.lua}" "$D/config/xournalpp/plugins/PdfFormulaProbe/main.lua"
cat > "$D/config/xournalpp/plugins/PdfFormulaProbe/plugin.ini" <<'INI'
[about]
author=PDF formula tests
description=Installed local OCR and math acceptance
version=1.0
[default]
enabled=true
[plugin]
mainfile=main.lua
INI
python3 - "$D" <<'PY'
import pathlib,re,sys,os
r=pathlib.Path(sys.argv[1]);p=r/'config/xournalpp/settings.xml';s=p.read_text()
w,h=map(int,os.environ['PDF_FORMULA_SCREEN'].split('x'))
for k,v in [('pluginEnabled',str(r/'config/xournalpp/plugins/PdfFormulaProbe')),('mainWndWidth',str(min(1500,w-40))),('mainWndHeight',str(min(1000,h-40))),('maximized','false')]:
 s,n=re.subn(r'name="'+k+r'" value="[^"]*"',f'name="{k}" value="{v}"',s);assert n==1
p.write_text(s)
PY
export XDG_CONFIG_HOME="$D/config" XDG_DATA_HOME="$D/data" XDG_RUNTIME_DIR="$D/run"
export GDK_BACKEND=x11 GTK_MODULES=gail:atk-bridge
unset WAYLAND_DISPLAY NO_AT_BRIDGE
xvfb-run -a -s "-screen 0 ${PDF_FORMULA_SCREEN}x24" dbus-run-session -- bash -euo pipefail -s <<'INNER'
"$APP" >"$PDF_FORMULA_DIR/app.log" 2>&1 &
PID=$!
export PDF_FORMULA_PID=$PID
trap 'kill "$PID" 2>/dev/null || true' EXIT
BUS=''
for _ in $(seq 1 100); do
 for name in $(busctl --user list 2>/dev/null | awk '$3=="xournalpp"{print $1}'); do
  if gdbus call --session --dest "$name" --object-path /com/github/xournalpp/xournalpp/window/1 --method org.gtk.Actions.DescribeAll 2>/dev/null | grep plugins.action-8 >/dev/null; then BUS=$name;break;fi
 done
 [ -z "$BUS" ] || break
 sleep .2
done
[ -n "$BUS" ] || { cat "$PDF_FORMULA_DIR/app.log";exit 1; }
export PDF_FORMULA_BUS=$BUS
# Resolve the application's PID as exported on this private bus (also supports
# a diagnostic debugger wrapper without touching the user's live process).
PDF_FORMULA_PID=$(busctl --user --no-legend list | awk -v bus="$BUS" '$1==bus{print $2}')
export PDF_FORMULA_PID
/usr/bin/python3 "${PDF_FORMULA_GUI:-$PDF_FORMULA_SCRIPTS/pdf-formula-gui.py}"
INNER
echo "PDF formula evidence: $D"
