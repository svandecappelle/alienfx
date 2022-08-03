#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

#include "utils/functions.h"
#include "keyboard.h"

#include "key.h"
#include "touchpad.h"
#include "mediabar.h"

#include "utils/vars.h"

const char * DEFAULT_COLOR = "rgba(255, 0, 0, 1)";

int to_zone_value(int zone) {
    if (zone == ZONE_LEFT) {
        return ZONE_KEYBOARD_LEFT;
    }
    if (zone == ZONE_MIDDLE_LEFT) {
        return ZONE_KEYBOARD_MIDDLE_LEFT;
    }
    if (zone == ZONE_MIDDLE_RIGHT) {
        return ZONE_KEYBOARD_MIDDLE_RIGHT;
    }
    if (zone == ZONE_RIGHT) {
        return ZONE_KEYBOARD_RIGHT;
    }
    return -1;
}

void set_color(LighteningWidget *widget, ColorZone *colors[], int nb_zones, libusb_device_handle *usbhandle) {
    static ColorZones zones;
    
    for (int i = 0; i < nb_zones; i+=1) {
        ColorZone * zc = (ColorZone*) malloc(sizeof(ColorZone));

        if (colors[i]->color != NULL) {
            zc->color = colors[i]->color;
            zc->zone = colors[i]->zone;
            zc->rgba = colors[i]->rgba;
        
            int r = (int) (zc->rgba->red * 16);
            int g = (int) (zc->rgba->green * 16);
            int b = (int) (zc->rgba->blue * 16);
    
            if (strcmp(widget->name, "keyboard") == 0) {
                printf("Try it: %s\n", zc->color);
                set_zone_color(usbhandle, to_zone_value(zc->zone), r, g, b);
            }
        } else {
            zc->color = NULL; // colors[0]->color;
            zc->zone = i;
            zc->rgba = NULL;
        }

        zones.zones[i] = zc;
    }
    zones.lenght = nb_zones;
    if (strcmp(widget->name, "keyboard") == 0) { 
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_keyboard, &zones, NULL); 
    }
    /*
    if (strcmp(widget->name, "touchpad") == 0) {
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_touchpad, &c, NULL); 
        set_zone_color(usbhandle, ZONE_TOUCHPAD, r, g, b);
    }
    if (strcmp(widget->name, "mediabar") == 0) {
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget->widget), draw_mediabar, &c, NULL); 
        set_zone_color(usbhandle, ZONE_MEDIABAR, r, g, b);
    }
    */
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
