#ifndef ZONES_H
#define ZONES_H

#include <gtk/gtk.h>
#include <libusb-1.0/libusb.h>

// Every lighting zone the UI can drive
typedef enum {
    LIGHT_KEYBOARD_LEFT,
    LIGHT_KEYBOARD_MIDDLE_LEFT,
    LIGHT_KEYBOARD_MIDDLE_RIGHT,
    LIGHT_KEYBOARD_RIGHT,
    LIGHT_TOUCHPAD,
    LIGHT_MEDIABAR,
    LIGHT_SPEAKERS,
    LIGHT_LOGO,
    LIGHT_COUNT
} LightZone;

// Special selections
#define LIGHT_ALL LIGHT_COUNT
#define LIGHT_NONE -1

typedef struct {
    const char *key;      // identifier used in the config file
    const char *label;    // human readable name
    const char *detail;   // short description shown in the UI
    int mask;             // hardware zone bitmask
    GdkRGBA color;        // color chosen by the user
    double brightness;    // 0.0 - 1.0
} LightZoneState;

LightZoneState * zones_get(int zone);

// Bitmask of the zones covered by a selection (a zone or LIGHT_ALL)
guint zones_selection_mask(int selection);

// Color as the hardware will display it (brightness applied, 4 bits per channel)
void zones_get_effective_color(int zone, GdkRGBA *out);
// Hardware value (0 - 15) of each channel
void zones_get_hardware_color(int zone, int *r, int *g, int *b);

void zones_set_color(int selection, const GdkRGBA *color);
void zones_set_brightness(int selection, double brightness);

// Send the zones of the given bitmask to the device (no-op when usbhandle is NULL)
void zones_write(libusb_device_handle *usbhandle, guint mask);

// Persist / restore colors in the user config dir
void zones_load(void);
void zones_save(void);

#endif
