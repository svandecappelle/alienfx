#ifndef POWER_H
#define POWER_H

#include <libusb-1.0/libusb.h>

// The power button light is driven by the controller firmware from the power
// state of the laptop: it can't be set live like the other zones. Instead one
// program per power state is stored in the controller.

typedef enum {
    POWER_STATE_BOOT,
    POWER_STATE_AC_CHARGED,
    POWER_STATE_AC_CHARGING,
    POWER_STATE_AC_SLEEP,
    POWER_STATE_BATTERY_ON,
    POWER_STATE_BATTERY_CRITICAL,
    POWER_STATE_BATTERY_SLEEP,
    POWER_STATE_COUNT
} PowerState;

typedef enum {
    POWER_EFFECT_OFF,
    POWER_EFFECT_STEADY,
    POWER_EFFECT_PULSE,   // fades back and forth between color and color2
    POWER_EFFECT_BLINK,
    POWER_EFFECT_COUNT
} PowerEffect;

typedef struct {
    PowerEffect effect;
    int color[3];    // hardware values, 0 - 15
    int color2[3];   // second color of a pulse
} PowerStyle;

typedef struct {
    const char *key;     // identifier used in config files
    const char *label;
    int code;            // state code of the controller
    int sleeping;        // the other lights are off in this state
} PowerStateInfo;

extern const PowerStateInfo POWER_STATES[POWER_STATE_COUNT];
// "off", "steady", "pulse", "blink"
extern const char *POWER_EFFECT_KEYS[POWER_EFFECT_COUNT];

// Styles of the Dell default theme
void power_default_styles(PowerStyle styles[POWER_STATE_COUNT]);

// Color of another zone, restored by the controller whenever the laptop is awake
typedef struct {
    int mask;
    int rgb[3];
} ZoneLight;

// Store the power button styles in the controller, together with the colors of
// the other zones for every awake state. Takes a few seconds.
// Returns 0 on success, -1 on a USB error.
int power_write(libusb_device_handle *usbhandle, const PowerStyle styles[POWER_STATE_COUNT],
        const ZoneLight *lights, int light_count);

// Current power state read from /sys/class/power_supply, -1 when unknown
int power_current_state(void);

#endif
