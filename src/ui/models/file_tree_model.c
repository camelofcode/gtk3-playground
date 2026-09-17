#include "file_tree_model.h"

struct _FileTreeModel {
    GObject parent_instance;
    GtkTreeStore *store;
};

G_DEFINE_TYPE(
    FileTreeModel,
    file_tree_model,
    G_TYPE_OBJECT
)

static void file_tree_model_dispose(GObject *object) {
    FileTreeModel *self = FILE_TREE_MODEL(object);
    g_clear_object(&self->store);
    G_OBJECT_CLASS(file_tree_model_parent_class)->dispose(object);
}

static void file_tree_model_class_init(FileTreeModelClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = file_tree_model_dispose;
}

static void file_tree_model_init(FileTreeModel *self) {
    self->store = gtk_tree_store_new(
        FILE_TREE_MODEL_N_COLUMNS,
        G_TYPE_ICON,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        FILE_SYSTEM_TYPE_NODE
    );
}

FileTreeModel *file_tree_model_new(void) {
    return g_object_new(FILE_TREE_TYPE_MODEL, NULL);
}

GtkTreeModel *file_tree_model_get_tree_model(FileTreeModel *self) {
    g_return_val_if_fail(FILE_TREE_IS_MODEL(self), NULL);
    return GTK_TREE_MODEL(self->store);
}

void file_tree_model_clear(FileTreeModel *self) {
    g_return_if_fail(FILE_TREE_IS_MODEL(self));
    gtk_tree_store_clear(self->store);
}

static gchar *format_modified_time(gint64 modified_time) {
    if (modified_time <= 0) {
        return g_strdup("");
    }

    g_autoptr(GDateTime) date_time =
        g_date_time_new_from_unix_local(modified_time);
    if (date_time == NULL) {
        return g_strdup("");
    }
    return g_date_time_format(date_time, "%Y-%m-%d %H:%M");
}

void file_tree_model_append(
    FileTreeModel *self,
    FileSystemNode *node,
    GtkTreeIter *parent,
    GtkTreeIter *result
) {
    g_return_if_fail(FILE_TREE_IS_MODEL(self));
    g_return_if_fail(FILE_SYSTEM_IS_NODE(node));
    g_return_if_fail(result != NULL);

    gboolean directory = file_system_node_is_directory(node);
    g_autoptr(GIcon) icon = g_themed_icon_new_with_default_fallbacks(
        directory ? "folder" : "text-x-generic"
    );
    g_autofree gchar *size = directory
        ? g_strdup("")
        : g_format_size_full(
            file_system_node_get_size(node),
            G_FORMAT_SIZE_IEC_UNITS
        );
    g_autofree gchar *file_count = directory
        ? g_strdup_printf(
            "%" G_GUINT64_FORMAT,
            file_system_node_get_file_count(node)
        )
        : g_strdup("");
    g_autofree gchar *modified = format_modified_time(
        file_system_node_get_modified_time(node)
    );

    gtk_tree_store_append(self->store, result, parent);
    gtk_tree_store_set(
        self->store,
        result,
        FILE_TREE_MODEL_COLUMN_ICON,
        icon,
        FILE_TREE_MODEL_COLUMN_NAME,
        file_system_node_get_name(node),
        FILE_TREE_MODEL_COLUMN_SIZE,
        size,
        FILE_TREE_MODEL_COLUMN_FILE_COUNT,
        file_count,
        FILE_TREE_MODEL_COLUMN_MODIFIED,
        modified,
        FILE_TREE_MODEL_COLUMN_FOREGROUND,
        NULL,
        FILE_TREE_MODEL_COLUMN_BACKGROUND,
        NULL,
        FILE_TREE_MODEL_COLUMN_NODE,
        node,
        -1
    );
}

void file_tree_model_set_row_colors(
    FileTreeModel *self,
    GtkTreeIter *iter,
    const gchar *foreground,
    const gchar *background
) {
    g_return_if_fail(FILE_TREE_IS_MODEL(self));
    g_return_if_fail(iter != NULL);

    gtk_tree_store_set(
        self->store,
        iter,
        FILE_TREE_MODEL_COLUMN_FOREGROUND,
        foreground,
        FILE_TREE_MODEL_COLUMN_BACKGROUND,
        background,
        -1
    );
}
