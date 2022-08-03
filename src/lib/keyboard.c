#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

#include "utils/functions.h"
#include "keyboard.h"

#include "key.h"
#include "touchpad.h"
#include "mediabar.h"

#include "utils/vars.h"

void set_color(LighteningWidget *widget, GdkRGBA *color, libusb_device_handle *usbhandle, char *subzone) {
    char *c = gdk_rgba_to_string(color);
    int r = (int) (color->red * 16);
    int g = (int) (color->green * 16);
    int b = (int) (color->blue * 16);

    if (strcmp(widget->name, "keyboard") == 0) {
        if (subzone != NULL) {
            char *zone_color = (char *) malloc(100 * sizeof(char));
            sprintf(zone_color, "zone:%s%s", subzone, c);
            gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_keyboard, &zone_color, NULL); 
            free(zone_color);
        } else {
            gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_keyboard, &c, NULL); 
        }
        if (subzone == NULL || strcmp(subzone, "left") == 0) {
            set_zone_color(usbhandle, ZONE_KEYBOARD_LEFT, r, g, b);
        }
        if (subzone == NULL || strcmp(subzone, "middle_left") == 0) {
            set_zone_color(usbhandle, ZONE_KEYBOARD_MIDDLE_LEFT, r, g, b);
        }
        if (subzone == NULL || strcmp(subzone, "middle_right") == 0) {
            set_zone_color(usbhandle, ZONE_KEYBOARD_MIDDLE_RIGHT, r, g, b);
        }
        if (subzone == NULL || strcmp(subzone, "right") == 0) {
            set_zone_color(usbhandle, ZONE_KEYBOARD_RIGHT, r, g, b);
        }
    }
    if (strcmp(widget->name, "touchpad") == 0) {
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_touchpad, &c, NULL); 
        set_zone_color(usbhandle, ZONE_TOUCHPAD, r, g, b);
    }
    if (strcmp(widget->name, "mediabar") == 0) {
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_mediabar, &c, NULL); 
        set_zone_color(usbhandle, ZONE_MEDIABAR, r, g, b);
    }
}

LighteningWidget * get_keyboard(LighteningWidget *keyboard) {
    GtkWidget *area = gtk_drawing_area_new();

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 1600);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 500);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_keyboard, NULL, NULL);

    GdkRGBA c;
    gdk_rgba_parse(&c, "rgba(255, 0, 0, 1)");

    keyboard->widget = area;
    keyboard->name = "keyboard";
    return keyboard;
}

LighteningWidget * get_touchpad(LighteningWidget *touchpad){
    GtkWidget *area = gtk_drawing_area_new();

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 1600);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 200);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_touchpad, NULL, NULL);

    GdkRGBA c;
    gdk_rgba_parse(&c, "rgba(255, 0, 0, 1)");

    touchpad->widget = area;
    touchpad->name = "touchpad";
    return touchpad;
}

LighteningWidget * get_mediabar(LighteningWidget *mediabar) {
    GtkWidget *area = gtk_drawing_area_new();

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 30);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 30);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_mediabar, NULL, NULL);

    GdkRGBA c;
    gdk_rgba_parse(&c, "rgba(255, 0, 0, 1)");

    mediabar->widget = area;
    mediabar->name = "mediabar";
    return mediabar;
}
