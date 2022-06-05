#include <gtk/gtk.h>
#include "lib/keyboard.h"
#include "lib/utils/functions.h"

libusb_device_handle *usbhandle;
LighteningWidget touchpad;
LighteningWidget keyboard;
LighteningWidget mediabar;

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


void set_colors_keyboard(LighteningWidget *w) {
    GdkRGBA *colors[4] = {
        &left_color, &middle_left_color, &middle_right_color, &right_color
    };
    for (int i =0; i<4; i+=1) {
        if (colors[i]) {
            // printf("zone key: %s colors ; %s\n", zones[i], keyboard_colors[i]->red);
            set_color(w, colors[i], usbhandle, zones[i]);
       }
    }
}

void color_selected_left(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &left_color);
    set_colors_keyboard(w);
}

void color_selected_middle_left(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &middle_left_color);
    set_colors_keyboard(w);
}

void color_selected_middle_right(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &middle_right_color);
    set_colors_keyboard(w);
}

void color_selected_right(GtkWidget *color_picker, gpointer data) {
    LighteningWidget *w = data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &right_color);
    set_colors_keyboard(w);
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

    GtkCssProvider *css_provider = NULL;

    css_provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_path(css_provider, "src/resources/css/gtk.css");

    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
            GTK_STYLE_PROVIDER (css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkGrid *grid = (GtkGrid *) gtk_grid_new();
    gtk_window_set_child(window, (GtkWidget *) grid);

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

    // All
    GtkWidget *color_picker = gtk_color_button_new();
    gtk_grid_attach(grid, color_picker, 0, 4, 8, 1);

    g_signal_connect(color_picker, "color-set", G_CALLBACK(color_selected), &keyboard);
    g_signal_connect(color_picker, "color-set", G_CALLBACK(color_selected), &touchpad);
    g_signal_connect(color_picker, "color-set", G_CALLBACK(color_selected), &mediabar);

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
