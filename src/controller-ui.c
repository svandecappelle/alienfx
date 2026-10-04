#include <gtk/gtk.h>

#include "lib/laptop-view.h"
#include "lib/profile.h"
#include "lib/zones.h"
#include "lib/utils/functions.h"

// Delay before sending colors to the keyboard, so dragging a slider does not flood the USB bus
#define FLUSH_DELAY_MS 80

static const char *PALETTE[] = {
    "#00b4ff", "#2952ff", "#5b3cff", "#9b30ff", "#ff2bd6", "#ff4f9a", "#ff1a1a", "#ff7a00",
    "#ffc400", "#fff200", "#9dff00", "#00ff55", "#00ffc3", "#9ee7ff", "#ffe6c7", "#ffffff",
};
#define PALETTE_SIZE (sizeof(PALETTE) / sizeof(PALETTE[0]))

typedef struct {
    const char *name;
    // keyboard left, middle left, middle right, right, touchpad, mediabar, speakers, logo
    const char *colors[LIGHT_COUNT];
} Preset;

static const Preset PRESETS[] = {
    {"Alien Blue", {"#00b4ff", "#00b4ff", "#00b4ff", "#00b4ff", "#00b4ff", "#00b4ff", "#00b4ff", "#00ffc3"}},
    {"Nebula", {"#2952ff", "#5b3cff", "#9b30ff", "#ff2bd6", "#9b30ff", "#ff2bd6", "#5b3cff", "#00b4ff"}},
    {"Inferno", {"#ff1a1a", "#ff4000", "#ff7a00", "#ffc400", "#ff4000", "#ff7a00", "#ff1a1a", "#ffc400"}},
    {"Toxic", {"#00ff55", "#9dff00", "#00ff55", "#9dff00", "#00ff55", "#9dff00", "#00ff55", "#fff200"}},
    {"Rainbow", {"#ff1a1a", "#ffc400", "#00ff55", "#2952ff", "#9b30ff", "#00ffc3", "#ff2bd6", "#ffffff"}},
    {"Sunset", {"#ff4f9a", "#ff1a1a", "#ff7a00", "#ffc400", "#ff4f9a", "#ff7a00", "#ff1a1a", "#ffc400"}},
    {"Arctic", {"#9ee7ff", "#ffffff", "#ffffff", "#9ee7ff", "#00b4ff", "#9ee7ff", "#00b4ff", "#ffffff"}},
    {"Stealth", {"#ff1a1a", "#ff1a1a", "#ff1a1a", "#ff1a1a", "#000000", "#000000", "#000000", "#ff1a1a"}},
};
#define PRESET_COUNT (sizeof(PRESETS) / sizeof(PRESETS[0]))

typedef struct {
    libusb_device_handle *usbhandle;
    int selection;
    gboolean updating;
    guint dirty;
    guint flush_source;

    GtkWidget *view;
    GtkWidget *chips[LIGHT_COUNT + 1];
    GtkWidget *zone_name;
    GtkWidget *zone_detail;
    GtkWidget *preview;
    GtkWidget *hex_label;
    GtkWidget *bits_label;
    GtkWidget *color_button;
    GtkWidget *brightness;

    GtkWidget *profile_entry;
    GtkWidget *profile_save;
    GtkWidget *profile_feedback;
    GtkWidget *profile_list;
    GtkWidget *profile_empty;
    guint feedback_source;
} App;

static App app;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static int first_selected_zone(void) {
    return app.selection == LIGHT_ALL ? LIGHT_KEYBOARD_LEFT : app.selection;
}

// TRUE when every zone of the selection displays the same color
static gboolean selection_is_uniform(void) {
    guint mask = zones_selection_mask(app.selection);
    GdkRGBA reference;
    zones_get_effective_color(first_selected_zone(), &reference);
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        GdkRGBA c;
        zones_get_effective_color(i, &c);
        if ((mask & (1u << i)) && !gdk_rgba_equal(&c, &reference)) {
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean flush(gpointer data) {
    (void) data;
    zones_write(app.usbhandle, app.dirty);
    zones_save();
    app.dirty = 0;
    app.flush_source = 0;
    return G_SOURCE_REMOVE;
}

static void schedule_flush(guint mask) {
    app.dirty |= mask;
    if (app.flush_source == 0) {
        app.flush_source = g_timeout_add(FLUSH_DELAY_MS, flush, NULL);
    }
}

static void refresh_colors(void) {
    int zone = first_selected_zone();
    int r, g, b;
    zones_get_hardware_color(zone, &r, &g, &b);

    if (selection_is_uniform()) {
        char *hex = g_strdup_printf("#%02X%02X%02X", r * 17, g * 17, b * 17);
        char *bits = g_strdup_printf("R %d · G %d · B %d", r, g, b);
        gtk_label_set_text(GTK_LABEL(app.hex_label), r + g + b == 0 ? "Off" : hex);
        gtk_label_set_text(GTK_LABEL(app.bits_label), bits);
        g_free(hex);
        g_free(bits);
    } else {
        gtk_label_set_text(GTK_LABEL(app.hex_label), "Mixed");
        gtk_label_set_text(GTK_LABEL(app.bits_label), "zones have different colors");
    }

    gtk_widget_queue_draw(app.preview);
    gtk_widget_queue_draw(app.view);
}

static void refresh_panel(void) {
    app.updating = TRUE;

    if (app.selection == LIGHT_ALL) {
        gtk_label_set_text(GTK_LABEL(app.zone_name), "All zones");
        gtk_label_set_text(GTK_LABEL(app.zone_detail), "Every light of the laptop at once");
    } else {
        LightZoneState *z = zones_get(app.selection);
        gtk_label_set_text(GTK_LABEL(app.zone_name), z->label);
        gtk_label_set_text(GTK_LABEL(app.zone_detail), z->detail);
    }

    LightZoneState *z = zones_get(first_selected_zone());
    gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(app.color_button), &z->color);
    gtk_range_set_value(GTK_RANGE(app.brightness), z->brightness * 100);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.chips[app.selection]), TRUE);

    app.updating = FALSE;
    refresh_colors();
}

static void select_zone(int selection) {
    app.selection = selection;
    laptop_view_set_selection(app.view, selection);
    refresh_panel();
}

static void apply_color(const GdkRGBA *color) {
    zones_set_color(app.selection, color);
    // Picking a color on a switched off zone turns it back on
    LightZoneState *z = zones_get(first_selected_zone());
    if (z->brightness == 0) {
        zones_set_brightness(app.selection, 1.0);
    }
    schedule_flush(zones_selection_mask(app.selection));
    refresh_panel();
}

// ---------------------------------------------------------------------------
// Signal handlers
// ---------------------------------------------------------------------------

static void on_view_select(int zone, gpointer data) {
    (void) data;
    select_zone(zone);
}

static void on_chip_toggled(GtkToggleButton *button, gpointer data) {
    if (app.updating || !gtk_toggle_button_get_active(button)) {
        return;
    }
    select_zone(GPOINTER_TO_INT(data));
}

static void on_swatch_clicked(GtkButton *button, gpointer data) {
    (void) button;
    GdkRGBA color;
    gdk_rgba_parse(&color, PALETTE[GPOINTER_TO_INT(data)]);
    apply_color(&color);
}

static void on_custom_color(GObject *button, GParamSpec *pspec, gpointer data) {
    (void) pspec;
    (void) data;
    if (app.updating) {
        return;
    }
    apply_color(gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(button)));
}

static void on_brightness_changed(GtkRange *range, gpointer data) {
    (void) data;
    if (app.updating) {
        return;
    }
    zones_set_brightness(app.selection, gtk_range_get_value(range) / 100);
    schedule_flush(zones_selection_mask(app.selection));
    refresh_colors();
}

static char * format_percent(GtkScale *scale, double value, gpointer data) {
    (void) scale;
    (void) data;
    return g_strdup_printf("%.0f%%", value);
}

static void on_apply_all(GtkButton *button, gpointer data) {
    (void) button;
    (void) data;
    LightZoneState *z = zones_get(first_selected_zone());
    GdkRGBA color = z->color;
    double brightness = z->brightness;
    zones_set_color(LIGHT_ALL, &color);
    zones_set_brightness(LIGHT_ALL, brightness);
    schedule_flush(zones_selection_mask(LIGHT_ALL));
    select_zone(LIGHT_ALL);
}

static void on_turn_off(GtkButton *button, gpointer data) {
    (void) button;
    (void) data;
    zones_set_brightness(app.selection, 0);
    schedule_flush(zones_selection_mask(app.selection));
    refresh_panel();
}

static void on_preset_clicked(GtkButton *button, gpointer data) {
    (void) button;
    const Preset *preset = &PRESETS[GPOINTER_TO_INT(data)];
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        GdkRGBA color;
        gdk_rgba_parse(&color, preset->colors[i]);
        zones_set_color(i, &color);
        zones_set_brightness(i, 1.0);
    }
    schedule_flush(zones_selection_mask(LIGHT_ALL));
    refresh_panel();
}

static void draw_preview(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    (void) area;
    (void) data;
    const double r = 12;

    cairo_new_sub_path(cr);
    cairo_arc(cr, width - r, r, r, -G_PI / 2, 0);
    cairo_arc(cr, width - r, height - r, r, 0, G_PI / 2);
    cairo_arc(cr, r, height - r, r, G_PI / 2, G_PI);
    cairo_arc(cr, r, r, r, G_PI, 3 * G_PI / 2);
    cairo_close_path(cr);

    // One stop per zone of the selection, a single color when they all match
    cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 0, width, 0);
    guint mask = selection_is_uniform() ? zones_selection_mask(first_selected_zone()) : zones_selection_mask(app.selection);
    int count = 0;
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        count += (mask & (1u << i)) != 0;
    }
    int n = 0;
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        if (mask & (1u << i)) {
            GdkRGBA c;
            zones_get_effective_color(i, &c);
            double offset = count == 1 ? 0 : (double) n / (count - 1);
            cairo_pattern_add_color_stop_rgb(pattern, offset, c.red, c.green, c.blue);
            if (count == 1) {
                cairo_pattern_add_color_stop_rgb(pattern, 1, c.red, c.green, c.blue);
            }
            n += 1;
        }
    }
    cairo_set_source(cr, pattern);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pattern);

    // Glossy highlight
    cairo_pattern_t *gloss = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgba(gloss, 0, 1, 1, 1, 0.22);
    cairo_pattern_add_color_stop_rgba(gloss, 0.5, 1, 1, 1, 0);
    cairo_set_source(cr, gloss);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(gloss);

    cairo_set_source_rgba(cr, 1, 1, 1, 0.12);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
}

static void on_shutdown(GApplication *application, gpointer data) {
    (void) application;
    (void) data;
    if (app.flush_source != 0) {
        g_source_remove(app.flush_source);
        flush(NULL);
    }
}

// ---------------------------------------------------------------------------
// Widgets
// ---------------------------------------------------------------------------

static GtkWidget * section_title(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_widget_add_css_class(label, "section-title");
    return label;
}

static void load_css(void) {
    GdkDisplay *display = gdk_display_get_default();

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, "/org/alienfx/ui/css/gtk.css");
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // Colors of the swatches and presets come from the tables above
    GString *css = g_string_new(NULL);
    for (guint i = 0; i < PALETTE_SIZE; i += 1) {
        g_string_append_printf(css, ".swatch-%u { background: %s; }\n", i, PALETTE[i]);
    }
    for (guint i = 0; i < PRESET_COUNT; i += 1) {
        const char **c = (const char **) PRESETS[i].colors;
        g_string_append_printf(css, ".preset-bar-%u { background-image: linear-gradient(90deg, %s, %s, %s, %s); }\n",
                i, c[0], c[1], c[2], c[3]);
    }
    GtkCssProvider *generated = gtk_css_provider_new();
    gtk_css_provider_load_from_string(generated, css->str);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(generated),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    g_object_unref(generated);
    g_string_free(css, TRUE);
}

static GtkWidget * build_header(void) {
    GtkWidget *header = gtk_header_bar_new();

    GtkWidget *titles = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(titles, GTK_ALIGN_CENTER);
    GtkWidget *title = gtk_label_new("ALIENFX");
    gtk_widget_add_css_class(title, "app-title");
    GtkWidget *subtitle = gtk_label_new("Lighting control center");
    gtk_widget_add_css_class(subtitle, "app-subtitle");
    gtk_box_append(GTK_BOX(titles), title);
    gtk_box_append(GTK_BOX(titles), subtitle);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), titles);

    GtkWidget *status = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_valign(status, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(status, "status");
    GtkWidget *dot = gtk_label_new("●");
    gtk_widget_add_css_class(dot, "dot");
    GtkWidget *text;
    if (app.usbhandle != NULL) {
        gtk_widget_add_css_class(status, "online");
        text = gtk_label_new("M14x connected");
    } else {
        gtk_widget_add_css_class(status, "offline");
        text = gtk_label_new("Preview mode");
        gtk_widget_set_tooltip_text(status,
                "No AlienFX device found or missing USB permissions.\nColors are only previewed (see README).");
    }
    gtk_box_append(GTK_BOX(status), dot);
    gtk_box_append(GTK_BOX(status), text);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), status);

    return header;
}

static GtkWidget * build_zone_chips(void) {
    GtkWidget *flow = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow), 3);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow), 6);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow), 6);

    GtkToggleButton *group = NULL;
    for (int n = 0; n <= LIGHT_COUNT; n += 1) {
        // "All zones" first, then the zones in order
        int zone = n == 0 ? LIGHT_ALL : n - 1;
        const char *label = zone == LIGHT_ALL ? "All zones" : zones_get(zone)->label;
        GtkWidget *chip = gtk_toggle_button_new_with_label(label);
        gtk_widget_add_css_class(chip, "chip");
        if (group != NULL) {
            gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(chip), group);
        } else {
            group = GTK_TOGGLE_BUTTON(chip);
        }
        g_signal_connect(chip, "toggled", G_CALLBACK(on_chip_toggled), GINT_TO_POINTER(zone));
        app.chips[zone] = chip;
        gtk_flow_box_append(GTK_FLOW_BOX(flow), chip);
    }
    return flow;
}

static GtkWidget * build_color_card(void) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(card, "card");

    app.zone_name = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(app.zone_name), 0);
    gtk_widget_add_css_class(app.zone_name, "zone-name");
    app.zone_detail = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(app.zone_detail), 0);
    gtk_widget_add_css_class(app.zone_detail, "zone-detail");

    app.preview = gtk_drawing_area_new();
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(app.preview), 64);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(app.preview), draw_preview, NULL, NULL);
    gtk_widget_set_margin_top(app.preview, 10);
    gtk_widget_set_margin_bottom(app.preview, 6);

    GtkWidget *values = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    app.hex_label = gtk_label_new(NULL);
    gtk_widget_add_css_class(app.hex_label, "hex");
    app.bits_label = gtk_label_new(NULL);
    gtk_widget_add_css_class(app.bits_label, "bits");
    gtk_widget_set_hexpand(app.bits_label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(app.bits_label), 1);
    gtk_box_append(GTK_BOX(values), app.hex_label);
    gtk_box_append(GTK_BOX(values), app.bits_label);

    gtk_box_append(GTK_BOX(card), app.zone_name);
    gtk_box_append(GTK_BOX(card), app.zone_detail);
    gtk_box_append(GTK_BOX(card), app.preview);
    gtk_box_append(GTK_BOX(card), values);
    return card;
}

static GtkWidget * build_palette(void) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    for (guint i = 0; i < PALETTE_SIZE; i += 1) {
        GtkWidget *swatch = gtk_button_new();
        char *css_class = g_strdup_printf("swatch-%u", i);
        gtk_widget_add_css_class(swatch, "swatch");
        gtk_widget_add_css_class(swatch, css_class);
        gtk_widget_set_tooltip_text(swatch, PALETTE[i]);
        gtk_widget_set_halign(swatch, GTK_ALIGN_CENTER);
        g_signal_connect(swatch, "clicked", G_CALLBACK(on_swatch_clicked), GINT_TO_POINTER(i));
        gtk_grid_attach(GTK_GRID(grid), swatch, i % 8, i / 8, 1, 1);
        g_free(css_class);
    }
    return grid;
}

static GtkWidget * build_custom_color(void) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(row, 4);
    GtkWidget *label = gtk_label_new("Custom color");
    gtk_widget_set_hexpand(label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_widget_add_css_class(label, "row-label");

    GtkColorDialog *dialog = gtk_color_dialog_new();
    gtk_color_dialog_set_with_alpha(dialog, FALSE);
    gtk_color_dialog_set_title(dialog, "Pick a lighting color");
    app.color_button = gtk_color_dialog_button_new(dialog);
    gtk_widget_add_css_class(app.color_button, "custom-color");
    g_signal_connect(app.color_button, "notify::rgba", G_CALLBACK(on_custom_color), NULL);

    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), app.color_button);
    return row;
}

static GtkWidget * build_actions(void) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_set_homogeneous(GTK_BOX(row), TRUE);
    gtk_widget_set_margin_top(row, 14);

    GtkWidget *all = gtk_button_new_with_label("Apply to all zones");
    gtk_widget_add_css_class(all, "action");
    gtk_widget_add_css_class(all, "primary");
    g_signal_connect(all, "clicked", G_CALLBACK(on_apply_all), NULL);

    GtkWidget *off = gtk_button_new_with_label("Turn off");
    gtk_widget_add_css_class(off, "action");
    g_signal_connect(off, "clicked", G_CALLBACK(on_turn_off), NULL);

    gtk_box_append(GTK_BOX(row), all);
    gtk_box_append(GTK_BOX(row), off);
    return row;
}

static GtkWidget * build_presets(void) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    for (guint i = 0; i < PRESET_COUNT; i += 1) {
        GtkWidget *button = gtk_button_new();
        gtk_widget_add_css_class(button, "preset");

        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        char *css_class = g_strdup_printf("preset-bar-%u", i);
        gtk_widget_add_css_class(bar, "preset-bar");
        gtk_widget_add_css_class(bar, css_class);
        g_free(css_class);
        GtkWidget *name = gtk_label_new(PRESETS[i].name);
        gtk_label_set_xalign(GTK_LABEL(name), 0);
        gtk_widget_add_css_class(name, "preset-name");
        gtk_box_append(GTK_BOX(content), bar);
        gtk_box_append(GTK_BOX(content), name);
        gtk_button_set_child(GTK_BUTTON(button), content);

        g_signal_connect(button, "clicked", G_CALLBACK(on_preset_clicked), GINT_TO_POINTER(i));
        gtk_grid_attach(GTK_GRID(grid), button, i % 2, i / 2, 1, 1);
    }
    return grid;
}

// ---------------------------------------------------------------------------
// Profiles (the same files as the command line tool)
// ---------------------------------------------------------------------------

// UI zones and profile zones are the same, in the same order
G_STATIC_ASSERT((int) LIGHT_COUNT == (int) PROFILE_ZONE_COUNT);

static void refresh_profiles(void);

static gboolean hide_feedback(gpointer data) {
    (void) data;
    gtk_widget_set_visible(app.profile_feedback, FALSE);
    app.feedback_source = 0;
    return G_SOURCE_REMOVE;
}

static void show_feedback(const char *message, gboolean error) {
    gtk_label_set_text(GTK_LABEL(app.profile_feedback), message);
    if (error) {
        gtk_widget_add_css_class(app.profile_feedback, "error");
    } else {
        gtk_widget_remove_css_class(app.profile_feedback, "error");
    }
    gtk_widget_set_visible(app.profile_feedback, TRUE);
    if (app.feedback_source != 0) {
        g_source_remove(app.feedback_source);
    }
    app.feedback_source = g_timeout_add_seconds(error ? 6 : 3, hide_feedback, NULL);
}

static gboolean profile_exists(const char *name) {
    char path[4096];
    return profile_valid_name(name)
        && profile_path(name, path, sizeof(path)) == 0
        && g_file_test(path, G_FILE_TEST_EXISTS);
}

static void update_save_button(void) {
    const char *name = gtk_editable_get_text(GTK_EDITABLE(app.profile_entry));
    gboolean valid = profile_valid_name(name);

    gtk_widget_set_sensitive(app.profile_save, valid);
    gtk_button_set_label(GTK_BUTTON(app.profile_save), valid && profile_exists(name) ? "Overwrite" : "Save");
    if (name[0] != '\0' && !valid) {
        gtk_widget_add_css_class(app.profile_entry, "error");
        gtk_widget_set_tooltip_text(app.profile_entry, "Use letters, digits, '.', '_' or '-'");
    } else {
        gtk_widget_remove_css_class(app.profile_entry, "error");
        gtk_widget_set_tooltip_text(app.profile_entry, NULL);
    }
}

static void on_profile_name_changed(GtkEditable *editable, gpointer data) {
    (void) editable;
    (void) data;
    update_save_button();
}

static void on_profile_save(GtkWidget *widget, gpointer data) {
    (void) widget;
    (void) data;
    const char *name = gtk_editable_get_text(GTK_EDITABLE(app.profile_entry));
    if (!profile_valid_name(name)) {
        return;
    }

    Profile profile = {0};
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        profile.set[i] = 1;
        zones_get_hardware_color(i, &profile.rgb[i][0], &profile.rgb[i][1], &profile.rgb[i][2]);
    }
    if (profile_save(name, &profile) != 0) {
        show_feedback(profile_last_error(), TRUE);
        return;
    }

    char *message = g_strdup_printf("Saved “%s”", name);
    show_feedback(message, FALSE);
    g_free(message);
    refresh_profiles();
    update_save_button();
}

static void apply_profile(const char *name) {
    Profile profile = {0};
    if (profile_load(name, &profile) != 0) {
        show_feedback(profile_last_error(), TRUE);
        return;
    }

    guint mask = 0;
    for (int i = 0; i < LIGHT_COUNT; i += 1) {
        if (profile.set[i]) {
            GdkRGBA color = {profile.rgb[i][0] / 15.0, profile.rgb[i][1] / 15.0, profile.rgb[i][2] / 15.0, 1};
            zones_set_color(i, &color);
            zones_set_brightness(i, 1.0);
            mask |= 1u << i;
        }
    }
    schedule_flush(mask);
    refresh_panel();

    // Saving again after a tweak overwrites the profile
    gtk_editable_set_text(GTK_EDITABLE(app.profile_entry), name);
    char *message = g_strdup_printf("Applied “%s”", name);
    show_feedback(message, FALSE);
    g_free(message);
}

static void on_profile_activated(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    (void) box;
    (void) data;
    apply_profile(g_object_get_data(G_OBJECT(row), "profile-name"));
}

static gboolean disarm_delete(gpointer data) {
    GtkWidget *button = data;
    g_object_set_data(G_OBJECT(button), "armed", NULL);
    gtk_button_set_icon_name(GTK_BUTTON(button), "user-trash-symbolic");
    gtk_widget_remove_css_class(button, "armed");
    return G_SOURCE_REMOVE;
}

// First click arms the button, a second click within 3 seconds deletes
static void on_profile_delete(GtkButton *button, gpointer data) {
    (void) data;
    const char *name = g_object_get_data(G_OBJECT(button), "profile-name");

    if (g_object_get_data(G_OBJECT(button), "armed") == NULL) {
        g_object_set_data(G_OBJECT(button), "armed", GINT_TO_POINTER(1));
        gtk_button_set_label(button, "Delete?");
        gtk_widget_add_css_class(GTK_WIDGET(button), "armed");
        g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, 3, disarm_delete, g_object_ref(button), g_object_unref);
        return;
    }

    if (profile_delete(name) != 0) {
        show_feedback(profile_last_error(), TRUE);
        return;
    }
    char *message = g_strdup_printf("Deleted “%s”", name);
    show_feedback(message, FALSE);
    g_free(message);
    refresh_profiles();
    update_save_button();
}

// One block per zone: color when lit, dark when off, outline when the profile leaves it unchanged
static void draw_profile_strip(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    (void) area;
    const Profile *profile = data;
    const double gap = 3, group_gap = 6;
    double block = (width - 6 * gap - group_gap) / PROFILE_ZONE_COUNT;

    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        // Small extra gap between the keyboard zones and the others
        double x = i * (block + gap) + (i >= PROFILE_TOUCHPAD ? group_gap - gap : 0);
        double r = 3;
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + block - r, r, r, -G_PI / 2, 0);
        cairo_arc(cr, x + block - r, height - r, r, 0, G_PI / 2);
        cairo_arc(cr, x + r, height - r, r, G_PI / 2, G_PI);
        cairo_arc(cr, x + r, r, r, G_PI, 3 * G_PI / 2);
        cairo_close_path(cr);

        const int *rgb = profile->rgb[i];
        if (!profile->set[i]) {
            cairo_set_source_rgba(cr, 1, 1, 1, 0.18);
            cairo_set_line_width(cr, 1);
            cairo_stroke(cr);
        } else if (rgb[0] + rgb[1] + rgb[2] == 0) {
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.13);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.1);
            cairo_set_line_width(cr, 1);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgb(cr, rgb[0] / 15.0, rgb[1] / 15.0, rgb[2] / 15.0);
            cairo_fill(cr);
        }
    }
}

static void add_profile_row(const char *name, void *data) {
    (void) data;
    Profile *profile = g_new0(Profile, 1);
    gboolean readable = profile_load(name, profile) == 0;

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *strip = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(strip), 112);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(strip), 14);
    gtk_widget_set_valign(strip, GTK_ALIGN_CENTER);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(strip), draw_profile_strip, profile, g_free);

    GtkWidget *label = gtk_label_new(name);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_add_css_class(label, "profile-name");
    if (!readable) {
        gtk_widget_add_css_class(label, "unreadable");
        gtk_widget_set_tooltip_text(label, profile_last_error());
    }

    GtkWidget *remove = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(remove, "flat");
    gtk_widget_add_css_class(remove, "profile-delete");
    gtk_widget_set_tooltip_text(remove, "Delete profile");
    gtk_widget_set_valign(remove, GTK_ALIGN_CENTER);
    g_object_set_data_full(G_OBJECT(remove), "profile-name", g_strdup(name), g_free);
    g_signal_connect(remove, "clicked", G_CALLBACK(on_profile_delete), NULL);

    gtk_box_append(GTK_BOX(content), strip);
    gtk_box_append(GTK_BOX(content), label);
    gtk_box_append(GTK_BOX(content), remove);

    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), content);
    gtk_widget_set_tooltip_text(row, "Apply this profile");
    g_object_set_data_full(G_OBJECT(row), "profile-name", g_strdup(name), g_free);
    gtk_list_box_append(GTK_LIST_BOX(app.profile_list), row);
}

static void refresh_profiles(void) {
    gtk_list_box_remove_all(GTK_LIST_BOX(app.profile_list));
    int count = profile_list(add_profile_row, NULL);
    gtk_widget_set_visible(app.profile_list, count > 0);
    gtk_widget_set_visible(app.profile_empty, count <= 0);
}

// Profiles saved by the command line tool show up when coming back to the window
static void on_window_active(GObject *window, GParamSpec *pspec, gpointer data) {
    (void) pspec;
    (void) data;
    if (gtk_window_is_active(GTK_WINDOW(window))) {
        refresh_profiles();
        update_save_button();
    }
}

static GtkWidget * build_profiles(void) {
    GtkWidget *section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *save_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    app.profile_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.profile_entry), "Name the current colors");
    gtk_entry_set_max_length(GTK_ENTRY(app.profile_entry), 64);
    gtk_widget_set_hexpand(app.profile_entry, TRUE);
    gtk_widget_add_css_class(app.profile_entry, "profile-entry");
    g_signal_connect(app.profile_entry, "changed", G_CALLBACK(on_profile_name_changed), NULL);
    g_signal_connect(app.profile_entry, "activate", G_CALLBACK(on_profile_save), NULL);

    app.profile_save = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(app.profile_save, "action");
    gtk_widget_add_css_class(app.profile_save, "primary");
    g_signal_connect(app.profile_save, "clicked", G_CALLBACK(on_profile_save), NULL);
    gtk_box_append(GTK_BOX(save_row), app.profile_entry);
    gtk_box_append(GTK_BOX(save_row), app.profile_save);

    app.profile_feedback = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(app.profile_feedback), 0);
    gtk_label_set_wrap(GTK_LABEL(app.profile_feedback), TRUE);
    gtk_widget_add_css_class(app.profile_feedback, "feedback");
    gtk_widget_set_visible(app.profile_feedback, FALSE);

    app.profile_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app.profile_list), GTK_SELECTION_NONE);
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(app.profile_list), TRUE);
    gtk_widget_add_css_class(app.profile_list, "profile-list");
    g_signal_connect(app.profile_list, "row-activated", G_CALLBACK(on_profile_activated), NULL);

    app.profile_empty = gtk_label_new("No saved profile yet. Name the current colors and save them to reuse them later, from here or with controller --load.");
    gtk_label_set_xalign(GTK_LABEL(app.profile_empty), 0);
    gtk_label_set_wrap(GTK_LABEL(app.profile_empty), TRUE);
    gtk_widget_add_css_class(app.profile_empty, "empty");

    gtk_box_append(GTK_BOX(section), save_row);
    gtk_box_append(GTK_BOX(section), app.profile_feedback);
    gtk_box_append(GTK_BOX(section), app.profile_list);
    gtk_box_append(GTK_BOX(section), app.profile_empty);

    refresh_profiles();
    update_save_button();
    return section;
}

static GtkWidget * build_side_panel(void) {
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(panel, "panel-content");

    gtk_box_append(GTK_BOX(panel), section_title("ZONE"));
    gtk_box_append(GTK_BOX(panel), build_zone_chips());
    gtk_box_append(GTK_BOX(panel), build_color_card());

    gtk_box_append(GTK_BOX(panel), section_title("COLOR"));
    gtk_box_append(GTK_BOX(panel), build_palette());
    gtk_box_append(GTK_BOX(panel), build_custom_color());

    gtk_box_append(GTK_BOX(panel), section_title("BRIGHTNESS"));
    app.brightness = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(app.brightness), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(app.brightness), GTK_POS_RIGHT);
    gtk_scale_set_format_value_func(GTK_SCALE(app.brightness), format_percent, NULL, NULL);
    g_signal_connect(app.brightness, "value-changed", G_CALLBACK(on_brightness_changed), NULL);
    gtk_box_append(GTK_BOX(panel), app.brightness);
    gtk_box_append(GTK_BOX(panel), build_actions());

    gtk_box_append(GTK_BOX(panel), section_title("MY PROFILES"));
    gtk_box_append(GTK_BOX(panel), build_profiles());

    gtk_box_append(GTK_BOX(panel), section_title("PRESETS"));
    gtk_box_append(GTK_BOX(panel), build_presets());

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), panel);
    gtk_widget_set_size_request(scroll, 360, -1);
    gtk_widget_add_css_class(scroll, "side-panel");
    return scroll;
}

static void activate(GtkApplication *application) {
    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, NULL);
    load_css();

    GtkWidget *window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window), "AlienFX");
    gtk_window_set_default_size(GTK_WINDOW(window), 1320, 820);
    gtk_widget_add_css_class(window, "alienfx");
    gtk_window_set_titlebar(GTK_WINDOW(window), build_header());
    g_signal_connect(window, "notify::is-active", G_CALLBACK(on_window_active), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *strip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(strip, "accent-strip");
    gtk_box_append(GTK_BOX(root), strip);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(content, TRUE);

    GtkWidget *stage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(stage, "stage");
    app.view = laptop_view_new(on_view_select, NULL);
    GtkWidget *hint = gtk_label_new("Click a zone on the laptop to select it · the keyboard shows 16 levels per channel, the preview matches it");
    gtk_widget_add_css_class(hint, "hint");
    gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
    gtk_box_append(GTK_BOX(stage), app.view);
    gtk_box_append(GTK_BOX(stage), hint);

    gtk_box_append(GTK_BOX(content), stage);
    gtk_box_append(GTK_BOX(content), build_side_panel());
    gtk_box_append(GTK_BOX(root), content);
    gtk_window_set_child(GTK_WINDOW(window), root);

    select_zone(app.selection);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    app.usbhandle = try_connect_usb();
    app.selection = LIGHT_ALL;
    zones_load();

    GtkApplication *application = gtk_application_new("org.alienfx.Controller", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(application, "shutdown", G_CALLBACK(on_shutdown), NULL);

    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
