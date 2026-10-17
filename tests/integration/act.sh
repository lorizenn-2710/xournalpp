#!/bin/bash
# act.sh <n> [nhãn] — gọi action "plugins.action-<n>" của app Xournal++ đang chạy (instance test)
# rồi in ra phần log MỚI. Dùng để gọi thẳng hàm plugin mà không phải bơm phím.
#
#   XPPKT=${XPPKT:-/tmp/xpp-kt2}   # thư mục instance test
#   $XPPKT/bus.txt                 # tên bus D-Bus của instance (busctl --user list | grep xournalpp)
#   $XPPKT/app.log                 # log stdout của app (plugin print() ra đây)
set -u
D=${XPPKT:-/tmp/xpp-kt2}
B=$(cat "$D/bus.txt")
OBJ=/com/github/xournalpp/xournalpp/window/1
LOG=${D}/app.log
before=$(wc -l < "$LOG")
timeout 15 gdbus call --session --dest "$B" --object-path "$OBJ" \
    --method org.gtk.Actions.Activate "plugins.action-$1" "[]" "{}" >"$D/act.err" 2>&1
sleep 0.6
echo "----- action-$1 ${2:-}"
echo "gdbus: $(cat "$D/act.err")"
tail -n +$((before + 1)) "$LOG"
