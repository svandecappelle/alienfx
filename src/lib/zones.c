#include <math.h>
#include <stdio.h>
#include <string.h>

#include "zones.h"
#include "utils/functions.h"
#include "utils/vars.h"

#define DEFAULT_COLOR {0.0, 0.706, 1.0, 1.0}

static LightZoneState zones[LIGHT_COUNT] = {
    {"keyboard-left", "Left keys", "Keyboard zone 1 of 4",
        ZONE_KEYBOARD_LEFT, DEFAULT_COLOR, 1.0},
    {"keyboard-middle-left", "Center-left keys", "Keyboard zone 2 of 4",
        ZONE_KEYBOARD_MIDDLE_LEFT, DEFAULT_COLOR, 1.0},
    {"keyboard-middle-right", "Center-right keys", "Keyboard zone 3 of 4",
        ZONE_KEYBOARD_MIDDLE_RIGHT, DEFAULT_COLOR, 1.0},
    {"keyboard-right", "Right keys", "Keyboard zone 4 of 4",
        ZONE_KEYBOARD_RIGHT, DEFAULT_COLOR, 1.0},
    {"touchpad", "Touchpad", "Glowing touchpad border",
        ZONE_TOUCHPAD, DEFAULT_COLOR, 1.0},
    {"mediabar", "Media bar", "Touch media controls",
        ZONE_MEDIABAR, DEFAULT_COLOR, 1.0},
    {"speakers", "Speakers", "Left and right speaker grilles",
        ZONE_SPEAKER_LEFT | ZONE_SPEAKER_RIGHT, DEFAULT_COLOR, 1.0},
    {"logo", "Logo", "Alienware name and alien head on the lid",
        ZONE_ALIEN_HEAD | ZONE_ALIEN_NAME, DEFAULT_COLOR, 1.0},
};

static PowerStyle power_styles[POWER_STATE_COUNT];
static gboolean power_styles_ready = FALSE;

PowerStyle * zones_power_styles(void) {
    if (!power_styles_ready) {
        power_default_styles(power_styles);
        power_styles_ready = TRUE;
    }
    return power_styles;
}

LightZoneState * zones_get(int zone) {
    if (zone < 0 || zone >= LIGHT_COUNT) {
        return NULL;
    }
    return &zones[zone];
}

guint zones_selection_mask(int selection) {
    if (selection == LIGHT_ALL) {
        return (1u << LIGHT_COUNT) - 1;
    }
    if (selection < 0 || selection >= LIGHT_COUNT) {
        return 0;
    }
    return 1u << selection;
}

static int to_hardware(double value, double brightness) {
    return (int) lround(CLAMP(value * brightness, 0.0, 1.0) * 15);
}

void zones_get_hardware_color(int zone, int *r, int *g, int *b) {
    LightZoneState *z = &zones[zone];
    *r = to_hardware(z->color.red, z->brightness);
    *g = to_hardware(z->color.green, z->brightness);
    *b = to_hardware(z->color.blue, z->brightness);
}

void zones_get_effective_color(int zone, GdkRGBA *out) {
    int r, g, b;
    zones_get_hardware_color(zone, &r, &g, &b);
    *out = (GdkRGBA) { r / 15.0, g / 15.0, b / 15.0, 1.0 };
}

void zones_set_color(int selection, const GdkRGBA *color) {
    guint mask = zones_selection_mask(selection);
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        if (mask & (1u << i)) {
            zones[i].color = *color;
            zones[i].color.alpha = 1.0;
        }
    }
}

void zones_set_brightness(int selection, double brightness) {
    guint mask = zones_selection_mask(selection);
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        if (mask & (1u << i)) {
            zones[i].brightness = CLAMP(brightness, 0.0, 1.0);
        }
    }
}

void zones_write(libusb_device_handle *usbhandle, guint mask) {
    if (usbhandle == NULL) {
        return;
    }
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        if (mask & (1u << i)) {
            int r, g, b;
            zones_get_hardware_color(i, &r, &g, &b);
            set_zone_color(usbhandle, zones[i].mask, r, g, b);
        }
    }
}

static gboolean parse_hardware_color(const char *text, int rgb[3]) {
    return text != NULL && sscanf(text, "%d,%d,%d", &rgb[0], &rgb[1], &rgb[2]) == 3
        && rgb[0] >= 0 && rgb[0] <= 15 && rgb[1] >= 0 && rgb[1] <= 15 && rgb[2] >= 0 && rgb[2] <= 15;
}

static void load_power_styles(GKeyFile *file) {
    PowerStyle *styles = zones_power_styles();
    for (int i = 0; i < POWER_STATE_COUNT; i += 1) {
        char *group = g_strdup_printf("power-%s", POWER_STATES[i].key);
        char *effect = g_key_file_get_string(file, group, "effect", NULL);
        char *color = g_key_file_get_string(file, group, "color", NULL);
        char *color2 = g_key_file_get_string(file, group, "color2", NULL);
        PowerStyle style = styles[i];
        for (int e = 0; effect != NULL && e < POWER_EFFECT_COUNT; e += 1) {
            if (strcmp(effect, POWER_EFFECT_KEYS[e]) == 0) {
                style.effect = e;
            }
        }
        parse_hardware_color(color, style.color);
        parse_hardware_color(color2, style.color2);
        styles[i] = style;
        g_free(effect);
        g_free(color);
        g_free(color2);
        g_free(group);
    }
}

static void save_power_styles(GKeyFile *file) {
    PowerStyle *styles = zones_power_styles();
    for (int i = 0; i < POWER_STATE_COUNT; i += 1) {
        char *group = g_strdup_printf("power-%s", POWER_STATES[i].key);
        char *color = g_strdup_printf("%d,%d,%d", styles[i].color[0], styles[i].color[1], styles[i].color[2]);
        char *color2 = g_strdup_printf("%d,%d,%d", styles[i].color2[0], styles[i].color2[1], styles[i].color2[2]);
        g_key_file_set_string(file, group, "effect", POWER_EFFECT_KEYS[styles[i].effect]);
        g_key_file_set_string(file, group, "color", color);
        g_key_file_set_string(file, group, "color2", color2);
        g_free(color);
        g_free(color2);
        g_free(group);
    }
}

static char * config_path(void) {
    return g_build_filename(g_get_user_config_dir(), "alienfx", "zones.ini", NULL);
}

void zones_load(void) {
    char *path = config_path();
    GKeyFile *file = g_key_file_new();

    if (g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, NULL)) {
        for (int i = 0; i < LIGHT_COUNT; i += 1) {
            char *color = g_key_file_get_string(file, zones[i].key, "color", NULL);
            if (color != NULL) {
                gdk_rgba_parse(&zones[i].color, color);
                g_free(color);
            }
            GError *error = NULL;
            double brightness = g_key_file_get_double(file, zones[i].key, "brightness", &error);
            if (error == NULL) {
                zones[i].brightness = CLAMP(brightness, 0.0, 1.0);
            } else {
                g_error_free(error);
            }
        }
        load_power_styles(file);
    }

    g_key_file_free(file);
    g_free(path);
}

void zones_save(void) {
    char *path = config_path();
    char *dir = g_path_get_dirname(path);
    GKeyFile *file = g_key_file_new();

    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        char *color = gdk_rgba_to_string(&zones[i].color);
        g_key_file_set_string(file, zones[i].key, "color", color);
        g_key_file_set_double(file, zones[i].key, "brightness", zones[i].brightness);
        g_free(color);
    }
    save_power_styles(file);

    g_mkdir_with_parents(dir, 0755);
    g_key_file_save_to_file(file, path, NULL);

    g_key_file_free(file);
    g_free(dir);
    g_free(path);
}
