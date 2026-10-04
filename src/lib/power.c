#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "power.h"
#include "utils/functions.h"
#include "utils/vars.h"

// Controller commands (byte 1 of a packet)
#define CMD_SET_MORPH_COLOR 0x01
#define CMD_SET_BLINK_COLOR 0x02
#define CMD_SET_COLOR 0x03
#define CMD_LOOP_BLOCK_END 0x04
#define CMD_TRANSMIT_EXECUTE 0x05
#define CMD_GET_STATUS 0x06
#define CMD_RESET 0x07
#define CMD_SAVE_NEXT 0x08
#define CMD_SAVE 0x09
#define CMD_SET_SPEED 0x0e

#define RESET_ALL_LIGHTS_ON 0x04
#define STATUS_READY 0x10
// Duration of a pulse or blink step, same as the Dell default theme
#define EFFECT_SPEED 200

const PowerStateInfo POWER_STATES[POWER_STATE_COUNT] = {
    [POWER_STATE_BOOT] = {"boot", "Booting", 1, 0},
    [POWER_STATE_AC_CHARGED] = {"ac-charged", "Plugged in", 5, 0},
    [POWER_STATE_AC_CHARGING] = {"ac-charging", "Charging", 6, 0},
    [POWER_STATE_AC_SLEEP] = {"ac-sleep", "Asleep on AC", 2, 1},
    [POWER_STATE_BATTERY_ON] = {"battery", "On battery", 8, 0},
    [POWER_STATE_BATTERY_CRITICAL] = {"battery-critical", "Battery critical", 9, 0},
    [POWER_STATE_BATTERY_SLEEP] = {"battery-sleep", "Asleep on battery", 7, 1},
};

const char *POWER_EFFECT_KEYS[POWER_EFFECT_COUNT] = {"off", "steady", "pulse", "blink"};

void power_default_styles(PowerStyle styles[POWER_STATE_COUNT]) {
    const PowerStyle red = {POWER_EFFECT_STEADY, {15, 0, 0}, {0, 0, 0}};
    const PowerStyle blue = {POWER_EFFECT_STEADY, {0, 0, 15}, {0, 0, 0}};
    const PowerStyle amber = {POWER_EFFECT_STEADY, {15, 9, 0}, {0, 0, 0}};

    styles[POWER_STATE_BOOT] = red;
    styles[POWER_STATE_AC_CHARGED] = blue;
    styles[POWER_STATE_AC_CHARGING] = (PowerStyle) {POWER_EFFECT_PULSE, {0, 0, 15}, {15, 9, 0}};
    styles[POWER_STATE_AC_SLEEP] = (PowerStyle) {POWER_EFFECT_PULSE, {0, 0, 15}, {0, 0, 0}};
    styles[POWER_STATE_BATTERY_ON] = amber;
    styles[POWER_STATE_BATTERY_CRITICAL] = (PowerStyle) {POWER_EFFECT_BLINK, {15, 9, 0}, {0, 0, 0}};
    styles[POWER_STATE_BATTERY_SLEEP] = (PowerStyle) {POWER_EFFECT_PULSE, {15, 9, 0}, {0, 0, 0}};
}

// ---------------------------------------------------------------------------
// Packets
// ---------------------------------------------------------------------------

typedef unsigned char Packet[SEND_DATA_SIZE];

typedef struct {
    libusb_device_handle *usbhandle;
    int failed;
} Link;

static void make(Packet packet, unsigned char command, unsigned char argument) {
    memset(packet, 0, SEND_DATA_SIZE);
    packet[0] = 0x02;
    packet[1] = command;
    packet[2] = argument;
}

static void set_zone(Packet packet, int zone) {
    packet[3] = (zone >> 16) & 0xff;
    packet[4] = (zone >> 8) & 0xff;
    packet[5] = zone & 0xff;
}

static void pack_color(Packet packet, const int rgb[3]) {
    packet[6] = ((rgb[0] & 0x0f) << 4) | (rgb[1] & 0x0f);
    packet[7] = (rgb[2] & 0x0f) << 4;
}

static void pack_color_pair(Packet packet, const int from[3], const int to[3]) {
    packet[6] = ((from[0] & 0x0f) << 4) | (from[1] & 0x0f);
    packet[7] = ((from[2] & 0x0f) << 4) | (to[0] & 0x0f);
    packet[8] = ((to[1] & 0x0f) << 4) | (to[2] & 0x0f);
}

static void send(Link *link, Packet packet) {
    if (!link->failed && usbwrite(link->usbhandle, packet, SEND_DATA_SIZE) != OK) {
        link->failed = 1;
    }
}

static void send_command(Link *link, unsigned char command, unsigned char argument) {
    Packet packet;
    make(packet, command, argument);
    send(link, packet);
}

static int read_status(Link *link) {
    unsigned char reply[READ_DATA_SIZE];
    if (link->failed || usbread(link->usbhandle, reply, READ_DATA_SIZE) != OK) {
        return -1;
    }
    return reply[0];
}

static void wait_ready(Link *link) {
    for (int attempt = 0; attempt < 100 && !link->failed; attempt += 1) {
        send_command(link, CMD_GET_STATUS, 0);
        if (read_status(link) == STATUS_READY) {
            return;
        }
        usleep(10000);
    }
    link->failed = 1;
}

// A loop block: its actions run in sequence and repeat. When storing (state_code > 0)
// every action is announced with SAVE_NEXT so the controller records it for that state.
static void send_block(Link *link, int state_code, Packet *actions, int count) {
    for (int i = 0; i < count; i += 1) {
        if (state_code > 0) {
            send_command(link, CMD_SAVE_NEXT, state_code);
        }
        send(link, actions[i]);
    }
    if (state_code > 0) {
        send_command(link, CMD_SAVE_NEXT, state_code);
    }
    send_command(link, CMD_LOOP_BLOCK_END, 0);
}

// Program of one power state. state_code 0 runs it right away instead of storing it.
static void send_state(Link *link, int state_code, PowerState state, const PowerStyle *style,
        const ZoneLight *lights, int light_count) {
    unsigned char block = 1;
    Packet actions[2];

    if (!POWER_STATES[state].sleeping) {
        for (int i = 0; i < light_count; i += 1) {
            make(actions[0], CMD_SET_COLOR, block);
            set_zone(actions[0], lights[i].mask);
            pack_color(actions[0], lights[i].rgb);
            send_block(link, state_code, actions, 1);
            block += 1;
        }
    }

    const int off[3] = {0, 0, 0};
    int count = 1;
    switch (style->effect) {
        case POWER_EFFECT_OFF:
        case POWER_EFFECT_COUNT:
            make(actions[0], CMD_SET_COLOR, block);
            pack_color(actions[0], off);
            break;
        case POWER_EFFECT_STEADY:
            make(actions[0], CMD_SET_COLOR, block);
            pack_color(actions[0], style->color);
            break;
        case POWER_EFFECT_BLINK:
            make(actions[0], CMD_SET_BLINK_COLOR, block);
            pack_color(actions[0], style->color);
            break;
        case POWER_EFFECT_PULSE:
            // Morph to the second color and back
            make(actions[0], CMD_SET_MORPH_COLOR, block);
            pack_color_pair(actions[0], style->color, style->color2);
            make(actions[1], CMD_SET_MORPH_COLOR, block);
            pack_color_pair(actions[1], style->color2, style->color);
            count = 2;
            break;
    }
    for (int i = 0; i < count; i += 1) {
        set_zone(actions[i], ZONE_POWER_BUTTON);
    }
    send_block(link, state_code, actions, count);

    if (state_code > 0) {
        send_command(link, CMD_SAVE, 0);
    }
}

int power_write(libusb_device_handle *usbhandle, const PowerStyle styles[POWER_STATE_COUNT],
        const ZoneLight *lights, int light_count, int live_state) {
    Link link = {usbhandle, 0};

    send_command(&link, CMD_GET_STATUS, 0);
    read_status(&link);
    send_command(&link, CMD_RESET, RESET_ALL_LIGHTS_ON);
    wait_ready(&link);

    for (int state = 0; state < POWER_STATE_COUNT; state += 1) {
        send_state(&link, POWER_STATES[state].code, state, &styles[state], lights, light_count);
    }

    Packet speed;
    make(speed, CMD_SET_SPEED, (EFFECT_SPEED >> 8) & 0xff);
    speed[3] = EFFECT_SPEED & 0xff;
    send(&link, speed);

    // Run the program of the current state now: the controller would otherwise
    // keep the last one it ran until the next power event
    if (live_state < 0 || live_state >= POWER_STATE_COUNT) {
        live_state = POWER_STATE_BOOT;
    }
    send_state(&link, 0, live_state, &styles[live_state], lights, light_count);
    send_command(&link, CMD_TRANSMIT_EXECUTE, 0);

    return link.failed ? -1 : 0;
}

// ---------------------------------------------------------------------------
// Current power state
// ---------------------------------------------------------------------------

static int read_value(const char *dir, const char *name, char *buf, size_t size) {
    char path[512];
    snprintf(path, sizeof(path), "/sys/class/power_supply/%s/%s", dir, name);
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }
    char *line = fgets(buf, size, file);
    fclose(file);
    if (line == NULL) {
        return -1;
    }
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

int power_current_state(void) {
    DIR *dir = opendir("/sys/class/power_supply");
    if (dir == NULL) {
        return -1;
    }

    int on_ac = -1, has_battery = 0, charging = 0, critical = 0;
    char value[64];
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' || read_value(entry->d_name, "type", value, sizeof(value)) != 0) {
            continue;
        }
        if (strcmp(value, "Mains") == 0 && read_value(entry->d_name, "online", value, sizeof(value)) == 0) {
            on_ac = atoi(value) == 1 || on_ac == 1;
        } else if (strcmp(value, "Battery") == 0) {
            has_battery = 1;
            if (read_value(entry->d_name, "status", value, sizeof(value)) == 0) {
                charging |= strcmp(value, "Charging") == 0;
                if (on_ac == -1 && strcmp(value, "Discharging") == 0) {
                    on_ac = 0;
                }
            }
            if (read_value(entry->d_name, "capacity_level", value, sizeof(value)) == 0) {
                critical |= strcmp(value, "Critical") == 0;
            } else if (read_value(entry->d_name, "capacity", value, sizeof(value)) == 0) {
                critical |= atoi(value) <= 5;
            }
        }
    }
    closedir(dir);

    if (on_ac == -1 && !has_battery) {
        return -1;
    }
    if (on_ac != 0) {
        return charging ? POWER_STATE_AC_CHARGING : POWER_STATE_AC_CHARGED;
    }
    return critical ? POWER_STATE_BATTERY_CRITICAL : POWER_STATE_BATTERY_ON;
}
