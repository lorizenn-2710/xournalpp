// SPDX-License-Identifier: GPL-2.0-or-later
#include "CoordinateMath.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace teaching::coordinate {
namespace {
constexpr double pi = 3.14159265358979323846;
void finite(double x) {
    if (!std::isfinite(x) || std::abs(x) > 1e100)
        throw std::runtime_error("VEC-001: Số phải hữu hạn, |giá trị| ≤ 1e100.");
}
Vec unit(Vec v) {
    double n = length(v);
    if (n == 0)
        throw std::runtime_error("VEC-002: Vectơ chỉ phương bằng 0 / hai điểm trùng nhau.");
    return {v.x / n, v.y / n, v.z / n};
}
std::string num(double x) {
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << std::setprecision(7) << (x == 0 ? 0 : x);
    return s.str();
}
double residualTolerance(Vec a, Vec b) {
    return relativeTolerance * length(subtract(a, b)) +
           32 * std::numeric_limits<double>::epsilon() * std::max(length(a), length(b));
}
}  // namespace
Vec checked(Vec v) {
    finite(v.x);
    finite(v.y);
    finite(v.z);
    return v;
}
Vec add(Vec a, Vec b) { return checked({a.x + b.x, a.y + b.y, a.z + b.z}); }
Vec subtract(Vec a, Vec b) { return checked({a.x - b.x, a.y - b.y, a.z - b.z}); }
Vec multiply(Vec a, double k) {
    checked(a);
    finite(k);
    return checked({a.x * k, a.y * k, a.z * k});
}
double length(Vec a) {
    checked(a);
    return std::hypot(a.x, a.y, a.z);
}
double dot(Vec a, Vec b) {
    checked(a);
    checked(b);
    long double value = (long double)a.x * b.x + (long double)a.y * b.y + (long double)a.z * b.z;
    double result = double(value);
    if (value != 0 && result == 0)
        throw std::runtime_error("VEC-012: Tích quá nhỏ để biểu diễn bằng số thực; đổi đơn vị tọa độ.");
    return result;
}
Vec cross(Vec a, Vec b) {
    return checked({double((long double)a.y * b.z - (long double)a.z * b.y),
                    double((long double)a.z * b.x - (long double)a.x * b.z),
                    double((long double)a.x * b.y - (long double)a.y * b.x)});
}
std::optional<double> angle(Vec a, Vec b) {
    if (length(a) == 0 || length(b) == 0)
        return {};
    a = unit(a);
    b = unit(b);
    return std::atan2(length(cross(a, b)), std::clamp(dot(a, b), -1., 1.)) * 180 / pi;
}
bool parallel(Vec a, Vec b) {
    return length(a) > 0 && length(b) > 0 && length(cross(unit(a), unit(b))) <= angularTolerance;
}
bool perpendicular(Vec a, Vec b) {
    return length(a) > 0 && length(b) > 0 && std::abs(dot(unit(a), unit(b))) <= angularTolerance;
}
Vec parseVec(const std::string& s, bool spatial) {
    if (s.size() > 512)
        throw std::runtime_error("VEC-003: Tọa độ quá dài.");
    std::istringstream in(s);
    std::string part;
    std::vector<double> values;
    while (std::getline(in, part, ';')) {
        Expression e(part);
        if (e.dependsOnX() || part.find('m') != std::string::npos)
            throw std::runtime_error("VEC-003: Tọa độ phải là hằng số; không dùng biến.");
        double v = e.at(0).y;
        finite(v);
        values.push_back(v);
    }
    if (values.size() != (spatial ? 3u : 2u) || s.empty() || s.back() == ';')
        throw std::runtime_error(spatial ? "VEC-003: Nhập x;y;z (dấu chấm thập phân)." :
                                           "VEC-003: Nhập x;y (dấu chấm thập phân).");
    return {values[0], values[1], spatial ? values[2] : 0};
}
std::string tuple(Vec v, bool spatial) {
    return "(" + num(v.x) + "; " + num(v.y) + (spatial ? "; " + num(v.z) : "") + ")";
}
Line line(Vec p, Vec d) {
    checked(p);
    unit(d);
    return {p, d};
}
Line through(Vec a, Vec b) { return line(a, subtract(b, a)); }
Vec project(Vec p, const Line& l) {
    auto d = unit(l.direction);
    return add(l.point, multiply(d, dot(subtract(p, l.point), d)));
}
bool contains(const Line& l, Vec p) {
    auto delta = subtract(p, l.point);
    return length(cross(delta, unit(l.direction))) <= residualTolerance(p, l.point);
}
LinePair compare(const Line& a, const Line& b) {
    auto u = unit(a.direction), v = unit(b.direction), delta = subtract(b.point, a.point);
    auto n = cross(u, v);
    double sn = length(n);
    if (sn <= angularTolerance) {
        auto q = project(b.point, a);
        double distance = length(subtract(q, b.point));
        bool same = distance <= residualTolerance(a.point, b.point);
        return {same ? Relation::Coincident : Relation::Parallel, distance, q, b.point};
    }
    // Triple products avoid subtracting 1-cos² for nearly parallel directions.
    long double denom = (long double)sn * sn;
    double t = double((long double)dot(cross(delta, v), n) / denom);
    double s = double((long double)dot(cross(delta, u), n) / denom);
    auto p = add(a.point, multiply(u, t)), q = add(b.point, multiply(v, s));
    double distance = std::abs(dot(delta, multiply(n, 1 / sn)));
    bool meet = distance <= residualTolerance(a.point, b.point);
    return {meet ? Relation::Intersecting : Relation::Skew, distance, p, q};
}
Vec polar(double f, double az, double el) {
    finite(f);
    finite(az);
    finite(el);
    if (f < 0 || el < -90 || el > 90)
        throw std::runtime_error("VEC-004: Độ lớn ≥ 0; góc nâng trong [−90°,90°].");
    az = std::remainder(az, 360.) * pi / 180;
    el *= pi / 180;
    auto clean = [](double x) { return std::abs(x) < 4 * std::numeric_limits<double>::epsilon() ? 0 : x; };
    return checked(
            {f * clean(std::cos(el) * std::cos(az)), f * clean(std::cos(el) * std::sin(az)), f * clean(std::sin(el))});
}
Forces combine(const std::vector<Vec>& fs) {
    if (fs.empty() || fs.size() > 16)
        throw std::runtime_error("VEC-005: Nhập từ 1 đến 16 lực đồng quy.");
    long double x = 0, y = 0, z = 0, total = 0;
    for (auto f: fs) {
        checked(f);
        x += f.x;
        y += f.y;
        z += f.z;
        total += length(f);
    }
    Vec r = checked({double(x), double(y), double(z)});
    double n = length(r);
    if (n <= 32 * std::numeric_limits<double>::epsilon() * total) {
        r = {};
        n = 0;
    }
    return {r, multiply(r, -1), n,
            n == 0 || (r.x == 0 && r.y == 0) ? std::nullopt : std::optional<double>(std::atan2(r.y, r.x) * 180 / pi),
            n == 0 ? std::nullopt : std::optional<double>(std::atan2(r.z, std::hypot(r.x, r.y)) * 180 / pi)};
}
Vec components(Vec v, double a) {
    finite(a);
    a = std::remainder(a, 360.) * pi / 180;
    auto clean = [](double t) { return std::abs(t) < 4 * std::numeric_limits<double>::epsilon() ? 0 : t; };
    double c = clean(std::cos(a)), s = clean(std::sin(a));
    return checked({v.x * c + v.y * s, -v.x * s + v.y * c, v.z});
}
}  // namespace teaching::coordinate
