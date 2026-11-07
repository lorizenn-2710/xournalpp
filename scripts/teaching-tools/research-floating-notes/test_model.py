"""Design checks only. These do not certify the Xournal++ implementation."""
import copy
import pathlib
import random
import tempfile
import unittest
from model import Notebook, clamp_panel


class DesignChecks(unittest.TestCase):
    def setUp(self):
        self.book = Notebook()
        self.ink = [[40, 45], [80, 90], [200, 95]]
        self.book.add_stroke(self.ink)

    def test_hide_show_keeps_ink(self):
        old = copy.deepcopy(self.book.note)
        for _ in range(100):
            self.book.data["visible"] = not self.book.data["visible"]
        self.assertEqual(old, self.book.note)

    def test_new_note_keeps_previous(self):
        old = copy.deepcopy(self.book.note)
        new = self.book.new_note()
        self.assertNotEqual(old["id"], new["id"])
        self.assertEqual(new["strokes"], [])
        self.book.select(old["id"])
        self.assertEqual(old, self.book.note)

    def test_background_page_does_not_replace_active_note(self):
        old = copy.deepcopy(self.book.note)
        self.book.data["page"] = 22
        self.assertEqual(old, self.book.note)

    def test_undo_scope_and_redo(self):
        first = self.book.note["id"]
        self.book.new_note()
        self.book.add_stroke([[2, 3], [4, 5]])
        self.book.undo()
        self.assertEqual([], self.book.note["strokes"])
        self.book.select(first)
        self.assertEqual([self.ink], self.book.note["strokes"])
        self.book.undo()
        self.book.redo_stroke()
        self.assertEqual([self.ink], self.book.note["strokes"])

    def test_drawing_does_not_mutate_pdf(self):
        self.assertEqual([], self.book.data["base_strokes"])

    def test_more_paper_does_not_rescale_ink(self):
        old = copy.deepcopy(self.book.note["strokes"])
        self.book.note["height"] += 1800
        self.book.add_stroke([[50, 3200], [60, 3300]])
        self.assertEqual(old, self.book.note["strokes"][:1])

    def test_panel_resize_does_not_rescale_ink(self):
        old = copy.deepcopy(self.book.note["strokes"])
        self.book.data["panel"] = list(clamp_panel(-400, 9999, 1600, 1400, 800, 600))
        self.assertEqual(old, self.book.note["strokes"])

    def test_random_layout_clamp(self):
        rng = random.Random(20261103)
        for _ in range(1000):
            vw, vh = rng.randint(360, 2560), rng.randint(300, 1600)
            x, y, w, h = clamp_panel(rng.randint(-2000, 3000), rng.randint(-2000, 3000),
                                      rng.randint(100, 3000), rng.randint(100, 3000), vw, vh)
            self.assertGreaterEqual(x, 8)
            self.assertGreaterEqual(y, 8)
            self.assertLessEqual(x + w, vw - 8)
            self.assertLessEqual(y + h, vh - 8)

    def test_roundtrip_completed_stroke_and_hidden_note(self):
        self.book.data["visible"] = False
        self.book.note["scroll"] = 900
        with tempfile.TemporaryDirectory() as tmp:
            p = pathlib.Path(tmp) / "state.json"
            self.book.save(p)
            restored = Notebook.load(p, self.book.data["owner"])
            self.assertEqual(self.book.data, restored.data)

    def test_owner_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = pathlib.Path(tmp) / "state.json"
            self.book.save(p)
            with self.assertRaises(ValueError):
                Notebook.load(p, "unrelated-document")

    def test_atomic_interrupted_temporary_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = pathlib.Path(tmp) / "state.json"
            self.book.save(p)
            p.with_suffix(".tmp").write_text('{"version":')
            restored = Notebook.load(p, self.book.data["owner"])
            self.assertEqual(self.book.data, restored.data)

    def test_reject_invalid_ink(self):
        for points in ([[[float("nan"), 2]]], [[[float("inf"), 0]]], [[[-1, 2]]], [[[600, 1801]]]):
            with self.assertRaises(ValueError):
                self.book.add_stroke(points[0])


if __name__ == "__main__":
    unittest.main(verbosity=2)
