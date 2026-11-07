// SPDX-License-Identifier: GPL-2.0-or-later
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <stdexcept>

#include "math/TeachingMath.h"
using namespace teaching;
int main() {
    int failed = 0, checks = 0;
    auto check = [&](bool b, const char* s) {
        ++checks;
        failed += !b;
        std::printf("%s %s\n", b ? "PASS" : "FAIL", s);
    };
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-8; };
    auto rejects = [&](const std::function<void()>& fn) {
        try {
            fn();
            return false;
        } catch (const std::exception&) {
            return true;
        }
    };
    auto finite = [](const Figure& f) {
        for (auto& s: f.strokes)
            for (auto p: s.points)
                if (!std::isfinite(p.x) || !std::isfinite(p.y) || p.x < 0 || p.x > f.width || p.y < 0 || p.y > f.height)
                    return false;
        for (auto& l: f.labels)
            if (!std::isfinite(l.pos.x) || !std::isfinite(l.pos.y) || !std::isfinite(l.size))
                return false;
        return true;
    };
    auto f = Expression("1/x");
    auto sampled = sample(f, {-2, 2, -6, 6});
    bool split = true;
    for (auto& b: sampled.branches)
        for (size_t i = 1; i < b.size(); ++i) split &= b[i - 1].x * b[i].x > 0;
    check(split && sampled.branches.size() == 2, "1/x: exactly two clipped branches, no edge across zero");
    f = Expression("(x^2-1)/(x-1)");
    auto a = analyze(f);
    check(!std::isfinite(f.at(1).y) && a.points.size() == 1 && a.points[0].hole && near(a.points[0].value, 2) &&
                  a.verticalAsymptotes.empty(),
          "original removable domain hole (1,2), no vertical asymptote");
    auto h = sample(f, {-2, 3, -4, 5});
    split = true;
    for (auto& b: h.branches)
        for (size_t i = 1; i < b.size(); ++i) split &= !(b[i - 1].x <= 1 && b[i].x >= 1);
    check(split && h.branches.size() == 2, "hole remains a break in actual sampled vectors");
    a = analyze(Expression("(x^2+1)/(x-1)"));
    check(a.verticalAsymptotes == std::vector<double>{1} && near(a.slope, 1) && near(a.intercept, 1),
          "quadratic rational: asymptotes x=1, y=x+1");
    a = analyze(Expression("x^3-3*x"));
    check(a.points.size() == 2 && near(a.points[0].x, -1) && near(a.points[0].value, 2) && a.points[0].extremum == 1 &&
                  near(a.points[1].x, 1) && near(a.points[1].value, -2) && a.points[1].extremum == -1 &&
                  a.signs == std::vector<int>({1, -1, 1}),
          "cubic extrema and derivative interval signs");
    a = analyze(Expression("x^3"));
    check(a.points.size() == 1 && a.points[0].x == 0 && a.points[0].extremum == 0 &&
                  a.signs == std::vector<int>({1, 1}),
          "stationary cubic: f'(0)=0 without extremum");
    auto v = Expression("x^2").at(1);
    check(v.y == 1 && v.derivative == 2 && v.y - v.derivative == -1,
          "analytic tangent y=2x-1 independent of page scale");
    check(!std::isfinite(Expression("ln(x)").at(0).y) && !std::isfinite(Expression("ln(x)").at(-1).y),
          "ln domain excludes nonpositive x");
    sampled = sample(Expression("ln(x)"), {-2, 2, -5, 5});
    bool ln = !sampled.branches.empty() && !sampled.limited;
    for (auto& b: sampled.branches)
        for (auto p: b) ln &= p.x > 0;
    check(ln, "sampler skips negative ln domain without exhausting budget");
    sampled = sample(Expression("sqrt(x)"), {-2, 2, -5, 5});
    bool zero = false;
    for (auto& b: sampled.branches)
        for (auto p: b) zero |= p.x == 0 && p.y == 0;
    check(zero, "sqrt includes boundary origin");
    check(std::isinf(Expression("sqrt(x)").at(0).derivative), "sqrt endpoint has vertical tangent");
    check(std::isnan(Expression("abs(x)").at(0).derivative), "abs at zero: no arbitrary derivative");
    check(near(Expression("sin(pi)").at(0).y, 0) && near(Expression("cos(π)").at(0).y, -1),
          "pi and Unicode pi, radians");
    check(Expression("-x^2").at(2).y == -4 && Expression("2^3^2").at(0).y == 512,
          "unary minus and right-associative exponent precedence");
    check(near(Expression("exp(m*x)", 2).at(1).derivative, 2 * std::exp(2.)), "parameter m participates in chain rule");
    for (const auto& s: {"7", "0*x^3+x^2", "(0*x^2+2*x+1)/(0*x+2)", "(x^2+1)/(x-1)", "-x^3", "1e-200*x^2", "1e200*x^2"})
        check(!analyze(Expression(s)).signs.empty(), s);
    check(analyze(Expression("7")).signs == std::vector<int>{0}, "constant derivative identically zero");
    check(analyze(Expression("1/(x-1)")).leftLimits == std::vector<std::string>({"0", "−∞", "0"}),
          "rational one-sided limits");
    check(analyze(Expression("2*x^2")).leftLimits == std::vector<std::string>({"+∞", "0", "+∞"}), "polynomial limits");
    check(rejects([&] { analyze(Expression("1/0")); }), "identically zero denominator is rejected");
    check(rejects([&] { analyze(Expression("sin(x)")); }), "automatic table declines unverified family");
    check(rejects([&] { Expression("os.execute(x)"); }) && rejects([&] { Expression("2x"); }),
          "safe grammar rejects commands and implicit multiplication");
    check(rejects([&] { Expression(std::string(200, '(') + "x" + std::string(200, ')')); }),
          "parser recursion is bounded");
    Request r;
    r.curves[0].formula = "x^2";
    r.tangent = true;
    r.x0 = 1;
    r.window = {-3, 3, -1, 10};
    auto graph = generate(r, 560, 260);
    bool tangent = false;
    for (auto& l: graph.labels) tangent |= l.text.find("y = 2x − 1") != std::string::npos;
    check(tangent && finite(graph), "actual figure tangent with unequal axis scales");
    r.curves[0].formula = "abs(x)";
    r.x0 = 0;
    check(rejects([&] { generate(r); }), "figure refuses tangent at abs corner");
    r.curves[0].formula = "sqrt(x)";
    check(finite(generate(r)), "figure vertical tangent at sqrt endpoint");
    r.curves[0].formula = "1/x";
    check(rejects([&] { generate(r); }), "figure refuses tangent outside domain");
    r = Request{};
    r.piTicks = true;
    r.curves = {{"sin(x)", true, 0x205bce}, {"cos(m*x)", true, 0xc42b35}};
    auto g1 = generate(r);
    r.m = 2;
    auto g2 = generate(r);
    check(finite(g1) && finite(g2) && g1.strokes.size() != 0 && g2.strokes.size() != 0,
          "two parameterized curves share finite frame");
    bool pi = false;
    for (auto& l: g1.labels) pi |= l.text.find("π") != std::string::npos;
    check(pi, "actual figure has pi ticks");
    r.window = {0, 0, -1, 1};
    check(rejects([&] { generate(r); }), "invalid range rejects before document mutation");
    r = Request{};
    r.xtick = 0;
    check(rejects([&] { generate(r); }), "zero tick rejected");
    r = Request{};
    r.xtick = 1e-300;
    check(finite(generate(r)), "tiny tick density is safely bounded");
    r = Request{};
    r.mode = 1;
    check(finite(generate(r)), "automatic table generates editable vector scene");
    r.mode = 2;
    r.manual = parseTable("−∞||0||+∞", "|−|0|+|", "+∞|↘|0|↗|+∞", "f′");
    check(finite(generate(r)), "manual variable-column sign and variation table");
    check(rejects([&] { parseTable("0|1", "+", "1|2", "f"); }), "manual row length mismatch rejected");
    check(std::abs(Expression("x/(1e-200*x+1e-200)").at(1).derivative / 2.5e199 - 1) < 1e-12,
          "quotient derivative avoids denominator square underflow");
    r = Request{};
    r.curves[0].formula = "1/0";
    check(rejects([&] { generate(r); }), "empty formula domain reports an error");
    check(rejects([&] { analyze(Expression("(1/x)^0")); }), "automatic survey never drops a hidden domain exclusion");
    a = analyze(Expression("(x^2-0.01)/(x-0.1)"));
    check(a.verticalAsymptotes.empty() && a.points.size() == 1 && a.points[0].hole && near(a.points[0].value, .2),
          "decimal coefficients: exact removable hole, no fake pole");
    a = analyze(Expression("(x^2-0.010000000000000001)/(x-0.1)"));
    check(a.verticalAsymptotes.size() == 1 && !a.points.back().hole, "nonzero decimal remainder stays a genuine pole");
    std::string large = "2";
    for (int i = 0; i < 30; ++i) large = "(" + large + ")^3";
    check(rejects([&] { analyze(Expression(large)); }),
          "exact algebra rejects exponentially growing coefficients within bounded resources");
    check(rejects([&] { analyze(Expression("1/(1/0)")); }), "nested zero denominator never becomes a defined constant");
    check(rejects([&] { analyze(Expression("x^0")); }), "automatic table preserves original 0^0 exclusion");
    for (const char* formula: {"2^x", "(1/2)^x", "(x+3)^x"}) {
        auto exponential = sample(Expression(formula), {-2, 2, -1, 30});
        check(!exponential.limited && !exponential.branches.empty(), formula);
    }
    check(near(Expression("2^x").at(2).derivative, 4 * std::log(2.)), "variable exponent uses the analytic chain rule");
    check(!Expression("ln(x^3+2)").continuous(-2, 1), "odd power enclosure retains negative values for domain checks");
    check(!Expression("sqrt(x^3+.1)").continuous(-1, 1),
          "sqrt of odd polynomial cannot certify undefined negative region");
    check(Expression("sqrt(x^2)").continuous(-1, 1), "even power still has the correct zero minimum");
    check(!Expression("(x-.12345)^0").continuous(0, 1),
          "zero exponent retains original 0^0 exclusion on whole interval");
    auto zeroPower = sample(Expression("(x-.12345)^0"), {-2, 2, -2, 2});
    bool zeroPowerSplit = true;
    for (auto& branch: zeroPower.branches)
        for (size_t i = 1; i < branch.size(); ++i)
            zeroPowerSplit &= (branch[i - 1].x - .12345) * (branch[i].x - .12345) > 0;
    check(!zeroPower.limited && zeroPowerSplit && zeroPower.branches.size() == 2,
          "sampler makes separate strokes at off-grid 0^0 exclusion");
    for (const char* formula: {"abs(0)", "sqrt(0)", "abs(x^2)", "x*sqrt(0)", "x+abs(0)"}) {
        auto value = Expression(formula).at(0);
        check(std::isfinite(value.derivative) && near(value.derivative, std::string(formula) == "x+abs(0)" ? 1 : 0),
              formula);
    }
    check(std::isnan(Expression("sqrt(x^2)").at(0).derivative), "sqrt(x^2) retains its actual corner at zero");
    check(!Expression("exp(1)").dependsOnX() && Expression("exp(x)").dependsOnX(),
          "numeric fields distinguish exp from the variable x by syntax tree");
    auto unicode = Expression("2×x²−3·x÷2").at(2);
    check(near(unicode.y, 5) && near(unicode.derivative, 6.5),
          "Unicode minus, multiplication, division and superscript preserve mathematics");
    check(Expression("2³²").at(0).y == 4294967296. && Expression("2⁻²").at(0).y == .25,
          "consecutive superscript digits form one exponent, with optional sign");
    auto zeroStart = std::chrono::steady_clock::now();
    auto zeroAnalysis = analyze(Expression("0e1000000000"));
    check(zeroAnalysis.signs == std::vector<int>{0} &&
                  std::chrono::duration<double>(std::chrono::steady_clock::now() - zeroStart).count() < .5,
          "huge exponent on zero never allocates an enormous exact denominator");
    r = Request{};
    r.curves[0].formula = "2^x";
    check(finite(generate(r)), "actual exponential figure generates within sampling budget");
    r = Request{};
    r.mode = 1;
    r.curves[0].formula = "1/x";
    auto centered = generate(r);
    bool poleCentered = false;
    for (auto& label: centered.labels)
        if (label.text == "0 (loại)")
            poleCentered = label.anchor == TextAnchor::Center;
    check(poleCentered, "variation breakpoint label is centered above its exclusion column");
    check(Expression("x^10000000000000000").at(-1).derivative == -1e16,
          "large even exponent retains negative derivative on a negative base");
    r = Request{};
    r.curves[0].formula = "sqrt(0)";
    r.tangent = true;
    r.x0 = 0;
    bool flat = false;
    for (auto& label: generate(r).labels) flat |= label.text == "Tiếp tuyến: y = 0";
    check(flat, "constant tangent equation is displayed without a redundant zero-x term");
    Expression bent("1000000*(x-.0078125)^3");
    auto inflection = sample(bent, {-1, 1, -1, 1}, 460, 300);
    double maxBendError = 0;
    for (auto& branch: inflection.branches)
        for (size_t i = 1; i < branch.size(); ++i)
            for (double t: {.1, .2, .3, .4, .5, .6, .7, .8, .9}) {
                auto p = branch[i - 1], q = branch[i];
                double x = std::lerp(p.x, q.x, t);
                maxBendError = std::max(maxBendError, std::abs(bent.at(x).y - std::lerp(p.y, q.y, t)) * 150);
            }
    check(!inflection.limited && !inflection.branches.empty() && maxBendError <= 1.2,
          "cubic inflection curvature is resolved between sample midpoints");
    auto affineHole = sample(Expression("(x^2-1)/(x-1)"), {-2, 3, -2, 5}, 460, 300);
    check(!affineHole.limited && affineHole.branches.size() == 2 && affineHole.evaluations < 30000,
          "verified affine quotient retains its hole without exhausting the default native viewport budget");
    auto start = std::chrono::steady_clock::now();
    sampled = sample(Expression("sin(1000000*x)/(x-0.123456)"), {-2, 2, -2, 2});
    auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    check(sampled.evaluations <= 30000 && seconds < 2,
          "difficult expression returns within bounded budget and 2 seconds");
    for (double width: {240., 400., 900.}) {
        sampled = sample(Expression("1/(x-.12345)"), {-2, 2, -100, 100}, width, 300);
        bool ok = !sampled.limited;
        for (auto& b: sampled.branches)
            for (size_t i = 1; i < b.size(); ++i) ok &= (b[i - 1].x - .12345) * (b[i].x - .12345) > 0;
        check(ok, "off-grid pole never crossed at multiple frame/zoom sizes");
    }
    sampled = sample(Expression("1/sin(x)"), {-4, 4, -10, 10});
    split = true;
    for (auto& b: sampled.branches)
        for (size_t i = 1; i < b.size(); ++i)
            split &= Expression("sin(x)").continuous(b[i - 1].x, b[i].x) &&
                     ((std::sin(b[i - 1].x) * std::sin(b[i].x)) > 0);
    check(split, "nonlinear denominator interval guard prevents pole bridges");
    std::printf("RESULT %d checks, %d failures\n", checks, failed);
    return failed ? 1 : 0;
}
