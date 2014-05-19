/*

This is an Ubuntu appindicator that displays the current network traffic.

Based on indicator-netspeed.c from https://gist.github.com/982939

License: this software is in the public domain.

*/

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <glibtop.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>
#include <pango/pango.h>
#include <gio/gio.h>
#include <stdbool.h>

/* update period in seconds */
int period = 1;
gboolean first_run = TRUE;

GSettings *settings;

AppIndicator *indicator;
GtkWidget *indicator_menu;
GtkWidget *interfaces_menu;

gchar* selected_if_name = NULL;

GtkWidget *if_chosen;
GtkWidget *net_down_item;
GtkWidget *net_up_item;
GtkWidget *quit_item;

gchar* format_net_label(int data, bool padding)
{
    gchar *string;
    /*if(data < 1000)
    {
        string = g_strdup_printf("%d B/s", data);
    }
    else*/ if(data < 1000000)  //should be < 1 MiB and not 1 MB, but this keeps width smaller
    {
        string = g_strdup_printf("%.1f KiB/s", data/1024.0);
    }
    else
    {
        string = g_strdup_printf("%.2f MiB/s", data/1048576.0);
    }
    //will someone have 1 gb/s ? maybe...

    if(padding)
    {
        //render string and get its pixel width
        int width = 0;
        static int maxWidth = 12;   //max width for label in pixels

        //TODO: should be determined from current panel font type and size
        int spaceWidth = 4;  //width of one space char in pixels,

        PangoContext* context = gtk_widget_get_pango_context(indicator_menu);
        PangoLayout* layout = pango_layout_new(context);
        pango_layout_set_text(layout, string, strlen(string));
        pango_layout_get_pixel_size(layout, &width, NULL);
        // frees the layout object, do not use after this point
        g_object_unref(layout);

        //push max size up as needed
        if (width > maxWidth) maxWidth = width + spaceWidth;

        gchar *old_string = string;

        //fill up with spaces
        string = g_strdup_printf("%*s%s", (int)((maxWidth-width)/spaceWidth), " ", string);
        g_free(old_string);
    }

    return string;
}

void get_net(int traffic[2])
{
    static int bytes_in_old = 0;
    static int bytes_out_old = 0;
    glibtop_netload netload;
    glibtop_netlist netlist;
    int bytes_in = 0;
    int bytes_out = 0;
    int i = 0;

    gchar **interfaces = glibtop_get_netlist(&netlist);

    for(i = 0; i < netlist.number; i++)
    {
        if (strcmp("lo", interfaces[i]) == 0)
        {
            continue;
        }

        if(strcmp("all", selected_if_name) == 0 || strcmp(selected_if_name, interfaces[i]) == 0) {
            glibtop_get_netload(&netload, interfaces[i]);
            bytes_in += netload.bytes_in;
            bytes_out += netload.bytes_out;
        }
    }
    g_strfreev(interfaces);

    if(first_run)
    {
        bytes_in_old = bytes_in;
        bytes_out_old = bytes_out;
        first_run = FALSE;
    }

    traffic[0] = (bytes_in - bytes_in_old) / period;
    traffic[1] = (bytes_out - bytes_out_old) / period;

    bytes_in_old = bytes_in;
    bytes_out_old = bytes_out;
}

void if_signal_select(GtkMenuItem *menu_item, gpointer user_data) {
    //set currently selected interface from user selection
    /*if (selected_if_name != NULL){
        g_free(selected_if_name);
        selected_if_name == NULL;
    }*/
    selected_if_name = g_strdup(gtk_menu_item_get_label(menu_item));
    gtk_menu_item_set_label(if_chosen, selected_if_name);
    g_settings_set_value (settings, "if-name", g_variant_new_string(selected_if_name));

    first_run = TRUE;
    update();
}

void add_netifs() {
    //populate list of interfaces
    //TODO: make this refresh when interfaces change
    glibtop_netlist netlist;
    gchar **interfaces = glibtop_get_netlist(&netlist);
    GtkWidget *if_item;

    for(int i = 0; i < netlist.number; i++) {
        if (strcmp("lo", interfaces[i]) == 0)
            continue;

        if_item = gtk_menu_item_new_with_label(interfaces[i]);
        gtk_menu_shell_append(GTK_MENU_SHELL(interfaces_menu), if_item);
        g_signal_connect (G_OBJECT(if_item), "activate", G_CALLBACK(if_signal_select), NULL);
    }

    if_item = gtk_menu_item_new_with_label("all");  //can name and id be different?
    gtk_menu_shell_append(GTK_MENU_SHELL(interfaces_menu), if_item);
    g_signal_connect (G_OBJECT(if_item), "activate", G_CALLBACK(if_signal_select), NULL);

    //set previously selected value
    gtk_menu_item_set_label(GTK_MENU_ITEM(if_chosen), selected_if_name);
    g_strfreev(interfaces);
}

gboolean update() {
    //get sum of up and down net traffic and separate values
    //and refresh labels of current interface
    int net_traffic[2] = {0, 0};
    get_net(net_traffic);
    int net_down = net_traffic[0];
    int net_up = net_traffic[1];
    int net_total = net_down + net_up;

    gchar *indicator_label = format_net_label(net_total, true);
    gchar *label_guide = "10000.00 MiB/s";   //maximum length label text, doesn't really work atm
    app_indicator_set_label(indicator, indicator_label, label_guide);
    g_free(indicator_label);

    gchar *net_down_label = format_net_label(net_down, false);
    gtk_menu_item_set_label(GTK_MENU_ITEM(net_down_item), net_down_label);
    g_free(net_down_label);

    gchar *net_up_label = format_net_label(net_up, false);
    gtk_menu_item_set_label(GTK_MENU_ITEM(net_up_item), net_up_label);
    g_free(net_up_label);

    if (net_down && net_up)
    {
        app_indicator_set_icon(indicator, "network-transmit-receive");
    }
    else if (net_down)
    {
        app_indicator_set_icon(indicator, "network-receive");
    }
    else if (net_up)
    {
        app_indicator_set_icon(indicator, "network-transmit");
    }
    else {
        app_indicator_set_icon(indicator, "network-idle");
    }

    return TRUE;
}

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    settings = g_settings_new("apps.indicators.netspeed");
    selected_if_name = g_variant_get_string (g_settings_get_value(settings, "if-name"), NULL);

    indicator_menu = gtk_menu_new();

    //add interfaces menu
    interfaces_menu = gtk_menu_new();

    //add interface names
    if_chosen = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), if_chosen);
    gtk_menu_item_set_submenu (if_chosen, interfaces_menu);
    add_netifs();

    //separator
    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sep);

    //add menu entries for up and down speed
    net_down_item = gtk_image_menu_item_new_with_label("");
    GtkWidget *net_down_icon = gtk_image_new_from_icon_name("network-receive", GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(net_down_item), net_down_icon);
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(net_down_item), TRUE);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), net_down_item);

    net_up_item = gtk_image_menu_item_new_with_label("");
    GtkWidget *net_up_icon = gtk_image_new_from_icon_name("network-transmit", GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(net_up_item), net_up_icon);
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(net_up_item), TRUE);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), net_up_item);

    //separator
    sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sep);

    //quit item
    quit_item = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), quit_item);
    g_signal_connect(quit_item, "activate", G_CALLBACK (gtk_main_quit), NULL);

    gtk_widget_show_all(indicator_menu);

    //create app indicator
    indicator = app_indicator_new ("netspeed", "network-idle", APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_label(indicator, "netspeed", "netspeed");
    app_indicator_set_menu(indicator, GTK_MENU (indicator_menu));

    //set indicator position. default: all the way left
    //TODO: make this optional so placement can be automatic
    guint32 ordering_index = g_variant_get_int32(g_settings_get_value(settings, "ordering-index"));
    app_indicator_set_ordering_index(indicator, ordering_index);

    update();

    /* update period in milliseconds */
    g_timeout_add(1000*period, update, NULL);

    gtk_main ();

    return 0;
}
