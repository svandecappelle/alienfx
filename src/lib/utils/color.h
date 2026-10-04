#ifndef COLOR_H
#define COLOR_H

#include <stdio.h>

// Parse "r,g,b" (0-15 each), "#rrggbb" / "rrggbb" or a color name into
// hardware values (0-15 per channel). Returns 0 on success, -1 otherwise.
int parse_color(const char *text, int rgb[3]);

// Parse a decimal integer that spans the whole string (surrounding spaces allowed)
int parse_int(const char *text, long min, long max, int *out);

// Print the color names and their hex value, one per line
void list_named_colors(FILE *out);

#endif
