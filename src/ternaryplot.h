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

#ifndef __TERNARY_PLOT_H__
#define __TERNARY_PLOT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TERNARY_TYPE_PLOT          (ternary_plot_get_type ())
#define TERNARY_PLOT(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    TERNARY_TYPE_PLOT, TernaryPlot))
#define TERNARY_PLOT_CLASS(obj)    (G_TYPE_CHECK_CLASS_CAST ((obj), \
                                    TERNARY_PLOT,  TernaryPlotClass))
#define TERNARY_IS_PLOT(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    TERNARY_TYPE_PLOT))
#define TERNARY_IS_PLOT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                                    TERNARY_TYPE_PLOT))
#define TERNARY_PLOT_GET_CLASS     (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                    TERNARY_TYPE_PLOT, TernaryPlotClass))

typedef struct _TernaryPlot         TernaryPlot;
typedef struct _TernaryPlotClass    TernaryPlotClass;

struct _TernaryPlot
{
    GtkDrawingArea parent;

    gdouble  GSEAL(tol);    /* rounding tolerance in percents */

    /* private */
};

struct _TernaryPlotClass
{
    GtkDrawingAreaClass parent_class;

    /* Signals */
    void (*point_changed) (TernaryPlot *plot,
                           gdouble x,
                           gdouble y,
                           gdouble z);
};

GType ternary_plot_get_type (void);
GtkWidget * ternary_plot_new (void);
void ternary_plot_set_xlabel (TernaryPlot *plot, const gchar *xlabel);
void ternary_plot_set_ylabel (TernaryPlot *plot, const gchar *ylabel);
void ternary_plot_set_zlabel (TernaryPlot *plot, const gchar *zlabel);
const gchar* ternary_plot_get_xlabel (TernaryPlot *plot);
const gchar* ternary_plot_get_ylabel (TernaryPlot *plot);
const gchar* ternary_plot_get_zlabel (TernaryPlot *plot);
void ternary_plot_set_tolerance (TernaryPlot *plot, gdouble tol);
gdouble ternary_plot_get_tolerance (TernaryPlot *plot);
void ternary_plot_set_point (TernaryPlot *plot, gdouble x, gdouble y, gdouble z);
void ternary_plot_get_point (TernaryPlot *plot, gdouble *x, gdouble *y, gdouble *z);

G_END_DECLS

#endif
