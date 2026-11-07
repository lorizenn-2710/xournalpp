// SPDX-License-Identifier: GPL-2.0-or-later
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

#include "math/FormulaImport.h"
#include "math/FormulaRender.h"
#include "math/TeachingMath.h"
#include "model/TexImage.h"
using namespace teaching;
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--import") {
        auto c = importFormula(argv[2]);
        if (!c.usable()) {
            std::cerr << c.message << '\n';
            return 2;
        }
        std::cout << c.expression << '\n';
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "--equivalent") {
        auto c = importFormula(argv[2]);
        if (!c.usable())
            return 2;
        Expression a(c.expression), b(argv[3]);
        // Parentheses/TeX grouping are gone at this point. Compare the real AST
        // (including denominator/root/power scope), then check excluded points.
        std::function<bool(std::shared_ptr<const MathNotation>, std::shared_ptr<const MathNotation>)> same;
        same = [&](auto u, auto v) -> bool {
            if (!u || !v)
                return !u && !v;
            if (u->op != v->op)
                return false;
            if (u->text != v->text) {
                if (u->op != 'n')
                    return false;
                try {
                    if (std::stod(u->text) != std::stod(v->text))
                        return false;
                } catch (...) {
                    return false;
                }
            }
            return same(u->a, v->a) && same(u->b, v->b);
        };
        if (!same(a.notation(), b.notation()))
            return 4;
        for (double x: {-3., -2., -1., -.1, 0., .1, 1., 2., 3.}) {
            auto u = a.at(x), v = b.at(x);
            if (std::isfinite(u.y) != std::isfinite(v.y) ||
                (std::isfinite(u.y) && std::abs(u.y - v.y) > 1e-8 * (1 + std::abs(v.y))))
                return 3;
        }
        return 0;
    }
    int count = 0;
    auto check = [&](bool ok, const char* label) {
        ++count;
        if (!ok)
            throw std::runtime_error(label);
        std::cout << "FORMULA_PASS " << label << '\n';
    };
    try {
        for (auto pair: {std::pair{"y=x²−3x+2", "x^2-3*x+2"},
                         {"f(x)=x³−3x", "x^3-3*x"},
                         {"y=\\frac{1}{x+1}", "1/(x+1)"},
                         {"\\frac{x^2-1}{x-1}", "(x^2-1)/(x-1)"},
                         {"\\sqrt{x+1}", "sqrt(x+1)"},
                         {"\\sqrt{x}+1", "sqrt(x)+1"},
                         {"−x²", "-x^2"},
                         {"(-x)^2", "(-x)^2"},
                         {"x⁻²", "x^(-2)"},
                         {"sin(x)+cos(π x)", "sin(x)+cos(pi*x)"},
                         {"y=\\sin(x)+\\cos(\\pi x)", "sin(x)+cos(pi*x)"},
                         {"2^x", "2^x"},
                         {"ln x", "ln(x)"},
                         {"|x|", "abs(x)"},
                         {"2x+m x", "2*x+m*x"},
                         {"\\frac{1}{1+\\frac{1}{x}}", "1/(1+1/x)"}}) {
            auto c = importFormula(pair.first);
            check(c.usable(), pair.first);
            Expression a(c.expression), b(pair.second);
            for (double x: {-2., -1., 0., 1., 2.}) {
                auto u = a.at(x), v = b.at(x);
                check(std::isfinite(u.y) == std::isfinite(v.y) && (!std::isfinite(u.y) || std::abs(u.y - v.y) < 1e-10),
                      "normalized AST preserves values and original domain");
            }
            auto latex = a.latex();
            auto back = importFormula(latex);
            check(back.usable(), "generated safe TeX roundtrips through notation reader");
        }
        for (const char* s: {"x²+y²=1", "I/x", "1/l", "O*x", "t^2", "sin x^2", "\\input{/etc/passwd}",
                             "\\write18{touch BAD}", "\\unknown{x}", "\\frac{1}{x", "|x", "x_1", "y=", "y=x; y=x^2",
                             "x=2", "1/2x", "1/x(x+1)", "√x+1", "x2-3x+2"})
            check(!importFormula(s).usable(), s);
        check(!importFormula(std::string(3000, 'x')).usable(), "oversize notation rejected");
        auto multiple = importFormulaLines("y=x²\ny=1/x\nx²+y²=1", "PDF");
        check(multiple.size() == 3 && multiple[0].usable() && multiple[1].usable() && !multiple[2].usable(),
              "multiple candidates never concatenated");
        auto hole = Expression(importFormula("\\frac{x²-1}{x-1}").expression);
        auto analysis = analyze(hole);
        check(analysis.points.size() == 1 && analysis.points[0].hole && !std::isfinite(hole.at(1).y),
              "hole at (1,2) retained after normalization");
        auto a = formulaMetrics("y = 1/(x+1)"), b = formulaMetrics("y = 1/x+1");
        check(a.height > 16 && b.height > 16 && a.width != b.width, "real metrics for fraction scope");
        auto start = std::chrono::steady_clock::now();
        for (const char* s: {"y = 1/(x+1)", "y = sqrt(x+1)", "y = -x^2", "y = (-x)^2", "y = 1/(1+1/x)", "f′ = 2*x-1"}) {
            auto data = formulaPdf(s);
            check(data.substr(0, 4) == "%PDF", "formula exports real vector PDF");
            TexImage t;
            check(t.loadData(std::move(data)), "native TexImage loads formula PDF");
            t.setText("x^2");
            auto clone = t.cloneTexImage();
            check(clone->getText() == "x^2" && clone->getBinaryData() == t.getBinaryData(),
                  "clone preserves source and embedded font PDF");
            auto m = t.getNativeSize();
            check(m.width > 0 && m.height > 0, "native bounds nonempty");
        }
        auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::cout << "RENDER_6_MS " << ms << '\n';
        check(ms < 2000, "bounded native rendering");
        Request only;
        only.formulaOnly = true;
        only.curves = {{"1/(x+1)", true, 0x205bce}};
        auto fig = generate(only);
        check(fig.strokes.empty() && fig.labels.size() == 1 && !fig.labels[0].formula.empty(),
              "standalone formula uses vector AST label without graph strokes");
        try {
            formulaPdf("invalid(x)");
            check(false, "invalid renderer input must throw");
        } catch (const std::exception&) {}
        check(formulaPdf("y = sqrt(x+1)").substr(0, 4) == "%PDF", "renderer cache recovers after error");
        std::cout << "FORMULA_RESULT PASS " << count << " checks\n";
    } catch (const std::exception& e) {
        std::cerr << "FORMULA_FAIL " << e.what() << '\n';
        return 1;
    }
}
