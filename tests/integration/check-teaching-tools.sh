#!/bin/bash
# Exercise the real VK640 Lua callbacks and selection/undo APIs in an isolated app.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
APP=${APP:-/home/phuc/.local/bin/xournalpp}
export VK640_PLUGIN=${VK640_PLUGIN:-$HERE/../../plugins/VK640ToolCycle/main.lua}
D=$(mktemp -d /tmp/xpp-teaching-tools.XXXXXX)
export TEACHING_TEST_DIR=$D
mkdir -p "$D/config/xournalpp/plugins/TeachingProbe" "$D/data" "$D/run"
chmod 700 "$D/run"
cp "$HOME/.config/xournalpp/settings.xml" "$D/config/xournalpp/"
[ ! -f "$HOME/.config/xournalpp/toolbar.ini" ] || cp "$HOME/.config/xournalpp/toolbar.ini" "$D/config/xournalpp/"
cat > "$D/config/xournalpp/plugins/TeachingProbe/plugin.ini" <<'INI'
[about]
author=VK640 regression tests
description=Isolated teaching tools probe
version=1.0
[default]
enabled=true
[plugin]
mainfile=main.lua
INI
cp "$HERE/teaching-probe.lua" "$D/config/xournalpp/plugins/TeachingProbe/main.lua"
python3 - "$D" <<'PY'
import pathlib,re,sys
root=pathlib.Path(sys.argv[1]); p=root/'config/xournalpp/settings.xml'
s=p.read_text(); value=str(root/'config/xournalpp/plugins/TeachingProbe')
s,n=re.subn(r'name="pluginEnabled" value="[^"]*"',f'name="pluginEnabled" value="{value}"',s)
assert n==1, 'pluginEnabled setting missing or ambiguous'
p.write_text(s)
PY
export XDG_CONFIG_HOME="$D/config" XDG_DATA_HOME="$D/data" XDG_RUNTIME_DIR="$D/run"
export APP GDK_BACKEND=x11 NO_AT_BRIDGE=1
unset WAYLAND_DISPLAY
xvfb-run -a -s '-screen 0 1500x1100x24' dbus-run-session -- bash -euo pipefail -s <<'INNER'
"$APP" >"$TEACHING_TEST_DIR/app.log" 2>&1 &
APP_PID=$!
trap 'kill "$APP_PID" 2>/dev/null || true' EXIT
OBJECT=/com/github/xournalpp/xournalpp/window/1
BUS=''
for _ in $(seq 1 100); do
    for name in $(busctl --user list 2>/dev/null | awk '$3=="xournalpp"{print $1}'); do
        if gdbus call --session --dest "$name" --object-path "$OBJECT" --method org.gtk.Actions.DescribeAll 2>/dev/null | grep -q 'plugins.action-0'; then
            BUS=$name; break
        fi
    done
    [ -z "$BUS" ] || break
    sleep 0.2
done
[ -n "$BUS" ] || { cat "$TEACHING_TEST_DIR/app.log"; exit 1; }
gdbus call --session --dest "$BUS" --object-path "$OBJECT" --method org.gtk.Actions.Activate plugins.action-0 '[]' '{}'
for _ in $(seq 1 100); do
    if grep -q 'TEACHING_RESULT' "$TEACHING_TEST_DIR/app.log"; then break; fi
    sleep 0.2
done
sleep 0.6  # Let the queued GTK canvas repaint finish before taking evidence.
if command -v import >/dev/null; then import -window root "$TEACHING_TEST_DIR/selected-sine.png"; fi
INNER
cat "$D/app.log"
echo "Test evidence: $D"
grep -q 'TEACHING_RESULT PASS' "$D/app.log"
! grep -q 'TEACHING_FAIL' "$D/app.log"
