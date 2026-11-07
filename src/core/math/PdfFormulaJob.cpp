// SPDX-License-Identifier: GPL-2.0-or-later
#include "PdfFormulaJob.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glib/gstdio.h>
namespace teaching {
PdfFormulaJob::~PdfFormulaJob() {
    if (deadline)
        g_source_remove(deadline);
    if (process)
        g_object_unref(process);
    if (!directory.empty()) {
        g_unlink((directory + "/crop.png").c_str());
        g_rmdir(directory.c_str());
    }
}
void PdfFormulaJob::cancel() {
    cancelled = true;
    std::lock_guard lock(mutex);
    if (process)
        g_subprocess_force_exit(process);
}
std::shared_ptr<PdfFormulaJob> PdfFormulaJob::start(PdfFormulaSnapshot snapshot,
                                                    std::function<void(RecognitionResult)> done, unsigned timeoutMs) {
    auto job = std::shared_ptr<PdfFormulaJob>(new PdfFormulaJob);
    job->snapshot = std::move(snapshot);
    job->done = std::move(done);
    auto* weak = new std::weak_ptr<PdfFormulaJob>(job);
    job->deadline = g_timeout_add_full(
            G_PRIORITY_DEFAULT, std::clamp(timeoutMs, 1u, 60000u),
            +[](gpointer p) -> gboolean {
                if (auto job = static_cast<std::weak_ptr<PdfFormulaJob>*>(p)->lock()) {
                    job->deadline = 0;
                    job->timedOut = true;
                    job->cancel();
                }
                return G_SOURCE_REMOVE;
            },
            weak, +[](gpointer p) { delete static_cast<std::weak_ptr<PdfFormulaJob>*>(p); });
    auto* ref = new std::shared_ptr<PdfFormulaJob>(job);
    auto* task = g_task_new(
            nullptr, nullptr,
            +[](GObject*, GAsyncResult* result, gpointer) {
                auto* task = G_TASK(result);
                auto job = *static_cast<std::shared_ptr<PdfFormulaJob>*>(g_task_get_task_data(task));
                auto* r = static_cast<RecognitionResult*>(g_task_propagate_pointer(task, nullptr));
                if (job->deadline) {
                    g_source_remove(job->deadline);
                    job->deadline = 0;
                }
                if (r) {
                    if (job->timedOut) {
                        r->code = RecognitionCode::Timeout;
                        r->message = "Nhận dạng vượt giới hạn thời gian. Chọn vùng nhỏ hơn rồi thử lại.";
                    }
                    job->done(std::move(*r));
                    delete r;
                }
            },
            nullptr);
    g_task_set_task_data(task, ref, +[](gpointer p) { delete static_cast<std::shared_ptr<PdfFormulaJob>*>(p); });
    g_task_run_in_thread(
            task, +[](GTask* task, gpointer, gpointer data, GCancellable*) {
                auto job = *static_cast<std::shared_ptr<PdfFormulaJob>*>(data);
                auto* result = new RecognitionResult(job->work());
                g_task_return_pointer(task, result, +[](gpointer p) { delete static_cast<RecognitionResult*>(p); });
            });
    g_object_unref(task);
    return job;
}
RecognitionResult PdfFormulaJob::work() {
    RecognitionResult out;
    try {
        const auto& b = snapshot.bounds;
        double w = b.getWidth(), h = b.getHeight();
        if (!snapshot.pdf || !std::isfinite(w) || !std::isfinite(h) || w < 3 || h < 3 || w > 4000 || h > 4000) {
            out.code = RecognitionCode::InvalidRegion;
            out.message = "Vùng PDF không hợp lệ. Chọn lại vùng công thức.";
            return out;
        }
        double scale = std::min({4., 4096. / w, 4096. / h, std::sqrt(8000000. / (w * h))});
        GError* error = nullptr;
        auto* tmp = g_dir_make_tmp("xournalpp-formula-XXXXXX", &error);
        if (!tmp)
            throw std::runtime_error("Không tạo được thư mục tạm riêng.");
        directory = tmp;
        g_free(tmp);
        out.imagePath = directory + "/crop.png";
        auto* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, static_cast<int>(std::ceil(w * scale)),
                                                   static_cast<int>(std::ceil(h * scale)));
        auto* cr = cairo_create(surface);
        cairo_scale(cr, scale, scale);
        cairo_translate(cr, -b.getX(), -b.getY());
        // The PDF abstraction already includes rotation/crop and oversized-page
        // normalization. Render only the PDF background, never ink/highlights.
        g_debug("Formula OCR: snapshot rendering");
        snapshot.pdf->render(cr);
        cairo_destroy(cr);
        g_debug("Formula OCR: snapshot rendered");
        cairo_surface_flush(surface);
        int iw = cairo_image_surface_get_width(surface), ih = cairo_image_surface_get_height(surface);
        int stride = cairo_image_surface_get_stride(surface);
        auto* pixels = cairo_image_surface_get_data(surface);
        int left = iw, right = -1, top = ih, bottom = -1;
        for (int y = 0; y < ih; ++y) {
            auto* row = reinterpret_cast<const uint32_t*>(pixels + y * stride);
            for (int x = 0; x < iw; ++x) {
                auto pixel = row[x];
                if ((pixel & 255) < 245 || ((pixel >> 8) & 255) < 245 || ((pixel >> 16) & 255) < 245) {
                    left = std::min(left, x);
                    right = std::max(right, x);
                    top = std::min(top, y);
                    bottom = std::max(bottom, y);
                }
            }
        }
        if (right >= left && bottom >= top) {
            int pad = std::max(8, static_cast<int>(std::ceil(std::min(iw, ih) * .02)));
            left = std::max(0, left - pad);
            top = std::max(0, top - pad);
            right = std::min(iw - 1, right + pad);
            bottom = std::min(ih - 1, bottom + pad);
            out.imageContent = {left, top, right - left + 1, bottom - top + 1};
        }
        auto status = cairo_surface_write_to_png(surface, out.imagePath.c_str());
        cairo_surface_destroy(surface);
        if (status != CAIRO_STATUS_SUCCESS)
            throw std::runtime_error("Không lấy được ảnh vùng PDF.");
        if (cancelled) {
            out.code = RecognitionCode::Cancelled;
            return out;
        }
        // Generic rectangle selection (Ctrl+Shift+R) supplies geometry, not text.
        // Extract from the immutable PDF snapshot here, off the UI thread.
        auto text = snapshot.text.empty() ?
                            snapshot.pdf->selectText({b.minX, b.minY, b.maxX, b.maxY}, XojPdfPageSelectionStyle::Area) :
                            snapshot.text;
        out.candidates = importFormulaLines(text, "Chữ PDF · kiểm tra bố trí mũ/phân số");
        if (!g_file_test(TEACHING_OCR_PYTHON, G_FILE_TEST_IS_EXECUTABLE) ||
            !g_file_test(TEACHING_OCR_SCRIPT, G_FILE_TEST_IS_REGULAR)) {
            out.code = RecognitionCode::MissingBackend;
            out.message =
                    "Thiếu runtime OCR cục bộ. Chạy setup-formula-ocr.sh đã cài trong share/xournalpp/teaching-tools.";
            return out;
        }
        auto* launcher = g_subprocess_launcher_new(
                static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE));
        g_subprocess_launcher_setenv(launcher, "HF_HUB_OFFLINE", "1", true);
        g_subprocess_launcher_setenv(launcher, "TOKENIZERS_PARALLELISM", "false", true);
        {
            std::lock_guard lock(mutex);
            if (!cancelled)
                process = g_subprocess_launcher_spawn(launcher, &error, TEACHING_OCR_PYTHON, TEACHING_OCR_SCRIPT,
                                                      out.imagePath.c_str(), nullptr);
        }
        g_object_unref(launcher);
        if (!process) {
            if (error)
                g_error_free(error);
            out.code = cancelled ? RecognitionCode::Cancelled : RecognitionCode::MissingBackend;
            out.message = "Không khởi động được OCR cục bộ.";
            return out;
        }
        gchar* stdoutText = nullptr;
        g_debug("Formula OCR: backend started");
        bool ok = g_subprocess_communicate_utf8(process, nullptr, nullptr, &stdoutText, nullptr, &error);
        g_debug("Formula OCR: backend returned");
        if (cancelled) {
            g_free(stdoutText);
            if (error)
                g_error_free(error);
            out.code = RecognitionCode::Cancelled;
            return out;
        }
        if (!ok || !g_subprocess_get_successful(process) || !stdoutText) {
            g_free(stdoutText);
            if (error)
                g_error_free(error);
            throw std::runtime_error("OCR dừng do lỗi hoặc vượt giới hạn CPU/RAM. Chọn vùng nhỏ hơn và thử lại.");
        }
        auto* k = g_key_file_new();
        if (!g_key_file_load_from_data(k, stdoutText, -1, G_KEY_FILE_NONE, nullptr)) {
            g_free(stdoutText);
            g_key_file_unref(k);
            throw std::runtime_error("Phản hồi OCR không hợp lệ.");
        }
        g_free(stdoutText);
        auto str = [&](const char* group, const char* key) {
            auto* v = g_key_file_get_string(k, group, key, nullptr);
            std::string s = v ? v : "";
            g_free(v);
            return s;
        };
        std::string code = str("result", "code");
        out.message = str("result", "message");
        out.warning = str("result", "notes");
        out.seconds = g_key_file_get_double(k, "result", "seconds", nullptr);
        int count = std::clamp(g_key_file_get_integer(k, "result", "count", nullptr), 0, 12);
        // OCR first: PDF extraction can flatten vertical fractions/superscripts.
        // Keep text-layer alternatives explicitly labelled rather than merging lines.
        std::vector<FormulaCandidate> candidates;
        for (int i = 0; i < count; ++i) {
            auto encoded = str(("candidate" + std::to_string(i)).c_str(), "raw64");
            gsize n = 0;
            auto* raw = g_base64_decode(encoded.c_str(), &n);
            if (n <= 2048 && g_utf8_validate(reinterpret_cast<const char*>(raw), n, nullptr)) {
                auto group = "candidate" + std::to_string(i);
                auto candidate = importFormula(std::string(reinterpret_cast<char*>(raw), n),
                                               "OCR dòng " + str(group.c_str(), "line") + " · xoay " +
                                                       str(group.c_str(), "rotation") + "° · tỉ lệ " +
                                                       str(group.c_str(), "scale"));
                auto notes = str("result", "notes");
                if (!notes.empty())
                    candidate.message = notes + "\n" + candidate.message;
                candidates.push_back(std::move(candidate));
            }
            g_free(raw);
        }
        // Text with explicit superscript encoding/caret preserves exponent
        // semantics. Prefer it over OCR while retaining both readings for review.
        for (auto& text: out.candidates) {
            bool explicitPower = text.raw.find('^') != std::string::npos || text.raw.find("²") != std::string::npos ||
                                 text.raw.find("³") != std::string::npos;
            if (text.usable() && explicitPower)
                candidates.insert(candidates.begin(), text);
            else
                candidates.push_back(text);
        }
        out.candidates = std::move(candidates);
        out.code = code == "ok"              ? RecognitionCode::Ready :
                   code == "missing-model"   ? RecognitionCode::MissingModel :
                   code == "missing-backend" ? RecognitionCode::MissingBackend :
                                               RecognitionCode::BackendError;
        g_key_file_unref(k);
        if (out.code == RecognitionCode::Ready)
            out.message = "Đã nhận dạng cục bộ. Chọn từng ứng viên, đối chiếu ảnh và sửa trước khi xác nhận.";
    } catch (const std::exception& e) {
        out.code = RecognitionCode::BackendError;
        out.message = e.what();
    }
    return out;
}
}  // namespace teaching
