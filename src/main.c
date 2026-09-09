#include <gtk/gtk.h>

#include "src/ui/windows/main_window.h"
#include "src/ui/windows/task_list_window.h"
#include "src/ui/windows/text_binding_window.h"

static void load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_css_provider_load_from_resource(
        provider,
        "/com/camelofdoom/gtk3-playground/resources/app.css"
    );

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}

static void activate(
    GtkApplication *app,
    gpointer user_data
) {
    GtkBuilder *builder;
    GError *error = NULL;

    (void)user_data;

    load_css();

    builder = gtk_builder_new();

    if (!gtk_builder_add_from_resource(
            builder,
            "/com/camelofdoom/gtk3-playground/resources/main.ui",
            &error
        )) {
        g_error("Failed to load UI: %s", error->message);
    }

    /*
     * GTK 3 resolves the signal handler names in main.ui from exported
     * symbols in this executable. Meson's export_dynamic option makes those
     * symbols available to GtkBuilder.
     */
    gtk_builder_connect_signals(builder, NULL);

    text_binding_window_init(builder, app);
    main_window_init(builder, app);
    task_list_window_init(builder, app);

    g_object_unref(builder);
}

int main(
    int argc,
    char *argv[]
) {
    GtkApplication *app;
    int status;

    app = gtk_application_new(
        "com.example.Gtk3Playground",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}
