#include "BoxHandler.h"

#include <algorithm>  // for max
#include <cmath>      // for abs

#include "control/Control.h"                       // for Control
#include "control/settings/Settings.h"             // for Settings
#include "control/tools/BaseShapeHandler.h"        // for BaseShapeHandler
#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHandler
#include "model/Point.h"                           // for Point
#include "model/SolidGeometry.h"                   // for SolidGeometry
#include "util/Range.h"                            // for Range

BoxHandler::BoxHandler(Control* control, const PageRef& page, bool flipShift, bool flipControl):
        BaseShapeHandler(control, page, flipShift, flipControl) {}

BoxHandler::~BoxHandler() = default;

auto BoxHandler::createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
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

    if (this->modShift) {
        // square front face -> cube
        const int signW = width > 0 ? 1 : -1;
        const int signH = height > 0 ? 1 : -1;
        width = std::max(width * signW, height * signH) * signW;
        height = (width * signW) * signH;
    }

    // Front face, like the rectangle tool
    Point f1, f2;
    if (!this->modControl) {
        f1 = this->startPoint;
        f2 = Point(this->startPoint.x + width, this->startPoint.y + height);
    } else {
        // Control: draw from the centre
        f1 = Point(this->startPoint.x - width, this->startPoint.y - height);
        f2 = Point(this->startPoint.x + width, this->startPoint.y + height);
    }

    this->wireframe = SolidGeometry::boxWireframe(f1, f2, DEPTH_FRACTION, DEPTH_DX, DEPTH_DY);

    // The outline comes from the same wireframe the hidden edges are derived from, so the two can
    // never disagree. It is six polylines, not one sequence: see SolidGeometry::boxOutline.
    std::vector<Point> shape = SolidGeometry::boxOutline(this->wireframe);

    Range rg(shape.front().x, shape.front().y);
    for (const Point& p: shape) {
        rg.addPoint(p.x, p.y);
    }
    return {std::move(shape), rg};
}

auto BoxHandler::createHiddenParts(const std::vector<Point>& shape) const -> std::vector<std::vector<Point>> {
    // Which edges are hidden is decided from the geometry of the figure — which of its faces turn
    // away from the viewer — not from the order the outline happens to be written in. `shape` is
    // unused on purpose: the wireframe it was built from is kept as a member, so the dashed edges
    // and the outline can never disagree.
    (void)shape;
    if (this->wireframe.vertices.empty()) {
        return {};
    }
    return SolidGeometry::hiddenEdgePolylines(this->wireframe);
}
