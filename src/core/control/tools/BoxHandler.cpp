#include "BoxHandler.h"

#include <algorithm>  // for max
#include <cmath>      // for abs

#include "control/Control.h"                       // for Control
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
    Point c = snappingHandler.snapToGridWithElements(this->currPoint, isAltDown, true, this->startPoint);

    double width = c.x - this->startPoint.x;
    double height = c.y - this->startPoint.y;

    this->modShift = isShiftDown != flipShift;
    this->modControl = isControlDown != flipControl;

    if (this->modShift) {
        // square front face -> cube
        const int signW = width > 0 ? 1 : -1;
        const int signH = height > 0 ? 1 : -1;
        width = std::max(width * signW, height * signH) * signW;
        height = (width * signW) * signH;
    }

    if (this->modControl) {
        this->wireframe = SolidGeometry::triangularPrismWireframe(
                this->startPoint, Point(this->startPoint.x + width, this->startPoint.y + height));
    } else {
        this->wireframe = SolidGeometry::boxWireframe(this->startPoint,
                                                      Point(this->startPoint.x + width, this->startPoint.y + height),
                                                      DEPTH_FRACTION, DEPTH_DX, DEPTH_DY);
    }
    std::vector<Point> shape = this->modControl ?
                                       SolidGeometry::triangularOutline(this->wireframe, this->getDrawHiddenEdges()) :
                                       SolidGeometry::boxOutline(this->wireframe, this->getDrawHiddenEdges());

    Range rg(shape.front().x, shape.front().y);
    for (const Point& p: shape) {
        rg.addPoint(p.x, p.y);
    }
    for (const Point& p: this->wireframe.vertices) {
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
