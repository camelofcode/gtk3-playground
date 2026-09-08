#include "text_binding_window.h"

void on_text_binding_reset_clicked(
    GtkButton *button,
    GtkEntry *entry
) {
    (void)button;

    gtk_entry_set_text(entry, "");
}

gboolean on_text_binding_window_delete_event(
    GtkWidget *widget,
    GdkEvent *event,
    gpointer user_data
) {
    (void)event;
    (void)user_data;

    return gtk_widget_hide_on_delete(widget);
}

void text_binding_window_init(
    GtkBuilder *builder,
    GtkApplication *app
) {
    GtkWindow *window;
    GtkWindow *main_window;

    window = GTK_WINDOW(gtk_builder_get_object(builder, "text_binding_window"));
    main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));

    gtk_window_set_application(window, app);
    gtk_window_set_transient_for(window, main_window);
    gtk_window_set_destroy_with_parent(window, TRUE);
}
