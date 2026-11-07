// SPDX-License-Identifier: GPL-2.0-or-later
#include "TeachingMath.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

#include <gmpxx.h>

namespace teaching {
namespace {
constexpr double nan = std::numeric_limits<double>::quiet_NaN();
constexpr double inf = std::numeric_limits<double>::infinity();
constexpr double pi = 3.14159265358979323846;
void require(bool b, const std::string& s) {
    if (!b)
        throw std::runtime_error(s);
}
}  // namespace
struct Expression::Node {
    char op = 'n';
    double n = 0;
    std::string fn;
    std::shared_ptr<const Node> a, b;
    std::string literal;
    bool variable = false;
    std::string symbol;
    Value eval(double x) const {
        auto value = evalRaw(x);
        // Constant compositions remain differentiable even when an outer
        // chain-rule formula is singular (sqrt(0), abs(0), ...).
        if (!variable && std::isfinite(value.y))
            value.derivative = 0;
        return value;
    }
    Value evalRaw(double x) const {
        if (op == 'n')
            return {n, 0};
        if (op == 'x')
            return {x, 1};
        auto u = a->eval(x);
        auto v = b ? b->eval(x) : Value{0, 0};
        if (!std::isfinite(u.y) || (b && !std::isfinite(v.y)))
            return {nan, nan};
        switch (op) {
            case '+':
                return {u.y + v.y, u.derivative + v.derivative};
            case '-':
                return {u.y - v.y, u.derivative - v.derivative};
            case '*':
                return {u.y * v.y, u.derivative * v.y + u.y * v.derivative};
            case '/':
                if (v.y == 0)
                    return {nan, nan};
                return {u.y / v.y, std::fma(-u.y / v.y, v.derivative, u.derivative) / v.y};
            case '^': {
                if (u.y == 0 && v.y <= 0)
                    return {nan, nan};
                double y = std::pow(u.y, v.y);
                if (v.derivative == 0) {
                    // For huge integer exponents n-1 can round back to n,
                    // reversing the derivative sign on a negative base.
                    double slope = v.y == 0                        ? 0 :
                                   u.y != 0 && std::abs(v.y) > 1e6 ? (v.y / u.y) * y :
                                                                     v.y * std::pow(u.y, v.y - 1);
                    return {y, slope * u.derivative};
                }
                return {y, u.y > 0 ? y * (v.derivative * std::log(u.y) + v.y * u.derivative / u.y) : nan};
            }
            default:
                break;
        }
        if (fn == "sin")
            return {std::sin(u.y), std::cos(u.y) * u.derivative};
        if (fn == "cos")
            return {std::cos(u.y), -std::sin(u.y) * u.derivative};
        if (fn == "exp") {
            double y = std::exp(u.y);
            return {y, y * u.derivative};
        }
        if (fn == "ln")
            return u.y > 0 ? Value{std::log(u.y), u.derivative / u.y} : Value{nan, nan};
        if (fn == "sqrt")
            return u.y >= 0 ? Value{std::sqrt(u.y),
                                    u.y == 0 ? (u.derivative == 0 ? nan : std::copysign(inf, u.derivative)) :
                                               u.derivative / (2 * std::sqrt(u.y))} :
                              Value{nan, nan};
        if (fn == "abs")
            return {std::abs(u.y), u.y == 0 ? (u.derivative == 0 ? 0 : nan) : std::copysign(1., u.y) * u.derivative};
        return {nan, nan};
    }
    bool possible(double l, double r) const {
        if (!variable)
            return std::isfinite(eval(0).y);
        if (!a)
            return true;
        if (!a->possible(l, r) || (b && !b->possible(l, r)))
            return false;
        auto u = a->interval(l, r);
        if (op == 'f' && fn == "ln" && u.second <= 0)
            return false;
        if (op == 'f' && fn == "sqrt" && u.second < 0)
            return false;
        if (op == '/') {
            auto v = b->interval(l, r);
            if (v.first == 0 && v.second == 0)
                return false;
        }
        return true;
    }
    // Conservative interval evaluation: an edge is accepted only when all domain
    // predicates hold on the ENTIRE interval, including the original denominator.
    std::pair<double, double> interval(double l, double r) const {
        if (op == 'n')
            return {n, n};
        if (op == 'x')
            return {l, r};
        if (!variable) {
            double y = eval(0).y;
            return std::isfinite(y) ? std::pair{y, y} : std::pair{nan, nan};
        }
        auto [a0, a1] = a->interval(l, r);
        auto [b0, b1] = b ? b->interval(l, r) : std::pair{0., 0.};
        if (std::isnan(a0) || std::isnan(b0))
            return {nan, nan};
        if (op == '+')
            return {a0 + b0, a1 + b1};
        if (op == '-')
            return {a0 - b1, a1 - b0};
        if (op == '*' || op == '/') {
            if (op == '/') {
                if (b0 <= 0 && b1 >= 0)
                    return {nan, nan};
                double t = 1 / b1;
                b1 = 1 / b0;
                b0 = t;
            }
            std::array<double, 4> p = {a0 * b0, a0 * b1, a1 * b0, a1 * b1};
            if (std::any_of(p.begin(), p.end(), [](double v) { return std::isnan(v); }))
                return {nan, nan};
            return {*std::min_element(p.begin(), p.end()), *std::max_element(p.begin(), p.end())};
        }
        if (op == '^') {
            if (b0 != b1) {
                // a^b = exp(b*ln(a)) for a>0. Enclose the whole rectangle,
                // rather than returning an unbounded interval for 2^x.
                if (a0 > 0) {
                    double l0 = std::log(a0), l1 = std::log(a1);
                    std::array<double, 4> p = {l0 * b0, l0 * b1, l1 * b0, l1 * b1};
                    if (std::any_of(p.begin(), p.end(), [](double v) { return std::isnan(v); }))
                        return {nan, nan};
                    return {std::exp(*std::min_element(p.begin(), p.end())),
                            std::exp(*std::max_element(p.begin(), p.end()))};
                }
                if (a0 == 0 && b0 > 0)
                    return {0, std::max(std::pow(a1, b0), std::pow(a1, b1))};
                return {nan, nan};
            }
            // Preserve excluded 0^0 as well as poles of negative powers.
            if (a0 <= 0 && a1 >= 0 && b0 <= 0)
                return {nan, nan};
            if (a0 < 0 && std::floor(b0) != b0)
                return {nan, nan};
            double p = std::pow(a0, b0), q = std::pow(a1, b0);
            if (std::isnan(p) || std::isnan(q))
                return {nan, nan};
            double low = std::min(p, q);
            // Only EVEN positive integer powers have a minimum of zero across
            // the origin; odd powers must retain their negative lower bound.
            if (a0 <= 0 && a1 >= 0 && b0 > 0 && std::fmod(b0, 2) == 0)
                low = 0;
            return {low, std::max(p, q)};
        }
        if (fn == "sin" || fn == "cos") {
            if (!std::isfinite(a0) || !std::isfinite(a1) || a1 - a0 >= 2 * pi)
                return {-1, 1};
            if (fn == "cos") {
                a0 += pi / 2;
                a1 += pi / 2;
            }
            double low = std::min(std::sin(a0), std::sin(a1)), high = std::max(std::sin(a0), std::sin(a1));
            if (std::ceil((a0 - pi / 2) / (2 * pi)) <= std::floor((a1 - pi / 2) / (2 * pi)))
                high = 1;
            if (std::ceil((a0 + pi / 2) / (2 * pi)) <= std::floor((a1 + pi / 2) / (2 * pi)))
                low = -1;
            return {low, high};
        }
        if (fn == "exp")
            return {std::exp(a0), std::exp(a1)};
        if (fn == "ln")
            return a0 > 0 ? std::pair{std::log(a0), std::log(a1)} : std::pair{nan, nan};
        if (fn == "sqrt")
            return a0 >= 0 ? std::pair{std::sqrt(a0), std::sqrt(a1)} : std::pair{nan, nan};
        if (fn == "abs")
            return {a0 <= 0 && a1 >= 0 ? 0 : std::min(std::abs(a0), std::abs(a1)),
                    std::max(std::abs(a0), std::abs(a1))};
        return {nan, nan};
    }
};
namespace {
using N = std::shared_ptr<const Expression::Node>;
N make(char op, N a = {}, N b = {}, double n = 0, std::string fn = {}, std::string literal = {}, std::string symbol = {}) {
    return std::make_shared<Expression::Node>(Expression::Node{op, n, std::move(fn), a, b, std::move(literal),
                                                               op == 'x' || (a && a->variable) || (b && b->variable), std::move(symbol)});
}
std::string shortest(double v) {
    char buffer[64];
    auto r = std::to_chars(buffer, buffer + 64, v);
    return std::string(buffer, r.ptr);
}
std::string normalized(std::string s) {
    for (const auto& pair: {std::pair{"−", "-"}, {"×", "*"}, {"·", "*"}, {"÷", "/"}}) {
        size_t at = 0;
        while ((at = s.find(pair.first, at)) != std::string::npos) {
            s.replace(at, std::char_traits<char>::length(pair.first), pair.second);
            at += std::char_traits<char>::length(pair.second);
        }
    }
    const std::array<std::string, 10> superscripts = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
    auto digit = [&](size_t pos) {
        for (size_t n = 0; n < superscripts.size(); ++n)
            if (s.compare(pos, superscripts[n].size(), superscripts[n]) == 0)
                return static_cast<int>(n);
        return -1;
    };
    std::string result;
    for (size_t pos = 0; pos < s.size();) {
        int d = digit(pos);
        bool minus = s.compare(pos, 3, "⁻") == 0, plus = s.compare(pos, 3, "⁺") == 0;
        if (d >= 0 || minus || plus) {
            std::string exponent;
            if (minus || plus) {
                exponent = minus ? "-" : "+";
                pos += 3;
            }
            require(digit(pos) >= 0, "Số mũ viết trên cần chữ số.");
            while ((d = digit(pos)) >= 0) {
                exponent += static_cast<char>('0' + d);
                pos += superscripts[static_cast<size_t>(d)].size();
            }
            require(s.compare(pos, 3, "⁻") != 0 && s.compare(pos, 3, "⁺") != 0,
                    "Số mũ viết trên không hợp lệ; dùng ^ và dấu ngoặc.");
            result += "^(" + exponent + ")";
        } else
            result += s[pos++];
    }
    return result;
}
struct Parser {
    std::string s;
    size_t i = 0;
    double m;
    unsigned depth = 0, nodes = 0;
    void ws() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    }
    bool eat(char c) {
        ws();
        if (i < s.size() && s[i] == c) {
            ++i;
            return true;
        }
        return false;
    }
    N expr() {
        auto a = term();
        for (;;) {
            if (eat('+'))
                a = make('+', a, term());
            else if (eat('-'))
                a = make('-', a, term());
            else
                return a;
        }
    }
    N term() {
        auto a = unary();
        for (;;) {
            if (eat('*'))
                a = make('*', a, unary());
            else if (eat('/'))
                a = make('/', a, unary());
            else
                return a;
        }
    }
    N unary() {
        require(++depth <= 64, "Công thức lồng quá sâu (tối đa 64).");
        N n;
        if (eat('+'))
            n = unary();
        else if (eat('-'))
            n = make('-', make('n'), unary());
        else
            n = power();
        --depth;
        return n;
    }
    N power() {
        auto a = atom();
        if (eat('^'))
            a = make('^', a, unary());
        return a;
    }
    N atom() {
        require(++nodes <= 256, "Công thức quá dài (tối đa 256 nút).");
        ws();
        if (eat('(')) {
            auto a = expr();
            require(eat(')'), "Thiếu dấu ).");
            return a;
        }
        require(i < s.size(), "Thiếu số hoặc biểu thức.");
        if (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.') {
            const size_t start = i;
            double v = 0;
            auto parsed = std::from_chars(s.data() + i, s.data() + s.size(), v);
            require(parsed.ec == std::errc{} && parsed.ptr != s.data() + i && std::isfinite(v), "Số không hữu hạn.");
            i = static_cast<size_t>(parsed.ptr - s.data());
            return make('n', {}, {}, v, {}, s.substr(start, i - start));
        }
        if (s.compare(i, 2, "π") == 0) {
            i += 2;
            return make('n', {}, {}, pi, {}, {}, "π");
        }
        size_t start = i;
        while (i < s.size() && std::isalpha(static_cast<unsigned char>(s[i]))) ++i;
        auto name = s.substr(start, i - start);
        if (name == "x")
            return make('x');
        if (name == "m")
            return make('n', {}, {}, m, {}, shortest(m), "m");
        if (name == "pi")
            return make('n', {}, {}, pi, {}, {}, "π");
        if (name == "e")
            return make('n', {}, {}, std::exp(1.), {}, {}, "e");
        require(name == "sin" || name == "cos" || name == "exp" || name == "ln" || name == "sqrt" || name == "abs",
                "Tên không hỗ trợ tại vị trí " + std::to_string(start + 1) + ".");
        require(eat('('), "Hàm cần dấu (.");
        auto a = expr();
        require(eat(')'), "Thiếu dấu ).");
        return make('f', a, {}, 0, name);
    }
};
}  // namespace
Expression::Expression(const std::string& f, double m) {
    require(f.size() <= 1024 && std::isfinite(m), "Công thức hoặc tham số m không hợp lệ.");
    Parser p{normalized(f), 0, m};
    root = p.expr();
    p.ws();
    require(p.i == p.s.size(), "Ký tự thừa tại vị trí " + std::to_string(p.i + 1) + "; dùng * để nhân.");
}
std::shared_ptr<const MathNotation> Expression::notation() const {
    std::function<std::shared_ptr<const MathNotation>(N)> visit = [&](N n) -> std::shared_ptr<const MathNotation> {
        if (!n) return {};
        std::string text = n->op == 'n' ? (!n->symbol.empty() ? n->symbol : !n->literal.empty() ? n->literal : shortest(n->n)) : n->fn;
        return std::make_shared<MathNotation>(MathNotation{n->op, text, visit(n->a), visit(n->b)});
    };
    return visit(root);
}
std::string Expression::latex() const {
    std::function<std::string(std::shared_ptr<const MathNotation>)> visit = [&](auto n) -> std::string {
        if(n->op=='x')return "x";
        if(n->op=='n')return n->text=="π" ? "\\pi" : n->text;
        auto a=visit(n->a);
        if(n->op=='f') {
            if(n->text=="sqrt")return "\\sqrt{"+a+"}";
            if(n->text=="abs")return "\\left|"+a+"\\right|";
            return "\\"+n->text+"\\left("+a+"\\right)";
        }
        auto b=visit(n->b);
        if(n->op=='/')return "\\frac{"+a+"}{"+b+"}";
        if(n->op=='^')return "{\\left("+a+"\\right)}^{"+b+"}";
        return "\\left("+a+(n->op=='*'?"\\cdot ":std::string(1,n->op))+b+"\\right)";
    };
    return visit(notation());
}
Value Expression::at(double x) const { return root->eval(x); }
bool Expression::dependsOnX() const { return root->variable; }
bool Expression::continuous(double a, double b) const {
    auto v = root->interval(a, b);
    return !std::isnan(v.first) && !std::isnan(v.second);
}
void Window::validate() const {
    require(std::isfinite(xmin) && std::isfinite(xmax) && std::isfinite(ymin) && std::isfinite(ymax) && xmin < xmax &&
                    ymin < ymax && std::isfinite(xmax - xmin) && std::isfinite(ymax - ymin) && xmax - xmin >= 1e-9 &&
                    ymax - ymin >= 1e-9 &&
                    std::max({std::abs(xmin), std::abs(xmax), std::abs(ymin), std::abs(ymax)}) <= 1e12,
            "Miền x/y: cần min < max, độ rộng ≥ 1e-9, |mốc| ≤ 1e12.");
}
Sample sample(const Expression& f, const Window& w, double width, double height) {
    w.validate();
    Sample out;
    std::vector<Point> run;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(180);
    // For a verified quadratic/linear quotient with a removable singularity,
    // exact polynomial division certifies an affine function on its original
    // domain. Natural interval division loses numerator/denominator correlation
    // near the hole and needlessly exhausts the point budget.
    bool affineWithHole = false;
    try {
        auto report = analyze(f);
        affineWithHole = report.hasAsymptote &&
                         std::any_of(report.points.begin(), report.points.end(), [](const auto& p) { return p.hole; });
    } catch (const std::runtime_error&) {}
    auto flush = [&] {
        if (run.size() > 1)
            out.branches.push_back(std::move(run));
        run.clear();
    };
    auto val = [&](double x) {
        if (out.evaluations >= 30000) {
            out.limited = true;
            return nan;
        }
        ++out.evaluations;
        double y = f.at(x).y;
        out.defined += std::isfinite(y);
        return y;
    };
    std::function<void(double, double, double, double, int)> edge = [&](double a, double ya, double b, double yb,
                                                                        int depth) {
        if (out.limited || out.evaluations >= 30000 ||
            ((out.evaluations % 64 == 0) && std::chrono::steady_clock::now() > deadline)) {
            out.limited = true;
            flush();
            return;
        }
        if (!f.root->possible(a, b)) {
            flush();
            return;
        }
        double c = a + (b - a) / 2, yc = val(c);
        bool safe = f.continuous(a, b);
        bool finite = std::isfinite(ya) && std::isfinite(yb) && std::isfinite(yc);
        double err = finite ? std::abs((ya / 2 + yb / 2) - yc) * height / (w.ymax - w.ymin) : inf;
        auto bounds = f.root->interval(a, b);
        if (affineWithHole && safe && finite)
            bounds = {std::min(ya, yb), std::max(ya, yb)};
        bool outside = safe && (bounds.first > w.ymax || bounds.second < w.ymin);
        if (safe && finite)
            err = std::max(err, std::max(std::min(ya, yb) - bounds.first, bounds.second - std::max(ya, yb)) * height /
                                        (w.ymax - w.ymin));
        if (!affineWithHole && safe && finite && !outside &&
            (depth == 16 || (err <= .3 && (b - a) * width / (w.xmax - w.xmin) <= 8))) {
            // A cubic inflection can lie exactly on its endpoint chord at the
            // midpoint while bowing far away at 1/4 and 3/4. Test those positions
            // as well before accepting the actual polyline edge.
            for (double t: {.25, .75}) {
                double y = val(a + (b - a) * t);
                if (!std::isfinite(y)) {
                    finite = false;
                    break;
                }
                err = std::max(err, std::abs(std::lerp(ya, yb, t) - y) * height / (w.ymax - w.ymin));
            }
        }
        if (depth < 16 && (!safe || !finite || (!outside && (err > .3 || (b - a) * width / (w.xmax - w.xmin) > 8)))) {
            edge(a, ya, c, yc, depth + 1);
            edge(c, yc, b, yb, depth + 1);
            return;
        }
        if (!safe || !finite || (!outside && err > 1.2)) {
            flush();
            return;
        }
        if (outside) {
            flush();
            return;
        }
        // Clip in mathematical coordinates before mapping to the page.
        double t0 = 0, t1 = 1, dy = yb - ya;
        if (dy == 0) {
            if (ya < w.ymin || ya > w.ymax) {
                flush();
                return;
            }
        } else {
            double p = (w.ymin - ya) / dy, q = (w.ymax - ya) / dy;
            if (p > q)
                std::swap(p, q);
            t0 = std::max(0., p);
            t1 = std::min(1., q);
            if (t0 > t1) {
                flush();
                return;
            }
        }
        Point p{a + (b - a) * t0, ya + dy * t0}, q{a + (b - a) * t1, ya + dy * t1};
        if (!run.empty() && (run.back().x != p.x || std::abs(run.back().y - p.y) > 1e-8 * (w.ymax - w.ymin)))
            flush();
        if (run.empty())
            run.push_back(p);
        run.push_back(q);
        if (t1 < 1)
            flush();
    };
    // A fixed coarse partition also prevents midpoint aliasing of ordinary oscillations.
    for (int i = 0; i < 128 && !out.limited; ++i) {
        double a = w.xmin + (w.xmax - w.xmin) * i / 128, b = w.xmin + (w.xmax - w.xmin) * (i + 1) / 128;
        edge(a, val(a), b, val(b), 0);
    }
    flush();
    return out;
}
namespace {
using Exact = mpq_class;
// Keep decimal literals as rationals, so 0.1²-0.01 is exactly zero while a
// genuinely nonzero residual is never mistaken for a removable singularity.
Exact exactNumber(const N& n) {
    if (n->literal.empty())
        return Exact(n->n);
    std::string s = n->literal;
    auto pos = s.find_first_of("eE");
    std::string exponentText = pos == std::string::npos ? "0" : s.substr(pos + 1);
    if (pos != std::string::npos)
        s.resize(pos);
    int fractional = 0;
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        fractional = static_cast<int>(s.size() - dot - 1);
        s.erase(dot, 1);
    }
    mpz_class numerator(s, 10), denominator(1), power;
    // 0e1000000000 is still zero. Do not allocate enormous powers of ten (or
    // overflow stoi) for a zero coefficient before the algebra bit limit runs.
    if (numerator == 0)
        return Exact(0);
    int exponent = std::stoi(exponentText) - fractional;
    require(exponent >= -4930 && exponent <= 4930, "Hệ số vượt giới hạn 16384 bit.");
    mpz_ui_pow_ui(power.get_mpz_t(), 10, static_cast<unsigned long>(std::abs(exponent)));
    if (exponent >= 0)
        numerator *= power;
    else
        denominator = power;
    Exact v(numerator, denominator);
    v.canonicalize();
    return v;
}
double asDouble(const Exact& q) {
    double v = q.get_d();
    require(std::isfinite(v) && (v != 0 || q == 0), "Kết quả vượt độ phân giải số; hãy dùng bảng nhập tay.");
    return v;
}
long double asLongDouble(const Exact& q) {
    if (q == 0)
        return 0;
    mp_exp_t exponent;
    mpf_class f(q, 160);
    std::string digits = f.get_str(exponent, 10, 40);
    bool neg = digits[0] == '-';
    if (neg)
        digits.erase(0, 1);
    std::string decimal = (neg ? "-0." : "0.") + digits + "e" + std::to_string(exponent);
    long double v = 0;
    auto parsed = std::from_chars(decimal.data(), decimal.data() + decimal.size(), v);
    require(parsed.ec == std::errc{} && std::isfinite(v), "Hệ số vượt độ phân giải số.");
    return v;
}
using Poly = std::vector<Exact>;
void trim(Poly& p) {
    for (const auto& c: p)
        require(mpz_sizeinbase(c.get_num_mpz_t(), 2) <= 16384 && mpz_sizeinbase(c.get_den_mpz_t(), 2) <= 16384,
                "Hệ số quá phức tạp (giới hạn 16384 bit); dùng bảng nhập tay.");
    while (p.size() > 1 && p.back() == 0) p.pop_back();
}
Poly add(Poly a, const Poly& b, int sign = 1) {
    a.resize(std::max(a.size(), b.size()));
    for (size_t i = 0; i < b.size(); ++i) a[i] += sign * b[i];
    trim(a);
    return a;
}
Poly mul(const Poly& a, const Poly& b) {
    require(a.size() + b.size() <= 10, "Bậc biểu thức vượt phạm vi khảo sát.");
    Poly p(a.size() + b.size() - 1);
    for (size_t i = 0; i < a.size(); ++i)
        for (size_t j = 0; j < b.size(); ++j) p[i + j] += a[i] * b[j];
    trim(p);
    return p;
}
Poly deriv(const Poly& a) {
    Poly p(std::max(size_t(1), a.size() - 1));
    for (size_t i = 1; i < a.size(); ++i) p[i - 1] = a[i] * static_cast<unsigned long>(i);
    return p;
}
Exact eval(const Poly& p, const Exact& x) {
    Exact y = 0;
    for (auto i = p.rbegin(); i != p.rend(); ++i) y = y * x + *i;
    return y;
}
struct Rational {
    Poly p{0}, q{1};
};
Rational rational(const N& n) {
    if (n->op == 'n')
        return {{exactNumber(n)}, {1}};
    if (n->op == 'x')
        return {{0, 1}, {1}};
    require(n->op != 'f', "Tự sinh chỉ hỗ trợ đa thức ≤ bậc 3 và phân thức tử ≤ bậc 2 / mẫu bậc 1.");
    auto a = rational(n->a);
    auto b = rational(n->b);
    require(!(a.q.size() == 1 && a.q[0] == 0) && !(b.q.size() == 1 && b.q[0] == 0), "Mẫu bằng 0 trong biểu thức gốc.");
    switch (n->op) {
        case '+':
            return {add(mul(a.p, b.q), mul(b.p, a.q)), mul(a.q, b.q)};
        case '-':
            return {add(mul(a.p, b.q), mul(b.p, a.q), -1), mul(a.q, b.q)};
        case '*':
            return {mul(a.p, b.p), mul(a.q, b.q)};
        case '/':
            require(b.q.size() == 1, "Mẫu lồng nhau chưa được khảo sát tự động.");
            return {mul(a.p, b.q), mul(a.q, b.p)};
        case '^': {
            require(b.p.size() == 1 && b.q.size() == 1, "Số mũ phải là số nguyên 0..3.");
            require(b.q[0] != 0, "Số mũ không xác định.");
            Exact exactExponent = b.p[0] / b.q[0];
            double e = asDouble(exactExponent);
            require(e >= 0 && e <= 3 && exactExponent.get_den() == 1, "Số mũ khảo sát phải là số nguyên 0..3.");
            require(!(a.q.size() == 1 && a.q[0] == 0) && !(e == 0 && (a.q.size() > 1 || a.p.size() > 1 || a.p[0] == 0)),
                    "Không tự khảo sát lũy thừa 0 của biểu thức có điểm loại.");
            Rational r{{1}, {1}};
            for (int i = 0; i < e; ++i) {
                r.p = mul(r.p, a.p);
                r.q = mul(r.q, a.q);
            }
            return r;
        }
        default:
            throw std::runtime_error("Không thuộc họ khảo sát.");
    }
}
std::vector<double> roots(Poly p) {
    trim(p);
    require(p.size() <= 3, "Đạo hàm vượt bậc hai.");
    if (p.size() == 1)
        return {};
    if (p.size() == 2)
        return {asDouble(Exact(-p[0] / p[1]))};
    Exact discriminant = p[1] * p[1] - 4 * p[2] * p[0];
    if (discriminant < 0)
        return {};
    if (discriminant == 0)
        return {asDouble(Exact(-p[1] / (2 * p[2])))};
    Exact scale = 0;
    for (const auto& v: p)
        if (abs(v) > scale)
            scale = abs(v);
    for (auto& v: p) v /= scale;
    long double a = asLongDouble(p[2]), b = asLongDouble(p[1]), c = asLongDouble(p[0]);
    long double d = asLongDouble(Exact(discriminant / (scale * scale)));
    long double q = -.5L * (b + std::copysign(std::sqrt(d), b));
    std::vector<double> r = {static_cast<double>(q / a), static_cast<double>(c / q)};
    std::sort(r.begin(), r.end());
    require(r[0] != r[1], "Hai nghiệm quá gần nhau so với độ phân giải số.");
    return r;
}
template <class T>
int sgn(const T& a) {
    return (a > 0) - (a < 0);
}
std::string limitInfinity(const Poly& p, const Poly& q, int side) {
    int deg = static_cast<int>(p.size()) - static_cast<int>(q.size());
    if (p.size() == 1 && p[0] == 0)
        return "0";
    if (deg < 0)
        return "0";
    if (deg == 0)
        return number(asDouble(Exact(p.back() / q.back())));
    return sgn(p.back() / q.back()) * (side < 0 && deg % 2 ? -1 : 1) > 0 ? "+∞" : "−∞";
}
}  // namespace
std::string number(double x) {
    if (std::isinf(x))
        return x > 0 ? "+∞" : "−∞";
    if (std::isnan(x))
        return "KXĐ";
    if (x == 0)
        x = 0;
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << std::setprecision(6) << x;
    return s.str();
}
Analysis analyze(const Expression& f) {
    auto r = rational(f.root);
    auto &p = r.p, &q = r.q;
    require(q.size() <= 2 && p.size() <= (q.size() == 1 ? 4u : 3u) && !(q.size() == 1 && q[0] == 0),
            "Tự sinh: đa thức ≤ 3 hoặc tử ≤ 2 / mẫu bậc 1 khác 0.");
    Poly d = add(mul(deriv(p), q), mul(p, deriv(q)), -1);
    auto dr = roots(d);
    Analysis a;
    Exact exactPole = q.size() == 2 ? Exact(-q[0] / q[1]) : Exact(0);
    double pole = q.size() == 2 ? asDouble(exactPole) : nan;
    for (double x: dr) {
        require(std::isfinite(x), "Nghiệm đạo hàm vượt phạm vi số.");
        require(x != pole || eval(d, exactPole) == 0, "Nghiệm đạo hàm quá gần điểm loại so với độ phân giải số.");
        if (x != pole) {
            double y = f.at(x).y;
            require(std::isfinite(y), "Giá trị tại mốc vượt phạm vi số; nhập bảng tay với giá trị thích hợp.");
            a.points.push_back({x, false, false, y, 0});
        }
    }
    bool hole = false;
    double holeValue = nan;
    if (std::isfinite(pole)) {
        // A removable singularity is claimed only when the polynomial evaluation
        // is exactly zero; near cancellation is kept as a genuine pole.
        hole = eval(p, exactPole) == 0;
        holeValue = hole ? asDouble(Exact(eval(deriv(p), exactPole) / q[1])) : nan;
        require(!hole || std::isfinite(holeValue), "Giá trị lỗ thủng vượt phạm vi số.");
        a.points.push_back({pole, true, hole, hole ? holeValue : nan, 0});
        if (!hole)
            a.verticalAsymptotes.push_back(pole);
    }
    std::sort(a.points.begin(), a.points.end(), [](auto l, auto r) { return l.x < r.x; });
    // The numerator of f' is a polynomial of degree <=2. Its leading sign,
    // real roots and multiplicities determine every interval sign; q² is positive.
    int initial = sgn(d.back());
    if ((d.size() - 1) % 2)
        initial = -initial;
    a.signs.push_back(initial);
    for (const auto& point: a.points) {
        int s = a.signs.back();
        if (std::find(dr.begin(), dr.end(), point.x) != dr.end()) {
            bool repeated = dr.size() == 1 && d.size() == 3;  // discriminant == 0
            if (!repeated)
                s = -s;
        }
        a.signs.push_back(s);
    }
    a.leftLimits.push_back(limitInfinity(p, q, -1));
    a.rightLimits.push_back(a.leftLimits.back());
    for (size_t i = 0; i < a.points.size(); ++i) {
        auto& b = a.points[i];
        std::string l, rval;
        if (!b.excluded || b.hole)
            l = rval = number(b.value);
        else {
            int right = sgn(eval(p, exactPole) / q[1]);
            l = right > 0 ? "−∞" : "+∞";
            rval = right > 0 ? "+∞" : "−∞";
        }
        a.leftLimits.push_back(l);
        a.rightLimits.push_back(rval);
        if (!b.excluded && a.signs[i] != a.signs[i + 1])
            b.extremum = a.signs[i] > 0 ? 1 : -1;
    }
    a.leftLimits.push_back(limitInfinity(p, q, 1));
    a.rightLimits.push_back(a.leftLimits.back());
    if (q.size() == 2) {
        a.hasAsymptote = true;
        if (p.size() == 3) {
            a.slope = asDouble(Exact(p[2] / q[1]));
            a.intercept = asDouble(Exact((p[1] - p[2] * q[0] / q[1]) / q[1]));
        } else {
            a.slope = 0;
            a.intercept = p.size() == 2 ? asDouble(Exact(p[1] / q[1])) : 0;
        }
    }
    // Equal-degree ratios have the same finite limit on both ends.
    if (p.size() == q.size()) {
        a.leftLimits.front() = a.rightLimits.front() = a.leftLimits.back() = a.rightLimits.back() =
                number(asDouble(Exact(p.back() / q.back())));
    }
    return a;
}
Table variationTable(const Analysis& a) {
    Table t;
    t.x = {"−∞"};
    t.value = {a.leftLimits.front()};
    t.sign = {""};
    for (size_t i = 0; i < a.signs.size(); ++i) {
        t.x.push_back("");
        t.sign.push_back(a.signs[i] > 0 ? "+" : a.signs[i] < 0 ? "−" : "0");
        t.value.push_back(a.signs[i] > 0 ? "↗" : a.signs[i] < 0 ? "↘" : "→");
        if (i < a.points.size()) {
            auto& b = a.points[i];
            t.x.push_back(number(b.x) + (b.excluded ? " (loại)" : ""));
            t.sign.push_back(b.excluded ? "||" : "0");
            t.value.push_back(b.excluded && !b.hole ? a.leftLimits[i + 1] + " || " + a.rightLimits[i + 1] :
                                                      a.leftLimits[i + 1] + (b.hole ? " ○" : ""));
        }
    }
    t.x.push_back("+∞");
    t.sign.push_back("");
    t.value.push_back(a.leftLimits.back());
    return t;
}
namespace {
std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> r;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i)
        if (i == s.size() || s[i] == '|') {
            r.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    return r;
}
}  // namespace
Table parseTable(const std::string& x, const std::string& s, const std::string& v, const std::string& n) {
    Table t{split(x), split(s), split(v), n};
    require(t.x.size() >= 2 && t.x.size() <= 19 && t.sign.size() == t.x.size() && t.value.size() == t.x.size(),
            "Bảng: 2..19 ô mỗi hàng, cùng số ô; phân cách bằng |.");
    for (const auto* row: {&t.x, &t.sign, &t.value})
        for (const auto& cell: *row) require(cell.size() <= 64, "Ô bảng quá dài (tối đa 64 byte).");
    require(n.size() <= 24, "Tên hàng quá dài.");
    return t;
}
}  // namespace teaching
