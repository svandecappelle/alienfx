#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

#include "key.h"
#include "keyboard.h"
#include "utils/vars.h"

void set_color(GtkWidget *keyboard_widget, GdkRGBA *color, libusb_device_handle *usbhandle) {
    char *c = gdk_rgba_to_string(color);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(keyboard_widget), draw_function, c, NULL); 

    int r = (int) (color->red * 16);
    int g = (int) (color->green * 16);
    int b = (int) (color->blue * 16);

    set_zone_color(usbhandle, ZONE_KEYBOARD_LEFT, r, g, b);
    set_zone_color(usbhandle, ZONE_KEYBOARD_MIDDLE_LEFT, r, g, b);
    set_zone_color(usbhandle, ZONE_KEYBOARD_MIDDLE_RIGHT, r, g, b);
    set_zone_color(usbhandle, ZONE_KEYBOARD_RIGHT, r, g, b);
}

KeyboardWidget * get_keyboard(KeyboardWidget *keyboard) {
    GtkWidget *area = gtk_drawing_area_new();

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 1600);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 530);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_function, NULL, NULL);

    GdkRGBA c;
    gdk_rgba_parse(&c, "rgba(255, 0, 0, 1)");

    keyboard->widget = area;
    return keyboard;
}
