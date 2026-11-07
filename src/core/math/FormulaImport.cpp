// SPDX-License-Identifier: GPL-2.0-or-later
#include "FormulaImport.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "TeachingMath.h"
namespace teaching {
namespace {
void replace(std::string& s, const std::string& a, const std::string& b) {
    for (size_t p = 0; (p = s.find(a, p)) != std::string::npos; p += b.size()) s.replace(p, a.size(), b);
}
// Strict LaTeX notation reader: commands are a closed whitelist, never executed.
struct Notation {
    std::string s;
    size_t i = 0;
    unsigned depth = 0;
    std::string group() {
        if (++depth > 32)
            throw std::runtime_error("Công thức lồng quá sâu.");
        if (i >= s.size() || s[i++] != '{')
            throw std::runtime_error("Cần nhóm {...} đầy đủ.");
        auto r = read(true);
        --depth;
        return "(" + r + ")";
    }
    std::string read(bool nested = false) {
        std::string r;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '}') {
                if (!nested)
                    throw std::runtime_error("Dấu } thừa.");
                return r;
            }
            if (c == '{') {
                --i;
                r += group();
                continue;
            }
            if (c != '\\') {
                r += c;
                continue;
            }
            auto start = i;
            while (i < s.size() && std::isalpha(static_cast<unsigned char>(s[i]))) ++i;
            std::string cmd = s.substr(start, i - start);
            while (i < s.size() && s[i] == ' ') ++i;
            if (cmd == "left" || cmd == "right" || cmd == "displaystyle" || cmd == "textstyle")
                continue;
            if (cmd == "frac" || cmd == "dfrac" || cmd == "tfrac") {
                auto a = group(), b = group();
                r += "(" + a + "/" + b + ")";
            } else if (cmd == "sqrt")
                r += "sqrt" + group();
            else if (cmd == "sin" || cmd == "cos" || cmd == "ln" || cmd == "exp")
                r += cmd + " ";
            else if (cmd == "pi")
                r += "pi ";
            else if (cmd == "cdot" || cmd == "times")
                r += "*";
            else if (cmd == "lvert" || cmd == "rvert" || cmd == "vert")
                r += "|";
            else if (cmd.empty() && i < s.size() && (s[i] == ',' || s[i] == ';' || s[i] == '!' || s[i] == ' '))
                ++i;
            else
                throw std::runtime_error("Ký hiệu LaTeX chưa hỗ trợ: \\" + cmd);
        }
        if (nested)
            throw std::runtime_error("Thiếu dấu }.");
        return r;
    }
};
}  // namespace
FormulaCandidate importFormula(const std::string& raw, const std::string& source) {
    FormulaCandidate c;
    c.raw = raw;
    c.source = source;
    if (raw.size() > 2048) {
        c.issue = FormulaIssue::TooLong;
        c.message = "Vùng quá dài; chọn riêng một công thức (≤2048 byte).";
        return c;
    }
    if (raw.find_first_not_of(" \r\n\t") == std::string::npos) {
        c.issue = FormulaIssue::Empty;
        c.message = "Không đọc được công thức. Chọn lại vùng hoặc nhận dạng ảnh.";
        return c;
    }
    try {
        std::string s = raw;
        replace(s, "−", "-");
        replace(s, "–", "-");
        replace(s, "×", "*");
        replace(s, "·", "*");
        replace(s, "÷", "/");
        replace(s, "π", "pi");
        replace(s, "\\(", "");
        replace(s, "\\)", "");
        replace(s, "$", "");
        // Unicode superscripts must be grouped as ONE exponent, including sign.
        const std::vector<std::pair<std::string, std::string>> supers = {
                {"⁰", "0"}, {"¹", "1"}, {"²", "2"}, {"³", "3"}, {"⁴", "4"}, {"⁵", "5"},
                {"⁶", "6"}, {"⁷", "7"}, {"⁸", "8"}, {"⁹", "9"}, {"⁻", "-"}, {"⁺", "+"}};
        std::string normal, exponent;
        for (size_t p = 0; p < s.size();) {
            bool matched = false;
            for (auto& [u, a]: supers)
                if (s.compare(p, u.size(), u) == 0) {
                    exponent += a;
                    p += u.size();
                    matched = true;
                    break;
                }
            if (matched)
                continue;
            if (!exponent.empty()) {
                normal += "^(" + exponent + ")";
                exponent.clear();
            }
            normal += s[p++];
        }
        if (!exponent.empty())
            normal += "^(" + exponent + ")";
        s = normal;
        replace(s, "{=}", "=");
        replace(s, "{+}", "+");
        replace(s, "{-}", "-");
        s = Notation{s}.read();
        s = std::regex_replace(s, std::regex(R"(^\s*(y|f\s*\(\s*x\s*\))\s*=\s*)"), "");
        if (s.find('=') != std::string::npos || s.find('_') != std::string::npos || s.find("≤") != std::string::npos)
            throw std::runtime_error("Chỉ hỗ trợ y=f(x); phương trình ẩn, hệ hoặc chỉ số cần chọn cách hiểu/sửa rõ.");
        // Absolute bars are paired; nested/adjacent bars are deliberately rejected.
        bool bar = false;
        std::string b;
        for (char ch: s) {
            if (ch == '|') {
                b += bar ? ")" : "abs(";
                bar = !bar;
            } else
                b += ch;
        }
        if (bar)
            throw std::runtime_error("Thiếu dấu |. Kiểm tra vùng bị cắt.");
        s = b;
        if (s.find("√") != std::string::npos && s.find("√(") == std::string::npos)
            throw std::runtime_error("Phạm vi gạch căn chưa rõ. Dùng sqrt(...) hoặc nhận dạng ảnh vùng gốc.");
        replace(s, "√", "sqrt");
        // Tokenize without guessing I/l/O or replacing unfamiliar variables.
        std::vector<std::string> tokens;
        for (size_t p = 0; p < s.size();) {
            unsigned char ch = s[p];
            if (std::isspace(ch)) {
                ++p;
                continue;
            }
            size_t start = p;
            if (std::isdigit(ch) || ch == '.') {
                ++p;
                while (p < s.size() && (std::isdigit(static_cast<unsigned char>(s[p])) || s[p] == '.')) ++p;
                if (p < s.size() && s[p] == 'e' && p + 1 < s.size() &&
                    (std::isdigit(static_cast<unsigned char>(s[p + 1])) || s[p + 1] == '+' || s[p + 1] == '-')) {
                    ++p;
                    if (s[p] == '+' || s[p] == '-')
                        ++p;
                    while (p < s.size() && std::isdigit(static_cast<unsigned char>(s[p]))) ++p;
                }
            } else if (std::isalpha(ch)) {
                ++p;
                while (p < s.size() && std::isalpha(static_cast<unsigned char>(s[p]))) ++p;
            } else
                ++p;
            auto token = s.substr(start, p - start);
            if (token == "mx") {
                tokens.push_back("m");
                tokens.push_back("x");
            } else
                tokens.push_back(token);
        }
        auto fn = [](const std::string& t) {
            return t == "sin" || t == "cos" || t == "ln" || t == "sqrt" || t == "exp" || t == "abs";
        };
        auto atom = [](const std::string& t) {
            return t == "x" || t == "m" || t == "pi" || t == "e" ||
                   (!t.empty() && (std::isdigit(static_cast<unsigned char>(t[0])) || t[0] == '.'));
        };
        // ln x / sin x are common printed notation. Require explicit parentheses
        // for powers or compound arguments rather than silently choosing a scope.
        for (size_t j = 0; j < tokens.size(); ++j)
            if (fn(tokens[j]) && j + 1 < tokens.size() && tokens[j + 1] != "(") {
                if (!atom(tokens[j + 1]) || (j + 2 < tokens.size() && tokens[j + 2] == "^"))
                    throw std::runtime_error("Phạm vi hàm chưa rõ; thêm ngoặc, ví dụ sin(x^2). ");
                tokens.insert(tokens.begin() + j + 1, "(");
                tokens.insert(tokens.begin() + j + 3, ")");
            }
        std::string result;
        std::vector<bool> division = {false};
        for (size_t j = 0; j < tokens.size(); ++j) {
            bool implicit = j > 0 && (atom(tokens[j - 1]) || tokens[j - 1] == ")") &&
                            (atom(tokens[j]) || tokens[j] == "(" || fn(tokens[j]));
            if (implicit) {
                if (division.back())
                    throw std::runtime_error("Phép nhân sau dấu / chưa rõ phạm vi mẫu. Thêm ngoặc hoặc dấu * rõ ràng.");
                if ((tokens[j - 1] == "x" || tokens[j - 1] == "m") && !tokens[j].empty() &&
                    std::isdigit(static_cast<unsigned char>(tokens[j][0])))
                    throw std::runtime_error(
                            "Chữ số sau biến có thể là mũ/chỉ số. Dùng ^ hoặc * rõ ràng; đối chiếu ảnh.");
                result += "*";
            }
            if (tokens[j] == "(")
                division.push_back(false);
            else if (tokens[j] == ")" && division.size() > 1)
                division.pop_back();
            else if (tokens[j] == "/")
                division.back() = true;
            else if (tokens[j] == "+" || tokens[j] == "-")
                division.back() = false;
            result += tokens[j];
        }
        Expression checked(result);
        c.expression = result;
        c.message = "Đối chiếu dấu, số mũ, phạm vi căn/phân số và miền xác định trước khi chèn.";
    } catch (const std::exception& e) {
        c.issue = FormulaIssue::Unsupported;
        c.message = e.what();
    }
    return c;
}
std::vector<FormulaCandidate> importFormulaLines(const std::string& raw, const std::string& source) {
    std::vector<FormulaCandidate> out;
    std::istringstream lines(raw);
    std::string s;
    while (std::getline(lines, s) && out.size() < 12)
        if (s.find_first_not_of(" \r\t") != std::string::npos)
            out.push_back(importFormula(s, source));
    return out;
}
}  // namespace teaching
