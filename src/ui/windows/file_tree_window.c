#include "file_tree_window.h"
#include "src/ui/widgets/file_tree_view.h"

typedef struct {
    GtkWindow *window;
    GtkWidget *folder_chooser_box;
    GtkEntry *folder_path_entry;
    GtkSpinner *spinner;
    GtkLabel *status_label;
    FileTreeView *file_tree_view;
    gboolean destroyed;
} FileTreeWindowState;

static void unref_signal_data(
    gpointer data,
    GClosure *closure
) {
    (void)closure;
    g_object_unref(data);
}

static void on_file_tree_view_loading_changed(
    FileTreeView *file_tree_view,
    GParamSpec *pspec,
    gpointer user_data
) {
    FileTreeWindowState *state = user_data;
    gtk_widget_set_sensitive(
        state->folder_chooser_box,
        !file_tree_view_get_loading(file_tree_view)
    );
    gtk_label_set_text(
        state->status_label,
        file_tree_view_get_status_message(file_tree_view)
    );
    if (file_tree_view_get_loading(file_tree_view)) {
        gtk_widget_show(GTK_WIDGET(state->spinner));
        gtk_spinner_start(state->spinner);
    } else {
        gtk_spinner_stop(state->spinner);
        gtk_widget_hide(GTK_WIDGET(state->spinner));
    }
    (void)pspec;
}

static void on_folder_dialog_response(
    GtkNativeDialog *dialog,
    gint response_id,
    gpointer user_data
) {
    GtkWindow *window = GTK_WINDOW(user_data);
    FileTreeWindowState *state = g_object_get_data(
        G_OBJECT(window),
        FILE_TREE_WINDOW_STATE_KEY
    );

    if (
        !state->destroyed &&
        (
            response_id == GTK_RESPONSE_ACCEPT ||
            response_id == GTK_RESPONSE_OK
        )
    ) {
        g_autofree gchar *path = gtk_file_chooser_get_filename(
            GTK_FILE_CHOOSER(dialog)
        );

        if (path != NULL) {
            gtk_entry_set_text(state->folder_path_entry, path);
            gtk_editable_set_position(
                GTK_EDITABLE(state->folder_path_entry),
                -1
            );
            file_tree_view_load_path(state->file_tree_view, path);
        }
    }

    g_object_unref(dialog);
}

static void on_folder_browse_button_clicked(
    GtkButton *button,
    gpointer user_data
) {
    FileTreeWindowState *state = user_data;
    GtkFileChooserNative *dialog = gtk_file_chooser_native_new(
        "Select Folder",
        state->window,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Select",
        "_Cancel"
    );

    const gchar *current_path = gtk_entry_get_text(state->folder_path_entry);
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
        g_object_ref(state->window),
        unref_signal_data,
        0
    );
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
    (void)button;
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

static void on_file_tree_window_destroy(
    GtkWidget *widget,
    gpointer user_data
) {
    FileTreeWindowState *state = user_data;
    state->destroyed = TRUE;
    (void)widget;
}

void file_tree_window_init(
    GtkBuilder *builder,
    GtkApplication *app
) {
    FileTreeWindowState *state = g_new0(FileTreeWindowState, 1);
    state->window = GTK_WINDOW(
        gtk_builder_get_object(builder, "file_tree_window")
    );
    state->folder_chooser_box = GTK_WIDGET(
        gtk_builder_get_object(builder, "folder_chooser_box")
    );
    state->folder_path_entry = GTK_ENTRY(
        gtk_builder_get_object(builder, "folder_path_entry")
    );
    state->spinner = GTK_SPINNER(
        gtk_builder_get_object(builder, "file_tree_spinner")
    );
    state->status_label = GTK_LABEL(
        gtk_builder_get_object(builder, "file_tree_status_label")
    );
    state->file_tree_view = file_tree_view_new();

    GtkBox *container = GTK_BOX(
        gtk_builder_get_object(builder, "file_tree_view_container")
    );
    gtk_box_pack_start(
        container,
        GTK_WIDGET(state->file_tree_view),
        TRUE,
        TRUE,
        0
    );

    g_object_set_data_full(
        G_OBJECT(state->window),
        FILE_TREE_WINDOW_STATE_KEY,
        state,
        g_free
    );
    g_signal_connect(
        state->file_tree_view,
        "notify::loading",
        G_CALLBACK(on_file_tree_view_loading_changed),
        state
    );
    g_signal_connect(
        state->file_tree_view,
        "notify::status-message",
        G_CALLBACK(on_file_tree_view_loading_changed),
        state
    );
    g_signal_connect(
        state->window,
        "delete-event",
        G_CALLBACK(on_file_tree_window_delete_event),
        NULL
    );
    g_signal_connect(
        state->window,
        "destroy",
        G_CALLBACK(on_file_tree_window_destroy),
        state
    );
    g_signal_connect(
        gtk_builder_get_object(builder, "folder_browse_button"),
        "clicked",
        G_CALLBACK(on_folder_browse_button_clicked),
        state
    );

    GtkWindow *main_window = GTK_WINDOW(
        gtk_builder_get_object(builder, "main_window")
    );
    gtk_window_set_application(state->window, app);
    gtk_window_set_transient_for(state->window, main_window);
    gtk_window_set_destroy_with_parent(state->window, TRUE);
}
