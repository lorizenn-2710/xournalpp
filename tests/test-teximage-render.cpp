// SPDX-License-Identifier: GPL-2.0-or-later
// Display/export may read the same TexImage concurrently under Document's shared lock.
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <cairo-pdf.h>
#include <poppler.h>
#include "math/FormulaRender.h"
#include "model/TexImage.h"
#include "view/TexImageView.h"
#include "view/View.h"

static cairo_status_t discard(void*, const unsigned char*, unsigned int) { return CAIRO_STATUS_SUCCESS; }
int main() {
    TexImage image;
    if (!image.loadData(teaching::formulaPdf("y = (x^2-1)/(x-1)"))) return 1;
    image.setText("(x^2-1)/(x-1)");
    std::atomic<unsigned> ready{0};
    std::atomic<bool> start{false}, failed{false};
    std::vector<std::thread> workers;
    for (unsigned i = 0; i < 4; ++i) workers.emplace_back([&, i] {
        ++ready;
        while (!start.load()) std::this_thread::yield();
        for (unsigned n = 0; n < 100; ++n) {
            auto* surface = i % 2 ? cairo_pdf_surface_create_for_stream(discard, nullptr, 180, 100) :
                                   cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 360, 200);
            auto* cr = cairo_create(surface);
            xoj::view::TexImageView(&image).draw(xoj::view::Context::createDefault(cr));
            const auto size = image.getNativeSize();
            if (size.width <= 0 || size.height <= 0 || cairo_status(cr) != CAIRO_STATUS_SUCCESS) failed = true;
            cairo_destroy(cr);
            cairo_surface_finish(surface);
            if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) failed = true;
            cairo_surface_destroy(surface);
        }
    });
    while (ready.load() < 4) std::this_thread::yield();
    start = true;
    for (auto& t: workers) t.join();
    if (failed) return 2;
    auto clone = image.cloneTexImage();
    image.loadData(teaching::formulaPdf("y = sqrt(x+1)"));
    if (clone->getNativeSize().width <= 0 || clone->getText() != "(x^2-1)/(x-1)") return 3;
    xoj::util::GObjectSPtr<PopplerDocument> retained;
    {
        TexImage temporary;
        if (!temporary.loadData(teaching::formulaPdf("y = 1/(x+1)"))) return 4;
        retained = xoj::util::GObjectSPtr<PopplerDocument>(temporary.getPdf(), xoj::util::ref);
    }
    auto* page = poppler_document_get_page(retained.get(), 0);
    auto* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 300, 100);
    auto* cr = cairo_create(surface);
    poppler_page_render(page, cr);
    bool ok = cairo_status(cr) == CAIRO_STATUS_SUCCESS;
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(page);
    if (!ok) return 5;
    std::cout << "TEXIMAGE_RENDER PASS 400 concurrent display/export renders, clone and retained PDF lifetime\n";
}
