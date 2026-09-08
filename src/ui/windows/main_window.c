#include "main_window.h"

void on_open_text_binding_window_button_clicked(
    GtkButton *button,
    GtkWindow *text_binding_window
) {
    (void)button;

    gtk_widget_show_all(GTK_WIDGET(text_binding_window));
    gtk_window_present(text_binding_window);
}

void on_open_task_list_window_button_clicked(
    GtkButton *button,
    GtkWindow *task_list_window
) {
    (void)button;

    gtk_widget_show_all(GTK_WIDGET(task_list_window));
    gtk_window_present(task_list_window);
}

void main_window_init(
    GtkBuilder *builder,
    GtkApplication *app
) {
    GtkWindow *window;

    window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));

    gtk_window_set_application(window, app);
    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_window_present(window);
}
