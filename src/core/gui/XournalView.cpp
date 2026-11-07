#include "XournalView.h"

#include <algorithm>  // for max, min
#include <iterator>   // for begin
#include <memory>     // for unique_ptr, make_unique
#include <optional>   // for optional
#include <unordered_set>

#include <gdk/gdk.h>         // for GdkEventKey, GDK_SHIF...
#include <gdk/gdkkeysyms.h>  // for GDK_KEY_Page_Down
#include <glib-object.h>     // for g_object_ref_sink

#include "control/Control.h"                     // for Control
#include "control/PdfCache.h"                    // for PdfCache
#include "control/ScrollHandler.h"               // for ScrollHandler
#include "control/ToolHandler.h"                 // for ToolHandler
#include "control/actions/ActionDatabase.h"      // for ActionDatabase
#include "control/jobs/XournalScheduler.h"       // for XournalScheduler
#include "control/settings/MetadataManager.h"    // for MetadataManager
#include "control/settings/Settings.h"           // for Settings
#include "control/tools/CursorSelectionType.h"   // for CURSOR_SELECTION_NONE
#include "control/tools/EditSelection.h"         // for EditSelection, SelectionFactory
#include "control/tools/ImageHandler.h"          // for ImageHandler
#include "control/zoom/ZoomControl.h"            // for ZoomControl
#include "plugin/PluginController.h"             // for PluginController
#include "gui/MainWindow.h"                      // for MainWindow
#include "gui/PdfFloatingToolbox.h"              // for PdfFloatingToolbox
#include "gui/inputdevices/GeometryToolInputHandler.h"  // for GeometryToolInputHandler
#include "gui/inputdevices/HandRecognition.h"    // for HandRecognition
#include "gui/inputdevices/InputContext.h"       // for InputContext
#include "gui/scroll/ScrollHandling.h"           // for ScrollHandling
#include "gui/toolbarMenubar/ColorToolItem.h"    // for ColorToolItem
#include "gui/toolbarMenubar/ToolMenuHandler.h"  // for ToolMenuHandler
#include "gui/widgets/XournalWidget.h"           // for gtk_xournal_get_layout
#include "model/Document.h"                      // for Document
#include "model/Element.h"                       // for Element, ELEMENT_STROKE
#include "model/Image.h"                         // for Image
#include "model/Layer.h"                         // for Layer
#include "model/PageRef.h"                       // for PageRef
#include "model/Stroke.h"                        // for Stroke, StrokeTool::E...
#include "model/XojPage.h"                       // for XojPage
#include "undo/DeleteUndoAction.h"               // for DeleteUndoAction
#include "undo/GroupUndoAction.h"                // for GroupUndoAction
#include "undo/InsertUndoAction.h"               // for InsertUndoAction
#include "undo/UndoRedoHandler.h"                // for UndoRedoHandler
#include "util/Assert.h"                         // for xoj_assert
#include "util/Point.h"                          // for Point
#include "util/Rectangle.h"                      // for Rectangle
#include "util/Util.h"                           // for npos
#include "util/glib_casts.h"                     // for wrap_v
#include "util/gtk4_helper.h"                    // for gtk_scrolled_window_set_child
#include "util/safe_casts.h"                     // for round_cast
#include "view/DocumentView.h"                   // for DocumentView
#include "view/LayerView.h"                      // for LayerView
#include "view/View.h"                           // for Context

#include "Layout.h"           // for Layout
#include "PageView.h"         // for XojPageView
#include "RepaintHandler.h"   // for RepaintHandler
#include "XournalppCursor.h"  // for XournalppCursor

using xoj::util::Rectangle;

constexpr int REGULAR_MOVE_AMOUNT = 3;
constexpr int SMALL_MOVE_AMOUNT = 1;
constexpr int LARGE_MOVE_AMOUNT = 10;

std::pair<size_t, size_t> XournalView::preloadPageBounds(size_t page, size_t maxPage) {
    const size_t preloadBefore = this->control->getSettings()->getPreloadPagesBefore();
    const size_t preloadAfter = this->control->getSettings()->getPreloadPagesAfter();
    const size_t lower = page > preloadBefore ? page - preloadBefore : 0;
    const size_t upper = std::min(maxPage, page + preloadAfter + 1);
    return {lower, upper};
}

XournalView::XournalView(GtkWidget* parent, Control* control, ScrollHandling* scrollHandling, PageRef floatingPage):
        scrollHandling(scrollHandling), control(control), floatingPage(std::move(floatingPage)) {
    Document* doc = control->getDocument();
    doc->lock_shared();
    if (doc->getPdfPageCount() != 0) {
        this->cache = std::make_unique<PdfCache>(doc->getPdfDocument(), control->getSettings());
    }
    doc->unlock_shared();

    if (!isFloating()) registerListener(control);

    InputContext* inputContext = new InputContext(this, scrollHandling);
    this->widget = gtk_xournal_new(this, inputContext, scrollHandling->getVertical(), scrollHandling->getHorizontal());
    g_object_ref_sink(this->widget);  // take ownership without increasing the ref count

    gtk_container_add(GTK_CONTAINER(parent), this->widget);
    gtk_widget_show(this->widget);


    g_signal_connect(getWidget(), "realize", G_CALLBACK(+[](GtkWidget* widget, gpointer) {
                         // Disable event compression
                         if (gtk_widget_get_realized(widget)) {
                             gdk_window_set_event_compression(gtk_widget_get_window(widget), false);
                         } else {
                             g_warning("could not disable event compression");
                         }
                     }),
                     nullptr);

    this->repaintHandler = std::make_unique<RepaintHandler>(this);

    if (!isFloating()) control->getZoomControl()->addZoomListener(this);

    gtk_widget_set_can_default(this->widget, true);
    if (!isFloating()) gtk_widget_grab_default(this->widget);

    if (!isFloating()) gtk_widget_grab_focus(this->widget);

    if (isFloating()) {
        floatingZoom = this->floatingPage->floatingNote->zoom;
        viewPages.emplace_back(std::make_unique<XojPageView>(this, this->floatingPage));
        layoutPages();
    }
    this->cleanupTimeout = g_timeout_add_seconds(5, xoj::util::wrap_v<clearMemoryTimer>, this);
}

XournalView::~XournalView() {
    g_source_remove(this->cleanupTimeout);

    finishInput();
    clearSelection();
    viewPages.clear();
    gtk_widget_destroy(this->widget);
    this->widget = nullptr;
}


auto XournalView::clearMemoryTimer(XournalView* widget) -> gboolean {
    widget->cleanupBufferCache();
    return G_SOURCE_CONTINUE;
}

auto XournalView::cleanupBufferCache() -> void {
    const auto& [pagesLower, pagesUpper] = this->preloadPageBounds(this->currentPage, this->viewPages.size());
    xoj_assert(pagesLower <= pagesUpper);

    std::unordered_set<size_t> retainedPdfPages;

    for (size_t i = 0; i < this->viewPages.size(); i++) {
        auto&& page = this->viewPages[i];
        const bool isPreload = pagesLower <= i && i < pagesUpper;
        const bool shouldRetain = isPreload || page->isVisible();

        if (shouldRetain) {
            const size_t pdfPageNo = page->getPage()->getPdfPageNr();
            if (pdfPageNo != npos) {
                retainedPdfPages.insert(pdfPageNo);
            }
            continue;
        } else if (page->hasBuffer()) {
            page->deleteViewBuffer();
        }
    }

    if (this->cache) {
        this->cache->evictAllExcept(retainedPdfPages);
    }
}

auto XournalView::getCurrentPage() const -> size_t { return currentPage; }

const int scrollKeySize = 30;

auto XournalView::onKeyPressEvent(const KeyEvent& event) -> bool {
    size_t p = getCurrentPage();
    if (p != npos && p < this->viewPages.size()) {
        auto& v = this->viewPages[p];
        if (v->onKeyPressEvent(event)) {
            return true;
        }
    }

    auto keyval = event.keyval;
    auto state = event.state;
    if (auto* tool = getControl()->getWindow()->getPdfToolbox(); tool->hasSelection()) {
        if ((keyval == GDK_KEY_c && state == GDK_CONTROL_MASK) || keyval == GDK_KEY_Copy) {
            // Shortcut to get selected PDF text.
            tool->copyTextToClipboard();
            return true;
        }
    }

    if (auto* selection = getSelection(); selection) {
        if (keyval == GDK_KEY_Escape) {
            clearSelection();
            return true;
        }

        int d = REGULAR_MOVE_AMOUNT;
        if (state == GDK_MOD1_MASK) {
            d = SMALL_MOVE_AMOUNT;
        } else if (state == GDK_SHIFT_MASK) {
            d = LARGE_MOVE_AMOUNT;
        }

        int xdir = 0;
        int ydir = 0;
        if (keyval == GDK_KEY_Left) {
            xdir = -1;
        } else if (keyval == GDK_KEY_Up) {
            ydir = -1;
        } else if (keyval == GDK_KEY_Right) {
            xdir = 1;
        } else if (keyval == GDK_KEY_Down) {
            ydir = 1;
        }
        if (xdir != 0 || ydir != 0) {
            selection->moveSelection(d * xdir, d * ydir, /*addMoveUndo=*/true);
            selection->ensureWithinVisibleArea();
            return true;
        }
    }

    if (auto* geom = GTK_XOURNAL(widget)->input->getGeometryToolInputHandler(); geom && geom->keyPressed(event)) {
        return true;
    }

    Layout* layout = this->getLayout();

    if (!state) {
        if (keyval == GDK_KEY_Page_Down || keyval == GDK_KEY_KP_Page_Down) {
            control->getScrollHandler()->goToNextPage();
            return true;
        }
        if (keyval == GDK_KEY_Page_Up || keyval == GDK_KEY_KP_Page_Up) {
            control->getScrollHandler()->goToPreviousPage();
            return true;
        }
    }

    if (keyval == GDK_KEY_space) {
        GtkAllocation alloc = {0};
        gtk_widget_get_allocation(gtk_widget_get_parent(this->widget), &alloc);
        int windowHeight = alloc.height - scrollKeySize;

        if (!state) {
            layout->scrollRelative(0, windowHeight);
            return true;
        }
        if (state == GDK_SHIFT_MASK) {
            layout->scrollRelative(0, -windowHeight);
            return true;
        }
    }

    // Numeric keypad always navigates by page
    if (keyval == GDK_KEY_KP_Up) {
        this->pageRelativeXY(0, -1);
        return true;
    }

    if (keyval == GDK_KEY_KP_Down) {
        this->pageRelativeXY(0, 1);
        return true;
    }

    if (keyval == GDK_KEY_KP_Left) {
        this->pageRelativeXY(-1, 0);
        return true;
    }

    if (keyval == GDK_KEY_KP_Right) {
        this->pageRelativeXY(1, 0);
        return true;
    }


    if (keyval == GDK_KEY_Up || keyval == GDK_KEY_k || keyval == GDK_KEY_K) {
        if (control->getSettings()->isPresentationMode()) {
            control->getScrollHandler()->goToPreviousPage();
            return true;
        }

        if (state == GDK_SHIFT_MASK) {
            this->pageRelativeXY(0, -1);
            return true;
        }
        if (!state) {
            layout->scrollRelative(0, -scrollKeySize);
            return true;
        }
    }

    if (keyval == GDK_KEY_Down || keyval == GDK_KEY_j || keyval == GDK_KEY_J) {
        if (control->getSettings()->isPresentationMode()) {
            control->getScrollHandler()->goToNextPage();
            return true;
        }

        if (state == GDK_SHIFT_MASK) {
            this->pageRelativeXY(0, 1);
            return true;
        }
        if (!state) {
            layout->scrollRelative(0, scrollKeySize);
            return true;
        }
    }

    if (keyval == GDK_KEY_Left || keyval == GDK_KEY_h) {
        if (state == GDK_SHIFT_MASK) {
            this->pageRelativeXY(-1, 0);
            return true;
        }
        if (!state) {
            if (control->getSettings()->isPresentationMode()) {
                control->getScrollHandler()->goToPreviousPage();
            } else {
                layout->scrollRelative(-scrollKeySize, 0);
            }
            return true;
        }
    }

    if (keyval == GDK_KEY_Right || keyval == GDK_KEY_l) {
        if (state == GDK_SHIFT_MASK) {
            this->pageRelativeXY(1, 0);
            return true;
        }

        if (!state) {
            if (control->getSettings()->isPresentationMode()) {
                control->getScrollHandler()->goToNextPage();
            } else {
                layout->scrollRelative(scrollKeySize, 0);
            }
            return true;
        }
    }

    if (keyval == GDK_KEY_End || keyval == GDK_KEY_KP_End) {
        control->getScrollHandler()->goToLastPage();
        return true;
    }

    if (keyval == GDK_KEY_Home || keyval == GDK_KEY_KP_Home) {
        control->getScrollHandler()->goToFirstPage();
        return true;
    }

    // Switch color on number key
    auto& colors = control->getWindow()->getToolMenuHandler()->getColorToolItems();
    if (!state && (keyval >= GDK_KEY_0) && (keyval < GDK_KEY_0 + std::min((std::size_t)10, colors.size()))) {
        std::size_t index = std::min(colors.size() - 1, (std::size_t)(9 + (keyval - GDK_KEY_0)) % 10);
        const auto& colorToolItem = colors.at(index);
        if (auto* db = control->getActionDatabase(); db->isActionEnabled(Action::TOOL_COLOR)) {
            control->getActionDatabase()->fireChangeActionState(Action::TOOL_COLOR, colorToolItem->getColor());
        }
        return true;
    }

    // VK640 / Split PDF Note shortcuts:
    // 1. Shift+Y: physical Shift+y produces GDK_KEY_Y (with Shift consumed or in state)
    // 2. Alt+Y: alternate ergonomic trigger immune to IME
    // 3. Ctrl+Y: prevent Redo collision and invoke split note if plugin is active
    if (((keyval == GDK_KEY_Y || ((keyval == GDK_KEY_y || keyval == GDK_KEY_Y) && (state & GDK_SHIFT_MASK))) &&
         !(state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) ||
        ((keyval == GDK_KEY_y || keyval == GDK_KEY_Y) && (state & GDK_MOD1_MASK) && !(state & GDK_CONTROL_MASK))) {
        if (auto* pc = control->getPluginController()) {
            if (pc->executeCallback("vk640SplitPdfNoteShift")) {
                return true;
            }
        }
    }
    if ((keyval == GDK_KEY_y || keyval == GDK_KEY_Y) && (state & GDK_CONTROL_MASK) && !(state & GDK_SHIFT_MASK)) {
        if (auto* pc = control->getPluginController()) {
            if (pc->executeCallback("vk640SplitPdfNote")) {
                return true;
            }
        }
    }

    return false;
}

auto XournalView::getRepaintHandler() const -> RepaintHandler* { return this->repaintHandler.get(); }

auto XournalView::onKeyReleaseEvent(const KeyEvent& event) -> bool {
    size_t p = getCurrentPage();
    if (p != npos && p < this->viewPages.size()) {
        auto& v = this->viewPages[p];
        if (v->onKeyReleaseEvent(event)) {
            return true;
        }
    }

    return false;
}

void XournalView::onSettingsChanged() {
    if (this->cache) {
        this->cache->updateSettings(control->getSettings());
    }
}

// send the focus back to the appropriate widget
void XournalView::requestFocus() { gtk_widget_grab_focus(this->widget); }

auto XournalView::searchTextOnPage(const std::string& text, size_t pageNumber, size_t index, size_t* occurrences,
                                   XojPdfRectangle* matchRect) -> bool {
    if (pageNumber == npos || pageNumber >= this->viewPages.size()) {
        return false;
    }
    auto& v = this->viewPages[pageNumber];

    return v->searchTextOnPage(text, index, occurrences, matchRect);
}

void XournalView::forceUpdatePagenumbers() {
    size_t p = this->currentPage;
    this->currentPage = npos;

    control->firePageSelected(p);
}

auto XournalView::getViewFor(size_t pageNr) const -> XojPageView* {
    if (pageNr == npos || pageNr >= this->viewPages.size()) {
        return nullptr;
    }
    return this->viewPages[pageNr].get();
}

void XournalView::finishInput() {
    if (widget && GTK_XOURNAL(widget)->input) GTK_XOURNAL(widget)->input->finishSequence();
    endSplineAllPages();
    endTextAllPages();
}
void XournalView::setFloatingZoom(double value) {
    if (!isFloating()) return;
    finishInput(); clearSelection();
    floatingZoom = std::clamp(value, .3, 3.);
    {std::unique_lock lock(*control->getDocument());floatingPage->floatingNote->zoom = floatingZoom;}
    layoutPages();
    viewPages[0]->rerenderPage(true);
}
void XournalView::selectPageInView(size_t page) {
    if (isFloating()) return;
    control->firePageSelected(page);
}

void XournalView::pageSelected(size_t page) {
    if (isFloating()) return;
    if (this->currentPage == page && this->lastSelectedPage == page) {
        return;
    }

    control->getWindow()->getPdfToolbox()->userCancelSelection();

    if (this->lastSelectedPage != npos && this->lastSelectedPage < this->viewPages.size()) {
        this->viewPages[this->lastSelectedPage]->setSelected(false);
    }

    endTextAllPages();

    this->currentPage = page;

    size_t pdfPage = npos;

    if (page != npos && page < viewPages.size()) {
        auto& vp = viewPages[page];
        vp->setSelected(true);
        lastSelectedPage = page;
        pdfPage = vp->getPage()->getPdfPageNr();
    }

    control->updatePageNumbers(currentPage, pdfPage);

    control->updateBackgroundSizeButton();
    control->updatePageActions();

    if (control->getSettings()->isEagerPageCleanup()) {
        this->cleanupBufferCache();
    }

    // Load surrounding pages if they are not
    const auto& [pagesLower, pagesUpper] = preloadPageBounds(page, this->viewPages.size());
    xoj_assert(pagesLower <= pagesUpper);
    for (size_t i = pagesLower; i < pagesUpper; i++) {
        if (!this->viewPages[i]->hasBuffer()) {
            this->viewPages[i]->rerenderPage();
        }
    }
}

auto XournalView::getControl() const -> Control* { return control; }

void XournalView::scrollTo(size_t pageNo, XojPdfRectangle rect) {
    double zoom = getZoom();
    if (pageNo >= this->viewPages.size()) {
        return;
    }

    // Make sure it is visible
    Layout* layout = this->getLayout();
    auto p = layout->getPixelCoordinatesOfEntry(pageNo);

    int x = p.x + round_cast<int>(rect.x1 * zoom);
    int y = p.y + round_cast<int>(rect.y1 * zoom);
    int width;
    int height;
    if (rect.x2 == -1 || rect.y2 == -1) {
        auto& v = this->viewPages[pageNo];
        width = v->getDisplayWidth();
        height = v->getDisplayHeight();
    } else {
        width = round_cast<int>((rect.x2 - rect.x1) * zoom);
        height = round_cast<int>((rect.y2 - rect.y1) * zoom);
    }

    layout->ensureRectIsVisible(x, y, width, height);

    // Select the page
    selectPageInView(pageNo);
}


void XournalView::pageRelativeXY(int offCol, int offRow) {
    Layout* layout = this->getLayout();
    auto optionalPageIndex = layout->getPageWithRelativePosition(getCurrentPage(), offCol, offRow);
    if (optionalPageIndex) {
        this->scrollTo(*optionalPageIndex);
    }
}


void XournalView::endTextAllPages(XojPageView* except) const {
    for (auto& v: this->viewPages) {
        if (except != v.get()) {
            v->endText();
        }
    }
}

void XournalView::endLinkAllPages(XojPageView* except) const {
    for (auto& v: this->viewPages) {
        if (except != v.get()) {
            v->endLink();
        }
    }
}

void XournalView::endSplineAllPages() const {
    for (auto& v: this->viewPages) {
        v->endSpline();
    }
}

void XournalView::layerChanged(size_t page) {
    if (page != npos && page < this->viewPages.size()) {
        this->viewPages[page]->rerenderPage();
    }
}

void XournalView::getPasteTarget(double& x, double& y) const {
    size_t pageNo = getCurrentPage();
    if (pageNo == npos) {
        return;
    }

    Rectangle<double>* rect = getVisibleRect(pageNo);

    if (rect) {
        x = rect->x + rect->width / 2;
        y = rect->y + rect->height / 2;
        delete rect;
    }
}

auto XournalView::getVisibleRect(size_t page) const -> Rectangle<double>* {
    if (page == npos || page >= this->viewPages.size()) {
        return nullptr;
    }
    auto& p = this->viewPages[page];

    return getVisibleRect(p.get());
}

auto XournalView::getVisibleRect(const XojPageView* redrawable) const -> Rectangle<double>* {
    if (isFloating() && redrawable) {
        // Auxiliary pages draw directly with a viewport clip. They do not depend
        // on the main render scheduler's visibility flag (e.g. before any scroll).
        const auto pos = redrawable->getPixelPosition();
        const auto zoom = getZoom();
        const Range paper(pos.x, pos.y, pos.x + redrawable->getWidth() * zoom,
                          pos.y + redrawable->getHeight() * zoom);
        const auto visible = paper.intersect(Range(getLayout()->getVisibleRect()));
        if (visible.empty()) return nullptr;
        return new Rectangle<double>((visible.minX - pos.x) / zoom, (visible.minY - pos.y) / zoom,
                                     visible.getWidth() / zoom, visible.getHeight() / zoom);
    }
    return gtk_xournal_get_visible_area(this->widget, redrawable);
}

void XournalView::recreatePdfCache() {
    this->cache.reset();

    Document* doc = control->getDocument();
    doc->lock_shared();
    if (doc->getPdfPageCount() != 0) {
        this->cache = std::make_unique<PdfCache>(doc->getPdfDocument(), control->getSettings());
    }
    doc->unlock_shared();
}

/**
 * @return Helper class for Touch specific fixes
 */
auto XournalView::getHandRecognition() const -> HandRecognition* {
    return GTK_XOURNAL(widget)->input->getHandRecognition();
}

/**
 * @return Scrollbars
 */
auto XournalView::getScrollHandling() const -> ScrollHandling* { return scrollHandling; }

auto XournalView::getWidget() const -> GtkWidget* { return widget; }

void XournalView::ensureRectIsVisible(int x, int y, int width, int height) {
    Layout* layout = this->getLayout();
    layout->ensureRectIsVisible(x, y, width, height);
}

void XournalView::zoomChanged() {
    ZoomControl* zoom = control->getZoomControl();
    this->getLayout()->recomputeCenteringPadding();

    if (zoom->isZoomPresentationMode()) {
        scrollTo(this->getCurrentPage());
    } else if (zoom->isZoomSequenceActive()) {
        auto pos = zoom->getScrollPositionAfterZoom();
        Layout* layout = this->getLayout();
        layout->scrollAbs(pos.x, pos.y);
    }

    // Updates the Eraser's cursor icon in order to make it as big as the erasing area
    control->getCursor()->updateCursor();

    // if we changed the zoom of the page, we should hide the pdf floating toolbox
    // and if user clicked the selection again, the floating toolbox shows again
    control->getWindow()->getPdfToolbox()->hide();

    this->control->getScheduler()->blockRerenderZoom();

    gtk_widget_queue_draw(getWidget());
}

void XournalView::pageSizeChanged(size_t page) {
    layoutPages();
    if (page != npos && page < this->viewPages.size()) {
        this->viewPages[page]->rerenderPage(/* sizeChanged */ true);
    }
}

void XournalView::pageChanged(size_t page) {
    if (page != npos && page < this->viewPages.size()) {
        this->viewPages[page]->rerenderPage();
    }
}

void XournalView::pageDeleted(size_t page) {
    clearLastSelectionRect();
    setPendingImageCapture(false);
    const size_t currentPageNo = control->getCurrentPageNo();

    viewPages.erase(begin(viewPages) + static_cast<long>(page));

    layoutPages();

    if (currentPageNo > page) {
        control->getScrollHandler()->scrollToPage(currentPageNo - 1);
    } else {
        control->getScrollHandler()->scrollToPage(currentPageNo);
    }
}

auto XournalView::getTextEditor() const -> TextEditor* {
    for (auto&& page: viewPages) {
        if (page->getTextEditor()) {
            return page->getTextEditor();
        }
    }

    return nullptr;
}

auto XournalView::getCache() const -> PdfCache* { return this->cache.get(); }

void XournalView::pageInserted(size_t page) {
    Document* doc = control->getDocument();
    doc->lock_shared();
    auto pageView = std::make_unique<XojPageView>(this, doc->getPage(page));
    doc->unlock_shared();

    viewPages.insert(begin(viewPages) + as_signed(page), std::move(pageView));

    layoutPages();
    // check which pages are visible and select the most visible page
    Layout* layout = this->getLayout();
    layout->updateVisibility();
}

auto XournalView::getZoom() const -> double { return isFloating() ? floatingZoom : control->getZoomControl()->getZoom(); }

auto XournalView::getDpiScaleFactor() const -> int { return gtk_widget_get_scale_factor(widget); }

void XournalView::clearSelection() {
    EditSelection* sel = GTK_XOURNAL(widget)->selection;
    GTK_XOURNAL(widget)->selection = nullptr;
    delete sel;

    control->setClipboardHandlerSelection(getSelection());

    getCursor()->setMouseSelectionType(CURSOR_SELECTION_NONE);
    control->getToolHandler()->setSelectionEditTools(false, false, false, false);
}

void XournalView::deleteSelection(EditSelection* sel) {
    if (sel == nullptr) {
        sel = getSelection();
    }

    if (sel) {
        auto undo = std::make_unique<DeleteUndoAction>(sel->getSourcePage(), false);
        sel->fillUndoItem(undo.get());
        control->getUndoRedoHandler()->addUndoAction(std::move(undo));

        clearSelection();

        repaintSelection(true);
    }
}

void XournalView::setSelection(EditSelection* selection) {
    clearSelection();
    GTK_XOURNAL(this->widget)->selection = selection;

    control->setClipboardHandlerSelection(getSelection());

    bool canChangeSize = false;
    bool canChangeColor = false;
    bool canChangeFill = false;
    bool canChangeLineStyle = false;

    for (const Element* e: selection->getElementsView()) {
        switch (e->getType()) {
            case ELEMENT_TEXT:
                canChangeColor = true;
                continue;
            case ELEMENT_STROKE: {
                canChangeSize = true;

                const auto* s = dynamic_cast<const Stroke*>(e);
                if (s->getToolType() == StrokeTool::PEN) {
                    // can change everything, leave loop with break
                    canChangeColor = true;
                    canChangeFill = true;
                    canChangeLineStyle = true;
                    break;
                }
                if (s->getToolType() == StrokeTool::HIGHLIGHTER) {
                    canChangeColor = true;
                    canChangeFill = true;
                }
                continue;
            }
            default:
                continue;
        }

        // leave loop
        break;
    }

    control->getToolHandler()->setSelectionEditTools(canChangeColor, canChangeSize, canChangeFill, canChangeLineStyle);

    repaintSelection();
}

void XournalView::repaintSelection(bool evenWithoutSelection) {
    if (evenWithoutSelection) {
        gtk_widget_queue_draw(this->widget);
        return;
    }

    EditSelection* selection = getSelection();
    if (selection == nullptr) {
        return;
    }

    // repaint always the whole widget
    gtk_widget_queue_draw(this->widget);
}

void XournalView::layoutPages() { this->getLayout()->recalculate(); }

auto XournalView::getDisplayHeight() const -> int {
    GtkAllocation allocation = {0};
    gtk_widget_get_allocation(this->widget, &allocation);
    return allocation.height;
}

auto XournalView::getDisplayWidth() const -> int {
    GtkAllocation allocation = {0};
    gtk_widget_get_allocation(this->widget, &allocation);
    return allocation.width;
}

auto XournalView::isPageVisible(size_t page, int* visibleHeight) const -> bool {
    Rectangle<double>* rect = getVisibleRect(page);
    if (rect) {
        if (visibleHeight) {
            *visibleHeight = round_cast<int>(rect->height);
        }

        delete rect;
        return true;
    }
    if (visibleHeight) {
        *visibleHeight = 0;
    }

    return false;
}

void XournalView::documentChanged(DocumentChangeType type) {
    if (type != DOCUMENT_CHANGE_CLEARED && type != DOCUMENT_CHANGE_COMPLETE) {
        return;
    }

    XournalScheduler* scheduler = this->control->getScheduler();
    scheduler->lock();
    scheduler->removeAllJobs();

    clearSelection();
    clearLastSelectionRect();
    setPendingImageCapture(false);

    recreatePdfCache();

    Document* doc = control->getDocument();
    doc->lock_shared();

    viewPages.clear();
    size_t pagecount = doc->getPageCount();
    viewPages.reserve(pagecount);
    for (size_t i = 0; i < pagecount; i++) {
        viewPages.emplace_back(std::make_unique<XojPageView>(this, doc->getPage(i)));
    }

    doc->unlock_shared();

    layoutPages();
    scrollTo(0);

    scheduler->unlock();
}

auto XournalView::cut() -> bool {
    size_t p = getCurrentPage();
    if (p == npos || p >= viewPages.size()) {
        return false;
    }

    auto& page = viewPages[p];
    return page->cut();
}

auto XournalView::copy() -> bool {
    size_t p = getCurrentPage();
    if (p == npos || p >= viewPages.size()) {
        return false;
    }

    auto& page = viewPages[p];
    return page->copy();
}

auto XournalView::paste() -> bool {
    size_t p = getCurrentPage();
    if (p == npos || p >= viewPages.size()) {
        return false;
    }

    auto& page = viewPages[p];
    return page->paste();
}

auto XournalView::actionDelete() -> bool {
    size_t p = getCurrentPage();
    if (p == npos || p >= viewPages.size()) {
        return false;
    }

    auto& page = viewPages[p];
    return page->actionDelete();
}

auto XournalView::getDocument() const -> Document* { return control->getDocument(); }

auto XournalView::getViewPages() const -> std::vector<std::unique_ptr<XojPageView>> const& { return viewPages; }

auto XournalView::getCursor() const -> XournalppCursor* { return control->getCursor(); }

auto XournalView::getSelection() const -> EditSelection* {
    g_return_val_if_fail(this->widget != nullptr, nullptr);
    g_return_val_if_fail(GTK_IS_XOURNAL(this->widget), nullptr);

    return GTK_XOURNAL(this->widget)->selection;
}

auto XournalView::getLayout() const -> Layout* { return gtk_xournal_get_layout(getWidget()); }

void XournalView::setPendingImageCapture(bool pending) { this->pendingImageCapture = pending; }

auto XournalView::consumePendingImageCapture() -> bool {
    const bool wasPending = this->pendingImageCapture;
    this->pendingImageCapture = false;
    return wasPending;
}

void XournalView::setLastSelectionRect(const PageRef& page, XojPageView* view, const Range& rect) {
    this->lastSelectionPage = page;
    this->lastSelectionView = view;
    this->lastSelectionRect = rect;
}

void XournalView::clearLastSelectionRect() {
    this->lastSelectionPage = nullptr;
    this->lastSelectionView = nullptr;
    this->lastSelectionRect = Range();
}

bool XournalView::peekLastSelectionRect(PageRef& outPage, Range& outRect) const {
    if (!lastSelectionPage || !lastSelectionView || !lastSelectionRect.isValid() ||
        lastSelectionRect.getWidth() <= 2 || lastSelectionRect.getHeight() <= 2) return false;
    outPage = lastSelectionPage;
    outRect = lastSelectionRect;
    return true;
}

auto XournalView::takeLastSelectionRect(PageRef& outPage, XojPageView*& outView, Range& outRect) -> bool {
    if (!this->lastSelectionPage || !this->lastSelectionView || this->lastSelectionRect.empty() ||
        this->lastSelectionRect.getWidth() <= 2.0 || this->lastSelectionRect.getHeight() <= 2.0) {
        clearLastSelectionRect();
        return false;
    }
    outPage = this->lastSelectionPage;
    outView = this->lastSelectionView;
    outRect = this->lastSelectionRect;
    clearLastSelectionRect();
    return true;
}

auto XournalView::capturePageRectAsImage(const PageRef& page, XojPageView* view, const Range& rawBox) -> bool {
    if (!page || !view || rawBox.empty()) {
        return false;
    }
    const double minX = std::clamp(rawBox.minX, 0.0, page->getWidth());
    const double minY = std::clamp(rawBox.minY, 0.0, page->getHeight());
    const double maxX = std::clamp(rawBox.maxX, 0.0, page->getWidth());
    const double maxY = std::clamp(rawBox.maxY, 0.0, page->getHeight());
    if (maxX - minX <= 2.0 || maxY - minY <= 2.0) {
        return false;
    }
    Range box(minX, minY, maxX, maxY);

    // Return any active selection to its layer so its elements are rendered in place
    this->clearSelection();
    this->clearLastSelectionRect();
    this->pendingImageCapture = false;

    const bool isPdf = page->getBackgroundType().isPdfPage() && page->getPdfPageNr() != npos;
    const bool isImgBg = page->getBackgroundType().isImagePage();

    if (!isPdf && !isImgBg) {
        bool hasElement = false;
        for (const Layer* l: page->getLayersView()) {
            if (!l->isVisible()) {
                continue;
            }
            for (const auto& e: l->getElementsView()) {
                if (e->intersectsArea(box.minX, box.minY, box.getWidth(), box.getHeight())) {
                    hasElement = true;
                    break;
                }
            }
            if (hasElement) {
                break;
            }
        }
        if (!hasElement) {
            return false;
        }
    }

    const double dpiFactor = 300.0 / Util::DPI_NORMALIZATION_FACTOR;
    const int pixelWidth = std::max(1, static_cast<int>(std::ceil(box.getWidth() * dpiFactor)));
    const int pixelHeight = std::max(1, static_cast<int>(std::ceil(box.getHeight() * dpiFactor)));

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixelWidth, pixelHeight);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return false;
    }
    cairo_t* cr = cairo_create(surface);
    cairo_scale(cr, dpiFactor, dpiFactor);
    cairo_translate(cr, -box.minX, -box.minY);

    if (isPdf) {
        Document* doc = this->control->getDocument();
        doc->lock_shared();
        XojPdfPageSPtr pdfPage = doc->getPdfPage(page->getPdfPageNr());
        doc->unlock_shared();
        if (pdfPage) {
            pdfPage->render(cr);
        }
        xoj::view::Context context = xoj::view::Context::createDefault(cr);
        for (const Layer* l: page->getLayersView()) {
            if (l->isVisible()) {
                xoj::view::LayerView layerView(l);
                layerView.draw(context);
            }
        }
    } else if (isImgBg) {
        DocumentView docView;
        docView.setPdfCache(this->getCache());
        docView.drawPage(page, cr, /*dontRenderEditingStroke=*/true, xoj::view::BACKGROUND_SHOW_ALL);
    } else {
        xoj::view::Context context = xoj::view::Context::createDefault(cr);
        for (const Layer* l: page->getLayersView()) {
            if (l->isVisible()) {
                xoj::view::LayerView layerView(l);
                layerView.draw(context);
            }
        }
    }
    cairo_destroy(cr);

    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, pixelWidth, pixelHeight);
    cairo_surface_destroy(surface);
    if (pixbuf == nullptr) {
        return false;
    }

    gchar* pngData = nullptr;
    gsize pngLen = 0;
    GError* error = nullptr;
    const bool encoded = gdk_pixbuf_save_to_buffer(pixbuf, &pngData, &pngLen, "png", &error, nullptr);
    g_object_unref(pixbuf);
    if (!encoded) {
        g_clear_error(&error);
        return false;
    }

    auto image = std::make_unique<Image>();
    {
        std::string data(pngData, pngLen);
        g_free(pngData);
        image->setImage(std::move(data));
    }
    if (auto err = image->renderBuffer(); err.has_value()) {
        return false;
    }

    auto* destination = control->getEditingView();
    if (destination != this && destination->isFloating()) {
        control->clipboardPaste(std::move(image));
        return true;
    }
    ImageHandler::automaticScaling(*image, {box.minX, box.minY, box.getWidth(), box.getHeight()}, page);

    Layer* layer = page->getSelectedLayer();
    Image* imagePtr = image.get();
    auto groupUndoAction = std::make_unique<GroupUndoAction>();
    groupUndoAction->addAction(std::make_unique<InsertUndoAction>(page, layer, imagePtr));
    this->control->getUndoRedoHandler()->addUndoAction(std::move(groupUndoAction));

    ToolType activeTool = this->control->getToolHandler()->getToolType();
    if (activeTool == TOOL_HAND || xoj::tool::isPdfSelectionTool(activeTool)) {
        this->control->selectTool(TOOL_SELECT_RECT);
    }

    auto newSelection = SelectionFactory::createFromFloatingElement(this->control, page, layer, view, std::move(image));
    this->setSelection(newSelection.release());
    this->repaintSelection(true);
    return true;
}
