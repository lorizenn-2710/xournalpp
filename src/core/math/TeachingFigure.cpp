// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "TeachingMath.h"

namespace teaching {
namespace {
constexpr double pi = 3.14159265358979323846;
void check(bool b, const std::string& s) {
    if (!b)
        throw std::runtime_error(s);
}
std::string fraction(double v) {
    for (int d = 1; d <= 24; ++d) {
        double n = std::round(v * d);
        if (std::abs(v * d - n) < 1e-8)
            return number(n) + (d == 1 ? "" : "/" + std::to_string(d));
    }
    return number(v);
}
std::string precise(double v) {
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << std::setprecision(17) << v;
    return s.str();
}
std::string affine(double slope, double intercept) {
    if (slope == 0)
        return number(intercept);
    std::string result = slope == 1 ? "x" : slope == -1 ? "−x" : (slope < 0 ? "−" : "") + number(std::abs(slope)) + "x";
    if (intercept != 0)
        result += (intercept < 0 ? " − " : " + ") + number(std::abs(intercept));
    return result;
}
std::string equality(double slope, double intercept) {
    return std::floor(slope) == slope && std::floor(intercept) == intercept ? "= " : "≈ ";
}
size_t chars(const std::string& s) {
    size_t n = 0;
    for (unsigned char c: s)
        if ((c & 0xc0) != 0x80)
            ++n;
    return n;
}
}  // namespace
Figure generate(const Request& r, double width, double height) {
    check(std::isfinite(width) && std::isfinite(height) && width >= 240 && height >= 160,
          "Vùng đang thấy quá nhỏ; hãy giảm zoom.");
    Figure f;
    f.width = width;
    f.height = height;
    auto line = [&](std::vector<Point> p, uint32_t color = 0x202020, double weight = 1, bool dashed = false) {
        f.strokes.push_back({std::move(p), color, weight, dashed});
    };
    auto label = [&](double x, double y, const std::string& s, double size = 10, uint32_t color = 0x202020,
                     double maxWidth = 0, TextAnchor anchor = TextAnchor::Left) {
        if (!s.empty())
            f.labels.push_back({{x, y}, s, size, color, maxWidth, anchor});
    };
    if (r.formulaOnly && r.mode == 0) {
        check(r.curves.size() <= 2, "Tối đa hai công thức.");
        size_t n = 0;
        for (const auto& curve: r.curves)
            if (curve.enabled) {
                Expression checked(curve.formula, r.m);
                label(8, 8 + 44 * static_cast<double>(n), "f" + std::to_string(n + 1) + " = " + curve.formula, 18,
                      curve.color, width - 16);
                f.labels.back().formula =
                        "f" + std::to_string(n + 1) +
                        (curve.formula.find('m') != std::string::npos ? " (m: " + number(r.m) + ")" : "") + " = " +
                        curve.formula;
                ++n;
            }
        check(n > 0, "Bật ít nhất một hàm.");
        f.height = 16 + 44 * static_cast<double>(n);
        return f;
    }
    if (r.mode != 0) {
        Table t = r.mode == 1 ? variationTable(analyze(Expression(r.curves.at(0).formula, r.m))) : r.manual;
        check(t.x.size() >= 2 && t.x.size() == t.sign.size() && t.x.size() == t.value.size() && t.x.size() <= 19,
              "Các hàng bảng phải cùng số ô (2..19).");
        const double left = 8, top = 32, row = (height - 50) / 3, head = 35,
                     cell = (width - 16 - head) / static_cast<double>(t.x.size());
        label(8, 8, r.mode == 1 ? "Bảng biến thiên: " + r.curves[0].formula : "Bảng xét dấu / biến thiên", 10);
        for (int i = 0; i <= 3; ++i) line({{left, top + i * row}, {width - 8, top + i * row}});
        line({{left, top}, {left, top + 3 * row}});
        line({{left + head, top}, {left + head, top + 3 * row}});
        line({{width - 8, top}, {width - 8, top + 3 * row}});
        label(left + 5, top + row * .4, "x");
        if (r.showSign)
            label(left + 4, top + row * 1.4, t.name, 10, 0x202020, head - 6);
        if (r.showValues)
            label(left + 5, top + row * 2.4, "f");
        for (size_t j = 0; j < t.x.size(); ++j) {
            double x = left + head + static_cast<double>(j) * cell;
            for (int i = 0; i < 3; ++i) {
                if ((i == 1 && !r.showSign) || (i == 2 && !r.showValues))
                    continue;
                const std::string& text = (i == 0 ? t.x : i == 1 ? t.sign : t.value)[j];
                bool excluded = t.sign[j] == "||" || t.sign[j] == "∥";
                if (i == 1 && excluded)
                    continue;
                if (i == 2 && excluded) {
                    auto split = text.find("||");
                    size_t count = 2;
                    if (split == std::string::npos) {
                        split = text.find("∥");
                        count = 3;
                    }
                    if (split != std::string::npos) {
                        double halfWidth = cell / 2 - 7;
                        label(x + cell / 2 - 6, top + (i + .4) * row, text.substr(0, split), 11, 0x202020, halfWidth,
                              TextAnchor::Right);
                        label(x + cell / 2 + 5, top + (i + .4) * row, text.substr(split + count), 11, 0x202020,
                              halfWidth);
                        continue;
                    }
                }
                double fs = std::min(11., (cell - 4) / std::max(1., .65 * static_cast<double>(chars(text))));
                check(fs >= 4 || text.empty(), "Bảng quá chật; bớt cột hoặc rút ngắn nội dung ô.");
                if (i == 2 && excluded)
                    label(x + cell / 2 - 6, top + (i + .4) * row, text, fs, 0x202020, cell / 2 - 7, TextAnchor::Right);
                else
                    label(x + cell / 2, top + (i + .4) * row, text, fs, 0x202020, cell - 4, TextAnchor::Center);
            }
            if (t.sign[j] == "||" || t.sign[j] == "∥") {
                line({{x + cell / 2 - 2, top + row}, {x + cell / 2 - 2, top + 3 * row}});
                line({{x + cell / 2 + 2, top + row}, {x + cell / 2 + 2, top + 3 * row}});
            }
        }
        return f;
    }
    r.window.validate();
    check(r.curves.size() <= 2, "Tối đa hai hàm trong cùng khung.");
    check(std::isfinite(r.xtick) && std::isfinite(r.ytick) && r.xtick > 0 && r.ytick > 0,
          "Bước chia phải dương, hữu hạn.");
    check(!r.tangent || (!r.curves.empty() && r.curves[0].enabled), "Bật hàm 1 để dựng tiếp tuyến.");
    check(chars(r.xlabel) <= 6 && chars(r.ylabel) <= 6, "Tên trục tối đa 6 ký tự.");
    const auto& w = r.window;
    double plotW = width - 100, plotH = height - (r.pretty ? 140 : 100);
    if (r.equalScale) {
        double s = std::min(plotW / (w.xmax - w.xmin), plotH / (w.ymax - w.ymin));
        plotW = s * (w.xmax - w.xmin);
        plotH = s * (w.ymax - w.ymin);
    }
    double left = (width - plotW) / 2, top = (r.pretty ? 88 : 48) + (height - (r.pretty ? 140 : 100) - plotH) / 2;
    auto map = [&](double x, double y) {
        return Point{left + (x - w.xmin) * plotW / (w.xmax - w.xmin), top + (w.ymax - y) * plotH / (w.ymax - w.ymin)};
    };
    double ax = std::clamp(0., w.xmin, w.xmax), ay = std::clamp(0., w.ymin, w.ymax);
    auto origin = map(ax, ay);
    line({{left, origin.y}, {left + plotW, origin.y}});
    line({{origin.x, top + plotH}, {origin.x, top}});
    line({{left + plotW - 6, origin.y - 3}, {left + plotW, origin.y}, {left + plotW - 6, origin.y + 3}});
    line({{origin.x - 3, top + 6}, {origin.x, top}, {origin.x + 3, top + 6}});
    label(left + plotW + 10, origin.y - 5, r.xlabel);
    label(origin.x + 8, top - 9, r.ylabel);
    if (w.xmin <= 0 && w.xmax >= 0 && w.ymin <= 0 && w.ymax >= 0)
        label(origin.x + 5, origin.y + 5, "O", 9);
    // Preserve the requested scale; thin ticks/labels by integer multiples only.
    auto ticks = [&](bool horizontal) {
        double step = horizontal ? r.xtick * (r.piTicks ? pi : 1) : r.ytick;
        double lo = horizontal ? w.xmin : w.ymin, hi = horizontal ? w.xmax : w.ymax, span = horizontal ? plotW : plotH;
        double stride = std::max(1., std::ceil((hi - lo) / step / std::max(1., span / (horizontal ? 65. : 28.))));
        step *= stride;
        check(std::isfinite(step) && step > 0, "Bước chia vượt phạm vi.");
        double first = std::ceil(lo / step);
        for (int i = 0; i < 64; ++i) {
            double v = (first + i) * step;
            if (v > hi)
                break;
            if (v == 0)
                continue;
            auto p = horizontal ? map(v, ay) : map(ax, v);
            std::string s = horizontal && r.piTicks ?
                                    (v < 0 ? "−" : "") + (std::abs(v / pi) == 1 ? std::string("π") :
                                                                                  fraction(std::abs(v / pi)) + "π") :
                                    fraction(v);
            if (horizontal) {
                line({{p.x, p.y - 3}, {p.x, p.y + 3}});
                label(p.x, p.y + 7, s, 9, 0x202020, 0, TextAnchor::Center);
            } else {
                line({{p.x - 3, p.y}, {p.x + 3, p.y}});
                label(p.x - 8, p.y - 5, s, 9, 0x202020, 0, TextAnchor::Right);
            }
        }
    };
    ticks(true);
    ticks(false);
    unsigned curveIndex = 0;
    for (const auto& c: r.curves) {
        ++curveIndex;
        if (!c.enabled)
            continue;
        Expression e(c.formula, r.m);
        auto sampled = sample(e, w, plotW, plotH);
        check(!sampled.limited, "Hàm " + std::to_string(curveIndex) +
                                        ": vượt ngân sách 30000 mẫu / 180 ms; thu hẹp miền hoặc giảm dao động.");
        check(sampled.defined > 0, "Hàm " + std::to_string(curveIndex) +
                                           ": không có giá trị hữu hạn trong miền x (kiểm tra mẫu / miền xác định).");
        std::optional<Analysis> survey;
        try {
            survey = analyze(e);
        } catch (const std::runtime_error&) {}
        for (auto& branch: sampled.branches) {
            std::vector<Point> run;
            auto flush = [&] {
                if (run.size() > 1)
                    line(std::move(run), c.color, 1.3);
                run.clear();
            };
            for (auto p: branch) {
                p = map(p.x, p.y);
                bool inHole = false;
                if (survey)
                    for (auto& b: survey->points)
                        if (b.hole) {
                            auto h = map(b.x, b.value);
                            inHole |= std::hypot(p.x - h.x, p.y - h.y) < 3.3;
                        }
                if (inHole)
                    flush();
                else
                    run.push_back(p);
            }
            flush();
        }
        label(left, 5 + (curveIndex - 1) * 16,
              "f" + std::to_string(curveIndex) + " = " + c.formula +
                      (c.formula.find('m') != std::string::npos ? " (m=" + number(r.m) + ")" : ""),
              10, c.color);
        if (r.pretty) {
            f.labels.back().pos.y = 4 + (curveIndex - 1) * 36;
            f.labels.back().size = 15;
            f.labels.back().maxWidth = width - left - 12;
            f.labels.back().formula = "f" + std::to_string(curveIndex) +
                                      (c.formula.find('m') != std::string::npos ? " (m: " + number(r.m) + ")" : "") +
                                      " = " + c.formula;
        }
        // The certified rational analyzer provides asymptotes and original holes.
        if (survey) {
            const auto& a = *survey;
            std::string asymptoteText;
            for (double x: a.verticalAsymptotes) asymptoteText += "x = " + number(x) + "; ";
            if (a.hasAsymptote && std::none_of(a.points.begin(), a.points.end(), [](const auto& p) { return p.hole; }))
                asymptoteText += "y " + equality(a.slope, a.intercept) + affine(a.slope, a.intercept);
            if (!asymptoteText.empty())
                label(left, height - 50 + (curveIndex - 1) * 13, "TC: " + asymptoteText, 9, c.color);
            for (double x: a.verticalAsymptotes)
                if (x > w.xmin && x < w.xmax)
                    line({map(x, w.ymin), map(x, w.ymax)}, c.color, .7, true);
            if (a.hasAsymptote &&
                std::none_of(a.points.begin(), a.points.end(), [](const auto& p) { return p.hole; })) {
                Expression asym(precise(a.slope) + "*x+(" + precise(a.intercept) + ")");
                for (auto b: sample(asym, w, plotW, plotH).branches) {
                    for (auto& p: b) p = map(p.x, p.y);
                    line(std::move(b), c.color, .7, true);
                }
            }
            for (auto& b: a.points)
                if (b.hole && b.x >= w.xmin && b.x <= w.xmax && b.value >= w.ymin && b.value <= w.ymax) {
                    auto p = map(b.x, b.value);
                    std::vector<Point> ring;
                    for (int i = 0; i <= 32; ++i)
                        ring.push_back({p.x + 3 * std::cos(i * pi / 16), p.y + 3 * std::sin(i * pi / 16)});
                    line(std::move(ring), c.color, 1);
                    label(p.x + 5, p.y - 13, "○ (" + number(b.x) + "; " + number(b.value) + ")", 9, c.color);
                }
        }
        if (r.tangent && curveIndex == 1) {
            check(std::isfinite(r.x0) && r.x0 >= w.xmin && r.x0 <= w.xmax, "x₀ phải nằm trong miền x đang vẽ.");
            auto v = e.at(r.x0);
            check(std::isfinite(v.y), "x₀ bị loại khỏi miền xác định.");
            check(v.y >= w.ymin && v.y <= w.ymax, "Điểm M ngoài cửa sổ y; hãy mở rộng miền y.");
            check(!std::isnan(v.derivative), "Không có tiếp tuyến duy nhất / đạo hàm chưa xác định tại x₀.");
            auto p = map(r.x0, v.y);
            line({map(r.x0, ay), p, map(ax, v.y)}, c.color, .7, true);
            std::vector<Point> dot;
            for (int i = 0; i <= 16; ++i)
                dot.push_back({p.x + 2 * std::cos(i * pi / 8), p.y + 2 * std::sin(i * pi / 8)});
            line(std::move(dot), c.color, 1);
            label(p.x + 7, p.y - 18, "M(" + number(r.x0) + "; " + number(v.y) + ")", 9, c.color);
            if (std::isinf(v.derivative)) {
                line({map(r.x0, w.ymin), map(r.x0, w.ymax)}, 0xc42b35, 1);
                label(left, height - 22, "Tiếp tuyến đứng: x = " + number(r.x0), 10);
            } else {
                double intercept = v.y - v.derivative * r.x0;
                check(std::isfinite(intercept), "Tiếp tuyến tràn số.");
                Expression tangent(precise(v.derivative) + "*x+(" + precise(intercept) + ")");
                for (auto b: sample(tangent, w, plotW, plotH).branches) {
                    for (auto& pp: b) pp = map(pp.x, pp.y);
                    line(std::move(b), 0xc42b35, 1);
                }
                label(left, height - 22,
                      std::string("Tiếp tuyến: y ") + equality(v.derivative, intercept) +
                              affine(v.derivative, intercept),
                      10);
            }
        }
    }
    if (r.pretty) {
        for (auto& l: f.labels) {
            if (l.text.rfind("Tiếp tuyến", 0) == 0 || l.text.rfind("TC:", 0) == 0) {
                l.formula = l.text;
                while (!l.formula.empty() && (l.formula.back() == ' ' || l.formula.back() == ';')) l.formula.pop_back();
                l.size = 11;
            }
        }
    }
    return f;
}
}  // namespace teaching
