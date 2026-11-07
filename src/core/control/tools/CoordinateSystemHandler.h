/*
 * Xournal++
 *
 * Handles input to draw an coordinate system
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

class XournalView;

class CoordinateSystemHandler: public BaseShapeHandler {
public:
    CoordinateSystemHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);
    ~CoordinateSystemHandler() override;

    /// Pure geometry shared by the interactive preview and regression tests.
    static auto axes(const Point& start, const Point& end, bool spatial, bool firstQuadrant)
            -> std::pair<std::vector<Point>, Range>;

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;
};
