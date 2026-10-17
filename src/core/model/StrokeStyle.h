/*
 * Xournal++
 *
 * Definition for StrokeStyles
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string>  // for string

#include "LineStyle.h"  // for LineStyle

namespace StrokeStyle {
/** Parse LineStyle from string.
 *
 * @return LineStyle deserialized from string
 */
LineStyle parseStyle(const std::string& style);
/** Convert a LineStyle to a string.
 *
 * @param style to be serialized
 * @return string containing serialized version of style
 */
std::string formatStyle(const LineStyle& style);

/**
 * @brief Scale the dash pattern of a style to the given stroke width.
 *
 * A dash pattern is a list of fixed lengths, so the same pattern looks dense on a thin stroke and
 * nearly solid on a thick one. This returns the style with its dashes scaled for `strokeWidth`,
 * and records the width so that the operation is idempotent.
 *
 * @param style the style to scale (a plain style is returned unchanged)
 * @param strokeWidth the pen thickness the pattern is meant for
 */
LineStyle scaleStyle(const LineStyle& style, double strokeWidth);
}  // namespace StrokeStyle
