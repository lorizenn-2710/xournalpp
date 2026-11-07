// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>

#include "TeachingMath.h"

namespace teaching::coordinate {
// Coordinates are independent of page units and projection. Finite |component| <= 1e100.
struct Vec {
    double x = 0, y = 0, z = 0;
};
constexpr double angularTolerance = 1e-10;   // sine/cosine of normalized directions
constexpr double relativeTolerance = 1e-10;  // residual / magnitude of input geometry
Vec checked(Vec);
Vec add(Vec, Vec);
Vec subtract(Vec, Vec);
Vec multiply(Vec, double);
double length(Vec);
double dot(Vec, Vec);
Vec cross(Vec, Vec);
std::optional<double> angle(Vec, Vec);           // degrees; null for either exact zero
bool parallel(Vec, Vec);                         // zero has no direction, returns false
bool perpendicular(Vec, Vec);                    // zero has no direction, returns false
Vec parseVec(const std::string&, bool spatial);  // explicit ; separated scalar constants
std::string tuple(Vec, bool spatial);
struct Line {
    Vec point, direction;
};
Line line(Vec point, Vec direction);
Line through(Vec a, Vec b);
Vec project(Vec, const Line&);
bool contains(const Line&, Vec);
enum class Relation { Intersecting, Parallel, Coincident, Skew };
struct LinePair {
    Relation relation;
    double distance;
    Vec first, second;
};
LinePair compare(const Line&, const Line&);
Vec polar(double magnitude, double azimuthDegrees, double elevationDegrees = 0);
struct Forces {
    Vec resultant, equilibrant;
    double magnitude;
    std::optional<double> azimuth, elevation;
};
Forces combine(const std::vector<Vec>&);
// Rotation of the orthonormal analysis axes around Oz (degrees).
Vec components(Vec, double axisDegrees);

struct Request {
    int mode = 0;  // vector, line, concurrent forces
    bool canonical = false;
    bool spatial = true, fromPoints = false, triangle = true, showComponents = true;
    int operation = 0;  // sum, difference, scalar multiple, opposite
    Vec a{1, 2, 3}, b{4, 0, 5}, u{1, 2, 0}, v{3, -1, 2}, q{1, 2, 3};
    double scalar = 2, axisDegrees = 0;
    std::vector<Vec> forces{{10, 0, 0}, {0, 10, 0}};
    std::string unit, name = "u";
    // Only the drawing uses these values.
    double viewDegrees = 45, xScale = 1, yScale = 1;
};
struct Scene {
    Figure figure;
    // Exact, declared input points that can be selected in the preview.
    std::vector<std::pair<Point, Vec>> anchors;
    std::string report;
};
Scene generate(const Request&, double width = 560, double height = 400);
}  // namespace teaching::coordinate
