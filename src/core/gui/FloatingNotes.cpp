// SPDX-License-Identifier: GPL-2.0-or-later
#include "FloatingNotes.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>

#include <cairo-pdf.h>
#include <gdk/gdkkeysyms.h>
#include <pango/pangocairo.h>

#include "control/Control.h"
#include "control/FloatingNotebook.h"
#include "control/ToolEnums.h"
#include "control/actions/ActionDatabase.h"
#include "control/layer/LayerController.h"
#include "control/tools/EditSelection.h"
#include "gui/MainWindow.h"
#include "gui/PageView.h"
#include "gui/XournalView.h"
#include "gui/scroll/ScrollHandling.h"
#include "gui/toolbarMenubar/ToolbarShortcutHints.h"
#include "model/Document.h"
#include "model/Layer.h"
#include "model/XojPage.h"
#include "undo/DeleteUndoAction.h"
#include "undo/InsertUndoAction.h"
#include "undo/UndoAction.h"
#include "undo/UndoRedoHandler.h"
#include "util/Rectangle.h"
#include "util/XojMsgBox.h"
#include "view/DocumentView.h"
#include "view/ElementContainerView.h"
#include "view/View.h"

namespace {
enum DockAction { ToggleNotes, CreateNote, ToggleLibrary };
constexpr std::array dockAccelerators{"<Alt>n", "<Control><Alt>n", "<Alt>k"};

GtkWidget* button(const char* label, const char* tip, std::function<void()> callback) {
    auto* b = gtk_button_new_with_label(label);
    gtk_widget_set_size_request(b, 44, 42);
    if (auto* text = gtk_bin_get_child(GTK_BIN(b)); GTK_IS_LABEL(text))
        gtk_label_set_ellipsize(GTK_LABEL(text), PANGO_ELLIPSIZE_END);
    gtk_widget_set_tooltip_text(b, tip);
    atk_object_set_name(gtk_widget_get_accessible(b), tip);
    auto* fn = new std::function<void()>(std::move(callback));
    g_signal_connect_data(
            b, "clicked", G_CALLBACK(+[](GtkButton*, gpointer p) { (*static_cast<std::function<void()>*>(p))(); }), fn,
            +[](gpointer p, GClosure*) { delete static_cast<std::function<void()>*>(p); }, GConnectFlags(0));
    return b;
}
void append(GtkWidget* box, GtkWidget* child, bool expand = false) {
    gtk_box_pack_start(GTK_BOX(box), child, expand, expand, 0);
}
void label(cairo_t* cr, const std::string& text, double x, double y, double width, int size, bool bold = false) {
    auto* layout = pango_cairo_create_layout(cr);
    auto* font = pango_font_description_from_string(bold ? "Sans Bold" : "Sans");
    pango_font_description_set_absolute_size(font, size * PANGO_SCALE);
    pango_layout_set_font_description(layout, font);
    pango_layout_set_text(layout, text.c_str(), -1);
    pango_layout_set_width(layout, int(width * PANGO_SCALE));
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);
    pango_font_description_free(font);
    g_object_unref(layout);
}
class NoteUndo final: public UndoAction {
    FloatingNoteState before, after;
    double oldHeight, newHeight;

public:
    NoteUndo(PageRef p, FloatingNoteState b, FloatingNoteState a, double h0 = 0, double h1 = 0):
            UndoAction("FloatingNoteUndoAction"),
            before(std::move(b)),
            after(std::move(a)),
            oldHeight(h0),
            newHeight(h1) {
        page = p;
    }
    bool apply(Control* c, const FloatingNoteState& s, double height) {
        {
            std::unique_lock lock(*c->getDocument());
            page->floatingNote = s;
            if (height)
                page->setSize(page->getWidth(), height);
            ++page->floatingNote->revision;
        }
        c->getWindow()->getFloatingNotes()->refresh();
        return true;
    }
    bool undo(Control* c) override { return apply(c, before, oldHeight); }
    bool redo(Control* c) override { return apply(c, after, newHeight); }
    std::string getText() override { return "Sổ nháp nổi"; }
};
class PaperUndo final: public UndoAction {
    PageType other;

public:
    PaperUndo(PageRef p, PageType original): UndoAction("FloatingPaperUndoAction"), other(original) { page = p; }
    bool swap(Control* c) {
        {
            std::unique_lock lock(*c->getDocument());
            auto old = page->getBackgroundType();
            page->setBackgroundType(other);
            other = old;
        }
        page->firePageChanged();
        return true;
    }
    bool undo(Control* c) override { return swap(c); }
    bool redo(Control* c) override { return swap(c); }
    std::string getText() override { return "Đổi giấy nháp"; }
};

}  // namespace

struct FloatingNotes::Editor {
    FloatingNotes* owner;
    PageRef page;
    GtkWidget *frame = nullptr, *title = nullptr, *scrolled = nullptr, *move = nullptr;
    std::unique_ptr<ScrollHandling> scroll;
    std::unique_ptr<XournalView> view;
    std::vector<std::pair<GtkWidget*, std::string>> controls;
    bool dragging = false, resizing = false, focused = false;
    double rootX = 0, rootY = 0, startX = 0, startY = 0;
    double smallX = 0, smallY = 0, smallW = 440, smallH = 430;
    guint restoreSource = 0;
    ~Editor() {
        if (restoreSource)
            g_source_remove(restoreSource);
        view.reset();
        if (frame)
            gtk_widget_destroy(frame);
    }
};

FloatingNotes::FloatingNotes(MainWindow* w, GtkOverlay* o): window(w), control(w->getControl()), overlay(o) {
    registerListener(control);
    // GTK3's window key signal does not see transient dialogs or grabbed menus.
    // Scope this pre-dispatch hook to this document's window/transient chain only.
    keySnooper = gtk_key_snooper_install(
            +[](GtkWidget* grab, GdkEventKey* event, gpointer data) -> gint {
                auto* self = static_cast<FloatingNotes*>(data);
                GtkWidget* widget = grab;
                if (!widget && event->window) {
                    gpointer owner = nullptr;
                    gdk_window_get_user_data(event->window, &owner);
                    if (owner && GTK_IS_WIDGET(owner))
                        widget = GTK_WIDGET(owner);
                }
                if (!widget)
                    return false;
                if (GTK_IS_MENU(widget))
                    widget = gtk_menu_get_attach_widget(GTK_MENU(widget));
                if (!widget)
                    return false;
                auto* top = gtk_widget_get_toplevel(widget);
                while (GTK_IS_WINDOW(top)) {
                    if (top == self->window->getWindow()) {
                        if (event->type == GDK_KEY_RELEASE)
                            return keyRelease(nullptr, event, self);
                        return gtk_widget_get_sensitive(GTK_WIDGET(self->overlay)) && keyPress(nullptr, event, self);
                    }
                    top = GTK_WIDGET(gtk_window_get_transient_for(GTK_WINDOW(top)));
                }
                return false;
            },
            this);
    focusOutSignal = g_signal_connect(window->getWindow(), "focus-out-event",
                                      G_CALLBACK(+[](GtkWidget*, GdkEventFocus*, gpointer data) -> gboolean {
                                          auto* self = static_cast<FloatingNotes*>(data);
                                          self->shortcutDown.fill(false);
                                          return false;
                                      }),
                                      this);
    // Theme colors come from GTK; local CSS only defines geometry and borders.
    auto* css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
            css,
            ".floating-note {background-color:@theme_bg_color; color:@theme_fg_color; border:2px solid @borders; "
            "border-radius:10px; box-shadow:0 3px 12px alpha(black,.25);}"
            ".floating-note.active {border-color:@theme_selected_bg_color;}"
            ".floating-note button {padding:3px 7px;}"
            ".floating-note entry {min-height:36px;}"
            ".floating-note .title {font-weight:bold;}"
            ".floating-note-dock {background-color:@theme_bg_color; border-radius:6px; padding:3px;}"
            ".floating-note button.gallery-hit, .floating-note button.gallery-hit:hover, .floating-note "
            "button.gallery-hit:active {background:transparent;border:none;box-shadow:none;padding:0;}",
            -1, nullptr);
    gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(GTK_WIDGET(o)), GTK_STYLE_PROVIDER(css),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_set_data_full(G_OBJECT(o), "floating-note-css", css, g_object_unref);
    allocationSignal = g_signal_connect(
            o, "size-allocate", G_CALLBACK(+[](GtkWidget*, GtkAllocation* allocation, gpointer p) {
                auto* self = static_cast<FloatingNotes*>(p);
                if (self->allocatedWidth == allocation->width && self->allocatedHeight == allocation->height)
                    return;
                self->allocatedWidth = allocation->width;
                self->allocatedHeight = allocation->height;
                // Changing child requisitions inside size-allocate can leave GtkOverlay
                // using the old sizes. Apply the new bounds after this allocation finishes.
                if (!self->allocationSource)
                    self->allocationSource = g_idle_add(
                            +[](gpointer data) -> gboolean {
                                auto* self = static_cast<FloatingNotes*>(data);
                                self->allocationSource = 0;
                                gtk_orientable_set_orientation(GTK_ORIENTABLE(self->dock),
                                                               self->allocatedWidth < 290 ? GTK_ORIENTATION_VERTICAL :
                                                                                            GTK_ORIENTATION_HORIZONTAL);
                                for (auto& e: self->editors) self->arrange(*e);
                                if (self->library)
                                    gtk_widget_set_size_request(
                                            self->library, std::max(180, std::min(650, self->allocatedWidth - 24)),
                                            std::max(140, std::min(620, self->allocatedHeight - 24)));
                                return G_SOURCE_REMOVE;
                            },
                            self);
            }),
            this);

    // Persistent pen entry point, inside the application viewport only.
    dock = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_style_context_add_class(gtk_widget_get_style_context(dock), "floating-note-dock");
    gtk_widget_set_halign(dock, GTK_ALIGN_END);
    gtk_widget_set_valign(dock, GTK_ALIGN_END);
    gtk_widget_set_margin_end(dock, 12);
    gtk_widget_set_margin_bottom(dock, 12);
    auto addDockButton = [this](DockAction action, const char* text, const char* tip, std::function<void()> callback) {
        auto* column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        auto* control = button(text, tip, std::move(callback));
        append(column, control);
        dockCaptions[action] = ToolbarShortcutHints::createCaption();
        append(column, dockCaptions[action]);
        append(dock, column);
        return control;
    };
    addDockButton(ToggleNotes, "Nháp nổi", "Nháp nổi — Alt+N", [this] { toggle(); });
    addDockButton(CreateNote, "Tờ mới", "Tạo tờ mới — Ctrl+Alt+N", [this] { create(); });
    auto* libraryButton =
            addDockButton(ToggleLibrary, "Kho nháp", "Kho nháp — tìm và mở lại các tờ", [this] { toggleLibrary(); });
    gtk_widget_set_tooltip_text(libraryButton, "Mở/cất kho nháp — Alt+K");
    gtk_overlay_add_overlay(overlay, dock);
    gtk_widget_show_all(dock);
}
void FloatingNotes::setShortcutHintsVisible(bool visible) {
    for (size_t i = 0; i < dockCaptions.size(); ++i)
        ToolbarShortcutHints::updateCaption(dockCaptions[i], dockAccelerators[i], visible);
}
FloatingNotes::~FloatingNotes() {
    closing = true;
    aliveToken.reset();
    if (allocationSource)
        g_source_remove(allocationSource);
    gtk_key_snooper_remove(keySnooper);
    g_signal_handler_disconnect(window->getWindow(), focusOutSignal);
    if (refreshSource)
        g_source_remove(refreshSource);
    if (thumbnailSource)
        g_source_remove(thumbnailSource);
    g_signal_handler_disconnect(overlay, allocationSignal);

    active = nullptr;
    editors.clear();
    if (library)
        gtk_widget_destroy(library);
}
XournalView* FloatingNotes::getActiveView() const { return active; }
XournalView* FloatingNotes::getViewForPage(const PageRef& p) const {
    auto* e = find(p);
    return e ? e->view.get() : nullptr;
}
FloatingNotes::Editor* FloatingNotes::find(const PageRef& p) const {
    for (const auto& e: editors)
        if (e->page == p)
            return e.get();
    return nullptr;
}
void FloatingNotes::activate(XournalView* view) {
    XournalView* next = view && view->isFloating() ? view : nullptr;
    if (next == active)
        return;
    auto* previous = active ? active : window->getXournal();
    if (previous) {
        previous->finishInput();
        previous->clearSelection();
    }
    control->editingTargetChanged();
    active = next;
    if (active) {
        {
            std::unique_lock lock(*control->getDocument());
            size_t next = 0;
            for (auto& p: control->getDocument()->floatingNotes) next = std::max(next, p->floatingNote->zOrder);
            active->getViewFor(0)->getPage()->floatingNote->zOrder = next + 1;
        }
        for (auto& e: editors) {
            if (e->view.get() == active) {
                selected = e->page;
                gtk_overlay_reorder_overlay(overlay, e->frame, -1);
                if (gtk_widget_get_realized(e->frame))
                    gdk_window_raise(gtk_widget_get_window(e->frame));
                gtk_style_context_add_class(gtk_widget_get_style_context(e->frame), "active");
            } else
                gtk_style_context_remove_class(gtk_widget_get_style_context(e->frame), "active");
        }
    }
    control->setClipboardHandlerSelection(active ? active->getSelection() : window->getXournal()->getSelection());
    control->undoRedoChanged();
    if (control->getLayerController())
        control->getLayerController()->fireRebuildLayerMenu();
    if (gallery)
        gtk_widget_queue_draw(gallery);
}
void FloatingNotes::rememberPenPosition(GtkWidget* widget, double x, double y) {
    int px = 0, py = 0;
    if (!gtk_widget_translate_coordinates(widget, GTK_WIDGET(overlay), int(x), int(y), &px, &py))
        return;
    if (px >= 0 && py >= 0 && px < gtk_widget_get_allocated_width(GTK_WIDGET(overlay)) &&
        py < gtk_widget_get_allocated_height(GTK_WIDGET(overlay)))
        lastPenPosition = std::pair{double(px), double(py)};
}
void FloatingNotes::setSensitive(bool enabled) { gtk_widget_set_sensitive(GTK_WIDGET(overlay), enabled); }
void FloatingNotes::prepareSave() {
    for (auto& e: editors) {
        e->view->finishInput();
        e->view->clearSelection();
    }
    flush();
}
void FloatingNotes::flush() {
    std::unique_lock lock(*control->getDocument());
    for (auto& e: editors) {
        auto& n = *e->page->floatingNote;
        n.scrollX = gtk_adjustment_get_value(e->scroll->getHorizontal());
        n.scrollY = gtk_adjustment_get_value(e->scroll->getVertical());
    }
}
PageRef FloatingNotes::create(const PageRef& clone) {
    auto p = clone ? PageRef(clone->clone()) : std::make_shared<XojPage>(600, 840);
    FloatingNoteState n = clone ? *clone->floatingNote : FloatingNoteState{};
    n.name = clone ? n.name + " (bản sao)" : "Nháp " + std::to_string(control->getDocument()->floatingNotes.size() + 1);
    n.deleted = false;
    n.open = true;
    n.revision = 1;
    n.created = n.modified = size_t(g_get_real_time() / 1000);
    if (!clone) {
        auto source = control->getCurrentPage();
        n.sourceId = source ? source->stableId : "";
    }
    auto count = control->getDocument()->floatingNotes.size();
    n.x = 24 + double(count % 4) * 48;
    n.y = 24 + double(count % 4) * 36;
    if (gtk_widget_get_allocated_width(GTK_WIDGET(overlay)) >= 940 && count % 2)
        n.x = 480;
    if (lastPenPosition) {
        const auto [px, py] = *lastPenPosition;
        const auto viewport = gtk_widget_get_allocated_width(GTK_WIDGET(overlay));
        n.x = px + n.width + 32 < viewport ? px + 32 : px - n.width - 32;
        n.y = py - 32;
    }
    p->floatingNote = n;
    {
        std::unique_lock lock(*control->getDocument());
        control->getDocument()->floatingNotes.push_back(p);
    }
    auto before = n;
    before.deleted = true;
    before.open = false;
    control->getUndoRedoHandler()->addUndoAction(std::make_unique<NoteUndo>(p, before, n));
    open(p);
    updateFilter();
    return p;
}
void FloatingNotes::toggle() {
    // Key repeat cannot create or flip the state repeatedly (see keyPress/release).
    auto& notes = control->getDocument()->floatingNotes;
    if (std::none_of(notes.begin(), notes.end(), [](auto& p) { return !p->floatingNote->deleted; })) {
        create();
        return;
    }
    if (std::any_of(notes.begin(), notes.end(),
                    [](const auto& p) { return p->floatingNote->open && !p->floatingNote->deleted; })) {
        hiddenGroup.clear();
        for (auto& p: notes)
            if (!p->floatingNote->deleted && p->floatingNote->open)
                hiddenGroup.push_back(p->stableId);
        for (auto& p: notes)
            if (p->floatingNote->open)
                stash(p);
        return;
    }
    bool any = false;
    for (auto& p: notes)
        if (!p->floatingNote->deleted &&
            std::find(hiddenGroup.begin(), hiddenGroup.end(), p->stableId) != hiddenGroup.end()) {
            open(p);
            any = true;
        }
    if (!any) {
        auto p = selected;
        if (!p || p->floatingNote->deleted)
            for (auto& candidate: notes)
                if (!candidate->floatingNote->deleted) {
                    p = candidate;
                    break;
                }
        if (p)
            open(p);
    }
}
void FloatingNotes::arrange(Editor& e) {
    auto& n = *e.page->floatingNote;
    const double w = std::max(1, gtk_widget_get_allocated_width(GTK_WIDGET(overlay)));
    const double h = std::max(1, gtk_widget_get_allocated_height(GTK_WIDGET(overlay)));
    if (w < 30 || h < 30)
        return;
    std::unique_lock lock(*control->getDocument());
    const double fw = std::min(n.width, std::max(200., w - 16));
    const double fh = std::min(n.height, std::max(180., h - 16));
    n.x = std::clamp(n.x, 0., std::max(0., w - fw - 4));
    n.y = std::clamp(n.y, 0., std::max(0., h - fh - 4));
    gtk_widget_set_margin_start(e.frame, int(n.x));
    gtk_widget_set_margin_top(e.frame, int(n.y));
    gtk_widget_set_size_request(e.frame, int(fw), int(fh));
}
void FloatingNotes::open(const PageRef& page) {
    if (!page || !page->floatingNote || page->floatingNote->deleted)
        return;
    auto* e = find(page);
    if (!e) {
        // At most six native editors; previews are independent and bounded.
        if (editors.size() >= 6) {
            auto it =
                    std::find_if(editors.begin(), editors.end(), [](auto& e) { return !e->page->floatingNote->open; });
            if (it == editors.end()) {
                stash(editors.front()->page);
                it = editors.begin();
            }
            editors.erase(it);
        }
        auto entry = std::make_unique<Editor>();
        e = entry.get();
        e->owner = this;
        e->page = page;
        auto sheetButton = [&](const char* label, const char* tip, std::function<void()> fn) {
            auto* b = button(label, tip, std::move(fn));
            e->controls.emplace_back(b, tip);
            atk_object_set_name(gtk_widget_get_accessible(b),
                                (std::string(tip) + " · " + page->floatingNote->name).c_str());
            return b;
        };
        e->frame = gtk_event_box_new();
        gtk_widget_set_halign(e->frame, GTK_ALIGN_START);
        gtk_widget_set_valign(e->frame, GTK_ALIGN_START);
        gtk_style_context_add_class(gtk_widget_get_style_context(e->frame), "floating-note");
        auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(e->frame), box);
        auto* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        e->move = gtk_event_box_new();
        e->title = gtk_label_new(page->floatingNote->name.c_str());
        gtk_label_set_ellipsize(GTK_LABEL(e->title), PANGO_ELLIPSIZE_END);
        gtk_widget_set_size_request(e->title, 60, 42);
        gtk_style_context_add_class(gtk_widget_get_style_context(e->title), "title");
        gtk_container_add(GTK_CONTAINER(e->move), e->title);
        gtk_widget_add_events(e->move, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
        append(bar, e->move, true);
        auto* options = sheetButton("⋯", "Thao tác tờ nháp", [] {});
        g_signal_connect(options, "clicked", G_CALLBACK(+[](GtkWidget* b, gpointer p) {
                             auto* e = static_cast<Editor*>(p);
                             e->owner->menu(*e, b);
                         }),
                         e);
        append(bar, options);
        append(bar, sheetButton("▣", "Mở rộng / trở lại khung nhỏ", [this, e] {
                   activate(e->view.get());
                   e->view->finishInput();
                   auto& n = *e->page->floatingNote;
                   {
                       std::unique_lock lock(*control->getDocument());
                       if (!e->focused) {
                           e->smallX = n.x;
                           e->smallY = n.y;
                           e->smallW = n.width;
                           e->smallH = n.height;
                           n.x = 8;
                           n.y = 8;
                           n.width = gtk_widget_get_allocated_width(GTK_WIDGET(overlay)) - 16;
                           n.height = gtk_widget_get_allocated_height(GTK_WIDGET(overlay)) - 16;
                       } else {
                           n.x = e->smallX;
                           n.y = e->smallY;
                           n.width = e->smallW;
                           n.height = e->smallH;
                       }
                   }
                   e->focused = !e->focused;
                   arrange(*e);
               }));
        append(bar, sheetButton("⌄", "Cất tờ về kho — giữ nguyên bài nháp", [this, page] { stash(page); }));
        append(box, bar);
        e->scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(e->scrolled), false);
        gtk_widget_set_vexpand(e->scrolled, true);
        append(box, e->scrolled, true);
        e->scroll = std::make_unique<ScrollHandling>(GTK_SCROLLED_WINDOW(e->scrolled));
        e->view = std::make_unique<XournalView>(e->scrolled, control, e->scroll.get(), page);
        atk_object_set_name(gtk_widget_get_accessible(e->view->getWidget()),
                            ("Giấy " + page->floatingNote->name).c_str());
        auto* footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        append(footer, sheetButton("−", "Thu nhỏ giấy nháp", [this, e] {
                   activate(e->view.get());
                   e->view->setFloatingZoom(e->view->getZoom() / 1.15);
               }));
        append(footer, sheetButton("+", "Phóng to giấy nháp", [this, e] {
                   activate(e->view.get());
                   e->view->setFloatingZoom(e->view->getZoom() * 1.15);
               }));
        append(footer,
               sheetButton("+ Giấy", "Thêm 840 pt giấy, không co mực",
                           [this, e] {
                               activate(e->view.get());
                               e->view->finishInput();
                               e->view->clearSelection();
                               auto before = *e->page->floatingNote;
                               const double h = e->page->getHeight();
                               if (h >= 840 * 64) {
                                   XojMsgBox::showErrorToUser(
                                           control->getGtkWindow(),
                                           "Tờ này đã đủ 64 đoạn giấy. Hãy tạo tờ mới; bài cũ được giữ nguyên.");
                                   return;
                               }
                               {
                                   std::unique_lock lock(*control->getDocument());
                                   e->page->setSize(e->page->getWidth(), h + 840);
                               }
                               control->getUndoRedoHandler()->addUndoAction(
                                       std::make_unique<NoteUndo>(e->page, before, *e->page->floatingNote, h, h + 840));
                               e->view->layoutPages();
                               e->view->getViewFor(0)->rerenderPage(true);
                           }),
               true);
        auto* resize = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(resize), gtk_label_new("◢"));
        gtk_widget_set_size_request(resize, 44, 42);
        gtk_widget_set_tooltip_text(resize, "Kéo để đổi khung nhìn, không đổi tỉ lệ mực");
        gtk_widget_add_events(resize, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
        append(footer, resize);
        append(box, footer);
        auto start = +[](GtkWidget* widget, GdkEventButton* ev, gpointer data) -> gboolean {
            if (ev->button != 1)
                return false;
            auto* e = static_cast<Editor*>(data);
            e->view->finishInput();
            e->owner->activate(e->view.get());
            e->resizing = widget != e->move;
            e->dragging = !e->resizing;
            e->rootX = ev->x_root;
            e->rootY = ev->y_root;
            auto& n = *e->page->floatingNote;
            e->startX = e->resizing ? n.width : n.x;
            e->startY = e->resizing ? n.height : n.y;
            return true;
        };
        auto motion = +[](GtkWidget*, GdkEventMotion* ev, gpointer data) -> gboolean {
            auto* e = static_cast<Editor*>(data);
            if (!e->dragging && !e->resizing)
                return false;
            auto& n = *e->page->floatingNote;
            {
                std::unique_lock lock(*e->owner->control->getDocument());
                if (e->resizing) {
                    n.width = std::max(280., e->startX + ev->x_root - e->rootX);
                    n.height = std::max(240., e->startY + ev->y_root - e->rootY);
                } else {
                    n.x = e->startX + ev->x_root - e->rootX;
                    n.y = e->startY + ev->y_root - e->rootY;
                }
            }
            e->owner->arrange(*e);
            return true;
        };
        auto end = +[](GtkWidget*, GdkEventButton*, gpointer data) -> gboolean {
            auto* e = static_cast<Editor*>(data);
            e->dragging = e->resizing = false;
            e->owner->flush();
            return true;
        };
        for (auto* handle: {e->move, resize}) {
            g_signal_connect(handle, "button-press-event", G_CALLBACK(start), e);
            g_signal_connect(handle, "motion-notify-event", G_CALLBACK(motion), e);
            g_signal_connect(handle, "button-release-event", G_CALLBACK(end), e);
        }
        gtk_overlay_add_overlay(overlay, e->frame);
        editors.push_back(std::move(entry));
    }
    {
        std::unique_lock lock(*control->getDocument());
        page->floatingNote->open = true;
    }
    arrange(*e);
    gtk_widget_show_all(e->frame);
    gtk_overlay_reorder_overlay(overlay, e->frame, -1);
    if (gtk_widget_get_realized(e->frame))
        gdk_window_raise(gtk_widget_get_window(e->frame));
    activate(e->view.get());
    e->view->getViewFor(0)->setSelected(true);
    // GTK adjustments become valid after allocation. Restore exactly once, never after writing.
    if (e->restoreSource)
        g_source_remove(e->restoreSource);
    e->restoreSource = g_idle_add(
            +[](gpointer d) -> gboolean {
                auto* e = static_cast<Editor*>(d);
                e->restoreSource = 0;
                gtk_adjustment_set_value(e->scroll->getHorizontal(), e->page->floatingNote->scrollX);
                gtk_adjustment_set_value(e->scroll->getVertical(), e->page->floatingNote->scrollY);
                return G_SOURCE_REMOVE;
            },
            e);
    if (gallery)
        gtk_widget_queue_draw(gallery);
}
void FloatingNotes::stash(const PageRef& p) {
    if (auto* e = find(p)) {
        e->view->finishInput();
        e->view->clearSelection();
        flush();
        if (active == e->view.get())
            activate(nullptr);
        gtk_widget_hide(e->frame);
    }
    {
        std::unique_lock lock(*control->getDocument());
        p->floatingNote->open = false;
    }
    if (gallery)
        gtk_widget_queue_draw(gallery);
}
void FloatingNotes::changed(const PageRef& p) {
    if (closing || rebuilding || !p || !p->floatingNote)
        return;
    if (std::find(pendingChanges.begin(), pendingChanges.end(), p) == pendingChanges.end())
        pendingChanges.push_back(p);
    // Commit callbacks can hold document/history locks. Defer metadata and thumbnails
    // to the UI loop; no render, I/O or second Document lock on a motion/undo callback.
    if (!refreshSource)
        refreshSource = g_timeout_add(
                120,
                +[](gpointer d) -> gboolean {
                    auto* self = static_cast<FloatingNotes*>(d);
                    self->refreshSource = 0;
                    {
                        std::unique_lock lock(*self->control->getDocument());
                        for (auto& p: self->pendingChanges) {
                            p->floatingNote->modified = size_t(g_get_real_time() / 1000);
                            ++p->floatingNote->revision;
                            self->thumbnails.erase(p->stableId);
                        }
                        self->pendingChanges.clear();
                    }
                    if (self->gallery)
                        self->updateFilter();
                    return G_SOURCE_REMOVE;
                },
                this);
}
void FloatingNotes::refresh() {
    for (auto& e: editors) {
        auto& n = *e->page->floatingNote;
        for (auto& [widget, tip]: e->controls)
            atk_object_set_name(gtk_widget_get_accessible(widget), (tip + " · " + n.name).c_str());
        if (n.deleted || !n.open) {
            if (active == e->view.get())
                activate(nullptr);
            gtk_widget_hide(e->frame);
        } else {
            gtk_label_set_text(GTK_LABEL(e->title), n.name.c_str());
            gtk_widget_show_all(e->frame);
            arrange(*e);
            e->view->layoutPages();
            e->view->getViewFor(0)->rerenderPage(true);
        }
        thumbnails.erase(e->page->stableId);
    }
    updateFilter();
}
void FloatingNotes::lifecycle(const PageRef& p, bool deleted) {
    stash(p);
    auto b = *p->floatingNote;
    auto a = b;
    a.deleted = deleted;
    {
        std::unique_lock lock(*control->getDocument());
        p->floatingNote = a;
    }
    control->getUndoRedoHandler()->addUndoAction(std::make_unique<NoteUndo>(p, b, a));
    refresh();
}
void FloatingNotes::menu(Editor& e, GtkWidget* anchor) {
    activate(e.view.get());
    auto* popover = gtk_popover_new(anchor);
    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    g_signal_connect(popover, "hide", G_CALLBACK(+[](GtkWidget* w, gpointer) {
                         g_object_ref(w);
                         g_idle_add_full(
                                 G_PRIORITY_DEFAULT_IDLE,
                                 +[](gpointer d) -> gboolean {
                                     gtk_widget_destroy(GTK_WIDGET(d));
                                     return G_SOURCE_REMOVE;
                                 },
                                 w, g_object_unref);
                     }),
                     nullptr);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    auto* scroller = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scroller), 270);
    gtk_scrolled_window_set_max_content_height(
            GTK_SCROLLED_WINDOW(scroller),
            std::max(180, std::min(540, gtk_widget_get_allocated_height(GTK_WIDGET(overlay)) - 36)));
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scroller), true);
    gtk_container_add(GTK_CONTAINER(scroller), box);
    gtk_container_add(GTK_CONTAINER(popover), scroller);
    auto* name = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(name), e.page->floatingNote->name.c_str());
    atk_object_set_name(gtk_widget_get_accessible(name), "Tên tờ nháp");
    append(box, name);
    auto p = e.page;
    append(box, button("Đổi tên", "Đổi tên tờ nháp", [this, p, name, popover] {
               auto b = *p->floatingNote;
               auto a = b;
               a.name = gtk_entry_get_text(GTK_ENTRY(name));
               if (a.name.empty())
                   return;
               {
                   std::unique_lock lock(*control->getDocument());
                   p->floatingNote = a;
               }
               control->getUndoRedoHandler()->addUndoAction(std::make_unique<NoteUndo>(p, b, a));
               refresh();
               gtk_widget_hide(popover);
           }));
    append(box, button(p->floatingNote->pinned ? "Bỏ ghim" : "Ghim tờ", "Ghim tờ thường dùng", [this, p, popover] {
               auto b = *p->floatingNote;
               auto a = b;
               a.pinned = !b.pinned;
               {
                   std::unique_lock lock(*control->getDocument());
                   p->floatingNote = a;
               }
               control->getUndoRedoHandler()->addUndoAction(std::make_unique<NoteUndo>(p, b, a));
               refresh();
               gtk_widget_hide(popover);
           }));
    append(box, button("Nhân bản", "Nhân bản nội dung vector sang tờ độc lập", [this, p, popover] {
               gtk_widget_hide(popover);
               create(p);
           }));
    append(box, button("Về trang nguồn", "Quay lại trang đề gắn với tờ này", [this, p, popover] {
               gtk_widget_hide(popover);
               auto* d = control->getDocument();
               for (size_t i = 0; i < d->getPageCount(); ++i)
                   if (d->getPage(i)->stableId == p->floatingNote->sourceId) {
                       window->getXournal()->scrollTo(i);
                       return;
                   }
               XojMsgBox::showErrorToUser(control->getGtkWindow(),
                                          "Trang nguồn không còn. Nội dung tờ nháp vẫn được giữ.");
           }));
    append(box, button("Chép chọn → trang chính", "Copy vector vùng đang chọn sang trang chính", [this, p, popover] {
               gtk_widget_hide(popover);
               if (auto* editor = find(p)) {
                   if (!editor->view->getSelection())
                       return;
                   copySelectionToMain();
               }
           }));
    append(box, button("Xuất tờ PDF…", "Xuất toàn bộ giấy nháp thành PDF vector", [this, p, popover] {
               gtk_widget_hide(popover);
               exportNotes(p);
           }));
    for (auto item: {std::pair{"Giấy trắng", PageTypeFormat::Plain}, std::pair{"Giấy kẻ dòng", PageTypeFormat::Lined},
                     std::pair{"Giấy ô vuông", PageTypeFormat::Graph}}) {
        append(box, button(item.first, "Đổi nền giấy native, giữ nguyên mực", [this, p, popover, type = item.second] {
                   gtk_widget_hide(popover);
                   auto previous = p->getBackgroundType();
                   {
                       std::unique_lock lock(*control->getDocument());
                       p->setBackgroundType(PageType(type));
                   }
                   control->getUndoRedoHandler()->addUndoAction(std::make_unique<PaperUndo>(p, previous));
                   p->firePageChanged();
               }));
    }
    append(box,
           button("Xóa nội dung (hoàn tác được)", "Xóa toàn bộ nội dung tờ này — Ctrl+Z khôi phục", [this, p, popover] {
               gtk_widget_hide(popover);
               auto* e = find(p);
               e->view->finishInput();
               e->view->clearSelection();
               auto undo = std::make_unique<DeleteUndoAction>(p, false);
               bool any = false;
               {
                   std::unique_lock lock(*control->getDocument());
                   for (auto* layer: p->getLayers()) {
                       auto elements = layer->clearNoFree();
                       Element::Index i = 0;
                       for (auto& element: elements) {
                           undo->addElement(layer, std::move(element), i++);
                           any = true;
                       }
                   }
               }
               if (any) {
                   control->getUndoRedoHandler()->addUndoAction(std::move(undo));
                   e->view->getViewFor(0)->rerenderPage();
               }
           }));
    append(box, button("Xóa tờ (hoàn tác được)", "Xóa riêng tờ này — Ctrl+Z khôi phục", [this, p, popover] {
               gtk_widget_hide(popover);
               lifecycle(p, true);
           }));
    gtk_widget_show_all(popover);
}
void FloatingNotes::copySelectionToMain() {
    if (!active || !active->getSelection())
        return;
    auto* source = active->getSelection();
    std::vector<ElementPtr> copies;
    Range bounds;
    for (auto* e: source->getElementsView()) {
        copies.push_back(e->clone());
        bounds = bounds.unite(Range(e->getBoundingBox()));
    }
    if (copies.empty() || !bounds.isValid())
        return;
    auto* main = window->getXournal();
    auto* view = main->getViewFor(main->getCurrentPage());
    std::unique_ptr<xoj::util::Rectangle<double>> visible(main->getVisibleRect(main->getCurrentPage()));
    if (!view || !visible || visible->width < 30 || visible->height < 30)
        return;
    const double scale = std::min({1., (visible->width - 24) / std::max(1., bounds.getWidth()),
                                   (visible->height - 24) / std::max(1., bounds.getHeight())});
    const double x = visible->x + (visible->width - bounds.getWidth() * scale) / 2;
    const double y = visible->y + (visible->height - bounds.getHeight() * scale) / 2;
    for (auto& e: copies) {
        e->scale(bounds.minX, bounds.minY, scale, scale, 0, false);
        e->move(x - bounds.minX, y - bounds.minY);
    }
    activate(nullptr);  // Commits the original selection back to its own sheet.
    auto page = view->getPage();
    auto* layer = page->getSelectedLayer();
    std::vector<const Element*> refs;
    InsertionOrderRef selected;
    {
        std::unique_lock lock(*control->getDocument());
        Element::Index index = static_cast<Element::Index>(layer->getElements().size());
        for (auto& e: copies) {
            refs.push_back(e.get());
            selected.emplace_back(e.get(), index++);
            layer->addElement(std::move(e));
        }
    }
    control->getUndoRedoHandler()->addUndoAction(std::make_unique<InsertsUndoAction>(page, layer, std::move(refs)));
    control->selectTool(TOOL_SELECT_RECT);
    auto selection = SelectionFactory::createFromElementsOnActiveLayer(control, page, view, std::move(selected));
    main->setSelection(selection.release());
    page->firePageChanged();
    main->requestFocus();
}

void FloatingNotes::buildLibrary() {
    library = gtk_event_box_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(library), "floating-note");
    gtk_widget_set_halign(library, GTK_ALIGN_END);
    gtk_widget_set_valign(library, GTK_ALIGN_START);
    gtk_widget_set_margin_end(library, 12);
    gtk_widget_set_margin_top(library, 12);
    gtk_widget_set_size_request(
            library, std::max(180, std::min(650, gtk_widget_get_allocated_width(GTK_WIDGET(overlay)) - 24)),
            std::max(140, std::min(620, gtk_widget_get_allocated_height(GTK_WIDGET(overlay)) - 24)));
    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(library), box);
    auto* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    auto* heading = gtk_label_new("SỔ NHÁP · Thẻ ảnh");
    gtk_label_set_ellipsize(GTK_LABEL(heading), PANGO_ELLIPSIZE_END);
    append(bar, heading, true);
    append(bar, button("+ Tờ", "Tạo tờ mới — Ctrl+Alt+N", [this] { create(); }));
    auto* closeLibrary = button("⌄", "Cất kho nháp", [this] { toggleLibrary(); });
    gtk_widget_set_tooltip_text(closeLibrary, "Cất kho nháp — Alt+K");
    append(bar, closeLibrary);
    append(box, bar);
    // Keep the create/stash header reachable even below the body controls' minimum
    // height (small window at high DPI). The body then scrolls as a whole.
    auto* bodyScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(bodyScroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    append(box, bodyScroll, true);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(bodyScroll), box);
    search = gtk_search_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(search), 8);
    gtk_entry_set_placeholder_text(GTK_ENTRY(search), "Tìm tên tờ hoặc trang nguồn…");
    atk_object_set_name(gtk_widget_get_accessible(search), "Tìm tờ nháp");
    append(box, search);
    sort = gtk_combo_box_text_new();
    for (auto* t:
         {"Ghim trước · Mới tạo", "Ghim trước · Vừa cập nhật", "Ghim trước · Tên tờ", "Thùng rác · Chạm để phục hồi"})
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sort), t);
    auto* cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(sort));
    for (auto* cell = cells; cell; cell = cell->next)
        if (GTK_IS_CELL_RENDERER_TEXT(cell->data))
            g_object_set(cell->data, "ellipsize", PANGO_ELLIPSIZE_END, "width-chars", 10, nullptr);
    g_list_free(cells);
    gtk_combo_box_set_active(GTK_COMBO_BOX(sort), 0);
    atk_object_set_name(gtk_widget_get_accessible(sort), "Sắp tờ nháp");
    append(box, sort);
    g_signal_connect(search, "search-changed",
                     G_CALLBACK(+[](GtkWidget*, gpointer p) { static_cast<FloatingNotes*>(p)->updateFilter(); }), this);
    g_signal_connect(sort, "changed",
                     G_CALLBACK(+[](GtkWidget*, gpointer p) { static_cast<FloatingNotes*>(p)->updateFilter(); }), this);
    galleryScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(galleryScroll), 205);
    gtk_widget_set_vexpand(galleryScroll, true);
    append(box, galleryScroll, true);
    gallery = gtk_drawing_area_new();
    gtk_widget_set_can_focus(gallery, true);
    atk_object_set_name(gtk_widget_get_accessible(gallery), "Kho nháp dạng ảnh — chạm mở; mũi tên chọn; Enter mở");
    gtk_widget_add_events(gallery, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK);
    g_signal_connect(gallery, "button-press-event",
                     G_CALLBACK(+[](GtkWidget* w, GdkEventButton*, gpointer) -> gboolean {
                         gtk_widget_grab_focus(w);
                         return true;
                     }),
                     nullptr);
    auto* galleryHost = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(galleryHost), gallery);
    galleryHits = gtk_fixed_new();
    gtk_overlay_add_overlay(GTK_OVERLAY(galleryHost), galleryHits);
    gtk_container_add(GTK_CONTAINER(galleryScroll), galleryHost);
    g_signal_connect(
            gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(galleryScroll)), "value-changed",
            G_CALLBACK(+[](GtkAdjustment*, gpointer p) { static_cast<FloatingNotes*>(p)->bindVisibleCards(); }), this);
    g_signal_connect(gallery, "size-allocate", G_CALLBACK(+[](GtkWidget* w, GtkAllocation* a, gpointer p) {
                         auto* self = static_cast<FloatingNotes*>(p);
                         int cols = std::max(1, a->width / 190);
                         int requested = std::max(205, int((self->filtered.size() + cols - 1) / cols) * 205);
                         int old = 0;
                         gtk_widget_get_size_request(w, nullptr, &old);
                         if (old != requested)
                             gtk_widget_set_size_request(w, -1, requested);
                         self->bindVisibleCards();
                     }),
                     this);
    g_signal_connect(gallery, "draw", G_CALLBACK(+[](GtkWidget*, cairo_t* cr, gpointer p) -> gboolean {
                         return static_cast<FloatingNotes*>(p)->drawGallery(cr);
                     }),
                     this);
    g_signal_connect(gallery, "button-release-event",
                     G_CALLBACK(+[](GtkWidget* w, GdkEventButton* ev, gpointer p) -> gboolean {
                         auto* self = static_cast<FloatingNotes*>(p);
                         const int cols = std::max(1, gtk_widget_get_allocated_width(w) / 190);
                         const int cell = gtk_widget_get_allocated_width(w) / cols;
                         const size_t idx = int(ev->y / 205) * cols + std::min(cols - 1, int(ev->x / cell));
                         if (idx >= self->filtered.size())
                             return true;
                         auto page = self->filtered[idx];
                         self->selected = page;
                         if (page->floatingNote->deleted)
                             self->lifecycle(page, false);
                         else
                             self->open(page);
                         return true;
                     }),
                     this);
    g_signal_connect(gallery, "key-press-event", G_CALLBACK(+[](GtkWidget* w, GdkEventKey* ev, gpointer p) -> gboolean {
                         auto* self = static_cast<FloatingNotes*>(p);
                         if (self->filtered.empty())
                             return false;
                         auto it = std::find(self->filtered.begin(), self->filtered.end(), self->selected);
                         int i = it == self->filtered.end() ? 0 : int(it - self->filtered.begin());
                         int cols = std::max(1, gtk_widget_get_allocated_width(w) / 190);
                         if (ev->keyval == GDK_KEY_Return) {
                             self->open(self->filtered[i]);
                             return true;
                         }
                         if (ev->keyval == GDK_KEY_Right)
                             ++i;
                         else if (ev->keyval == GDK_KEY_Left)
                             --i;
                         else if (ev->keyval == GDK_KEY_Down)
                             i += cols;
                         else if (ev->keyval == GDK_KEY_Up)
                             i -= cols;
                         else
                             return false;
                         self->selected = self->filtered[std::clamp(i, 0, int(self->filtered.size() - 1))];
                         atk_object_set_name(gtk_widget_get_accessible(w),
                                             ("Đã chọn: " + self->selected->floatingNote->name).c_str());
                         gtk_widget_queue_draw(w);
                         return true;
                     }),
                     this);
    status = gtk_label_new("");
    gtk_label_set_ellipsize(GTK_LABEL(status), PANGO_ELLIPSIZE_END);
    gtk_label_set_line_wrap(GTK_LABEL(status), true);
    append(box, status);
    append(box, button("Xuất cả sổ PDF…", "Xuất tất cả tờ chưa xóa thành PDF vector", [this] { exportNotes(); }));
    gtk_overlay_add_overlay(overlay, library);
}
void FloatingNotes::showLibrary() {
    if (!library)
        buildLibrary();
    updateFilter();
    gtk_widget_show_all(library);
    gtk_overlay_reorder_overlay(overlay, library, -1);
    if (gtk_widget_get_realized(library))
        gdk_window_raise(gtk_widget_get_window(library));
}
void FloatingNotes::toggleLibrary() {
    if (library && gtk_widget_get_visible(library)) {
        gtk_widget_hide(library);
        control->getEditingView()->requestFocus();
    } else {
        showLibrary();
        gtk_widget_grab_focus(search);
    }
}
void FloatingNotes::updateFilter() {
    if (!gallery)
        return;
    const auto* q = gtk_entry_get_text(GTK_ENTRY(search));
    auto* query = g_utf8_casefold(q, -1);
    int order = gtk_combo_box_get_active(GTK_COMBO_BOX(sort));
    filtered.clear();
    for (auto& p: control->getDocument()->floatingNotes) {
        auto& n = *p->floatingNote;
        if (n.deleted != (order == 3))
            continue;
        std::string source;
        auto* doc = control->getDocument();
        for (size_t i = 0; i < doc->getPageCount(); ++i)
            if (doc->getPage(i)->stableId == n.sourceId)
                source = "trang " + std::to_string(i + 1);
        auto* hay = g_utf8_casefold((n.name + " " + source).c_str(), -1);
        bool match = std::string(hay).find(query) != std::string::npos;
        g_free(hay);
        if (match)
            filtered.push_back(p);
    }
    g_free(query);
    std::stable_sort(filtered.begin(), filtered.end(), [order](auto& a, auto& b) {
        auto& x = *a->floatingNote;
        auto& y = *b->floatingNote;
        if (x.pinned != y.pinned)
            return x.pinned;
        if (order == 1)
            return x.modified > y.modified;
        if (order == 2)
            return x.name < y.name;
        return x.created < y.created;
    });
    int cols = std::max(1, gtk_widget_get_allocated_width(gallery) / 190);
    gtk_widget_set_size_request(gallery, -1, std::max(205, int((filtered.size() + cols - 1) / cols) * 205));
    std::string text = filtered.empty() ?
                               "Chưa có tờ phù hợp. Bấm + Tờ để bắt đầu." :
                               std::to_string(filtered.size()) + " tờ · Chạm ảnh để mở · Tối đa 6 khung viết đồng thời";
    gtk_label_set_text(GTK_LABEL(status), text.c_str());
    bindVisibleCards();
    gtk_widget_queue_draw(gallery);
}
void FloatingNotes::bindVisibleCards() {
    if (!galleryHits || !galleryScroll)
        return;
    const int width = gtk_widget_get_allocated_width(gallery);
    const int cols = std::max(1, width / 190), cell = std::max(1, width / cols);
    auto* adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(galleryScroll));
    const double top = gtk_adjustment_get_value(adjustment);
    const double height = std::max(1., gtk_adjustment_get_page_size(adjustment));
    const size_t first = size_t(std::max(0., std::floor(top / 205))) * cols;
    const size_t last = std::min(filtered.size(), size_t(std::ceil((top + height) / 205)) * cols);
    const size_t count = first < last ? last - first : 0;
    for (size_t j = cardButtons.size(); j < count; ++j) {
        auto* b = gtk_button_new();
        gtk_widget_set_no_show_all(b, true);  // Only the currently bound visible card slots may be shown.
        gtk_style_context_add_class(gtk_widget_get_style_context(b), "gallery-hit");
        gtk_fixed_put(GTK_FIXED(galleryHits), b, 0, 0);
        cardButtons.push_back(b);
        g_signal_connect(b, "clicked", G_CALLBACK(+[](GtkWidget* w, gpointer p) {
                             auto* self = static_cast<FloatingNotes*>(p);
                             auto* stored = static_cast<PageRef*>(g_object_get_data(G_OBJECT(w), "note-page"));
                             if (!stored)
                                 return;
                             auto page = *stored;
                             self->selected = page;
                             if (page->floatingNote->deleted)
                                 self->lifecycle(page, false);
                             else
                                 self->open(page);
                         }),
                         this);
        g_signal_connect(b, "focus-in-event", G_CALLBACK(+[](GtkWidget* w, GdkEventFocus*, gpointer p) -> gboolean {
                             auto* self = static_cast<FloatingNotes*>(p);
                             auto* stored = static_cast<PageRef*>(g_object_get_data(G_OBJECT(w), "note-page"));
                             if (stored)
                                 self->selected = *stored;
                             gtk_widget_queue_draw(self->gallery);
                             return false;
                         }),
                         this);
    }
    for (size_t j = 0; j < cardButtons.size(); ++j) {
        auto* b = cardButtons[j];
        if (j >= count) {
            gtk_widget_hide(b);
            continue;
        }
        const size_t i = first + j;
        auto page = filtered[i];
        const auto& n = *page->floatingNote;
        g_object_set_data_full(
                G_OBJECT(b), "note-page", new PageRef(page), +[](gpointer p) { delete static_cast<PageRef*>(p); });
        std::string name = n.name + " · " +
                           (n.deleted ? "Chạm phục hồi" :
                            n.open    ? "Đang mở" :
                                        "Đã cất") +
                           (n.pinned ? " · Ghim" : "");
        atk_object_set_name(gtk_widget_get_accessible(b), name.c_str());
        gtk_widget_set_tooltip_text(b, name.c_str());
        gtk_widget_set_size_request(b, std::max(1, cell - 12), 193);
        gtk_fixed_move(GTK_FIXED(galleryHits), b, int(i % cols) * cell + 6, int(i / cols) * 205 + 6);
        gtk_widget_show(b);
    }
}
bool FloatingNotes::drawGallery(cairo_t* cr) {
    const int width = gtk_widget_get_allocated_width(gallery);
    const int cols = std::max(1, width / 190);
    const int cell = width / cols;
    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    GdkRGBA fg{}, bg{};
    auto* style = gtk_widget_get_style_context(gallery);
    gtk_style_context_get_color(style, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_lookup_color(style, "theme_bg_color", &bg);
    for (size_t i = size_t(std::max(0., std::floor(y1 / 205))) * cols;
         i < filtered.size() && double(i / cols) * 205 < y2; ++i) {
        auto p = filtered[i];
        auto& n = *p->floatingNote;
        double x = (i % cols) * cell + 6, y = (i / cols) * 205 + 6;
        cairo_save(cr);
        cairo_rectangle(cr, x, y, cell - 12, 193);
        gdk_cairo_set_source_rgba(cr, &bg);
        cairo_fill_preserve(cr);
        if (p == selected)
            cairo_set_source_rgb(cr, .15, .45, .9);
        else
            cairo_set_source_rgba(cr, fg.red, fg.green, fg.blue, .35);
        cairo_set_line_width(cr, p == selected ? 3 : 1);
        cairo_stroke(cr);
        const int tw = std::max(1, cell - 28);
        auto it = thumbnails.find(p->stableId);
        const bool dark = window->isDarkTheme();
        if (it == thumbnails.end() || it->second.revision != n.revision || it->second.width != tw ||
            it->second.dark != dark) {
            if (std::none_of(pendingThumbnails.begin(), pendingThumbnails.end(), [&](auto& q) {
                    return q.page == p && q.revision == n.revision && q.width == tw && q.dark == dark;
                })) {
                pendingThumbnails.erase(std::remove_if(pendingThumbnails.begin(), pendingThumbnails.end(),
                                                       [&](auto& q) { return q.page == p; }),
                                        pendingThumbnails.end());
                if (pendingThumbnails.size() >= 64)
                    pendingThumbnails.erase(pendingThumbnails.begin());
                pendingThumbnails.push_back({p, n.revision, tw, dark});
            }
            if (!thumbnailSource)
                thumbnailSource = g_idle_add_full(
                        G_PRIORITY_LOW,
                        +[](gpointer d) -> gboolean {
                            auto* self = static_cast<FloatingNotes*>(d);
                            if (!self->renderNextThumbnail()) {
                                self->thumbnailSource = 0;
                                return G_SOURCE_REMOVE;
                            }
                            return G_SOURCE_CONTINUE;
                        },
                        this, nullptr);
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_rectangle(cr, x + 8, y + 8, tw, 125);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, .35, .4, .45);
            label(cr, "Đang tạo ảnh…", x + 16, y + 50, tw - 16, 12);
        } else if (it->second.image) {
            it->second.used = ++thumbnailClock;
            cairo_set_source_surface(cr, it->second.image.get(), x + 8, y + 8);
            cairo_paint(cr);
        } else {
            gdk_cairo_set_source_rgba(cr, &fg);
            label(cr, "Mở tờ để xem nội dung", x + 10, y + 50, tw, 12);
        }
        gdk_cairo_set_source_rgba(cr, &fg);
        label(cr, (n.pinned ? "◆ " : "") + n.name, x + 8, y + 138, cell - 30, 13, true);
        std::string source = "Nguồn đã xóa";
        auto* doc = control->getDocument();
        for (size_t j = 0; j < doc->getPageCount(); ++j)
            if (doc->getPage(j)->stableId == n.sourceId)
                source = "Trang " + std::to_string(j + 1);
        source += " · " + std::to_string(int(std::ceil(p->getHeight() / 840))) + " đoạn";
        label(cr, source, x + 8, y + 158, cell - 30, 11);
        auto* editor = find(p);
        bool empty = !p->isAnnotated() && !(editor && editor->view->getSelection());
        label(cr,
              (p == selected ? "✓ " : "") +
                      std::string(n.deleted ? "Đã xóa · Chạm phục hồi" :
                                  n.open    ? "Đang mở" :
                                  empty     ? "Tờ trống" :
                                              "Đã cất") +
                      (control->getUndoRedoHandler()->isPageChanged(p) ? " · *" : ""),
              x + 8, y + 176, cell - 30, 10);
        cairo_restore(cr);
    }
    return false;
}
bool FloatingNotes::renderNextThumbnail() {
    if (pendingThumbnails.empty())
        return false;
    auto request = pendingThumbnails.front();
    pendingThumbnails.erase(pendingThumbnails.begin());
    auto p = request.page;
    if (!p->floatingNote || p->floatingNote->revision != request.revision ||
        std::find(control->getDocument()->floatingNotes.begin(), control->getDocument()->floatingNotes.end(), p) ==
                control->getDocument()->floatingNotes.end())
        return !pendingThumbnails.empty();
    auto image = xoj::util::CairoSurfaceSPtr(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, request.width, 125),
                                             xoj::util::adopt);
    if (cairo_surface_status(image.get()) == CAIRO_STATUS_SUCCESS) {
        auto* cr = cairo_create(image.get());
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        std::shared_lock lock(*control->getDocument());
        double minY = p->getHeight();
        for (auto* layer: p->getLayers())
            for (const auto& e: layer->getElements()) minY = std::min(minY, e->getBoundingBox().y);
        auto* editor = find(p);
        auto* selection = editor ? editor->view->getSelection() : nullptr;
        if (selection && selection->getSourcePage() == p)
            minY = std::min(minY, selection->getYOnView());
        double top = minY < p->getHeight() ? std::max(0., minY - 16) : 0;
        double zoom = double(request.width) / p->getWidth();
        cairo_scale(cr, zoom, zoom);
        cairo_translate(cr, 0, -top);
        DocumentView renderer;
        renderer.drawPage(p, cr, true);
        if (selection && selection->getSourcePage() == p) {
            xoj::view::ElementContainerView selected(selection);
            selected.draw(xoj::view::Context::createDefault(cr));
        }
        cairo_destroy(cr);
    } else
        image.reset();
    if (thumbnails.size() >= 64 && thumbnails.find(p->stableId) == thumbnails.end()) {
        auto oldest = std::min_element(thumbnails.begin(), thumbnails.end(),
                                       [](auto& a, auto& b) { return a.second.used < b.second.used; });
        thumbnails.erase(oldest);
    }
    thumbnails.insert_or_assign(
            p->stableId, Thumb{request.revision, ++thumbnailClock, request.width, request.dark, std::move(image)});
    if (gallery)
        gtk_widget_queue_draw(gallery);
    return !pendingThumbnails.empty();
}
void FloatingNotes::documentChanged(DocumentChangeType type) {
    if (type != DOCUMENT_CHANGE_COMPLETE && type != DOCUMENT_CHANGE_CLEARED)
        return;
    if (refreshSource) {
        g_source_remove(refreshSource);
        refreshSource = 0;
    }
    if (thumbnailSource) {
        g_source_remove(thumbnailSource);
        thumbnailSource = 0;
    }
    pendingThumbnails.clear();
    pendingChanges.clear();
    rebuilding = true;
    active = nullptr;
    editors.clear();
    rebuilding = false;
    selected.reset();
    lastPenPosition.reset();
    thumbnails.clear();
    filtered.clear();
    hiddenGroup.clear();
    if (library)
        gtk_widget_hide(library);
    if (type == DOCUMENT_CHANGE_COMPLETE) {
        auto pages = control->getDocument()->floatingNotes;
        std::stable_sort(pages.begin(), pages.end(),
                         [](auto& a, auto& b) { return a->floatingNote->zOrder < b->floatingNote->zOrder; });
        for (auto& p: pages)
            if (p->floatingNote->open && !p->floatingNote->deleted)
                open(p);
    }
}
bool FloatingNotes::handleKey(GdkEventKey* e) {
    return e->type == GDK_KEY_PRESS ? keyPress(nullptr, e, this) : keyRelease(nullptr, e, this);
}
gboolean FloatingNotes::keyPress(GtkWidget*, GdkEventKey* e, gpointer p) {
    auto* self = static_cast<FloatingNotes*>(p);
    const auto mods = e->state & gtk_accelerator_get_default_mod_mask();
    const auto key = gdk_keyval_to_lower(e->keyval);
    if (key != GDK_KEY_n && key != GDK_KEY_k)
        return false;
    for (size_t i = 0; i < dockAccelerators.size(); ++i) {
        guint shortcutKey = 0;
        GdkModifierType shortcutMods{};
        gtk_accelerator_parse(dockAccelerators[i], &shortcutKey, &shortcutMods);
        if (key != shortcutKey || mods != shortcutMods)
            continue;
        bool& down = self->shortcutDown[i];
        if (!down) {
            down = true;
            switch (i) {
                case ToggleNotes:
                    self->toggle();
                    break;
                case CreateNote:
                    self->create();
                    break;
                case ToggleLibrary:
                    self->toggleLibrary();
                    break;
            }
        }
        return true;
    }
    return false;
}
gboolean FloatingNotes::keyRelease(GtkWidget*, GdkEventKey* e, gpointer p) {
    auto* self = static_cast<FloatingNotes*>(p);
    const auto key = gdk_keyval_to_lower(e->keyval);
    if (key == GDK_KEY_n)
        self->shortcutDown[ToggleNotes] = self->shortcutDown[CreateNote] = false;
    if (key == GDK_KEY_k)
        self->shortcutDown[ToggleLibrary] = false;
    return false;
}
void FloatingNotes::exportNotes(const PageRef& one) {
    struct Request {
        std::weak_ptr<int> alive;
        FloatingNotes* self;
        PageRef page;
    };
    auto* request = new Request{aliveToken, this, one};
    g_idle_add_full(
            G_PRIORITY_DEFAULT_IDLE,
            +[](gpointer p) -> gboolean {
                auto* r = static_cast<Request*>(p);
                if (!r->alive.expired())
                    r->self->doExportNotes(r->page);
                return G_SOURCE_REMOVE;
            },
            request, +[](gpointer p) { delete static_cast<Request*>(p); });
}
void FloatingNotes::doExportNotes(const PageRef& one) {
    for (auto& e: editors) {
        e->view->finishInput();
        e->view->clearSelection();
    }
    flush();
    auto* dialog =
            gtk_file_chooser_dialog_new("Xuất nháp PDF vector", control->getGtkWindow(), GTK_FILE_CHOOSER_ACTION_SAVE,
                                        "Hủy", GTK_RESPONSE_CANCEL, "Xuất", GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "so-nhap.pdf");
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        auto* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        auto error = floating_notebook::exportPdf(*control->getDocument(), filename, one);
        g_free(filename);
        if (!error.empty())
            XojMsgBox::showErrorToUser(control->getGtkWindow(), error);
    }
    gtk_widget_destroy(dialog);
}
