#include "ShapeToolView.h"

#include <vector>

#include "control/tools/BaseShapeHandler.h"
#include "model/LineStyle.h"      // for LineStyle
#include "model/StrokeStyle.h"    // for parseStyle, scaleStyle
#include "util/Util.h"            // for cairo_set_dash_from_vector
#include "util/raii/CairoWrappers.h"
#include "view/Repaintable.h"
#include "view/StrokeViewHelper.h"

using namespace xoj::view;

ShapeToolView::ShapeToolView(const BaseShapeHandler* toolHandler, Repaintable* parent):
        BaseShapeOrSplineToolView(toolHandler, parent), toolHandler(toolHandler) {
    this->registerToPool(toolHandler->getViewPool());
}

ShapeToolView::~ShapeToolView() noexcept { this->unregisterFromPool(); }

void ShapeToolView::draw(cairo_t* cr) const {
    const auto& pts = this->toolHandler->getShape();

    if (pts.empty()) {
        // The input sequence has been cancelled. This view should soon be deleted
        return;
    }

    xoj::util::CairoSaveGuard saveGuard(cr);  // cairo_save

    // The edges the viewer cannot see are drawn first and dashed, so that the solid outline of the
    // figure ends up on top of them: the preview under the pen then shows the finished figure
    // exactly, dashed edges included.
    if (this->toolHandler->getDrawHiddenEdges()) {
        const auto& hidden = this->toolHandler->getHiddenParts();
        if (!hidden.empty()) {
            cairo_t* effCr = this->prepareContext(cr);
            // Match the pattern to the pen thickness, like the committed stroke does: a plain
            // "dash" pattern on a thick pen closes up and reads as a solid line.
            const LineStyle style = StrokeStyle::scaleStyle(StrokeStyle::parseStyle("dash"), this->strokeWidth);
            Util::cairo_set_dash_from_vector(effCr, style.getDashes(), 0.0);

            for (const auto& part: hidden) {
                if (part.size() < 2) {
                    continue;
                }
                StrokeViewHelper::pathToCairo(effCr, part);
            }
            this->commitDrawing(cr);
        }
    }

    cairo_t* effCr = this->prepareContext(cr);

    StrokeViewHelper::pathToCairo(effCr, pts);

    this->commitDrawing(cr);
}

bool ShapeToolView::isViewOf(const OverlayBase* overlay) const { return overlay == this->toolHandler; }

void ShapeToolView::on(ShapeToolView::FlagDirtyRegionRequest, const Range& rg) {
    maskWipeExtent = maskWipeExtent.unite(rg);
    this->parent->flagDirtyRegion(rg);
}

void ShapeToolView::deleteOn(ShapeToolView::FinalizationRequest, const Range& rg) {
    this->parent->drawAndDeleteToolView(this, rg);
}
