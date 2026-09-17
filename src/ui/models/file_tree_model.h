#ifndef FILE_TREE_MODEL_H
#define FILE_TREE_MODEL_H

#include "src/core/file_system_node.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
    FILE_TREE_MODEL_COLUMN_ICON,
    FILE_TREE_MODEL_COLUMN_NAME,
    FILE_TREE_MODEL_COLUMN_SIZE,
    FILE_TREE_MODEL_COLUMN_FILE_COUNT,
    FILE_TREE_MODEL_COLUMN_MODIFIED,
    FILE_TREE_MODEL_COLUMN_FOREGROUND,
    FILE_TREE_MODEL_COLUMN_BACKGROUND,
    FILE_TREE_MODEL_COLUMN_NODE,
    FILE_TREE_MODEL_N_COLUMNS,
} FileTreeModelColumn;

#define FILE_TREE_TYPE_MODEL file_tree_model_get_type()

G_DECLARE_FINAL_TYPE(
    FileTreeModel,
    file_tree_model,
    FILE_TREE,
    MODEL,
    GObject
)

FileTreeModel *file_tree_model_new(void);
GtkTreeModel *file_tree_model_get_tree_model(FileTreeModel *self);
void file_tree_model_clear(FileTreeModel *self);
void file_tree_model_append(
    FileTreeModel *self,
    FileSystemNode *node,
    GtkTreeIter *parent,
    GtkTreeIter *result
);
void file_tree_model_set_row_colors(
    FileTreeModel *self,
    GtkTreeIter *iter,
    const gchar *foreground,
    const gchar *background
);

G_END_DECLS

#endif
