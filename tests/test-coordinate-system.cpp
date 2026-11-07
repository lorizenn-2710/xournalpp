#include <cmath>
#include <cstdio>

#include "control/tools/CoordinateSystemHandler.h"

int main() {
    int failures = 0;
    auto check = [&](bool ok, const char* message) {
        std::printf("%s %s\n", ok ? "PASS" : "FAIL", message);
        failures += !ok;
    };
    auto has = [](const std::vector<Point>& points, double x, double y) {
        for (const auto& p: points) {
            if (std::hypot(p.x - x, p.y - y) < 1e-8)
                return true;
        }
        return false;
    };
    for (const auto& end: {Point(300, 400), Point(100, 400), Point(300, 200), Point(100, 200)}) {
        const Point start(200, 300);
        for (bool quadrant: {false, true}) {
            const auto [p, range] = CoordinateSystemHandler::axes(start, end, false, quadrant);
            const double left = std::min(start.x, end.x), top = std::min(start.y, end.y);
            const double ox = quadrant ? left : left + 50, oy = quadrant ? top + 100 : top + 50;
            check(has(p, left, oy) && has(p, left + 100, oy) && has(p, ox, top) && has(p, ox, top + 100),
                  "Oxy: correct axis ends in every drag direction");
            check(p.size() == (quadrant ? 13 : 14), "Oxy: two retraced arrowheads, no disconnected diagonal");
            bool covered = true;
            for (const auto& v: p) covered &= range.contains(v.x, v.y);
            check(covered, "preview bounds include all axis and arrow points");
        }
        const auto [p, range] = CoordinateSystemHandler::axes(start, end, true, false);
        const double d = 60 / std::sqrt(2.0);
        check(has(p, 200, 200) && has(p, 300, 300) && has(p, 200 - d, 300 + d),
              "Oxyz: Oz up, Oy right, Ox 45 degrees down-left with depth 0.6");
        check(p.size() == 19, "Oxyz: all three axes have arrowheads");
        check(range.minX < start.x && range.maxY > start.y, "Oxyz bounds include depth outside drag box");
    }
    for (const auto& end: {Point(0, 0), Point(0, 100), Point(100, 0), Point(1e-12, 1e-12)}) {
        for (bool spatial: {false, true}) {
            const auto [p, range] = CoordinateSystemHandler::axes(Point(0, 0), end, spatial, false);
            bool finite = true;
            for (const auto& v: p) finite &= std::isfinite(v.x) && std::isfinite(v.y);
            check(finite, "degenerate/tiny drag never creates NaN");
        }
    }
    return failures ? 1 : 0;
}
