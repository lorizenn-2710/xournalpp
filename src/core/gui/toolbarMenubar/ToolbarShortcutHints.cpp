#include "ToolbarShortcutHints.h"

#include <string>

namespace {
constexpr const char* LABEL = "xopp-shortcut-hint-label";
constexpr const char* ACCEL = "xopp-shortcut-hint-accelerator";

std::string acceleratorFor(GtkApplication* app, GtkWidget* widget) {
    if (const auto* stored = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), ACCEL)); stored && *stored) {
        return stored;
    }
    if (GTK_IS_ACTIONABLE(widget)) {
        const char* action = gtk_actionable_get_action_name(GTK_ACTIONABLE(widget));
        if (action && *action) {
            auto* target = gtk_actionable_get_action_target_value(GTK_ACTIONABLE(widget));
            char* detailed = g_action_print_detailed_name(action, target);
            char** accels = gtk_application_get_accels_for_action(app, detailed);
            std::string result = accels && accels[0] ? accels[0] : "";
            g_strfreev(accels);
            g_free(detailed);
            if (!result.empty()) {
                return result;
            }
        }
    }
    if (GTK_IS_CONTAINER(widget)) {
        GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
        std::string result;
        for (GList* c = children; c && result.empty(); c = c->next) {
            result = acceleratorFor(app, GTK_WIDGET(c->data));
        }
        g_list_free(children);
        return result;
    }
    return "";
}
}  // namespace

void ToolbarShortcutHints::attach(GtkWidget* item, const std::string& id) {
    if (!GTK_IS_TOOL_ITEM(item) || GTK_IS_SEPARATOR_TOOL_ITEM(item) || labelFor(item)) {
        return;
    }
    GtkWidget* child = gtk_bin_get_child(GTK_BIN(item));
    if (!child) {
        return;
    }
    // Wrap the whole control, so combo/dropdown buttons can still replace their icon.
    // Keep a reference while reparenting; labels do not take keyboard focus.
    g_object_ref(child);
    gtk_container_remove(GTK_CONTAINER(item), child);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* label = createCaption();
    gtk_box_pack_start(GTK_BOX(box), child, false, false, 0);
    gtk_box_pack_start(GTK_BOX(box), label, false, false, 0);
    g_object_unref(child);
    gtk_container_add(GTK_CONTAINER(item), box);
    g_object_set_data(G_OBJECT(item), LABEL, label);
    g_object_set_data_full(G_OBJECT(item), "xopp-shortcut-hint-id", g_strdup(id.c_str()), g_free);
    gtk_widget_show_all(box);
}

GtkWidget* ToolbarShortcutHints::createCaption() {
    GtkWidget* label = gtk_label_new("");
    gtk_widget_set_name(label, "toolbar-shortcut-hint");
    gtk_widget_set_can_focus(label, false);
    gtk_widget_set_no_show_all(label, true);
    gtk_widget_set_margin_start(label, 3);
    gtk_widget_set_margin_end(label, 3);
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_size_new(8 * PANGO_SCALE));
    gtk_label_set_attributes(GTK_LABEL(label), attrs);
    pango_attr_list_unref(attrs);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "dim-label");
    return label;
}

void ToolbarShortcutHints::update(GtkApplication* app, GtkWidget* item, bool visible) {
    GtkWidget* label = labelFor(item);
    if (!label) {
        return;
    }
    updateCaption(label, acceleratorFor(app, item), visible);
}

void ToolbarShortcutHints::updateCaption(GtkWidget* label, const std::string& accelerator, bool visible) {
    guint key = 0;
    GdkModifierType modifiers{};
    gtk_accelerator_parse(accelerator.c_str(), &key, &modifiers);
    char* text = key ? gtk_accelerator_get_label(key, modifiers) : g_strdup("");
    gtk_label_set_text(GTK_LABEL(label), text);
    gtk_widget_set_visible(label, visible && *text);
    g_free(text);
}

void ToolbarShortcutHints::setAccelerator(GtkWidget* item, const std::string& accelerator) {
    g_object_set_data_full(G_OBJECT(item), ACCEL, g_strdup(accelerator.c_str()), g_free);
}

GtkWidget* ToolbarShortcutHints::labelFor(GtkWidget* item) {
    return GTK_WIDGET(g_object_get_data(G_OBJECT(item), LABEL));
}
