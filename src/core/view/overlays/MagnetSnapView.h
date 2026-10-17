/*
 * Xournal++
 *
 * Overlay view drawing the magnet marker on the anchor point the pen snapped to.
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cairo.h>  // for cairo_t

#include "model/Point.h"  // for Point

#include "view/overlays/OverlayView.h"  // for OverlayView

namespace xoj::view {
class Repaintable;

/**
 * @brief Draws a small ring on the anchor point the pen is currently snapped to.
 *
 * Without this marker the snapping is invisible: the user cannot tell whether the pen has
 * grabbed the end point of the previous stroke or is merely drawing next to it. The marker
 * is what turns "the line stopped short" into "the line is connected".
 */
class MagnetSnapView: public OverlayView {
public:
    MagnetSnapView(Repaintable* parent, Point anchor);

    ~MagnetSnapView() noexcept override;

    void draw(cairo_t* cr) const override;

    bool isViewOf(const OverlayBase* overlay) const override;

    /**
     * @brief The point this view draws. Used to detect that the marker has moved.
     */
    [[nodiscard]] const Point& getAnchor() const { return this->anchor; }

    /**
     * @brief Radius of the drawn ring, in page units. Big enough to be seen through the pen tip.
     */
    static constexpr double RADIUS = 9.0;

private:
    Point anchor;
};
};  // namespace xoj::view
