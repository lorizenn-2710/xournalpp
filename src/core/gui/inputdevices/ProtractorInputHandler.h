/*
 * Xournal++
 *
 * Input handler for the protractor.
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include "GeometryToolInputHandler.h"

class Stroke;
class GeometryToolController;
struct InputEvent;

/**
 * @brief Input handler for the protractor
 *
 * Only the pointer handling (mouse / stylus) is defined in this class.
 * The rest comes from the GeometryToolHandler
 * */
class ProtractorInputHandler: public GeometryToolInputHandler {

public:
    explicit ProtractorInputHandler(XournalView* xournalView, GeometryToolController* controller);
    ~ProtractorInputHandler() noexcept override;

private:
    /**
     * @brief handles input from mouse and stylus for the protractor
     */
    bool handlePointer(InputEvent const& event) override;

    double lastProj = NAN;

    double getMinHeight() const override;
    double getMaxHeight() const override;
};
