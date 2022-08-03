#include <gtk/gtk.h>
#include "layout/color_selector.h"
#include "lib/keyboard.h"
#include "lib/utils/functions.h"

LighteningWidget touchpad;
LighteningWidget keyboard;
LighteningWidget mediabar;


static void activate(GtkApplication *app) {
    GtkWindow *window = (GtkWindow *) gtk_application_window_new(app);
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "AlienFx");
    gtk_window_maximize(GTK_WINDOW(window));

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *all_zones_color_button = gtk_color_button_new();
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), all_zones_color_button);
    gtk_window_set_titlebar(window, header);

    GtkCssProvider *css_provider = NULL;

    css_provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_path(css_provider, "src/resources/css/gtk.css");

    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
            GTK_STYLE_PROVIDER (css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkFixed *content = (GtkFixed *) gtk_fixed_new();
    GtkGrid *grid = (GtkGrid *) gtk_grid_new();
    gtk_fixed_put(content, GTK_WIDGET(grid), 0, 0);
    gtk_window_set_child(window, (GtkWidget *) content);
    
    get_mediabar(&mediabar);
    gtk_grid_attach(grid, mediabar.widget, 0, 0, 2, 1);

    get_keyboard(&keyboard);
    gtk_grid_attach(grid, keyboard.widget, 0, 1, 8, 1);

    get_touchpad(&touchpad);
    gtk_grid_attach(grid, touchpad.widget, 0, 3, 8, 1);

    gtk_fixed_put(content, color_selectors_widget_new(&keyboard), 0, 0);

    // TODO set all zones not only keyboard
    g_signal_connect(all_zones_color_button, "color-set", G_CALLBACK(color_selected), &keyboard);
    g_signal_connect(all_zones_color_button, "color-set", G_CALLBACK(color_selected), &touchpad);
    g_signal_connect(all_zones_color_button, "color-set", G_CALLBACK(color_selected), &mediabar);

   gtk_widget_show((GtkWidget *) window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example",
            G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate",
            G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return (status);
}
