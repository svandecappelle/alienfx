#include <gtk/gtk.h>

void draw_mediabar(GtkDrawingArea *da, cairo_t *cr, int width, int height, gpointer data){

    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(da));
    GdkRGBA background_color, color;

    gtk_style_context_get_color(context, &background_color);

    if (data != NULL) {
        char *given_color = (char *)data;
        gdk_rgba_parse(&color, given_color);
    } else {
        gdk_rgba_parse(&color, "rgba(255, 0, 0, 1)");
    }

    gdk_rgba_parse(&background_color, "#292630");

    double x = 8,
           y = 8,
           tw = 4;

    gdk_cairo_set_source_rgba(cr, &color);

    cairo_new_sub_path(cr);
    cairo_arc(cr, x, y, tw, 0, 2 * M_PI);
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_new_sub_path(cr);
    cairo_arc(cr, 3 * x, y, tw, 0, 2 * M_PI);
    cairo_close_path(cr);
    cairo_fill(cr);
}
