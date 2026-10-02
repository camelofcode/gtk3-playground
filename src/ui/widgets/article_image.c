#include "article_image.h"

typedef struct {
    GWeakRef image;
    guint generation;
    gint max_width;
    gint max_height;
} ImageRequest;

typedef struct {
    GCancellable *cancellable;
    guint generation;
} ImageState;

static const gchar IMAGE_STATE_KEY[] = "article-image-state";

static void image_state_free(gpointer pointer) {
    ImageState *state = pointer;
    if (state->cancellable) {
        g_cancellable_cancel(state->cancellable);
    }
    g_clear_object(&state->cancellable);
    g_free(state);
}

static ImageState *get_image_state(GtkImage *image) {
    ImageState *state = g_object_get_data(G_OBJECT(image), IMAGE_STATE_KEY);
    if (!state) {
        state = g_new0(ImageState, 1);
        g_object_set_data_full(
            G_OBJECT(image),
            IMAGE_STATE_KEY,
            state,
            image_state_free
        );
    }
    return state;
}

static void image_request_free(ImageRequest *request) {
    g_weak_ref_clear(&request->image);
    g_free(request);
}

static void image_loaded(
    ArticlesApi *api,
    GdkPixbuf *pixbuf,
    const GError *error,
    gpointer user_data
) {
    (void)api;
    (void)error;

    ImageRequest *request = user_data;

    g_autoptr(GtkImage) image = g_weak_ref_get(&request->image);
    if (!image || gtk_widget_in_destruction(GTK_WIDGET(image))) {
        image_request_free(request);
        return;
    }

    ImageState *state = g_object_get_data(G_OBJECT(image), IMAGE_STATE_KEY);
    if (!state || state->generation != request->generation) {
        image_request_free(request);
        return;
    }

    g_clear_object(&state->cancellable);
    if (pixbuf) {
        gint source_width = gdk_pixbuf_get_width(pixbuf);
        gint source_height = gdk_pixbuf_get_height(pixbuf);
        gdouble scale =
            MIN((gdouble)request->max_width / source_width,
                (gdouble)request->max_height / source_height);
        scale = MIN(scale, 1.0);
        gint display_width = MAX(1, (gint)(source_width * scale));
        gint display_height = MAX(1, (gint)(source_height * scale));
        g_autoptr(GdkPixbuf) scaled = gdk_pixbuf_scale_simple(
            pixbuf,
            display_width,
            display_height,
            GDK_INTERP_BILINEAR
        );
        gtk_image_set_from_pixbuf(image, scaled);
    }

    image_request_free(request);
}

void article_image_load(
    GtkImage *image,
    ArticlesApi *api,
    const gchar *url,
    gint max_width,
    gint max_height
) {
    g_return_if_fail(GTK_IS_IMAGE(image));
    g_return_if_fail(ARTICLES_IS_API(api));
    g_return_if_fail(max_width > 0);
    g_return_if_fail(max_height > 0);

    ImageState *state = get_image_state(image);
    if (state->cancellable) {
        g_cancellable_cancel(state->cancellable);
    }
    g_clear_object(&state->cancellable);
    state->generation++;
    gtk_image_clear(image);

    if (!url || !*url) {
        return;
    }

    state->cancellable = g_cancellable_new();
    ImageRequest *request = g_new0(ImageRequest, 1);
    g_weak_ref_init(&request->image, image);
    request->generation = state->generation;
    request->max_width = max_width;
    request->max_height = max_height;
    articles_api_download_image_async(
        api,
        url,
        state->cancellable,
        image_loaded,
        request
    );
}

void article_image_cancel(GtkImage *image) {
    g_return_if_fail(GTK_IS_IMAGE(image));

    ImageState *state = g_object_get_data(G_OBJECT(image), IMAGE_STATE_KEY);
    if (!state) {
        return;
    }

    if (state->cancellable) {
        g_cancellable_cancel(state->cancellable);
    }
    g_clear_object(&state->cancellable);
    state->generation++;
}
