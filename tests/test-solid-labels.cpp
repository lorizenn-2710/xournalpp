/*
 * Test for naming the corners of the figures the shape tools draw (SolidGeometry::vertexLabels).
 *
 * Links the REAL implementation (src/core/model/SolidGeometry.cpp), so what is tested is the shipped code.
 * The figures are built by the same functions the tools use (boxWireframe/boxOutline, pyramidWireframe/
 * pyramidOutline), so the test cannot drift away from what actually gets drawn on the page.
 *
 * WHAT IS ASSERTED, AND WHY IT IS THE RIGHT THING
 *  - A box gets EIGHT labels: A, B, C, D on the front face and A', B', C', D' on the matching corner behind.
 *    Every label sits exactly on a corner of the wireframe, each name is used once, and each front corner's
 *    twin is the corner it is actually joined to (not merely the nearest one).
 *  - A pyramid gets FIVE labels: A, B, C, D on the base and S at the apex — the apex being the drawn point
 *    that is NOT on the base plane.
 *  - A triangle and a quadrilateral get A, B, C (, D).
 *  - Anything else (a circle, a random squiggle, two corners) yields NOTHING: better to label nothing than
 *    to write letters on the wrong places.
 *  - The result does not depend on the order of the strokes: the hidden-edge strokes of a box may come
 *    before or after the outline and the names stay the same.
 */

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "model/SolidGeometry.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::printf("  [SAI] %s\n", what.c_str());
    } else {
        std::printf("  [đạt] %s\n", what.c_str());
    }
}

std::string fmt(const Point& p) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "(%.2f, %.2f)", p.x, p.y);
    return buf;
}

/// Có nhãn nào tại đúng điểm này không, và tên là gì
std::string nameAt(const std::vector<SolidGeometry::VertexLabel>& labels, const Point& p) {
    for (const auto& l: labels) {
        if (std::abs(l.position.x - p.x) < 0.01 && std::abs(l.position.y - p.y) < 0.01) {
            return l.name;
        }
    }
    return "";
}

bool allDistinctNames(const std::vector<SolidGeometry::VertexLabel>& labels) {
    for (std::size_t i = 0; i < labels.size(); ++i) {
        for (std::size_t j = i + 1; j < labels.size(); ++j) {
            if (labels[i].name == labels[j].name) {
                return false;
            }
        }
    }
    return true;
}

/// Mọi nhãn phải nằm trên một đỉnh thật của khung dây
bool allOnVertices(const std::vector<SolidGeometry::VertexLabel>& labels, const std::vector<Point>& vertices) {
    for (const auto& l: labels) {
        bool found = false;
        for (const Point& v: vertices) {
            if (std::abs(l.position.x - v.x) < 0.01 && std::abs(l.position.y - v.y) < 0.01) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::printf("      nhãn %s tại %s KHÔNG nằm trên đỉnh nào\n", l.name.c_str(), fmt(l.position).c_str());
            return false;
        }
    }
    return true;
}

/// Chỉ số của một điểm trong danh sách đỉnh (dùng để lần theo các cạnh)
std::size_t indexOf(const std::vector<Point>& vertices, const Point& p) {
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        if (std::abs(vertices[i].x - p.x) < 0.01 && std::abs(vertices[i].y - p.y) < 0.01) {
            return i;
        }
    }
    return vertices.size();
}

}  // namespace

int main() {
    // ---------------------------------------------------------------- HÌNH HỘP
    std::printf("== HÌNH HỘP ==\n");
    {
        // Kéo theo 4 hướng khác nhau: kết quả không được phụ thuộc hướng kéo
        const double dirs[4][2] = {{0.70, -0.70}, {-0.70, -0.70}, {0.70, 0.70}, {-0.70, 0.70}};
        for (int d = 0; d < 4; ++d) {
            auto wire = SolidGeometry::boxWireframe({100.0, 300.0}, {400.0, 500.0}, 0.35, dirs[d][0], dirs[d][1]);
            std::vector<Point> outline = SolidGeometry::boxOutline(wire, true);
            std::vector<std::vector<Point>> hidden = SolidGeometry::hiddenEdgePolylines(wire);

            std::vector<std::vector<Point>> strokes;
            strokes.push_back(outline);
            for (const auto& h: hidden) {
                strokes.push_back(h);
            }

            auto labels = SolidGeometry::vertexLabels(strokes);
            char msg[220];
            std::snprintf(msg, sizeof(msg), "hướng %d: đủ 8 nhãn cho 8 đỉnh", d);
            check(labels.size() == 8, msg);
            std::snprintf(msg, sizeof(msg), "hướng %d: mỗi nhãn nằm ĐÚNG trên một đỉnh của khung dây", d);
            check(allOnVertices(labels, wire.vertices), msg);
            std::snprintf(msg, sizeof(msg), "hướng %d: 8 tên khác nhau (A..D và A'..D')", d);
            check(allDistinctNames(labels), msg);

            // Đúng 4 nhãn có phẩy (mặt sau) và 4 nhãn không
            int primed = 0;
            for (const auto& l: labels) {
                if (!l.name.empty() && l.name.back() == '\'') {
                    ++primed;
                }
            }
            std::snprintf(msg, sizeof(msg), "hướng %d: đúng 4 nhãn mặt sau (có dấu ')", d);
            check(primed == 4, msg);

            // Đỉnh "A" phải nối với đỉnh "A'" bằng MỘT CẠNH THẬT của hình
            bool pairOk = true;
            for (char c = 'A'; c <= 'D'; ++c) {
                const std::string front(1, c);
                const std::string back = front + "'";
                Point pf{}, pb{};
                bool haveF = false, haveB = false;
                for (const auto& l: labels) {
                    if (l.name == front) {
                        pf = l.position;
                        haveF = true;
                    }
                    if (l.name == back) {
                        pb = l.position;
                        haveB = true;
                    }
                }
                if (!haveF || !haveB) {
                    pairOk = false;
                    break;
                }
                const std::size_t i = indexOf(wire.vertices, pf);
                const std::size_t j = indexOf(wire.vertices, pb);
                bool edge = false;
                for (const auto& e: wire.edges) {
                    if ((e.from == i && e.to == j) || (e.from == j && e.to == i)) {
                        edge = true;
                        break;
                    }
                }
                if (!edge) {
                    std::printf("      %s=%s không nối với %s=%s bằng cạnh nào\n", front.c_str(), fmt(pf).c_str(),
                                back.c_str(), fmt(pb).c_str());
                    pairOk = false;
                    break;
                }
            }
            std::snprintf(msg, sizeof(msg), "hướng %d: mỗi cặp A-A', B-B', C-C', D-D' là MỘT CẠNH THẬT", d);
            check(pairOk, msg);

            // Mặt chứa A, B, C, D (không phẩy) phải là MẶT THẤY ĐƯỢC (mặt trước)
            auto frontFaceVisible = [&]() {
                std::vector<Point> face;
                for (char c = 'A'; c <= 'D'; ++c) {
                    for (const auto& l: labels) {
                        if (l.name == std::string(1, c)) {
                            face.push_back(l.position);
                        }
                    }
                }
                // dấu diện tích: dùng chính hàm của module qua một wireframe dựng lại thứ tự
                std::vector<std::size_t> idx;
                for (const Point& p: face) {
                    idx.push_back(indexOf(wire.vertices, p));
                }
                return SolidGeometry::signedArea(wire.vertices, idx);
            };
            // Mặt trước phải cùng dấu với mặt được khai là luôn thấy (visibleFace)
            const double reference = SolidGeometry::signedArea(wire.vertices, wire.faces[wire.visibleFace]);
            std::snprintf(msg, sizeof(msg), "hướng %d: A,B,C,D là mặt TRƯỚC (cùng chiều với mặt luôn thấy)", d);
            check((frontFaceVisible() > 0) == (reference > 0), msg);
        }
    }

    // Thứ tự stroke đảo lộn (nét đứt lên trước) vẫn phải cho cùng tên
    {
        auto wire = SolidGeometry::boxWireframe({100.0, 300.0}, {400.0, 500.0}, 0.35, 0.70, -0.70);
        std::vector<Point> outline = SolidGeometry::boxOutline(wire);
        std::vector<std::vector<Point>> hidden = SolidGeometry::hiddenEdgePolylines(wire);
        std::vector<std::vector<Point>> strokes;
        for (const auto& h: hidden) {
            strokes.push_back(h);
        }
        strokes.push_back(outline);  // đường bao đặt CUỐI
        auto labels = SolidGeometry::vertexLabels(strokes);
        check(labels.size() == 8, "đảo thứ tự stroke: vẫn đủ 8 nhãn");
        // "A" phải vẫn là đỉnh đầu tiên của ĐƯỜNG BAO
        const Point first = outline.front();
        check(!nameAt(labels, first).empty() && nameAt(labels, first).back() != '\'',
              "đảo thứ tự stroke: đỉnh đầu đường bao vẫn là A (không phẩy)");
    }

    // ---------------------------------------------------------------- HÌNH CHÓP
    std::printf("== HÌNH CHÓP ==\n");
    {
        auto wire = SolidGeometry::pyramidWireframe({300.0, 400.0}, 300.0, 300.0, 0.35, 0.70, -0.70);
        std::vector<Point> outline = SolidGeometry::pyramidOutline(wire, true);
        std::vector<std::vector<Point>> hidden = SolidGeometry::hiddenEdgePolylines(wire);
        std::vector<std::vector<Point>> strokes;
        strokes.push_back(outline);
        for (const auto& h: hidden) {
            strokes.push_back(h);
        }
        auto labels = SolidGeometry::vertexLabels(strokes);
        check(labels.size() == 5, "chóp: đủ 5 nhãn");
        check(allOnVertices(labels, wire.vertices), "chóp: mọi nhãn nằm ĐÚNG trên đỉnh");
        check(allDistinctNames(labels), "chóp: 5 tên khác nhau");

        const Point apex = outline.back();
        check(nameAt(labels, apex) == "S", "chóp: đỉnh chóp (điểm cuối nét vẽ) tên là S");
        int baseLetters = 0;
        for (char c = 'A'; c <= 'D'; ++c) {
            bool found = false;
            for (const auto& l: labels) {
                if (l.name == std::string(1, c)) {
                    found = true;
                }
            }
            if (found) {
                ++baseLetters;
            }
        }
        check(baseLetters == 4, "chóp: 4 đỉnh đáy là A, B, C, D");

        // Các đỉnh A..D phải nằm trên mặt đáy (4 đỉnh đáy của wireframe là 4 đỉnh KHÔNG phải đỉnh chóp)
        bool baseOnBase = true;
        for (const auto& l: labels) {
            if (l.name == "S") {
                continue;
            }
            bool isBase = false;
            for (const Point& v: wire.vertices) {
                if (std::abs(v.x - l.position.x) < 0.01 && std::abs(v.y - l.position.y) < 0.01 &&
                    std::abs(v.x - apex.x) > 0.01) {
                    isBase = true;
                }
            }
            if (!isBase) {
                baseOnBase = false;
            }
        }
        check(baseOnBase, "chóp: A..D nằm ở đáy, không lẫn đỉnh chóp");

        // Nét đứt đặt SAU đường bao và đầu mút của nó KHÔNG phải đỉnh chóp: đỉnh chóp vẫn phải là S.
        // (Đây là ca thật đã lộ lỗi trong app: lấy điểm cuối của TOÀN BỘ các nét thay vì của đường bao.)
        {
            std::vector<std::vector<Point>> withDashAfter;
            withDashAfter.push_back(outline);
            withDashAfter.push_back({wire.vertices[3], wire.vertices[2]});  // cạnh đáy xa, không dính đỉnh
            auto lb = SolidGeometry::vertexLabels(withDashAfter);
            check(lb.size() == 5, "chóp + nét đứt đặt sau: vẫn đủ 5 nhãn");
            check(nameAt(lb, apex) == "S", "chóp + nét đứt đặt sau: đỉnh chóp vẫn tên là S");
            int lettersOnBase = 0;
            for (char c = 'A'; c <= 'D'; ++c) {
                for (const auto& l: lb) {
                    if (l.name == std::string(1, c) && std::abs(l.position.x - apex.x) < 0.01 &&
                        std::abs(l.position.y - apex.y) < 0.01) {
                        lettersOnBase = -100;  // một chữ A..D lại nằm ở đỉnh chóp
                    }
                }
            }
            for (const auto& l: lb) {
                if (!l.name.empty() && l.name.size() == 1 && l.name[0] >= 'A' && l.name[0] <= 'D') {
                    ++lettersOnBase;
                }
            }
            check(lettersOnBase == 4, "chóp + nét đứt đặt sau: 4 chữ A..D đều ở đáy");
        }
    }

    // ---------------------------------------------------------------- TAM GIÁC / TỨ GIÁC
    std::printf("== TAM GIÁC / TỨ GIÁC ==\n");
    {
        std::vector<std::vector<Point>> tri = {{{100.0, 100.0}, {300.0, 100.0}, {200.0, 260.0}, {100.0, 100.0}}};
        auto labels = SolidGeometry::vertexLabels(tri);
        check(labels.size() == 3, "tam giác: 3 nhãn");
        check(nameAt(labels, {100.0, 100.0}) == "A" && nameAt(labels, {300.0, 100.0}) == "B" &&
                      nameAt(labels, {200.0, 260.0}) == "C",
              "tam giác: A, B, C theo thứ tự vẽ");

        std::vector<std::vector<Point>> quad = {
                {{100.0, 100.0}, {400.0, 100.0}, {400.0, 300.0}, {100.0, 300.0}, {100.0, 100.0}}};
        auto labelsQ = SolidGeometry::vertexLabels(quad);
        check(labelsQ.size() == 4, "tứ giác: 4 nhãn");
        check(nameAt(labelsQ, {400.0, 300.0}) == "C", "tứ giác: đỉnh thứ ba là C");
    }

    // ---------------------------------------------------------------- KHÔNG NHẬN RA THÌ IM LẶNG
    std::printf("== CA KHÔNG NHẬN RA (phải KHÔNG đặt tên gì) ==\n");
    {
        std::vector<std::vector<Point>> circle;
        for (int i = 0; i < 40; ++i) {
            const double a = i * 2 * 3.14159265358979 / 40.0;
            circle.push_back({{200.0 + 100.0 * std::cos(a), 200.0 + 100.0 * std::sin(a)}});
        }
        check(SolidGeometry::vertexLabels(circle).empty(), "đường tròn: không đặt tên");

        std::vector<std::vector<Point>> squiggle = {
                {{0.0, 0.0}, {10.0, 30.0}, {40.0, 12.0}, {70.0, 90.0}, {95.0, 25.0}, {120.0, 60.0}, {130.0, 5.0}}};
        check(SolidGeometry::vertexLabels(squiggle).empty(), "đường ngoằn ngoèo 7 đỉnh: không đặt tên");

        check(SolidGeometry::vertexLabels({}).empty(), "không có nét nào: không đặt tên");

        std::vector<std::vector<Point>> two = {{{0.0, 0.0}, {100.0, 0.0}}};
        check(SolidGeometry::vertexLabels(two).empty(), "một đoạn thẳng: không đặt tên");
    }

    for (double height: {300.0, -300.0}) {
        auto w = SolidGeometry::triangularPyramidWireframe({300, 400}, 240, height);
        for (bool exclude: {false, true}) {
            auto outline = SolidGeometry::triangularOutline(w, exclude);
            auto strokes = SolidGeometry::hiddenEdgePolylines(w);
            strokes.push_back(outline);  // Hidden edges first must not change S.
            auto labels = SolidGeometry::vertexLabels(strokes);
            check(labels.size() == 4 && allDistinctNames(labels) && allOnVertices(labels, w.vertices),
                  "S.ABC: exactly four distinct labels on real vertices");
            for (std::size_t j = 0; j < 4; ++j) {
                check(nameAt(labels, w.vertices[j]) == (j == 3 ? "S" : std::string(1, 'A' + j)),
                      "S.ABC: correct base/apex names");
            }
            check(SolidGeometry::vertexLabels({outline}).size() == 4,
                  "S.ABC: visible outline alone never reconstructs a fifth vertex");
        }
        auto right = SolidGeometry::rightPyramidWireframe({300, 400}, 240, height, .35, .7, -.7);
        auto strokes = SolidGeometry::hiddenEdgePolylines(right);
        strokes.push_back(SolidGeometry::pyramidOutline(right, true));
        auto labels = SolidGeometry::vertexLabels(strokes);
        check(labels.size() == 5 && nameAt(labels, right.vertices[0]) == "A" &&
                      nameAt(labels, right.vertices[4]) == "S",
              "SA perpendicular base: A is foot, S is apex");
    }
    check(SolidGeometry::vertexLabels(
                  {{{0, 100}, {50, 130}, {100, 100}, {0, 100}}, {{0, 0}, {50, 30}, {100, 0}, {0, 0}}})
                  .empty(),
          "two unconnected triangles: do not invent prism labels");

    for (const auto& end: {Point(400, 600), Point(0, 600), Point(400, 100), Point(0, 100)}) {
        auto w = SolidGeometry::triangularPrismWireframe({200, 300}, end);
        for (bool exclude: {false, true}) {
            auto outline = SolidGeometry::triangularOutline(w, exclude);
            auto strokes = SolidGeometry::hiddenEdgePolylines(w);
            strokes.push_back(outline);
            auto labels = SolidGeometry::vertexLabels(strokes);
            check(labels.size() == 6 && allDistinctNames(labels) && allOnVertices(labels, w.vertices),
                  "ABC.A'B'C': six distinct names on real vertices in all drag directions");
            for (std::size_t j = 0; j < 3; ++j) {
                const std::string name(1, 'A' + j);
                check(nameAt(labels, w.vertices[j]) == name && nameAt(labels, w.vertices[j + 3]) == name + "'",
                      "prism: bottom A,B,C paired vertically with top A',B',C'");
            }
            check(SolidGeometry::vertexLabels({outline}).size() == 6,
                  "prism: visible outline alone carries all six vertices");
        }
    }

    std::printf("\n%s (%d phép kiểm, %d lỗi)\n", failures == 0 ? "TẤT CẢ ĐỀU ĐẠT" : "CÓ LỖI", checks, failures);
    return failures == 0 ? 0 : 1;
}
