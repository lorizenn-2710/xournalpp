#include <cstdio>
#include <string>

#include <gtk/gtk.h>

#include "gui/toolbarMenubar/ToolbarShortcutHints.h"

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);
    int failures = 0;
    auto check = [&](bool ok, const char* what) {
        std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
        failures += !ok;
    };
    GtkApplication* app = gtk_application_new("com.xournalpp.ShortcutHintTest", G_APPLICATION_NON_UNIQUE);
    auto item = [] {
        auto* w = GTK_WIDGET(gtk_tool_item_new());
        g_object_ref_sink(w);
        gtk_container_add(GTK_CONTAINER(w), gtk_toggle_button_new());
        return w;
    };
    auto* rect = item();
    auto* button = gtk_bin_get_child(GTK_BIN(rect));
    gtk_actionable_set_action_name(GTK_ACTIONABLE(button), "win.select-tool");
    gtk_actionable_set_action_target_value(GTK_ACTIONABLE(button), g_variant_new_uint64(6));
    const char* rectangleAccels[] = {"<Control><Shift>r", nullptr};
    const char* handAccels[] = {"<Control><Shift>a", nullptr};
    gtk_application_set_accels_for_action(app, "win.select-tool(uint64 6)", rectangleAccels);
    gtk_application_set_accels_for_action(app, "win.select-tool(uint64 13)", handAccels);
    ToolbarShortcutHints::attach(rect, "SELECT_RECTANGLE");
    auto* label = ToolbarShortcutHints::labelFor(rect);
    ToolbarShortcutHints::update(app, rect, true);
    char* rectangleLabel = gtk_accelerator_get_label(GDK_KEY_r, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK));
    check(label && std::string(gtk_label_get_text(GTK_LABEL(label))) == rectangleLabel,
          "caption respects action target: rectangle is not confused with hand");
    g_free(rectangleLabel);
    check(gtk_widget_get_visible(label), "caption is visible when enabled");
    check(gtk_widget_is_ancestor(button, rect), "original toggle button retained with its action/target");
    ToolbarShortcutHints::update(app, rect, false);
    gtk_widget_show_all(rect);
    check(!gtk_widget_get_visible(label), "show_all does not reveal hidden hints");
    ToolbarShortcutHints::attach(rect, "SELECT_RECTANGLE");
    check(label == ToolbarShortcutHints::labelFor(rect), "repeated attachment does not duplicate the caption");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
    ToolbarShortcutHints::update(app, rect, true);
    check(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)), "showing captions does not change tool state");
    const char* replacement[] = {"<Alt>r", nullptr};
    gtk_application_set_accels_for_action(app, "win.select-tool(uint64 6)", replacement);
    ToolbarShortcutHints::update(app, rect, true);
    check(std::string(gtk_label_get_text(GTK_LABEL(label))) == "Alt+R", "refresh follows changed registered shortcuts");
    auto* plugin = item();
    ToolbarShortcutHints::setAccelerator(plugin, "<Shift>y");
    ToolbarShortcutHints::attach(plugin, "Plugin::SPLIT_PDF_NOTE");
    ToolbarShortcutHints::update(app, plugin, true);
    check(std::string(gtk_label_get_text(GTK_LABEL(ToolbarShortcutHints::labelFor(plugin)))) == "Shift+Y",
          "PDF plus scratch paper plugin shows its own registered shortcut");
    auto* unknown = item();
    ToolbarShortcutHints::attach(unknown, "NO_SHORTCUT");
    ToolbarShortcutHints::update(app, unknown, true);
    check(!gtk_widget_get_visible(ToolbarShortcutHints::labelFor(unknown)),
          "unassigned tool never shows an invented shortcut");
    auto* separator = GTK_WIDGET(gtk_separator_tool_item_new());
    g_object_ref_sink(separator);
    ToolbarShortcutHints::attach(separator, "SEPARATOR");
    check(!ToolbarShortcutHints::labelFor(separator), "separators remain undecorated");
    for (auto* w: {rect, plugin, unknown, separator}) {
        gtk_widget_destroy(w);
        g_object_unref(w);
    }
    g_object_unref(app);
    return failures ? 1 : 0;
}
