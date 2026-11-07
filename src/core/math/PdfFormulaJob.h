// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <gio/gio.h>

#include "pdf/base/XojPdfPage.h"
#include "util/Range.h"

#include "FormulaImport.h"
namespace teaching {
enum class RecognitionCode {
    Ready,
    Cancelled,
    Timeout,
    Empty,
    MissingBackend,
    MissingModel,
    BackendError,
    InvalidRegion
};
struct PdfFormulaSnapshot {
    XojPdfPageSPtr pdf;
    Range bounds;
    size_t pdfPage = 0;
    std::string text;
};
struct RecognitionResult {
    RecognitionCode code = RecognitionCode::BackendError;
    std::string message, imagePath, warning;
    std::vector<FormulaCandidate> candidates;
    double seconds = 0;
    // Ink bounds in the full snapshot PNG: used only to enlarge its on-screen
    // comparison. The original crop/data/coordinates are kept unchanged.
    std::array<int, 4> imageContent = {0, 0, 0, 0};
};
class PdfFormulaJob: public std::enable_shared_from_this<PdfFormulaJob> {
public:
    ~PdfFormulaJob();
    void cancel();
    static std::shared_ptr<PdfFormulaJob> start(PdfFormulaSnapshot, std::function<void(RecognitionResult)>,
                                                unsigned timeoutMs = 60000);

private:
    std::atomic_bool cancelled = false, timedOut = false;
    guint deadline = 0;
    std::mutex mutex;
    GSubprocess* process = nullptr;
    std::string directory;
    PdfFormulaSnapshot snapshot;
    std::function<void(RecognitionResult)> done;
    RecognitionResult work();
};
}  // namespace teaching
