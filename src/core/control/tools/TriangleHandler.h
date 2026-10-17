/*
 * Xournal++
 *
 * Handles input to draw a triangle
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <vector>  // for vector

#include "model/PageRef.h"  // for PageRef

#include "BaseShapeHandler.h"  // for BaseShapeHandler

class Point;
class Control;

/**
 * @brief Draws a triangle.
 *
 * By default the triangle is isosceles, standing on the horizontal base that the drag defines:
 * the base is the horizontal segment under the pen, the apex sits above its middle. With Shift
 * the triangle is equilateral, which is what geometry exercises ask for. With Control the
 * triangle is drawn around the point where the gesture started instead of beside it.
 */
class TriangleHandler: public BaseShapeHandler {
public:
    TriangleHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);
    ~TriangleHandler() override;

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;
};
