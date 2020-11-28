#include <gtk/gtk.h>
#include <gtk/gtkbuilder.h>

static void activate(GtkApplication* app, gpointer user_data) {
    GtkBuilder* builder = gtk_builder_new_from_resource("/n64/main_window.ui");
    GtkApplicationWindow* window = GTK_APPLICATION_WINDOW(gtk_builder_get_object(builder, "mainwindow"));
    gtk_application_add_window(app, GTK_WINDOW(window));
    gtk_widget_show(GTK_WIDGET(window));
}

int main(int argc, char** argv) {
    GtkApplication* app;
    int status;
    app = gtk_application_new("com.dillonbeliveau.n64", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    printf("passed it\n");
    g_object_unref(app);

    return status;
}