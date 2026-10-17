#include "PyramidHandler.h"

#include <algorithm>  // for max
#include <cmath>      // for abs

#include "control/Control.h"                       // for Control
#include "control/settings/Settings.h"             // for Settings
#include "control/tools/BaseShapeHandler.h"        // for BaseShapeHandler
#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHandler
#include "model/Point.h"                           // for Point
#include "model/SolidGeometry.h"                   // for SolidGeometry
#include "util/Range.h"                            // for Range

PyramidHandler::PyramidHandler(Control* control, const PageRef& page, bool flipShift, bool flipControl):
        BaseShapeHandler(control, page, flipShift, flipControl) {}

PyramidHandler::~PyramidHandler() = default;

auto PyramidHandler::createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
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

    const double base = std::abs(width);
    // The base of the pyramid: a parallelogram, like every textbook draws it. The front edge is
    // the horizontal one; the two side edges run along the oblique depth, so the far edge sits
    // BEHIND and slightly ABOVE the front edge — not level with it. Drawing the far edge level with
    // the front one (or lifting only its two corners, as an earlier version did) makes the base a
    // trapezium, which reads as a tilted, non-planar quadrilateral instead of a parallelogram.
    //
    // Vertical drag = height of the apex above the base. A drag with no vertical component would
    // put the apex on the base, so fall back to a proportion of the width.
    const double apexHeight = std::abs(height) > 0.0 ? std::abs(height) : base;
    // Follow the drag direction, like the triangle tool does: dragging down puts the apex above the
    // base, dragging up mirrors the figure (the apex hangs below the base). A pyramid is therefore
    // drawn upright by dragging down, which is the natural gesture.
    const double signH = height >= 0 ? 1.0 : -1.0;
    const double signW = width >= 0 ? 1.0 : -1.0;

    // Centre of the base
    Point centre;
    if (!this->modControl) {
        centre = Point(this->startPoint.x + signW * base / 2.0, this->startPoint.y);
    } else {
        centre = this->startPoint;
    }

    this->wireframe = SolidGeometry::pyramidWireframe(centre, base, apexHeight * signH, BASE_DEPTH_FRACTION, DEPTH_DX,
                                                      DEPTH_DY);

    // The outline comes from the same wireframe the hidden edges are derived from, so the two can
    // never disagree. It is six polylines, not one sequence: see SolidGeometry::pyramidOutline.
    std::vector<Point> shape = SolidGeometry::pyramidOutline(this->wireframe);

    Range rg(shape.front().x, shape.front().y);
    for (const Point& p: shape) {
        rg.addPoint(p.x, p.y);
    }
    return {std::move(shape), rg};
}

auto PyramidHandler::createHiddenParts(const std::vector<Point>& shape) const -> std::vector<std::vector<Point>> {
    // As for the box: the dashed edges come from the geometry of the figure, not from the order the
    // outline is written in.
    (void)shape;
    if (this->wireframe.vertices.empty()) {
        return {};
    }
    return SolidGeometry::hiddenEdgePolylines(this->wireframe);
}
