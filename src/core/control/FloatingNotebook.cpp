// SPDX-License-Identifier: GPL-2.0-or-later
#include "FloatingNotebook.h"

#include <algorithm>
#include <shared_mutex>

#include <cairo-pdf.h>
#include <glib.h>

#include "model/Document.h"
#include "model/Layer.h"
#include "model/XojPage.h"
#include "view/DocumentView.h"
namespace floating_notebook {
std::string exportPdf(Document& document, const fs::path& path, const PageRef& only) {
    auto* uuid = g_uuid_string_random();
    auto temporary = path;
    temporary += std::string(".part-") + uuid;
    g_free(uuid);
    auto* surface = cairo_pdf_surface_create(temporary.string().c_str(), 600, 840);
    auto* cr = cairo_create(surface);
    size_t count = 0;
    {
        std::shared_lock lock(document);
        DocumentView renderer;
        for (auto& page: document.floatingNotes) {
            if ((only && page != only) || page->floatingNote->deleted)
                continue;
            for (double y = 0; y < page->getHeight();) {
                double end = std::min(y + 840., page->getHeight());
                // Keep short native objects (especially fractions/text) together at
                // a paper boundary. Long diagrams still paginate with a bounded clip.
                const double budget = std::min(y + 1050., page->getHeight());
                bool extended;
                do {
                    extended = false;
                    for (auto* layer: page->getLayers())
                        for (const auto& element: layer->getElements()) {
                            const auto bounds = element->getBoundingBox();
                            const double bottom = bounds.y + bounds.height;
                            if (bounds.height <= 210 && bounds.y < end && bottom > end && bottom + 8 <= budget) {
                                end = bottom + 8;
                                extended = true;
                            }
                        }
                } while (extended);
                const double height = end - y;
                cairo_pdf_surface_set_size(surface, page->getWidth(), height);
                cairo_save(cr);
                cairo_rectangle(cr, 0, 0, page->getWidth(), height);
                cairo_clip(cr);
                cairo_translate(cr, 0, -y);
                renderer.drawPage(page, cr, true);
                cairo_restore(cr);
                cairo_show_page(cr);
                ++count;
                y = end;
            }
        }
    }
    cairo_destroy(cr);
    cairo_surface_finish(surface);
    auto status = cairo_surface_status(surface);
    cairo_surface_destroy(surface);
    std::string error = status == CAIRO_STATUS_SUCCESS ? "" : cairo_status_to_string(status);
    if (!count)
        error = "Sổ chưa có tờ để xuất.";
    std::error_code ec;
    if (error.empty()) {
        fs::rename(temporary, path, ec);
        if (ec)
            error = ec.message();
    }
    if (!error.empty())
        fs::remove(temporary, ec);
    return error;
}
}  // namespace floating_notebook
