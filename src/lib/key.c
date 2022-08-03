#include <gtk/gtk.h>

const int KEY_HEIGHT = 24;
const int KEY_WIDTH = 24;
const int KEY_PADDING = 8;
int window_width = 0;

char *keys[6][18] = {
    {"Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Pause", "Impr.E", "Inser", "Suppr"},
    {"²", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "°", "+", "ret", "arrow", NULL, NULL, NULL},
    {"⇄", "A", "Z", "E", "R", "T", "Y", "U", "I", "O", "P", "-", "$", "Enter", "Page up", NULL, NULL, NULL},
    {"🔒", "Q", "S", "D", "F", "G", "H", "J", "K", "L", "M", "%", "µ", "Page down", NULL, NULL, NULL, NULL},
    {"↑", "Chev", "W", "X", "C", "V", "B", "N", "?", ".", "/", "!", "Shift", "Fin", NULL, NULL, NULL, NULL},
    {"ctrl", "Fn", "Super", "Alt", "Space", "Alt Gr", "Menu", "Ctrl", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

struct KeyProperty {
    int left;
    int top;
    int height;
    int width;
    int zone;
};

char *ZONES[4] = {
    "left",
    "middle_left",
    "middle_right",
    "right"
};

void print_one_key(cairo_t *cr, const char *key_text, struct KeyProperty *k_prop, GdkRGBA *background_color, GdkRGBA *color) {
    int w, h;
    w = k_prop->width;
    h = k_prop->height;
    gdk_cairo_set_source_rgba(cr, color);

    // parameters:
    // 1 - cairo
    // 2 - position from left
    // 3 - position from top
    // 4 - draw width
    // 5 - draw height
    //

    // special case for enter key
    if (strcmp(key_text, "Enter") == 0) {
        // Border key:
        cairo_move_to(cr, k_prop->left + KEY_PADDING, k_prop->top + KEY_PADDING);
        cairo_rel_line_to(cr, 0, h);
        cairo_rel_line_to(cr, 22, 0);
        cairo_rel_line_to(cr, 0, h + KEY_PADDING - 3);
        cairo_rel_line_to(cr, w - KEY_PADDING - 14, 0);
        cairo_rel_line_to(cr, 0, - 2 * h - KEY_PADDING + 2);
        cairo_close_path(cr);
        cairo_fill(cr);

        gdk_cairo_set_source_rgba(cr, background_color);
        // Content key
        cairo_move_to(cr, k_prop->left + KEY_PADDING + 1, k_prop->top + KEY_PADDING + 1);
        cairo_rel_line_to(cr, 0, h - 2);
        cairo_rel_line_to(cr, 22, 0);
        cairo_rel_line_to(cr, 0, h + KEY_PADDING - 3);
        cairo_rel_line_to(cr, w - KEY_PADDING - 16, 0);
        cairo_rel_line_to(cr, 0, - 2 * (h - 2) - KEY_PADDING);
        cairo_close_path(cr);

        cairo_fill(cr);
    } else {
        double x = k_prop->left + KEY_PADDING,
               y = k_prop->top + KEY_PADDING,
               tw = w,
               th = h;
        double aspect = 1.0;
        double corner_radius = (th - 4) / 30.0;
        double radius = corner_radius / aspect;
        double degrees = M_PI / 180.0;

        // cairo_rectangle(cr, w, 0, w + 4, h - 4);
        gdk_cairo_set_source_rgba(cr, background_color);

        cairo_new_sub_path(cr);
        cairo_arc(cr, x + tw - radius, y + radius, radius, -90 * degrees, 0 * degrees);
        cairo_arc(cr, x + tw - radius, y + th - radius, radius, 0 * degrees, 90 * degrees);
        cairo_arc(cr, x + radius, y + th - radius, radius, 90 * degrees, 180 * degrees);
        cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
        cairo_close_path(cr);
        // cairo_fill(cr);
        cairo_fill_preserve(cr);
        gdk_cairo_set_source_rgba(cr, color);

        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    gdk_cairo_set_source_rgba(cr, color);
    gchar c[strlen(key_text)];
    sprintf(c, "%s", key_text);
    cairo_text_extents_t text;
    cairo_text_extents(cr, c, &text);
    cairo_move_to(cr, k_prop->left + KEY_PADDING + 5, k_prop->top + KEY_PADDING + 12);
    cairo_show_text(cr, c);
}

struct KeyProperty * get_key_properties(const int key_num_top, const int key_num_left, const int w, const int h) {
    static struct KeyProperty p = {
        .left=0,
        .top=0,
        .height=0,
        .width=0,
        .zone=0
    };
    p.height = h - 5;
    p.width = w - 5;
    p.top = (key_num_top - 1) * h - (h / 2) + KEY_PADDING;
    p.zone = key_num_left / 5;

    if (key_num_top == 4 && key_num_left == 1) {
        // Maj lock Key
        p.width = w + (w / 2);
    }
    if (key_num_top == 2 && key_num_left == 14) {
        p.width = 2 * (w + 7);
    }
    if (key_num_top == 1) {
        p.width = w - 9;

        p.top = 0; // p.top + (h / 2) - 5;
        p.height = h / 2;
    }
    if (key_num_top == 5 && key_num_left == 13) {
        // Left shift
        p.width = 2.5 * w + (w / 2) + 10;
    }
    if (key_num_top == 6 && key_num_left > 5) {
        // After space apply a padding
        p.width = w + (w / 3);
    }
    if (key_num_top == 6 && key_num_left == 5) {
        // Space key
        p.width = (5 * w )- 5;
    }
    if (key_num_top == 3) {
        int first_width = w + (w / 4);
        if (key_num_left == 1) {
            // Tab
            p.width = first_width;
        }

        if (key_num_left == 14) {
            // Enter
            p.width = 2 * (w - 8);
        }
    }

    if (
            (
             key_num_left == 15 && (
                 key_num_top == 2 || key_num_top == 3
                 )
            ) || (
                key_num_left == 14 && (key_num_top == 4 || key_num_top == 5)
                )
       ) {
        p.width = 18/16 * (w - 9);
    }

    return &p;
}

void draw_keyboard (GtkDrawingArea *da,
        cairo_t        *cr,
        int             width,
        int             height,
        gpointer        data) {
    window_width = width;
    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(da));
    GdkRGBA background_color, color;
    const int w = width / 19;
    const int h = w;

    gtk_style_context_get_color(context, &background_color);
    int specific_zone = -1;

    if (data != NULL) {
        char *given_color = NULL; // malloc( * sizeof(char));

        for (int i=0; i < 4; i+=1) {
            char *given_input = *((char **) data);
            char *zone_name = ZONES[i];
            char c_zone[strlen(zone_name) + 5];
            sprintf(c_zone, "zone:%s", zone_name);
            if (strstr(given_input, c_zone)) {
                given_color = strncpy(c_zone, &given_input[strlen(c_zone)], strlen((char *)data));
                specific_zone = i;
            } else {
                given_color = (char *) data;
            }

            printf("Color given: %s\n", (char *) data);
            gdk_rgba_parse(&color, given_color);
        }
    } else {
        gdk_rgba_parse(&color, "rgba(255, 0, 0, 1)");
    }
    // h = height / 2.0;

    printf("Set color to %s\n", gdk_rgba_to_string(&color));
    gdk_rgba_parse(&background_color, "#292630");

    for (int i=1; i < 7; i+=1) {
        int left = 90;
        for (int j=1; j < 18; j+=1) {
            const char *key_char = keys[i - 1][j - 1];
            if (key_char != NULL) {
                struct KeyProperty *key_prop = get_key_properties(i, j, w, h);
                if (
                        (
                         j == 15 && i == 3
                        ) || (
                            j == 14 && (i == 4 || i == 5)
                            )
                   ) {
                    left += 5;
                }
                if (i == 2 && j == 15) {
                    left += 1;
                }
                if (i == 4 && j == 14) {
                    // special case after enter
                    left += w + 52;
                }
                key_prop->left = left;
                if (specific_zone == -1 || specific_zone == key_prop->zone) {
                    print_one_key(cr, key_char, key_prop, &background_color, &color);
                }
                left = left + key_prop->width + 5;
            }
        }
    }
}
