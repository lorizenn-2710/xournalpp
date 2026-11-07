/*
 * Xournal++
 *
 * Handles Undo and Redo
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <map>
#include <mutex>
#include <deque>   // for deque
#include <string>  // for string
#include <vector>  // for vector

#include "model/PageRef.h"  // for PageRef

#include "UndoAction.h"  // for UndoActionPtr

class Control;

class UndoRedoListener {
public:
    virtual void undoRedoChanged() = 0;
    virtual void undoRedoPageChanged(PageRef page) = 0;

    virtual ~UndoRedoListener() = default;
};

class UndoRedoHandler {
public:
    explicit UndoRedoHandler(Control* control);
    virtual ~UndoRedoHandler();

    void undo();
    void redo();

    bool canUndo();
    bool canRedo();

    void addUndoAction(UndoActionPtr action);

    std::string undoDescription();
    std::string redoDescription();

    void clearContents();

    void fireUpdateUndoRedoButtons(const std::vector<PageRef>& pages);
    void addUndoRedoListener(UndoRedoListener* listener);

    bool isChanged();
    bool isPageChanged(const PageRef& page) const;
    bool isChangedAutosave();
    void documentAutosaved();
    std::vector<size_t> snapshotState() const;
    void documentAutosaved(std::vector<size_t> snapshot);
    void documentSaved();

private:
    bool matches(const UndoActionPtr& action) const;
    std::vector<size_t> state() const;
    std::vector<size_t> savedState, autosavedState;
    std::map<PageRef,std::vector<size_t>> savedPageState;
    std::vector<size_t> pageState(const PageRef& page) const;
    size_t nextSerial = 0;
    void clearRedo();
    void printContents();

private:
    mutable std::recursive_mutex historyMutex;
    std::deque<UndoActionPtr> undoList;
    std::deque<UndoActionPtr> redoList;

    UndoAction* savedUndo = nullptr;
    UndoAction* autosavedUndo = nullptr;

    std::vector<UndoRedoListener*> listener;

    Control* control = nullptr;
};
