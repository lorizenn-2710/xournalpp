// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string>

#include <cairo.h>

#include "TeachingMath.h"
namespace teaching {
struct FormulaMetrics {
    double width, height;
};
// All glyphs and rules are vector; input is always parsed by the common AST.
FormulaMetrics formulaMetrics(const std::string&, double size = 14);
void paintFormula(cairo_t*, const std::string&, double size = 14);
std::string formulaLatex(const std::string&);
std::string formulaPdf(const std::string&, double size = 14, uint32_t color = 0x202020);
FormulaMetrics formulaMetrics(std::shared_ptr<const MathNotation>, double size = 14);
void paintFormula(cairo_t*, std::shared_ptr<const MathNotation>, double size = 14);
std::string formulaPdf(std::shared_ptr<const MathNotation>, double size = 14, uint32_t color = 0x202020);
std::string notationLatex(std::shared_ptr<const MathNotation>);
}  // namespace teaching
