#include "luminescence.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

Lumi lumi;

typedef struct {
    void *handle;
    char *filename;
    const char *name;
    const char *description;
    void (*init)(Lumi*);
    Command *commands;
} Plugin;

Plugin *plugins = 0;
int plugin_count = 0;

Command **commands = 0;
int command_count = 0;

bool on_key_press(GtkWidget *widget, GdkEventKey *event){
    return TRUE;
}

void load_plugin(const char *filename){
    char path[256];
    strcpy(path, "plugins/");
    strncat(path, filename, 247);
    void *handle = dlopen(path, RTLD_LAZY);
    if(!handle) return;

    plugins = (Plugin*) realloc(plugins, sizeof(Plugin) * (plugin_count+1));
    Plugin *plugin = plugins + plugin_count++;
    plugin->handle = handle;
    plugin->filename = strdup(filename);
    const char **name = dlsym(handle, "name");
    plugin->name = name ? *name : 0;
    const char **description = dlsym(handle, "description");
    plugin->description = description ? *description : 0;
    plugin->init = dlsym(handle, "init");

    // Commands
    plugin->commands = dlsym(handle, "commands");
    if(plugin->commands){
        Command *c = plugin->commands;
        for(; c->name; c++){
            commands = (Command**) realloc(commands, sizeof(Command*) * (command_count+1));
            commands[command_count++] = c;
        }
    }
}

int is_visible(const struct dirent *entry){
    return entry->d_name[0] != '.';
}

void load_plugins(){
    struct dirent **entries;
    int entry_count = scandir("plugins", &entries, is_visible, alphasort);
    if(entry_count < 0) return;
    int i = 0;
    for(; i<entry_count; i++){
        load_plugin(entries[i]->d_name);
        free(entries[i]);
    }
    free(entries);
}

void run_command(const char *cmd, const char *arg){
    int i = 0;
    for(; i<command_count; i++){
        if(strcmp(commands[i]->name, cmd) == 0)
            commands[i]->exec(arg);
    }
}

void load_config(){
    FILE *f = fopen("config", "r");
    if(!f) return;
    char *line = 0;
    size_t n = 0, count;
    char *name;
    while(getline(&line, &n, f) != -1){
        if(line[0] == '\0') continue; // null
        if(line[0] == '\n' || line[0] == '#') continue; // comment or empty
        for(count = 0; line[count] != '\0' && line[count] != '\n'; count++);
        line[count] = '\0'; // drop the newline
        for(count = 0; line[count] != '\0' && line[count] != ' '; count++);
        name = strndup(line, count);
        while(line[count] == ' ') count++;
        run_command(name, line[count] == '\0' ? 0 : line + count);
        free(name);
    }
    free(line);
    fclose(f);
}

void parse_arguments(int argc, char **argv){
    int i, eq;
    char *arg = 0;
    for(i=1; i<argc; i++){
        if(argv[i][0] == '\0') continue;
        if(argv[i][0] == '-' && argv[i][1] == '-'){
            if(arg) run_command(arg, 0);
            for(eq=0; argv[i][2+eq] != '=' && argv[i][2+eq] != '\0'; eq++);
            if(argv[i][2+eq] == '='){
                arg = strndup(argv[i]+2, eq);
                run_command(arg, argv[i]+3+eq);
                free(arg);
                arg = 0;
            }
            else
                arg = argv[i]+2;
        }
        else if(arg){
            run_command(arg, argv[i]);
            arg = 0;
        }
    }
    if(arg) run_command(arg, 0);
}

void print_help(){
    puts("Usage: luminescence --COMMAND[=ARG] ...\n");
    if(!plugin_count){
        puts("No plugins.");
        return;
    }
    puts("Available plugins:");
    int i, j, pad;
    Command *c;
    int longest = 0;
    for(i=0; i<command_count; i++){
        if(strlen(commands[i]->name) > longest)
            longest = strlen(commands[i]->name);
    }
    for(i=0; i<plugin_count; i++){
        fputs("* ", stdout);
        if(plugins[i].name)
            printf("%s (%s)\n", plugins[i].name, plugins[i].filename);
        else
            puts(plugins[i].filename);
        if(plugins[i].description)
            printf("  %s\n", plugins[i].description);
        c = plugins[i].commands;
        if(!c) continue;
        puts("  Commands:");
        for(; c->name; c++){
            fputs("    ", stdout);
            fputs(c->name, stdout);
            if(c->help){
                pad = longest - strlen(c->name) + 3;
                for(j=0; j<pad; j++)
                    putchar(' ');
                fputs(c->help, stdout);
            }
            putchar('\n');
        }
    }
}

int main(int argc, char **argv){
    char *lumi_dir = strdup(getenv("HOME"));
    lumi_dir = realloc(lumi_dir, strlen(lumi_dir) + 15);
    strcat(lumi_dir, "/.luminescence");
    chdir(lumi_dir);

    load_plugins();

    // Help
    if(argc == 2){
        if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
            print_help();
            return 0;
        }
    }

    gtk_init(&argc, &argv);

    // Window
    lumi.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_add_events(lumi.window, GDK_KEY_PRESS_MASK);
    gtk_window_set_has_resize_grip(GTK_WINDOW(lumi.window), FALSE);
    g_signal_connect(lumi.window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(lumi.window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    // Layout
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(lumi.window), layout);
    gtk_widget_show(layout);

    // Web view
    lumi.web_view = webkit_web_view_new();
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), lumi.web_view);
    gtk_box_pack_start(GTK_BOX(layout), scrolled_window, TRUE, TRUE, 0);
    gtk_widget_show_all(scrolled_window);

    // Status bar
    lumi.status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(layout), lumi.status_bar, FALSE, FALSE, 0);
    gtk_widget_show(lumi.status_bar);

    // Plugins
    int i;
    for(i=0; i<plugin_count; i++)
        if(plugins[i].init) (*plugins[i].init)(&lumi);
    load_config();
    parse_arguments(argc, argv);

    // Exec
    gtk_widget_show(lumi.window);
    gtk_main();

    return 0;
}
