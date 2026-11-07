// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>

#include "control/tools/ElementSnapper.h"
#include "math/CoordinateMath.h"
#include "math/FormulaRender.h"
using namespace teaching::coordinate;
int main() {
    int count = 0, failed = 0;
    auto check = [&](bool ok, const char* what) {
        ++count;
        failed += !ok;
        std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    };
    auto near = [](double a, double b) {
        return std::abs(a - b) <= 1e-12 * std::max({std::abs(a), std::abs(b), 1e-200});
    };
    auto same = [&](Vec a, Vec b) { return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z); };
    auto rejects = [](std::function<void()> f) {
        try {
            f();
            return false;
        } catch (const std::exception&) {
            return true;
        }
    };
    Vec u{1, 2, 0}, v{3, -1, 2};
    auto ab = subtract({4, 0, 5}, {1, 2, 3});
    check(same(ab, {3, -2, 2}) && near(length(ab), std::sqrt(17.)), "AB and sqrt(17)");
    check(same(add(u, v), {4, 1, 2}) && dot(u, v) == 1, "sum and scalar product");
    check(same(subtract(u, v), {-2, 3, -2}) && same(multiply(u, 2), {2, 4, 0}), "difference and scalar multiple");
    check(length({}) == 0 && !angle({}, u) && !angle(u, {}), "zero has zero length and undefined angle");
    check(!parallel({}, u) && !perpendicular({}, u), "zero has no direction predicates");
    check(parallel(u, multiply(u, -1)) && near(*angle(u, multiply(u, -1)), 180), "opposite parallel vectors");
    check(perpendicular({1, 0, 0}, {0, 1, 0}) && near(*angle({1, 0, 0}, {0, 1, 0}), 90), "orthogonal vectors");
    check(parallel({1e-100, 0, 0}, {-1e100, 0, 0}), "normalized directions across 200 orders of magnitude");
    check(near(length({1e100, 1e100, 1e100}), std::sqrt(3.) * 1e100) &&
                  near(length({1e-100, 1e-100, 0}), std::sqrt(2.) * 1e-100),
          "scaled norm large and small");
    check(parallel({1, 0, 0}, {1, 5e-11, 0}) && !parallel({1, 0, 0}, {1, 2e-10, 0}),
          "explicit angular tolerance boundary");
    auto f = combine({{10, 0, 0}, {0, 10, 0}});
    check(same(f.resultant, {10, 10, 0}) && near(f.magnitude, 10 * std::sqrt(2.)) && near(*f.azimuth, 45),
          "two perpendicular 10 N forces");
    f = combine({{10, 0, 0}, {-10, 0, 0}});
    check(f.magnitude == 0 && !f.azimuth && !f.elevation, "opposite forces have no resultant direction");
    check(same(combine({{1, 2, 3}, {3, -1, 2}, {-4, -1, -5}}).resultant, {}), "many concurrent forces");
    f = combine({polar(10, 30), polar(10, 210)});
    check(f.magnitude == 0 && !f.azimuth, "polar opposite forces cancel within floating-point roundoff");
    check(same(f.equilibrant, multiply(f.resultant, -1)), "equilibrant is negative resultant");
    check(same(polar(10, 90), {0, 10, 0}) && same(polar(10, 0, 90), {0, 0, 10}),
          "degree convention and vertical force");
    check(same(components({0, 10, 0}, 90), {10, 0, 0}), "orthonormal analysis axes rotation");
    auto l = line({1, 2, 3}, {0, 1, 0});
    check(contains(l, {1, 99, 3}) && !contains(l, {2, 2, 3}), "membership with zero direction components");
    check(same(project({1, 2, 3}, line({}, {1, 0, 0})), {1, 0, 0}) &&
                  near(length(subtract({1, 2, 3}, project({1, 2, 3}, line({}, {1, 0, 0})))), std::sqrt(13.)),
          "orthogonal foot on Ox and sqrt(13)");
    auto p = compare(line({}, {1, 0, 0}), line({0, 0, 1}, {0, 1, 0}));
    check(p.relation == Relation::Skew && near(p.distance, 1) && same(p.first, {}) && same(p.second, {0, 0, 1}),
          "skew lines and shortest segment");
    p = compare(line({}, {1, 0, 0}), line({1, 2, 0}, {0, 1, 0}));
    check(p.relation == Relation::Intersecting && same(p.first, {1, 0, 0}) && same(p.second, {1, 0, 0}),
          "intersecting lines");
    p = compare(line({}, {1, 0, 0}), line({3, 0, 0}, {-2, 0, 0}));
    check(p.relation == Relation::Coincident && p.distance == 0, "coincident lines");
    p = compare(line({}, {1, 0, 0}), line({0, 2, 0}, {2, 0, 0}));
    check(p.relation == Relation::Parallel && near(p.distance, 2), "parallel distinct lines");
    p = compare(line({}, {1, 0, 0}), line({0, 1, 0}, {1, 1e-8, 0}));
    check(p.relation == Relation::Intersecting && near(p.first.x, -1e8) && near(p.second.x, -1e8),
          "nearly parallel but resolvable intersection");
    check(contains(line({1e-100, 0, 0}, {1e-100, 0, 0}), {2e-100, 0, 0}) &&
                  !contains(line({1e-100, 0, 0}, {1, 0, 0}), {0, 1e-100, 0}),
          "small geometry is not collapsed by absolute tolerance");
    check(rejects([] { through({1, 2, 3}, {1, 2, 3}); }) && rejects([] { line({}, {}); }),
          "coincident input points and zero direction rejected");
    check(rejects([] { checked({std::numeric_limits<double>::infinity(), 0, 0}); }) &&
                  rejects([] { checked({1e101, 0, 0}); }),
          "nonfinite and out-of-range inputs rejected");
    check(same(parseVec("1; -2; sqrt(4)", true), {1, -2, 2}) && same(parseVec("1e-100;2", false), {1e-100, 2, 0}),
          "typed coordinate parser with scalar constants");
    for (auto s: {"1;2;", "1;2;3;4", "x;2;3", "m;2;3", "nan;0;0", "1/0;2;3", "1,2,3"})
        check(rejects([&] { parseVec(s, true); }), s);
    check(rejects([] { polar(-1, 0); }) && rejects([] { polar(1, 0, 91); }) && rejects([] { combine({}); }),
          "invalid force inputs rejected");
    Request r;
    r.fromPoints = true;
    auto scene = generate(r);
    auto changed = r;
    changed.viewDegrees = 20;
    changed.xScale = 3;
    changed.yScale = .2;
    auto other = generate(changed);
    bool formulas = true;
    for (size_t i = 0; i < scene.figure.labels.size(); ++i)
        if (scene.figure.labels[i].notation)
            formulas &= teaching::notationLatex(scene.figure.labels[i].notation) ==
                        teaching::notationLatex(other.figure.labels[i].notation);
    check(formulas && scene.figure.strokes[0].points[1].x != other.figure.strokes[0].points[1].x,
          "projection changes picture without changing mathematical results");
    r.mode = 1;
    r.fromPoints = false;
    r.a = {1, 2, 3};
    r.u = {0, 1, 0};
    r.canonical = true;
    scene = generate(r);
    std::string source;
    for (auto l: scene.figure.labels)
        if (l.notation)
            source += teaching::notationLatex(l.notation);
    check(source.find("x = 1") != std::string::npos && source.find("z = 3") != std::string::npos &&
                  source.find("frac") != std::string::npos && source.find("{0}") == std::string::npos,
          "canonical system uses fixed coordinates for zero components");
    for (int mode = 0; mode < 3; ++mode)
        for (int op = 0; op < 4; ++op) {
            r = Request{};
            r.mode = mode;
            r.operation = op;
            scene = generate(r);
            bool valid = true;
            for (auto s: scene.figure.strokes)
                for (auto p: s.points)
                    valid &= std::isfinite(p.x) && std::isfinite(p.y) && p.x >= 0 && p.x <= 560 && p.y >= 0 &&
                             p.y <= 400;
            for (auto l: scene.figure.labels)
                if (l.notation)
                    valid &= teaching::formulaPdf(l.notation, l.size, l.color).rfind("%PDF", 0) == 0;
            check(valid, "real figure paths bounded and formulas vector PDF");
        }
    r = Request{};
    r.spatial = false;
    r.a = {1, 2, 0};
    r.b = {4, 0, 0};
    r.q = {1, 2, 0};
    r.u = {0, 1, 0};
    r.v = {1, 0, 0};
    r.mode = 1;
    scene = generate(r);
    source.clear();
    for (auto l: scene.figure.labels)
        if (l.notation)
            source += teaching::notationLatex(l.notation);
    check(source.find("x = 1") != std::string::npos && source.find("y = 2 + t") != std::string::npos &&
                  source.find("-1x") != std::string::npos,
          "Oxy line has parameter and general equations from real coordinates");
    r.spatial = true;
    r.a = {1, 2, 3};
    r.u = {0, 1, 0};
    scene = generate(r);
    source.clear();
    for (auto l: scene.figure.labels)
        if (l.notation)
            source += teaching::notationLatex(l.notation);
    check(source.find("y = 2 + t") != std::string::npos && source.find("z = 3") != std::string::npos,
          "Oxyz zero direction components use fixed coordinates and unit coefficient is omitted");
    auto force = combine({{10, 0, 0}, {0, 10, 0}});
    check(same(force.equilibrant, {-10, -10, 0}) && near(length(force.equilibrant), force.magnitude),
          "nonzero equilibrant has equal magnitude and opposite direction");
    check(!contains(line({1e10, 0, 0}, {1, 0, 0}), {1e10, 1, 0}),
          "large translation does not hide a unit perpendicular residual");
    check(rejects([] { dot({1e-200, 0, 0}, {1e-200, 0, 0}); }),
          "underflow of a nonzero product is reported instead of displayed as zero");
    r = Request{};
    scene = generate(r);
    auto hasLabel = [](const teaching::Figure& f, const std::string& name) {
        return std::any_of(f.labels.begin(), f.labels.end(), [&](const auto& l) { return l.text == name; });
    };
    check(scene.anchors.size() == 1 && !hasLabel(scene.figure, "A") && !hasLabel(scene.figure, "B") &&
                  !hasLabel(scene.figure, "Q"),
          "component vector mode hides unrelated points and their anchors");
    check(!hasLabel(scene.figure, "uz"), "zero z component does not draw a named phantom segment");
    r.fromPoints = true;
    scene = generate(r);
    check(hasLabel(scene.figure, "A") && hasLabel(scene.figure, "B") && !hasLabel(scene.figure, "Q"),
          "two-point vector keeps A/B but not the unused projection point Q");
    r = Request{};
    r.mode = 2;
    scene = generate(r);
    check(!hasLabel(scene.figure, "Rz"), "planar resultant in Oxyz does not label a zero Rz segment");
    r.forces = {{0, 0, 10}};
    scene = generate(r);
    check(!hasLabel(scene.figure, "Rx") && !hasLabel(scene.figure, "Ry") && hasLabel(scene.figure, "Rz"),
          "vertical force only draws its nonzero component");
    std::vector<::Point> anchors{{1, 2}, {5, 6}};
    check(ElementSnapper::nearestAnchor({1.5, 2.5}, anchors, 2) == 0 &&
                  !ElementSnapper::nearestAnchor({99, 99}, anchors, 2),
          "same nearest-anchor implementation used by page and coordinate preview");
    std::printf("COORDINATE_RESULT %d checks %d failures\n", count, failed);
    return failed ? 1 : 0;
}
