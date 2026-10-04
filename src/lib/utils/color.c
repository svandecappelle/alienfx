#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "color.h"

typedef struct {
    const char *name;
    const char *hex;
} NamedColor;

static const NamedColor NAMED_COLORS[] = {
    {"off", "#000000"},
    {"white", "#ffffff"},
    {"red", "#ff0000"},
    {"orange", "#ff7a00"},
    {"yellow", "#ffff00"},
    {"lime", "#9dff00"},
    {"green", "#00ff00"},
    {"teal", "#00ffc3"},
    {"cyan", "#00ffff"},
    {"alien", "#00b4ff"},
    {"blue", "#0000ff"},
    {"indigo", "#5b3cff"},
    {"purple", "#9b30ff"},
    {"magenta", "#ff00ff"},
    {"pink", "#ff4f9a"},
};
#define NAMED_COLOR_COUNT (sizeof(NAMED_COLORS) / sizeof(NAMED_COLORS[0]))

void list_named_colors(FILE *out) {
    for (size_t i = 0; i < NAMED_COLOR_COUNT; i += 1) {
        fprintf(out, "%-8s %s\n", NAMED_COLORS[i].name, NAMED_COLORS[i].hex);
    }
}

int parse_int(const char *text, long min, long max, int *out) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    while (isspace((unsigned char) *end)) {
        end += 1;
    }
    if (errno != 0 || end == text || *end != '\0' || value < min || value > max) {
        return -1;
    }
    *out = (int) value;
    return 0;
}

static int parse_hex(const char *text, int rgb[3]) {
    if (text[0] == '#') {
        text += 1;
    }
    if (strlen(text) != 6 || strspn(text, "0123456789abcdefABCDEF") != 6) {
        return -1;
    }
    unsigned long value = strtoul(text, NULL, 16);
    for (int i = 0; i < 3; i += 1) {
        int channel = (value >> (16 - 8 * i)) & 0xff;
        // 0-255 -> 0-15, rounded
        rgb[i] = (channel * 15 + 127) / 255;
    }
    return 0;
}

static int parse_triplet(const char *text, int rgb[3]) {
    char *copy = strdup(text);
    char *save = NULL;
    int count = 0;
    int status = 0;

    for (char *part = strtok_r(copy, ",", &save); part != NULL; part = strtok_r(NULL, ",", &save)) {
        if (count == 3 || parse_int(part, 0, 15, &rgb[count]) != 0) {
            status = -1;
            break;
        }
        count += 1;
    }
    free(copy);
    return status == 0 && count == 3 ? 0 : -1;
}

int parse_color(const char *text, int rgb[3]) {
    for (size_t i = 0; i < NAMED_COLOR_COUNT; i += 1) {
        if (strcasecmp(text, NAMED_COLORS[i].name) == 0) {
            return parse_hex(NAMED_COLORS[i].hex, rgb);
        }
    }
    if (strchr(text, ',') != NULL) {
        return parse_triplet(text, rgb);
    }
    return parse_hex(text, rgb);
}
