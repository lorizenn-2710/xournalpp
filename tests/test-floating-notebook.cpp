// SPDX-License-Identifier: GPL-2.0-or-later
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <set>

#include <sys/resource.h>

#include "control/FloatingNotebook.h"
#include "control/xojfile/LoadHandler.h"
#include "control/xojfile/SaveHandler.h"
#include "math/FormulaRender.h"
#include "model/Document.h"
#include "model/DocumentHandler.h"
#include "model/Image.h"
#include "model/Stroke.h"
#include "model/TexImage.h"
#include "model/Text.h"
#include "model/XojPage.h"
#include "pdf/base/XojPdfPage.h"
#include "util/Matrix.h"

int main(int argc, char** argv) {
    fs::path out = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "xpp-native-notebook-oracle";
    fs::create_directories(out);
    DocumentHandler events;
    Document doc(&events);
    auto main = std::make_shared<XojPage>(600, 840);
    doc.addPage(main);
    int checks = 0, failed = 0;
    auto check = [&](bool ok, const char* message) {
        ++checks;
        failed += !ok;
        std::printf("%s %s\n", ok ? "PASS" : "FAIL", message);
    };
    std::string formula = teaching::formulaPdf("y = (x^2-1)/(x-1)");
    size_t strokes = 0, points = 0;
    for (int i = 0; i < 100; ++i) {
        auto page = std::make_shared<XojPage>(1200, 1680);  // both dimensions >900 must NOT normalize floating paper
        page->floatingNote = FloatingNoteState{};
        auto& n = *page->floatingNote;
        n.name = "Nháp " + std::to_string(i + 1);
        n.sourceId = main->stableId;
        n.open = i < 2;
        n.pinned = i % 7 == 0;
        n.scrollY = 100 + i;
        n.zoom = .75;
        n.x = i % 4 * 100;
        n.y = i % 4 * 70;
        n.created = i;
        n.modified = i + 100;
        n.zOrder = i;
        page->setBackgroundColor(Color(0xffffff));
        auto* layer = page->getSelectedLayer();
        for (int j = 0; j < 10; ++j) {
            auto stroke = std::make_unique<Stroke>();
            stroke->setWidth(2.5);
            stroke->setColor(Color(0x143e90));
            std::vector<Point> v;
            for (int k = 0; k < 100; ++k)
                v.emplace_back(40 + k * 4, 60 + j * 65 + std::sin(k * .1 + i) * 20, .6 + k * .01);
            stroke->setPointVector(v);
            points += v.size();
            ++strokes;
            layer->addElement(std::move(stroke));
        }
        auto text = std::make_unique<Text>();
        text->setText("Tờ " + std::to_string(i + 1) + " — dữ liệu native");
        text->setFont(XojFont("Sans", 18));
        text->setTransformation(xoj::util::Matrix::TRANSLATION(40, 740));
        layer->addElement(std::move(text));
        auto tex = std::make_unique<TexImage>();
        if (!tex->loadData(std::string(formula)))
            return 2;
        tex->setText("y=\\frac{x^2-1}{x-1}");
        tex->setTransformation(xoj::util::Matrix::TRANSLATION(40, 810));
        layer->addElement(std::move(tex));
        if (i % 10 == 0) {
            auto* pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, 8, 8);
            gdk_pixbuf_fill(pix, 0x50a050ff);
            auto image = std::make_unique<Image>();
            image->setImage(pix);
            g_object_unref(pix);
            image->setTransformation(xoj::util::Matrix::TRANSLATION(40, 900));
            layer->addElement(std::move(image));
        }
        doc.floatingNotes.push_back(page);
    }
    Document few(&events);
    few.addPage(main);
    few.floatingNotes.assign(doc.floatingNotes.begin(), doc.floatingNotes.begin() + 2);
    const auto fewStart = std::chrono::steady_clock::now();
    SaveHandler small;
    small.prepareSave(&few, out / "fixture-2.xopp");
    small.saveTo(out / "fixture-2.xopp");
    const auto fewSaved = std::chrono::steady_clock::now();
    auto fewRead = LoadHandler().loadDocument(out / "fixture-2.xopp");
    const auto fewLoaded = std::chrono::steady_clock::now();
    check(fewRead && fewRead->floatingNotes.size() == 2, "two-sheet native baseline roundtrip");
    std::printf("MEASURE notes=2 save_ms=%.3f load_ms=%.3f\n",
                std::chrono::duration<double, std::milli>(fewSaved - fewStart).count(),
                std::chrono::duration<double, std::milli>(fewLoaded - fewSaved).count());
    auto start = std::chrono::steady_clock::now();
    SaveHandler save;
    save.prepareSave(&doc, out / "fixture-100.xopp");
    save.saveTo(out / "fixture-100.xopp");
    check(save.getErrorMessage().empty(), "native SaveHandler stores 100 note payloads");
    auto saved = std::chrono::steady_clock::now();
    LoadHandler loader;
    auto read = loader.loadDocument(out / "fixture-100.xopp");
    auto loaded = std::chrono::steady_clock::now();
    check(read->getPageCount() == 1 && read->floatingNotes.size() == 100,
          "notes excluded from main page count, native reader restores 100");
    check(read->getPage(0)->stableId == main->stableId, "main source stable identity survives reopen");
    std::set<std::string> ids;
    for (size_t i = 0; i < read->floatingNotes.size(); ++i) {
        auto p = read->floatingNotes[i];
        ids.insert(p->stableId);
        auto& n = *p->floatingNote;
        check(p->stableId == doc.floatingNotes[i]->stableId && n.sourceId == main->stableId &&
                      n.name == "Nháp " + std::to_string(i + 1),
              "per-note identities and UTF8 names roundtrip");
        check(p->getWidth() == 1200 && p->getHeight() == 1680 && n.scrollY == 100 + i && n.zoom == .75 &&
                      n.open == (i < 2) && n.zOrder == i,
              "oversize auxiliary pages exempt from normalization; viewport independent");
        auto elements = p->getSelectedLayer()->getElementsView();
        auto* stroke = dynamic_cast<const Stroke*>(elements[0]);
        check(elements.size() == (i % 10 == 0 ? 13 : 12) && stroke && stroke->hasPressure() &&
                      stroke->getPointVector().size() == 100,
              "pressure stroke, Text, TexImage and images preserved");
    }
    check(ids.size() == 100, "100 independent note IDs");
    auto clone = PageRef(read->floatingNotes[0]->clone());
    auto originalText = dynamic_cast<Text*>(read->floatingNotes[0]->getSelectedLayer()->getElements()[10].get());
    auto cloneText = dynamic_cast<Text*>(clone->getSelectedLayer()->getElements()[10].get());
    cloneText->setText("independent");
    check(clone->stableId != read->floatingNotes[0]->stableId && originalText->getText() != cloneText->getText(),
          "deep clone owns independent text/ink/ID");
    check(floating_notebook::exportPdf(*read, out / "notebook.pdf").empty(),
          "native vector notebook export includes paper outside viewport");
    Document exported(&events);
    check(exported.readPdf(out / "notebook.pdf", false, true) && exported.getPdfPageCount() == 200 &&
                  exported.getPdfPage(0)->getHeight() > 848.12,
          "export keeps boundary fraction together and retains 200 complete PDF pages");
    // Atomic replacement failure: a nonempty directory cannot be replaced by a gzip file.
    fs::create_directories(out / "blocked.xopp");
    std::ofstream(out / "blocked.xopp" / "keep") << "intact";
    SaveHandler fail;
    fail.prepareSave(read.get(), out / "blocked.xopp");
    fail.saveTo(out / "blocked.xopp");
    check(!fail.getErrorMessage().empty() && fs::exists(out / "blocked.xopp" / "keep"),
          "write failure reported and previous destination preserved");
    // Main-page normalization contract is retained independently.
    Document legacy(&events);
    legacy.addPage(std::make_shared<XojPage>(1200, 1680));
    SaveHandler old;
    old.prepareSave(&legacy, out / "legacy.xopp");
    old.saveTo(out / "legacy.xopp");
    auto normalized = LoadHandler().loadDocument(out / "legacy.xopp");
    check(normalized->getPage(0)->getWidth() < 900, "existing main page normalization retained");
    read->setFilepath(out / "fixture-100.xopp");
    check(!read->hasExternalModification(), "loaded file starts with the expected on-disk generation");
    auto same = LoadHandler().loadDocument(out / "fixture-100.xopp");
    auto current = fs::last_write_time(out / "fixture-100.xopp");
    fs::last_write_time(out / "fixture-100.xopp", current + std::chrono::seconds(1));
    check(read->hasExternalModification() && same->hasExternalModification(),
          "external file generation change is detected by both open documents");
    const auto watched = out / "watched.xopp";
    fs::copy_file(out / "fixture-100.xopp", watched, fs::copy_options::overwrite_existing);
    read->setFilepath(watched);
    const auto watchedTime = fs::last_write_time(watched);
    fs::resize_file(watched, fs::file_size(watched) - 1);
    fs::last_write_time(watched, watchedTime);
    check(read->hasExternalModification(), "external replacement with unchanged timestamp is detected by size");
    read->setFilepath(watched);
    fs::remove(watched);
    check(read->hasExternalModification(), "external deletion blocks silent recreation on save");
    read->setFilepath(out / "fixture-100.xopp");
    check(doc.recoveryId != read->recoveryId && read->recoveryId != same->recoveryId,
          "independent document recovery targets even for the same basename");
    fs::create_directories(out / "relocated" / "different-folder");
    const auto portableFile = out / "relocated" / "different-folder" / "fixture-100.xopp";
    fs::copy_file(out / "fixture-100.xopp", portableFile, fs::copy_options::overwrite_existing);
    auto portable = LoadHandler().loadDocument(portableFile);
    check(portable && portable->floatingNotes.size() == 100 &&
                  portable->floatingNotes[72]->stableId == read->floatingNotes[72]->stableId &&
                  portable->floatingNotes[72]->getSelectedLayer()->getElements().size() == 12,
          "relocated xopp with same basename contains its own complete embedded notebook");
    auto sourceId = portable->getPage(0)->stableId;
    portable->insertPage(std::make_shared<XojPage>(600, 840), 0);
    check(portable->getPage(1)->stableId == sourceId && portable->floatingNotes[72]->floatingNote->sourceId == sourceId,
          "insert before source does not change the stable anchor");
    portable->deletePage(1);
    check(portable->floatingNotes[72]->floatingNote->sourceId == sourceId && portable->floatingNotes.size() == 100,
          "deleting source preserves notebook and unresolved anchor");
    const auto previousRecovery = portable->recoveryId;
    portable->clearDocument(true);
    check(portable->recoveryId != previousRecovery && portable->floatingNotes.empty(),
          "replacing a document allocates a new recovery identity");
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    std::printf("MEASURE notes=100 strokes=%zu points=%zu texts=100 tex=100 images=10 save_ms=%.3f load_ms=%.3f "
                "max_rss_kib=%ld\n",
                strokes, points, std::chrono::duration<double, std::milli>(saved - start).count(),
                std::chrono::duration<double, std::milli>(loaded - saved).count(), usage.ru_maxrss);
    std::printf("FLOATING_NOTEBOOK %d checks, %d failures\n", checks, failed);
    return failed ? 1 : 0;
}
