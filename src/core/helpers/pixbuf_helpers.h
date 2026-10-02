#ifndef PIXBUF_HELPERS_H
#define PIXBUF_HELPERS_H

#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

/* Deserializes response_body as a GdkPixbuf. pixbuf_type must be
 * GDK_TYPE_PIXBUF or one of its subtypes.
 */
GObject *pixbuf_helpers_deserialize_response(
    GBytes *response_body,
    GType pixbuf_type,
    GError **error
);

G_END_DECLS

#endif
