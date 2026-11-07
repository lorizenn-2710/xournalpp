#include "ElementSnapper.h"

#include <algorithm>  // for max
#include <cmath>      // for hypot
#include <utility>    // for move

#include "control/settings/Settings.h"  // for Settings
#include "model/Element.h"              // for Element, ElementType
#include "model/Layer.h"                // for Layer
#include "model/Stroke.h"               // for Stroke
#include "model/XojPage.h"              // for XojPage

/**
 * @brief A stroke with at most this many points was drawn by a shape tool (line, triangle,
 *        rectangle, arrow, ellipse) rather than freehand, so every vertex is a corner worth
 *        snapping to. Above that limit the stroke is treated as a freehand curve: only its two
 *        ends are anchors, otherwise a freehand stroke drawn next to an existing one would be
 *        pulled onto it all along its length.
 */
constexpr size_t POLYLINE_MAX_POINTS = 256;

ElementSnapper::ElementSnapper(const Settings* settings): settings(settings) {}

void ElementSnapper::setPageRef(PageRef page) {
    this->currentPage = std::move(page);
    this->built = false;
    this->latched.reset();
}

void ElementSnapper::reset() {
    this->built = false;
    this->latched.reset();
}

void ElementSnapper::rebuild() const {
    this->anchors.clear();
    this->midpoints.clear();
    this->built = true;

    if (!this->currentPage) {
        return;
    }
    Layer* layer = this->currentPage->getSelectedLayer();
    if (!layer) {
        return;
    }

    for (const ElementPtr& e: layer->getElements()) {
        if (!e) {
            continue;
        }
        switch (e->getType()) {
            case ELEMENT_STROKE: {
                const auto* s = static_cast<const Stroke*>(e.get());
                const std::vector<Point>& pts = s->getPointVector();
                if (pts.empty()) {
                    break;
                }
                // EllipseHandler increases its sampling density with the radius: classroom
                // ellipses can exceed the short-polyline limit without being freehand curves.
                const bool closedCurve = !s->hasPressure() && pts.size() > 20 && pts.size() <= 4096 &&
                                         pts.front().lineLengthTo(pts.back()) < 2.0;
                if (closedCurve) {
                    const auto& bb = s->getBoundingBox();
                    const double cx = bb.x + bb.width / 2.0, cy = bb.y + bb.height / 2.0;
                    this->anchors.insert(this->anchors.end(),
                                         {Point(cx, cy), Point(cx, bb.y), Point(cx, bb.y + bb.height), Point(bb.x, cy),
                                          Point(bb.x + bb.width, cy)});
                } else if (!s->hasPressure() && pts.size() <= POLYLINE_MAX_POINTS) {
                    for (const Point& p: pts) {
                        this->anchors.emplace_back(p.x, p.y);
                    }
                    if (pts.size() <= 20) {
                        for (std::size_t i = 1; i < pts.size(); ++i) {
                            if (pts[i - 1].lineLengthTo(pts[i]) >= 35.0) {
                                this->midpoints.emplace_back((pts[i - 1].x + pts[i].x) / 2.0,
                                                             (pts[i - 1].y + pts[i].y) / 2.0);
                            }
                        }
                    }
                } else {
                    // Freehand curve: only the two ends (and skip tiny handwritten characters/marks
                    // that have pressure and bounding box < 30pt), so that drawing next to handwriting
                    // is not constantly pulled onto letters.
                    const xoj::util::Rectangle<double>& bb = s->getBoundingBox();
                    if (!s->hasPressure() || std::hypot(bb.width, bb.height) >= 30.0) {
                        this->anchors.emplace_back(pts.front().x, pts.front().y);
                        this->anchors.emplace_back(pts.back().x, pts.back().y);
                    }
                }
                break;
            }
            case ELEMENT_TEXT:
            case ELEMENT_IMAGE:
            case ELEMENT_TEXIMAGE: {
                const xoj::util::Rectangle<double>& bb = e->getBoundingBox();
                this->anchors.emplace_back(bb.x, bb.y);
                this->anchors.emplace_back(bb.x + bb.width, bb.y);
                this->anchors.emplace_back(bb.x, bb.y + bb.height);
                this->anchors.emplace_back(bb.x + bb.width, bb.y + bb.height);
                break;
            }
            default:
                break;
        }
    }
}

auto ElementSnapper::nearestAnchor(const Point& pos, const std::vector<Point>& candidates, double tolerance,
                                   std::optional<Point> exclude) -> std::optional<size_t> {
    std::optional<size_t> best;
    double distance = tolerance;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (exclude && candidates[i].lineLengthTo(*exclude) <= 1.)
            continue;
        double d = pos.lineLengthTo(candidates[i]);
        if (d <= tolerance && (!best || d < distance - 1e-9)) {
            best = i;
            distance = d;
        }
    }
    return best;
}

auto ElementSnapper::snap(Point const& pos, bool alt, bool latch, std::optional<Point> exclude) const -> Point {
    if (!this->settings) {
        return pos;
    }
    // Alt inverts the setting, exactly like the grid and rotation snapping do
    if (alt == this->settings->isSnapToElements()) {
        this->latched.reset();
        return pos;
    }

    const double tolerance = this->settings->getSnapToElementsTolerance();
    if (tolerance <= 0.0) {
        this->latched.reset();
        return pos;
    }

    if (!this->built) {
        this->rebuild();
    }

    // Keep the anchor the pen already grabbed as long as it is close enough. This is the
    // "magnetic pause" that makes a connection feel deliberate.
    if (latch && this->latched) {
        if (exclude.has_value() && this->latched->lineLengthTo(*exclude) <= 1.0) {
            this->latched.reset();
        } else if (pos.lineLengthTo(*this->latched) <= tolerance * this->latchedRadiusFactor * STICKY_FACTOR) {
            return *this->latched;
        } else {
            this->latched.reset();
        }
    }

    if (this->anchors.empty() && this->midpoints.empty()) {
        this->latched.reset();
        return pos;
    }

    const Point* best = nullptr;
    double bestDistance = tolerance;
    double radiusFactor = 1.0;
    auto consider = [&](const std::vector<Point>& candidates, double factor) {
        auto index = nearestAnchor(pos, candidates, tolerance * factor, exclude);
        if (index) {
            const auto& a = candidates[*index];
            double d = pos.lineLengthTo(a);
            if (!best || d < bestDistance - 1e-9) {
                bestDistance = d;
                best = &a;
                radiusFactor = factor;
            }
        }
    };
    consider(this->anchors, 1.0);
    consider(this->midpoints, 0.85);

    if (!best) {
        this->latched.reset();
        return pos;
    }
    this->latched = *best;
    this->latchedRadiusFactor = radiusFactor;
    return *best;
}

auto ElementSnapper::getLatchedAnchor() const -> std::optional<Point> { return this->latched; }
