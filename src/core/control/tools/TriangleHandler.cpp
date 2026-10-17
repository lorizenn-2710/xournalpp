#include "TriangleHandler.h"

#include <algorithm>  // for max
#include <cmath>      // for sqrt

#include "control/Control.h"                       // for Control
#include "control/settings/Settings.h"             // for Settings
#include "control/tools/BaseShapeHandler.h"        // for BaseShapeHandler
#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHandler
#include "model/Point.h"                           // for Point

TriangleHandler::TriangleHandler(Control* control, const PageRef& page, bool flipShift, bool flipControl):
        BaseShapeHandler(control, page, flipShift, flipControl) {}

TriangleHandler::~TriangleHandler() = default;

auto TriangleHandler::createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
        -> std::pair<std::vector<Point>, Range> {
    Point c = snappingHandler.snapWithElements(this->currPoint, this->startPoint, isAltDown);

    double width = c.x - this->startPoint.x;
    double height = c.y - this->startPoint.y;

    this->modShift = isShiftDown;
    this->modControl = isControlDown;

    Settings* settings = control->getSettings();
    if (settings->getDrawDirModsEnabled()) {
        this->modifyModifiersByDrawDir(width, height, true);
    }

    // The base is horizontal and has the width of the gesture; the height is the vertical drag.
    const double base = std::abs(width);
    const double apexHeight = this->modShift ? std::sqrt(3.0) / 2.0 * base : std::abs(height);
    const double signH = height >= 0 ? 1.0 : -1.0;
    const double signW = width >= 0 ? 1.0 : -1.0;

    // Apex: above the middle of the base. When the drag has no vertical component the apex
    // would sit on the base, so give it a default height to keep the triangle visible.
    const double apexY = (apexHeight > 0.0 ? apexHeight : base) * signH;

    Point p1, p2, apex;
    if (!this->modControl) {
        // Draw beside the starting point
        p1 = Point(this->startPoint.x, this->startPoint.y);
        p2 = Point(this->startPoint.x + signW * base, this->startPoint.y);
        apex = Point(this->startPoint.x + signW * base / 2.0, this->startPoint.y - apexY);
    } else {
        // Control down: the starting point is the middle of the base
        p1 = Point(this->startPoint.x - base / 2.0, this->startPoint.y);
        p2 = Point(this->startPoint.x + base / 2.0, this->startPoint.y);
        apex = Point(this->startPoint.x, this->startPoint.y - apexY);
    }

    Range rg(p1.x, p1.y);
    rg.addPoint(p2.x, p2.y);
    rg.addPoint(apex.x, apex.y);

    return {{p1, p2, apex, p1}, rg};
}
