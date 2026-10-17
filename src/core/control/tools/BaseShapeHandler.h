/*
 * Xournal++
 *
 * Handles input of the ruler
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <memory>  // for shared_ptr
#include <optional>  // for optional
#include <utility>  // for pair
#include <vector>   // for vector

#include <gdk/gdk.h>  // for GdkEventKey

#include "model/PageRef.h"  // for PageRef
#include "model/Point.h"    // for Point
#include "util/Range.h"     // for Range

#include "InputHandler.h"            // for InputHandler
#include "SnapToGridInputHandler.h"  // for SnapToGridInputHandler

class PositionInputData;

namespace xoj::util {
template <class T>
class DispatchPool;
}

namespace xoj::view {
class OverlayView;
class Repaintable;
class ShapeToolView;
};

enum DIRSET_MODIFIERS { NONE = 0, SET = 1, SHIFT = 1 << 1, CONTROL = 1 << 2 };


class BaseShapeHandler: public InputHandler {
public:
    BaseShapeHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);

    ~BaseShapeHandler() override;

    void onSequenceCancelEvent() override;
    bool onMotionNotifyEvent(const PositionInputData& pos, double zoom) override;
    void onButtonReleaseEvent(const PositionInputData& pos, double zoom) override;
    void onButtonPressEvent(const PositionInputData& pos, double zoom) override;
    void onButtonDoublePressEvent(const PositionInputData& pos, double zoom) override;
    bool onKeyPressEvent(const KeyEvent& event) override;
    bool onKeyReleaseEvent(const KeyEvent& event) override;

    std::unique_ptr<xoj::view::OverlayView> createView(xoj::view::Repaintable* parent) const override;

    const std::shared_ptr<xoj::util::DispatchPool<xoj::view::ShapeToolView>>& getViewPool() const;

    /**
     * @brief Get the shape's points.
     */
    const std::vector<Point>& getShape() const;

    /**
     * @brief The anchor point of an existing element the shape is currently snapped to, if any.
     * Used to draw the on-screen magnet marker.
     */
    auto getSnapAnchor() const -> std::optional<Point>;

    /**
     * @brief The parts of the current shape that the viewer cannot see, to be drawn DASHED.
     *
     * WHY THIS EXISTS: a solid figure (a box, a pyramid) is a closed wireframe, but it is
     * unreadable if every edge is drawn the same way. Every textbook draws the edges the viewer
     * cannot see as dashed lines, and doing that by hand is exactly where the figure comes out
     * wrong.
     *
     * The pieces are computed from the shape that was just built (see createShape), so they can
     * never disagree with the outline. They are exposed to the view so that the preview under the
     * pen already shows the dashes, exactly as the figure will be committed.
     *
     * Empty for a flat shape: the whole outline is then one solid stroke, as before.
     */
    auto getHiddenParts() const -> const std::vector<std::vector<Point>>&;

    /**
     * @brief Whether the hidden (dashed) edges should be drawn at all. False when the user's pen
     * style makes a dashed line meaningless (the hidden edges are then part of the solid outline).
     */
    auto getDrawHiddenEdges() const -> bool;

private:
    /**
     * @brief Create the shape (to be drawn and added as a stroke), depending on the last event in
     * @return Pair [vector, range] where vector contains the points drawing the shape and range is the smallest range
     * containing all those points. WARNING: Stroke thickness is not taken into account.
     */
    virtual std::pair<std::vector<Point>, Range> createShape(bool isAltDown, bool isShiftDown, bool isControlDown) = 0;

    /**
     * @brief Compute the parts of the shape the viewer cannot see. Overridden by the handlers that
     * draw a solid figure; the default (a flat shape) has none.
     *
     * @param shape the points of the finished shape, as returned by createShape()
     */
    virtual auto createHiddenParts(const std::vector<Point>& shape) const -> std::vector<std::vector<Point>> {
        (void)shape;
        return {};
    }

    /**
     * @brief Update the current shape with the latest event info.
     *      Also warns the listeners about the change, usually triggering a redraw during the next screen update.
     */
    void updateShape(bool isAltDown, bool isShiftDown, bool isControlDown);

    /**
     * @brief Cancel the current shape creation: clears all data and wipes any drawing made
     */
    void cancelStroke();

    bool onKeyEvent(const KeyEvent& event, bool pressed);

protected:
    /**
     * modifyModifiersByDrawDir
     * @brief 	Toggle shift and control modifiers depending on initial drawing direction.
     */
    void modifyModifiersByDrawDir(double width, double height, double zoom, bool changeCursor = true);

protected:
    std::vector<Point> shape;

    /**
     * @brief Parts of `shape` the viewer cannot see (see getHiddenParts). Recomputed on every
     * updateShape() so the preview and the committed figure always agree.
     */
    std::vector<std::vector<Point>> hiddenParts;

    /**
     * @brief Bounding box of the stroke after its last update
     *      WARNING: The stroke width is not taken into account (i.e. this is the snapping box)
     */
    Range lastSnappingRange;

    DIRSET_MODIFIERS drawModifierFixed = NONE;
    bool flipShift =
            false;  // use to reverse Shift key modifier action. i.e.  for separate Rectangle and Square Tool buttons.
    bool flipControl = false;  // use to reverse Control key modifier action.
    bool modShift = false;
    bool modControl = false;
    SnapToGridInputHandler snappingHandler;

    Point currPoint;
    Point buttonDownPoint;  // used for tapSelect and filtering - never snapped to grid.
    Point startPoint;       // May be snapped to grid

    std::shared_ptr<xoj::util::DispatchPool<xoj::view::ShapeToolView>> viewPool;
};
