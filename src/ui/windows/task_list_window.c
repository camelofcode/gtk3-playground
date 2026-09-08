#include "task_list_window.h"

#include "src/ui/widgets/task_row.h"

typedef struct {
    GtkEntry *input;
    GtkListBox *list;
} TaskListWindowData;

static void on_task_delete_requested(
    TaskRow *row,
    gpointer user_data
) {
    (void)user_data;

    gtk_widget_destroy(GTK_WIDGET(row));
}

static void add_task(TaskListWindowData *data) {
    TaskRow *row;
    gchar *text;

    text = g_strdup(gtk_entry_get_text(data->input));
    g_strstrip(text);

    if (*text == '\0') {
        g_free(text);
        return;
    }

    row = task_row_new(text);
    g_signal_connect(
        row,
        "delete-requested",
        G_CALLBACK(on_task_delete_requested),
        NULL
    );

    gtk_container_add(GTK_CONTAINER(data->list), GTK_WIDGET(row));
    gtk_widget_show_all(GTK_WIDGET(row));
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

gboolean on_task_list_window_delete_event(
    GtkWidget *widget,
    GdkEvent *event,
    gpointer user_data
) {
    (void)event;
    (void)user_data;

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

    data = g_new(TaskListWindowData, 1);
    data->input = GTK_ENTRY(gtk_builder_get_object(builder, "task_input"));
    data->list = GTK_LIST_BOX(gtk_builder_get_object(builder, "task_list"));

    g_object_set_data_full(
        G_OBJECT(window),
        "task-list-window-data",
        data,
        g_free
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
