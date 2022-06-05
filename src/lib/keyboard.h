#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

typedef struct _LighteningWidget {
    GtkWidget *widget;
    char *name;
} LighteningWidget;

struct _LighteningWidget * get_keyboard(LighteningWidget *keyboard);
struct _LighteningWidget * get_touchpad(LighteningWidget *touchpad);
struct _LighteningWidget * get_mediabar(LighteningWidget *touchpad);


void set_color(LighteningWidget *widget, GdkRGBA *color, libusb_device_handle *usbhandle, char *subzone);

#endif
