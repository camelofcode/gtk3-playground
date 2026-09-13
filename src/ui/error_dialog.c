#include "error_dialog.h"

void ui_show_error(
    GtkWindow *parent,
    const gchar *action,
    const GError *error
) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s",
        action
    );
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog),
        "%s",
        error->message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
