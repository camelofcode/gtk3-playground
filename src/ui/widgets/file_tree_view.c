#include "file_tree_view.h"
#include "src/core/file_system_loader.h"
#include "src/ui/models/file_tree_model.h"

#define POPULATE_BATCH_SIZE 200
#define POPULATE_TIME_SLICE_US 8000

enum {
    PROP_0,
    PROP_LOADING,
    PROP_STATUS_MESSAGE,
    N_PROPERTIES,
};

typedef struct {
    FileSystemNode *node;
    gboolean has_parent;
    GtkTreeIter parent;
} PendingRow;

struct _FileTreeView {
    GtkScrolledWindow parent_instance;

    GtkTreeView *tree_view;

    FileTreeModel *model;
    GCancellable *cancellable;
    FileSystemNode *loaded_root;
    GQueue pending_rows;
    guint populate_source_id;
    guint64 rows_added;
    gchar *status_message;
    gboolean loading;
    gboolean destroyed;
};

G_DEFINE_TYPE(
    FileTreeView,
    file_tree_view,
    GTK_TYPE_SCROLLED_WINDOW
)

static GParamSpec *properties[N_PROPERTIES];

static void file_tree_view_get_property(
    GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec
) {
    FileTreeView *self = FILE_TREE_VIEW(object);

    switch (property_id) {
    case PROP_LOADING:
        g_value_set_boolean(value, self->loading);
        break;
    case PROP_STATUS_MESSAGE:
        g_value_set_string(value, self->status_message);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void file_tree_view_set_loading(
    FileTreeView *self,
    gboolean loading,
    const gchar *message
) {
    if (g_strcmp0(self->status_message, message) != 0) {
        g_free(self->status_message);
        self->status_message = g_strdup(message);
        g_object_notify_by_pspec(
            G_OBJECT(self),
            properties[PROP_STATUS_MESSAGE]
        );
    }

    if (self->loading != loading) {
        self->loading = loading;
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_LOADING]);
    }
}

static void file_tree_view_cancel(FileTreeView *self) {
    if (self->cancellable != NULL) {
        g_cancellable_cancel(self->cancellable);
    }
    if (self->populate_source_id != 0) {
        g_source_remove(self->populate_source_id);
        self->populate_source_id = 0;
    }
    g_queue_clear_full(&self->pending_rows, g_free);
    g_clear_object(&self->loaded_root);
}

static void file_tree_view_dispose(GObject *object) {
    FileTreeView *self = FILE_TREE_VIEW(object);
    file_tree_view_cancel(self);
    g_clear_object(&self->cancellable);
    g_clear_object(&self->model);
    G_OBJECT_CLASS(file_tree_view_parent_class)->dispose(object);
}

static void file_tree_view_finalize(GObject *object) {
    FileTreeView *self = FILE_TREE_VIEW(object);
    g_clear_pointer(&self->status_message, g_free);
    G_OBJECT_CLASS(file_tree_view_parent_class)->finalize(object);
}

static void file_tree_view_destroy(GtkWidget *widget) {
    FileTreeView *self = FILE_TREE_VIEW(widget);
    self->destroyed = TRUE;
    file_tree_view_cancel(self);
    GTK_WIDGET_CLASS(file_tree_view_parent_class)->destroy(widget);
}

static void enqueue_node(
    FileTreeView *self,
    FileSystemNode *node,
    GtkTreeIter *parent
) {
    PendingRow *pending = g_new0(PendingRow, 1);
    pending->node = node;
    pending->has_parent = parent != NULL;
    if (parent != NULL) {
        pending->parent = *parent;
    }
    g_queue_push_tail(&self->pending_rows, pending);
}

static gboolean populate_tree_batch(gpointer user_data) {
    FileTreeView *self = FILE_TREE_VIEW(user_data);
    gint64 deadline = g_get_monotonic_time() + POPULATE_TIME_SLICE_US;
    guint rows_in_batch = 0;

    while (
        !g_queue_is_empty(&self->pending_rows) &&
        rows_in_batch < POPULATE_BATCH_SIZE &&
        g_get_monotonic_time() < deadline
    ) {
        g_autofree PendingRow *pending = g_queue_pop_head(
            &self->pending_rows
        );
        GtkTreeIter iter;
        file_tree_model_append(
            self->model,
            pending->node,
            pending->has_parent ? &pending->parent : NULL,
            &iter
        );

        const GPtrArray *children = file_system_node_get_children(
            pending->node
        );
        for (guint i = 0; i < children->len; i++) {
            enqueue_node(self, g_ptr_array_index(children, i), &iter);
        }

        self->rows_added++;
        rows_in_batch++;
    }

    if (!g_queue_is_empty(&self->pending_rows)) {
        return G_SOURCE_CONTINUE;
    }

    self->populate_source_id = 0;
    guint64 file_count = file_system_node_get_file_count(self->loaded_root);
    g_autofree gchar *message = g_strdup_printf(
        "Loaded %" G_GUINT64_FORMAT " files in %" G_GUINT64_FORMAT " rows",
        file_count,
        self->rows_added
    );
    file_tree_view_set_loading(self, FALSE, message);

    GtkTreePath *root_path = gtk_tree_path_new_first();
    gtk_tree_view_expand_row(self->tree_view, root_path, FALSE);
    gtk_tree_path_free(root_path);

    g_clear_object(&self->loaded_root);
    return G_SOURCE_REMOVE;
}

static void on_folder_loaded(
    GObject *source_object,
    GAsyncResult *result,
    gpointer user_data
) {
    (void)source_object;
    FileTreeView *self = FILE_TREE_VIEW(user_data);
    g_autoptr(GError) error = NULL;

    g_clear_object(&self->cancellable);
    self->loaded_root = file_system_loader_load_finish(result, &error);
    if (self->destroyed) {
        g_clear_object(&self->loaded_root);
        g_object_unref(self);
        return;
    }

    if (self->loaded_root == NULL) {
        if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            file_tree_view_set_loading(self, FALSE, "Loading cancelled");
        } else {
            g_autofree gchar *message = g_strdup_printf(
                "Could not load folder: %s",
                error->message
            );
            file_tree_view_set_loading(self, FALSE, message);
        }
        g_object_unref(self);
        return;
    }

    file_tree_view_set_loading(self, TRUE, "Building tree…");
    enqueue_node(self, self->loaded_root, NULL);
    self->populate_source_id = g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        populate_tree_batch,
        g_object_ref(self),
        g_object_unref
    );
    g_object_unref(self);
}

static void append_text_column(
    GtkTreeView *tree_view,
    const gchar *title,
    gint model_column,
    gboolean expand,
    gfloat alignment
) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", alignment, NULL);
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        title,
        renderer,
        "text",
        model_column,
        "foreground",
        FILE_TREE_MODEL_COLUMN_FOREGROUND,
        "cell-background",
        FILE_TREE_MODEL_COLUMN_BACKGROUND,
        NULL
    );
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_expand(column, expand);
    gtk_tree_view_column_set_alignment(column, alignment);
    gtk_tree_view_append_column(tree_view, column);
}

static void configure_tree_view(FileTreeView *self) {
    GtkCellRenderer *icon = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer *name = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *name_column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(name_column, "Name");
    gtk_tree_view_column_pack_start(name_column, icon, FALSE);
    gtk_tree_view_column_pack_start(name_column, name, TRUE);
    gtk_tree_view_column_add_attribute(
        name_column,
        icon,
        "gicon",
        FILE_TREE_MODEL_COLUMN_ICON
    );
    gtk_tree_view_column_add_attribute(
        name_column,
        icon,
        "cell-background",
        FILE_TREE_MODEL_COLUMN_BACKGROUND
    );
    gtk_tree_view_column_add_attribute(
        name_column,
        name,
        "text",
        FILE_TREE_MODEL_COLUMN_NAME
    );
    gtk_tree_view_column_add_attribute(
        name_column,
        name,
        "foreground",
        FILE_TREE_MODEL_COLUMN_FOREGROUND
    );
    gtk_tree_view_column_add_attribute(
        name_column,
        name,
        "cell-background",
        FILE_TREE_MODEL_COLUMN_BACKGROUND
    );
    gtk_tree_view_column_set_expand(name_column, TRUE);
    gtk_tree_view_column_set_resizable(name_column, TRUE);
    gtk_tree_view_append_column(self->tree_view, name_column);

    append_text_column(
        self->tree_view,
        "Size",
        FILE_TREE_MODEL_COLUMN_SIZE,
        FALSE,
        1.0f
    );
    append_text_column(
        self->tree_view,
        "Files",
        FILE_TREE_MODEL_COLUMN_FILE_COUNT,
        FALSE,
        1.0f
    );
    append_text_column(
        self->tree_view,
        "Modified",
        FILE_TREE_MODEL_COLUMN_MODIFIED,
        FALSE,
        0.0f
    );

    gtk_tree_view_set_model(
        self->tree_view,
        file_tree_model_get_tree_model(self->model)
    );
    gtk_tree_view_set_search_column(
        self->tree_view,
        FILE_TREE_MODEL_COLUMN_NAME
    );
}

static void file_tree_view_class_init(FileTreeViewClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    object_class->get_property = file_tree_view_get_property;
    object_class->dispose = file_tree_view_dispose;
    object_class->finalize = file_tree_view_finalize;
    widget_class->destroy = file_tree_view_destroy;

    properties[PROP_LOADING] = g_param_spec_boolean(
        "loading",
        "Loading",
        "Whether the filesystem or tree model is being loaded",
        FALSE,
        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY
    );
    properties[PROP_STATUS_MESSAGE] = g_param_spec_string(
        "status-message",
        "Status message",
        "A human-readable description of the current loading state",
        "Choose a folder to load its contents",
        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY
    );
    g_object_class_install_properties(object_class, N_PROPERTIES, properties);

    gtk_widget_class_set_template_from_resource(
        widget_class,
        "/com/camelofdoom/gtk3-playground/ui/widgets/file-tree-view.ui"
    );
    gtk_widget_class_bind_template_child(widget_class, FileTreeView, tree_view);
}

static void file_tree_view_init(FileTreeView *self) {
    g_queue_init(&self->pending_rows);
    self->status_message = g_strdup("Choose a folder to load its contents");
    gtk_widget_init_template(GTK_WIDGET(self));
    self->model = file_tree_model_new();
    configure_tree_view(self);
}

FileTreeView *file_tree_view_new(void) {
    return g_object_new(FILE_TREE_TYPE_VIEW, NULL);
}

void file_tree_view_load_path(
    FileTreeView *self,
    const gchar *path
) {
    g_return_if_fail(FILE_TREE_IS_VIEW(self));
    g_return_if_fail(path != NULL);
    g_return_if_fail(!self->loading);

    file_tree_model_clear(self->model);
    self->rows_added = 0;
    self->cancellable = g_cancellable_new();
    file_tree_view_set_loading(self, TRUE, "Loading folder…");

    file_system_loader_load_async(
        path,
        self->cancellable,
        on_folder_loaded,
        g_object_ref(self)
    );
}

gboolean file_tree_view_get_loading(FileTreeView *self) {
    g_return_val_if_fail(FILE_TREE_IS_VIEW(self), FALSE);
    return self->loading;
}

const gchar *file_tree_view_get_status_message(FileTreeView *self) {
    g_return_val_if_fail(FILE_TREE_IS_VIEW(self), NULL);
    return self->status_message;
}
