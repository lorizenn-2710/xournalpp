// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtk/gtk.h>

#include "model/DocumentListener.h"
#include "model/PageRef.h"
#include "util/raii/CairoWrappers.h"
class Control;
class MainWindow;
class XournalView;
class ScrollHandling;

// A notebook belongs to Document; this controller owns only its bounded native views and previews.
class FloatingNotes final: public DocumentListener {
public:
    FloatingNotes(MainWindow* window, GtkOverlay* overlay);
    ~FloatingNotes() override;
    XournalView* getActiveView() const;
    XournalView* getViewForPage(const PageRef& page) const;
    void activate(XournalView* view);
    bool handleKey(GdkEventKey* e);
    void toggle();
    PageRef create(const PageRef& clone = {});
    void showLibrary();
    void toggleLibrary();
    void setShortcutHintsVisible(bool visible);
    void open(const PageRef& page);
    void stash(const PageRef& page);
    void refresh();
    void changed(const PageRef& page);
    void flush();
    void prepareSave();
    void setSensitive(bool enabled);
    void rememberPenPosition(GtkWidget* widget, double x, double y);
    void documentChanged(DocumentChangeType type) override;
    void exportNotes(const PageRef& page = {});
    void copySelectionToMain();

private:
    struct Editor;
    struct Thumb {
        size_t revision, used;
        int width;
        bool dark;
        xoj::util::CairoSurfaceSPtr image;
    };
    struct PendingThumb {
        PageRef page;
        size_t revision;
        int width;
        bool dark;
    };
    std::vector<PendingThumb> pendingThumbnails;
    std::vector<PageRef> pendingChanges;
    guint thumbnailSource = 0;
    size_t thumbnailClock = 0;
    bool renderNextThumbnail();
    void doExportNotes(const PageRef& page);
    std::shared_ptr<int> aliveToken = std::make_shared<int>(0);
    MainWindow* window;
    Control* control;
    GtkOverlay* overlay;
    GtkWidget* dock = nullptr;
    std::array<GtkWidget*, 3> dockCaptions{};
    GtkWidget *library = nullptr, *gallery = nullptr, *search = nullptr, *sort = nullptr, *status = nullptr;
    GtkWidget* galleryScroll = nullptr;
    GtkWidget* galleryHits = nullptr;
    std::vector<GtkWidget*> cardButtons;
    void bindVisibleCards();
    std::vector<std::unique_ptr<Editor>> editors;
    std::unordered_map<std::string, Thumb> thumbnails;
    std::vector<PageRef> filtered;
    PageRef selected;
    std::optional<std::pair<double, double>> lastPenPosition;
    XournalView* active = nullptr;
    guint refreshSource = 0, keySnooper = 0, allocationSource = 0;
    int allocatedWidth = 0, allocatedHeight = 0;
    gulong focusOutSignal = 0;
    gulong allocationSignal = 0, keyPressSignal = 0, keyReleaseSignal = 0;
    std::array<bool, 3> shortcutDown{};
    bool closing = false, rebuilding = false;
    std::vector<std::string> hiddenGroup;
    Editor* find(const PageRef& page) const;
    void buildLibrary();
    void arrange(Editor& editor);
    void menu(Editor& editor, GtkWidget* button);
    void lifecycle(const PageRef& page, bool deleted);
    void updateFilter();
    bool drawGallery(cairo_t* cr);
    static gboolean keyPress(GtkWidget*, GdkEventKey*, gpointer);
    static gboolean keyRelease(GtkWidget*, GdkEventKey*, gpointer);
};
