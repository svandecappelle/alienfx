#ifndef LAPTOP_VIEW_H
#define LAPTOP_VIEW_H

#include <gtk/gtk.h>

// Called when the user clicks a lighting zone on the laptop drawing
typedef void (*LaptopViewSelectFunc)(int zone, gpointer user_data);

GtkWidget * laptop_view_new(LaptopViewSelectFunc on_select, gpointer user_data);

// Highlight a zone (LIGHT_ALL highlights the whole laptop, LIGHT_NONE nothing)
void laptop_view_set_selection(GtkWidget *view, int selection);

// Power state (POWER_STATE_*) whose style the power button shows, -1 when unknown
void laptop_view_set_power_state(GtkWidget *view, int state);

#endif
