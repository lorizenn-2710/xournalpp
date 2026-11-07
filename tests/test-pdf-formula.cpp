// SPDX-License-Identifier: GPL-2.0-or-later
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include <poppler.h>

#include "math/PdfFormulaJob.h"
#include "math/TeachingMath.h"
#include "pdf/popplerapi/PopplerGlibPage.h"
using namespace teaching;
namespace {
std::string fixtures = std::string(PDF_FORMULA_SOURCE_DIR) + "/output/pdf/pdf-formula-fixtures/";
PdfFormulaSnapshot snapshot(const char* file, int page, Range bounds, bool text = false) {
    auto* uri = g_filename_to_uri((fixtures + file).c_str(), nullptr, nullptr);
    GError* error = nullptr;
    auto* doc = poppler_document_new_from_file(uri, nullptr, &error);
    g_free(uri);
    if (!doc)
        throw std::runtime_error(error ? error->message : "fixture PDF not found; run make-formula-fixtures.py");
    auto* raw = poppler_document_get_page(doc, page);
    auto pdf = std::make_shared<PopplerGlibPage>(raw, doc, std::make_shared<std::mutex>());
    g_object_unref(raw);
    g_object_unref(doc);  // snapshot must independently own its PDF
    std::string source = text ? pdf->selectText({bounds.minX, bounds.minY, bounds.maxX, bounds.maxY},
                                                XojPdfPageSelectionStyle::Area) :
                                "";
    return {pdf, bounds, static_cast<size_t>(page), source};
}
RecognitionResult recognize(PdfFormulaSnapshot source, int cancel = 0, unsigned timeoutMs = 60000) {
    auto* loop = g_main_loop_new(nullptr, false);
    RecognitionResult result;
    auto job = PdfFormulaJob::start(
            std::move(source),
            [&](RecognitionResult r) {
                result = std::move(r);
                g_main_loop_quit(loop);
            },
            timeoutMs);
    guint cancelTimer = 0;
    if (cancel == 1)
        job->cancel();
    if (cancel == 2)
        cancelTimer = g_timeout_add(
                150,
                +[](gpointer p) -> gboolean {
                    static_cast<PdfFormulaJob*>(p)->cancel();
                    return G_SOURCE_REMOVE;
                },
                job.get());
    auto timeout = g_timeout_add_seconds(
            15,
            +[](gpointer p) -> gboolean {
                static_cast<PdfFormulaJob*>(p)->cancel();
                return G_SOURCE_CONTINUE;
            },
            job.get());
    g_main_loop_run(loop);
    g_source_remove(timeout);
    if (cancelTimer && g_main_context_find_source_by_id(nullptr, cancelTimer))
        g_source_remove(cancelTimer);
    g_main_loop_unref(loop);
    job.reset();
    if (!result.imagePath.empty() && std::filesystem::exists(result.imagePath))
        throw std::runtime_error("private crop leaked after job destruction");
    return result;
}
bool matches(const RecognitionResult& r, const char* expression) {
    for (const auto& c: r.candidates)
        if (c.usable()) {
            Expression a(c.expression), b(expression);
            bool ok = true;
            for (double x: {-2., -1., 0., 1., 2.}) {
                auto u = a.at(x), v = b.at(x);
                if (std::isfinite(u.y) != std::isfinite(v.y) || (std::isfinite(u.y) && std::abs(u.y - v.y) > 1e-9))
                    ok = false;
            }
            if (ok)
                return true;
        }
    return false;
}
}  // namespace
int main() {
    int n = 0;
    auto check = [&](bool ok, const char* label) {
        if (!ok)
            throw std::runtime_error(label);
        std::cout << "PDF_JOB_PASS " << label << '\n';
        ++n;
    };
    try {
        auto r = recognize(snapshot("unicode-text.pdf", 0, {55, 84, 330, 130}, true));
        check(r.code == RecognitionCode::Ready && matches(r, "x^2-3*x+2"),
              "real PDF text + installed OCR gives correct AST candidate");
        r = recognize(snapshot("unicode-text.pdf", 0, {55, 84, 330, 130}));
        check(r.code == RecognitionCode::Ready && matches(r, "x^2-3*x+2"),
              "generic rectangle snapshot extracts its own PDF text on worker");
        check(r.imageContent[2] > 0 && r.imageContent[2] < 1100 && r.imageContent[3] > 0,
              "source comparison bounds enlarge the printed formula without changing snapshot");
        r = recognize(snapshot("formulas-scan.pdf", 3, {55, 84, 310, 160}));
        check(r.code == RecognitionCode::Ready && matches(r, "(x^2-1)/(x-1)"),
              "real scan crop + OCR preserves removable hole");
        r = recognize(snapshot("unicode-text.pdf", 0, {55, 84, 330, 210}, true));
        check(r.code == RecognitionCode::Ready && matches(r, "x^2-3*x+2") && matches(r, "x^3-3*x"),
              "text region has two separate correct formula candidates");
        r = recognize(snapshot("multiple-scan.pdf", 0, {55, 84, 330, 260}));
        check(r.code == RecognitionCode::Ready && matches(r, "x^2-3*x+2") && matches(r, "x^3-3*x"),
              "real multiline scan OCR never concatenates two functions");
        r = recognize(snapshot("oversized.pdf", 0, {55, 84, 330, 150}));
        check(r.code == RecognitionCode::Ready && matches(r, "x^2-3*x+2"),
              "normalized 10x oversized PDF crop uses normalized coordinates");
        r = recognize(snapshot("rotated.pdf", 0, {710, 55, 765, 300}));
        check(r.code == RecognitionCode::Ready && matches(r, "x^2-3*x+2"),
              "90-degree PDF crop recognizes orientation alternatives");
        r = recognize(snapshot("formulas-scan.pdf", 0, {1, 1, 2, 2}));
        check(r.code == RecognitionCode::InvalidRegion, "invalid small region is structured error");
        r = recognize(snapshot("formulas-scan.pdf", 0, {55, 84, 330, 150}), true);
        check(r.code == RecognitionCode::Cancelled, "cancel real job before backend starts, private crop cleaned");
        r = recognize(snapshot("formulas-scan.pdf", 0, {55, 84, 330, 150}), 2);
        check(r.code == RecognitionCode::Cancelled, "cancel real running OCR subprocess and clean private crop");
        r = recognize(snapshot("formulas-scan.pdf", 0, {55, 84, 330, 150}), 0, 1);
        check(r.code == RecognitionCode::Timeout, "real worker deadline emits structured timeout; no mock OCR");
        r = recognize(snapshot("formulas-scan.pdf", 0, {400, 400, 500, 500}));
        check(r.code == RecognitionCode::BackendError, "empty scan reports error and allows next job");
        auto* empty = g_dir_make_tmp("formula-no-model-XXXXXX", nullptr);
        g_setenv("XOURNALPP_FORMULA_MODELS", empty, true);
        r = recognize(snapshot("formulas-scan.pdf", 0, {55, 84, 330, 150}));
        check(r.code == RecognitionCode::MissingModel, "real worker reports missing model without download");
        g_unsetenv("XOURNALPP_FORMULA_MODELS");
        std::filesystem::remove(empty);
        g_free(empty);
        r = recognize(snapshot("formulas-scan.pdf", 0, {55, 84, 330, 150}));
        check(r.code == RecognitionCode::Ready, "retry recovers after model error");
        std::cout << "PDF_JOB_RESULT PASS " << n << " checks\n";
    } catch (const std::exception& e) {
        std::cerr << "PDF_JOB_FAIL " << e.what() << '\n';
        return 1;
    }
}
