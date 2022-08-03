#include <gtk/gtk.h>

GtkWidget* color_selectors_widget_new() {
  GtkFixed *fixed_layout = (GtkFixed *) gtk_fixed_new();

  GtkLabel *label = (GtkLabel *) gtk_label_new("toto");

  gtk_fixed_put(fixed_layout, label, 100, 300);
  
  return fixed_layout;
}
