/*
 * Oracle test for the geometry of the three new shapes: triangle, box (hình hộp),
 * pyramid (hình chóp tứ giác).
 *
 * HONEST SCOPE: this file re-implements the vertex maths independently instead of calling the
 * real handlers, because the handlers need a live Control. It therefore pins the MATHS (a wrong
 * formula or a wrong offset shows up as a wrong vertex), and it catches the class of bug that
 * actually matters here. It does NOT prove that the real handlers compute the same thing.
 *
 * The real handlers are verified separately, and more strongly, by drawing each shape in the
 * running application with a virtual stylus and reading back the stroke coordinates the app
 * itself wrote to its autosave (see scratch/xpp-shapes-verify.py). Keep the two in sync: if a
 * formula changes in a handler, change it here too.
 */

#include <algorithm>  // for max
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

/// Local stand-in for Xournal++'s Point (the test does not link the app's headers)
struct Point {
    double x, y;
    Point() = default;
    Point(double x, double y): x(x), y(y) {}
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

namespace {
int failures = 0;

void checkClose(double a, double b, const char* what, double eps = 1e-6) {
    const bool ok = std::abs(a - b) <= eps;
    std::printf("%-72s %s  (%g vs %g)\n", what, ok ? "PASS" : "FAIL", a, b);
    if (!ok) {
        ++failures;
    }
}

void check(bool ok, const char* what) {
    std::printf("%-72s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) {
        ++failures;
    }
}
}  // namespace

// ---- replication of the maths of the three handlers -------------------------
// Kept in sync by hand; the real handlers are compiled by the normal build.

static std::vector<Point> triangleShape(double x0, double y0, double x1, double y1, bool shift, bool ctrl) {
    double width = x1 - x0;
    double height = y1 - y0;
    if (shift) {
        width = std::max(std::abs(width), std::abs(height)) * (width >= 0 ? 1 : -1);
    }
    const double base = std::abs(width);
    const double apexHeight = shift ? std::sqrt(3.0) / 2.0 * base : std::abs(height);
    const double signH = height >= 0 ? 1.0 : -1.0;
    const double signW = width >= 0 ? 1.0 : -1.0;
    const double apexY = (apexHeight > 0.0 ? apexHeight : base) * signH;

    Point p1, p2, apex;
    if (!ctrl) {
        p1 = Point(x0, y0);
        p2 = Point(x0 + signW * base, y0);
        apex = Point(x0 + signW * base / 2.0, y0 - apexY);
    } else {
        p1 = Point(x0 - base / 2.0, y0);
        p2 = Point(x0 + base / 2.0, y0);
        apex = Point(x0, y0 - apexY);
    }
    return {p1, p2, apex, p1};
}

static std::vector<Point> boxShape(double x0, double y0, double x1, double y1, bool shift, bool ctrl,
                                   double depthFrac = 0.45, double dx = 0.75, double dy = -0.75) {
    double width = x1 - x0;
    double height = y1 - y0;
    if (shift) {
        const int signW = width > 0 ? 1 : -1;
        const int signH = height > 0 ? 1 : -1;
        width = std::max(width * signW, height * signH) * signW;
        height = (width * signW) * signH;
    }
    Point f1, f2;
    if (!ctrl) {
        f1 = Point(x0, y0);
        f2 = Point(x0 + width, y0 + height);
    } else {
        f1 = Point(x0 - width, y0 - height);
        f2 = Point(x0 + width, y0 + height);
    }
    const double size = std::max(std::abs(f2.x - f1.x), std::abs(f2.y - f1.y));
    const double depth = depthFrac * size;
    const Point d{dx * depth, dy * depth};
    const Point a{f1.x, f1.y}, b{f2.x, f1.y}, cc{f2.x, f2.y}, dd{f1.x, f2.y};
    const Point a2{a.x + d.x, a.y + d.y}, b2{b.x + d.x, b.y + d.y}, c2{cc.x + d.x, cc.y + d.y},
            d2{dd.x + d.x, dd.y + d.y};
    return {a2, b2, c2, d2, a2, a, a2, b, b2, cc, c2, dd, d2, a, b, cc, dd, a};
}

static std::vector<Point> pyramidShape(double x0, double y0, double x1, double y1, bool ctrl,
                                       double baseDepthFrac = 0.45, double dx = 0.75, double dy = -0.75,
                                       double baseHeightFrac = 0.35) {
    const double width = x1 - x0;
    const double height = y1 - y0;
    const double base = std::abs(width);
    const double apexHeight = std::abs(height) > 0.0 ? std::abs(height) : base;
    const double signH = height >= 0 ? 1.0 : -1.0;
    const double signW = width >= 0 ? 1.0 : -1.0;
    Point centre = ctrl ? Point(x0, y0) : Point(x0 + signW * base / 2.0, y0);

    const double halfW = base / 2.0;
    const double baseHeight = baseHeightFrac * base;
    const Point d{dx * baseDepthFrac * base, dy * baseDepthFrac * base};
    const Point b1{centre.x - halfW, centre.y};
    const Point b2{centre.x + halfW, centre.y};
    const Point b3{centre.x + halfW + d.x, centre.y + d.y - baseHeight};
    const Point b4{centre.x - halfW + d.x, centre.y + d.y - baseHeight};
    const Point apex{centre.x + d.x / 2.0, centre.y - apexHeight * signH};
    return {b1, b2, b3, b4, b1, b4, apex, b1, apex, b2, apex, b3, apex};
}

int main() {
    std::printf("=== TAM GIAC ===\n");
    {
        auto s = triangleShape(100, 200, 300, 250, false, false);
        checkClose(s[0].x, 100, "đáy: góc trái tại điểm bắt đầu (x)");
        checkClose(s[0].y, 200, "đáy: góc trái tại điểm bắt đầu (y)");
        checkClose(s[1].x, 300, "đáy: góc phải cách 200 (x)");
        checkClose(s[1].y, 200, "đáy: góc phải cùng hàng với góc trái (y)");
        checkClose(s[2].x, 200, "đỉnh nằm giữa đáy theo x");
        checkClose(s[2].y, 150, "kéo XUỐNG 50 -> đỉnh nằm TRÊN đáy 50 (y)");
        check(s[0] == s[3], "hình khép kín (điểm cuối trùng điểm đầu)");
        check(s.size() == 4, "4 điểm: tam giác khép kín");
    }
    {
        // Shift: equilateral. height must be sqrt(3)/2 * base
        auto s = triangleShape(0, 0, 200, 30, true, false);
        checkClose(std::abs(s[2].y - 0), std::sqrt(3.0) / 2.0 * 200, "Shift: tam giác ĐỀU, cao = √3/2 × đáy");
    }
    {
        // Drag downwards -> apex above the base (negative y direction is "up" on the page)
        auto s = triangleShape(0, 0, 200, -150, false, false);
        checkClose(s[2].y, 150, "kéo LÊN 150 -> đỉnh lật xuống dưới đáy 150 (gương lại)");
    }

    std::printf("\n=== HINH HOP ===\n");
    {
        auto s = boxShape(100, 100, 300, 250, false, false);
        // front face corners
        checkClose(s[13].x, 100, "mặt trước: góc dưới-trái (x)");
        checkClose(s[13].y, 100, "mặt trước: góc dưới-trái (y)");
        checkClose(s[15].x, 300, "mặt trước: góc trên-phải (x)");
        checkClose(s[15].y, 250, "mặt trước: góc trên-phải (y)");
        // back face is offset up-right by depth
        const double depth = 0.45 * 200;  // size = max(200,150)
        checkClose(s[0].x, 100 + 0.75 * depth, "mặt sau lệch sang PHẢI đúng độ sâu (x)");
        checkClose(s[0].y, 100 - 0.75 * depth, "mặt sau lệch LÊN đúng độ sâu (y)");
        check(s.size() == 18, "18 điểm: 2 mặt + 4 cạnh nối");
        checkClose(s[5].x, s[13].x, "cạnh nối bắt đầu đúng tại góc mặt trước (x)");
        checkClose(s[6].x, s[0].x, "cạnh nối kết thúc đúng tại góc mặt sau (x)");
    }
    {
        auto s = boxShape(0, 0, 200, 100, true, false);
        // Shift -> front face must be a square: |dx| == |dy| and both 200
        checkClose(std::abs(s[15].x - s[13].x), 200, "Shift: mặt trước thành HÌNH VUÔNG (rộng)");
        checkClose(std::abs(s[15].y - s[13].y), 200, "Shift: mặt trước thành HÌNH VUÔNG (cao)");
    }

    std::printf("\n=== HINH CHOP TU GIAC ===\n");
    {
        auto s = pyramidShape(100, 300, 300, 500, false);
        // base: b1 front-left, b2 front-right, b3 back-right, b4 back-left
        checkClose(s[0].x, 100, "đáy: góc trước-trái (x)");
        checkClose(s[0].y, 300, "đáy: góc trước-trái (y)");
        checkClose(s[1].x, 300, "đáy: góc trước-phải (x)");
        checkClose(s[1].y, 300, "đáy: góc trước-phải cùng hàng");
        const double depth = 0.45 * 200;
        const double baseH = 0.35 * 200;
        checkClose(s[2].x, 300 + 0.75 * depth, "đáy: góc sau-phải lệch sang phải (x)");
        checkClose(s[2].y, 300 - 0.75 * depth - baseH, "đáy: góc sau-phải lệch lên (y)");
        checkClose(s[3].x, 100 + 0.75 * depth, "đáy: góc sau-trái lệch sang phải (x)");
        // apex
        checkClose(s[6].x, 200 + 0.75 * depth / 2.0, "đỉnh nằm trên tâm đáy (x)");
        checkClose(s[6].y, 100, "kéo xuống 200 -> đỉnh nằm trên đáy 200 (y)");
        check(s.size() == 13, "13 điểm: đáy khép kín + 4 cạnh bên");
        check(s[0] == s[4], "đáy khép kín");
    }

    std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures,
                failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
