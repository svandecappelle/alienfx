#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

void set_color(GtkWidget *area_widget, GdkRGBA *color, libusb_device_handle *usbhandle);

typedef struct _KeyboardWidget {
    GtkWidget *widget;
} KeyboardWidget;

struct _KeyboardWidget * get_keyboard(KeyboardWidget *keyboard);

#endif
