#include <gtk/gtk.h>
#include "layout/color_selector.h"
#include "lib/keyboard.h"
#include "lib/utils/functions.h"

libusb_device_handle *usbhandle;
LighteningWidget touchpad;
LighteningWidget keyboard;
LighteningWidget mediabar;

const int LEFT = 0;
const int MIDDLE_LEFT = 1;
const int MIDDLE_RIGHT = 2;
const int RIGHT = 3;

char * zones[4] = {
    "left",
    "middle_left",
    "middle_right",
    "right"
};

GdkRGBA left_color;
GdkRGBA middle_left_color;
GdkRGBA middle_right_color;
GdkRGBA right_color;

void set_colors_keyboard(LighteningWidget *w, int zone) {
    GdkRGBA *colors[4] = {
        &left_color, &middle_left_color, &middle_right_color, &right_color
    };
    for (int i =0; i<4; i+=1) {
        if (colors[i] && i == zone) {
            printf("zone key: %s colors ; %f\n", zones[i], colors[i]->red);
            set_color(w, colors[i], usbhandle, zones[i]);
       }
    }
}

void color_selected_left(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &left_color);
    set_colors_keyboard(w, LEFT);
}

void color_selected_middle_left(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &middle_left_color);
    set_colors_keyboard(w, MIDDLE_LEFT);
}

void color_selected_middle_right(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &middle_right_color);
    set_colors_keyboard(w, MIDDLE_RIGHT);
}

void color_selected_right(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &right_color);
    set_colors_keyboard(w, RIGHT);
}

void color_selected(GtkWidget *color_picker, gpointer data) {
    GdkRGBA color;
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &color);

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &left_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &middle_left_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &middle_right_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &right_color);

    set_color(w, &color, usbhandle, NULL);
}

static void activate(GtkApplication *app) {
    GtkWindow *window = (GtkWindow *) gtk_application_window_new(app);
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "AlienFx");
    gtk_window_maximize(GTK_WINDOW(window));

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *all_zones_color_button = gtk_color_button_new();
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), all_zones_color_button);
    gtk_window_set_titlebar(window, header);

    GtkCssProvider *css_provider = NULL;

    css_provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_path(css_provider, "src/resources/css/gtk.css");

    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
            GTK_STYLE_PROVIDER (css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkFixed *content = (GtkFixed *) gtk_fixed_new();
    GtkGrid *grid = (GtkGrid *) gtk_grid_new();
    gtk_fixed_put(content, GTK_WIDGET(grid), 0, 0);
    gtk_window_set_child(window, (GtkWidget *) content);
    gtk_fixed_put(content, color_selectors_widget_new(), 0, 0);

    get_mediabar(&mediabar);
    gtk_grid_attach(grid, mediabar.widget, 0, 0, 2, 1);

    get_keyboard(&keyboard);
    gtk_grid_attach(grid, keyboard.widget, 0, 1, 8, 1);

    get_touchpad(&touchpad);
    gtk_grid_attach(grid, touchpad.widget, 0, 3, 8, 1);

    // color selections
    // Keys zones
    GtkWidget *color_picker_left = gtk_color_button_new();
    gtk_grid_attach(grid, color_picker_left, 0, 2, 2, 1);

    GtkWidget *color_picker_middle_left = gtk_color_button_new();
    gtk_grid_attach(grid, color_picker_middle_left, 2, 2, 2, 1);

    GtkWidget *color_picker_middle_right = gtk_color_button_new();
    gtk_grid_attach(grid, color_picker_middle_right, 4, 2, 2, 1);

    GtkWidget *color_picker_right = gtk_color_button_new();
    gtk_grid_attach(grid, color_picker_right, 6, 2, 2, 1);


    // TODO set all zones not only keyboard
    g_signal_connect(all_zones_color_button, "color-set", G_CALLBACK(color_selected), &keyboard);
    g_signal_connect(all_zones_color_button, "color-set", G_CALLBACK(color_selected), &touchpad);
    g_signal_connect(all_zones_color_button, "color-set", G_CALLBACK(color_selected), &mediabar);

    g_signal_connect(color_picker_left, "color-set", G_CALLBACK(color_selected_left), &keyboard);
    g_signal_connect(color_picker_middle_left, "color-set", G_CALLBACK(color_selected_middle_left), &keyboard);
    g_signal_connect(color_picker_middle_right, "color-set", G_CALLBACK(color_selected_middle_right), &keyboard);
    g_signal_connect(color_picker_right, "color-set", G_CALLBACK(color_selected_right), &keyboard);

    gtk_widget_show((GtkWidget *) window);
}

int main(int argc, char **argv) {
    usbhandle = connect_usb();
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example",
            G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate",
            G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return (status);
}
