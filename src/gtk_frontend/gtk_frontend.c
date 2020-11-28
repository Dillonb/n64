#include <gtk/gtk.h>
#include <gtk/gtkbuilder.h>
#include <gdk/gdkx.h>
#include <log.h>
#include <system/n64system.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <mem/pif.h>

static void activate(GtkApplication* app, gpointer user_data) {
    GtkBuilder* builder = gtk_builder_new_from_resource("/n64/main_window.ui");
    GtkApplicationWindow* window = GTK_APPLICATION_WINDOW(gtk_builder_get_object(builder, "mainwindow"));
    GtkDrawingArea* drawing_area = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "drawingarea"));

    gtk_application_add_window(app, GTK_WINDOW(window));
    gtk_widget_show(GTK_WIDGET(window));

    GdkWindow* drawWindow = gtk_widget_get_window(GTK_WIDGET(drawing_area));

    if (drawWindow == NULL) {
        logfatal("Unable to get native window handle for drawing area!\n");
    }

    n64_system_t* system = init_n64system("sm64.z64", true, false, VULKAN, drawWindow);

    load_parallel_rdp(system);
    pif_rom_execute(system);
    // TODO this needs to be in a separate thread
    n64_system_loop(system);
}

int main(int argc, char** argv) {
    GtkApplication* app;
    int status;
    app = gtk_application_new("com.dillonbeliveau.n64", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);


    return status;
}