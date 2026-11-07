/*
 * Test for the solid-figure geometry: which edges of a box and of a pyramid the viewer cannot see.
 *
 * Links the REAL implementation (src/core/model/SolidGeometry.cpp), so it exercises the shipped
 * code rather than a copy of its formulas. Run it after any change to that file.
 *
 * WHAT IS ASSERTED, AND WHY IT IS THE RIGHT THING
 *
 * Box: whatever direction the gesture is dragged in, exactly THREE edges are hidden and they all
 * meet at ONE vertex, which lies strictly inside the front face — the corner the viewer cannot see.
 * No edge of the front face is ever dashed.
 *
 * Pyramid: the base is a parallelogram, and an edge is dashed only when BOTH faces meeting at it
 * are hidden. For an upright pyramid the far base edge is therefore dashed when the lateral face
 * standing on it is itself hidden (a normal, reasonably tall pyramid), while a squat pyramid — one
 * whose apex projects below the far base edge, so that the viewer sees over the top of every
 * lateral face — has no dashed edge at all. The near base edge is never dashed.
 *
 * The rule is symmetric, so it also covers an UPSIDE-DOWN pyramid (apex below the base, which is
 * what dragging upwards gives): the depth then points toward the viewer and the hidden lateral
 * faces are the near ones. Picking the reference face by the sign of the apex height is what makes
 * that case come out right; before, every dashed edge of such a figure landed on the wrong side.
 *
 * The alternative rule "an edge is hidden when it runs inside the outline of the figure" is WRONG
 * and was abandoned: in oblique projection a visible vertex (the near-bottom corner of a box) can
 * project inside the outline, so that rule invents hidden edges that do not exist.
 */

#include <cmath>
#include <cstdio>
#include <set>
#include <tuple>
#include <vector>

#include "model/Point.h"
#include "model/SolidGeometry.h"

namespace {
int failures = 0;

void check(bool ok, const char* what) {
    std::printf("%-76s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) {
        ++failures;
    }
}

/// The hidden edges, as sorted pairs of vertex indices
auto hiddenPairs(const SolidGeometry::Wireframe& w) -> std::set<std::pair<std::size_t, std::size_t>> {
    std::set<std::pair<std::size_t, std::size_t>> pairs;
    for (std::size_t i: SolidGeometry::hiddenEdges(w)) {
        const auto& e = w.edges[i];
        pairs.emplace(std::min(e.from, e.to), std::max(e.from, e.to));
    }
    return pairs;
}

/// The single vertex all the hidden edges meet at, or npos when they do not all meet at one
auto commonVertex(const SolidGeometry::Wireframe& w) -> std::size_t {
    auto pairs = hiddenPairs(w);
    if (pairs.empty()) {
        return static_cast<std::size_t>(-1);
    }
    auto first = *pairs.begin();
    for (std::size_t candidate: {first.first, first.second}) {
        bool all = true;
        for (const auto& [a, b]: pairs) {
            if (a != candidate && b != candidate) {
                all = false;
                break;
            }
        }
        if (all) {
            return candidate;
        }
    }
    return static_cast<std::size_t>(-1);
}

bool strictlyInside(double x, double y, const Point& f1, const Point& f2) {
    const double xLo = std::min(f1.x, f2.x), xHi = std::max(f1.x, f2.x);
    const double yLo = std::min(f1.y, f2.y), yHi = std::max(f1.y, f2.y);
    return x > xLo && x < xHi && y > yLo && y < yHi;
}

/// The box corner that lands inside the front face: the one the viewer cannot see
std::size_t hiddenCornerOf(const SolidGeometry::Wireframe& w, const Point& f1, const Point& f2) {
    for (std::size_t i = 0; i < w.vertices.size(); ++i) {
        if (strictlyInside(w.vertices[i].x, w.vertices[i].y, f1, f2)) {
            return i;
        }
    }
    return static_cast<std::size_t>(-1);
}

bool anyFrontFaceEdgeHidden(const SolidGeometry::Wireframe& w, std::size_t frontFace) {
    for (std::size_t i: SolidGeometry::hiddenEdges(w)) {
        if (w.edges[i].faceA == frontFace || w.edges[i].faceB == frontFace) {
            return true;
        }
    }
    return false;
}
}  // namespace

int main() {
    // ---------------------------------------------------------------- hình hộp
    std::printf("=== HÌNH HỘP: mọi hướng kéo -> 3 cạnh khuất tại MỘT đỉnh nằm trong mặt trước ===\n");

    struct Case {
        const char* label;
        Point f1, f2;
    };
    const std::vector<Case> cases = {
            {"kéo xuống-phải (thường dùng)", {100, 100}, {300, 250}},
            {"kéo xuống-trái", {300, 100}, {100, 250}},
            {"kéo lên-phải", {100, 250}, {300, 100}},
            {"kéo lên-trái", {300, 250}, {100, 100}},
            {"mặt trước vuông", {100, 100}, {300, 300}},
    };

    for (const Case& c: cases) {
        auto w = SolidGeometry::boxWireframe(c.f1, c.f2, 0.45, 0.75, -0.75);
        char msg[160];

        std::snprintf(msg, sizeof(msg), "%-30s -> đúng 3 cạnh khuất", c.label);
        check(hiddenPairs(w).size() == 3, msg);

        const std::size_t corner = commonVertex(w);
        std::snprintf(msg, sizeof(msg), "%-30s -> cả 3 cạnh gặp nhau tại 1 đỉnh", c.label);
        check(corner != static_cast<std::size_t>(-1), msg);

        std::snprintf(msg, sizeof(msg), "%-30s -> đỉnh đó nằm TRONG mặt trước", c.label);
        check(corner != static_cast<std::size_t>(-1) && corner == hiddenCornerOf(w, c.f1, c.f2), msg);

        std::snprintf(msg, sizeof(msg), "%-30s -> không cạnh nào của mặt TRƯỚC bị khuất", c.label);
        check(!anyFrontFaceEdgeHidden(w, 0), msg);
    }

    // The default depth direction (up and to the right) hides the BACK-BOTTOM-LEFT corner, which
    // lands at a known place: 100+0.75*90, 250-0.75*90.
    {
        auto w = SolidGeometry::boxWireframe({100, 100}, {300, 250}, 0.45, 0.75, -0.75);
        const std::size_t corner = commonVertex(w);
        check(corner == 7, "hướng mặc định (lên-phải): góc khuất là góc sau-dưới-trái (chỉ số 7)");
        const Point& p = w.vertices[corner];
        check(std::abs(p.x - 167.5) < 1e-6 && std::abs(p.y - 182.5) < 1e-6, "toạ độ góc khuất đúng (167.5, 182.5)");
    }

    // Degenerate front face (no height): the figure is a line, no face has an area to judge, and
    // the safe answer is "nothing is hidden" rather than a crash or a wrong guess.
    {
        auto w = SolidGeometry::boxWireframe({100, 100}, {300, 100}, 0.45, 0.75, -0.75);
        check(hiddenPairs(w).empty(), "mặt trước dẹt (cao 0): không cạnh nào bị coi là khuất");
    }

    // ---------------------------------------------------------------- hình chóp
    std::printf("\n=== HÌNH CHÓP: đáy PHẲNG (hình bình hành); cạnh khuất ⟺ CẢ HAI mặt kề nó khuất ===\n");

    {
        // The base is a PARALLELOGRAM: the far edge is the front edge translated by the depth
        // vector, so the two side edges are parallel and equal. An earlier version lifted only the
        // two far corners, which made the base a trapezium — a visible warp on a figure that is
        // supposed to look like a flat quadrilateral seen in perspective.
        auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, 200.0, 0.45, 0.75, -0.75);
        const Point& b1 = w.vertices[0];
        const Point& b2 = w.vertices[1];
        const Point& b3 = w.vertices[2];
        const Point& b4 = w.vertices[3];
        const double side1x = b4.x - b1.x, side1y = b4.y - b1.y;
        const double side2x = b3.x - b2.x, side2y = b3.y - b2.y;
        char msg[200];
        std::snprintf(msg, sizeof(msg), "đáy là HÌNH BÌNH HÀNH: hai cạnh bên song song (%.4f,%.4f) vs (%.4f,%.4f)",
                      side1x, side1y, side2x, side2y);
        check(std::abs(side1x - side2x) < 1e-9 && std::abs(side1y - side2y) < 1e-9, msg);
        check(std::abs(b1.x + b3.x - (b2.x + b4.x)) < 1e-9 && std::abs(b1.y + b3.y - (b2.y + b4.y)) < 1e-9,
              "đáy PHẲNG: hai đường chéo cắt nhau tại trung điểm (đúng là hình bình hành, hết vênh)");
        check(b3.y < b2.y, "cạnh đáy sau nằm CAO HƠN cạnh trước (đúng phép chiếu xiên, không bị nâng lệch)");
    }

    {
        // Normal pyramid: apex as high as the base is wide. The lateral face on the far base edge
        // turns away, so that edge is the dashed one; the near and side base edges are not.
        auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, 200.0, 0.45, 0.75, -0.75);
        auto pairs = hiddenPairs(w);
        check(pairs == std::set<std::pair<std::size_t, std::size_t>>{{0, 3}, {2, 3}, {3, 4}},
              "đỉnh cao bằng đáy: 3 cạnh khuất tại góc đáy XA (b3-b4, b4-b1, b4-apex)");
    }

    {
        // A squat pyramid: the apex projects below the far base edge, the viewer sees over the top
        // of every lateral face, so no edge separates two hidden faces.
        auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, 60.0, 0.45, 0.75, -0.75);
        check(hiddenPairs(w).empty(), "đỉnh thấp (60): không cạnh nào khuất (nhìn được qua mọi mặt bên)");
    }

    {
        // A very tall pyramid: the two lateral faces on the far side turn away as well, so the
        // three edges meeting at the far base corner are hidden.
        auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, 400.0, 0.45, 0.75, -0.75);
        auto pairs = hiddenPairs(w);
        std::set<std::pair<std::size_t, std::size_t>> expected{{0, 3}, {2, 3}, {3, 4}};
        check(pairs == expected, "đỉnh rất cao (400): 3 cạnh khuất tại góc đáy xa (b3-b4, b4-b1, b4-apex)");
    }

    {
        // An UPSIDE-DOWN pyramid (apex below the base, what dragging upwards gives). The depth
        // vector now points toward the viewer, so the hidden lateral faces are the NEAR ones. Using
        // the front lateral face as the "known visible" reference here reported the far faces
        // instead — every dashed edge on the wrong side of the figure.
        auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, -200.0, 0.45, 0.75, -0.75);
        auto pairs = hiddenPairs(w);
        check(pairs == std::set<std::pair<std::size_t, std::size_t>>{{3, 4}},
              "chóp chĩa XUỐNG (đỉnh dưới đáy): cạnh khuất là cạnh bên tới góc đáy XA (b4-apex)");
    }

    {
        // The base must be wound the same way round as the lateral faces. With the opposite winding
        // the sign of its projected area points the other way and every figure comes out with the
        // wrong edges dashed — measured: a 200-wide pyramid came out with NO dashed edge at all
        // instead of the far base edge. This test pins the far base edge down.
        for (double apex: {100.0, 200.0, 300.0, 400.0}) {
            auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, apex, 0.45, 0.75, -0.75);
            auto pairs = hiddenPairs(w);
            char msg[160];
            std::snprintf(msg, sizeof(msg), "đỉnh cao %.0f: cạnh đáy XA (b3-b4) nằm trong tập khuất", apex);
            check(pairs.count({2, 3}) != 0, msg);
        }
    }

    {
        // The near base edge is never dashed for an upright pyramid, whatever the apex height.
        bool everHidden = false;
        for (double apex: {60.0, 100.0, 200.0, 400.0}) {
            auto w = SolidGeometry::pyramidWireframe({200, 300}, 200.0, apex, 0.45, 0.75, -0.75);
            if (hiddenPairs(w).count({0, 1}) != 0) {
                everHidden = true;
            }
        }
        check(!everHidden, "chóp dựng đứng: cạnh đáy TRƯỚC (b1-b2) không bao giờ bị khuất, ở mọi độ cao đỉnh");
    }

    // ---------------------------------------------------------------- đường viền
    // A regression guard: the outline used to be written as one concatenated point list, which
    // silently drew a diagonal across a face at every junction where one polyline ended somewhere
    // other than where the next one began. Every drawn segment must be a real edge of the figure.
    std::printf("\n=== ĐƯỜNG VIỀN: không đoạn nào được là đường chéo xuyên mặt ===\n");
    {
        auto edgeKey = [](const Point& p, const Point& q) {
            Point a = p, b = q;
            if (b.x < a.x || (b.x == a.x && b.y < a.y)) {
                std::swap(a, b);
            }
            return std::make_tuple(a.x, a.y, b.x, b.y);
        };

        auto w = SolidGeometry::boxWireframe({100, 100}, {300, 250}, 0.45, 0.75, -0.75);
        std::set<std::tuple<double, double, double, double>> realEdges;
        for (const auto& e: w.edges) {
            realEdges.insert(edgeKey(w.vertices[e.from], w.vertices[e.to]));
        }
        const std::vector<Point> outline = SolidGeometry::boxOutline(w);
        std::size_t drawn = 0, spurious = 0;
        for (std::size_t i = 1; i < outline.size(); ++i) {
            ++drawn;
            if (realEdges.count(edgeKey(outline[i - 1], outline[i])) == 0) {
                ++spurious;
            }
        }
        char msg[160];
        std::snprintf(msg, sizeof(msg), "hình hộp: %zu đoạn vẽ, %zu là cạnh thật, %zu là đường chéo thừa", drawn,
                      drawn - spurious, spurious);
        check(spurious == 0, msg);
        std::snprintf(msg, sizeof(msg), "hình hộp: vẽ đủ cả 12 cạnh");
        std::set<std::tuple<double, double, double, double>> covered;
        for (std::size_t i = 1; i < outline.size(); ++i) {
            covered.insert(edgeKey(outline[i - 1], outline[i]));
        }
        check(covered.size() == 12, msg);
        check(outline.front() == outline.back(), "hình hộp: đường vẽ KHÉP KÍN (để tô màu vẫn đúng)");

        auto wp = SolidGeometry::pyramidWireframe({200, 300}, 200.0, 200.0, 0.45, 0.75, -0.75);
        std::set<std::tuple<double, double, double, double>> realP;
        for (const auto& e: wp.edges) {
            realP.insert(edgeKey(wp.vertices[e.from], wp.vertices[e.to]));
        }
        const std::vector<Point> outlineP = SolidGeometry::pyramidOutline(wp);
        std::size_t drawnP = 0, spuriousP = 0;
        for (std::size_t i = 1; i < outlineP.size(); ++i) {
            ++drawnP;
            if (realP.count(edgeKey(outlineP[i - 1], outlineP[i])) == 0) {
                ++spuriousP;
            }
        }
        std::snprintf(msg, sizeof(msg), "hình chóp: %zu đoạn vẽ, %zu là cạnh thật, %zu là đường chéo thừa", drawnP,
                      drawnP - spuriousP, spuriousP);
        check(spuriousP == 0, msg);
        std::set<std::tuple<double, double, double, double>> coveredP;
        for (std::size_t i = 1; i < outlineP.size(); ++i) {
            coveredP.insert(edgeKey(outlineP[i - 1], outlineP[i]));
        }
        std::snprintf(msg, sizeof(msg), "hình chóp: vẽ đủ cả 8 cạnh");
        check(coveredP.size() == 8, msg);

        // Khi excludeHidden = true (chế độ vẽ thực tế: nét liền chỉ đi qua cạnh THẤY, cạnh khuất vẽ nét đứt riêng):
        // Tập cạnh liền và tập cạnh đứt phải RỜI NHAU hoàn toàn (giao = rỗng) và hợp lại đủ 12 cạnh (hộp) / 8 cạnh
        // (chóp).
        for (const Case& c: cases) {
            auto wb = SolidGeometry::boxWireframe(c.f1, c.f2, 0.45, 0.75, -0.75);
            const auto visB = SolidGeometry::boxOutline(wb, true);
            const auto hidB = SolidGeometry::hiddenEdgePolylines(wb);
            std::set<std::tuple<double, double, double, double>> visSet, hidSet;
            for (std::size_t i = 1; i < visB.size(); ++i) {
                visSet.insert(edgeKey(visB[i - 1], visB[i]));
            }
            for (const auto& h: hidB) {
                if (h.size() >= 2) {
                    hidSet.insert(edgeKey(h[0], h[1]));
                }
            }
            bool disjoint = true;
            for (const auto& e: hidSet) {
                if (visSet.count(e) != 0) {
                    disjoint = false;
                }
            }
            std::snprintf(msg, sizeof(msg), "%-30s -> nét liền (9 cạnh thấy) KHÔNG đè lên 3 cạnh nét đứt", c.label);
            check(disjoint && visSet.size() == 9 && hidSet.size() == 3, msg);
        }

        {
            const auto visP = SolidGeometry::pyramidOutline(wp, true);
            const auto hidP = SolidGeometry::hiddenEdgePolylines(wp);
            std::set<std::tuple<double, double, double, double>> visSet, hidSet;
            for (std::size_t i = 1; i < visP.size(); ++i) {
                visSet.insert(edgeKey(visP[i - 1], visP[i]));
            }
            for (const auto& h: hidP) {
                if (h.size() >= 2) {
                    hidSet.insert(edgeKey(h[0], h[1]));
                }
            }
            bool disjoint = true;
            for (const auto& e: hidSet) {
                if (visSet.count(e) != 0) {
                    disjoint = false;
                }
            }
            check(disjoint && visSet.size() + hidSet.size() == 8,
                  "hình chóp (excludeHidden=true): nét liền (5 cạnh thấy) KHÔNG đè lên 3 cạnh nét đứt");
        }
    }

    // New textbook solids: topology, hidden edges and edge-only continuous walks.
    for (double height: {60.0, 300.0, -300.0}) {
        auto w = SolidGeometry::triangularPyramidWireframe({300, 400}, 240, height);
        check(w.vertices.size() == 4 && w.edges.size() == 6 && w.faces.size() == 4,
              "triangular pyramid: 4 vertices, 6 edges, 4 faces");
        check(hiddenPairs(w) == std::set<std::pair<std::size_t, std::size_t>>{{0, 2}},
              "triangular pyramid: only rear base edge AC is hidden");
        check(w.vertices[0].y == w.vertices[2].y && (w.vertices[1].y - w.vertices[0].y) * height > 0,
              "triangular pyramid: horizontal AC, front B follows orientation");
    }
    for (const auto& second: {Point(400, 600), Point(0, 600), Point(400, 100), Point(0, 100)}) {
        auto w = SolidGeometry::triangularPrismWireframe({200, 300}, second);
        check(w.vertices.size() == 6 && w.edges.size() == 9 && w.faces.size() == 5,
              "triangular prism: 6 vertices, 9 edges, 5 faces in every drag direction");
        check(hiddenPairs(w) == std::set<std::pair<std::size_t, std::size_t>>{{0, 2}},
              "triangular prism: only lower rear AC is hidden; upper triangle solid");
        bool vertical = true;
        const double h = w.vertices[0].y - w.vertices[3].y;
        for (std::size_t j = 0; j < 3; ++j) {
            vertical &= w.vertices[j].x == w.vertices[j + 3].x &&
                        std::abs(w.vertices[j].y - w.vertices[j + 3].y - h) < 1e-9;
        }
        check(vertical && h > 0, "prism: AA', BB', CC' vertical, equal and parallel");
    }
    for (double height: {300.0, -300.0}) {
        const auto w = SolidGeometry::rightPyramidWireframe({300, 400}, 240, height, .35, .7, -.7);
        check(w.vertices[4].x == w.vertices[0].x && std::abs(w.vertices[0].y - w.vertices[4].y - height) < 1e-9,
              "right pyramid: S directly above/below A, SA retains requested height");
        check(!hiddenPairs(w).empty(), "right pyramid: occluded edges classified automatically");
    }
    for (const auto& w: {SolidGeometry::triangularPyramidWireframe({300, 400}, 240, 300),
                         SolidGeometry::triangularPrismWireframe({100, 100}, {400, 500})}) {
        for (bool exclude: {false, true}) {
            auto p = SolidGeometry::triangularOutline(w, exclude);
            const auto hidden = hiddenPairs(w);
            std::set<std::pair<std::size_t, std::size_t>> drawn;
            bool legal = true;
            for (std::size_t j = 1; j < p.size(); ++j) {
                std::size_t a = w.vertices.size(), b = w.vertices.size();
                for (std::size_t k = 0; k < w.vertices.size(); ++k) {
                    if (p[j - 1].lineLengthTo(w.vertices[k]) < 1e-9)
                        a = k;
                    if (p[j].lineLengthTo(w.vertices[k]) < 1e-9)
                        b = k;
                }
                if (a > b)
                    std::swap(a, b);
                bool edge = false;
                for (const auto& e: w.edges) edge |= std::min(e.from, e.to) == a && std::max(e.from, e.to) == b;
                legal &= edge && (!exclude || hidden.count({a, b}) == 0);
                drawn.emplace(a, b);
            }
            check(legal, "triangular outline never crosses a face or overwrites a hidden edge");
            check(drawn.size() + (exclude ? hidden.size() : 0) == w.edges.size(),
                  "triangular outline covers every required edge");
        }
    }

    std::printf("\n%s (%d lỗi)\n", failures == 0 ? "TẤT CẢ ĐỀU ĐẠT" : "CÓ LỖI", failures);
    return failures == 0 ? 0 : 1;
}
