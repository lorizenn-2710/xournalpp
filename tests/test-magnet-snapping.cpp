/*
 * Standalone unit test for the magnet snapping (ElementSnapper + SnapToGridInputHandler).
 *
 * Not part of the Xournal++ build: it is compiled by hand against the same sources, so that the
 * snapping logic can be exercised without launching the GUI. Build and run with:
 *
 *   g++ -std=c++20 -O1 -Isrc/core -Isrc/util/include -Ibuild/src \
 *       $(pkg-config --cflags gtk+-3.0) \
 *       tests/test-magnet-snapping.cpp \
 *       src/core/control/tools/ElementSnapper.cpp \
 *       src/core/control/tools/SnapToGridInputHandler.cpp \
 *       src/core/model/Snapping.cpp src/core/model/Point.cpp \
 *       src/core/model/Stroke.cpp src/core/model/Element.cpp \
 *       src/core/model/Layer.cpp src/core/model/XojPage.cpp \
 *       src/core/model/BackgroundConfig.cpp src/core/model/PageType.cpp \
 *       src/core/control/settings/Settings.cpp ... (see run-test.sh)
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

#include "control/settings/Settings.h"
#include "control/tools/ElementSnapper.h"
#include "control/tools/SnapToGridInputHandler.h"
#include "model/Layer.h"
#include "model/Stroke.h"
#include "model/XojPage.h"

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::printf("%-70s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) {
        ++failures;
    }
}

void checkClose(double a, double b, const char* what, double eps = 1e-9) {
    const bool ok = std::abs(a - b) <= eps;
    std::printf("%-70s %s  (%g vs %g)\n", what, ok ? "PASS" : "FAIL", a, b);
    if (!ok) {
        ++failures;
    }
}

/// Build a stroke from a list of (x, y) pairs, pressure left at NO_PRESSURE
std::unique_ptr<Stroke> makeStroke(std::initializer_list<std::pair<double, double>> pts) {
    auto s = std::make_unique<Stroke>();
    std::vector<Point> v;
    for (auto& [x, y]: pts) {
        v.emplace_back(x, y);
    }
    s->setPointVector(std::move(v));
    return s;
}

}  // namespace

int main() {
    // Settings wants a path; a throwaway one keeps the real settings.xml untouched
    Settings settings("/tmp/xpp-magnet-snapping-test-settings.xml");

    auto page = std::make_shared<XojPage>(595.0, 842.0);
    Layer* layer = page->getSelectedLayer();
    assert(layer);

    // A line from (100,100) to (200,100): its two ends are anchors
    layer->addElement(makeStroke({{100.0, 100.0}, {150.0, 100.0}, {200.0, 100.0}}));
    // A "rectangle" drawn by the rectangle tool: 5 points, closed. All corners are anchors
    layer->addElement(makeStroke({{300.0, 300.0}, {300.0, 400.0}, {400.0, 400.0}, {400.0, 300.0}, {300.0, 300.0}}));

    ElementSnapper snapper(&settings);
    snapper.setPageRef(page);

    settings.setSnapToElements(true);
    settings.setSnapToElementsTolerance(14.17);
    settings.setSnapGrid(false);   // keep the grid out of the way for the element tests
    settings.setSnapRotation(false);

    // 1. A point right on an end point snaps exactly onto it
    {
        Point p = snapper.snap(Point(198.0, 101.0), false);
        checkClose(p.x, 200.0, "point 2.2pt from the end of the line snaps onto it (x)");
        checkClose(p.y, 100.0, "point 2.2pt from the end of the line snaps onto it (y)");
    }

    // 2. A point far from everything is left alone
    {
        Point p = snapper.snap(Point(500.0, 700.0), false);
        checkClose(p.x, 500.0, "point far from every anchor is left alone (x)");
        checkClose(p.y, 700.0, "point far from every anchor is left alone (y)");
    }

    // 3. The latch holds: once grabbed, the point stays put even when the pen drifts away a bit
    {
        snapper.reset();
        Point a = snapper.snap(Point(200.5, 100.5), false);  // grabs (200,100)
        checkClose(a.x, 200.0, "the pen grabs the end point when it comes close");
        Point b = snapper.snap(Point(208.0, 100.0), false);  // 8pt away: inside the sticky radius
        checkClose(b.x, 200.0, "the anchor is kept while the pen is still within the sticky radius");
        Point c = snapper.snap(Point(230.0, 100.0), false);  // 30pt away: released
        checkClose(c.x, 230.0, "the anchor is released once the pen leaves the sticky radius");
    }

    // 4. A shape vertex is an anchor too (corner of the rectangle)
    {
        snapper.reset();
        Point p = snapper.snap(Point(402.0, 399.0), false);
        checkClose(p.x, 400.0, "a rectangle corner is an anchor (x)");
        checkClose(p.y, 400.0, "a rectangle corner is an anchor (y)");
    }

    // 5. Turning the feature off (or pressing Alt) disables the snapping
    {
        snapper.reset();
        Point p = snapper.snap(Point(199.0, 100.0), false);
        checkClose(p.x, 200.0, "snapping is active when the setting is on");
        Point q = snapper.snap(Point(199.0, 100.0), true);  // alt inverts
        checkClose(q.x, 199.0, "holding Alt suspends the magnet snapping");
    }

    // 6. Through the handler used by the tools: the element snapping wins over the grid
    {
        settings.setSnapGrid(true);
        settings.setSnapGridSize(14.17);
        settings.setSnapGridTolerance(0.5);
        SnapToGridInputHandler handler(&settings);
        handler.setPageRef(page);
        // (200,100) is not a multiple of 14.17, so the grid would move the point
        Point p = handler.snapWithElements(Point(201.0, 100.0), Point(100.0, 100.0), false);
        checkClose(p.x, 200.0, "element snapping wins over grid snapping (x)");
        checkClose(p.y, 100.0, "element snapping wins over grid snapping (y)");
        handler.resetElementSnapper();
    }

    // 7. Freehand strokes keep their two ends as anchors but do not expose every point
    {
        snapper.reset();
        auto s = std::make_unique<Stroke>();
        std::vector<Point> v;
        for (int i = 0; i < 5000; ++i) {
            v.emplace_back(10.0 + i * 0.1, 500.0);
        }
        s->setPointVector(std::move(v));
        layer->addElement(std::move(s));

        ElementSnapper fresh(&settings);
        fresh.setPageRef(page);
        Point p = fresh.snap(Point(10.5, 500.5), false);
        checkClose(p.x, 10.0, "the first point of a long freehand stroke is an anchor");
        Point q = fresh.snap(Point(260.0, 500.0), false);
        checkClose(q.x, 260.0, "a random interior point of a long stroke is not an anchor");
    }

    std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures,
                failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
