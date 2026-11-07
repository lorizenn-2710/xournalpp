#!/usr/bin/python3
"""GTK3 interaction experiment ONLY. No Xournal++ profile/document is touched.

This deliberately is NOT a substitute for the native pen/eraser/selection engine.
JSON is disposable research evidence, NOT the proposed production format.
"""
import copy
import os
import pathlib
import sys
import gi
gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gdk, GLib, Gtk
from model import Notebook, clamp_panel

ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = ROOT / "output/floating-notes-research-2026-11-03"
STATE = OUT / "prototype-state.json"


class Prototype:
    def __init__(self):
        OUT.mkdir(parents=True, exist_ok=True)
        self.book = Notebook.load(STATE, "research-document-A") if STATE.exists() else Notebook()
        self.ink, self.drag_start = [], None
        self.window = Gtk.Window(title="THỬ NGHIỆM – NHÁP NỔI (chưa tích hợp Xournal++)")
        self.window.set_default_size(1120, 760)
        self.window.set_focus_on_map(os.environ.get("FLOATING_NOTES_ISOLATED") == "1")
        self.window.connect("destroy", lambda *_: Gtk.main_quit())
        self.window.connect("delete-event", lambda *_: self.save() or False)
        self.window.connect("key-press-event", self.key)
        layout = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.window.add(layout)
        bar = Gtk.Box(spacing=5)
        layout.pack_start(bar, False, False, 0)
        self.button(bar, "Ẩn/hiện nháp • Alt+N", self.toggle)
        self.button(bar, "Tờ mới", self.new)
        self.button(bar, "Trang đề kế", self.next_page)
        self.button(bar, "Cửa sổ nhỏ", self.small)
        self.button(bar, "Tải lại bản lưu thử", self.reload)
        self.button(bar, "Đóng thử nghiệm", lambda *_: self.window.destroy())
        self.tabs = Gtk.Box(spacing=4)
        layout.pack_start(self.tabs, False, False, 0)
        self.overlay = Gtk.Overlay()
        layout.pack_start(self.overlay, True, True, 0)
        self.base = Gtk.DrawingArea()
        self.base.add_events(Gdk.EventMask.BUTTON_PRESS_MASK)
        self.base.connect("draw", self.draw_base)
        self.base.connect("button-press-event", self.base_click)
        self.overlay.add(self.base)
        self.panel = Gtk.Frame()
        self.panel.set_shadow_type(Gtk.ShadowType.ETCHED_IN)
        self.overlay.add_overlay(self.panel)
        self.overlay.set_overlay_pass_through(self.panel, False)
        self.overlay.connect("get-child-position", self.position)
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.panel.add(box)
        self.header = Gtk.EventBox()
        provider = Gtk.CssProvider()
        provider.load_from_data(b"#note-header { background-color: #eecb69; color: #202530; } #note-header label { color: #202530; }")
        self.header.set_name("note-header")
        Gtk.StyleContext.add_provider_for_screen(Gdk.Screen.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        self.header.add_events(Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK |
                               Gdk.EventMask.POINTER_MOTION_MASK)
        self.header.connect("button-press-event", self.start_move)
        self.header.connect("motion-notify-event", self.move)
        self.header.connect("button-release-event", self.finish_move)
        self.title = Gtk.Label(label="Kéo tại đây để dời nháp")
        self.title.set_size_request(-1, 38)
        self.header.add(self.title)
        box.pack_start(self.header, False, False, 0)
        tools = Gtk.Box(spacing=2)
        box.pack_start(tools, False, False, 0)
        self.button(tools, "Ẩn", self.toggle)
        self.button(tools, "Hoàn tác nháp", self.undo)
        self.button(tools, "Làm lại", self.redo)
        self.button(tools, "Thêm giấy", self.more)
        self.scroller = Gtk.ScrolledWindow()
        self.scroller.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self.scroller.set_min_content_width(280)
        self.scroller.set_min_content_height(170)
        box.pack_start(self.scroller, True, True, 0)
        self.paper = Gtk.DrawingArea()
        self.paper.add_events(Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK |
                              Gdk.EventMask.POINTER_MOTION_MASK)
        self.paper.connect("draw", self.draw_paper)
        self.paper.connect("button-press-event", self.start_ink)
        self.paper.connect("motion-notify-event", self.continue_ink)
        self.paper.connect("button-release-event", self.finish_ink)
        self.scroller.add_with_viewport(self.paper)
        self.scroller.get_vadjustment().connect("value-changed", self.scrolled)
        self.status = Gtk.Label(xalign=0)
        layout.pack_start(self.status, False, False, 0)
        self.refresh()
        self.window.show_all()
        self.panel.set_visible(self.book.data["visible"])
        GLib.idle_add(self.restore_scroll)

    def button(self, box, text, callback):
        button = Gtk.Button(label=text)
        button.set_size_request(-1, 36)
        button.connect("clicked", callback)
        box.pack_start(button, False, False, 0)
        return button

    def save(self):
        self.book.save(STATE)
        return False

    def refresh(self):
        for child in self.tabs.get_children():
            self.tabs.remove(child)
        for note in self.book.data["notes"]:
            self.button(self.tabs, note["title"], lambda _, key=note["id"]: self.select(key))
        self.tabs.show_all()
        self.paper.set_size_request(600, self.book.note["height"])
        self.title.set_text(f"{self.book.note['title']} • nguồn trang {self.book.note['source_page']} • KÉO ĐỂ DỜI")
        self.status.set_text(f"Thử giao diện • {len(self.book.note['strokes'])} nét nháp • "
                             f"{len(self.book.data['base_strokes'])} lần chạm nền • "
                             f"Đề trang {self.book.data['page']} • JSON nghiên cứu, chưa phải .xopp")
        self.paper.queue_draw()
        self.base.queue_draw()
        self.save()

    def position(self, overlay, widget, allocation):
        if widget != self.panel:
            return False
        values = clamp_panel(*self.book.data["panel"], overlay.get_allocated_width(), overlay.get_allocated_height())
        allocation.x, allocation.y, allocation.width, allocation.height = map(int, values)
        if self.book.data["panel"] != list(values):
            self.book.data["panel"] = list(values)
            GLib.idle_add(self.save)
        return True

    def draw_base(self, widget, cr):
        cr.set_source_rgb(.91, .92, .94)
        cr.paint()
        cr.set_source_rgb(1, 1, 1)
        cr.rectangle(40, 25, 750, 950)
        cr.fill()
        cr.set_source_rgb(.15, .18, .23)
        cr.select_font_face("Sans", 0, 0)
        cr.set_font_size(22)
        for y, text in [(65, f"TRANG ĐỀ {self.book.data['page']} – NỀN THỬ NGHIỆM"),
                        (115, "Cho hai vectơ u = (1; 2), v = (3; -1)."),
                        (155, "Tính u + v và độ dài của vectơ tổng."),
                        (245, "Nháp nổi không làm đổi bố cục trang đề."),
                        (285, "Ẩn tờ nháp để đọc đề, mở lại để viết tiếp.")]:
            cr.move_to(70, y)
            cr.show_text(text)
        cr.set_source_rgb(.75, .1, .1)
        for x, y in self.book.data["base_strokes"]:
            cr.arc(x, y, 5, 0, 6.284)
            cr.fill()

    def draw_paper(self, widget, cr):
        cr.set_source_rgb(1, .985, .90)
        cr.paint()
        cr.set_line_width(.6)
        cr.set_source_rgb(.72, .80, .88)
        for y in range(30, self.book.note["height"], 30):
            cr.move_to(0, y)
            cr.line_to(600, y)
        cr.stroke()
        cr.set_source_rgb(.1, .2, .65)
        cr.set_line_width(2.8)
        cr.set_line_cap(1)
        for points in self.book.note["strokes"] + ([self.ink] if self.ink else []):
            cr.move_to(*points[0])
            for p in points[1:]:
                cr.line_to(*p)
            cr.stroke()
        return False

    def base_click(self, widget, event):
        self.book.data["base_strokes"].append([event.x, event.y])
        self.refresh()
        return True

    def start_ink(self, widget, event):
        if event.button == 1:
            self.ink = [[event.x, event.y]]
            return True
        return False

    def continue_ink(self, widget, event):
        if self.ink:
            self.ink.append([event.x, event.y])
            self.paper.queue_draw()
            return True
        return False

    def finish_ink(self, widget, event):
        if self.ink:
            self.ink.append([event.x, event.y])
            self.book.add_stroke(self.ink)
            self.ink = []
            self.refresh()
            return True
        return False

    def start_move(self, widget, event):
        if event.button == 1:
            self.drag_start = (event.x_root, event.y_root, *self.book.data["panel"][:2])
            return True
        return False

    def move(self, widget, event):
        if self.drag_start:
            sx, sy, x, y = self.drag_start
            self.book.data["panel"][:2] = [x + event.x_root - sx, y + event.y_root - sy]
            self.overlay.queue_resize()
            return True
        return False

    def finish_move(self, widget, event):
        self.drag_start = None
        self.save()
        return True

    def scrolled(self, adjustment):
        self.book.note["scroll"] = adjustment.get_value()
        self.save()

    def restore_scroll(self):
        self.scroller.get_vadjustment().set_value(self.book.note["scroll"])
        return False

    def toggle(self, *_):
        self.book.data["visible"] = not self.book.data["visible"]
        self.panel.set_visible(self.book.data["visible"])
        self.save()

    def select(self, key):
        self.book.select(key)
        self.refresh()
        self.panel.show_all()
        GLib.idle_add(self.restore_scroll)

    def new(self, *_):
        self.book.new_note()
        self.refresh()
        self.panel.show_all()
        GLib.idle_add(self.restore_scroll)

    def next_page(self, *_):
        self.book.data["page"] += 1
        self.refresh()

    def small(self, *_):
        self.window.resize(760, 600)

    def reload(self, *_):
        self.book = Notebook.load(STATE, "research-document-A")
        self.refresh()
        self.panel.set_visible(self.book.data["visible"])
        GLib.idle_add(self.restore_scroll)

    def undo(self, *_):
        self.book.undo()
        self.refresh()

    def redo(self, *_):
        self.book.redo_stroke()
        self.refresh()

    def more(self, *_):
        self.book.note["height"] += 1800
        self.refresh()

    def key(self, widget, event):
        if event.state & Gdk.ModifierType.MOD1_MASK and event.keyval in (Gdk.KEY_n, Gdk.KEY_N):
            self.toggle()
            return True
        if event.keyval == Gdk.KEY_Escape and self.book.data["visible"]:
            self.toggle()
            return True
        return False


if __name__ == "__main__":
    Prototype()
    Gtk.main()
