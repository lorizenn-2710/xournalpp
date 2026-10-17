#include "ProtractorView.h"

#include <algorithm>  // for max, min
#include <cmath>      // for cos, sin, M_PI, isnan
#include <iomanip>
#include <sstream>
#include <utility>  // for move

#include "model/Protractor.h"  // for Protractor
#include "util/Rectangle.h"    // for Rectangle
#include "view/Repaintable.h"

#include "config-debug.h"  // for debug

using xoj::util::Rectangle;

namespace xoj::view {
namespace {
constexpr double RADIUS_TO_FONT_SIZE_RATIO = .075;
constexpr double FONT_TO_LINE = .5;
constexpr double MINIMUM_FONT_SIZE = 2.5;  // In pixels, for legibility
constexpr double AXIS_GUIDE_RATIO = .22;
constexpr double AXIS_GUIDE_THICKNESS = .8;

constexpr double OUTER_MARKS_RATIO = 1.;
constexpr double INNER_MARKS_RATIO = .985;

constexpr double SMALL_MARK_LENGTH_RATIO = .02;
constexpr double MEDIUM_MARK_LENGTH_RATIO = .035;
constexpr double BIG_MARK_LENGTH_RATIO = .075;

constexpr double OUTER_CAPTION_RADIUS_RATIO = .80;
constexpr double INNER_CAPTION_RADIUS_RATIO = .62;

constexpr double CAPTION_TO_LINE_RATIO = .50;
constexpr double INNER_SCALE_CIRCLE_RATIO = .42;
constexpr double CENTER_HOLE_RATIO = .04;

constexpr const char* ROTATION_DISPLAY_SEPARATOR = "  ";

inline double radians(int degrees) { return static_cast<double>(degrees) * M_PI / 180.; }
}  // namespace

ProtractorView::ProtractorView(const Protractor* protractor, Repaintable* parent, ZoomControl* zoomControl):
        GeometryToolView(protractor, parent, zoomControl) {
    this->registerToPool(protractor->getViewPool());
}

ProtractorView::~ProtractorView() noexcept { this->unregisterFromPool(); }

void ProtractorView::on(FlagDirtyRegionRequest, const Range& rg) { this->parent->flagDirtyRegion(rg); }

void ProtractorView::on(UpdateValuesRequest, double h, double rot, cairo_matrix_t m) {
    xoj_assert_message(h > 0, "Non-positive protractor height");
    this->height = h;
    this->rotation = rot;
    this->matrix = m;

    this->drawRotationDisplay = h >= 2.;
    this->angularOffset = (h >= 2.) ? 1 : (h >= 1.2) ? 2 : (h >= .8) ? 5 : 10;
    this->angularCaptionOffset = (h >= 3.) ? 10 : (h >= 2.) ? 15 : (h >= 1.2) ? 30 : 45;
}

void ProtractorView::deleteOn(FinalizationRequest, const Range& rg) {
    this->parent->drawAndDeleteToolView(this, rg);
}

void ProtractorView::drawGeometryTool(cairo_t* cr) const {
    xoj::util::CairoSaveGuard saveGuard(cr);
    cairo_scale(cr, CM, CM);

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    const double R = this->height;

    // Body of the protractor: semicircle + straight edge
    cairo_move_to(cr, -R, 0.);
    cairo_arc(cr, 0., 0., R, M_PI, 2 * M_PI);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, .94, .94, .99, 1.);  // Slightly blueish background
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0., 0., 0.);
    cairo_set_line_width(cr, this->LINE_WIDTH_IN_CM);
    cairo_stroke(cr);

    // Inner "window": makes the tool look like a physical protractor and keeps
    // the captions legible at small sizes.
    cairo_arc(cr, 0., 0., R * INNER_SCALE_CIRCLE_RATIO, M_PI, 2 * M_PI);
    cairo_set_line_width(cr, this->LINE_WIDTH_IN_CM * .6);
    cairo_stroke(cr);

    // Center hole
    cairo_arc(cr, 0., 0., R * CENTER_HOLE_RATIO, 0., 2 * M_PI);
    cairo_set_line_width(cr, this->LINE_WIDTH_IN_CM * .8);
    cairo_stroke(cr);

    this->drawMarks(cr);
    this->drawCaptions(cr);
}

void ProtractorView::drawMark(cairo_t* cr, int degree, double lenRatio) const {
    // Marks are placed counter-clockwise, starting at the right end of the
    // straight edge (0°) and ending at its left end (180°).
    const double angle = 2 * M_PI - radians(degree);
    const double R = this->height;

    const double rIn = R - R * lenRatio;
    cairo_move_to(cr, rIn * cos(angle), rIn * sin(angle));
    cairo_line_to(cr, R * OUTER_MARKS_RATIO * cos(angle), R * OUTER_MARKS_RATIO * sin(angle));
}

void ProtractorView::drawMarks(cairo_t* cr) const {
    cairo_set_source_rgba(cr, 0., 0., 0., 1.);

    // Big marks every angularCaptionOffset degrees, with a caption
    cairo_set_line_width(cr, this->LINE_WIDTH_IN_CM * .8);
    for (int i = 0; i <= 180; i += this->angularCaptionOffset) {
        this->drawMark(cr, i, BIG_MARK_LENGTH_RATIO);
    }
    cairo_stroke(cr);

    // Medium marks every 5 and small marks every other degree
    const int medium = std::max(this->angularCaptionOffset / 2, 1);
    for (int i = 0; i <= 180; i += this->angularOffset) {
        if (i % this->angularCaptionOffset == 0) {
            continue;  // Already drawn as a big mark
        }
        this->drawMark(cr, i, (i % medium == 0) ? MEDIUM_MARK_LENGTH_RATIO : SMALL_MARK_LENGTH_RATIO);
    }
    cairo_set_line_width(cr, this->LINE_WIDTH_IN_CM * .45);
    cairo_stroke(cr);
}

void ProtractorView::drawCaptions(cairo_t* cr) const {
    const double R = this->height;
    const double fontSize = std::max(R * RADIUS_TO_FONT_SIZE_RATIO, MINIMUM_FONT_SIZE / CM);
    cairo_set_font_size(cr, fontSize);

    cairo_set_source_rgba(cr, 0., 0., 0., 1.);
    for (int i = 0; i <= 180; i += this->angularCaptionOffset) {
        const double angle = 2 * M_PI - radians(i);

        // Outer scale: 0° on the right, read counter-clockwise
        {
            const double r = R * OUTER_CAPTION_RADIUS_RATIO;
            cairo_save(cr);
            cairo_move_to(cr, r * cos(angle), r * sin(angle));
            this->showTextCenteredAndRotated(cr, std::to_string(i), i - 90);
            cairo_fill(cr);
            cairo_restore(cr);
        }

        // Inner scale: 0° on the left, read clockwise
        {
            const double r = R * INNER_CAPTION_RADIUS_RATIO;
            cairo_save(cr);
            cairo_move_to(cr, r * cos(angle), r * sin(angle));
            this->showTextCenteredAndRotated(cr, std::to_string(180 - i), 90 - i);
            cairo_fill(cr);
            cairo_restore(cr);
        }
    }
}

void ProtractorView::drawDisplays(cairo_t* cr) const {
    this->drawRotation(cr);
}

void ProtractorView::drawRotation(cairo_t* cr) const {
    if (!this->drawRotationDisplay || std::isnan(this->rotation)) {
        return;
    }
    xoj::util::CairoSaveGuard saveGuard(cr);
    cairo_transform(cr, &this->matrix);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << std::fmod(this->rotation, 180.);
    oss << ROTATION_DISPLAY_SEPARATOR << "°";

    const double fontSize = std::max(this->height * RADIUS_TO_FONT_SIZE_RATIO * 1.6, MINIMUM_FONT_SIZE / CM);
    cairo_set_font_size(cr, fontSize);

    const std::string text = oss.str();

    cairo_text_extents_t te;
    cairo_text_extents(cr, text.c_str(), &te);

    const double pad = fontSize * FONT_TO_LINE;
    const double w = te.width + 2 * pad;
    const double h = te.height + 2 * pad;
    const double x = 0.;                      // Centered horizontally
    const double y = this->height * .30;      // Below the straight edge

    cairo_set_source_rgb(cr, .3, .7, .7);
    cairo_rectangle(cr, x - w / 2, y - h / 2, w, h);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0., 0., 0.);
    cairo_move_to(cr, x - te.width / 2 - te.x_bearing, y - te.height / 2 - te.y_bearing);
    cairo_show_text(cr, text.c_str());
}
};  // namespace xoj::view
