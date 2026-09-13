#include "gtk_helpers.h"

void ui_toggle_css_class(
    GtkWidget *widget,
    const gchar *class_name,
    gboolean enabled
) {
    g_return_if_fail(GTK_IS_WIDGET(widget));
    g_return_if_fail(class_name != NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    if (enabled) {
        gtk_style_context_add_class(context, class_name);
    } else {
        gtk_style_context_remove_class(context, class_name);
    }
}
