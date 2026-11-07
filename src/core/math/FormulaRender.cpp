// SPDX-License-Identifier: GPL-2.0-or-later
#include "FormulaRender.h"

#include <algorithm>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <tuple>

#include <cairo-pdf.h>
#include <pango/pangocairo.h>

#include "FormulaImport.h"
#include "TeachingMath.h"
namespace teaching {
namespace {
struct Box {
    double w = 0, up = 0, down = 0;
    std::function<void(cairo_t*, double, double)> draw;
};
Box glyph(const std::string& text, double size) {
    auto* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    auto* cr = cairo_create(surface);
    auto* l = pango_cairo_create_layout(cr);
    auto* font = pango_font_description_from_string("Latin Modern Math, STIX Two Math, DejaVu Serif");
    pango_font_description_set_absolute_size(font, size * PANGO_SCALE);
    pango_layout_set_font_description(l, font);
    pango_layout_set_text(l, text.c_str(), -1);
    PangoRectangle ink, logical;
    pango_layout_get_extents(l, &ink, &logical);
    double baseline = pango_layout_get_baseline(l) / double(PANGO_SCALE);
    double w = logical.width / double(PANGO_SCALE), h = logical.height / double(PANGO_SCALE);
    pango_font_description_free(font);
    g_object_unref(l);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return {w, baseline, h - baseline, [text, size, baseline](cairo_t* c, double x, double y) {
                auto* p = pango_cairo_create_layout(c);
                auto* f = pango_font_description_from_string("Latin Modern Math, STIX Two Math, DejaVu Serif");
                pango_font_description_set_absolute_size(f, size * PANGO_SCALE);
                pango_layout_set_font_description(p, f);
                pango_layout_set_text(p, text.c_str(), -1);
                cairo_move_to(c, x, y - baseline);
                pango_cairo_show_layout(c, p);
                pango_font_description_free(f);
                g_object_unref(p);
            }};
}
Box join(Box a, Box b, double gap = 0) {
    return {a.w + b.w + gap, std::max(a.up, b.up), std::max(a.down, b.down),
            [a, b, gap](cairo_t* c, double x, double y) {
                a.draw(c, x, y);
                b.draw(c, x + a.w + gap, y);
            }};
}
int priority(char op) { return op == '+' || op == '-' ? 1 : op == '*' || op == '/' ? 2 : op == '^' ? 3 : 4; }
Box layout(std::shared_ptr<const MathNotation> n, double size, int parent = 0) {
    if (size < 4)
        throw std::runtime_error("Công thức lồng quá nhỏ để đọc. Chọn biểu thức ngắn hơn hoặc tăng khung.");
    Box r;
    if (n->op == 'n' || n->op == 'x')
        r = glyph(n->op == 'x' ? "𝑥" : n->text == "m" ? "𝑚" : n->text, size);
    else if (n->op == 'j')
        r = join(layout(n->a, size), layout(n->b, size), size * .15);
    else if (n->op == 'h') {
        auto a = layout(n->a, size);
        double lead = size * .65;
        r = {a.w + lead, a.up, a.down, [a, lead](cairo_t* c, double x, double y) {
                 a.draw(c, x + lead, y);
                 double top = y - a.up, bottom = y + a.down, mid = (top + bottom) / 2;
                 cairo_set_line_width(c, .7);
                 cairo_move_to(c, x + lead * .8, top);
                 cairo_curve_to(c, x + lead * .2, top, x + lead * .7, mid - 3, x, mid);
                 cairo_curve_to(c, x + lead * .7, mid + 3, x + lead * .2, bottom, x + lead * .8, bottom);
                 cairo_stroke(c);
             }};
    } else if (n->op == 'v') {
        auto a = layout(n->a, size);
        double top = -a.up - size * .20;
        r = {std::max(a.w, size * .7), -top + 2, a.down, [a, top, size](cairo_t* c, double x, double y) {
                 a.draw(c, x, y);
                 double w = std::max(a.w, size * .7);
                 cairo_set_line_width(c, .7);
                 cairo_move_to(c, x, y + top);
                 cairo_line_to(c, x + w, y + top);
                 cairo_line_to(c, x + w - size * .22, y + top - size * .12);
                 cairo_move_to(c, x + w, y + top);
                 cairo_line_to(c, x + w - size * .22, y + top + size * .12);
                 cairo_stroke(c);
             }};
    } else if (n->op == 's') {
        auto a = layout(n->a, size), b = layout(n->b, size);
        double shift = a.down + b.up + size * .3;
        r = {std::max(a.w, b.w), a.up, shift + b.down, [a, b, shift](cairo_t* c, double x, double y) {
                 a.draw(c, x, y);
                 b.draw(c, x, y + shift);
             }};
    } else if (n->op == '/') {
        auto a = layout(n->a, size * .88), b = layout(n->b, size * .88);
        double w = std::max(a.w, b.w) + size * .4;
        double axis = -size * .23, gap = size * .14;
        double ay = axis - gap - a.down, by = axis + gap + b.up;
        r = {w, -ay + a.up, by + b.down, [a, b, w, ay, by, axis, size](cairo_t* c, double x, double y) {
                 a.draw(c, x + (w - a.w) / 2, y + ay);
                 b.draw(c, x + (w - b.w) / 2, y + by);
                 cairo_set_line_width(c, std::max(.5, size * .045));
                 cairo_move_to(c, x, y + axis);
                 cairo_line_to(c, x + w, y + axis);
                 cairo_stroke(c);
             }};
    } else if (n->op == '^') {
        auto a = layout(n->a, size, 4), b = layout(n->b, size * .7);
        double shift = a.up * .72;
        r = {a.w + b.w + 1, std::max(a.up, shift + b.up), std::max(a.down, b.down - shift),
             [a, b, shift](cairo_t* c, double x, double y) {
                 a.draw(c, x, y);
                 b.draw(c, x + a.w + 1, y - shift);
             }};
    } else if (n->op == 'f') {
        auto a = layout(n->a, size);
        if (n->text == "sqrt") {
            double lead = size * .8, top = -a.up - size * .12;
            r = {a.w + lead + size * .2, -top + size * .12, a.down,
                 [a, lead, top, size](cairo_t* c, double x, double y) {
                     a.draw(c, x + lead, y);
                     cairo_set_line_width(c, std::max(.55, size * .05));
                     cairo_move_to(c, x + size * .05, y - size * .15);
                     cairo_line_to(c, x + size * .22, y - size * .27);
                     cairo_line_to(c, x + size * .4, y + a.down);
                     cairo_line_to(c, x + lead - size * .08, y + top);
                     cairo_line_to(c, x + lead + a.w + size * .18, y + top);
                     cairo_stroke(c);
                 }};
        } else if (n->text == "abs") {
            r = {a.w + size * .5, a.up, a.down, [a, size](cairo_t* c, double x, double y) {
                     a.draw(c, x + size * .25, y);
                     cairo_set_line_width(c, size * .045);
                     for (double xx: {x + size * .1, x + a.w + size * .4}) {
                         cairo_move_to(c, xx, y - a.up);
                         cairo_line_to(c, xx, y + a.down);
                     }
                     cairo_stroke(c);
                 }};
        } else
            r = join(glyph(n->text, size), join(glyph("(", size), join(a, glyph(")", size))), size * .12);
    } else {
        auto a = layout(n->a, size, priority(n->op));
        auto b = layout(n->b, size, priority(n->op) + (n->op == '-' ? 1 : 0));
        // The unary minus is encoded as 0 - expression in the shared AST.
        bool unary = n->op == '-' && n->a->op == 'n' && n->a->text == "0";
        auto op = glyph(n->op == '*' ? "·" : n->op == '-' ? "−" : "+", size);
        r = unary ? join(op, b, size * .08) : join(join(a, op, size * .15), b, size * .15);
    }
    // Fractions have their own grouping. Power bases need parentheses for powers
    // and negative constants as well as sums, preserving -x² versus (-x)².
    if ((priority(n->op) < parent && n->op != '/') ||
        (parent == 4 && n->op == 'n' && !n->text.empty() && n->text[0] == '-'))
        r = join(glyph("(", size), join(r, glyph(")", size)));
    return r;
}
Box equation(const std::string& text, double size) {
    auto semi = text.find(';');
    if (semi != std::string::npos && semi + 1 < text.size())
        return join(join(equation(text.substr(0, semi), size), glyph(";", size), size * .2),
                    equation(text.substr(semi + 1), size), size * .4);
    auto p = text.find('=');
    size_t length = 1;
    if (p == std::string::npos) {
        p = text.find("≈");
        length = 3;
    }
    if (p == std::string::npos)
        return layout(Expression(text).notation(), size);
    auto left = text.substr(0, p), right = text.substr(p + length);
    if (left.rfind("f1", 0) == 0)
        left.replace(0, 2, "f₁");
    else if (left.rfind("f2", 0) == 0)
        left.replace(0, 2, "f₂");
    auto normalized = importFormula(right);
    if (!normalized.usable())
        throw std::runtime_error(normalized.message);
    // Prefixes are generated by the application, not interpreted as TeX.
    return join(glyph(left + (length == 1 ? " =" : " ≈"), size),
                layout(Expression(normalized.expression).notation(), size), size * .2);
}
Box cachedEquation(const std::string& s, double size) {
    static std::mutex mutex;
    static std::map<std::pair<std::string, double>, Box> cache;
    std::lock_guard lock(mutex);
    auto key = std::make_pair(s, size);
    auto i = cache.find(key);
    if (i != cache.end())
        return i->second;
    auto box = equation(s, size);
    if (cache.size() >= 128)
        cache.clear();
    cache[key] = box;
    return box;
}
}  // namespace
FormulaMetrics formulaMetrics(const std::string& s, double size) {
    auto b = cachedEquation(s, size);
    return {b.w + 4, b.up + b.down + 4};
}
void paintFormula(cairo_t* c, const std::string& s, double size) {
    auto b = cachedEquation(s, size);
    b.draw(c, 2, b.up + 2);
}
std::string formulaLatex(const std::string& s) {
    auto semi = s.find(';');
    if (semi != std::string::npos && semi + 1 < s.size())
        return formulaLatex(s.substr(0, semi)) + ";\\quad " + formulaLatex(s.substr(semi + 1));
    auto p = s.find('=');
    size_t length = 1;
    if (p == std::string::npos) {
        p = s.find("≈");
        length = 3;
    }
    auto c = importFormula(p == std::string::npos ? s : s.substr(p + length));
    if (!c.usable())
        throw std::runtime_error(c.message);
    std::string prefix;
    if (p != std::string::npos) {
        auto left = s.substr(0, p);
        auto last = left.find_last_not_of(" ");
        prefix = last != std::string::npos && left[last] == 'y' ? "y" :
                 last != std::string::npos && left[last] == 'x' ? "x" :
                                                                  "f(x)";
        prefix += length == 1 ? "=" : "\\approx ";
    }
    std::string parameter;
    auto mp = s.find("(m: ");
    if (mp != std::string::npos && p != std::string::npos && mp < p) {
        auto end = s.find(')', mp);
        if (end != std::string::npos) {
            auto value = s.substr(mp + 4, end - mp - 4);
            parameter = "\\quad (m=" + Expression(value).latex() + ")";
        }
    }
    return prefix + Expression(c.expression).latex() + parameter;
}
std::string formulaPdf(const std::string& s, double size, uint32_t color) {
    // Bounded in-memory cache stores only formulas explicitly typed/confirmed;
    // source PDF crops never enter a persistent cache or log.
    static std::mutex mutex;
    static std::map<std::tuple<std::string, double, uint32_t>, std::string> cache;
    std::lock_guard lock(mutex);
    auto key = std::make_tuple(s, size, color);
    auto it = cache.find(key);
    if (it != cache.end())
        return it->second;
    auto m = formulaMetrics(s, size);
    std::string data;
    auto write = +[](void* p, const unsigned char* b, unsigned n) -> cairo_status_t {
        static_cast<std::string*>(p)->append(reinterpret_cast<const char*>(b), n);
        return CAIRO_STATUS_SUCCESS;
    };
    auto* surface = cairo_pdf_surface_create_for_stream(write, &data, m.width, m.height);
    auto* c = cairo_create(surface);
    cairo_set_source_rgb(c, ((color >> 16) & 255) / 255., ((color >> 8) & 255) / 255., (color & 255) / 255.);
    paintFormula(c, s, size);
    cairo_destroy(c);
    cairo_surface_finish(surface);
    auto status = cairo_surface_status(surface);
    cairo_surface_destroy(surface);
    if (status != CAIRO_STATUS_SUCCESS)
        throw std::runtime_error("Không dựng được PDF công thức.");
    if (cache.size() >= 64)
        cache.clear();
    cache[key] = data;
    return data;
}
FormulaMetrics formulaMetrics(std::shared_ptr<const MathNotation> n, double size) {
    auto b = layout(n, size);
    return {b.w + 4, b.up + b.down + 4};
}
void paintFormula(cairo_t* c, std::shared_ptr<const MathNotation> n, double size) {
    auto b = layout(n, size);
    b.draw(c, 2, b.up + 2);
}
std::string formulaPdf(std::shared_ptr<const MathNotation> n, double size, uint32_t color) {
    auto m = formulaMetrics(n, size);
    std::string data;
    auto write = +[](void* p, const unsigned char* b, unsigned count) -> cairo_status_t {
        static_cast<std::string*>(p)->append(reinterpret_cast<const char*>(b), count);
        return CAIRO_STATUS_SUCCESS;
    };
    auto* surface = cairo_pdf_surface_create_for_stream(write, &data, m.width, m.height);
    auto* cr = cairo_create(surface);
    cairo_set_source_rgb(cr, ((color >> 16) & 255) / 255., ((color >> 8) & 255) / 255., (color & 255) / 255.);
    paintFormula(cr, n, size);
    cairo_destroy(cr);
    cairo_surface_finish(surface);
    auto status = cairo_surface_status(surface);
    cairo_surface_destroy(surface);
    if (status != CAIRO_STATUS_SUCCESS)
        throw std::runtime_error("VEC-006: Không dựng được PDF công thức.");
    return data;
}
std::string notationLatex(std::shared_ptr<const MathNotation> n) {
    if (n->op == 'h')
        return "\\left\\{" + notationLatex(n->a) + "\\right.";
    if (n->op == 'v')
        return "\\overrightarrow{" + notationLatex(n->a) + "}";
    if (n->op == 'j')
        return notationLatex(n->a) + " " + notationLatex(n->b);
    if (n->op == 's')
        return "\\begin{gathered}" + notationLatex(n->a) + "\\\\" + notationLatex(n->b) + "\\end{gathered}";
    if (n->op == '/')
        return "\\frac{" + notationLatex(n->a) + "}{" + notationLatex(n->b) + "}";
    if (n->op == 'f' && n->text == "sqrt")
        return "\\sqrt{" + notationLatex(n->a) + "}";
    // Coordinate builders supply only application-generated numbers, ASCII names and symbols.
    std::string s = n->text;
    for (const auto& pair:
         {std::pair{"≈", "\\approx "}, {"·", "\\cdot "}, {"°", "^{\\circ}"}, {"−", "-"}, {"∈", "\\in "}}) {
        size_t p = 0;
        while ((p = s.find(pair.first, p)) != std::string::npos) {
            s.replace(p, std::string(pair.first).size(), pair.second);
            p += std::string(pair.second).size();
        }
    }
    return s;
}
}  // namespace teaching
