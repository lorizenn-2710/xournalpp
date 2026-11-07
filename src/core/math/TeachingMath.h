// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace teaching {
struct Point {
    double x, y;
};
struct Value {
    double y, derivative;
};  // derivative may be NaN (corner) or infinity (vertical tangent)
struct MathNotation {
    char op;
    std::string text;
    std::shared_ptr<const MathNotation> a, b;
};
class Expression {
public:
    struct Node;
    explicit Expression(const std::string& formula, double m = 1);
    Value at(double x) const;
    bool dependsOnX() const;
    std::shared_ptr<const MathNotation> notation() const;
    std::string latex() const;
    bool continuous(double a, double b) const;
    std::shared_ptr<const Node> root;
};
struct Window {
    double xmin = -5, xmax = 5, ymin = -4, ymax = 4;
    void validate() const;
};
struct Sample {
    std::vector<std::vector<Point>> branches;
    unsigned evaluations = 0, defined = 0;
    bool limited = false;
};
Sample sample(const Expression&, const Window&, double width = 480, double height = 300);
struct Breakpoint {
    double x;
    bool excluded = false, hole = false;
    double value = 0;
    int extremum = 0;
};
struct Analysis {
    std::vector<Breakpoint> points;
    std::vector<int> signs;
    std::vector<std::string> leftLimits, rightLimits;
    std::vector<double> verticalAsymptotes;
    bool hasAsymptote = false;
    double slope = 0, intercept = 0;
};
Analysis analyze(const Expression&);  // polynomial <= 3 or polynomial <= 2 / linear, otherwise throws
std::string number(double);
struct Table {
    std::vector<std::string> x, sign, value;
    std::string name = "f′";
};
Table variationTable(const Analysis&);
Table parseTable(const std::string& x, const std::string& signs, const std::string& values, const std::string& name);
struct Stroke {
    std::vector<Point> points;
    uint32_t color = 0x202020;
    double width = 1;
    bool dashed = false;
};
enum class TextAnchor { Left, Center, Right };
struct Label {
    Point pos;
    std::string text;
    double size = 10;
    uint32_t color = 0x202020;
    double maxWidth = 0;
    TextAnchor anchor = TextAnchor::Left;
    std::string formula;  // optional AST-rendered vector formula, same meaning as graph
    std::shared_ptr<const MathNotation> notation; // typed coordinate formula; never sent to scalar parser
};
struct Figure {
    std::vector<Stroke> strokes;
    std::vector<Label> labels;
    double width = 560, height = 400;
};
struct Curve {
    std::string formula = "x^3-3*x";
    bool enabled = true;
    uint32_t color = 0x205bce;
};
struct Request {
    Window window;
    std::vector<Curve> curves = {{"x^3-3*x", true, 0x205bce}, {"m*x", false, 0xc42b35}};
    double m = 1, x0 = 1, xtick = 1, ytick = 1;
    bool pretty = false, formulaOnly = false;
    bool piTicks = false, equalScale = false, tangent = false, showSign = true, showValues = true;
    std::string xlabel = "x", ylabel = "y";
    int mode = 0;  // 0 graph, 1 automatic table, 2 manual table
    Table manual;
};
Figure generate(const Request&, double width = 560, double height = 400);
}  // namespace teaching
