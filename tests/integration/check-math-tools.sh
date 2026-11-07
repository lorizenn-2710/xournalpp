#!/bin/bash
# Real GTK + native math generator in an isolated display, configuration and bus.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
APP=${APP:-/home/phuc/.local/bin/xournalpp}
export VK640_PLUGIN=${VK640_PLUGIN:-$HERE/../../plugins/VK640ToolCycle/main.lua}
D=$(mktemp -d /tmp/xpp-math-tools.XXXXXX)
export MATH_TEST_DIR=$D MATH_TEST_SCRIPTS=$HERE APP
mkdir -p "$D/config/xournalpp/plugins/MathProbe" "$D/data" "$D/run"
chmod 700 "$D/run"
cp "$HOME/.config/xournalpp/"{settings.xml,toolbar.ini} "$D/config/xournalpp/"
cat > "$D/config/xournalpp/plugins/MathProbe/plugin.ini" <<'INI'
[about]
author=Stage 1 tests
description=Native teaching math acceptance
version=1.0
[default]
enabled=true
[plugin]
mainfile=main.lua
INI
cp "$HERE/math-probe.lua" "$D/config/xournalpp/plugins/MathProbe/main.lua"
python3 - "$D" <<'PY'
import pathlib,re,sys
root=pathlib.Path(sys.argv[1]);p=root/'config/xournalpp/settings.xml'
s=p.read_text();s,n=re.subn(r'name="pluginEnabled" value="[^"]*"',f'name="pluginEnabled" value="{root}/config/xournalpp/plugins/MathProbe"',s);assert n==1
# The copied desktop profile can be wider than Xvfb. Set only this isolated
# instance to a real on-screen size so cropped screenshots cannot pass silently.
for key,value in [('mainWndWidth','1500'),('mainWndHeight','1000'),('maximized','false')]:
 s,n=re.subn(r'name="'+key+r'" value="[^"]*"',f'name="{key}" value="{value}"',s);assert n==1
p.write_text(s)
PY
export XDG_CONFIG_HOME="$D/config" XDG_DATA_HOME="$D/data" XDG_RUNTIME_DIR="$D/run"
export GDK_BACKEND=x11 GTK_MODULES=gail:atk-bridge
unset WAYLAND_DISPLAY NO_AT_BRIDGE
xvfb-run -a -s '-screen 0 1600x1100x24' dbus-run-session -- bash -euo pipefail -s <<'INNER'
"$APP" >"$MATH_TEST_DIR/app.log" 2>&1 &
APP_PID=$!
trap 'kill "$APP_PID" 2>/dev/null || true' EXIT
OBJECT=/com/github/xournalpp/xournalpp/window/1
BUS=''
for _ in $(seq 1 100); do
 for name in $(busctl --user list 2>/dev/null | awk '$3=="xournalpp"{print $1}'); do
  if gdbus call --session --dest "$name" --object-path "$OBJECT" --method org.gtk.Actions.DescribeAll 2>/dev/null | grep -q plugins.action-5; then BUS=$name;break;fi
 done
 [ -z "$BUS" ] || break
 sleep .2
done
[ -n "$BUS" ] || { cat "$MATH_TEST_DIR/app.log";exit 1; }
export MATH_TEST_BUS=$BUS
/usr/bin/python3 "$MATH_TEST_SCRIPTS/math-gui-test.py"
INNER
cat "$D/app.log"
echo "Math evidence: $D"
if [ -z "${MATH_STRESS_ONLY:-}" ]; then grep -q 'MATH_RESULT PASS' "$D/app.log"; fi
! grep -q MATH_FAIL "$D/app.log"
