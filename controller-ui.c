#include <gtk/gtk.h>
#include "ui/keyboard.h"

static void activate(GtkApplication *app) {
    GtkWindow *window = (GtkWindow *) gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "AlienFx");

    GtkBox *box = (GtkBox *) gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_window_set_child(window, (GtkWidget *) box);
    gtk_box_append(box, (GtkWidget *) get_keyboard());

    gtk_widget_show((GtkWidget *) window);
}

int main(int argc, char **argv) {
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
