#include <gtk/gtk.h>
#include "lib/keyboard.h"
#include "lib/connector.h"

libusb_device_handle *usbhandle;

void color_selected(GtkWidget *color_picker, gpointer data) {
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_picker), &color);
    set_color((GtkWidget *) data, &color, usbhandle);
}

static void activate(GtkApplication *app) {
    GtkWindow *window = (GtkWindow *) gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "AlienFx");

    GtkGrid *grid = (GtkGrid *) gtk_grid_new();

    gtk_window_set_child(window, (GtkWidget *) grid);
    KeyboardWidget keyboard;
    get_keyboard(&keyboard);
    gtk_grid_attach(grid, keyboard.widget, 0, 0, 4, 1);

    GtkWidget *color_picker = gtk_color_button_new();
    gtk_grid_attach(grid, color_picker, 0, 1, 1, 1);

    g_signal_connect(color_picker, "color-set", G_CALLBACK(color_selected), keyboard.widget);
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
