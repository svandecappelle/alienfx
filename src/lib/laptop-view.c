#include <math.h>
#include <pango/pangocairo.h>

#include "laptop-view.h"
#include "zones.h"

// The laptop is laid out in "key units" (1u = width of a standard key)
// and scaled to fit the widget.
#define VIEW_WIDTH 19.0
#define VIEW_HEIGHT 13.8

#define KB_X 1.5
#define KB_Y 3.4
#define KB_W 16.0
#define KB_H 5.6
#define KB_ZONE_W (KB_W / 4)
#define KEY_GAP 0.08

typedef struct {
    double x, y, w, h;
} Rect;

typedef struct {
    int zone;
    Rect rect;
} HitRegion;

#define BEZEL_RECT          {3.0, 0.0, 13.0, 1.2}
#define LOGO_TEXT_RECT      {4.5, 0.15, 10.0, 0.9}
#define DECK_RECT           {0.0, 1.5, 19.0, 12.3}
#define SPEAKER_LEFT_RECT   {1.5, 2.05, 4.3, 0.8}
#define SPEAKER_RIGHT_RECT  {13.2, 2.05, 4.3, 0.8}
#define MEDIABAR_RECT       {6.3, 2.05, 6.4, 0.8}
#define HEAD_RECT           {9.0, 1.95, 1.0, 1.0}
#define TOUCHPAD_RECT       {6.5, 9.6, 6.0, 3.4}
#define KB_ZONE_RECT(i)     {KB_X + (i) * KB_ZONE_W, KB_Y, KB_ZONE_W, KB_H}

static const Rect BEZEL = BEZEL_RECT;
static const Rect DECK = DECK_RECT;
static const Rect SPEAKER_LEFT = SPEAKER_LEFT_RECT;
static const Rect SPEAKER_RIGHT = SPEAKER_RIGHT_RECT;
static const Rect MEDIABAR = MEDIABAR_RECT;
static const Rect HEAD = HEAD_RECT;
static const Rect TOUCHPAD = TOUCHPAD_RECT;

// First match wins: the power button sits on top of the media bar
static const HitRegion HIT_REGIONS[] = {
    {LIGHT_POWER_BUTTON, HEAD_RECT},
    {LIGHT_LOGO, LOGO_TEXT_RECT},
    {LIGHT_MEDIABAR, MEDIABAR_RECT},
    {LIGHT_SPEAKERS, SPEAKER_LEFT_RECT},
    {LIGHT_SPEAKERS, SPEAKER_RIGHT_RECT},
    {LIGHT_TOUCHPAD, TOUCHPAD_RECT},
    {LIGHT_KEYBOARD_LEFT, KB_ZONE_RECT(0)},
    {LIGHT_KEYBOARD_MIDDLE_LEFT, KB_ZONE_RECT(1)},
    {LIGHT_KEYBOARD_MIDDLE_RIGHT, KB_ZONE_RECT(2)},
    {LIGHT_KEYBOARD_RIGHT, KB_ZONE_RECT(3)},
};
#define HIT_REGION_COUNT (sizeof(HIT_REGIONS) / sizeof(HIT_REGIONS[0]))

// Keyboard (French AZERTY layout). A NULL label with a width is an empty space.
typedef struct {
    const char *label;
    double width;
} Key;

#define ENTER_LABEL "↵"
#define END_ROW {NULL, 0}

static const Key ROW_FUNCTION[] = {
    {"Esc", 1}, {"F1", 1}, {"F2", 1}, {"F3", 1}, {"F4", 1}, {"F5", 1}, {"F6", 1},
    {"F7", 1}, {"F8", 1}, {"F9", 1}, {"F10", 1}, {"F11", 1}, {"F12", 1},
    {"Pause", 1}, {"Impr", 1}, {"Inser", 1}, {"Suppr", 1}, END_ROW
};
static const Key ROW_NUMBERS[] = {
    {"²", 1}, {"1", 1}, {"2", 1}, {"3", 1}, {"4", 1}, {"5", 1}, {"6", 1},
    {"7", 1}, {"8", 1}, {"9", 1}, {"0", 1}, {")", 1}, {"=", 1},
    {"⌫", 2}, {"⇱", 1}, END_ROW
};
static const Key ROW_TOP[] = {
    {"⇥", 1.5}, {"A", 1}, {"Z", 1}, {"E", 1}, {"R", 1}, {"T", 1}, {"Y", 1},
    {"U", 1}, {"I", 1}, {"O", 1}, {"P", 1}, {"^", 1}, {"$", 1},
    {ENTER_LABEL, 1.5}, {"⇞", 1}, END_ROW
};
static const Key ROW_HOME[] = {
    {"⇪", 1.75}, {"Q", 1}, {"S", 1}, {"D", 1}, {"F", 1}, {"G", 1}, {"H", 1},
    {"J", 1}, {"K", 1}, {"L", 1}, {"M", 1}, {"ù", 1}, {"*", 1},
    {NULL, 1.25}, {"⇟", 1}, END_ROW
};
static const Key ROW_BOTTOM[] = {
    {"⇧", 1.25}, {"<", 1}, {"W", 1}, {"X", 1}, {"C", 1}, {"V", 1}, {"B", 1},
    {"N", 1}, {",", 1}, {";", 1}, {":", 1}, {"!", 1},
    {"⇧", 1.75}, {"↑", 1}, {"⇲", 1}, END_ROW
};
static const Key ROW_SPACE[] = {
    {"Ctrl", 1.25}, {"Fn", 1}, {"❖", 1}, {"Alt", 1.25}, {"", 5.25},
    {"Alt Gr", 1.25}, {"☰", 1}, {"Ctrl", 1}, {"←", 1}, {"↓", 1}, {"→", 1}, END_ROW
};

static const Key *ROWS[] = {ROW_FUNCTION, ROW_NUMBERS, ROW_TOP, ROW_HOME, ROW_BOTTOM, ROW_SPACE};
static const double ROW_HEIGHTS[] = {0.6, 1, 1, 1, 1, 1};
#define ROW_COUNT 6

typedef struct {
    int selection;
    int hover;
    double scale, ox, oy;
    LaptopViewSelectFunc on_select;
    gpointer user_data;
    int power_state;        // shown on the power button, -1 when unknown
    guint tick_id;          // animation of a pulsing / blinking power button
    gint64 last_frame;
} LaptopView;

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------

static void rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    r = MIN(r, MIN(w, h) / 2);
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -G_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, G_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, G_PI / 2, G_PI);
    cairo_arc(cr, x + r, y + r, r, G_PI, 3 * G_PI / 2);
    cairo_close_path(cr);
}

static void set_source(cairo_t *cr, const GdkRGBA *c, double alpha) {
    cairo_set_source_rgba(cr, c->red, c->green, c->blue, alpha);
}

static void set_hex(cairo_t *cr, guint hex, double alpha) {
    cairo_set_source_rgba(cr, ((hex >> 16) & 0xff) / 255.0, ((hex >> 8) & 0xff) / 255.0, (hex & 0xff) / 255.0, alpha);
}

static double intensity(const GdkRGBA *c) {
    return MAX(c->red, MAX(c->green, c->blue));
}

// Lit color, slightly washed towards white like a real LED behind plastic
static void glow_color(GdkRGBA c, GdkRGBA *out, double *level) {
    *level = intensity(&c);
    if (*level > 0) {
        // Normalize so a dim color stays recognizable, the level carries the brightness
        c.red /= *level;
        c.green /= *level;
        c.blue /= *level;
    }
    out->red = c.red + (1 - c.red) * 0.15;
    out->green = c.green + (1 - c.green) * 0.15;
    out->blue = c.blue + (1 - c.blue) * 0.15;
    out->alpha = 1;
}

static void lit_color(int zone, GdkRGBA *out, double *level) {
    GdkRGBA c;
    zones_get_effective_color(zone, &c);
    glow_color(c, out, level);
}

static void vertical_gradient(cairo_t *cr, double y0, double y1, guint top, guint bottom) {
    cairo_pattern_t *p = cairo_pattern_create_linear(0, y0, 0, y1);
    cairo_pattern_add_color_stop_rgb(p, 0, ((top >> 16) & 0xff) / 255.0, ((top >> 8) & 0xff) / 255.0, (top & 0xff) / 255.0);
    cairo_pattern_add_color_stop_rgb(p, 1, ((bottom >> 16) & 0xff) / 255.0, ((bottom >> 8) & 0xff) / 255.0, (bottom & 0xff) / 255.0);
    cairo_set_source(cr, p);
    cairo_pattern_destroy(p);
}

// Text is laid out in device pixels so Pango can hint it properly
static PangoLayout * create_text(cairo_t *cr, LaptopView *view, const char *text,
        double size, int weight, double spacing, double cx, double cy) {
    double x = cx, y = cy;
    cairo_user_to_device(cr, &x, &y);
    cairo_identity_matrix(cr);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *font = pango_font_description_from_string("Cantarell, Sans");
    pango_font_description_set_absolute_size(font, size * view->scale * PANGO_SCALE);
    pango_font_description_set_weight(font, weight);
    pango_layout_set_font_description(layout, font);
    pango_font_description_free(font);

    if (spacing > 0) {
        PangoAttrList *attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_letter_spacing_new((int) (spacing * view->scale * PANGO_SCALE)));
        pango_layout_set_attributes(layout, attrs);
        pango_attr_list_unref(attrs);
    }
    pango_layout_set_text(layout, text, -1);

    int w, h;
    pango_layout_get_pixel_size(layout, &w, &h);
    cairo_move_to(cr, x - w / 2.0, y - h / 2.0);
    return layout;
}

// ---------------------------------------------------------------------------
// Laptop parts
// ---------------------------------------------------------------------------

static void draw_body(cairo_t *cr) {
    // Floor shadow
    cairo_save(cr);
    cairo_translate(cr, VIEW_WIDTH / 2, DECK.y + DECK.h - 0.1);
    cairo_scale(cr, VIEW_WIDTH / 2 + 0.6, 0.9);
    cairo_pattern_t *shadow = cairo_pattern_create_radial(0, 0, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba(shadow, 0, 0, 0, 0, 0.6);
    cairo_pattern_add_color_stop_rgba(shadow, 1, 0, 0, 0, 0);
    cairo_set_source(cr, shadow);
    cairo_arc(cr, 0, 0, 1, 0, 2 * G_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(shadow);
    cairo_restore(cr);

    // Ambient glow of the keyboard on the surroundings
    GdkRGBA ambient = {0, 0, 0, 1};
    double level = 0;
    for (int i = LIGHT_KEYBOARD_LEFT; i <= LIGHT_KEYBOARD_RIGHT; i += 1) {
        GdkRGBA c;
        zones_get_effective_color(i, &c);
        ambient.red += c.red / 4;
        ambient.green += c.green / 4;
        ambient.blue += c.blue / 4;
    }
    level = intensity(&ambient);
    if (level > 0) {
        cairo_save(cr);
        cairo_translate(cr, VIEW_WIDTH / 2, KB_Y + KB_H / 2);
        cairo_scale(cr, VIEW_WIDTH * 0.62, KB_H * 1.3);
        cairo_pattern_t *glow = cairo_pattern_create_radial(0, 0, 0, 0, 0, 1);
        cairo_pattern_add_color_stop_rgba(glow, 0, ambient.red / level, ambient.green / level, ambient.blue / level, 0.22 * level);
        cairo_pattern_add_color_stop_rgba(glow, 1, ambient.red / level, ambient.green / level, ambient.blue / level, 0);
        cairo_set_source(cr, glow);
        cairo_arc(cr, 0, 0, 1, 0, 2 * G_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(glow);
        cairo_restore(cr);
    }

    // Screen bezel and hinge
    rounded_rect(cr, BEZEL.x, BEZEL.y, BEZEL.w, BEZEL.h, 0.3);
    vertical_gradient(cr, BEZEL.y, BEZEL.y + BEZEL.h, 0x0f1016, 0x1a1b23);
    cairo_fill(cr);
    cairo_rectangle(cr, BEZEL.x - 0.5, BEZEL.y + BEZEL.h, BEZEL.w + 1, DECK.y - BEZEL.y - BEZEL.h);
    set_hex(cr, 0x08090d, 1);
    cairo_fill(cr);

    // Deck
    rounded_rect(cr, DECK.x, DECK.y, DECK.w, DECK.h, 0.7);
    vertical_gradient(cr, DECK.y, DECK.y + DECK.h, 0x262734, 0x181922);
    cairo_fill_preserve(cr);
    set_hex(cr, 0xffffff, 0.08);
    cairo_set_line_width(cr, 0.03);
    cairo_stroke(cr);

    // Brushed top edge highlight
    cairo_move_to(cr, DECK.x + 0.7, DECK.y + 0.04);
    cairo_line_to(cr, DECK.x + DECK.w - 0.7, DECK.y + 0.04);
    set_hex(cr, 0xffffff, 0.12);
    cairo_set_line_width(cr, 0.03);
    cairo_stroke(cr);

    // Keyboard well
    rounded_rect(cr, KB_X - 0.2, KB_Y - 0.2, KB_W + 0.4, KB_H + 0.4, 0.25);
    set_hex(cr, 0x0c0d12, 1);
    cairo_fill(cr);
}

static void draw_logo(cairo_t *cr, LaptopView *view) {
    GdkRGBA color;
    double level;
    lit_color(LIGHT_LOGO, &color, &level);

    // "ALIENWARE" under the screen
    cairo_save(cr);
    PangoLayout *layout = create_text(cr, view, "ALIENWARE", 0.42, PANGO_WEIGHT_HEAVY, 0.32,
            VIEW_WIDTH / 2, BEZEL.y + BEZEL.h / 2 + 0.02);
    pango_cairo_layout_path(cr, layout);
    if (level > 0) {
        set_source(cr, &color, 0.12 * level);
        cairo_set_line_width(cr, 0.24 * view->scale);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_stroke_preserve(cr);
        set_source(cr, &color, 0.3 * level);
        cairo_set_line_width(cr, 0.1 * view->scale);
        cairo_stroke_preserve(cr);
        set_source(cr, &color, 0.35 + 0.65 * level);
    } else {
        set_hex(cr, 0x34363f, 1);
    }
    cairo_fill(cr);
    g_object_unref(layout);
    cairo_restore(cr);
}

static const PowerStyle * shown_power_style(LaptopView *view) {
    int state = view->power_state >= 0 ? view->power_state : POWER_STATE_AC_CHARGED;
    return &zones_power_styles()[state];
}

static gboolean is_animated(const PowerStyle *style) {
    return style->effect == POWER_EFFECT_PULSE || style->effect == POWER_EFFECT_BLINK;
}

static gboolean on_tick(GtkWidget *widget, GdkFrameClock *clock, gpointer data) {
    LaptopView *view = data;
    if (!is_animated(shown_power_style(view))) {
        view->tick_id = 0;
        return G_SOURCE_REMOVE;
    }
    // Around 20 frames per second is plenty for a slow pulse
    gint64 now = gdk_frame_clock_get_frame_time(clock);
    if (now - view->last_frame >= 50000) {
        view->last_frame = now;
        gtk_widget_queue_draw(widget);
    }
    return G_SOURCE_CONTINUE;
}

// Color of the power button right now, following its effect
static GdkRGBA power_button_color(const PowerStyle *style) {
    double t = g_get_monotonic_time() / 1e6;
    const int *a = style->color, *b = style->color2;
    double mix = 0;

    switch (style->effect) {
        case POWER_EFFECT_OFF:
        case POWER_EFFECT_COUNT:
            return (GdkRGBA) {0, 0, 0, 1};
        case POWER_EFFECT_STEADY:
            break;
        case POWER_EFFECT_BLINK:
            if (fmod(t, 1.0) >= 0.5) {
                return (GdkRGBA) {0, 0, 0, 1};
            }
            break;
        case POWER_EFFECT_PULSE:
            mix = (1 - cos(2 * G_PI * t / 2.4)) / 2;
            break;
    }
    return (GdkRGBA) {
        (a[0] + (b[0] - a[0]) * mix) / 15.0,
        (a[1] + (b[1] - a[1]) * mix) / 15.0,
        (a[2] + (b[2] - a[2]) * mix) / 15.0,
        1,
    };
}

// Alien head shaped power button, in the middle of the media bar
static void draw_power_button(cairo_t *cr, GtkWidget *widget, LaptopView *view) {
    const PowerStyle *style = shown_power_style(view);
    if (is_animated(style) && view->tick_id == 0) {
        view->tick_id = gtk_widget_add_tick_callback(widget, on_tick, view, NULL);
    }
    GdkRGBA color;
    double level;
    glow_color(power_button_color(style), &color, &level);
    double cx = HEAD.x + HEAD.w / 2, cy = HEAD.y + HEAD.h / 2, s = 0.34;

    cairo_arc(cr, cx, cy, 0.5, 0, 2 * G_PI);
    set_hex(cr, 0x0c0d12, 1);
    cairo_fill(cr);

    cairo_move_to(cr, cx, cy + s);
    cairo_curve_to(cr, cx - 0.35 * s, cy + 0.85 * s, cx - 0.85 * s, cy + 0.25 * s, cx - 0.85 * s, cy - 0.25 * s);
    cairo_curve_to(cr, cx - 0.85 * s, cy - 0.8 * s, cx - 0.45 * s, cy - s, cx, cy - s);
    cairo_curve_to(cr, cx + 0.45 * s, cy - s, cx + 0.85 * s, cy - 0.8 * s, cx + 0.85 * s, cy - 0.25 * s);
    cairo_curve_to(cr, cx + 0.85 * s, cy + 0.25 * s, cx + 0.35 * s, cy + 0.85 * s, cx, cy + s);
    cairo_close_path(cr);
    if (level > 0) {
        set_source(cr, &color, 0.3 * level);
        cairo_set_line_width(cr, 0.16);
        cairo_stroke_preserve(cr);
        set_source(cr, &color, 0.35 + 0.65 * level);
    } else {
        set_hex(cr, 0x34363f, 1);
    }
    cairo_fill(cr);

    for (int side = -1; side <= 1; side += 2) {
        cairo_save(cr);
        cairo_translate(cr, cx + side * 0.36 * s, cy - 0.08 * s);
        cairo_rotate(cr, -side * 0.55);
        cairo_scale(cr, 0.34 * s, 0.15 * s);
        cairo_arc(cr, 0, 0, 1, 0, 2 * G_PI);
        cairo_restore(cr);
        set_hex(cr, 0x0c0d12, 1);
        cairo_fill(cr);
    }
}

static void draw_speaker(cairo_t *cr, const Rect *r) {
    GdkRGBA color;
    double level;
    lit_color(LIGHT_SPEAKERS, &color, &level);

    if (level > 0) {
        rounded_rect(cr, r->x - 0.1, r->y - 0.1, r->w + 0.2, r->h + 0.2, 0.3);
        set_source(cr, &color, 0.18 * level);
        cairo_fill(cr);
    }
    rounded_rect(cr, r->x, r->y, r->w, r->h, 0.22);
    set_hex(cr, 0x0c0d12, 1);
    cairo_fill(cr);

    for (int row = 0; r->y + 0.16 + row * 0.16 < r->y + r->h - 0.1; row += 1) {
        double y = r->y + 0.16 + row * 0.16;
        for (double x = r->x + 0.18 + (row % 2) * 0.1; x < r->x + r->w - 0.12; x += 0.2) {
            cairo_arc(cr, x, y, 0.05, 0, 2 * G_PI);
            cairo_close_path(cr);
        }
    }
    if (level > 0) {
        set_source(cr, &color, 0.2 + 0.8 * level);
    } else {
        set_hex(cr, 0x262731, 1);
    }
    cairo_fill(cr);
}

typedef enum { ICON_PREV, ICON_PLAY, ICON_NEXT, ICON_EJECT, ICON_VOL_DOWN, ICON_VOL_UP } MediaIcon;

static void media_icon_path(cairo_t *cr, MediaIcon icon, double cx, double cy) {
    switch (icon) {
        case ICON_PREV:
        case ICON_NEXT: {
            double d = icon == ICON_PREV ? 1 : -1;
            cairo_rectangle(cr, cx - d * 0.17 - 0.03, cy - 0.15, 0.06, 0.3);
            cairo_move_to(cr, cx + d * 0.15, cy - 0.15);
            cairo_line_to(cr, cx - d * 0.11, cy);
            cairo_line_to(cr, cx + d * 0.15, cy + 0.15);
            cairo_close_path(cr);
            break;
        }
        case ICON_PLAY:
            cairo_move_to(cr, cx - 0.11, cy - 0.17);
            cairo_line_to(cr, cx + 0.17, cy);
            cairo_line_to(cr, cx - 0.11, cy + 0.17);
            cairo_close_path(cr);
            break;
        case ICON_EJECT:
            cairo_move_to(cr, cx - 0.17, cy + 0.03);
            cairo_line_to(cr, cx, cy - 0.16);
            cairo_line_to(cr, cx + 0.17, cy + 0.03);
            cairo_close_path(cr);
            cairo_rectangle(cr, cx - 0.17, cy + 0.09, 0.34, 0.06);
            break;
        case ICON_VOL_UP:
            cairo_rectangle(cr, cx - 0.03, cy - 0.15, 0.06, 0.3);
            // fall through
        case ICON_VOL_DOWN:
            cairo_rectangle(cr, cx - 0.15, cy - 0.03, 0.3, 0.06);
            break;
    }
}

static void draw_mediabar(cairo_t *cr) {
    static const MediaIcon icons[] = {ICON_PREV, ICON_PLAY, ICON_NEXT, ICON_EJECT, ICON_VOL_DOWN, ICON_VOL_UP};
    static const double positions[] = {6.95, 7.75, 8.55, 10.45, 11.25, 12.05};
    GdkRGBA color;
    double level;
    lit_color(LIGHT_MEDIABAR, &color, &level);

    rounded_rect(cr, MEDIABAR.x, MEDIABAR.y, MEDIABAR.w, MEDIABAR.h, 0.22);
    set_hex(cr, 0x0c0d12, 1);
    cairo_fill(cr);

    for (int i = 0; i < 6; i += 1) {
        media_icon_path(cr, icons[i], positions[i], MEDIABAR.y + MEDIABAR.h / 2);
    }
    if (level > 0) {
        set_source(cr, &color, 0.3 * level);
        cairo_set_line_width(cr, 0.12);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_stroke_preserve(cr);
        set_source(cr, &color, 0.35 + 0.65 * level);
    } else {
        set_hex(cr, 0x34363f, 1);
    }
    cairo_fill(cr);
}

static void key_path(cairo_t *cr, const Key *key, double x, double y, double w, double h) {
    if (g_strcmp0(key->label, ENTER_LABEL) == 0) {
        // L shaped key spanning this row and the next one
        const double bottom_w = 1.25, r = 0.1;
        cairo_move_to(cr, x + KEY_GAP + r, y + KEY_GAP);
        cairo_line_to(cr, x + w - KEY_GAP - r, y + KEY_GAP);
        cairo_arc(cr, x + w - KEY_GAP - r, y + KEY_GAP + r, r, -G_PI / 2, 0);
        cairo_line_to(cr, x + w - KEY_GAP, y + 2 * h - KEY_GAP - r);
        cairo_arc(cr, x + w - KEY_GAP - r, y + 2 * h - KEY_GAP - r, r, 0, G_PI / 2);
        cairo_line_to(cr, x + w - bottom_w + KEY_GAP + r, y + 2 * h - KEY_GAP);
        cairo_arc(cr, x + w - bottom_w + KEY_GAP + r, y + 2 * h - KEY_GAP - r, r, G_PI / 2, G_PI);
        cairo_line_to(cr, x + w - bottom_w + KEY_GAP, y + h - KEY_GAP);
        cairo_line_to(cr, x + KEY_GAP + r, y + h - KEY_GAP);
        cairo_arc(cr, x + KEY_GAP + r, y + h - KEY_GAP - r, r, G_PI / 2, G_PI);
        cairo_line_to(cr, x + KEY_GAP, y + KEY_GAP + r);
        cairo_arc(cr, x + KEY_GAP + r, y + KEY_GAP + r, r, G_PI, 3 * G_PI / 2);
        cairo_close_path(cr);
    } else {
        rounded_rect(cr, x + KEY_GAP, y + KEY_GAP, w - 2 * KEY_GAP, h - 2 * KEY_GAP, 0.1);
    }
}

// A key placed on the keyboard, in view units
typedef struct {
    const Key *key;
    int row;
    int zone;   // LIGHT_KEYBOARD_* zone the key belongs to
    double x, y, w, h;
} KeyBox;

typedef void (*KeyFunc)(const KeyBox *box, gpointer data);

static void for_each_key(KeyFunc fn, gpointer data) {
    double y = KB_Y;
    for (int row = 0; row < ROW_COUNT; row += 1) {
        const Key *keys = ROWS[row];
        double total = 0;
        for (const Key *k = keys; k->width > 0; k += 1) {
            total += k->width;
        }
        double factor = KB_W / total;

        double x = KB_X;
        for (const Key *k = keys; k->width > 0; k += 1) {
            double w = k->width * factor;
            if (k->label != NULL) {
                KeyBox box = {
                    .key = k,
                    .row = row,
                    .zone = LIGHT_KEYBOARD_LEFT + CLAMP((int) ((x + w / 2 - KB_X) / KB_ZONE_W), 0, 3),
                    .x = x, .y = y, .w = w, .h = ROW_HEIGHTS[row],
                };
                fn(&box, data);
            }
            x += w;
        }
        y += ROW_HEIGHTS[row];
    }
}

typedef struct {
    cairo_t *cr;
    LaptopView *view;
    GdkRGBA colors[4];
    double levels[4];
} KeyboardPaint;

static void draw_key(const KeyBox *box, gpointer data) {
    KeyboardPaint *paint = data;
    cairo_t *cr = paint->cr;
    GdkRGBA *color = &paint->colors[box->zone - LIGHT_KEYBOARD_LEFT];
    double level = paint->levels[box->zone - LIGHT_KEYBOARD_LEFT];
    double x = box->x, y = box->y, w = box->w, h = box->h;

    // Backlight bleeding around the key
    key_path(cr, box->key, x, y, w, h);
    if (level > 0) {
        set_source(cr, color, 0.28 * level);
        cairo_set_line_width(cr, 0.16);
        cairo_stroke_preserve(cr);
    }
    vertical_gradient(cr, y, y + h, 0x22232e, 0x15161d);
    cairo_fill_preserve(cr);
    if (level > 0) {
        set_source(cr, color, 0.25 + 0.45 * level);
    } else {
        set_hex(cr, 0xffffff, 0.07);
    }
    cairo_set_line_width(cr, 0.025);
    cairo_stroke(cr);

    // Legend
    const char *label = box->key->label;
    if (label[0] != '\0') {
        double size = box->row == 0 ? 0.2 : (g_utf8_strlen(label, -1) > 2 ? 0.2 : 0.3);
        double ty = box->row == 0 ? y + h / 2 : y + h * 0.42;
        cairo_save(cr);
        PangoLayout *layout = create_text(cr, paint->view, label, size, PANGO_WEIGHT_BOLD, 0, x + w / 2, ty);
        if (level > 0) {
            set_source(cr, color, 0.3 + 0.7 * level);
        } else {
            set_hex(cr, 0x4a4d5c, 1);
        }
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
        cairo_restore(cr);
    }
}

static void draw_keyboard(cairo_t *cr, LaptopView *view) {
    KeyboardPaint paint = {.cr = cr, .view = view};
    for (int i = 0; i < 4; i += 1) {
        lit_color(LIGHT_KEYBOARD_LEFT + i, &paint.colors[i], &paint.levels[i]);
    }
    for_each_key(draw_key, &paint);
}

static void draw_touchpad(cairo_t *cr) {
    GdkRGBA color;
    double level;
    lit_color(LIGHT_TOUCHPAD, &color, &level);
    const Rect *r = &TOUCHPAD;

    rounded_rect(cr, r->x, r->y, r->w, r->h, 0.25);
    if (level > 0) {
        set_source(cr, &color, 0.08 * level);
        cairo_set_line_width(cr, 0.5);
        cairo_stroke_preserve(cr);
        set_source(cr, &color, 0.18 * level);
        cairo_set_line_width(cr, 0.25);
        cairo_stroke_preserve(cr);
    }
    vertical_gradient(cr, r->y, r->y + r->h, 0x1f202a, 0x17181f);
    cairo_fill_preserve(cr);
    if (level > 0) {
        set_source(cr, &color, 0.4 + 0.6 * level);
    } else {
        set_hex(cr, 0xffffff, 0.1);
    }
    cairo_set_line_width(cr, 0.06);
    cairo_stroke(cr);

    // Click buttons
    double by = r->y + r->h * 0.78;
    cairo_move_to(cr, r->x + 0.05, by);
    cairo_line_to(cr, r->x + r->w - 0.05, by);
    cairo_move_to(cr, r->x + r->w / 2, by);
    cairo_line_to(cr, r->x + r->w / 2, r->y + r->h - 0.05);
    if (level > 0) {
        set_source(cr, &color, 0.15 + 0.35 * level);
    } else {
        set_hex(cr, 0xffffff, 0.07);
    }
    cairo_set_line_width(cr, 0.03);
    cairo_stroke(cr);
}

typedef struct {
    cairo_t *cr;
    int zone;
} KeyOutline;

static void key_outline_path(const KeyBox *box, gpointer data) {
    KeyOutline *outline = data;
    if (box->zone == outline->zone) {
        key_path(outline->cr, box->key, box->x, box->y, box->w, box->h);
    }
}

static void outline_zone(cairo_t *cr, LaptopView *view, int zone, gboolean selected) {
    if (zone >= LIGHT_KEYBOARD_LEFT && zone <= LIGHT_KEYBOARD_RIGHT) {
        KeyOutline outline = {cr, zone};
        for_each_key(key_outline_path, &outline);
    } else {
        for (guint i = 0; i < HIT_REGION_COUNT; i += 1) {
            if (HIT_REGIONS[i].zone == zone) {
                const Rect *r = &HIT_REGIONS[i].rect;
                rounded_rect(cr, r->x - 0.12, r->y - 0.12, r->w + 0.24, r->h + 0.24, 0.3);
            }
        }
    }
    if (selected) {
        set_hex(cr, 0xffffff, 0.12);
        cairo_set_line_width(cr, 6 / view->scale);
        cairo_stroke_preserve(cr);
        set_hex(cr, 0xffffff, 0.9);
        cairo_set_line_width(cr, 1.5 / view->scale);
    } else {
        set_hex(cr, 0xffffff, 0.4);
        cairo_set_line_width(cr, 1.2 / view->scale);
    }
    cairo_stroke(cr);
}

static void draw_laptop(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    LaptopView *view = data;

    view->scale = MIN(width / (VIEW_WIDTH + 1.0), height / (VIEW_HEIGHT + 1.0));
    view->ox = (width - VIEW_WIDTH * view->scale) / 2;
    view->oy = (height - VIEW_HEIGHT * view->scale) / 2;

    cairo_translate(cr, view->ox, view->oy);
    cairo_scale(cr, view->scale, view->scale);

    draw_body(cr);
    draw_speaker(cr, &SPEAKER_LEFT);
    draw_speaker(cr, &SPEAKER_RIGHT);
    draw_mediabar(cr);
    draw_logo(cr, view);
    draw_power_button(cr, GTK_WIDGET(area), view);
    draw_keyboard(cr, view);
    draw_touchpad(cr);

    if (view->hover != LIGHT_NONE && view->hover != view->selection && view->selection != LIGHT_ALL) {
        outline_zone(cr, view, view->hover, FALSE);
    }
    if (view->selection == LIGHT_ALL) {
        rounded_rect(cr, DECK.x - 0.1, BEZEL.y - 0.1, DECK.w + 0.2, DECK.y + DECK.h - BEZEL.y + 0.2, 0.8);
        cairo_set_source_rgba(cr, 0, 0.706, 1, 0.9);
        cairo_set_line_width(cr, 2 / view->scale);
        cairo_stroke(cr);
    } else if (view->selection != LIGHT_NONE) {
        outline_zone(cr, view, view->selection, TRUE);
    }
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

typedef struct {
    double x, y;
    int zone;
} KeyHit;

static void key_hit(const KeyBox *box, gpointer data) {
    KeyHit *hit = data;
    // The enter key spans two rows
    double h = g_strcmp0(box->key->label, ENTER_LABEL) == 0 ? 2 * box->h : box->h;
    if (hit->x >= box->x && hit->x <= box->x + box->w && hit->y >= box->y && hit->y <= box->y + h) {
        hit->zone = box->zone;
    }
}

static int hit_test(LaptopView *view, double x, double y) {
    if (view->scale <= 0) {
        return LIGHT_NONE;
    }
    KeyHit hit = {(x - view->ox) / view->scale, (y - view->oy) / view->scale, LIGHT_NONE};
    for_each_key(key_hit, &hit);
    if (hit.zone != LIGHT_NONE) {
        return hit.zone;
    }
    for (guint i = 0; i < HIT_REGION_COUNT; i += 1) {
        const Rect *r = &HIT_REGIONS[i].rect;
        if (hit.x >= r->x && hit.x <= r->x + r->w && hit.y >= r->y && hit.y <= r->y + r->h) {
            return HIT_REGIONS[i].zone;
        }
    }
    return LIGHT_NONE;
}

static void on_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
    (void) n_press;
    LaptopView *view = data;
    int zone = hit_test(view, x, y);
    if (zone == LIGHT_NONE) {
        return;
    }
    view->selection = zone;
    gtk_widget_queue_draw(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
    if (view->on_select) {
        view->on_select(zone, view->user_data);
    }
}

static void set_hover(GtkWidget *widget, LaptopView *view, int zone) {
    if (zone == view->hover) {
        return;
    }
    view->hover = zone;
    gtk_widget_set_cursor_from_name(widget, zone != LIGHT_NONE ? "pointer" : NULL);
    gtk_widget_queue_draw(widget);
}

static void on_motion(GtkEventControllerMotion *controller, double x, double y, gpointer data) {
    LaptopView *view = data;
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    set_hover(widget, view, hit_test(view, x, y));
}

static void on_leave(GtkEventControllerMotion *controller, gpointer data) {
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    set_hover(widget, data, LIGHT_NONE);
}

static gboolean on_query_tooltip(GtkWidget *widget, int x, int y, gboolean keyboard_mode,
        GtkTooltip *tooltip, gpointer data) {
    (void) widget;
    (void) keyboard_mode;
    int zone = hit_test(data, x, y);
    if (zone == LIGHT_NONE) {
        return FALSE;
    }
    gtk_tooltip_set_text(tooltip, zone == LIGHT_POWER_BUTTON ? "Power button" : zones_get(zone)->label);
    return TRUE;
}

GtkWidget * laptop_view_new(LaptopViewSelectFunc on_select, gpointer user_data) {
    GtkWidget *area = gtk_drawing_area_new();
    LaptopView *view = g_new0(LaptopView, 1);
    view->selection = LIGHT_NONE;
    view->hover = LIGHT_NONE;
    view->power_state = -1;
    view->on_select = on_select;
    view->user_data = user_data;
    g_object_set_data_full(G_OBJECT(area), "laptop-view", view, g_free);

    gtk_widget_add_css_class(area, "laptop-view");
    gtk_widget_set_hexpand(area, TRUE);
    gtk_widget_set_vexpand(area, TRUE);
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 760);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 560);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_laptop, view, NULL);

    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "released", G_CALLBACK(on_click), view);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(click));

    GtkEventController *motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "motion", G_CALLBACK(on_motion), view);
    g_signal_connect(motion, "leave", G_CALLBACK(on_leave), view);
    gtk_widget_add_controller(area, motion);

    gtk_widget_set_has_tooltip(area, TRUE);
    g_signal_connect(area, "query-tooltip", G_CALLBACK(on_query_tooltip), view);

    return area;
}

void laptop_view_set_selection(GtkWidget *widget, int selection) {
    LaptopView *view = g_object_get_data(G_OBJECT(widget), "laptop-view");
    view->selection = selection;
    gtk_widget_queue_draw(widget);
}

void laptop_view_set_power_state(GtkWidget *widget, int state) {
    LaptopView *view = g_object_get_data(G_OBJECT(widget), "laptop-view");
    view->power_state = state;
    gtk_widget_queue_draw(widget);
}
