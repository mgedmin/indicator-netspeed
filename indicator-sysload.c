/*

This is an Ubuntu appindicator that displays the system load
(CPU, memory usage, network traffic).

It looks like this: http://imgur.com/W4PCc .

Compile with:
gcc -Wall `pkg-config --cflags --libs gtk+-2.0 appindicator-0.1 libgtop-2.0` -o indicator-sysload ./indicator-sysload.c

WARNING: 

The periodic update makes Compiz leak memory.
12-24 hours of running will cause performance
issues, and you have to restart Gnome.

License: this software is in the public domain.

*/

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>

/* update period in seconds */
int period = 1;

AppIndicator *indicator;
GtkWidget *indicator_menu;

GtkWidget *cpu_item;
GtkWidget *memory_item;
GtkWidget *net_down_item;
GtkWidget *net_up_item;
GtkWidget *sysmon_item;
GtkWidget *quit_item;

int get_cpu()
{
    static int cpu_total_old = 0;
    static int cpu_idle_old = 0;

    glibtop_cpu cpu;
	glibtop_get_cpu (&cpu);
	
    int diff_total = cpu.total - cpu_total_old;
    int diff_idle = cpu.idle - cpu_idle_old;

    cpu_total_old = cpu.total;
    cpu_idle_old = cpu.idle;

    return 100 * (diff_total-diff_idle) / diff_total;
}

int get_memory(void)
{
    glibtop_mem mem;
    glibtop_get_mem (&mem);
    /* used memory in percents */
    return 100 - 100 * (mem.free + mem.buffer + mem.cached) / mem.total; 
}

gchar* format_net_label(gchar *prefix, int data)
{
    gchar *string;
    if(data < 1000)
    {
        string = g_strdup_printf("%s%d B/s", prefix, data);
    }
    else if(data < 1000000)
    {
        string = g_strdup_printf("%s%.1f KiB/s", prefix, data/1000.0);
    }
    else
    {
        string = g_strdup_printf("%s%.2f MiB/s", prefix, data/1000000.0);
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
    int cpu = get_cpu();
    int memory = get_memory();
    
    int net_traffic[2] = {0, 0};
    get_net(net_traffic);
    int net_down = net_traffic[0];
    int net_up = net_traffic[1];  
    
    static const gchar *separator = "｜";
    static const gchar *nothing = "　";
    
    /* the half and full block doesn't fit (at least in my system, perhaps a font substitution issue)*/
    /*static const gchar *levels[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};*/
    static const gchar *levels[] = {"▁", "▂", "▃", "▅", "▆", "▇"};
    
    int level_num = sizeof(levels) / sizeof("▂");

    int cpu_max = 100;
    int mem_max = 100;
    
    static int net_down_max = 0;
    static int net_up_max = 0;
    int weight = 4;
    net_down_max = MAX(net_down, (weight*net_down_max + net_down) / (weight+1));
    net_up_max = MAX(net_up, (weight*net_up_max + net_up) / (weight+1));
    
    gchar *cpu_level      = (gchar *) ((cpu == 0)      ? nothing : levels[level_num * cpu / (cpu_max+1)]);
    gchar *memory_level   = (gchar *) ((memory == 0)   ? nothing : levels[level_num * memory / (mem_max+1)]);
    gchar *net_down_level = (gchar *) ((net_down == 0) ? nothing : levels[level_num * net_down / (net_down_max+1)]);
    gchar *net_up_level   = (gchar *) ((net_up == 0)   ? nothing : levels[level_num * net_up / (net_up_max+1)]);
    
    gchar *indicator_label = g_strconcat( separator
                                        , cpu_level
                                        , memory_level
                                        , net_down_level
                                        , net_up_level
                                        , separator
                                        , NULL);

    static const gchar *label_guide = "｜▁▂▃▅｜"; 
    app_indicator_set_label(indicator, indicator_label, label_guide);
    g_free(indicator_label);

    gchar *cpu_label = g_strdup_printf("CPU: %d%%", cpu);
    gtk_menu_item_set_label(GTK_MENU_ITEM(cpu_item), cpu_label);
    g_free(cpu_label);
    
    gchar *memory_label = g_strdup_printf("Memory: %d%%", memory);
    gtk_menu_item_set_label(GTK_MENU_ITEM(memory_item), memory_label);
    g_free(memory_label);
    
    gchar *net_down_label = format_net_label("Net ↓: ", net_down);
    gtk_menu_item_set_label(GTK_MENU_ITEM(net_down_item), net_down_label);
    g_free(net_down_label);

    gchar *net_up_label = format_net_label("Net ↑: ", net_up);
    gtk_menu_item_set_label(GTK_MENU_ITEM(net_up_item), net_up_label);
    g_free(net_up_label);
    
    return TRUE;
}

static void start_sysmon(GtkWidget *widget, gpointer data)
{
    GError *err = NULL;
    gchar *argv[] = {"gnome-system-monitor"};
    if( ! g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err))
    {
        fprintf(stderr, "Error: %s\n", err->message);
    }
}

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    indicator_menu = gtk_menu_new();
       
    cpu_item = gtk_image_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), cpu_item);

    memory_item = gtk_image_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), memory_item);

    net_down_item = gtk_image_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), net_down_item);

    net_up_item = gtk_image_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), net_up_item);
     
    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sep);

    sysmon_item = gtk_menu_item_new_with_label("Gnome System Monitor");
    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sysmon_item);
    g_signal_connect(sysmon_item, "activate", G_CALLBACK (start_sysmon), NULL);    

/* I don't think it's necessary, uncomment if you want it*/

/*    quit_item = gtk_menu_item_new_with_label("Quit");*/
/*    gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), quit_item);*/
/*    g_signal_connect(quit_item, "activate", G_CALLBACK (gtk_main_quit), NULL);*/

    gtk_widget_show_all(indicator_menu);

    indicator = app_indicator_new ("sysload", "", APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_label(indicator, "sysload", "sysload");
    app_indicator_set_menu(indicator, GTK_MENU (indicator_menu));
 
    update();
    
    /* update period in milliseconds */
    g_timeout_add(1000*period, (GtkFunction) update, NULL);

    gtk_main ();

    return 0;
}