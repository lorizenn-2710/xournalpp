#include "CrashHandler.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "control/Control.h"
#include "model/Document.h"
#include "control/xojfile/SaveHandler.h"  // for SaveHandler
#include "util/PathUtil.h"
#include "util/Stacktrace.h"
#include "util/VersionInfo.h"

#include "filesystem.h"  // for path

static std::atomic<const Document*> document = nullptr;
static std::atomic<Control*> emergencyControl = nullptr;
static std::atomic<int> alreadyCrashed = 0;

void setEmergencyControl(Control* ctrl) { emergencyControl = ctrl; }

extern "C" void forceClose(int sig) {
    g_warning("Force close requested with signal %i", sig);
    Control* ctrl = emergencyControl.load();
    if (!ctrl || !ctrl->saveSessionOnQuit(true)) {
        emergencySave();
    }
    exit(0);
}

void setEmergencyDocument(const Document* doc) { document = doc; }

void emergencySave() {
    const auto* doc = document.load();
    if (!doc) {
        return;
    }

    std::cerr << "Trying to emergency save the current open document..." << std::endl;

    const auto filepath = Util::getConfigFolder() / ("emergencysave-" + doc->recoveryId + ".xopp");

    SaveHandler handler;
    handler.prepareSave(doc, filepath);
    handler.saveTo(filepath);

    if (!handler.getErrorMessage().empty()) {
        std::cerr << "Error: " << handler.getErrorMessage() << std::endl;
    } else {
        std::cerr << "Successfully saved document to \"" << char_cast(filepath.u8string()) << "\"" << std::endl;
    }
}


/// Print a backtrace and try to make an emergency save
extern "C" void crashHandler(int sig) {
    int crash = ++alreadyCrashed;

    std::cerr << "\n\n\n*************************************************************************\n\n";
    std::cerr << "[Crash Handler] Crashed " << crash << " time(s) with signal " << sig << std::endl;
    if (crash == 1) {  // Avoid a loop in case we crash again on emergencySave()
        std::cerr << xoj::util::getVersionInfo() << std::endl;

        std::cerr << "\nTry to get a stacktrace...\n";

        Stacktrace::printStacktrace(std::cerr);
        std::cerr << std::endl;

        emergencySave();
    }

#ifdef __unix__
    // Forward the signal for the system's default handling - we may get a coredump
    std::signal(sig, SIG_DFL);
    kill(getpid(), sig);
#endif

    exit(crash);
}

void installCrashHandlers() {
    std::signal(SIGTERM, forceClose);
    std::signal(SIGINT, forceClose);
    std::signal(SIGHUP, forceClose);
    std::signal(SIGSEGV, crashHandler);
    std::signal(SIGFPE, crashHandler);
    std::signal(SIGILL, crashHandler);
    std::signal(SIGABRT, crashHandler);
#ifdef SIGTRAP
    std::signal(SIGTRAP, crashHandler);
#endif
}
