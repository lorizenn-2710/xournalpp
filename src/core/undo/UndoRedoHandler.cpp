#include "UndoRedoHandler.h"

#include <algorithm>  // for find_if
#include <cinttypes>  // for PRIu64
#include <cstdint>    // for uint64_t
#include <iterator>   // for end, begin
#include <memory>     // for unique_ptr, allocator_traits<>::value_type
#include <utility>    // for move

#include <glib.h>  // for g_message

#include "control/Control.h"  // for Control
#include "model/XojPage.h"
#include "model/Document.h"   // for Document
#include "undo/UndoAction.h"  // for UndoActionPtr, UndoAction
#include "util/Assert.h"      // for xoj_assert
#include "util/XojMsgBox.h"   // for XojMsgBox
#include "util/i18n.h"        // for _, FS, _F

using std::string;


template <typename T>
T* GetPtr(T* ptr) {
    return ptr;
}

template <typename T>
T* GetPtr(const std::unique_ptr<T>& ptr) {
    return ptr.get();
}

template <typename PtrType>
inline void printAction(PtrType& action, size_t n = 666) {
    if (action) {
        g_message("%3zu -- %p / %s", n, GetPtr(action), action->getClassName().c_str());
    } else {
        g_message("%3zu -- (null)", n);
    }
}

template <typename PtrType>
inline void printUndoList(const std::deque<PtrType>& list) {
    size_t n = 1;
    for (const auto& action: list) {
        printAction(action, n++);
    }
}

#ifdef UNDO_TRACE
constexpr bool UNDO_TRACE_BOOL = true;
#else
constexpr bool UNDO_TRACE_BOOL = false;
#endif

void UndoRedoHandler::printContents() {
    std::lock_guard historyLock(historyMutex);
    if constexpr (UNDO_TRACE_BOOL)  // NOLINT
    {
        g_message("redoList");             // NOLINT
        printUndoList(this->redoList);     // NOLINT
        g_message("undoList");             // NOLINT
        printUndoList(this->undoList);     // NOLINT
        g_message("savedUndo");            // NOLINT
        if (this->savedUndo)               // NOLINT
        {                                  // NOLINT
            printAction(this->savedUndo);  // NOLINT
        }                                  // NOLINT
    }
}

UndoRedoHandler::UndoRedoHandler(Control* control): control(control) {
    std::lock_guard historyLock(historyMutex);}

UndoRedoHandler::~UndoRedoHandler() { clearContents(); }

void UndoRedoHandler::clearContents() {
    std::lock_guard historyLock(historyMutex);
#ifdef UNDO_TRACE
    for (auto const& undoAction: this->undoList) {
        g_message("clearContents()::Delete UndoAction: %p / %s", undoAction.get(), undoAction->getClassName().c_str());
    }
#endif  // UNDO_TRACE

    savedState.clear(); autosavedState.clear(); savedPageState.clear();
    undoList.clear();
    redoList.clear();

    this->savedUndo = nullptr;
    this->autosavedUndo = nullptr;

    printContents();
}

void UndoRedoHandler::clearRedo() {
    std::lock_guard historyLock(historyMutex);
#ifdef UNDO_TRACE
    for (auto const& undoAction: this->redoList) {
        g_message("clearRedo()::Delete UndoAction: %p / %s", undoAction.get(), undoAction->getClassName().c_str());
    }
#endif
    std::erase_if(redoList, [this](const auto& a) { return matches(a); });
    printContents();
}

void UndoRedoHandler::undo() {
    std::lock_guard historyLock(historyMutex);
    auto it = std::find_if(undoList.rbegin(), undoList.rend(), [this](auto& a) { return matches(a); });
    if (it == undoList.rend()) return;
    auto action = std::move(*it);
    undoList.erase(std::next(it).base());
    redoList.push_back(std::move(action));
    auto& undoAction = *redoList.back();

    bool undoResult = undoAction.undo(this->control);

    if (!undoResult) {
        string msg = FS(_F("Could not undo \"{1}\"\n"
                           "Something went wrong… Please write a bug report…") %
                        undoAction.getText());
        XojMsgBox::showErrorToUser(control->getGtkWindow(), msg);
    }

    fireUpdateUndoRedoButtons(undoAction.getPages());

    printContents();
}

void UndoRedoHandler::redo() {
    std::lock_guard historyLock(historyMutex);
    auto it = std::find_if(redoList.rbegin(), redoList.rend(), [this](auto& a) { return matches(a); });
    if (it == redoList.rend()) return;
    auto action = std::move(*it);
    redoList.erase(std::next(it).base());
    undoList.push_back(std::move(action));
    auto& redoAction = *undoList.back();

    bool redoResult = redoAction.redo(this->control);

    if (!redoResult) {
        string msg = FS(_F("Could not redo \"{1}\"\n"
                           "Something went wrong… Please write a bug report…") %
                        redoAction.getText());
        XojMsgBox::showErrorToUser(control->getGtkWindow(), msg);
    }

    fireUpdateUndoRedoButtons(redoAction.getPages());

    printContents();
}

auto UndoRedoHandler::canUndo() -> bool {
    std::lock_guard historyLock(historyMutex); return std::any_of(undoList.begin(), undoList.end(), [this](auto& a) { return matches(a); }); }

auto UndoRedoHandler::canRedo() -> bool {
    std::lock_guard historyLock(historyMutex); return std::any_of(redoList.begin(), redoList.end(), [this](auto& a) { return matches(a); }); }

/**
 * Adds an undo Action to the list, or if nullptr does nothing
 */
void UndoRedoHandler::addUndoAction(UndoActionPtr action) {
    std::lock_guard historyLock(historyMutex);
    if (!action) {
        return;
    }

    action->serial = ++nextSerial;
    this->undoList.emplace_back(std::move(action));
    clearRedo();
    fireUpdateUndoRedoButtons(this->undoList.back()->getPages());

    printContents();
}

auto UndoRedoHandler::undoDescription() -> string {
    std::lock_guard historyLock(historyMutex);
    if (auto it = std::find_if(undoList.rbegin(), undoList.rend(), [this](auto& a) { return matches(a); }); it != undoList.rend()) {
        UndoAction& a = **it;
        if (!a.getText().empty()) {
            string txt = _("Undo: ");
            txt += a.getText();
            return txt;
        }
    }
    return _("Undo");
}

auto UndoRedoHandler::redoDescription() -> string {
    std::lock_guard historyLock(historyMutex);
    if (auto it = std::find_if(redoList.rbegin(), redoList.rend(), [this](auto& a) { return matches(a); }); it != redoList.rend()) {
        UndoAction& a = **it;
        if (!a.getText().empty()) {
            string txt = _("Redo: ");
            txt += a.getText();
            return txt;
        }
    }
    return _("Redo");
}

void UndoRedoHandler::fireUpdateUndoRedoButtons(const std::vector<PageRef>& pages) {
    std::lock_guard historyLock(historyMutex);
    for (auto&& undoRedoListener: this->listener) { undoRedoListener->undoRedoChanged(); }

    for (PageRef page: pages) {
        if (!page) {
            continue;
        }

        for (auto&& undoRedoListener: this->listener) { undoRedoListener->undoRedoPageChanged(page); }
    }
}

void UndoRedoHandler::addUndoRedoListener(UndoRedoListener* listener) {
    std::lock_guard historyLock(historyMutex); this->listener.emplace_back(listener); }

bool UndoRedoHandler::matches(const UndoActionPtr& a) const {
    std::lock_guard historyLock(historyMutex);
    if (!control) return true;
    const auto target = control->getEditingPage();
    const bool note = target && target->floatingNote;
    const auto pages = a->getPages();
    // Notebook lifecycle actions remain reachable after the sheet was hidden/deleted.
    if (a->getClassName() == "FloatingNoteUndoAction" && !note) return true;
    if (note) return std::find(pages.begin(), pages.end(), target) != pages.end();
    return std::none_of(pages.begin(), pages.end(), [](auto& p) { return p && p->floatingNote; });
}
std::vector<size_t> UndoRedoHandler::state() const {
    std::lock_guard historyLock(historyMutex);
    std::vector<size_t> s;
    for (auto& a : undoList) s.push_back(a->serial);
    std::sort(s.begin(), s.end());
    return s;
}
auto UndoRedoHandler::isChanged() -> bool {
    std::lock_guard historyLock(historyMutex); return state() != savedState; }
auto UndoRedoHandler::isChangedAutosave() -> bool {
    std::lock_guard historyLock(historyMutex); return state() != autosavedState; }
void UndoRedoHandler::documentAutosaved() {
    std::lock_guard historyLock(historyMutex); autosavedState = state(); }
void UndoRedoHandler::documentSaved() {
    std::lock_guard historyLock(historyMutex); savedState = state();
    savedPageState.clear();
    for(auto& action:undoList)for(auto& p:action->getPages()) if(p && !savedPageState.count(p)) savedPageState[p]=pageState(p);
}

std::vector<size_t> UndoRedoHandler::snapshotState() const { return state(); }
void UndoRedoHandler::documentAutosaved(std::vector<size_t> snapshot) {
    std::lock_guard lock(historyMutex); autosavedState = std::move(snapshot);
}

std::vector<size_t> UndoRedoHandler::pageState(const PageRef& page) const {
    std::lock_guard lock(historyMutex);
    std::vector<size_t> result;
    for(auto& action:undoList) {
        auto pages=action->getPages();
        if(std::find(pages.begin(),pages.end(),page)!=pages.end())result.push_back(action->serial);
    }
    std::sort(result.begin(),result.end());return result;
}
bool UndoRedoHandler::isPageChanged(const PageRef& page) const {
    std::lock_guard lock(historyMutex);
    auto it=savedPageState.find(page);
    return pageState(page)!=(it==savedPageState.end()?std::vector<size_t>{}:it->second);
}
