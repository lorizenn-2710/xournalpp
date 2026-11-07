#include "ProtractorInputHandler.h"

#include <algorithm>  // for clamp
#include <memory>     // for __shared_ptr_access, shar...

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
                const double h = controller->getGeometryTool()->getHeight();
                if (p.y > 0.35 && controller->isInsideGeometryTool(coords.x, coords.y, 0) &&
                    !controller->isInsideGeometryTool(coords.x, coords.y, -0.5)) {
                    // initialize range along curved outer arc
                    lastProj = std::atan2(-p.y, p.x);
                    protractorController->createOutlineStroke(lastProj);
                    return true;
                } else if (controller->isInsideGeometryTool(coords.x, coords.y, 0.) &&
                           std::abs(p.y) <= 0.35 && std::abs(p.x) <= h) {
                    // initialize range along full straight bottom edge [-h, +h]
                    protractorController->createRadialStroke(std::clamp(p.x, -h, h));
                    return true;
                } else if (controller->isInsideGeometryTool(coords.x, coords.y, 0.)) {
                    // Dragging inside the protractor body moves/rotates/scales it directly
                    sequenceStart(event);
                    this->handScrolling = true;
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
                    const double h = controller->getGeometryTool()->getHeight();
                    protractorController->updateRadialStroke(std::clamp(p.x, -h, h));
                    return true;
                } else if (this->handScrolling) {
                    scrollMotion(event);
                    return true;
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
                } else if (this->handScrolling) {
                    this->handScrolling = false;
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
                return true;
            }
        default:
            return false;
    }
}

auto ProtractorInputHandler::getMinHeight() const -> double { return MIN_HEIGHT; }
auto ProtractorInputHandler::getMaxHeight() const -> double { return MAX_HEIGHT; }
