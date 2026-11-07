#include "CoordinateSystemHandler.h"

#include <algorithm>
#include <cmath>

#include "control/Control.h"                       // for Control
#include "control/tools/BaseShapeHandler.h"        // for BaseShapeHandler
#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHan...
#include "gui/XournalView.h"                       // for XournalView
#include "gui/inputdevices/PositionInputData.h"    // for PositionInputData
#include "model/Point.h"                           // for Point

CoordinateSystemHandler::CoordinateSystemHandler(Control* control, const PageRef& page, bool flipShift,
                                                 bool flipControl):
        BaseShapeHandler(control, page, flipShift, flipControl) {}

CoordinateSystemHandler::~CoordinateSystemHandler() = default;

// All branches retrace real axis/arrow segments, so no diagonal joins are introduced.
auto CoordinateSystemHandler::axes(const Point& start, const Point& end, bool spatial, bool firstQuadrant)
        -> std::pair<std::vector<Point>, Range> {
    const double width = std::abs(end.x - start.x);
    const double height = std::abs(end.y - start.y);
    std::vector<Point> points;
    auto axis = [&](const Point& from, const Point& tip, const Point& origin) {
        const double length = from.lineLengthTo(tip);
        if (length < 1e-6) {
            return;
        }
        const double ux = (tip.x - from.x) / length;
        const double uy = (tip.y - from.y) / length;
        const double head = std::min(10.0, length * 0.12);
        points.insert(
                points.end(),
                {from, tip, Point(tip.x - head * ux - head * 0.45 * uy, tip.y - head * uy + head * 0.45 * ux), tip,
                 Point(tip.x - head * ux + head * 0.45 * uy, tip.y - head * uy - head * 0.45 * ux), tip, origin});
    };
    if (spatial) {
        // Vietnamese textbook convention: Oz up, Oy right, Ox down-left.
        const Point o = start;
        const double depth = 0.6 * std::min(width, height) / std::sqrt(2.0);
        axis(o, Point(o.x, o.y - height), o);
        axis(o, Point(o.x + width, o.y), o);
        axis(o, Point(o.x - depth, o.y + depth), o);
    } else {
        const double left = std::min(start.x, end.x), top = std::min(start.y, end.y);
        const Point o(firstQuadrant ? left : left + width / 2.0, firstQuadrant ? top + height : top + height / 2.0);
        axis(Point(left, o.y), Point(left + width, o.y), o);
        // Return to the next branch's start along its axis, through O.
        points.emplace_back(o.x, top + height);
        axis(Point(o.x, top + height), Point(o.x, top), o);
    }
    // Adjacent retrace joins need only one point. Keep Oxyz below the short-polyline
    // threshold so snapping sees real axes and midpoints rather than a closed ellipse.
    points.erase(std::unique(points.begin(), points.end(),
                             [](const Point& a, const Point& b) { return a.lineLengthTo(b) < 1e-9; }),
                 points.end());
    Range range(start.x, start.y);
    for (const auto& p: points) {
        range.addPoint(p.x, p.y);
    }
    return {std::move(points), range};
}

auto CoordinateSystemHandler::createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
        -> std::pair<std::vector<Point>, Range> {
    const Point c = snappingHandler.snapToGridWithElements(currPoint, isAltDown, true, startPoint);
    // Explicit modifiers select the mode independently of the direction of the drag.
    return axes(startPoint, c, isShiftDown != flipShift, isControlDown != flipControl);
}
