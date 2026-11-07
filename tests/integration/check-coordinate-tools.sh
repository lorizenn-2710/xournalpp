#!/bin/bash
# Same isolated GTK/X11/AT-SPI harness as the existing PDF acceptance suite.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
export PDF_FORMULA_PROBE="$HERE/coordinate-probe.lua"
export PDF_FORMULA_GUI="$HERE/coordinate-gui.py"
exec bash "$HERE/check-pdf-formula.sh"
