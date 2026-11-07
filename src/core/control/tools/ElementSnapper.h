/*
 * Xournal++
 *
 * Magnet snapping to the anchor points of the elements already on the page.
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cstddef>   // for size_t
#include <optional>  // for optional
#include <vector>    // for vector

#include "model/PageRef.h"  // for PageRef
#include "model/Point.h"    // for Point

class Settings;

/**
 * @brief Snaps a position to the anchor points of the elements of a layer.
 *
 * "Anchor point" means a point a user would naturally want to connect to:
 *  - the two ends of a freehand stroke;
 *  - every vertex of a stroke that was drawn with one of the shape tools
 *    (a rectangle is a closed polyline, so its corners are vertices);
 *  - midpoints of straight edges at least 35pt long (slightly smaller capture radius);
 *  - the centre and four cardinal extrema of a dense closed geometry stroke;
 *  - the four corners of the box of a text or an image.
 *
 * The snapper latches onto an anchor: once the pen has grabbed a point, it keeps
 * returning that point until the pen has moved further away than the sticky
 * radius. That hysteresis is what makes the point feel "magnetic" instead of
 * merely being rounded to the nearest anchor.
 */
class ElementSnapper {
public:
    explicit ElementSnapper(const Settings* settings);

    /**
     * @brief Set the page whose elements are used as anchors
     */
    void setPageRef(PageRef page);

    /**
     * @brief Drop the cached anchors and the latched point
     */
    void reset();

    /**
     * @brief Snap to the closest anchor point of the page
     * @param pos the position, in page coordinates
     * @param alt whether the snapping mode is inverted (Alt key), like the other snapping helpers
     * @param latch whether the snapper may hold on to the point it just grabbed. Pass false for
     *        one-shot snaps (e.g. the first point of a shape) so that the latch stays free for the
     *        point the user is currently dragging.
     * @param exclude optional anchor point to ignore (e.g. the start point of the current drag, so
     *        the dragged end point never snaps back onto the start point).
     */
    [[nodiscard]] Point snap(Point const& pos, bool alt, bool latch = true,
                             std::optional<Point> exclude = std::nullopt) const;

    /**
     * @brief The anchor the last snapping call grabbed, if any. Used to draw the on-screen marker.
     */
    [[nodiscard]] std::optional<Point> getLatchedAnchor() const;

    /**
     * @brief How much further than the tolerance the pen may move before the anchor is released
     */
    static constexpr double STICKY_FACTOR = 1.5;
    // Shared nearest-anchor rule for declared coordinate points in teaching previews.
    static std::optional<size_t> nearestAnchor(const Point&, const std::vector<Point>&, double tolerance,
                                               std::optional<Point> exclude = std::nullopt);

private:
    /**
     * @brief Collect the anchor points of the page's selected layer. Called once per gesture.
     */
    void rebuild() const;

    const Settings* settings;
    PageRef currentPage;

    mutable bool built = false;
    mutable std::vector<Point> anchors;
    mutable std::vector<Point> midpoints;
    mutable double latchedRadiusFactor = 1.0;
    mutable std::optional<Point> latched;
};
