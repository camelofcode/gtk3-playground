#include "pixbuf_helpers.h"

GObject *pixbuf_helpers_deserialize_response(
    GBytes *response_body,
    GType pixbuf_type,
    GError **error
) {
    g_return_val_if_fail(response_body != NULL, NULL);

    if (!g_type_is_a(pixbuf_type, GDK_TYPE_PIXBUF)) {
        g_set_error_literal(
            error,
            G_IO_ERROR,
            G_IO_ERROR_INVALID_ARGUMENT,
            "Expected a GdkPixbuf response type"
        );
        return NULL;
    }

    g_autoptr(GInputStream) stream =
        g_memory_input_stream_new_from_bytes(response_body);
    return G_OBJECT(gdk_pixbuf_new_from_stream(stream, NULL, error));
}
