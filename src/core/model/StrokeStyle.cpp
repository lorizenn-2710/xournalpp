#include "StrokeStyle.h"

#include <cstring>   // for memcmp, strcmp, strncmp
#include <iomanip>   // for setprecision
#include <iterator>  // for ostream_iterator
#include <map>       // for map
#include <sstream>   // for istringstream
#include <vector>    // for vector

#include "model/LineStyle.h"  // for LineStyle
#include "util/serdesstream.h"

namespace {

constexpr auto CUSTOM_KEY = "cust: ";

/**
 * @brief Base lengths of the dash patterns, in points.
 *
 * These are the lengths for a pen of width 1.0. The actual pattern is scaled by the pen
 * thickness (see getScaledDashes), because a pattern that reads well on a thin stroke closes up
 * into a solid-looking line on a thick one.
 */
const std::map<std::string, std::vector<double>> predefinedPatterns = {
        {"dash", {6, 3}}, {"dashdot", {6, 3, 0.5, 3}}, {"dot", {0.5, 3}}};

/**
 * @brief Scale a dash pattern by the stroke width.
 *
 * The scale is proportional but damped: the pattern grows with the thickness, yet slower than the
 * thickness itself. With a purely proportional scale, a thick stroke would end up with dashes
 * longer than most segments a teacher draws, which looks worse than the original problem.
 * The pattern is kept recognisable instead: about 1.5x at "very thick" (width ~5.7) for the
 * medium pattern.
 */
auto scaleDashes(const std::vector<double>& base, double width) -> std::vector<double> {
    constexpr double BASE_WIDTH = 1.0;
    constexpr double FULL_SCALE_AT = 5.0;
    constexpr double MAX_SCALE = 1.6;

    if (width <= BASE_WIDTH || base.empty()) {
        return base;
    }
    const double t = std::min(1.0, (width - BASE_WIDTH) / (FULL_SCALE_AT - BASE_WIDTH));
    const double scale = 1.0 + t * (MAX_SCALE - 1.0);

    std::vector<double> out;
    out.reserve(base.size());
    for (double d: base) {
        out.push_back(d * scale);
    }
    return out;
}

auto formatStyle(const std::vector<double>& dashes) -> std::string {

    // Check if dashes match named predefined dashes.
    for (auto& pair: predefinedPatterns) {
        if (pair.second == dashes) {
            return pair.first;
        }
    }

    // Else generate custom dashes string
    auto custom = serdes_stream<std::ostringstream>();
    custom << std::setprecision(2) << std::fixed;
    custom << CUSTOM_KEY;
    std::copy(dashes.begin(), dashes.end(), std::ostream_iterator<double>(custom, " "));

    // Return dashes string with traling space removed.
    return custom.str().substr(0, custom.str().length() - 1);
}

}  // namespace

auto StrokeStyle::parseStyle(const std::string& style) -> LineStyle {
    auto it = predefinedPatterns.find(style);
    if (it != predefinedPatterns.end()) {
        LineStyle ls;
        std::vector<double> dashes = it->second;
        ls.setDashes(std::move(dashes));
        return ls;
    }

    if (style.substr(0, strlen(CUSTOM_KEY)) != CUSTOM_KEY) {
        return LineStyle();
    }

    auto dashStream = serdes_stream<std::stringstream>(style);
    std::vector<double> dashes;

    dashStream.seekg(strlen(CUSTOM_KEY));
    for (double value; dashStream >> value;) {
        dashes.push_back(value);
    }

    if (dashes.empty()) {
        return LineStyle();
    }

    LineStyle ls;
    ls.setDashes(std::move(dashes));
    return ls;
}

auto StrokeStyle::scaleStyle(const LineStyle& style, double strokeWidth) -> LineStyle {
    const auto& dashes = style.getDashes();
    if (dashes.empty()) {
        return style;
    }
    // Already scaled for this width: nothing to do (the pen did not change)
    if (std::abs(style.getReferenceWidth() - strokeWidth) < 1e-9) {
        return style;
    }

    LineStyle scaled;
    scaled.setDashes(scaleDashes(dashes, strokeWidth));
    scaled.setReferenceWidth(strokeWidth);
    return scaled;
}

auto StrokeStyle::formatStyle(const LineStyle& style) -> std::string {
    const auto& dashes = style.getDashes();
    if (!dashes.empty()) {
        // The dashes are scaled by the pen width, so the result is a custom pattern rather than
        // one of the named ones. Report the base pattern when the scale happens to be 1, so that
        // the toolbar button for the plain named styles still lights up.
        return ::formatStyle(dashes);
    }

    // Should not be returned, in this case the attribute is not written
    return "plain";
}
