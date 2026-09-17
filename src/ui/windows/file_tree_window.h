#ifndef FILE_TREE_WINDOW_H
#define FILE_TREE_WINDOW_H

#include <gtk/gtk.h>

static const char FILE_TREE_WINDOW_STATE_KEY[] = "file-tree-window-state";

void file_tree_window_init(
    GtkBuilder *builder,
    GtkApplication *app
);

#endif
