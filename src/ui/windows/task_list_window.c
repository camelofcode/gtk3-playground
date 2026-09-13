#include "task_list_window.h"

#include "src/core/task_storage.h"
#include "src/ui/error_dialog.h"
#include "src/ui/widgets/task_row.h"

typedef struct {
    GtkEntry *input;
    GtkListBox *list;
    GListStore *tasks;
    gchar *path;
    gboolean writable;
} TaskListWindowData;

static gboolean save_tasks(
    TaskListWindowData *data,
    GError **error
) {
    return !data->writable ||
           task_storage_save(G_LIST_MODEL(data->tasks), data->path, error);
}

/* Application-owned data survives destruction of either window. */
static void free_data(gpointer pointer) {
    TaskListWindowData *data = pointer;
    g_clear_object(&data->tasks);
    g_free(data->path);
    g_free(data);
}

static void on_shutdown(
    GApplication *app,
    TaskListWindowData *data
) {
    (void)app;
    g_autoptr(GError) error = NULL;
    if (!save_tasks(data, &error)) {
        ui_show_error(NULL, "Could not save tasks", error);
    }
}

static void on_task_delete_requested(
    TaskRow *row,
    TaskListWindowData *data
) {
    Task *task = task_row_get_task(row);
    for (guint i = 0; i < g_list_model_get_n_items(G_LIST_MODEL(data->tasks));
         i++) {
        g_autoptr(Task) item =
            g_list_model_get_item(G_LIST_MODEL(data->tasks), i);
        if (item == task) {
            g_list_store_remove(data->tasks, i);
            return;
        }
    }
}

static GtkWidget *create_row(
    gpointer item,
    gpointer user_data
) {
    TaskRow *row = task_row_new(TASK_ITEM(item));
    g_signal_connect(
        row,
        "delete-requested",
        G_CALLBACK(on_task_delete_requested),
        user_data
    );
    gtk_widget_show_all(GTK_WIDGET(row));
    return GTK_WIDGET(row);
}

static void add_task(TaskListWindowData *data) {
    gchar *text;

    text = g_strdup(gtk_entry_get_text(data->input));
    g_strstrip(text);

    if (*text == '\0') {
        g_free(text);
        return;
    }

    g_autoptr(Task) task = task_new(text);
    g_list_store_append(data->tasks, task);
    gtk_entry_set_text(data->input, "");

    g_free(text);
}

static void on_add_task_button_clicked(
    GtkButton *button,
    TaskListWindowData *data
) {
    (void)button;

    add_task(data);
}

static void on_task_input_activate(
    GtkEntry *entry,
    TaskListWindowData *data
) {
    (void)entry;

    add_task(data);
}

static gboolean on_task_list_window_delete_event(
    GtkWidget *widget,
    GdkEvent *event,
    gpointer user_data
) {
    (void)event;
    (void)user_data;

    TaskListWindowData *data =
        g_object_get_data(G_OBJECT(widget), "task-list-window-data");
    g_autoptr(GError) error = NULL;
    if (!save_tasks(data, &error)) {
        ui_show_error(GTK_WINDOW(widget), "Could not save tasks", error);
        return TRUE;
    }
    return gtk_widget_hide_on_delete(widget);
}

void task_list_window_init(
    GtkBuilder *builder,
    GtkApplication *app
) {
    GtkWindow *window;
    GtkWindow *main_window;
    GtkButton *add_button;
    TaskListWindowData *data;

    window = GTK_WINDOW(gtk_builder_get_object(builder, "task_list_window"));
    main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    add_button = GTK_BUTTON(gtk_builder_get_object(builder, "add_task_button"));

    data = g_new0(TaskListWindowData, 1);
    data->input = GTK_ENTRY(gtk_builder_get_object(builder, "task_input"));
    data->list = GTK_LIST_BOX(gtk_builder_get_object(builder, "task_list"));

    data->path = task_storage_default_path();
    g_autoptr(GError) error = NULL;
    data->tasks = task_storage_load(data->path, &error);
    data->writable = data->tasks != NULL;
    if (!data->writable) {
        data->tasks = g_list_store_new(TASK_TYPE_ITEM);
        ui_show_error(
            main_window,
            "Could not load tasks. Editing is disabled to protect the saved "
            "file.",
            error
        );
        gtk_widget_set_sensitive(GTK_WIDGET(data->input), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(add_button), FALSE);
    }
    g_object_set_data_full(G_OBJECT(app), "task-list-data", data, free_data);
    g_object_set_data(G_OBJECT(window), "task-list-window-data", data);
    g_signal_connect(
        window,
        "delete-event",
        G_CALLBACK(on_task_list_window_delete_event),
        NULL
    );
    g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), data);
    gtk_list_box_bind_model(
        data->list,
        G_LIST_MODEL(data->tasks),
        create_row,
        data,
        NULL
    );

    g_signal_connect(
        add_button,
        "clicked",
        G_CALLBACK(on_add_task_button_clicked),
        data
    );
    g_signal_connect(
        data->input,
        "activate",
        G_CALLBACK(on_task_input_activate),
        data
    );

    gtk_window_set_application(window, app);
    gtk_window_set_transient_for(window, main_window);
    gtk_window_set_destroy_with_parent(window, TRUE);
}
