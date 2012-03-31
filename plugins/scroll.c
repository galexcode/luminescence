#include <luminescence.h>
#include <string.h>

const char *name = "Scroll";

Lumi *lumi;

#define ARG_CASE(str) if(strcasecmp(arg, (str)) == 0)

void scroll(const char *arg){
    if(!arg) return;
    GtkAdjustment *adjustment = arg[0] == 'u' || arg[0] == 'd'
        ? gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(lumi->web_view))
        : gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(lumi->web_view));
    gdouble factor = 0;
    ARG_CASE("up")    factor = -1;
    ARG_CASE("right") factor = 1;
    ARG_CASE("down")  factor = 1;
    ARG_CASE("left")  factor = -1;
    if(factor == 0) return;
    gtk_adjustment_set_value(adjustment,
        gtk_adjustment_get_value(adjustment) + factor*gtk_adjustment_get_step_increment(adjustment));
}

Command commands[] = {
    { "scroll", scroll },
    { 0 } };
