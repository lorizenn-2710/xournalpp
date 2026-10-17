/*
 * Xournal++
 *
 * Handles input to draw a quadrilateral pyramid
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
 * @brief Draws a quadrilateral pyramid (hinh chop tu giac).
 *
 * The base is a parallelogram drawn in oblique projection, so that it reads as a quadrilateral seen
 * in perspective rather than a flat rectangle. The apex sits above the middle of the base. The
 * gesture defines the height of the apex and the width of the base; the base depth follows the same
 * oblique convention as the box tool.
 *
 * The edges the viewer cannot see are drawn dashed: normally the far base edge, and the lateral edge
 * from the apex down to the hidden corner of the base when the apex is tall enough that this edge
 * stays inside the outline. Which edges those are is decided from the geometry of the figure, not
 * hard-coded; see SolidGeometry.
 */
class PyramidHandler: public BaseShapeHandler {
public:
    PyramidHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);
    ~PyramidHandler() override;

    /**
     * @brief Depth of the base, as a fraction of its width
     */
    static constexpr double BASE_DEPTH_FRACTION = 0.45;

    /**
     * @brief Direction of the base depth (up and to the right)
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
