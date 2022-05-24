#include <gtk/gtk.h>

#include "key.h"

GtkWidget * get_keyboard() {
    GtkWidget *area = gtk_drawing_area_new();

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 1600);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 800);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_function, NULL, NULL);
    
    return area;
}
