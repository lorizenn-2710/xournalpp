#include "ProtractorInputHandler.h"

#include <memory>  // for __shared_ptr_access, shar...

#include "control/ProtractorController.h"  // for ProtractorController, HYPOTENUSE
#include "control/Control.h"
#include "control/ToolEnums.h"    // for TOOL_HAND, TOOL_HIGHLIGHTER
#include "control/ToolHandler.h"  // for ToolHandler
#include "gui/XournalView.h"      // for XournalView
#include "gui/inputdevices/InputEvents.h"
#include "model/Protractor.h"

constexpr double MIN_HEIGHT = 0.5;
constexpr double MAX_HEIGHT = 10.0;

ProtractorInputHandler::ProtractorInputHandler(XournalView* xournal, GeometryToolController* controller):
        GeometryToolInputHandler(xournal, controller) {}

ProtractorInputHandler::~ProtractorInputHandler() noexcept = default;

auto ProtractorInputHandler::handlePointer(InputEvent const& event) -> bool {
    const auto coords = getCoords(event);
    ProtractorController* protractorController = static_cast<ProtractorController*>(controller);
    const auto p = protractorController->posRelToSide(coords.x, coords.y);

    const auto toolHandler = xournal->getControl()->getToolHandler();
    switch (toolHandler->getToolType()) {
        case TOOL_HIGHLIGHTER:
        case TOOL_PEN:
            if (event.type == BUTTON_PRESS_EVENT) {
                if (controller->isInsideGeometryTool(coords.x, coords.y, 0) &&
                    !controller->isInsideGeometryTool(coords.x, coords.y, -0.5)) {
                    // initialize range
                    lastProj = std::atan2(-p.y, p.x);
                    protractorController->createOutlineStroke(lastProj);
                    return true;
                } else if (double h = controller->getGeometryTool()->getHeight();
                           controller->isInsideGeometryTool(coords.x, coords.y, 0.) &&
                           std::abs(protractorController->posRelToSide(coords.x, coords.y).y) <= 0.5 &&
                           std::abs(protractorController->posRelToSide(coords.x, coords.y).x - 0.5 * h) <= 0.5 * h) {
                    protractorController->createRadialStroke(std::hypot(p.x, p.y));
                    return true;
                }
                return false;
            } else if (event.type == MOTION_EVENT) {
                // update range and paint
                if (protractorController->existsOutlineStroke()) {
                    auto proj = std::atan2(-p.y, p.x);
                    proj = lastProj + std::remainder(proj - lastProj, 2 * M_PI);
                    protractorController->updateOutlineStroke(proj);
                    lastProj = proj;
                    return true;
                } else if (protractorController->existsRadialStroke()) {
                    protractorController->updateRadialStroke(std::hypot(p.x, p.y));
                }
                return false;
            } else if (event.type == BUTTON_RELEASE_EVENT) {
                // add stroke to layer and reset
                if (protractorController->existsOutlineStroke()) {
                    protractorController->finalizeOutlineStroke();
                    lastProj = NAN;
                    return true;
                } else if (protractorController->existsRadialStroke()) {
                    protractorController->finalizeRadialStroke();
                    return true;
                }
            }
            return false;
        case TOOL_HAND:
            if (event.type == BUTTON_PRESS_EVENT) {
                if (!controller->isInsideGeometryTool(coords.x, coords.y, 0)) {
                    return false;
                } else {
                    sequenceStart(event);
                    this->handScrolling = true;
                    return true;
                }
            } else if (event.type == MOTION_EVENT && this->handScrolling) {
                scrollMotion(event);
                return true;
            } else if (event.type == BUTTON_RELEASE_EVENT && this->handScrolling) {
                this->handScrolling = false;
            }
        default:
            return false;
    }
}

auto ProtractorInputHandler::getMinHeight() const -> double { return MIN_HEIGHT; }
auto ProtractorInputHandler::getMaxHeight() const -> double { return MAX_HEIGHT; }
