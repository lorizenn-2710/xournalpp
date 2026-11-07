// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

#include "CoordinateMath.h"
#include "FormulaRender.h"

namespace teaching::coordinate {
namespace {
using N = std::shared_ptr<const MathNotation>;
N text(std::string s) { return std::make_shared<MathNotation>(MathNotation{'n', std::move(s), {}, {}}); }
N node(char op, N a, N b = {}) {
    return std::make_shared<MathNotation>(MathNotation{op, "", std::move(a), std::move(b)});
}
N row(N a, N b) { return node('j', a, b); }
N vec(std::string s) { return node('v', text(std::move(s))); }
std::string n(double x) {
    auto s = tuple({x, 0, 0}, false);
    return s.substr(1, s.find(';') - 1);
}
N root(double x) { return std::make_shared<MathNotation>(MathNotation{'f', "sqrt", text(n(x)), {}}); }
N equation(const Line& l, bool spatial, bool canonical) {
    std::vector<N> rows;
    const char* names[] = {"x", "y", "z"};
    double p[] = {l.point.x, l.point.y, l.point.z}, d[] = {l.direction.x, l.direction.y, l.direction.z};
    N chain;
    for (int i = 0; i < (spatial ? 3 : 2); ++i) {
        if (canonical && d[i] != 0) {
            auto fraction = node('/', text(std::string(names[i]) + " − (" + n(p[i]) + ")"), text(n(d[i])));
            chain = chain ? row(row(chain, text("=")), fraction) : fraction;
        } else {
            std::string s = std::string(names[i]) + " = " + n(p[i]);
            if (d[i] != 0)
                s += (d[i] < 0 ? " − " : " + ") + (std::abs(d[i]) == 1 ? "" : n(std::abs(d[i]))) + "t";
            rows.push_back(text(s));
        }
    }
    if (chain)
        rows.insert(rows.begin(), row(chain, text(" = t")));
    N result = rows.front();
    for (size_t i = 1; i < rows.size(); ++i) result = node('s', result, rows[i]);
    return node('h', result);
}
struct Path {
    Vec a, b;
    uint32_t color;
    bool arrow = false, dash = false;
    std::string name;
};
constexpr uint32_t blue = 0x205bce, red = 0xc42b35, green = 0x008063, gray = 0x526174;
}  // namespace
Scene generate(const Request& r, double width, double height) {
    if (width < 340 || height < 290)
        throw std::runtime_error("VEC-007: Giảm zoom để vùng nhìn đủ 364 × 314 pt cho hình và công thức.");
    if (r.mode < 0 || r.mode > 2 || r.operation < 0 || r.operation > 3)
        throw std::runtime_error("VEC-008: Chế độ không hợp lệ.");
    for (auto s: {r.name, r.unit})
        if (s.size() > 12 ||
            !std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalnum(c) && c < 128; }))
            throw std::runtime_error("VEC-009: Tên/đơn vị dùng tối đa 12 chữ hoặc số ASCII.");
    checked(r.a);
    checked(r.b);
    checked(r.u);
    checked(r.v);
    checked(r.q);
    if (!r.spatial)
        for (auto v: {r.a, r.b, r.u, r.v, r.q})
            if (v.z != 0)
                throw std::runtime_error("VEC-003: Oxy yêu cầu z=0.");
    for (auto v: r.forces) {
        checked(v);
        if (!r.spatial && v.z != 0)
            throw std::runtime_error("VEC-003: Lực Oxy yêu cầu z=0.");
    }
    Scene scene;
    auto& f = scene.figure;
    f.width = width;
    f.height = height;
    double fy = 12, fx = width * .51;
    std::vector<Path> paths;
    auto label = [&](N formula, uint32_t color = 0x202020, double size = 11.5) {
        auto metrics = formulaMetrics(formula, size);
        double max = width - fx - 12;
        if (metrics.width > max) {
            size *= max / metrics.width;
            metrics = formulaMetrics(formula, size);
        }
        if (size < 6)
            throw std::runtime_error("VEC-010: Công thức quá dài; rút ngắn số/tên hoặc tăng vùng nhìn.");
        if (fy + metrics.height > height - 32)
            throw std::runtime_error("VEC-010: Công thức quá chật; tăng vùng nhìn.");
        Label l{{fx, fy}, "", size, color, max};
        l.notation = formula;
        f.labels.push_back(l);
        fy += metrics.height + 9;
    };
    auto note = [&](std::string s) { scene.report += s + "\n"; };
    auto resultLabel = [&](std::string name, Vec v, uint32_t c) {
        label(row(vec(name), text(" = " + tuple(v, r.spatial) + (r.unit.empty() ? "" : " " + r.unit))), c);
    };
    auto magnitude = [&](std::string name, Vec v) {
        label(row(row(text("|" + name + "| ="), root(dot(v, v))),
                  text(" ≈ " + n(length(v)) + (r.unit.empty() ? "" : " " + r.unit))));
    };
    std::vector<std::pair<std::string, Vec>> anchors = {{"O", {}}};
    if (r.mode == 1)
        anchors = {{"A", r.a}, {"B", r.b}, {"Q", r.q}, {"O", {}}};
    else if (r.mode == 0 && r.fromPoints)
        anchors = {{"A", r.a}, {"B", r.b}, {"O", {}}};
    if (r.mode == 0) {
        Vec u = r.fromPoints ? subtract(r.b, r.a) : r.u, v = r.v, w;
        std::string name = r.fromPoints ? "AB" : r.name.empty() ? "u" : r.name;
        w = r.operation == 0 ? add(u, v) :
            r.operation == 1 ? subtract(u, v) :
                               multiply(u, r.operation == 2 ? r.scalar : -1);
        label(row(row(vec("u"), name == "u" ? text("") : row(text(" = "), vec(name))),
                  text(" = " + tuple(u, r.spatial) + (r.unit.empty() ? "" : " " + r.unit))),
              blue);
        resultLabel("v", v, red);
        label(row(row(vec("w"), text(r.operation == 0 ? " = u + v = " :
                                     r.operation == 1 ? " = u − v = " :
                                     r.operation == 2 ? " = " + n(r.scalar) + "u = " :
                                                        " = −u = ")),
                  text(tuple(w, r.spatial))),
              green);
        magnitude(name, u);
        magnitude("v", v);
        magnitude("w", w);
        auto a = angle(u, v);
        label(text("u·v = " + n(dot(u, v)) + "; góc ≈ " + (a ? n(*a) + "°" : "không xác định")));
        auto comp = components(u, r.axisDegrees);
        label(text("u theo trục đã chọn: " + tuple(comp, r.spatial)));
        note("Trục phân tích quay " + n(r.axisDegrees) + "° quanh Oz; dung sai hướng 1e−10.");
        note("Cùng phương: " + std::string(parallel(u, v) ? "có" : "không") +
             "; vuông góc: " + (perpendicular(u, v) ? "có" : "không") + ". Vectơ 0 không có hướng.");
        label(text("Cùng phương: " + std::string(parallel(u, v) ? "có" : "không") +
                   "; vuông góc: " + (perpendicular(u, v) ? "có" : "không")),
              gray, 10);
        paths.push_back({{}, u, blue, true, false, "u"});
        paths.push_back({{}, v, red, true, false, "v"});
        if (r.operation == 1)
            paths.push_back({{}, multiply(v, -1), red, true, true, "−v"});
        paths.push_back({{}, w, green, true, false, "w"});
        if (r.operation <= 1) {
            auto vv = r.operation == 0 ? v : multiply(v, -1);
            paths.push_back({u, w, red, true, true, ""});
            if (!r.triangle)
                paths.push_back({vv, w, blue, true, true, ""});
        }
        if (r.fromPoints) {
            paths.push_back({r.a, r.b, blue, true, false, "AB"});
            note("AB = B − A; chọn A/B chỉ lấy dữ liệu tọa độ đã khai báo.");
        }
        if (r.showComponents) {
            double a = r.axisDegrees * 3.141592653589793 / 180;
            Vec x{comp.x * std::cos(a), comp.x * std::sin(a), 0}, y{-comp.y * std::sin(a), comp.y * std::cos(a), 0};
            if (comp.x != 0)
                paths.push_back({{}, x, gray, true, true, "ux"});
            if (comp.y != 0)
                paths.push_back({x, add(x, y), gray, true, true, "uy"});
            if (r.spatial && comp.z != 0)
                paths.push_back({add(x, y), u, gray, true, true, "uz"});
        }
    } else if (r.mode == 1) {
        auto l = r.fromPoints ? through(r.a, r.b) : line(r.a, r.u), k = line(r.b, r.v);
        auto pair = compare(l, k);
        auto foot = project(r.q, l);
        double distance = length(subtract(r.q, foot));
        label(text("d qua A" + tuple(l.point, r.spatial)));
        resultLabel("u", l.direction, blue);
        label(equation(l, r.spatial, r.spatial && r.canonical));
        if (!r.spatial) {
            auto u = multiply(l.direction, 1 / length(l.direction));
            label(text(n(-u.y) + "x + (" + n(u.x) + ")y + (" + n(u.y * l.point.x - u.x * l.point.y) + ") = 0"));
        }
        label(text("e qua B" + tuple(k.point, r.spatial)));
        resultLabel("v", k.direction, red);
        const char* relations[] = {"Cắt nhau", "Song song (dung sai)", "Trùng nhau (dung sai)", "Chéo nhau"};
        label(text(std::string(relations[int(pair.relation)]) + "; d(d,e) ≈ " + n(pair.distance)));
        label(text("H ≈ " + tuple(foot, r.spatial) + "; QH ≈ " + n(distance)));
        label(text("Q " + tuple(r.q, r.spatial) + (contains(l, r.q) ? " thuộc d" : " không thuộc d")), gray, 9);
        note("Q " + tuple(r.q, r.spatial) + (contains(l, r.q) ? " thuộc d" : " không thuộc d"));
        note("e: " + tuple(k.point, r.spatial) + " + s" + tuple(k.direction, r.spatial));
        label(text(std::string("e: ") + (r.spatial ? "(x;y;z)" : "(x;y)") + " = " + tuple(k.point, r.spatial) + " + s" +
                   tuple(k.direction, r.spatial)),
              red, 9);
        note("Chân trên d/e ≈ " + tuple(pair.first, r.spatial) + " / " + tuple(pair.second, r.spatial));
        double span = std::max(length(subtract(r.a, r.b)), length(subtract(r.a, r.q)));
        if (span == 0)
            span = 1;
        auto u = multiply(l.direction, 1 / length(l.direction)), v = multiply(k.direction, 1 / length(k.direction));
        paths.push_back(
                {subtract(l.point, multiply(u, span)), add(l.point, multiply(u, span)), blue, false, false, "d"});
        paths.push_back(
                {subtract(k.point, multiply(v, span)), add(k.point, multiply(v, span)), red, false, false, "e"});
        paths.push_back({r.q, foot, green, false, true, "QH"});
        paths.push_back({pair.first, pair.second, gray, false, true, ""});
        anchors.push_back({"H", foot});
    } else {
        auto forces = combine(r.forces);
        resultLabel("R", forces.resultant, green);
        magnitude("R", forces.resultant);
        resultLabel("E", forces.equilibrant, red);
        label(text("φ ≈ " + (forces.azimuth ? n(*forces.azimuth) + "°" : "không xác định") + "; θ ≈ " +
                   (forces.elevation ? n(*forces.elevation) + "°" : "không xác định")));
        auto comp = components(forces.resultant, r.axisDegrees);
        label(text("R theo trục đã chọn: " + tuple(comp, r.spatial) + " N"));
        for (int axis = 0; axis < (r.spatial ? 3 : 2); ++axis) {
            std::string sum = std::string("R") + "xyz"[axis] + " = ";
            for (size_t i = 0; i < r.forces.size(); ++i) {
                auto v = r.forces[i];
                if (i)
                    sum += " + ";
                sum += "(" + n(axis == 0 ? v.x : axis == 1 ? v.y : v.z) + ")";
            }
            sum += " = " +
                   n(axis == 0 ? forces.resultant.x :
                     axis == 1 ? forces.resultant.y :
                                 forces.resultant.z) +
                   " N";
            note(sum);
            // Wrap long sums in small groups instead of shrinking them into unreadable text.
            N sumRows;
            std::string segment = std::string("R") + "xyz"[axis] + " = ";
            for (size_t i = 0; i < r.forces.size(); ++i) {
                auto v = r.forces[i];
                segment += (i ? " + " : "") + std::string("(") + n(axis == 0 ? v.x : axis == 1 ? v.y : v.z) + ")";
                if ((i + 1) % 4 == 0 && i + 1 < r.forces.size()) {
                    sumRows = sumRows ? node('s', sumRows, text(segment)) : text(segment);
                    segment = "";
                }
            }
            segment += " = " +
                       n(axis == 0 ? forces.resultant.x :
                         axis == 1 ? forces.resultant.y :
                                     forces.resultant.z) +
                       " N";
            label(sumRows ? node('s', sumRows, text(segment)) : text(segment), 0x202020, 9);
        }
        note("φ: từ +Ox về +Oy; θ: góc nâng so với Oxy. Trục phân tích quay " + n(r.axisDegrees) + "°.");
        note("Chỉ xét lực đồng quy. Tổng lực 0 chưa đủ kết luận cân bằng vật rắn nếu chưa xét mômen.");
        label(text("Lực đồng quy · đơn vị N"));
        label(node('s', text("R = 0: cần xét thêm mômen"), text("trước khi kết luận cân bằng vật rắn")), gray, 9);
        size_t i = 0;
        for (auto v: r.forces) {
            ++i;
            paths.push_back({{}, v, blue, true, false, "F" + std::to_string(i)});
        }
        paths.push_back({{}, forces.resultant, green, true, false, "R"});
        paths.push_back({{}, forces.equilibrant, red, true, true, "E"});
        if (r.showComponents) {
            double a = r.axisDegrees * 3.141592653589793 / 180;
            Vec x{comp.x * std::cos(a), comp.x * std::sin(a), 0}, y{-comp.y * std::sin(a), comp.y * std::cos(a), 0};
            if (comp.x != 0)
                paths.push_back({{}, x, gray, true, true, "Rx"});
            if (comp.y != 0)
                paths.push_back({x, add(x, y), gray, true, true, "Ry"});
            if (r.spatial && comp.z != 0)
                paths.push_back({add(x, y), forces.resultant, gray, true, true, "Rz"});
        }
    }
    // Axes describe only the representation, never the source of any calculation.
    if (!std::isfinite(r.viewDegrees) || !std::isfinite(r.xScale) || !std::isfinite(r.yScale) || r.xScale < .1 ||
        r.xScale > 10 || r.yScale < .1 || r.yScale > 10)
        throw std::runtime_error("VEC-011: Tỉ lệ hiển thị trong [0.1,10].");
    auto raw = [&](Vec v) {
        double a = r.viewDegrees * 3.141592653589793 / 180;
        return r.spatial ?
                       Point{r.xScale * (-v.x * std::cos(a) * .65 + v.y), r.yScale * (v.x * std::sin(a) * .65 - v.z)} :
                       Point{r.xScale * v.x, -r.yScale * v.y};
    };
    double extent = 0;
    for (auto p: paths) extent = std::max({extent, length(p.a), length(p.b)});
    for (auto p: anchors) extent = std::max(extent, length(p.second));
    if (extent == 0)
        extent = 1;
    double axis = extent * .55;
    if (r.mode != 1 && r.showComponents && std::abs(std::remainder(r.axisDegrees, 360.)) > 1e-12) {
        paths.push_back({{}, polar(axis, r.axisDegrees), gray, true, true, "x′"});
        paths.push_back({{}, polar(axis, r.axisDegrees + 90), gray, true, true, "y′"});
    }
    for (auto p: {Path{{}, {axis, 0, 0}, gray, true, false, "x"}, Path{{}, {0, axis, 0}, gray, true, false, "y"}})
        paths.push_back(p);
    if (r.spatial)
        paths.push_back({{}, {0, 0, axis}, gray, true, false, "z"});
    double xmin = 0, xmax = 0, ymin = 0, ymax = 0;
    auto bounds = [&](Vec v) {
        auto p = raw(v);
        xmin = std::min(xmin, p.x);
        xmax = std::max(xmax, p.x);
        ymin = std::min(ymin, p.y);
        ymax = std::max(ymax, p.y);
    };
    for (auto p: paths) {
        bounds(p.a);
        bounds(p.b);
    }
    for (auto p: anchors) bounds(p.second);
    double factor = std::min((width * .48 - 48) / std::max(xmax - xmin, extent * .1),
                             (height - 94) / std::max(ymax - ymin, extent * .1));
    auto map = [&](Vec v) {
        auto p = raw(v);
        return Point{24 + (p.x - xmin) * factor, 34 + (p.y - ymin) * factor};
    };
    for (auto path: paths) {
        auto a = map(path.a), b = map(path.b);
        f.strokes.push_back({{a, b}, path.color, 1.25, path.dash});
        double len = std::hypot(b.x - a.x, b.y - a.y);
        if (path.arrow && len > 1e-8) {
            double dx = (b.x - a.x) / len, dy = (b.y - a.y) / len, h = std::min(7., len * .25);
            f.strokes.push_back({{{b.x - h * dx - h * .45 * dy, b.y - h * dy + h * .45 * dx},
                                  b,
                                  {b.x - h * dx + h * .45 * dy, b.y - h * dy - h * .45 * dx}},
                                 path.color,
                                 1.25,
                                 false});
        }
        if (!path.name.empty() && len > 1e-8)
            f.labels.push_back({{b.x + 3, b.y + 3}, path.name, 9, path.color, 60});
    }
    for (auto [name, v]: anchors) {
        auto p = map(v);
        scene.anchors.push_back({p, v});
        f.strokes.push_back({{{p.x - 2, p.y}, {p.x + 2, p.y}}, 0x202020, 2, false});
        f.labels.push_back({{p.x + 4, p.y - 12}, name, 9, 0x202020, 60});
    }
    f.labels.push_back({{12, height - 23},
                        r.spatial ? "Oxyz · hình chiếu, không đo góc trên hình" : "Oxy · tính theo tọa độ gốc",
                        8,
                        gray,
                        width * .48});
    f.labels.push_back({{fx, height - 23}, "≈ số làm tròn · hình tĩnh", 8, gray, width - fx - 12});
    return scene;
}
}  // namespace teaching::coordinate
