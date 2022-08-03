#include <gtk/gtk.h>


void draw_selectors (GtkDrawingArea *da,
        cairo_t        *cr,
        int             width,
        int             height,
        gpointer        data) {
    window_width = width;
    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(da));

}
