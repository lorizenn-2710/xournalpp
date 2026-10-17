#include "Protractor.h"

#include <cmath>             // for sin, cos
#include <initializer_list>  // for initializer_list

#include "gui/inputdevices/ProtractorInputHandler.h"
#include "view/ProtractorView.h"

Protractor::Protractor(): Protractor(INITIAL_HEIGHT, .0, 0., 0.) {}

Protractor::Protractor(double height, double rotation, double x, double y): GeometryTool(height, rotation, x, y) {
    this->lastRepaintRange = getToolRange(true);
}

Protractor::~Protractor() { viewPool->dispatchAndClear(xoj::view::ProtractorView::FINALIZATION_REQUEST, lastRepaintRange); }

auto Protractor::getToolRange(bool transformed) const -> Range {
    const auto h = height * CM;
    Range rg;
    if (transformed) {
        rg.addPoint(origin.x - h, origin.y - h);
        rg.addPoint(origin.x + h, origin.y + h);
    } else {
        rg.addPoint(-h, -h);
        rg.addPoint(h, h);
    }

    // Padding required to fully render the boundary red lines and last blue digit
    constexpr double RENDER_PADDING = 2.0;

    rg.addPadding(RENDER_PADDING + .5 * xoj::view::ProtractorView::LINE_WIDTH_IN_CM * CM);  // account for line width
    return rg;
}

void Protractor::notify(bool resetMask) const {
    if (resetMask) {
        viewPool->dispatch(xoj::view::GeometryToolView::RESET_MASK);
    }

    viewPool->dispatch(xoj::view::ProtractorView::UPDATE_VALUES, this->getHeight(), this->getRotation(),
                       this->getMatrix());
    Range rg = this->getToolRange(true);
    viewPool->dispatch(xoj::view::ProtractorView::FLAG_DIRTY_REGION, this->computeRepaintRange(rg));
}
