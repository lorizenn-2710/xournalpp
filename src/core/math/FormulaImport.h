// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace teaching {
enum class FormulaIssue { None, Empty, TooLong, Unsupported, Ambiguous, Invalid };
struct FormulaCandidate {
    std::string raw, expression, message, source;
    FormulaIssue issue = FormulaIssue::None;
    bool usable() const { return issue == FormulaIssue::None; }
};
// Converts notation only. All mathematical parsing/evaluation remains in Expression.
FormulaCandidate importFormula(const std::string& raw, const std::string& source = "");
std::vector<FormulaCandidate> importFormulaLines(const std::string&, const std::string& source);
}  // namespace teaching
