#include "file_tree_window.h"

static void unref_signal_data(
    gpointer data,
    GClosure *closure
) {
    (void)closure;
    g_object_unref(data);
}

static void on_folder_dialog_response(
    GtkNativeDialog *dialog,
    gint response_id,
    gpointer user_data
) {
    GtkEntry *path_entry = GTK_ENTRY(user_data);

    if (
        response_id == GTK_RESPONSE_ACCEPT ||
        response_id == GTK_RESPONSE_OK
    ) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (path != NULL) {
            gtk_entry_set_text(path_entry, path);
            gtk_editable_set_position(GTK_EDITABLE(path_entry), -1);
            g_free(path);
        }
    }

    g_object_unref(dialog);
}

static void on_folder_browse_button_clicked(
    GtkButton *button,
    gpointer user_data
) {
    GtkEntry *path_entry = GTK_ENTRY(user_data);
    GtkWindow *parent;
    GtkFileChooserNative *dialog;
    const gchar *current_path;

    parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button)));
    dialog = gtk_file_chooser_native_new(
        "Select Folder",
        parent,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Select",
        "_Cancel"
    );

    current_path = gtk_entry_get_text(path_entry);
    if (current_path[0] != '\0') {
        gtk_file_chooser_set_filename(
            GTK_FILE_CHOOSER(dialog),
            current_path
        );
    }

    g_signal_connect_data(
        dialog,
        "response",
        G_CALLBACK(on_folder_dialog_response),
        g_object_ref(path_entry),
        unref_signal_data,
        0
    );

    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static gboolean on_file_tree_window_delete_event(
    GtkWidget *widget,
    GdkEvent *event,
    gpointer user_data
) {
    (void)event;
    (void)user_data;

    return gtk_widget_hide_on_delete(widget);
}

void file_tree_window_init(
    GtkBuilder *builder,
    GtkApplication *app
) {
    GtkWindow *window;
    GtkWindow *main_window;
    GtkEntry *folder_path_entry;
    GtkButton *folder_browse_button;

    window = GTK_WINDOW(gtk_builder_get_object(builder, "file_tree_window"));
    main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    folder_path_entry = GTK_ENTRY(
        gtk_builder_get_object(builder, "folder_path_entry")
    );
    folder_browse_button = GTK_BUTTON(
        gtk_builder_get_object(builder, "folder_browse_button")
    );

    g_signal_connect(
        window,
        "delete-event",
        G_CALLBACK(on_file_tree_window_delete_event),
        NULL
    );
    g_signal_connect(
        folder_browse_button,
        "clicked",
        G_CALLBACK(on_folder_browse_button_clicked),
        folder_path_entry
    );

    gtk_window_set_application(window, app);
    gtk_window_set_transient_for(window, main_window);
    gtk_window_set_destroy_with_parent(window, TRUE);
}
