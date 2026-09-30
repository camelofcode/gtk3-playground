#ifndef ARTICLE_IMAGE_H
#define ARTICLE_IMAGE_H

#include "src/core/articles_api.h"
#include <gtk/gtk.h>

void article_image_load(
    GtkImage *image,
    ArticlesApi *api,
    const gchar *url,
    gint max_width,
    gint max_height
);

void article_image_cancel(GtkImage *image);

#endif
