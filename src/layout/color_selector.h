#include "gtk/gtk.h"
#include "../lib/keyboard.h"

GtkWidget* color_selectors_widget_new(LighteningWidget *keyboard);

void color_selected(GtkWidget *color_picker, gpointer data); 
