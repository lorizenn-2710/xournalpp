#!/bin/bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
export VK640_PLUGIN=${VK640_PLUGIN:-$HOME/.config/xournalpp/plugins/VK640ToolCycle/main.lua}
export PDF_FORMULA_PROBE="$HERE/pdf-region-shortcuts-probe.lua" PDF_FORMULA_GUI="$HERE/pdf-region-shortcuts-gui.py"
exec bash "$HERE/check-pdf-formula.sh"
