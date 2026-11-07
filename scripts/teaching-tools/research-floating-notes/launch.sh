#!/bin/bash
# Launch only the isolated research prototype; never the user's document.
set -euo pipefail
export GDK_BACKEND=x11
exec /usr/bin/python3 /home/phuc/xpp-src/scripts/teaching-tools/research-floating-notes/prototype.py \
  > /home/phuc/xpp-src/build/floating-notes-research/prototype-app.log 2>&1
