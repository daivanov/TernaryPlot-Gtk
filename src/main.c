/*
 * Copyright 2012 Daniil Ivanov <daniil.ivanov@gmail.com>
 *
 * This file is part of TernaryPlot-Gtk2.
 *
 * TernaryPlot-Gtk2 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * TernaryPlot-Gtk2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with TernaryPlot-Gtk2. If not, see http://www.gnu.org/licenses/.
 */

#include <gtk/gtk.h>
#include <math.h>

#include "ternaryplot.h"

void destroyed_cb (GtkWidget *widget, gpointer data)
{
    /* quit GTK application */
    gtk_main_quit ();
}

void point_changed_cb (TernaryPlot *plot, gdouble x, gdouble y, gdouble z,
    gpointer data)
{
    gdouble new_x = x, new_y = y, new_z = z;
    if (!validator (&new_x, &new_y, &new_z))
    {
        ternary_plot_set_point (plot, new_x, new_y, new_z);
    }
}

gboolean validator (gdouble *x, gdouble *y, gdouble *z)
{
    double r;

    if (*x > 0.8)
    {
        *y += (*x - 0.8)/2;
        *z += (*x - 0.8)/2;
        *x = 0.8;
        g_print ("Point: %.1f,%.1f,%.1f\n",*x,*y,*z);
        r = fmod (*y, 0.1);
        r = r < 0.05 ? -r : r; 
        *y += r;
        *z += -r;
        g_print ("Updated point: %.1f,%.1f,%.1f\n",*x,*y,*z);
        return FALSE;
    }
    if (*y > 0.8)
    {
        *x += (*y - 0.8)/2;
        *z += (*y - 0.8)/2;
        *y = 0.8;
        g_print ("Point: %.1f,%.1f,%.1f\n",*x,*y,*z);
        r = fmod (*x, 0.1);
        r = r < 0.05 ? -r : r; 
        *x += r;
        *z += -r;
        g_print ("Updated point: %.1f,%.1f,%.1f\n",*x,*y,*z);
        return FALSE;
    }
    if (*z > 0.8)
    {
        *x += (*z - 0.8)/2;
        *y += (*z - 0.8)/2;
        *z = 0.8;
        g_print ("Point: %.1f,%.1f,%.1f\n",*x,*y,*z);
        r = fmod (*x, 0.1);
        r = r < 0.05 ? -r : r; 
        *x += r;
        *y += -r;
        g_print ("Updated point: %.1f,%.1f,%.1f\n",*x,*y,*z);
        return FALSE;
    }
    return TRUE;
}

int main (int argc,char *argv[])
{
    GtkWidget *window, *plot;

    /* initilaize GTK */
    gtk_init (&argc, &argv);

    /* create ternary plot widget*/
    plot = ternary_plot_new ();
    ternary_plot_set_xlabel ((TernaryPlot*) plot, "Tax");
    ternary_plot_set_ylabel ((TernaryPlot*) plot, "Luxury");
    ternary_plot_set_zlabel ((TernaryPlot*) plot, "Science");
    g_signal_connect (TERNARY_PLOT (plot), "point-changed",
                      G_CALLBACK (point_changed_cb), NULL);

    /* create window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "GTK2 and Cairo");
    gtk_window_set_default_size (GTK_WINDOW (window), 300, 300);
    gtk_container_set_border_width (GTK_CONTAINER (window), 2);
    gtk_container_add (GTK_CONTAINER (window), plot);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroyed_cb), NULL);

    /* show widgets */
    gtk_widget_show_all (window);

    gtk_main ();

    return 0;
}
