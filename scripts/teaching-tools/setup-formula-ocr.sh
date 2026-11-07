#!/bin/bash
# Installs isolated user runtime/models; never sends document contents anywhere.
set -euo pipefail
PREFIX=${1:-$HOME/.local}
RUNTIME="$PREFIX/share/xournalpp/formula-ocr"
uv python install 3.12.14
uv venv --python 3.12.14 --allow-existing "$RUNTIME"
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
uv pip install --python "$RUNTIME/bin/python" -r "$SCRIPT_DIR/requirements-ocr.lock.txt"
# Downloads public model weights once during SETUP only.
"$RUNTIME/bin/python" - "$SCRIPT_DIR/models-sha256.json" <<'PY'
from rapid_latex_ocr import LaTeXOCR
from pathlib import Path
import rapid_latex_ocr, hashlib, json, sys
LaTeXOCR()
p=Path(rapid_latex_ocr.__file__).parent/'models'
names=['encoder.onnx','decoder.onnx','image_resizer.onnx','tokenizer.json']
manifest={name:hashlib.sha256((p/name).read_bytes()).hexdigest() for name in names}
expected=json.loads(Path(sys.argv[1]).read_text())
if manifest != expected:raise RuntimeError('Model hash mismatch; preserve files and inspect before using.')
(p/'xournalpp-sha256.json').write_text(json.dumps(manifest,indent=2))
print(json.dumps(manifest,indent=2))
PY
uv pip freeze --python "$RUNTIME/bin/python" > "$RUNTIME/requirements-installed.txt"
