#!/usr/bin/env python3
"""Exercise the OCR worker protocol when two safety warnings apply at once.

The inference engine is replaced with deterministic readings; the real worker
still crops, splits lines, caps candidates and writes its INI response.
Run with the installed formula OCR Python environment.
"""
import configparser
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw


worker = Path(os.environ.get(
    'XPP_FORMULA_OCR_WORKER',
    Path(__file__).resolve().parents[2] / 'scripts/teaching-tools/formula-ocr.py',
))
with tempfile.TemporaryDirectory(prefix='xpp-ocr-warnings-') as directory:
    root = Path(directory)
    package = root / 'rapid_latex_ocr'
    package.mkdir()
    (package / '__init__.py').write_text(
        'class LaTeXOCR:\n'
        '    def __init__(self, **kwargs): self.calls = 0\n'
        '    def __call__(self, image):\n'
        '        self.calls += 1\n'
        '        return (f"x+{self.calls}", None)\n'
    )
    (package / 'utils_load.py').write_text(
        'class OrtInferSession:\n'
        '    def _init_sess_opt(self): pass\n'
    )
    models = root / 'models'
    models.mkdir()
    for name in ('image_resizer.onnx', 'encoder.onnx', 'decoder.onnx', 'tokenizer.json'):
        (models / name).touch()

    image = Image.new('RGB', (200, 220), 'white')
    draw = ImageDraw.Draw(image)
    for y in (1, 45, 89, 133, 177):
        draw.rectangle((1, y, 90, y + 3), fill='black')
    crop = root / 'crop.png'
    image.save(crop)

    env = os.environ.copy()
    env['PYTHONPATH'] = str(root)
    env['XOURNALPP_FORMULA_MODELS'] = str(models)
    process = subprocess.run(
        [sys.executable, str(worker), str(crop)],
        env=env, capture_output=True, text=True, timeout=10, check=True,
    )
    reply = configparser.ConfigParser()
    reply.read_string(process.stdout)
    assert reply.get('result', 'code') == 'ok'
    assert reply.getint('result', 'count') == 12
    notes = reply.get('result', 'notes')
    assert 'hơn 12 cách đọc' in notes, notes
    assert 'chạm mép vùng' in notes, notes
    print('OCR_WORKER_WARNINGS PASS: both warnings, 12-candidate cap')
