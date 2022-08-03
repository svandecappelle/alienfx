#ifndef KEYBOARD_H
#define KEYBOARD_H

#define ZONE_LEFT 0
#define ZONE_MIDDLE_LEFT 1
#define ZONE_MIDDLE_RIGHT 2
#define ZONE_RIGHT 3

#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

typedef struct _LighteningWidget {
    GtkWidget *widget;
    char *name;
} LighteningWidget;

struct _LighteningWidget * get_keyboard(LighteningWidget *keyboard);
struct _LighteningWidget * get_touchpad(LighteningWidget *touchpad);
struct _LighteningWidget * get_mediabar(LighteningWidget *touchpad);

typedef struct _ColorZone {
    int zone;
    char *color;
    GdkRGBA * rgba;
} ColorZone;

typedef struct _ColorZones {
    int lenght;
    ColorZone * zones[4];
} ColorZones;


void set_color(LighteningWidget *widget, ColorZone *color[], int nb_zones, libusb_device_handle *usbhandle);

#endif
