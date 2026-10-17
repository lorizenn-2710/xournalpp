/*
 * Xournal++
 *
 * A protractor model
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include "model/GeometryTool.h"

/**
 * @brief A class that models a protractor
 *
 * The protractor has the shape of a circle and has a certain height (radius), may be rotated and
 * translated. User coordinates are specified in cm.
 */

namespace xoj::view {
class ProtractorView;
};

class ProtractorInputHandler;

class Protractor: public GeometryTool {
public:
    Protractor();

    /**
     * @brief A protractor specified by its radius, rotation angle and translations in x- and y-directions
     * @param height the radius of the protractor
     * @param rotation the angle (in radian) around which the protractor is rotated with respect to the x-axis
     * @param x the x-coordinate (in pt) of the center of the protractor
     * @param y the y-coordinate (in pt) of the center of the protractor
     */
    Protractor(double height, double rotation, double x, double y);

    virtual ~Protractor();

    void notify(bool resetMask) const override;  // calls the update method of all observers
    Range getToolRange(bool transformed) const override;

    // parameters used when initially displaying the protractor on a page
    static constexpr double INITIAL_HEIGHT = 3.0;
};
