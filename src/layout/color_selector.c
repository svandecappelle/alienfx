#include <gtk/gtk.h>
#include "../lib/keyboard.h"
#include "../lib/utils/functions.h"

libusb_device_handle *usbhandle;

static ColorZone left_zone;
static ColorZone middle_left_zone;
static ColorZone middle_right_zone;
static ColorZone right_zone;


void color_selected(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    ColorZone zc;
    set_color_zone(color_picker, &zc, -1);
    // each colors are same
    ColorZone * zone[] = {&zc};
    set_color(w, zone, 1, usbhandle);
}

void set_colors_keyboard(LighteningWidget *w) {
    static ColorZone *colors[4];
    colors[0] = &left_zone;
    colors[1] = &middle_left_zone;
    colors[2] = &middle_right_zone;
    colors[3] = &right_zone;

    int nb = sizeof(colors) / sizeof(ColorZone*);
    set_color(w, colors, nb, usbhandle);
}

void set_color_zone(GtkWidget * color_picker, ColorZone * zc, int zone) {
    if (color_picker != NULL) {
        static GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &color);
        char *c = gdk_rgba_to_string(&color);
        zc->color = c;
        zc->rgba = &color;
    }
    zc->zone = zone;
}

void color_selected_left(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    set_color_zone(color_picker, &left_zone, ZONE_LEFT);
    set_colors_keyboard(w);
}

void color_selected_middle_left(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    set_color_zone(color_picker, &middle_left_zone, ZONE_MIDDLE_LEFT);
    set_colors_keyboard(w);
}

void color_selected_middle_right(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    set_color_zone(color_picker, &middle_right_zone, ZONE_MIDDLE_RIGHT);
    set_colors_keyboard(w);
}

void color_selected_right(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    set_color_zone(color_picker, &right_zone, ZONE_RIGHT);
    set_colors_keyboard(w);
}

GtkWidget* color_selectors_widget_new(LighteningWidget *keyboard) {

    usbhandle = connect_usb();
    GtkFixed *fixed_layout = (GtkFixed *) gtk_fixed_new();

    GtkLabel *label = (GtkLabel *) gtk_label_new("toto");

    gtk_fixed_put(fixed_layout, label, 100, 300);
  
    // color selections
    // Keys zones
    GtkWidget *color_picker_left = gtk_color_button_new();
    gtk_fixed_put(fixed_layout, color_picker_left, 100, 100);

    GtkWidget *color_picker_middle_left = gtk_color_button_new();
    gtk_fixed_put(fixed_layout, color_picker_middle_left, 600, 100);

    GtkWidget *color_picker_middle_right = gtk_color_button_new();
    gtk_fixed_put(fixed_layout, color_picker_middle_right, 800, 100);

    GtkWidget *color_picker_right = gtk_color_button_new();
    gtk_fixed_put(fixed_layout, color_picker_right, 1000, 100);

    set_color_zone(NULL, &left_zone, ZONE_LEFT);
    set_color_zone(NULL, &middle_left_zone, ZONE_MIDDLE_LEFT);
    set_color_zone(NULL, &middle_right_zone, ZONE_MIDDLE_RIGHT);
    set_color_zone(NULL, &right_zone, ZONE_RIGHT);

    g_signal_connect(color_picker_left, "color-set", G_CALLBACK(color_selected_left), keyboard);
    g_signal_connect(color_picker_middle_left, "color-set", G_CALLBACK(color_selected_middle_left), keyboard);
    g_signal_connect(color_picker_middle_right, "color-set", G_CALLBACK(color_selected_middle_right), keyboard);
    g_signal_connect(color_picker_right, "color-set", G_CALLBACK(color_selected_right), keyboard);


  return fixed_layout;
}
