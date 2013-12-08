/*

This is an Ubuntu appindicator that displays the current network traffic.

Build dependencies:
apt-get install libgtop2-dev libgtk-3-dev libappindicator3-dev

Compile with:
gcc -Wall `pkg-config --cflags --libs gtk+-3.0 appindicator-0.1 libgtop-2.0` -o indicator-netspeed ./indicator-netspeed.c

Based on indicator-netspeed.c from https://gist.github.com/982939

License: this software is in the public domain.

*/

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <glibtop.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>

/* update period in seconds */
int period = 1;

AppIndicator *indicator;
GtkWidget *indicator_menu;

GtkWidget *net_down_item;
GtkWidget *net_up_item;
GtkWidget *quit_item;

gchar* format_net_label(int data)
{
    gchar *string;
    if(data < 1000)
    {
        string = g_strdup_printf("%d B/s", data);
    }
    else if(data < 1000000)
    {
        string = g_strdup_printf("%.1f KiB/s", data/1000.0);
    }
    else
    {
        string = g_strdup_printf("%.2f MiB/s", data/1000000.0);
    }
    return string;
}

void get_net(int traffic[2])
{
    static int bytes_in_old = 0;
    static int bytes_out_old = 0;
    static gboolean first_run = TRUE;
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
        glibtop_get_netload(&netload, interfaces[i]);
        bytes_in += netload.bytes_in;
        bytes_out += netload.bytes_out;
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

gboolean update()
{
    int net_traffic[2] = {0, 0};
    get_net(net_traffic);
    int net_down = net_traffic[0];
    int net_up = net_traffic[1];  
    int net_total = net_down + net_up;
    
    gchar *indicator_label = format_net_label(net_total);
    gchar *label_guide = "Net: 10000.00 MiB/s"; /* I wish... */
    app_indicator_set_label(indicator, indicator_label, label_guide);
    g_free(indicator_label);

    gchar *net_down_label = format_net_label(net_down);
    gtk_menu_item_set_label(GTK_MENU_ITEM(net_down_item), net_down_label);
    g_free(net_down_label);

    gchar *net_up_label = format_net_label(net_up);
    gtk_menu_item_set_label(GTK_MENU_ITEM(net_up_item), net_up_label);
    g_free(net_up_label);
    
    return TRUE;
}

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    indicator_menu = gtk_menu_new();
       
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

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sep);

    quit_item = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), quit_item);
    g_signal_connect(quit_item, "activate", G_CALLBACK (gtk_main_quit), NULL);

    gtk_widget_show_all(indicator_menu);

    indicator = app_indicator_new ("netspeed", "network-transmit-receive", APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_label(indicator, "netspeed", "netspeed");
    app_indicator_set_menu(indicator, GTK_MENU (indicator_menu));
 
    update();
    
    /* update period in milliseconds */
    g_timeout_add(1000*period, update, NULL);

    gtk_main ();

    return 0;
}
