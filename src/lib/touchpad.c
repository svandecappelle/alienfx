#include <gtk/gtk.h>

void draw_touchpad(GtkDrawingArea *da, cairo_t *cr, int width, int height, gpointer data){

    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(da));
    GdkRGBA background_color, color;
    const int w = width / 5;
    const int h = height;

    gtk_style_context_get_color(context, &background_color);

    if (data != NULL) {
        char *given_color = (char *)data;
        gdk_rgba_parse(&color, given_color);
    } else {
        gdk_rgba_parse(&color, "rgba(255, 0, 0, 1)");
    }

    gdk_rgba_parse(&background_color, "#292630");

    double x = w,
           y = 2,
           tw = w + 4,
           th = h - 4;
    double aspect = 1.0;
    double corner_radius = (h - 4) / 30.0;
    double radius = corner_radius / aspect;
    double degrees = M_PI / 180.0;

    gdk_cairo_set_source_rgba(cr, &background_color);

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + tw - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + tw - radius, y + th - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + th - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    gdk_cairo_set_source_rgba(cr, &color);
    
    cairo_set_line_width(cr, 4.0);
    cairo_stroke(cr);
}
