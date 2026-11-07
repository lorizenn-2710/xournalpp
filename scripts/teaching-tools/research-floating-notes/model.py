"""Disposable design experiment. NOT Xournal++ storage or a production editor."""
import copy
import json
import math
import os
import pathlib
import uuid


def clamp_panel(x, y, width, height, viewport_width, viewport_height):
    """Keep the whole panel inside a usable viewport; resize the view, not ink."""
    width = min(max(320, width), max(1, viewport_width - 16))
    height = min(max(260, height), max(1, viewport_height - 16))
    return (max(8, min(x, viewport_width - width - 8)),
            max(8, min(y, viewport_height - height - 8)), width, height)


class Notebook:
    def __init__(self, owner="research-document-A"):
        self.data = {"version": 1, "owner": owner, "visible": True,
                     "active": None, "notes": [], "base_strokes": [], "page": 1,
                     "panel": [600, 75, 420, 440]}
        self.redo = {}
        self.new_note()

    @property
    def note(self):
        return next(n for n in self.data["notes"] if n["id"] == self.data["active"])

    def new_note(self):
        note = {"id": str(uuid.uuid4()), "title": f"Nháp {len(self.data['notes']) + 1}",
                "source_page": self.data["page"], "width": 600, "height": 1800,
                "scroll": 0, "strokes": []}
        self.data["notes"].append(note)
        self.data["active"] = note["id"]
        self.data["visible"] = True
        return note

    def add_stroke(self, points):
        if not points or any(not math.isfinite(v) for p in points for v in p):
            raise ValueError("invalid coordinates")
        if any(x < 0 or y < 0 or x > self.note["width"] or y > self.note["height"] for x, y in points):
            raise ValueError("outside paper")
        self.note["strokes"].append(copy.deepcopy(points))
        self.redo[self.note["id"]] = []

    def undo(self):
        if self.note["strokes"]:
            self.redo.setdefault(self.note["id"], []).append(self.note["strokes"].pop())

    def redo_stroke(self):
        redo = self.redo.setdefault(self.note["id"], [])
        if redo:
            self.note["strokes"].append(redo.pop())

    def select(self, note_id):
        if not any(n["id"] == note_id for n in self.data["notes"]):
            raise ValueError("unknown note")
        self.data["active"] = note_id
        self.data["visible"] = True

    def save(self, path):
        path = pathlib.Path(path)
        payload = json.dumps(self.data, ensure_ascii=False, allow_nan=False).encode()
        tmp = path.with_suffix(".tmp")
        with tmp.open("wb") as f:
            f.write(payload)
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, path)

    @classmethod
    def load(cls, path, owner):
        data = json.loads(pathlib.Path(path).read_text())
        if data["version"] != 1 or data["owner"] != owner:
            raise ValueError("wrong schema or owner")
        if not any(n["id"] == data["active"] for n in data["notes"]):
            raise ValueError("missing active note")
        result = cls.__new__(cls)
        result.data, result.redo = data, {}
        return result
