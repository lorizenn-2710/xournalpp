/*
 * Xournal++
 *
 * Handles input to draw a rectangular box in oblique projection
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <vector>  // for vector

#include "model/PageRef.h"        // for PageRef
#include "model/Point.h"          // for Point
#include "model/SolidGeometry.h"  // for SolidGeometry::Wireframe

#include "BaseShapeHandler.h"  // for BaseShapeHandler

class Control;

/**
 * @brief Draws a rectangular box (hinh hop) in oblique projection.
 *
 * The gesture defines the FRONT face of the box, exactly like the rectangle tool. A second
 * parallelogram is drawn behind it, offset up and to the right, and four edges connect the two
 * faces. With Shift the front face is a square, giving a cube.
 *
 * The edges the viewer cannot see — the three meeting at the corner that lands behind the front
 * face — are drawn dashed, which is what a textbook figure looks like. Which edges those are is
 * decided from the geometry of the figure; see SolidGeometry.
 */
class BoxHandler: public BaseShapeHandler {
public:
    BoxHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);
    ~BoxHandler() override;

    /**
     * @brief How deep the box is, as a fraction of the size of its front face
     */
    static constexpr double DEPTH_FRACTION = 0.45;

    /**
     * @brief Direction of the depth, as a fraction of the depth. Up and to the right, which is
     * the convention in Vietnamese textbooks.
     */
    static constexpr double DEPTH_DX = 0.75;
    static constexpr double DEPTH_DY = -0.75;

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;

    auto createHiddenParts(const std::vector<Point>& shape) const -> std::vector<std::vector<Point>> override;

    /// Wireframe of the shape most recently built by createShape(), used by createHiddenParts()
    SolidGeometry::Wireframe wireframe;
};
