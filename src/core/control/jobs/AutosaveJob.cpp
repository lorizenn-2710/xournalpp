#include "AutosaveJob.h"

#include <glib.h>  // for g_message, g_warning

#include "control/Control.h"              // for Control
#include "control/jobs/Job.h"             // for JOB_TYPE_AUTOSAVE, JobType
#include "control/xojfile/SaveHandler.h"  // for SaveHandler
#include "model/Document.h"               // for Document
#include "undo/UndoRedoHandler.h"         // for UndoRedoHandler
#include "util/PathUtil.h"                // for clearExtensions, getAutosav...
#include "util/XojMsgBox.h"               // for XojMsgBox
#include "util/i18n.h"                    // for FS, _F

#include "filesystem.h"  // for path

AutosaveJob::AutosaveJob(Control* control): control(control) {}

AutosaveJob::~AutosaveJob() = default;

void AutosaveJob::afterRun() {
    std::string msg = FS(_F("Error while autosaving: {1}") % this->error);
    XojMsgBox::showErrorToUser(control->getGtkWindow(), msg);
}

void AutosaveJob::run() {
    SaveHandler handler;

    auto savedState = control->getUndoRedoHandler()->snapshotState();

    Document* doc = control->getDocument();

    doc->lock_shared();
    auto filepath = doc->getFilepath();
    const auto recoveryId = doc->recoveryId;

    if (filepath.empty()) {
        filepath = Util::getAutosaveFilepath().parent_path() / (recoveryId + ".xopp");
    } else {
        filepath.replace_filename(fs::path(".") += filepath.filename());
    }
    const bool namedNotebook = !doc->getFilepath().empty() && !doc->floatingNotes.empty();
    Util::clearExtensions(filepath);
    if (namedNotebook) filepath += "." + recoveryId;
    filepath += ".autosave.xopp";

    handler.prepareSave(doc, filepath);
    doc->unlock_shared();

    g_message("%s", FS(_F("Autosaving to {1}") % filepath.string()).c_str());

    // SaveHandler closes the gzip stream and atomically replaces this path.
    // There is no intermediate window in which a valid recovery is moved away.
    handler.saveTo(filepath);
    this->error = handler.getErrorMessage();
    if (!this->error.empty()) {
        callAfterRun();
        return;
    }

    doc->lock();
    const bool sameDocument = doc->recoveryId == recoveryId;
    if (sameDocument) handler.updateDocumentInfo(doc);
    doc->unlock();
    if (sameDocument) {
        control->setLastAutosaveFile(filepath);
        control->getUndoRedoHandler()->documentAutosaved(std::move(savedState));
    }
}

auto AutosaveJob::getType() -> JobType { return JOB_TYPE_AUTOSAVE; }
