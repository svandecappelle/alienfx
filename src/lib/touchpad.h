#ifndef TOUCHPAD_H
#define TOUCHPAD_H

#include <gtk/gtk.h>

void draw_touchpad(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);

#endif
