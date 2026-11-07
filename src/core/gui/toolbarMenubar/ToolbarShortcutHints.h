#pragma once

#include <string>

#include <gtk/gtk.h>

/** Small, non-interactive captions under existing toolbar controls. */
namespace ToolbarShortcutHints {
/// Create a caption for a control outside a toolbar, using the same style.
GtkWidget* createCaption();
void updateCaption(GtkWidget* label, const std::string& accelerator, bool visible);
/// Attach a hidden caption while preserving the original control and its signals.
void attach(GtkWidget* item, const std::string& id);
/// Refresh from the application's registered accelerators, including action targets.
void update(GtkApplication* app, GtkWidget* item, bool visible);
/// Register a plugin shortcut alongside its toolbar control.
void setAccelerator(GtkWidget* item, const std::string& accelerator);
/// The caption, or nullptr for controls that have not been decorated.
GtkWidget* labelFor(GtkWidget* item);
}  // namespace ToolbarShortcutHints
