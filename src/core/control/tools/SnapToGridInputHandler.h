/*
 * Xournal++
 *
 * Snapping methods which take account of the settings
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */
#pragma once

#include <optional>  // for optional

#include "model/PageRef.h"  // for PageRef
#include "model/Point.h"

#include "ElementSnapper.h"  // for ElementSnapper

class BackgroundConfig;
class Settings;

class SnapToGridInputHandler final {

public:
    SnapToGridInputHandler(const Settings* settings);

    /**
     * @brief Updates the page context for grid offset calculations
     * @param page the current page (used to get background configuration)
     */
    void setPageRef(PageRef page);

protected:
    const Settings* settings;
    PageRef currentPage;

    /**
     * @brief Snapping to the anchor points of the elements already on the page
     */
    ElementSnapper elementSnapper;

    // Cached offsets (computed once per page)
    mutable double cachedXOffset = 0.0;
    mutable double cachedYOffset = 0.0;
    mutable bool offsetsCached = false;

    /**
     * @brief Ensure cached offsets are computed and up-to-date
     */
    void ensureOffsetsCached() const;

    /**
     * @brief Calculate grid offsets from the current page's background configuration
     * @param xOffset output parameter for horizontal grid offset
     * @param yOffset output parameter for vertical grid offset
     * @param pageWidth page width
     * @param pageHeight page height
     */
    void calculateGridOffsets(double& xOffset, double& yOffset, double pageWidth, double pageHeight) const;

    /**
     * @brief Calculate offsets for isometric backgrounds
     */
    void calculateIsometricOffsets(const BackgroundConfig& config, double pageWidth, double pageHeight, double& xOffset,
                                   double& yOffset) const;

    /**
     * @brief Check if current page has an isometric background
     * @param triangleSize output parameter for triangle size (if isometric)
     * @return true if current page has isometric background
     */
    bool isIsometricBackground(double& triangleSize) const;

public:
    /**
     * @brief If a value is near enough to the y-coordinate of a grid point, it returns the nearest y-coordinate of the
     * grid point. Otherwise the original value itself.
     * @param y the value
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     */
    [[nodiscard]] double snapVertically(double y, bool alt) const;

    /**
     * @brief If a value is near enough to the x-coordinate of a grid point, it returns the nearest x-coordinate of the
     * grid point. Otherwise the original value itself.
     * @param x the value
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     */
    [[nodiscard]] double snapHorizontally(double x, bool alt) const;

    /**
     * @brief If a points distance to the nearest grid point is under a certain tolerance, it returns the nearest
     * grid point. Otherwise the original Point itself.
     * @param pos the position
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     */
    [[nodiscard]] Point snapToGrid(Point const& pos, bool alt) const;

    /**
     * @brief if the angles distance to a multiple quarter of PI is under a certain tolerance, it returns the latter.
     * Otherwise the original angle.
     * @param radian the angle (in radian)
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     */
    [[nodiscard]] double snapAngle(double radian, bool alt) const;

    /**
     * @brief Snaps the angle between the horizontal axis and the line between the given point and center
     * and returns the point rotated accordingly
     * @param pos the coordinate of the point
     * @param center the center of rotation
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     */
    [[nodiscard]] Point snapRotation(Point const& pos, Point const& center, bool alt) const;

    /**
     * @brief Does rotation snapping followed by snapping to grid
     * @param pos the coordinate of the point
     * @param center the center of rotation
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     */
    [[nodiscard]] Point snap(Point const& pos, Point const& center, bool alt) const;

    /**
     * @brief Snap to the anchor points (stroke ends, shape vertices, box corners) of the
     *        elements already on the page, on top of the grid/rotation snapping.
     *
     * This is what makes two strokes meet exactly instead of leaving a small gap. The
     * element snapping wins over the grid snapping: connecting to a real point of the
     * drawing matters more than landing on a grid line.
     *
     * @param pos the coordinate of the point
     * @param alt indicates whether snapping mode is altered (via the Alt key)
     * @param latch whether the snapper may hold on to the grabbed anchor, see ElementSnapper::snap
     */
    [[nodiscard]] Point snapWithElements(Point const& pos, Point const& center, bool alt, bool latch = true) const;

    /**
     * @brief Snap to the anchor points of the elements already on the page, falling back to grid
     *        snapping only (no rotation snapping — used by area shapes like rectangle/triangle/box/pyramid/ellipse).
     */
    [[nodiscard]] Point snapToGridWithElements(Point const& pos, bool alt, bool latch = true,
                                               std::optional<Point> exclude = std::nullopt) const;

    /**
     * @brief Snap a single point to the elements of the page, without grid or rotation snapping
     * @param latch whether the snapper may hold on to the grabbed anchor
     */
    [[nodiscard]] Point snapToElements(Point const& pos, bool alt, bool latch = true) const;

    /**
     * @brief The anchor the snapper is currently holding, if any. Used to draw the marker.
     */
    [[nodiscard]] std::optional<Point> getLatchedAnchor() const;

    /**
     * @brief Drop the latched anchor and re-read the page's elements on the next snap
     */
    void resetElementSnapper();
};
