// SPDX-License-Identifier: GPL-2.0-or-later
#include "TeachingToolsDialog.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include "control/Control.h"
#include "control/ToolHandler.h"
#include "control/actions/ActionDatabase.h"
#include "control/settings/Settings.h"
#include "control/tools/EditSelection.h"
#include "control/tools/ElementSnapper.h"
#include "control/tools/PdfElemSelection.h"
#include "gui/MainWindow.h"
#include "gui/PdfFloatingToolbox.h"
#include "gui/XournalView.h"
#include "math/CoordinateMath.h"
#include "math/FormulaImport.h"
#include "math/FormulaRender.h"
#include "math/PdfFormulaJob.h"
#include "model/Document.h"
#include "model/Font.h"
#include "model/Layer.h"
#include "model/LineStyle.h"
#include "model/Stroke.h"
#include "model/TexImage.h"
#include "model/Text.h"
#include "model/XojPage.h"
#include "undo/InsertUndoAction.h"
#include "undo/UndoRedoHandler.h"
#include "util/Rectangle.h"

namespace teaching {
namespace {
void rgb(cairo_t* cr, uint32_t c) {
    cairo_set_source_rgb(cr, ((c >> 16) & 255) / 255., ((c >> 8) & 255) / 255., (c & 255) / 255.);
}
void paint(cairo_t* cr, const Figure& f, bool skipCurveHeadings = false) {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    for (const auto& s: f.strokes) {
        rgb(cr, s.color);
        cairo_set_line_width(cr, s.width);
        double dash[] = {4, 3};
        cairo_set_dash(cr, dash, s.dashed ? 2 : 0, 0);
        bool first = true;
        for (auto p: s.points) {
            if (first)
                cairo_move_to(cr, p.x, p.y);
            else
                cairo_line_to(cr, p.x, p.y);
            first = false;
        }
        cairo_stroke(cr);
    }
    cairo_set_dash(cr, nullptr, 0, 0);
    for (const auto& l: f.labels) {
        if (skipCurveHeadings && !l.formula.empty() && l.pos.y < 76)
            continue;
        rgb(cr, l.color);
        if (!l.formula.empty() || l.notation) {
            cairo_save(cr);
            cairo_translate(cr, l.pos.x, l.pos.y);
            if (l.notation)
                paintFormula(cr, l.notation, l.size);
            else
                paintFormula(cr, l.formula, l.size);
            cairo_restore(cr);
            continue;
        }
        auto* layout = pango_cairo_create_layout(cr);
        auto* font = pango_font_description_from_string("Sans");
        pango_font_description_set_absolute_size(font, l.size * PANGO_SCALE);
        pango_layout_set_font_description(layout, font);
        pango_layout_set_text(layout, l.text.c_str(), -1);
        cairo_move_to(cr, l.pos.x, l.pos.y);
        pango_cairo_show_layout(cr, layout);
        pango_font_description_free(font);
        g_object_unref(layout);
    }
}
struct Area {
    double x, y, width, height;
};
Area visible(Control* c) {
    auto v = std::unique_ptr<xoj::util::Rectangle<double>>(
            c->getEditingView()->getVisibleRect(c->getEditingView()->getCurrentPage()));
    if (!v || v->width < 270 || v->height < 185)
        throw std::runtime_error("Vùng trang đang thấy quá nhỏ. Giảm zoom để có ít nhất 270 × 185 pt.");
    double w = std::min(560., v->width - 24), h = std::min(400., v->height - 24);
    return {v->x + (v->width - w) / 2, v->y + (v->height - h) / 2, w, h};
}
// Fit every Pango label inside its assigned page bounds. Computed using the same
// font metrics used by the editor, rather than estimating byte/string lengths.
void fitLabels(Figure& f) {
    auto* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    auto* cr = cairo_create(surface);
    struct Box {
        double x, y, w, h;
    };
    std::vector<Box> occupied;
    for (auto& l: f.labels) {
        auto* layout = pango_cairo_create_layout(cr);
        auto* font = pango_font_description_from_string("Sans");
        for (int i = 0; i < 5; ++i) {
            pango_font_description_set_absolute_size(font, l.size * PANGO_SCALE);
            pango_layout_set_font_description(layout, font);
            pango_layout_set_text(layout, l.text.c_str(), -1);
            int w, h;
            pango_layout_get_pixel_size(layout, &w, &h);
            if (!l.formula.empty() || l.notation) {
                auto m = l.notation ? formulaMetrics(l.notation, l.size) : formulaMetrics(l.formula, l.size);
                w = static_cast<int>(std::ceil(m.width));
                h = static_cast<int>(std::ceil(m.height));
            }
            double available = l.maxWidth > 0 ? std::min(l.maxWidth, f.width - 12) : f.width - 12;
            if (w > available) {
                l.size *= available / w;
                if (l.size < 4) {
                    pango_font_description_free(font);
                    g_object_unref(layout);
                    cairo_destroy(cr);
                    cairo_surface_destroy(surface);
                    throw std::runtime_error("Nhãn quá dài để đọc; rút ngắn tên hàng/công thức hoặc bớt cột.");
                }
                continue;
            }
            double x = l.pos.x - (l.anchor == TextAnchor::Center ? w / 2. : l.anchor == TextAnchor::Right ? w : 0);
            double y = l.pos.y;
            auto clamp = [&](double xx, double yy) {
                return Box{std::clamp(xx, 6., std::max(6., f.width - w - 6)),
                           std::clamp(yy, 4., std::max(4., f.height - h - 4)), static_cast<double>(w),
                           static_cast<double>(h)};
            };
            auto clear = [&](const Box& b) {
                for (const auto& a: occupied)
                    if (b.x < a.x + a.w + 2 && b.x + b.w + 2 > a.x && b.y < a.y + a.h + 2 && b.y + b.h + 2 > a.y)
                        return false;
                return true;
            };
            Box box = clamp(x, y);
            bool placed = false;
            for (auto delta: {Point{0, 0}, Point{0, static_cast<double>(h + 4)}, Point{0, static_cast<double>(-h - 4)},
                              Point{0, static_cast<double>(2 * h + 8)}, Point{0, static_cast<double>(-2 * h - 8)},
                              Point{static_cast<double>(w + 4), 0}, Point{static_cast<double>(-w - 4), 0}}) {
                auto candidate = clamp(x + delta.x, y + delta.y);
                if (clear(candidate)) {
                    box = candidate;
                    placed = true;
                    break;
                }
            }
            if (!placed) {
                pango_font_description_free(font);
                g_object_unref(layout);
                cairo_destroy(cr);
                cairo_surface_destroy(surface);
                throw std::runtime_error("Nhãn quá chật; giảm số mốc/độ dài nhãn hoặc giảm zoom.");
            }
            l.pos = {box.x, box.y};
            l.anchor = TextAnchor::Left;
            occupied.push_back(box);
            break;
        }
        pango_font_description_free(font);
        g_object_unref(layout);
    }
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}
}  // namespace
void insertFigure(Control* c, Figure f) {
    auto page = c->getEditingPage();
    if (!page || !page->getSelectedLayer())
        throw std::runtime_error("Không có trang/lớp đang hoạt động.");
    auto box = visible(c);
    fitLabels(f);
    std::vector<std::unique_ptr<Element>> elements;
    for (const auto& path: f.strokes) {
        if (path.points.size() < 2)
            continue;
        auto s = std::make_unique<::Stroke>();
        s->setColor(Color(path.color));
        s->setWidth(path.width);
        if (path.dashed) {
            LineStyle style;
            style.setDashes({4, 3});
            s->setLineStyle(style);
        }
        for (auto p: path.points) s->addPoint(::Point(p.x + box.x, p.y + box.y));
        elements.push_back(std::move(s));
    }
    for (const auto& l: f.labels) {
        if (!l.formula.empty() || l.notation) {
            auto t = std::make_unique<TexImage>();
            if (!t->loadData(l.notation ? formulaPdf(l.notation, l.size, l.color) :
                                          formulaPdf(l.formula, l.size, l.color)))
                throw std::runtime_error("Không tạo được công thức vector.");
            // Store safe TeX generated from AST, so native LaTeX editing remains available.
            t->setText(l.notation ? notationLatex(l.notation) : formulaLatex(l.formula));
            t->move(l.pos.x + box.x, l.pos.y + box.y);
            elements.push_back(std::move(t));
            continue;
        }
        auto t = std::make_unique<Text>();
        XojFont font;
        font.setName("Sans");
        font.setSize(l.size);
        t->setFont(font);
        t->setColor(Color(l.color));
        t->setText(l.text);
        t->move(l.pos.x + box.x, l.pos.y + box.y);
        elements.push_back(std::move(t));
    }
    // Do all fallible parsing/generation before touching the document or selection.
    c->clearSelectionEndText();
    auto* layer = page->getSelectedLayer();
    std::vector<const Element*> refs;
    InsertionOrderRef selected;
    {
        std::lock_guard lock(*c->getDocument());
        Element::Index index = static_cast<Element::Index>(layer->getElements().size());
        for (auto& e: elements) {
            refs.push_back(e.get());
            selected.emplace_back(e.get(), index++);
            layer->addElement(std::move(e));
        }
    }
    c->getUndoRedoHandler()->addUndoAction(std::make_unique<InsertsUndoAction>(page, layer, std::move(refs)));
    page->firePageChanged();
    c->getActionDatabase()->fireChangeActionState(Action::SELECT_TOOL, TOOL_SELECT_RECT);
    auto* view = c->getEditingView();
    auto selection = SelectionFactory::createFromElementsOnActiveLayer(c, page, view->getViewFor(c->getEditingView()->getCurrentPage()),
                                                                       std::move(selected));
    view->setSelection(selection.release());
}
void insert(Control* c, const Request& r) {
    auto box = visible(c);
    insertFigure(c, generate(r, box.width, box.height));
}
namespace {
struct Dialog {
    Control* c;
    GtkWidget *win, *preview, *error, *mode, *preset, *m, *enabled[2], *colors[2], *equal, *pi, *showSign, *showValues,
            *insertButton, *graphBox, *tableBox, *tableGrid, *addColumn, *removeColumn, *x0Label, *tableScroll,
            *formulaOnly;
    GtkWidget *curveEditor, *graphSettings, *parameterBox, *syntaxBox, *sourceFrame, *titleText, *stepsText;
    GtkWidget *coordinateBox, *coordinateDimension, *coordinatePreset, *coordinateSource, *coordinateOperation,
            *coordinateRule, *coordinateEquation, *forceInput, *coordinateComponents, *coordinatePick,
            *coordinateReport;
    std::map<std::string, GtkWidget*> coordinateRows;
    std::vector<std::pair<Point, coordinate::Vec>> coordinateAnchors;
    GtkWidget* curveRows[2];
    std::map<std::string, GtkWidget*> entries;
    std::array<std::vector<GtkWidget*>, 3> tableCells;
    GtkWidget* symbolTarget = nullptr;
    bool syncTableBusy = false, tableNeedsRebuild = true;
    Figure figure;
    bool valid = false, loading = true;
    guint timer = 0;
    std::string path;
    enum class SourceState { Waiting, Running, Review, Ready, Error, Cancelled };
    SourceState sourceState = SourceState::Waiting;
    GtkWidget *sourceStatus, *sourceImage, *candidateCombo, *candidateEdit, *recognizeButton, *confirmSource,
            *stopRecognitionButton;
    GtkWidget* applyCandidate[2];
    GtkWidget *sourceCard, *sourceHeading, *equationCard, *equationPreview, *fullSource, *graphHeading;
    GtkCssProvider* css = nullptr;
    std::vector<std::string> equationStrings;
    std::vector<uint32_t> equationColors;
    bool firstSourceApplication = false;
    std::string recognitionWarning;
    GdkPixbuf* fullCrop = nullptr;
    std::vector<FormulaCandidate> candidates;
    int activeCandidate = -1;
    bool selectingCandidate = false, imported = false, sourceApplied = false;
    GdkPixbuf* crop = nullptr;
    PdfFormulaSnapshot source;
    PageRef targetPage;
    size_t targetGeneration = 0;
    Layer* targetLayer = nullptr;
    double targetZoom = 1;
    guint contextTimer = 0, ocrTimeout = 0;
    unsigned generation = 0, previewGeneration = 0;
    bool previewBusy = false, previewPending = false, previewGraphOnly = false;
    std::shared_ptr<PdfFormulaJob> job;
    struct Lifetime {
        Dialog* owner;
    };
    std::shared_ptr<Lifetime> lifetime = std::make_shared<Lifetime>(Lifetime{this});
    explicit Dialog(Control* control): c(control) {
        targetPage = c->getEditingPage();
        targetGeneration = c->getEditingGeneration();
        targetLayer = targetPage ? targetPage->getSelectedLayer() : nullptr;
        targetZoom = c->getEditingView()->getZoom();
        path = std::string(g_get_user_config_dir()) + "/xournalpp/math-tools.ini";
        win = gtk_dialog_new_with_buttons("Toán THPT · Đồ thị, tiếp tuyến, bảng",
                                          GTK_WINDOW(c->getWindow()->getWindow()), GTK_DIALOG_MODAL, "Hủy",
                                          GTK_RESPONSE_CANCEL, "Chèn cả cụm", GTK_RESPONSE_ACCEPT, nullptr);
        gtk_widget_set_name(win, "teaching-tools-dialog");
        int dialogWidth = 1160, dialogHeight = 850;
        if (auto* monitor = gdk_display_get_monitor_at_window(gtk_widget_get_display(win),
                                                              gtk_widget_get_window(c->getWindow()->getWindow()))) {
            GdkRectangle area;
            gdk_monitor_get_workarea(monitor, &area);
            dialogWidth = std::min(dialogWidth, area.width - 40);
            dialogHeight = std::min(dialogHeight, area.height - 40);
        }
        gtk_window_set_default_size(GTK_WINDOW(win), dialogWidth, dialogHeight);
        styleDialog();
        insertButton = gtk_dialog_get_widget_for_response(GTK_DIALOG(win), GTK_RESPONSE_ACCEPT);
        gtk_widget_set_size_request(insertButton, 150, 44);
        auto* content = gtk_dialog_get_content_area(GTK_DIALOG(win));
        auto* masthead = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_container_set_border_width(GTK_CONTAINER(masthead), 16);
        auto* title = titleText = gtk_label_new("Công thức & đồ thị");
        gtk_label_set_xalign(GTK_LABEL(title), 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "math-title");
        gtk_box_pack_start(GTK_BOX(masthead), title, false, false, 0);
        auto* steps = stepsText = gtk_label_new("QUÉT VÙNG PDF   →   ĐỐI CHIẾU CÔNG THỨC   →   CHÈN VÀO TRANG");
        gtk_label_set_xalign(GTK_LABEL(steps), 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(steps), "math-steps");
        gtk_box_pack_start(GTK_BOX(masthead), steps, false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), masthead, false, false, 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(insertButton), "math-primary");
        auto* body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
        gtk_container_set_border_width(GTK_CONTAINER(body), 12);
        gtk_box_pack_start(GTK_BOX(content), body, true, true, 0);
        auto* scroll = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(scroll, 420, -1);
        gtk_box_pack_start(GTK_BOX(body), scroll, false, false, 0);
        auto* form = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_add(GTK_CONTAINER(scroll), form);
        gtk_container_set_border_width(GTK_CONTAINER(form), 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(scroll), "math-panel");
        mode = gtk_combo_box_text_new();
        for (auto s:
             {"Đồ thị", "Điểm & tiếp tuyến (hàm 1)", "Bảng biến thiên / xét dấu", "Vectơ", "Đường thẳng", "Hợp lực"})
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mode), s);
        gtk_combo_box_set_active(GTK_COMBO_BOX(mode), 0);
        pack(form, mode, "math-mode");
        atk_object_set_name(gtk_widget_get_accessible(mode), "math-mode");
        coordinateControls(form);
        preset = gtk_combo_box_text_new();
        for (auto s:
             {"Mẫu gần nhất", "Bậc hai & tham số m", "Bậc ba", "Phân thức bậc nhất / bậc nhất",
              "Phân thức bậc hai / bậc nhất", "Sin & Cos (radian)", "Mũ & Log", "Căn & Trị tuyệt đối", "Lỗ thủng"})
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(preset), s);
        gtk_combo_box_set_active(GTK_COMBO_BOX(preset), 0);
        pack(form, preset, "math-preset");
        auto* hint = gtk_label_new("Cú pháp: x, m, pi/π, + − * / ^, ( ), sin, cos, exp, ln, sqrt, abs.\nDùng * để "
                                   "nhân; lũy thừa trước dấu −. Góc: radian.");
        gtk_label_set_line_wrap(GTK_LABEL(hint), true);
        gtk_label_set_xalign(GTK_LABEL(hint), 0);
        auto* syntax = syntaxBox = gtk_expander_new("Cú pháp nhập hàm & ký hiệu");
        gtk_container_add(GTK_CONTAINER(syntax), hint);
        pack(form, syntax, "");
        sourceControls(form);
        curveEditor = gtk_expander_new("Hai hàm & màu nét");
        gtk_expander_set_expanded(GTK_EXPANDER(curveEditor), true);
        auto* curveBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_add(GTK_CONTAINER(curveEditor), curveBox);
        pack(form, curveEditor, "");
        for (int i = 0; i < 2; ++i) {
            auto* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
            curveRows[i] = row;
            enabled[i] = gtk_check_button_new_with_label(("Hàm " + std::to_string(i + 1)).c_str());
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled[i]), i == 0);
            gtk_box_pack_start(GTK_BOX(row), enabled[i], false, false, 0);
            auto* e = entry("f" + std::to_string(i + 1), i == 0 ? "x^3-3*x" : "m*x");
            gtk_box_pack_start(GTK_BOX(row), e, true, true, 0);
            colors[i] = gtk_color_button_new();
            GdkRGBA color;
            i == 0 ? gdk_rgba_parse(&color, "#205bce") : gdk_rgba_parse(&color, "#c42b35");
            gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(colors[i]), &color);
            gtk_box_pack_start(GTK_BOX(row), colors[i], false, false, 0);
            pack(curveBox, row, "");
        }
        formulaOnly = toggle(form, "Chỉ chèn công thức (PDF vector)");
        auto* parameter = parameterBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_pack_start(GTK_BOX(parameter), gtk_label_new("Tham số m"), false, false, 0);
        m = gtk_spin_button_new_with_range(-1e6, 1e6, .1);
        atk_object_set_name(gtk_widget_get_accessible(m), "math-m");
        gtk_spin_button_set_digits(GTK_SPIN_BUTTON(m), 4);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(m), 1);
        gtk_box_pack_start(GTK_BOX(parameter), m, true, true, 0);
        pack(form, parameter, "math-parameter");
        graphBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        graphSettings = gtk_expander_new("Miền vẽ & thang đo");
        gtk_expander_set_expanded(GTK_EXPANDER(graphSettings), true);
        gtk_container_add(GTK_CONTAINER(graphSettings), graphBox);
        pack(form, graphSettings, "");
        tableBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        auto* grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
        int index = 0;
        for (auto item: {std::pair{"xmin", "-5"},
                         {"xmax", "5"},
                         {"ymin", "-4"},
                         {"ymax", "4"},
                         {"xtick", "1"},
                         {"ytick", "1"},
                         {"xlabel", "x"},
                         {"ylabel", "y"},
                         {"x0", "1"}}) {
            const std::map<std::string, std::string> names = {
                    {"xmin", "x từ"},     {"xmax", "đến"},      {"ymin", "y từ"},
                    {"ymax", "đến"},      {"xtick", "Bước x"},  {"ytick", "Bước y"},
                    {"xlabel", "Trục x"}, {"ylabel", "Trục y"}, {"x0", "x₀"}};
            int col = (index % 2) * 2, row = index / 2;
            auto* label = gtk_label_new(names.at(item.first).c_str());
            if (std::string(item.first) == "x0")
                x0Label = label;
            gtk_grid_attach(GTK_GRID(grid), label, col, row, 1, 1);
            auto* e = entry(item.first, item.second);
            gtk_entry_set_width_chars(GTK_ENTRY(e), 9);
            gtk_grid_attach(GTK_GRID(grid), e, col + 1, row, 1, 1);
            ++index;
        }
        pack(graphBox, grid, "");
        equal = toggle(graphBox, "Hai trục cùng tỉ lệ");
        pi = toggle(graphBox, "Mốc x theo π (bước chia là bội π)");
        pack(form, tableBox, "");
        auto* explanation = gtk_label_new(
                "Bấm ô dấu để đổi + / − / 0 / ∥. Thêm/bớt cột bằng nút.\nTự sinh: đa thức ≤ 3; tử ≤ 2 / mẫu bậc 1.");
        gtk_label_set_xalign(GTK_LABEL(explanation), 0);
        gtk_label_set_line_wrap(GTK_LABEL(explanation), true);
        pack(tableBox, explanation, "");
        auto* autoButton = gtk_button_new_with_label("Tự sinh bảng từ hàm 1 → sửa các ô bên dưới");
        pack(tableBox, autoButton, "math-auto-table");
        g_signal_connect(autoButton, "clicked",
                         G_CALLBACK(+[](GtkButton*, gpointer p) { static_cast<Dialog*>(p)->automatic(); }), this);
        auto* nameRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_pack_start(GTK_BOX(nameRow), gtk_label_new("Tên hàng dấu"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(nameRow), entry("table-name", "f′"), true, true, 0);
        pack(tableBox, nameRow, "");
        auto* columnBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        addColumn = gtk_button_new_with_label("+ Cột");
        removeColumn = gtk_button_new_with_label("− Cột");
        atk_object_set_name(gtk_widget_get_accessible(addColumn), "math-add-column");
        atk_object_set_name(gtk_widget_get_accessible(removeColumn), "math-remove-column");
        gtk_widget_set_tooltip_text(addColumn, "Thêm một cột trống ngay trước cột cuối (giữ mốc +∞).");
        gtk_widget_set_tooltip_text(removeColumn, "Bớt một cột ngay trước cột cuối. Tối thiểu 2 cột.");
        pack(columnBar, addColumn, "");
        pack(columnBar, removeColumn, "");
        for (int direction: {-1, 1}) {
            auto* b = gtk_button_new_with_label(direction < 0 ? "←" : "→");
            gtk_widget_set_tooltip_text(b, "Cuộn bảng từng cột.");
            atk_object_set_name(gtk_widget_get_accessible(b), direction < 0 ? "math-scroll-left" : "math-scroll-right");
            g_object_set_data(G_OBJECT(b), "direction", GINT_TO_POINTER(direction));
            pack(columnBar, b, "");
            g_signal_connect(b, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer p) {
                                 auto& d = *static_cast<Dialog*>(p);
                                 auto* a = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(d.tableScroll));
                                 int direction = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "direction"));
                                 gtk_adjustment_set_value(a, gtk_adjustment_get_value(a) + direction * 60);
                             }),
                             this);
        }
        pack(tableBox, columnBar, "");
        g_signal_connect(addColumn, "clicked",
                         G_CALLBACK(+[](GtkButton*, gpointer p) { static_cast<Dialog*>(p)->resizeTable(1); }), this);
        g_signal_connect(removeColumn, "clicked",
                         G_CALLBACK(+[](GtkButton*, gpointer p) { static_cast<Dialog*>(p)->resizeTable(-1); }), this);
        auto* visibility = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        showSign = toggle(visibility, "Hiện dấu", true);
        showValues = toggle(visibility, "Hiện giá trị / chiều", true);
        atk_object_set_name(gtk_widget_get_accessible(showSign), "Hiện hàng dấu");
        atk_object_set_name(gtk_widget_get_accessible(showValues), "Hiện hàng giá trị / chiều biến thiên");
        pack(tableBox, visibility, "");
        tableScroll = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(tableScroll), false);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tableScroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
        gtk_widget_set_size_request(tableScroll, -1, 155);
        tableGrid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(tableGrid), 5);
        gtk_grid_set_column_spacing(GTK_GRID(tableGrid), 4);
        gtk_container_add(GTK_CONTAINER(tableScroll), tableGrid);
        pack(tableBox, tableScroll, "");
        auto* symbols = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_box_pack_start(GTK_BOX(symbols), gtk_label_new("Chèn:"), false, false, 0);
        for (const char* symbol: {"−∞", "+∞", "↗", "↘", "→", "∥", "○"}) {
            auto* b = gtk_button_new_with_label(symbol);
            gtk_widget_set_size_request(b, 38, 36);
            gtk_widget_set_can_focus(b, false);
            gtk_box_pack_start(GTK_BOX(symbols), b, false, false, 0);
            g_signal_connect(b, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer p) {
                                 static_cast<Dialog*>(p)->insertSymbol(gtk_button_get_label(button));
                             }),
                             this);
        }
        pack(tableBox, symbols, "");
        // Keep the serialized rows for compatibility and fast keyboard entry.
        // The default pen workflow edits individual cells instead of pipe syntax.
        auto* advanced = gtk_expander_new("Nhập cả hàng bằng dấu | (tùy chọn)");
        auto* raw = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
        gtk_container_add(GTK_CONTAINER(advanced), raw);
        for (auto item:
             {std::pair{"table-x", "−∞||0||+∞"}, {"table-sign", "|−|0|+|"}, {"table-value", "+∞|↘|0|↗|+∞"}}) {
            gtk_box_pack_start(GTK_BOX(raw), gtk_label_new(item.first), false, false, 0);
            gtk_box_pack_start(GTK_BOX(raw), entry(item.first, item.second), false, false, 0);
        }
        pack(tableBox, advanced, "");
        auto* right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_box_pack_start(GTK_BOX(body), right, true, true, 0);
        sourceCard = card(right, "Ảnh gốc · vùng PDF đã quét", &sourceHeading);
        sourceImage = gtk_drawing_area_new();
        gtk_widget_set_size_request(sourceImage, 460, dialogHeight < 780 ? 64 : 112);
        gtk_box_pack_start(GTK_BOX(sourceCard), sourceImage, false, false, 0);
        fullSource = gtk_check_button_new_with_label("Hiện toàn bộ khung đã quét");
        gtk_box_pack_start(GTK_BOX(sourceCard), fullSource, false, false, 0);
        g_signal_connect(fullSource, "toggled", G_CALLBACK(+[](GtkToggleButton*, gpointer p) {
                             gtk_widget_queue_draw(static_cast<Dialog*>(p)->sourceImage);
                         }),
                         this);
        g_signal_connect(sourceImage, "draw", G_CALLBACK(+[](GtkWidget* w, cairo_t* cr, gpointer p) -> gboolean {
                             auto& d = *static_cast<Dialog*>(p);
                             cairo_set_source_rgb(cr, 1, 1, 1);
                             cairo_paint(cr);
                             auto* im = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d.fullSource)) ? d.fullCrop :
                                                                                                        d.crop;
                             if (im) {
                                 double width = gtk_widget_get_allocated_width(w);
                                 double height = gtk_widget_get_allocated_height(w);
                                 double scale = std::min((width - 16) / gdk_pixbuf_get_width(im),
                                                         (height - 12) / gdk_pixbuf_get_height(im));
                                 cairo_translate(cr, (width - gdk_pixbuf_get_width(im) * scale) / 2,
                                                 (height - gdk_pixbuf_get_height(im) * scale) / 2);
                                 cairo_scale(cr, scale, scale);
                                 gdk_cairo_set_source_pixbuf(cr, im, 0, 0);
                                 cairo_paint(cr);
                             }
                             return false;
                         }),
                         this);
        equationCard = card(right, "Công thức dùng để vẽ");
        equationPreview = gtk_drawing_area_new();
        gtk_widget_set_size_request(equationPreview, 460, dialogHeight < 780 ? 56 : 82);
        atk_object_set_name(gtk_widget_get_accessible(equationPreview), "math-equation-preview");
        gtk_box_pack_start(GTK_BOX(equationCard), equationPreview, false, false, 0);
        g_signal_connect(equationPreview, "draw", G_CALLBACK(+[](GtkWidget* w, cairo_t* cr, gpointer p) -> gboolean {
                             auto& d = *static_cast<Dialog*>(p);
                             d.paintEquations(w, cr);
                             return false;
                         }),
                         this);
        auto* graphCard = card(right, "Đồ thị xem trước · chưa thay đổi tài liệu", &graphHeading);
        gtk_widget_set_vexpand(graphCard, true);
        gtk_box_set_child_packing(GTK_BOX(right), graphCard, true, true, 0, GTK_PACK_START);
        preview = gtk_drawing_area_new();
        gtk_widget_set_size_request(preview, 460, dialogHeight < 780 ? 130 : 240);
        gtk_box_pack_start(GTK_BOX(graphCard), preview, true, true, 0);
        atk_object_set_name(gtk_widget_get_accessible(preview), "math-scene-preview");
        gtk_widget_add_events(preview, GDK_BUTTON_PRESS_MASK);
        g_signal_connect(preview, "button-press-event",
                         G_CALLBACK(+[](GtkWidget*, GdkEventButton* e, gpointer p) -> gboolean {
                             static_cast<Dialog*>(p)->pickCoordinate(e->x, e->y);
                             return true;
                         }),
                         this);
        g_signal_connect(preview, "draw", G_CALLBACK(+[](GtkWidget* w, cairo_t* cr, gpointer p) -> gboolean {
                             auto& d = *static_cast<Dialog*>(p);
                             cairo_set_source_rgb(cr, 1, 1, 1);
                             cairo_paint(cr);
                             if (d.valid && (d.coordinateMode() || !d.imported || d.sourceApplied)) {
                                 double top = d.previewGraphOnly ? 72. : 0.;
                                 double scale =
                                         std::min((gtk_widget_get_allocated_width(w) - 12) / d.figure.width,
                                                  (gtk_widget_get_allocated_height(w) - 12) / (d.figure.height - top));
                                 cairo_translate(cr, (gtk_widget_get_allocated_width(w) - d.figure.width * scale) / 2,
                                                 6 - top * scale);
                                 cairo_scale(cr, scale, scale);
                                 paint(cr, d.figure, d.previewGraphOnly);
                             } else if (d.imported && !d.sourceApplied) {
                                 d.placeholder(cr, w, "Chọn cách đọc đúng ở bên trái để xem đồ thị.");
                             }
                             return false;
                         }),
                         this);
        error = gtk_label_new("");
        gtk_label_set_line_wrap(GTK_LABEL(error), true);
        gtk_label_set_max_width_chars(GTK_LABEL(error), 62);
        gtk_label_set_xalign(GTK_LABEL(error), 0);
        gtk_box_pack_start(GTK_BOX(right), error, false, false, 0);
        load();
        for (auto [name, e]: entries) g_signal_connect(e, "changed", G_CALLBACK(changed), this);
        for (auto* widget: {mode, m, enabled[0], enabled[1], equal, pi, showSign, showValues, formulaOnly})
            g_signal_connect(widget,
                             GTK_IS_COMBO_BOX(widget)   ? "changed" :
                             GTK_IS_SPIN_BUTTON(widget) ? "value-changed" :
                                                          "toggled",
                             G_CALLBACK(changed), this);
        for (auto* color: colors) g_signal_connect(color, "color-set", G_CALLBACK(changed), this);
        g_signal_connect(preset, "changed",
                         G_CALLBACK(+[](GtkComboBox*, gpointer p) { static_cast<Dialog*>(p)->choosePreset(); }), this);
        loading = false;
        gtk_widget_show_all(win);
        refresh();
        contextTimer = g_timeout_add(
                250,
                +[](gpointer p) -> gboolean {
                    auto& d = *static_cast<Dialog*>(p);
                    if (!d.contextValid()) {
                        gtk_widget_set_sensitive(d.insertButton, false);
                        gtk_label_set_text(GTK_LABEL(d.error),
                                           "Trang/lớp/zoom đã đổi. Đóng rồi mở lại bảng để xác nhận đích chèn.");
                    }
                    return G_SOURCE_CONTINUE;
                },
                this);
        captureSelection(false);
        if (!source.pdf)
            gtk_widget_hide(sourceCard);
    }
    ~Dialog() {
        if (css) {
            gtk_style_context_remove_provider_for_screen(gtk_widget_get_screen(win), GTK_STYLE_PROVIDER(css));
            g_object_unref(css);
        }
        if (fullCrop)
            g_object_unref(fullCrop);
        lifetime->owner = nullptr;
        cancelRecognition();
        if (contextTimer)
            g_source_remove(contextTimer);
        if (crop)
            g_object_unref(crop);
        if (timer)
            g_source_remove(timer);
        gtk_widget_destroy(win);
    }
    void styleDialog() {
        css = gtk_css_provider_new();
        const char* rules = R"CSS(
#teaching-tools-dialog { background-color:#f3f6fb; color:#20314b; }
#teaching-tools-dialog .math-title { font-size:23px; font-weight:700; color:#152b4b; }
#teaching-tools-dialog .math-steps { font-size:11px; font-weight:600; color:#54708e; }
#teaching-tools-dialog .math-panel { background-color:white; border:1px solid #dce4ef; border-radius:12px; }
#teaching-tools-dialog .math-preview-card { padding:10px; }
#teaching-tools-dialog .math-card-title { font-size:12px; font-weight:700; color:#456180; }
#teaching-tools-dialog .math-import { background-color:#eef5ff; border:1px solid #d5e4fb; border-radius:10px; padding:10px; }
#teaching-tools-dialog entry, #teaching-tools-dialog spinbutton { border-radius:7px; min-height:32px; }
#teaching-tools-dialog button { border-radius:7px; min-height:28px; }
#teaching-tools-dialog entry, #teaching-tools-dialog spinbutton,
#teaching-tools-dialog combobox button { background-image:none; background-color:#ffffff; color:#20314b; border-color:#cbd5e1; }
#teaching-tools-dialog combobox button:hover { background-color:#eef4fc; }
#teaching-tools-dialog button:not(.math-primary):not(.math-secondary) { color:#20314b; background-image:none; background-color:#f8fafc; border-color:#cbd5e1; }
#teaching-tools-dialog button.math-primary { background-image:none; background-color:#2563eb; color:white; border-color:#2563eb; font-weight:700; }
#teaching-tools-dialog button.math-primary:hover { background-color:#1d4ed8; }
#teaching-tools-dialog button.math-primary:disabled { background-color:#d7e1f1; border-color:#d7e1f1; color:#61718d; }
#teaching-tools-dialog button.math-secondary { background-image:none; background-color:#dbeafe; color:#17469b; border-color:#c2d7fc; }
)CSS";
        gtk_css_provider_load_from_data(css, rules, -1, nullptr);
        gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(win), GTK_STYLE_PROVIDER(css),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    GtkWidget* card(GtkWidget* parent, const char* title, GtkWidget** titleOut = nullptr) {
        auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(box), "math-panel");
        gtk_style_context_add_class(gtk_widget_get_style_context(box), "math-preview-card");
        auto* label = gtk_label_new(title);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(label), "math-card-title");
        gtk_box_pack_start(GTK_BOX(box), label, false, false, 0);
        gtk_box_pack_start(GTK_BOX(parent), box, false, false, 0);
        if (titleOut)
            *titleOut = label;
        return box;
    }
    void placeholder(cairo_t* cr, GtkWidget* w, const char* text) {
        auto* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, text, -1);
        pango_layout_set_width(layout, std::max(1, gtk_widget_get_allocated_width(w) - 40) * PANGO_SCALE);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        cairo_set_source_rgb(cr, .40, .48, .59);
        cairo_move_to(cr, 20, std::max(10., gtk_widget_get_allocated_height(w) / 2. - 14));
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
    }
    void paintEquations(GtkWidget* w, cairo_t* cr) {
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        if (!valid || (imported && !sourceApplied) || equationStrings.empty()) {
            placeholder(cr, w,
                        imported ? "Công thức nhận được sẽ hiện ở đây sau khi đối chiếu cách đọc." :
                                   "Nhập hàm hoặc lấy công thức từ vùng PDF.");
            return;
        }
        try {
            double width = 0, height = 0;
            std::vector<FormulaMetrics> sizes;
            for (const auto& eq: equationStrings) {
                auto size = formulaMetrics(eq, 24);
                width = std::max(width, size.width);
                height += size.height + 6;
                sizes.push_back(size);
            }
            double scale = std::min({1.25, (gtk_widget_get_allocated_width(w) - 24) / width,
                                     (gtk_widget_get_allocated_height(w) - 8) / height});
            cairo_save(cr);
            cairo_translate(cr, 12, (gtk_widget_get_allocated_height(w) - height * scale) / 2);
            cairo_scale(cr, scale, scale);
            for (size_t i = 0; i < equationStrings.size(); ++i) {
                rgb(cr, equationColors[i]);
                paintFormula(cr, equationStrings[i], 24);
                cairo_translate(cr, 0, sizes[i].height + 6);
            }
            cairo_restore(cr);
        } catch (const std::exception&) {
            placeholder(cr, w, "Cần sửa công thức ở bên trái.");
        }
    }
    bool readSourceRegion(Range& bounds, std::string& text) {
        if (!contextValid() || c->getCurrentPage()->getPdfPageNr() == npos)
            return false;
        auto* view = c->getWindow()->getXournal();
        auto sourcePage = c->getCurrentPage();
        if (xoj::tool::isPdfSelectionTool(c->getToolHandler()->getToolType())) {
            auto* selection = c->getWindow()->getPdfToolbox()->getSelection();
            if (selection && selection->isFinalized() && selection->getDocumentPage() == sourcePage &&
                selection->getSelectionPageNr() == sourcePage->getPdfPageNr()) {
                bounds = selection->getBoundsRange();
                text = selection->getSelectedText();
                if (c->getWindow()->getPdfToolbox()->selectionStyle != XojPdfPageSelectionStyle::Area &&
                    !selection->getSelectedTextRects().empty()) {
                    Range tight;
                    for (const auto& r: selection->getSelectedTextRects())
                        tight = tight.unite(Range(r.x1, r.y1, r.x2, r.y2));
                    bounds = tight;
                }
                return true;
            }
        }
        // The ordinary rectangle tool can finish with NO selected elements on
        // a PDF, yet its raw dragged frame is valid. Do not read the selected
        // object bbox, insert a temporary image, or consume the copy-image cache.
        PageRef page;
        return view->peekLastSelectionRect(page, bounds) && page == sourcePage;
    }
    bool contextValid() {
        return c->getEditingGeneration() == targetGeneration && c->getEditingPage() == targetPage && targetPage && targetPage->getSelectedLayer() == targetLayer &&
               c->getEditingView()->getZoom() == targetZoom;
    }
    void sourceControls(GtkWidget* form) {
        auto* frame = sourceFrame = gtk_expander_new("Lấy công thức từ vùng PDF");
        Range region;
        std::string text;
        gtk_expander_set_expanded(GTK_EXPANDER(frame), readSourceRegion(region, text));
        gtk_style_context_add_class(gtk_widget_get_style_context(frame), "math-import");
        auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(frame), box);
        pack(form, frame, "");
        auto* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        recognizeButton = gtk_button_new_with_label("Nhận dạng vùng PDF");
        pack(row, recognizeButton, "");
        auto* choose = gtk_button_new_with_label("Chọn vùng khác…");
        pack(row, choose, "");
        pack(box, row, "");
        g_signal_connect(recognizeButton, "clicked",
                         G_CALLBACK(+[](GtkButton*, gpointer p) { static_cast<Dialog*>(p)->captureSelection(true); }),
                         this);
        g_signal_connect(choose, "clicked", G_CALLBACK(+[](GtkButton*, gpointer p) {
                             gtk_dialog_response(GTK_DIALOG(static_cast<Dialog*>(p)->win), 1001);
                         }),
                         this);
        auto* cancel = gtk_button_new_from_icon_name("process-stop-symbolic", GTK_ICON_SIZE_BUTTON);
        atk_object_set_name(gtk_widget_get_accessible(cancel), "Dừng nhận dạng");
        gtk_widget_set_tooltip_text(cancel, "Dừng nhận dạng");
        stopRecognitionButton = cancel;
        pack(row, cancel, "");
        g_signal_connect(cancel, "clicked", G_CALLBACK(+[](GtkButton*, gpointer p) {
                             auto& d = *static_cast<Dialog*>(p);
                             d.cancelRecognition();
                             d.sourceState = SourceState::Cancelled;
                             gtk_label_set_text(GTK_LABEL(d.sourceStatus),
                                                "Đã dừng. Có thể nhận dạng lại; tài liệu chưa thay đổi.");
                             d.refresh();
                         }),
                         this);
        sourceStatus = gtk_label_new("Ctrl+Shift+R → kéo bao trọn công thức → Alt+G.\nChọn chữ PDF cũng dùng được. "
                                     "Nhận dạng hoàn toàn cục bộ.");
        gtk_label_set_line_wrap(GTK_LABEL(sourceStatus), true);
        gtk_label_set_max_width_chars(GTK_LABEL(sourceStatus), 48);
        gtk_label_set_xalign(GTK_LABEL(sourceStatus), 0);
        pack(box, sourceStatus, "");
        candidateCombo = gtk_combo_box_text_new();
        atk_object_set_name(gtk_widget_get_accessible(candidateCombo), "pdf-formula-candidates");
        auto* readingLabel = gtk_label_new("Cách đọc nhận được · chọn để đối chiếu");
        gtk_label_set_xalign(GTK_LABEL(readingLabel), 0);
        gtk_box_pack_start(GTK_BOX(box), readingLabel, false, false, 0);
        auto* cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(candidateCombo));
        for (auto* i = cells; i; i = i->next)
            if (GTK_IS_CELL_RENDERER_TEXT(i->data))
                g_object_set(i->data, "ellipsize", PANGO_ELLIPSIZE_END, "max-width-chars", 30, nullptr);
        g_list_free(cells);
        pack(box, candidateCombo, "");
        candidateEdit = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(candidateEdit), 2048);
        atk_object_set_name(gtk_widget_get_accessible(candidateEdit), "pdf-formula-edit");
        auto* editLabel = gtk_label_new("Sửa chỗ nhận sai, rồi cập nhật hàm");
        gtk_label_set_xalign(GTK_LABEL(editLabel), 0);
        gtk_box_pack_start(GTK_BOX(box), editLabel, false, false, 0);
        pack(box, candidateEdit, "");
        gtk_widget_set_tooltip_text(
                candidateEdit, "Sửa công thức gốc hoặc LaTeX: \\frac{...}{...}, \\sqrt{...}. Không thực thi macro.");
        g_signal_connect(candidateCombo, "changed",
                         G_CALLBACK(+[](GtkComboBox*, gpointer p) { static_cast<Dialog*>(p)->selectCandidate(); }),
                         this);
        g_signal_connect(candidateEdit, "changed", G_CALLBACK(+[](GtkEditable*, gpointer p) {
                             auto& d = *static_cast<Dialog*>(p);
                             if (d.selectingCandidate)
                                 return;
                             if (d.activeCandidate >= 0 && size_t(d.activeCandidate) < d.candidates.size())
                                 d.candidates[d.activeCandidate].raw = gtk_entry_get_text(GTK_ENTRY(d.candidateEdit));
                             d.sourceApplied = false;
                             gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d.confirmSource), false);
                             d.syncSourceControls();
                         }),
                         this);
        auto* apply = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        for (int i = 0; i < 2; ++i) {
            applyCandidate[i] = gtk_button_new_with_label(i == 0 ? "Dùng làm hàm 1" : "Thêm làm hàm 2");
            pack(apply, applyCandidate[i], "");
            if (i == 0)
                gtk_style_context_add_class(gtk_widget_get_style_context(applyCandidate[i]), "math-secondary");
            g_object_set_data(G_OBJECT(applyCandidate[i]), "curve", GINT_TO_POINTER(i));
            g_signal_connect(
                    applyCandidate[i], "clicked", G_CALLBACK(+[](GtkButton* b, gpointer p) {
                        static_cast<Dialog*>(p)->applyFormula(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "curve")));
                    }),
                    this);
        }
        pack(box, apply, "");
        auto* palette = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        for (const char* symbol: {"^2", "/", "sqrt()", "π", "−", "(", ")"}) {
            auto* b = gtk_button_new_with_label(symbol);
            gtk_widget_set_focus_on_click(b, false);
            pack(palette, b, "");
            g_signal_connect(b, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer p) {
                                 auto& d = *static_cast<Dialog*>(p);
                                 auto* e = GTK_EDITABLE(d.candidateEdit);
                                 int a;
                                 int z;
                                 if (gtk_editable_get_selection_bounds(e, &a, &z))
                                     gtk_editable_delete_text(e, a, z);
                                 int pos = gtk_editable_get_position(e);
                                 gtk_editable_insert_text(e, gtk_button_get_label(b), -1, &pos);
                                 gtk_editable_set_position(e, pos);
                                 gtk_widget_grab_focus(d.candidateEdit);
                             }),
                             this);
        }
        pack(box, palette, "");
        confirmSource = gtk_check_button_new_with_label("Đã đối chiếu ảnh, công thức và đồ thị");
        pack(box, confirmSource, "");
        g_signal_connect(confirmSource, "toggled", G_CALLBACK(+[](GtkToggleButton*, gpointer p) {
                             auto& d = *static_cast<Dialog*>(p);
                             if (!d.loading)
                                 d.updateInsertState();
                         }),
                         this);
    }
    void syncSourceControls() {
        bool running = sourceState == SourceState::Running;
        gtk_widget_set_sensitive(stopRecognitionButton, running);
        gtk_widget_set_sensitive(candidateCombo, !running && !candidates.empty());
        gtk_widget_set_sensitive(candidateEdit, !running);
        for (auto* b: applyCandidate)
            gtk_widget_set_sensitive(b, !running && *gtk_entry_get_text(GTK_ENTRY(candidateEdit)));
        gtk_widget_set_sensitive(confirmSource, !running && imported && sourceApplied);
    }
    void cancelRecognition() {
        ++generation;
        if (ocrTimeout) {
            g_source_remove(ocrTimeout);
            ocrTimeout = 0;
        }
        if (job) {
            job->cancel();
            job.reset();
        }
        gtk_widget_set_sensitive(recognizeButton, true);
    }
    void captureSelection(bool required) {
        Range bounds;
        std::string text;
        if (!readSourceRegion(bounds, text)) {
            if (required)
                gtk_label_set_text(GTK_LABEL(sourceStatus),
                                   "Chưa có vùng trên trang PDF hiện tại. Ctrl+Shift+R, kéo khung quanh công thức rồi "
                                   "Alt+G; hoặc bấm Chọn vùng khác…");
            return;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(mode), 0);
        auto* doc = c->getDocument();
        {
            std::shared_lock lock(*doc);
            source.pdf = doc->getPdfPage(c->getCurrentPage()->getPdfPageNr());
        }
        if (!source.pdf)
            return;
        bounds.addPadding(3);
        source.bounds = bounds.intersect(Range(0, 0, source.pdf->getWidth(), source.pdf->getHeight()));
        source.pdfPage = c->getCurrentPage()->getPdfPageNr();
        source.text = std::move(text);
        cancelRecognition();
        unsigned requestGeneration = generation;
        sourceState = SourceState::Running;
        if (!imported) {
            gtk_expander_set_expanded(GTK_EXPANDER(curveEditor), false);
            gtk_expander_set_expanded(GTK_EXPANDER(graphSettings), false);
        }
        imported = true;
        sourceApplied = false;
        firstSourceApplication = true;
        recognitionWarning.clear();
        gtk_widget_show_all(sourceCard);
        auto heading = "Vùng gốc · PDF trang " + std::to_string(source.pdfPage + 1);
        gtk_label_set_text(GTK_LABEL(sourceHeading), heading.c_str());
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(confirmSource), false);
        gtk_label_set_text(GTK_LABEL(sourceStatus), "Đang nhận dạng cục bộ… Giới hạn 60 giây. Không gửi PDF ra ngoài.");
        gtk_widget_set_sensitive(recognizeButton, false);
        gtk_widget_set_sensitive(insertButton, false);
        syncSourceControls();
        std::weak_ptr<Lifetime> weak = lifetime;
        job = PdfFormulaJob::start(source, [weak, requestGeneration](RecognitionResult result) {
            auto life = weak.lock();
            if (!life || !life->owner)
                return;
            auto& d = *life->owner;
            if (requestGeneration != d.generation)
                return;
            if (d.ocrTimeout) {
                g_source_remove(d.ocrTimeout);
                d.ocrTimeout = 0;
            }
            d.sourceState = result.code == RecognitionCode::Ready ? SourceState::Review : SourceState::Error;
            gtk_widget_set_sensitive(d.recognizeButton, true);
            if (d.crop) {
                g_object_unref(d.crop);
                d.crop = nullptr;
            }
            if (d.fullCrop) {
                g_object_unref(d.fullCrop);
                d.fullCrop = nullptr;
            }
            if (!result.imagePath.empty())
                d.fullCrop = gdk_pixbuf_new_from_file(result.imagePath.c_str(), nullptr);
            if (d.fullCrop) {
                const auto& box = result.imageContent;
                if (box[2] > 0 && box[3] > 0)
                    d.crop = gdk_pixbuf_new_subpixbuf(d.fullCrop, box[0], box[1], box[2], box[3]);
                else
                    d.crop = GDK_PIXBUF(g_object_ref(d.fullCrop));
            }
            gtk_widget_queue_draw(d.sourceImage);
            d.recognitionWarning = result.warning;
            d.candidates = std::move(result.candidates);
            d.activeCandidate = -1;
            d.selectingCandidate = true;
            gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(d.candidateCombo));
            for (size_t i = 0; i < d.candidates.size(); ++i) {
                auto& c = d.candidates[i];
                std::string label = std::to_string(i + 1) + "  ·  " +
                                    (c.usable() ? c.expression : "Cần sửa ký hiệu / chọn lại vùng");
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(d.candidateCombo), label.c_str());
            }
            d.selectingCandidate = false;
            gtk_label_set_text(GTK_LABEL(d.sourceStatus), result.message.c_str());
            if (!d.candidates.empty()) {
                std::set<std::string> meanings;
                int firstUsable = -1;
                for (size_t i = 0; i < d.candidates.size(); ++i)
                    if (d.candidates[i].usable()) {
                        meanings.insert(Expression(d.candidates[i].expression).latex());
                        if (firstUsable < 0)
                            firstUsable = static_cast<int>(i);
                    }
                d.selectingCandidate = true;
                gtk_combo_box_set_active(GTK_COMBO_BOX(d.candidateCombo), firstUsable < 0 ? 0 : firstUsable);
                d.selectingCandidate = false;
                d.selectCandidate();
                // One distinct validated AST can be previewed immediately. The
                // mandatory review checkbox stays off; ambiguous meanings still
                // require an explicit choice/apply action, never a guessed insert.
                if (result.code == RecognitionCode::Ready && meanings.size() == 1 && result.warning.empty()) {
                    d.applyFormula(0);
                    gtk_label_set_text(
                            GTK_LABEL(d.sourceStatus),
                            "Đã lấy công thức vào Hàm 1 và xem trước bên phải. Đối chiếu ảnh gốc trước khi xác nhận.");
                } else if (result.code == RecognitionCode::Ready && !result.warning.empty()) {
                    gtk_label_set_text(GTK_LABEL(d.sourceStatus), result.warning.c_str());
                } else if (result.code == RecognitionCode::Ready && meanings.size() > 1) {
                    gtk_label_set_text(GTK_LABEL(d.sourceStatus), "Có nhiều công thức hoặc cách đọc khác nhau. Chọn "
                                                                  "một cách đọc, rồi Dùng làm hàm 1/2 để đối chiếu.");
                }
            }
            d.job.reset();
            d.refresh();
        });
        ocrTimeout = g_timeout_add_seconds(
                60,
                +[](gpointer p) -> gboolean {
                    auto& d = *static_cast<Dialog*>(p);
                    d.ocrTimeout = 0;
                    d.cancelRecognition();
                    d.sourceState = SourceState::Error;
                    gtk_label_set_text(GTK_LABEL(d.sourceStatus),
                                       "OCR quá 60 giây; đã dừng. Chọn vùng nhỏ hơn hoặc nhận dạng lại.");
                    d.refresh();
                    return G_SOURCE_REMOVE;
                },
                this);
    }
    void selectCandidate() {
        if (selectingCandidate)
            return;
        int i = gtk_combo_box_get_active(GTK_COMBO_BOX(candidateCombo));
        if (i < 0 || size_t(i) >= candidates.size())
            return;
        activeCandidate = i;
        selectingCandidate = true;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(confirmSource), false);
        gtk_entry_set_text(GTK_ENTRY(candidateEdit), candidates[i].raw.c_str());
        selectingCandidate = false;
        if (sourceState == SourceState::Review)
            gtk_label_set_text(GTK_LABEL(sourceStatus), candidates[i].message.c_str());
        gtk_widget_set_tooltip_text(candidateEdit,
                                    (candidates[i].source + "\nBản nhận dạng: " + candidates[i].raw).c_str());
    }
    void applyFormula(int curve) {
        if (sourceState == SourceState::Running)
            return;
        auto candidate = importFormula(gtk_entry_get_text(GTK_ENTRY(candidateEdit)), "Đã sửa/đối chiếu");
        if (!candidate.usable()) {
            gtk_label_set_text(GTK_LABEL(sourceStatus), candidate.message.c_str());
            sourceState = SourceState::Error;
            return;
        }
        imported = true;
        sourceState = SourceState::Review;
        if (firstSourceApplication && curve == 0)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled[1]), false);
        firstSourceApplication = false;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(confirmSource), false);
        gtk_entry_set_text(GTK_ENTRY(entries.at(curve == 0 ? "f1" : "f2")), candidate.expression.c_str());
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled[curve]), true);
        gtk_combo_box_set_active(GTK_COMBO_BOX(mode), 0);
        sourceApplied = true;
        auto message = recognitionWarning.empty() ? candidate.message : recognitionWarning + "\n" + candidate.message;
        gtk_label_set_text(GTK_LABEL(sourceStatus), message.c_str());
        refresh();
    }
    void pack(GtkWidget* box, GtkWidget* w, const char* name) {
        if (*name)
            gtk_widget_set_name(w, name);
        gtk_widget_set_size_request(w, -1, 36);
        gtk_box_pack_start(GTK_BOX(box), w, false, false, 0);
    }
    GtkWidget* entry(const std::string& name, const std::string& value) {
        auto* e = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(e), value.c_str());
        gtk_entry_set_width_chars(GTK_ENTRY(e), 20);
        gtk_widget_set_name(e, ("math-" + name).c_str());
        atk_object_set_name(gtk_widget_get_accessible(e), ("math-" + name).c_str());
        gtk_widget_set_size_request(e, -1, 36);
        entries[name] = e;
        return e;
    }
    GtkWidget* toggle(GtkWidget* box, const char* s, bool on = false) {
        auto* w = gtk_check_button_new_with_label(s);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), on);
        pack(box, w, "");
        return w;
    }
    static std::vector<std::string> cells(const std::string& s) {
        std::vector<std::string> result;
        size_t start = 0;
        for (size_t i = 0; i <= s.size(); ++i)
            if (i == s.size() || s[i] == '|') {
                result.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        return result;
    }
    void setRows(const std::array<std::vector<std::string>, 3>& rows) {
        syncTableBusy = true;
        const char* names[] = {"table-x", "table-sign", "table-value"};
        for (size_t r = 0; r < 3; ++r) {
            std::string s;
            for (size_t j = 0; j < rows[r].size(); ++j) {
                if (j)
                    s += '|';
                s += rows[r][j];
            }
            gtk_entry_set_text(GTK_ENTRY(entries.at(names[r])), s.c_str());
        }
        syncTableBusy = false;
    }
    void rebuildTable() {
        tableNeedsRebuild = false;
        symbolTarget = nullptr;
        for (auto& row: tableCells) row.clear();
        auto* children = gtk_container_get_children(GTK_CONTAINER(tableGrid));
        for (auto* i = children; i; i = i->next) gtk_widget_destroy(GTK_WIDGET(i->data));
        g_list_free(children);
        symbolTarget = nullptr;
        const char* names[] = {"table-x", "table-sign", "table-value"};
        std::array<std::vector<std::string>, 3> rows;
        size_t columns = 2;
        for (size_t r = 0; r < 3; ++r) {
            rows[r] = cells(text(names[r]));
            columns = std::max(columns, rows[r].size());
        }
        columns = std::min(size_t(19), columns);
        for (size_t r = 0; r < 3; ++r) {
            gtk_grid_attach(GTK_GRID(tableGrid),
                            gtk_label_new(r == 0 ? "x" :
                                          r == 1 ? "Dấu" :
                                                   "f"),
                            0, static_cast<int>(r), 1, 1);
            for (size_t j = 0; j < columns; ++j) {
                std::string value = j < rows[r].size() ? rows[r][j] : "";
                GtkWidget* w;
                if (r == 1) {
                    w = gtk_button_new_with_label(value.empty() ? "·" : value.c_str());
                    g_signal_connect(
                            w, "clicked",
                            G_CALLBACK(+[](GtkButton* b, gpointer p) { static_cast<Dialog*>(p)->cycleSign(b); }), this);
                } else {
                    w = gtk_entry_new();
                    gtk_entry_set_width_chars(GTK_ENTRY(w), 4);
                    gtk_entry_set_text(GTK_ENTRY(w), value.c_str());
                    gtk_entry_set_placeholder_text(GTK_ENTRY(w), r == 0 ? "mốc / trống" : "giá trị / ↗");
                    g_signal_connect(w, "changed",
                                     G_CALLBACK(+[](GtkWidget*, gpointer p) { static_cast<Dialog*>(p)->syncTable(); }),
                                     this);
                    g_signal_connect(w, "focus-in-event",
                                     G_CALLBACK(+[](GtkWidget* e, GdkEventFocus*, gpointer p) -> gboolean {
                                         static_cast<Dialog*>(p)->symbolTarget = e;
                                         return false;
                                     }),
                                     this);
                }
                gtk_widget_set_size_request(w, 56, 36);
                std::string name = "math-cell-" +
                                   std::string(r == 0 ? "x" :
                                               r == 1 ? "sign" :
                                                        "value") +
                                   "-" + std::to_string(j);
                atk_object_set_name(gtk_widget_get_accessible(w), name.c_str());
                gtk_grid_attach(GTK_GRID(tableGrid), w, static_cast<int>(j) + 1, static_cast<int>(r), 1, 1);
                tableCells[r].push_back(w);
            }
        }
        gtk_widget_set_sensitive(addColumn, columns < 19);
        gtk_widget_set_sensitive(removeColumn, columns > 2);
        gtk_widget_show_all(tableGrid);
    }
    void syncTable() {
        if (syncTableBusy || loading)
            return;
        std::array<std::vector<std::string>, 3> rows;
        for (size_t r = 0; r < 3; ++r)
            for (auto* w: tableCells[r]) {
                std::string s = r == 1 ? gtk_button_get_label(GTK_BUTTON(w)) : gtk_entry_get_text(GTK_ENTRY(w));
                rows[r].push_back(r == 1 && s == "·" ? "" : s);
            }
        setRows(rows);
        changed(nullptr, this);
    }
    void cycleSign(GtkButton* b) {
        std::array<std::string, 5> values = {"·", "+", "−", "0", "∥"};
        auto i = std::find(values.begin(), values.end(), gtk_button_get_label(b));
        auto next = i == values.end() || std::next(i) == values.end() ? values.begin() : std::next(i);
        gtk_button_set_label(b, next->c_str());
        syncTable();
    }
    void resizeTable(int delta) {
        std::array<std::vector<std::string>, 3> rows = {cells(text("table-x")), cells(text("table-sign")),
                                                        cells(text("table-value"))};
        size_t n = rows[0].size();
        if (n < 2 || n > 19 || (delta > 0 && n == 19) || (delta < 0 && n == 2))
            return;
        for (auto& row: rows) {
            row.resize(n);
            if (delta > 0)
                row.insert(row.end() - 1, "");
            else
                row.erase(row.end() - 2);
        }
        setRows(rows);
        tableNeedsRebuild = true;
        refresh();
    }
    void insertSymbol(const char* symbol) {
        auto* target = symbolTarget;
        if (!target && !tableCells[2].empty())
            target = tableCells[2].front();
        if (!target)
            return;
        int start, end;
        auto* editable = GTK_EDITABLE(target);
        if (gtk_editable_get_selection_bounds(editable, &start, &end))
            gtk_editable_delete_text(editable, start, end);
        int position = gtk_editable_get_position(editable);
        gtk_editable_insert_text(editable, symbol, -1, &position);
        gtk_editable_set_position(editable, position);
        gtk_widget_grab_focus(target);
    }
    static void changed(GtkWidget* widget, gpointer p) {
        auto& d = *static_cast<Dialog*>(p);
        if (d.loading || d.syncTableBusy)
            return;
        if (widget == d.entries["table-x"] || widget == d.entries["table-sign"] || widget == d.entries["table-value"])
            d.tableNeedsRebuild = true;
        gtk_widget_set_sensitive(d.insertButton, false);
        d.valid = false;
        ++d.previewGeneration;
        if (d.imported && (widget == d.entries["f1"] || widget == d.entries["f2"]))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d.confirmSource), false);
        if (d.timer)
            g_source_remove(d.timer);
        d.timer = g_timeout_add(
                140,
                +[](gpointer p) -> gboolean {
                    auto& d = *static_cast<Dialog*>(p);
                    d.timer = 0;
                    d.refresh();
                    return G_SOURCE_REMOVE;
                },
                p);
    }
    std::string text(const std::string& key) { return gtk_entry_get_text(GTK_ENTRY(entries.at(key))); }
    double numeric(const std::string& key) {
        auto s = text(key);
        try {
            if (key.rfind("coord-", 0) == 0 && s.find('m') != std::string::npos)
                throw std::runtime_error("Cần hằng số, không dùng biến m.");
            auto e = Expression(s, gtk_spin_button_get_value(GTK_SPIN_BUTTON(m)));
            auto v = e.at(0);
            if (e.dependsOnX() || !std::isfinite(v.y))
                throw std::runtime_error("Cần số hữu hạn, phân số hoặc pi.");
            return v.y;
        } catch (const std::exception& e) {
            mark(key, e.what());
            throw std::runtime_error(key + ": " + e.what());
        }
    }
    void mark(const std::string& key, const std::string& msg) {
        auto* e = GTK_ENTRY(entries.at(key));
        gtk_entry_set_icon_from_icon_name(e, GTK_ENTRY_ICON_SECONDARY, "dialog-error");
        gtk_entry_set_icon_tooltip_text(e, GTK_ENTRY_ICON_SECONDARY, msg.c_str());
    }
    bool coordinateMode() const { return gtk_combo_box_get_active(GTK_COMBO_BOX(mode)) >= 3; }
    void coordinateControls(GtkWidget* form) {
        coordinateBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
        pack(form, coordinateBox, "");
        auto combo = [&](const char* id, std::initializer_list<const char*> values) {
            auto* w = gtk_combo_box_text_new();
            for (auto v: values) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w), v);
            gtk_combo_box_set_active(GTK_COMBO_BOX(w), 0);
            pack(coordinateBox, w, id);
            atk_object_set_name(gtk_widget_get_accessible(w), id);
            g_signal_connect(w, "changed", G_CALLBACK(changed), this);
            return w;
        };
        coordinateDimension = combo("coord-dimension", {"Oxyz · tọa độ 3D thật", "Oxy · lấy x,y (bỏ z)"});
        g_signal_connect(coordinateDimension, "changed", G_CALLBACK(+[](GtkComboBox* w, gpointer p) {
                             auto& d = *static_cast<Dialog*>(p);
                             if (d.loading)
                                 return;
                             bool spatial = gtk_combo_box_get_active(w) == 0;
                             d.loading = true;
                             auto convert = [&](std::string s) {
                                 auto v = coordinate::parseVec(s, !spatial);
                                 std::ostringstream out;
                                 out.imbue(std::locale::classic());
                                 out << std::setprecision(17) << v.x << ";" << v.y;
                                 if (spatial)
                                     out << ";0";
                                 return out.str();
                             };
                             for (auto key: {"A", "B", "Q", "u", "v"}) try {
                                     auto id = std::string("coord-") + key;
                                     auto value = convert(d.text(id));
                                     gtk_entry_set_text(GTK_ENTRY(d.entries.at(id)), value.c_str());
                                 } catch (const std::exception&) {}
                             try {
                                 std::istringstream in(d.text("coord-forces"));
                                 std::string part;
                                 std::string result;
                                 while (std::getline(in, part, '|')) {
                                     if (!result.empty())
                                         result += " | ";
                                     result += convert(part);
                                 }
                                 gtk_entry_set_text(GTK_ENTRY(d.entries.at("coord-forces")), result.c_str());
                             } catch (const std::exception&) {}
                             d.loading = false;
                         }),
                         this);
        coordinatePreset =
                combo("coord-preset", {"Giữ dữ liệu hiện tại", "AB: (1;2;3) → (4;0;5)", "Tổng u, v và tích vô hướng",
                                       "Hai lực 10 N vuông góc", "Hai lực đối nhau",
                                       "Hai đường chéo nhau, khoảng cách 1", "Chiếu Q lên Ox", "Nhiều lực đồng quy"});
        g_signal_connect(coordinatePreset, "changed",
                         G_CALLBACK(+[](GtkComboBox*, gpointer p) { static_cast<Dialog*>(p)->coordinateExample(); }),
                         this);
        coordinateSource =
                combo("coord-source", {"Dùng vectơ u làm chỉ phương / thành phần", "Dùng hai điểm A, B (AB = B − A)"});
        coordinateOperation = combo("coord-operation", {"u + v · quy tắc cộng", "u − v", "k u", "−u · vectơ đối"});
        coordinateRule = combo("coord-rule", {"Quy tắc tam giác", "Quy tắc hình bình hành"});
        coordinateEquation = combo("coord-equation", {"Phương trình tham số", "Chính tắc + tọa độ cố định (Oxyz)"});
        forceInput = combo("coord-force-input", {"Lực theo thành phần x;y;z (N)", "Độ lớn; phương vị; góc nâng (độ)"});
        auto* hint = gtk_label_new(
                "Tọa độ dùng dấu ; (Oxy: x;y, Oxyz: x;y;z).\nLực cách nhau bằng |, tối đa 16 lực đồng quy.\nGóc phương "
                "vị: +Ox → +Oy; góc nâng từ Oxy.\nChỉ chọn điểm có tọa độ đã khai báo; không suy từ nét tay.");
        gtk_label_set_line_wrap(GTK_LABEL(hint), true);
        gtk_label_set_xalign(GTK_LABEL(hint), 0);
        pack(coordinateBox, hint, "");
        for (auto item: {std::pair{"A", "1;2;3"},
                         {"B", "4;0;5"},
                         {"u", "1;2;0"},
                         {"v", "3;-1;2"},
                         {"Q", "1;2;3"},
                         {"k", "2"},
                         {"name", "u"},
                         {"unit", ""},
                         {"forces", "10;0;0 | 0;10;0"},
                         {"axes", "0"},
                         {"view", "45"},
                         {"xscale", "1"},
                         {"yscale", "1"}}) {
            auto* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            std::map<std::string, std::string> names = {{"axes", "Trục phân tích (°)"},
                                                        {"view", "Góc nhìn Oxyz (°)"},
                                                        {"xscale", "Tỉ lệ hình x"},
                                                        {"yscale", "Tỉ lệ hình y"},
                                                        {"forces", "Lực (N)"},
                                                        {"name", "Tên vectơ"},
                                                        {"unit", "Đơn vị"}};
            auto it = names.find(item.first);
            auto* l = gtk_label_new(it == names.end() ? item.first : it->second.c_str());
            gtk_box_pack_start(GTK_BOX(row), l, false, false, 0);
            gtk_box_pack_start(GTK_BOX(row), entry(std::string("coord-") + item.first, item.second), true, true, 0);
            coordinateRows[item.first] = row;
            pack(coordinateBox, row, "");
        }
        coordinateComponents = toggle(coordinateBox, "Hiện các thành phần theo trục đã chọn");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(coordinateComponents), true);
        g_signal_connect(coordinateComponents, "toggled", G_CALLBACK(changed), this);
        coordinatePick = combo("coord-pick", {"Chạm điểm khai báo → đặt A", "Chạm điểm khai báo → đặt B",
                                              "Chạm điểm khai báo → đặt Q"});
        coordinateReport = gtk_label_new("");
        gtk_label_set_line_wrap(GTK_LABEL(coordinateReport), true);
        gtk_label_set_max_width_chars(GTK_LABEL(coordinateReport), 42);
        gtk_label_set_xalign(GTK_LABEL(coordinateReport), 0);
        pack(coordinateBox, coordinateReport, "coord-report");
        atk_object_set_name(gtk_widget_get_accessible(coordinateReport), "coord-report");
    }
    coordinate::Request coordinateRequest() {
        coordinate::Request r;
        r.mode = gtk_combo_box_get_active(GTK_COMBO_BOX(mode)) - 3;
        r.spatial = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateDimension)) == 0;
        r.fromPoints = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateSource)) == 1;
        r.operation = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateOperation));
        r.triangle = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateRule)) == 0;
        r.canonical = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateEquation)) == 1;
        r.showComponents = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(coordinateComponents));
        auto point = [&](const char* key) {
            try {
                return coordinate::parseVec(text(std::string("coord-") + key), r.spatial);
            } catch (const std::exception& e) {
                mark(std::string("coord-") + key, e.what());
                throw std::runtime_error(std::string(key) + ": " + e.what());
            }
        };
        // Read only the fields relevant to the active operation.
        r.a = r.b = r.q = {};
        if (r.mode == 1 || (r.mode == 0 && r.fromPoints)) {
            r.a = point("A");
            r.b = point("B");
        }
        if (r.mode == 1)
            r.q = point("Q");
        if (r.mode != 2) {
            if (!r.fromPoints)
                r.u = point("u");
            r.v = point("v");
        }
        if (!r.spatial) {
            r.u.z = r.v.z = 0;
        }
        r.scalar = r.mode == 0 && r.operation == 2 ? numeric("coord-k") : 1;
        r.axisDegrees = numeric("coord-axes");
        r.viewDegrees = numeric("coord-view");
        r.xScale = numeric("coord-xscale");
        r.yScale = numeric("coord-yscale");
        r.name = r.mode == 0 ? text("coord-name") : "u";
        r.unit = r.mode == 2 ? "N" : r.mode == 0 ? text("coord-unit") : "";
        if (r.mode == 2) {
            r.forces.clear();
            std::istringstream in(text("coord-forces"));
            std::string part;
            while (std::getline(in, part, '|')) {
                auto v = coordinate::parseVec(part, r.spatial);
                if (gtk_combo_box_get_active(GTK_COMBO_BOX(forceInput)) == 1)
                    v = coordinate::polar(v.x, v.y, r.spatial ? v.z : 0);
                r.forces.push_back(v);
            }
            if (text("coord-forces").empty() || text("coord-forces").back() == '|')
                throw std::runtime_error("VEC-005: Thiếu lực sau dấu |.");
        }
        return r;
    }
    void coordinateExample() {
        int i = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinatePreset));
        if (i == 0)
            return;
        loading = true;
        gtk_combo_box_set_active(GTK_COMBO_BOX(coordinateDimension), 0);
        auto set = [&](const char* k, const char* v) {
            gtk_entry_set_text(GTK_ENTRY(entries.at(std::string("coord-") + k)), v);
        };
        set("A", "1;2;3");
        set("B", "4;0;5");
        set("Q", "1;2;3");
        set("u", "1;2;0");
        set("v", "3;-1;2");
        set("axes", "0");
        set("xscale", "1");
        set("yscale", "1");
        set("k", "2");
        set("name", "u");
        set("unit", "");
        gtk_combo_box_set_active(GTK_COMBO_BOX(forceInput), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(coordinateSource), i == 1 ? 1 : 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(coordinateOperation), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(mode), i == 3 || i == 4 || i == 7 ? 5 : i == 5 || i == 6 ? 4 : 3);
        if (i == 3 || i == 4 || i == 7)
            set("forces", i == 3 ? "10;0;0 | 0;10;0" : i == 4 ? "10;0;0 | -10;0;0" : "10;0;0 | 0;10;0 | -3;-2;4");
        if (i == 5 || i == 6) {
            set("A", "0;0;0");
            set("B", "0;0;1");
            set("u", "1;0;0");
            set("v", "0;1;0");
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(coordinatePreset), 0);
        loading = false;
        refresh();
    }
    void pickCoordinate(double x, double y) {
        if (!coordinateMode() || !valid)
            return;
        double scale = std::min((gtk_widget_get_allocated_width(preview) - 12) / figure.width,
                                (gtk_widget_get_allocated_height(preview) - 12) / figure.height);
        double offset = (gtk_widget_get_allocated_width(preview) - figure.width * scale) / 2;
        std::vector<::Point> anchors;
        for (auto a: coordinateAnchors) anchors.emplace_back(offset + a.first.x * scale, 6 + a.first.y * scale);
        auto i = ElementSnapper::nearestAnchor(::Point(x, y), anchors,
                                               std::max(8., c->getSettings()->getSnapToElementsTolerance()));
        if (!i) {
            gtk_label_set_text(GTK_LABEL(error),
                               "Chạm gần dấu điểm A/B/Q/O/H đã khai báo. Không lấy tọa độ từ khoảng trống.");
            return;
        }
        for (size_t j = 0; j < anchors.size(); ++j)
            if (j != *i && anchors[j].lineLengthTo(anchors[*i]) < 1e-6 &&
                coordinate::length(coordinate::subtract(coordinateAnchors[j].second, coordinateAnchors[*i].second)) >
                        0) {
                gtk_label_set_text(GTK_LABEL(error),
                                   "Hai điểm khác tọa độ trùng hình chiếu; nhập tọa độ để chọn rõ điểm.");
                return;
            }
        const char* targets[] = {"coord-A", "coord-B", "coord-Q"};
        auto v = coordinateAnchors[*i].second;
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(17) << v.x << ";" << v.y;
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateDimension)) == 0)
            out << ";" << v.z;
        auto s = out.str();
        gtk_entry_set_text(GTK_ENTRY(entries.at(targets[gtk_combo_box_get_active(GTK_COMBO_BOX(coordinatePick))])),
                           s.c_str());
    }
    Request request() {
        Request r;
        r.pretty = true;
        r.formulaOnly = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(formulaOnly));
        r.m = gtk_spin_button_get_value(GTK_SPIN_BUTTON(m));
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(mode));
        r.mode = active == 2 ? 2 : 0;
        r.tangent = active == 1;
        r.curves.clear();
        for (int i = 0; i < 2; ++i) {
            GdkRGBA color;
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colors[i]), &color);
            uint32_t rgb = (static_cast<uint32_t>(std::lround(color.red * 255)) << 16) |
                           (static_cast<uint32_t>(std::lround(color.green * 255)) << 8) |
                           static_cast<uint32_t>(std::lround(color.blue * 255));
            r.curves.push_back({text("f" + std::to_string(i + 1)),
                                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enabled[i])) != 0, rgb});
            if (r.mode == 0 && r.curves.back().enabled)
                try {
                    Expression e(r.curves.back().formula, r.m);
                } catch (const std::exception& e) {
                    mark("f" + std::to_string(i + 1), e.what());
                    throw;
                }
        }
        if (r.mode == 2)
            r.manual = parseTable(text("table-x"), text("table-sign"), text("table-value"), text("table-name"));
        else if (!r.formulaOnly) {
            r.window = {numeric("xmin"), numeric("xmax"), numeric("ymin"), numeric("ymax")};
            if (r.window.xmin >= r.window.xmax) {
                mark("xmin", "x từ phải nhỏ hơn x đến.");
                mark("xmax", "x đến phải lớn hơn x từ.");
                throw std::runtime_error("Khoảng x: cần x từ < x đến.");
            }
            if (r.window.ymin >= r.window.ymax) {
                mark("ymin", "y từ phải nhỏ hơn y đến.");
                mark("ymax", "y đến phải lớn hơn y từ.");
                throw std::runtime_error("Khoảng y: cần y từ < y đến.");
            }
            r.window.validate();
            r.xtick = numeric("xtick");
            r.ytick = numeric("ytick");
            for (auto item: {std::pair{"xtick", r.xtick}, {"ytick", r.ytick}})
                if (item.second <= 0) {
                    mark(item.first, "Bước chia phải lớn hơn 0.");
                    throw std::runtime_error(std::string(item.first) + ": bước chia phải lớn hơn 0.");
                }
            r.x0 = r.tangent ? numeric("x0") : 0;
            r.xlabel = text("xlabel");
            r.ylabel = text("ylabel");
        }
        r.piTicks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pi));
        r.equalScale = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(equal));
        r.showSign = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(showSign));
        r.showValues = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(showValues));
        return r;
    }
    void refresh() {
        syncSourceControls();
        const int active = gtk_combo_box_get_active(GTK_COMBO_BOX(mode));
        bool coordinates = active >= 3;
        gtk_label_set_text(GTK_LABEL(titleText),
                           coordinates ? "Toán/Lý · Vectơ, đường thẳng, hợp lực" : "Công thức & đồ thị");
        gtk_label_set_text(GTK_LABEL(stepsText),
                           coordinates ? "NHẬP TỌA ĐỘ   →   XEM HÌNH VÀ KẾT QUẢ   →   CHÈN VÀO TRANG" :
                                         "QUÉT VÙNG PDF   →   ĐỐI CHIẾU CÔNG THỨC   →   CHÈN VÀO TRANG");
        gtk_widget_set_visible(coordinateBox, coordinates);
        for (auto* w: {curveEditor, parameterBox, preset, syntaxBox, sourceFrame})
            gtk_widget_set_visible(w, !coordinates);
        gtk_widget_set_visible(sourceCard, !coordinates && source.pdf != nullptr);
        if (coordinates) {
            if (sourceState == SourceState::Running) {
                cancelRecognition();
                sourceState = SourceState::Review;
            }
            gtk_widget_hide(tableBox);
            gtk_widget_hide(graphSettings);
            gtk_widget_hide(equationCard);
            gtk_widget_hide(formulaOnly);
            bool pointVector = active == 3 && gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateSource)) == 1;
            bool canPick = active == 4 || pointVector;
            gtk_label_set_text(GTK_LABEL(graphHeading),
                               gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateDimension)) == 0 ?
                                       "Oxyz · hình chiếu; xem góc và độ dài thật ở công thức" :
                                       "Oxy · tính theo tọa độ; hình tĩnh");
            gtk_widget_set_visible(coordinatePick, canPick);
            int pickCount = active == 4 ? 3 : 2;
            if (gtk_tree_model_iter_n_children(gtk_combo_box_get_model(GTK_COMBO_BOX(coordinatePick)), nullptr) !=
                pickCount) {
                bool wasLoading = loading;
                loading = true;
                int old = gtk_combo_box_get_active(GTK_COMBO_BOX(coordinatePick));
                gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(coordinatePick));
                for (auto name: {"Chạm điểm khai báo → đặt A", "Chạm điểm khai báo → đặt B"})
                    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(coordinatePick), name);
                if (active == 4)
                    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(coordinatePick), "Chạm điểm khai báo → đặt Q");
                gtk_combo_box_set_active(GTK_COMBO_BOX(coordinatePick), std::clamp(old, 0, pickCount - 1));
                loading = wasLoading;
            }
            gtk_widget_set_visible(coordinateOperation, active == 3);
            gtk_widget_set_visible(coordinateRule, active == 3);
            gtk_widget_set_visible(coordinateSource, active != 5);
            gtk_widget_set_visible(coordinateEquation, active == 4);
            gtk_widget_set_visible(forceInput, active == 5);
            for (auto key: {"A", "B"}) gtk_widget_set_visible(coordinateRows.at(key), canPick);
            gtk_widget_set_visible(coordinateRows.at("Q"), active == 4);
            for (auto [key, e]: entries)
                if (key.rfind("coord-", 0) == 0)
                    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(e), GTK_ENTRY_ICON_SECONDARY, nullptr);
            for (auto key: {"u", "v", "k", "name", "unit"})
                gtk_widget_set_visible(coordinateRows.at(key), active != 5 && (active == 3 || std::string(key) == "u" ||
                                                                               std::string(key) == "v"));
            gtk_widget_set_visible(coordinateRows.at("u"),
                                   active != 5 && gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateSource)) == 0);
            gtk_widget_set_visible(coordinateRows.at("k"),
                                   active == 3 && gtk_combo_box_get_active(GTK_COMBO_BOX(coordinateOperation)) == 2);
            gtk_widget_set_visible(coordinateComponents, active != 4);
            gtk_widget_set_visible(coordinateRows.at("forces"), active == 5);
            valid = false;
            ++previewGeneration;
            previewPending = false;
            previewGraphOnly = false;
            try {
                if (!contextValid())
                    throw std::runtime_error("Trang/lớp/zoom đã đổi. Đóng rồi mở lại bảng.");
                auto box = visible(c);
                auto scene = coordinate::generate(coordinateRequest(), box.width, box.height);
                coordinateAnchors = std::move(scene.anchors);
                figure = std::move(scene.figure);
                fitLabels(figure);
                gtk_label_set_text(GTK_LABEL(coordinateReport), scene.report.c_str());
                valid = true;
                gtk_label_set_text(GTK_LABEL(error),
                                   "Sẵn sàng · hình tĩnh; tính từ tọa độ gốc, độc lập tỉ lệ/góc nhìn.");
            } catch (const std::exception& e) {
                gtk_label_set_text(GTK_LABEL(error), e.what());
            }
            updateInsertState();
            return;
        }
        gtk_widget_set_visible(curveRows[1], active != 2);
        gtk_widget_set_visible(x0Label, active == 1);
        gtk_widget_set_visible(entries.at("x0"), active == 1);
        if (tableNeedsRebuild)
            rebuildTable();
        gtk_label_set_text(GTK_LABEL(graphHeading), active == 2 ? "Bảng biến thiên / xét dấu · xem trước" :
                                                    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(formulaOnly)) ?
                                                                  "Công thức vector sẽ chèn" :
                                                                  "Đồ thị xem trước · chưa thay đổi tài liệu");
        gtk_widget_set_visible(equationCard, active != 2);
        gtk_widget_set_visible(formulaOnly, active != 2);
        gtk_widget_set_visible(graphSettings,
                               active != 2 && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(formulaOnly)));
        gtk_widget_set_visible(tableBox, gtk_combo_box_get_active(GTK_COMBO_BOX(mode)) == 2);
        valid = false;
        for (auto [name, e]: entries)
            gtk_entry_set_icon_from_icon_name(GTK_ENTRY(e), GTK_ENTRY_ICON_SECONDARY, nullptr);
        try {
            if (!contextValid())
                throw std::runtime_error("Trang/lớp/zoom đã đổi. Đóng rồi mở lại bảng để xác nhận đích chèn.");
            auto r = request();
            auto box = visible(c);
            ++previewGeneration;
            if (previewBusy) {
                previewPending = true;
                updateInsertState();
                return;
            }
            previewBusy = true;
            previewPending = false;
            gtk_label_set_text(GTK_LABEL(error), "Đang dựng công thức và đồ thị…");
            struct Preview {
                Request request;
                Area box;
                unsigned version;
                std::weak_ptr<Lifetime> life;
                Figure figure;
                std::string error;
                std::vector<std::string> equations;
                std::vector<uint32_t> colors;
            };
            auto* data = new Preview{std::move(r), box, previewGeneration, lifetime, {}, {}};
            auto* task = g_task_new(
                    nullptr, nullptr,
                    +[](GObject*, GAsyncResult* result, gpointer) {
                        auto* data = static_cast<Preview*>(g_task_get_task_data(G_TASK(result)));
                        auto life = data->life.lock();
                        if (!life || !life->owner)
                            return;
                        auto& d = *life->owner;
                        d.previewBusy = false;
                        if (data->version != d.previewGeneration) {
                            if (d.previewPending) {
                                d.previewPending = false;
                                d.refresh();
                            }
                            return;
                        }
                        d.valid = data->error.empty() && d.contextValid();
                        if (d.valid) {
                            d.figure = std::move(data->figure);
                            d.previewGraphOnly = data->request.mode == 0 && !data->request.formulaOnly;
                            d.equationStrings = std::move(data->equations);
                            d.equationColors = std::move(data->colors);
                            gtk_label_set_text(
                                    GTK_LABEL(d.error),
                                    "Sẵn sàng · Chèn vào trang/lớp hiện tại.\nCtrl+Z hoàn tác cả cụm một lần.");
                        } else {
                            gtk_label_set_text(GTK_LABEL(d.error), data->error.c_str());
                            if (data->error.find("x₀") != std::string::npos)
                                d.mark("x0", data->error);
                        }
                        d.updateInsertState();
                        gtk_widget_queue_draw(d.preview);
                        gtk_widget_queue_draw(d.equationPreview);
                    },
                    nullptr);
            g_task_set_task_data(task, data, +[](gpointer p) { delete static_cast<Preview*>(p); });
            g_task_run_in_thread(
                    task, +[](GTask* task, gpointer, gpointer p, GCancellable*) {
                        auto& data = *static_cast<Preview*>(p);
                        try {
                            data.figure = generate(data.request, data.box.width, data.box.height);
                            fitLabels(data.figure);
                            if (data.request.mode == 0) {
                                for (size_t i = 0; i < data.request.curves.size(); ++i) {
                                    auto& curve = data.request.curves[i];
                                    if (!curve.enabled)
                                        continue;
                                    std::string eq = "f" + std::to_string(i + 1) +
                                                     (curve.formula.find('m') != std::string::npos ?
                                                              " (m: " + number(data.request.m) + ")" :
                                                              "") +
                                                     " = " + curve.formula;
                                    formulaMetrics(eq, 24);
                                    data.equations.push_back(std::move(eq));
                                    data.colors.push_back(curve.color);
                                }
                            }
                        } catch (const std::exception& e) {
                            data.error = e.what();
                        }
                        g_task_return_boolean(task, true);
                    });
            g_object_unref(task);
        } catch (const std::exception& e) {
            std::string message = e.what();
            if (message.find("x₀") != std::string::npos)
                mark("x0", message);
            if (message.find("Bảng") != std::string::npos)
                for (auto key: {"table-x", "table-sign", "table-value"}) mark(key, message);
            if (message.find("Miền x/y") != std::string::npos)
                for (auto key: {"xmin", "xmax", "ymin", "ymax"}) mark(key, message);
            gtk_label_set_text(GTK_LABEL(error), e.what());
        }
        updateInsertState();
        gtk_widget_queue_draw(preview);
    }
    void updateInsertState() {
        syncSourceControls();
        bool reviewed = coordinateMode() || !imported ||
                        (sourceApplied && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(confirmSource)));
        if (sourceState == SourceState::Ready && !reviewed)
            sourceState = SourceState::Review;
        if (valid && !coordinateMode() && imported && !reviewed)
            gtk_label_set_text(GTK_LABEL(error),
                               "Cần đối chiếu ảnh, công thức và đồ thị; đánh dấu xác nhận bên trái trước khi chèn.");
        else if (valid && !coordinateMode() && imported && reviewed && contextValid() &&
                 sourceState != SourceState::Running) {
            sourceState = SourceState::Ready;
            gtk_label_set_text(
                    GTK_LABEL(error),
                    "Đã đối chiếu · Sẵn sàng chèn vào trang/lớp đã xác nhận.\nCtrl+Z hoàn tác cả cụm một lần.");
        }
        gtk_widget_set_sensitive(insertButton,
                                 valid && contextValid() && reviewed && sourceState != SourceState::Running);
        gtk_widget_queue_draw(preview);
        gtk_widget_queue_draw(equationPreview);
    }
    void automatic() {
        try {
            auto t = variationTable(analyze(Expression(text("f1"), gtk_spin_button_get_value(GTK_SPIN_BUTTON(m)))));
            loading = true;
            for (auto pair: {std::pair{"table-x", t.x}, {"table-sign", t.sign}, {"table-value", t.value}}) {
                std::string s;
                for (size_t i = 0; i < pair.second.size(); ++i) {
                    if (i)
                        s += '|';
                    auto cell = pair.second[i];
                    if (cell == "||")
                        cell = "∥";
                    size_t pos = 0;
                    while ((pos = cell.find("||", pos)) != std::string::npos) {
                        cell.replace(pos, 2, "∥");
                        pos += 3;
                    }
                    s += cell;
                }
                gtk_entry_set_text(GTK_ENTRY(entries[pair.first]), s.c_str());
            }
            gtk_combo_box_set_active(GTK_COMBO_BOX(mode), 2);
            tableNeedsRebuild = true;
            loading = false;
            refresh();
        } catch (const std::exception& e) {
            mark("f1", e.what());
            gtk_label_set_text(GTK_LABEL(error), e.what());
        }
    }
    void choosePreset() {
        int index = gtk_combo_box_get_active(GTK_COMBO_BOX(preset));
        if (index <= 0) {
            if (index == 0) {
                loading = true;
                load();
                loading = false;
                refresh();
            }
            return;
        }
        const char* samples[][2] = {{"x^2+m*x", "m*x"},       {"x^3-3*x", "m*x"},      {"1/(x-m)", "(2*x+1)/(x+1)"},
                                    {"(x^2+1)/(x-m)", "m*x"}, {"sin(x)", "cos(x)"},    {"exp(x)", "ln(x)"},
                                    {"sqrt(x)", "abs(x)"},    {"(x^2-1)/(x-1)", "x+1"}};
        loading = true;
        for (int i = 0; i < 2; ++i)
            gtk_entry_set_text(GTK_ENTRY(entries["f" + std::to_string(i + 1)]), samples[index - 1][i]);
        gtk_combo_box_set_active(GTK_COMBO_BOX(mode), 0);
        loading = false;
        refresh();
    }
    void load() {
        tableNeedsRebuild = true;
        auto* k = g_key_file_new();
        if (g_key_file_load_from_file(k, path.c_str(), G_KEY_FILE_NONE, nullptr)) {
            for (auto [name, e]: entries) {
                auto* s = g_key_file_get_string(k, "entries", name.c_str(), nullptr);
                if (s) {
                    gtk_entry_set_text(GTK_ENTRY(e), s);
                    g_free(s);
                }
            }
            auto read = [&](const char* name, double fallback) {
                GError* err = nullptr;
                double v = g_key_file_get_double(k, "options", name, &err);
                if (err) {
                    g_error_free(err);
                    return fallback;
                }
                return std::isfinite(v) ? v : fallback;
            };
            for (auto item: {std::pair{"dimension", coordinateDimension},
                             {"source", coordinateSource},
                             {"operation", coordinateOperation},
                             {"rule", coordinateRule},
                             {"equation", coordinateEquation},
                             {"forceInput", forceInput}}) {
                int value = g_key_file_get_integer(k, "coordinate", item.first, nullptr);
                gtk_combo_box_set_active(GTK_COMBO_BOX(item.second),
                                         std::clamp(value, 0, std::string(item.first) == "operation" ? 3 : 1));
            }
            if (g_key_file_has_key(k, "coordinate", "components", nullptr))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(coordinateComponents),
                                             g_key_file_get_boolean(k, "coordinate", "components", nullptr));
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(m), read("m", 1));
            gtk_combo_box_set_active(GTK_COMBO_BOX(mode), static_cast<int>(std::clamp(read("mode", 0), 0., 5.)));
            for (auto item: {std::pair{"enabled1", enabled[0]},
                             {"enabled2", enabled[1]},
                             {"equal", equal},
                             {"pi", pi},
                             {"sign", showSign},
                             {"values", showValues},
                             {"formulaOnly", formulaOnly}})
                gtk_toggle_button_set_active(
                        GTK_TOGGLE_BUTTON(item.second),
                        read(item.first, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item.second))) != 0);
            for (int i = 0; i < 2; ++i) {
                auto* s = g_key_file_get_string(k, "options", ("color" + std::to_string(i)).c_str(), nullptr);
                GdkRGBA color;
                if (s && gdk_rgba_parse(&color, s))
                    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(colors[i]), &color);
                g_free(s);
            }
        }
        g_key_file_unref(k);
    }
    void save() {
        auto* k = g_key_file_new();
        for (auto [name, e]: entries)
            g_key_file_set_string(k, "entries", name.c_str(), gtk_entry_get_text(GTK_ENTRY(e)));
        g_key_file_set_double(k, "options", "m", gtk_spin_button_get_value(GTK_SPIN_BUTTON(m)));
        g_key_file_set_integer(k, "options", "mode", gtk_combo_box_get_active(GTK_COMBO_BOX(mode)));
        for (auto item: {std::pair{"dimension", coordinateDimension},
                         {"source", coordinateSource},
                         {"operation", coordinateOperation},
                         {"rule", coordinateRule},
                         {"equation", coordinateEquation},
                         {"forceInput", forceInput}})
            g_key_file_set_integer(k, "coordinate", item.first, gtk_combo_box_get_active(GTK_COMBO_BOX(item.second)));
        g_key_file_set_boolean(k, "coordinate", "components",
                               gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(coordinateComponents)));
        for (auto item: {std::pair{"enabled1", enabled[0]},
                         {"enabled2", enabled[1]},
                         {"equal", equal},
                         {"pi", pi},
                         {"sign", showSign},
                         {"values", showValues},
                         {"formulaOnly", formulaOnly}})
            g_key_file_set_double(k, "options", item.first,
                                  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item.second)));
        for (int i = 0; i < 2; ++i) {
            GdkRGBA color;
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colors[i]), &color);
            auto* s = gdk_rgba_to_string(&color);
            g_key_file_set_string(k, "options", ("color" + std::to_string(i)).c_str(), s);
            g_free(s);
        }
        GError* err = nullptr;
        if (!g_key_file_save_to_file(k, path.c_str(), &err)) {
            gtk_label_set_text(GTK_LABEL(error), err->message);
            g_error_free(err);
        }
        g_key_file_unref(k);
    }
    bool run() {
        int response;
        while ((response = gtk_dialog_run(GTK_DIALOG(win))) == GTK_RESPONSE_ACCEPT) {
            if (!valid || !contextValid() ||
                (!coordinateMode() && imported &&
                 (!sourceApplied || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(confirmSource)))) ||
                sourceState == SourceState::Running)
                continue;
            try {
                if (coordinateMode()) {
                    auto box = visible(c);
                    insertFigure(c, coordinate::generate(coordinateRequest(), box.width, box.height).figure);
                } else
                    insert(c, request());
                save();
                return false;
            } catch (const std::exception& e) {
                gtk_label_set_text(GTK_LABEL(error), e.what());
            }
        }
        return response == 1001;
    }
};
}  // namespace
void showTools(Control* c) {
    bool choose;
    {
        Dialog d(c);
        choose = d.run();
    }
    if (choose) {
        c->getWindow()->getPdfToolbox()->userCancelSelection();
        c->getActionDatabase()->fireChangeActionState(Action::SELECT_TOOL, TOOL_SELECT_PDF_TEXT_RECT);
    }
}
}  // namespace teaching
