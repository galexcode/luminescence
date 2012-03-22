#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

struct {
    GtkWidget *web_view;
    GtkWidget *address_label;
    GtkWidget *address_entry;
    GtkWidget *scripts_label;
    GtkWidget *insert_label;
} ui;

void scripts_set_enabled(gboolean enable){
    g_object_set(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(ui.web_view)), "enable-scripts", enable, NULL);
    gtk_widget_set_visible(ui.scripts_label, enable);
}

void show_address_label(){
    gtk_widget_hide(ui.address_entry);
    gtk_widget_show(ui.address_label);
}

void go(){
    const gchar *raw_uri = gtk_entry_get_text(GTK_ENTRY(ui.address_entry));
    gchar *uri = (gchar*) malloc(strlen(raw_uri)+8);
    uri[0] = '\0';
    if(!strstr(raw_uri, "://"))
        strcat(uri, "http://");
    strcat(uri, raw_uri);
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(ui.web_view), uri);
    free(uri);
    show_address_label();
}

void update_address_label(){
    if(webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(ui.web_view)) == WEBKIT_LOAD_COMMITTED)
        gtk_label_set_text(GTK_LABEL(ui.address_label), webkit_web_view_get_uri(WEBKIT_WEB_VIEW(ui.web_view)));
}

void show_address_entry(){
    const gchar *uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(ui.web_view));
    gtk_entry_set_text(GTK_ENTRY(ui.address_entry), uri ? uri : "");
    gtk_widget_hide(ui.address_label);
    gtk_widget_show(ui.address_entry);
    gtk_widget_grab_focus(ui.address_entry);
}

bool on_key_press(GtkWidget *widget, GdkEventKey *event){
    if(gtk_widget_get_visible(ui.address_entry) || gtk_widget_get_visible(ui.insert_label)){
        if(event->keyval == GDK_KEY_Escape){
            show_address_label();
            gtk_widget_hide(ui.insert_label);
            return TRUE;
        }
        // Let Gtk/WebKit handle the key.
        else return FALSE;
    }
    switch(event->keyval){
        case GDK_KEY_i: gtk_widget_show(ui.insert_label);  break;
        case GDK_KEY_u: show_address_entry();  break;
        case GDK_KEY_s: scripts_set_enabled(FALSE); break;
        case GDK_KEY_S: scripts_set_enabled(TRUE);  break;
        case GDK_KEY_r: webkit_web_view_reload(WEBKIT_WEB_VIEW(ui.web_view)); break;
        case GDK_KEY_R: webkit_web_view_reload_bypass_cache(WEBKIT_WEB_VIEW(ui.web_view)); break;
    }
    // Unknown key, forget about it.
    return TRUE;
}

int main(int argc, char **argv){
    gtk_init(&argc, &argv);

    // Window

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_window_set_has_resize_grip(GTK_WINDOW(window), FALSE);
    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(layout);

    // Web view

    ui.web_view = webkit_web_view_new();
    g_signal_connect(ui.web_view, "notify::load-status", G_CALLBACK(update_address_label), NULL);
    g_object_set(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(ui.web_view)),
        "enable-plugins", FALSE, "enable-scripts", FALSE, NULL);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), ui.web_view);
    gtk_box_pack_start(GTK_BOX(layout), scrolled_window, TRUE, TRUE, 0);
    gtk_widget_show_all(scrolled_window);

    // Status

    GtkWidget *status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(layout), status_bar, FALSE, FALSE, 0);
    gtk_widget_show(status_bar);

    ui.address_label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(ui.address_label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(ui.address_label), 2, 2);
    gtk_box_pack_start(GTK_BOX(status_bar), ui.address_label, TRUE, TRUE, 0);
    gtk_widget_show(ui.address_label);

    ui.address_entry = gtk_entry_new();
    gtk_entry_set_has_frame(GTK_ENTRY(ui.address_entry), FALSE);
    g_signal_connect(ui.address_entry, "activate", G_CALLBACK(go), NULL);
    gtk_box_pack_start(GTK_BOX(status_bar), ui.address_entry, TRUE, TRUE, 0);

    ui.scripts_label = gtk_label_new("JS");
    gtk_box_pack_start(GTK_BOX(status_bar), ui.scripts_label, FALSE, FALSE, 3);

    ui.insert_label = gtk_label_new("INSERT");
    gtk_box_pack_start(GTK_BOX(status_bar), ui.insert_label, FALSE, FALSE, 3);

    // Show

    gtk_container_add(GTK_CONTAINER(window), layout);
    gtk_widget_show(window);
    gtk_main();

    return 0;
}
