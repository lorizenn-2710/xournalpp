/*
 * Test for the dash-pattern scaling: a fixed 6-3 pattern must not close up into a
 * nearly solid line on a thick pen.
 *
 * This links the REAL StrokeStyle implementation, so it exercises the shipped code.
 */

#include <cmath>
#include <cstdio>
#include <vector>

#include "model/LineStyle.h"
#include "model/StrokeStyle.h"

namespace {
int failures = 0;

void check(bool ok, const char* what) {
    std::printf("%-74s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) {
        ++failures;
    }
}

void checkNear(double a, double b, const char* what, double eps = 1e-9) {
    const bool ok = std::abs(a - b) <= eps;
    std::printf("%-74s %s  (%g vs %g)\n", what, ok ? "PASS" : "FAIL", a, b);
    if (!ok) {
        ++failures;
    }
}

void dump(const char* label, const LineStyle& ls) {
    std::printf("   %-28s refWidth=%.2f  dashes=[", label, ls.getReferenceWidth());
    const auto& d = ls.getDashes();
    for (size_t i = 0; i < d.size(); ++i) {
        std::printf("%s%.2f", i ? ", " : "", d[i]);
    }
    std::printf("]\n");
}

}  // namespace

int main() {
    std::printf("=== pattern co giãn theo bề dày ===\n");

    const LineStyle base = StrokeStyle::parseStyle("dash");
    dump("dash gốc (bề dày 1)", base);
    checkNear(base.getDashes()[0], 6.0, "pattern gốc: gạch 6 pt");

    // Bề dày 1: không đổi
    LineStyle w1 = StrokeStyle::scaleStyle(base, 1.0);
    checkNear(w1.getDashes()[0], 6.0, "bề dày 1 -> giữ nguyên 6");
    checkNear(w1.getDashes()[1], 3.0, "bề dày 1 -> giữ nguyên 3");

    // Bề dày lớn nhất (VERY_THICK = 5.67): phải to ra
    const double veryThick = 5.67;
    LineStyle wMax = StrokeStyle::scaleStyle(base, veryThick);
    dump("dash ở bề dày rất dày", wMax);
    check(wMax.getDashes()[0] > 6.0, "bề dày rất dày -> gạch DÀI hơn (không bị bí)");
    checkNear(wMax.getDashes()[0], 6.0 * 1.6, "bề dày rất dày -> đúng hệ số 1.6", 1e-6);

    // Bề dày trung bình: ở giữa
    LineStyle wMed = StrokeStyle::scaleStyle(base, 1.41);
    const double medScale = wMed.getDashes()[0] / 6.0;
    check(medScale > 1.0 && medScale < 1.6, "bề dày trung bình -> hệ số nằm giữa 1.0 và 1.6");

    // Idempotent: gọi lại với cùng bề dày không đổi gì thêm
    LineStyle again = StrokeStyle::scaleStyle(wMax, veryThick);
    checkNear(again.getDashes()[0], wMax.getDashes()[0], "gọi lại cùng bề dày -> KHÔNG nhân thêm");

    // Gọi lại với bề dày khác từ style ĐÃ co giãn: kết quả phải bằng tính từ gốc
    LineStyle rescaled = StrokeStyle::scaleStyle(w1, veryThick);
    checkNear(rescaled.getDashes()[0], 6.0 * 1.6, "đổi bề dày -> tính lại đúng từ pattern gốc", 1e-6);

    // Nét liền: không bị đụng tới
    LineStyle plain = StrokeStyle::parseStyle("plain");
    LineStyle plainScaled = StrokeStyle::scaleStyle(plain, veryThick);
    check(plainScaled.getDashes().empty(), "nét liền -> vẫn không có gạch");

    // Nét chấm: chấm cũng phải to ra, nếu không chấm biến mất trong nét dày
    LineStyle dot = StrokeStyle::parseStyle("dot");
    LineStyle dotScaled = StrokeStyle::scaleStyle(dot, veryThick);
    dump("dot ở bề dày rất dày", dotScaled);
    check(dotScaled.getDashes()[0] > dot.getDashes()[0], "nét chấm -> chấm to ra theo bề dày");

    // đứt-chấm
    LineStyle dd = StrokeStyle::parseStyle("dashdot");
    LineStyle ddScaled = StrokeStyle::scaleStyle(dd, veryThick);
    dump("dashdot ở bề dày rất dày", ddScaled);
    check(ddScaled.getDashes().size() == 4, "đứt-chấm -> vẫn đủ 4 đoạn pattern");

    std::printf("\n%s (%d lỗi)\n", failures == 0 ? "TẤT CẢ ĐỀU ĐẠT" : "CÓ LỖI", failures);
    return failures == 0 ? 0 : 1;
}
