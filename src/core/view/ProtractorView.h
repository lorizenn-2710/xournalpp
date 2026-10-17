/*
 * Xournal++
 *
 * A protractor view
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cairo.h>  // for cairo_t

#include "model/Protractor.h"  // for Protractor

#include "GeometryToolView.h"

namespace xoj::view {
class Repaintable;

/**
 * @brief Renders a semicircular protractor of radius `height`
 *
 * The tool is drawn in its own frame: the centre of the straight edge is the
 * origin, the semicircle covers y <= 0 and the scale runs from 0° (right end of
 * the straight edge) to 180° (left end).
 *
 * The outer scale is read clockwise (0 on the right), the inner one
 * counter-clockwise (0 on the left), like on a physical protractor.
 * The marks are placed counter-clockwise from the right end of the diameter.
 */
class ProtractorView: public GeometryToolView {
public:
    ProtractorView(const Protractor* s, Repaintable* parent, ZoomControl* zoomControl);
    ~ProtractorView() noexcept override;

    void on(FlagDirtyRegionRequest, const Range& rg) override;
    void on(UpdateValuesRequest, double h, double rot, cairo_matrix_t m) override;
    void deleteOn(FinalizationRequest, const Range& rg) override;

private:
    void drawGeometryTool(cairo_t* cr) const override;
    void drawDisplays(cairo_t* cr) const override;

    void drawMark(cairo_t* cr, int degree, double lenRatio) const;
    void drawMarks(cairo_t* cr) const;
    void drawCaptions(cairo_t* cr) const;
    void drawRotation(cairo_t* cr) const;

    double height = Protractor::INITIAL_HEIGHT;
    double rotation = 0.;
    cairo_matrix_t matrix{CM, 0., 0., CM, 0., 0.};

    /// Number of degrees between two small marks
    int angularOffset = 1;
    /// Number of degrees between two big marks (with a caption)
    int angularCaptionOffset = 10;

    bool drawRotationDisplay = true;

public:
    static constexpr double LINE_WIDTH_IN_CM = .02;
};
};  // namespace xoj::view
