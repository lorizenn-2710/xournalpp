#include "ElementSnapper.h"

#include <algorithm>  // for max
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
                if (pts.size() <= POLYLINE_MAX_POINTS) {
                    for (const Point& p: pts) {
                        this->anchors.emplace_back(p.x, p.y);
                    }
                } else {
                    // Freehand curve: only the two ends, so that drawing next to it is not
                    // constantly pulled onto it
                    this->anchors.emplace_back(pts.front().x, pts.front().y);
                    this->anchors.emplace_back(pts.back().x, pts.back().y);
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

auto ElementSnapper::snap(Point const& pos, bool alt, bool latch) const -> Point {
    if (!this->settings) {
        return pos;
    }
    // Alt inverts the setting, exactly like the grid and rotation snapping do
    if (alt == this->settings->isSnapToElements()) {
        if (latch) {
            this->latched.reset();
        }
        return pos;
    }

    const double tolerance = this->settings->getSnapToElementsTolerance();
    if (tolerance <= 0.0) {
        return pos;
    }

    if (!this->built) {
        this->rebuild();
    }

    // Keep the anchor the pen already grabbed as long as it is close enough. This is the
    // "magnetic pause" that makes a connection feel deliberate.
    if (latch && this->latched) {
        if (pos.lineLengthTo(*this->latched) <= tolerance * STICKY_FACTOR) {
            return *this->latched;
        }
        this->latched.reset();
    }

    if (this->anchors.empty()) {
        return pos;
    }

    const Point* best = nullptr;
    double bestDistance = tolerance;
    for (const Point& a: this->anchors) {
        const double d = pos.lineLengthTo(a);
        if (d <= bestDistance) {
            bestDistance = d;
            best = &a;
        }
    }

    if (!best) {
        return pos;
    }
    if (latch) {
        this->latched = *best;
    }
    return *best;
}

auto ElementSnapper::getLatchedAnchor() const -> std::optional<Point> { return this->latched; }
