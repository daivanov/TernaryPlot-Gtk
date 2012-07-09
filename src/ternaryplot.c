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
#include <string.h>

#include "ternaryplot.h"
#include "ternaryplot-marshallers.h"

#define TERNARY_PLOT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                       TERNARY_TYPE_PLOT, TernaryPlotPrivate))

typedef struct _TernaryPlotPrivate TernaryPlotPrivate;

struct _TernaryPlotPrivate
{
    gdouble x1, y1, x2, y2, x3, y3; /* vertices */
    gdouble xc, yc; /* center */
    gdouble radius; /* radius */
    gdouble x, y, z; /* x-,y-,z-values */
    gchar *xlabel, *ylabel, *zlabel; /* x-,y-,z-labels */
    gdouble tol; /* rounding tolerance in percents */
    gchar is_dragged; /* is pointer being dragged */
};

G_DEFINE_TYPE (TernaryPlot, ternary_plot, GTK_TYPE_DRAWING_AREA);

enum {
    POINT_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* event handlers */
static gboolean ternary_plot_expose (GtkWidget* plot, GdkEventExpose *event);
static gboolean ternary_plot_button_press (GtkWidget *plot, GdkEventButton *event);
static gboolean ternary_plot_button_release (GtkWidget *plot, GdkEventButton *event);
static gboolean ternary_plot_motion_notify (GtkWidget *plot, GdkEventMotion *event);

static void ternary_plot_finalize (GObject *object);

/* utility functions */
static gdouble ternary_plot_dot_to_line_distance (gdouble x, gdouble y,
    gdouble x1, gdouble y1, gdouble x2, gdouble y2);

static void ternary_plot_class_init (TernaryPlotClass *class)
{
    GObjectClass *obj_class;
    GtkWidgetClass *widget_class;

    obj_class = G_OBJECT_CLASS (class);
    widget_class = GTK_WIDGET_CLASS (class);

    obj_class->finalize = ternary_plot_finalize;

    signals[POINT_CHANGED] =
        g_signal_new ("point-changed",
                      G_OBJECT_CLASS_TYPE (obj_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (TernaryPlotClass, point_changed),
                      NULL, NULL,
                      ternaryplot_marshal_VOID__DOUBLE_DOUBLE_DOUBLE,
                      G_TYPE_NONE, 3,
                      G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

    /* event handlers */
    widget_class->expose_event = ternary_plot_expose;
    widget_class->button_press_event = ternary_plot_button_press;
    widget_class->button_release_event = ternary_plot_button_release;
    widget_class->motion_notify_event = ternary_plot_motion_notify;

    g_type_class_add_private (obj_class, sizeof (TernaryPlotPrivate));
}

static void ternary_plot_init (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    priv->x = 1./3;
    priv->y = 1./3;
    priv->z = 1./3;

    priv->tol = 0.1; /* 10% */

    priv->is_dragged = FALSE;

    gtk_widget_add_events (GTK_WIDGET (plot),
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK);
}

static void ternary_plot_finalize (GObject *object)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (object);

    if (priv->xlabel)
        g_free (priv->xlabel);
    if (priv->ylabel)
        g_free (priv->ylabel);
    if (priv->zlabel)
        g_free (priv->zlabel);

    G_OBJECT_CLASS (ternary_plot_parent_class)->finalize (object);
}

static void draw (GtkWidget *plot, cairo_t *cr)
{
    gdouble frac;
    gint i;
    gchar label[100];
    cairo_text_extents_t extents;
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* large triangle */
    cairo_move_to (cr, priv->x1, priv->y1);
    cairo_line_to (cr, priv->x2, priv->y2);
    cairo_line_to (cr, priv->x3, priv->y3);
    cairo_close_path(cr);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_fill_preserve (cr);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_stroke (cr);

    cairo_save (cr); /* stack-pen-size */

    /* 10% lines */
    cairo_set_line_width (cr, 0.25 * cairo_get_line_width (cr));
    cairo_set_source_rgb (cr, .8, .8, .8);

    /* small triangle */
    cairo_move_to (cr, (priv->x1+priv->x2)/2, (priv->y1+priv->y2)/2);
    cairo_line_to (cr, (priv->x2+priv->x3)/2, (priv->y2+priv->y3)/2);
    cairo_line_to (cr, (priv->x3+priv->x1)/2, (priv->y3+priv->y1)/2);
    cairo_close_path(cr);
    cairo_stroke (cr);

    for (i = 1; i < 5; i++)
    {
        frac = i/10.0;
        cairo_move_to (cr, frac*priv->x1 + (1-frac)*priv->x2, frac*priv->y1 + (1-frac)*priv->y2);
        cairo_line_to (cr, (1-frac)*priv->x2 + frac*priv->x3, (1-frac)*priv->y2 + frac*priv->y3);
        cairo_line_to (cr, frac*priv->x3 + (1-frac)*priv->x1, frac*priv->y3 + (1-frac)*priv->y1);
        cairo_line_to (cr, (1-frac)*priv->x1 + frac*priv->x2, (1-frac)*priv->y1 + frac*priv->y2);
        cairo_line_to (cr, frac*priv->x2 + (1-frac)*priv->x3, frac*priv->y2 + (1-frac)*priv->y3);
        cairo_line_to (cr, (1-frac)*priv->x3 + frac*priv->x1, (1-frac)*priv->y3 + frac*priv->y1);
        cairo_close_path(cr);
        cairo_stroke (cr);
    }
    cairo_restore (cr); /* stack-pen-size */

    /* font settings */
    cairo_set_font_size (cr, 12);
    cairo_select_font_face (cr, "Nimbus Sans L", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    g_snprintf (label, sizeof(label), "%s: %.1f",
        priv->xlabel != NULL ? priv->xlabel : "", 100 * priv->x);
    cairo_text_extents (cr, label, &extents);
    cairo_move_to (cr, priv->xc - extents.width/2, priv->yc + priv->radius/2 - extents.y_bearing + 5);
    cairo_show_text (cr, label);

    g_snprintf (label, sizeof(label), "%s: %.1f",
        priv->ylabel != NULL ? priv->ylabel : "", 100 * priv->y);
    cairo_text_extents (cr, label, &extents);
    cairo_move_to (cr, priv->xc, priv->yc);
    cairo_rotate (cr, -M_PI/3);
    cairo_rel_move_to (cr, -extents.width/2, -priv->radius/2 - (extents.height + extents.y_bearing) - 5);
    cairo_show_text (cr, label);
    cairo_rotate (cr, M_PI/3);

    g_snprintf (label, sizeof(label), "%s: %.1f",
        priv->zlabel != NULL ? priv->zlabel : "", 100 * priv->z);
    cairo_text_extents (cr, label, &extents);
    cairo_move_to (cr, priv->xc , priv->yc);
    cairo_rotate (cr, M_PI/3);
    cairo_rel_move_to (cr, -extents.width/2, -priv->radius/2 - (extents.height + extents.y_bearing) - 5);
    cairo_show_text (cr, label);
    cairo_rotate (cr, -M_PI/3);

    /* altitudes */
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_move_to (cr,
        priv->x*priv->x1 + priv->y*priv->x2 + priv->z*priv->x3,
        priv->x*priv->y1 + priv->y*priv->y2 + priv->z*priv->y3);
    cairo_line_to (cr,
        (priv->y+priv->x/2)*priv->x2 + (priv->z+priv->x/2)*priv->x3,
        (priv->y+priv->x/2)*priv->y2 + (priv->z+priv->x/2)*priv->y3);
    cairo_stroke (cr);
    cairo_move_to (cr,
        priv->x*priv->x1 + priv->y*priv->x2 + priv->z*priv->x3,
        priv->x*priv->y1 + priv->y*priv->y2 + priv->z*priv->y3);
    cairo_line_to (cr,
        (priv->x+priv->y/2)*priv->x1 + (priv->z+priv->y/2)*priv->x3,
        (priv->x+priv->y/2)*priv->y1 + (priv->z+priv->y/2)*priv->y3);
    cairo_stroke (cr);
    cairo_move_to (cr,
        priv->x*priv->x1 + priv->y*priv->x2 + priv->z*priv->x3,
        priv->x*priv->y1 + priv->y*priv->y2 + priv->z*priv->y3);
    cairo_line_to (cr,
        (priv->x+priv->z/2)*priv->x1 + (priv->y+priv->z/2)*priv->x2,
        (priv->x+priv->z/2)*priv->y1 + (priv->y+priv->z/2)*priv->y2);
    cairo_stroke (cr);

    /* pointer */
    cairo_arc (cr,
        priv->x*priv->x1 + priv->y*priv->x2 + priv->z*priv->x3,
        priv->x*priv->y1 + priv->y*priv->y2 + priv->z*priv->y3,
        5, 0, 2 * M_PI);
    cairo_close_path(cr);
    cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
    cairo_fill_preserve (cr);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_stroke (cr);
}

static gboolean ternary_plot_expose (GtkWidget* plot, GdkEventExpose *event)
{
    cairo_t *cr;
    TernaryPlotPrivate *priv;

    /* update private data */
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* radius and center */
    priv->radius = (MIN (plot->allocation.width,
        (plot->allocation.height - 15) / sin (M_PI / 3)) - 5)*
        sin (M_PI/3)/3*2;
    priv->xc = plot->allocation.x + plot->allocation.width/2;
    priv->yc = plot->allocation.y + (plot->allocation.height - 15)/3*2;

    /* vetices */
    priv->x1 = priv->xc + priv->radius*0; /* cos (-M_PI/2) */
    priv->y1 = priv->yc + priv->radius*-1; /* sin (-M_PI/2) */
    priv->x2 = priv->xc + priv->radius*sqrt (3)/2; /* cos (-11*M_PI/6) */
    priv->y2 = priv->yc + priv->radius*0.5; /* sin (-11*M_PI/6) */
    priv->x3 = priv->xc + priv->radius*-sqrt (3)/2; /* cos (-7*M_PI/6) */
    priv->y3 = priv->yc + priv->radius*0.5; /* sin (-7*M_PI/6) */

    /* get a cairo_t */
    cr = gdk_cairo_create (plot->window);

    /* set a clip region for the expose event */
    cairo_rectangle (cr,
        event->area.x, event->area.y,
        event->area.width, event->area.height);
    cairo_clip (cr);

    draw (plot, cr);

    cairo_destroy (cr);

    return FALSE;
}

static gboolean ternary_plot_button_press (GtkWidget *plot, GdkEventButton *event)
{
    TernaryPlotPrivate *priv;
    gdouble dx, dy;

    if (event->button != 1)
        return FALSE;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* distance from mouse coordinates to pointer */
    dx = priv->x*priv->x1 + priv->y*priv->x2 + priv->z*priv->x3 - event->x;
    dy = priv->x*priv->y1 + priv->y*priv->y2 + priv->z*priv->y3 - event->y;

    /* if closer than 5 pixels, start dragging */
    if (sqrt (dx*dx + dy*dy) < 5)
        priv->is_dragged = TRUE;

    return FALSE;
}

static gboolean ternary_plot_motion_notify (GtkWidget *plot, GdkEventMotion *event)
{
    gdouble z;
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (!priv->is_dragged)
        return FALSE;

    z = ternary_plot_dot_to_line_distance (event->x, event->y,
        priv->x2, priv->y2, priv->x1, priv->y1) / (1.5 * priv->radius);
    if (z >= 0)
    {
        gdouble y;
        y = ternary_plot_dot_to_line_distance (event->x, event->y,
            priv->x1, priv->y1, priv->x3, priv->y3) / (1.5 * priv->radius);
        if (y >= 0)
        {
            gdouble x;
            x = ternary_plot_dot_to_line_distance (event->x, event->y,
                priv->x3, priv->y3, priv->x2, priv->y2) / (1.5 * priv->radius);
            if (x >= 0)
            {
                priv->x = x;
                priv->y = y;
                priv->z = z;

                gtk_widget_queue_draw (plot);
            }
        }
    }

    return FALSE;
}

static gboolean ternary_plot_button_release (GtkWidget *plot, GdkEventButton *event)
{
    TernaryPlotPrivate *priv;
    gdouble dx, dy, dz;
    gdouble px, py, pz;
    gdouble tol;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (event->button != 1 || !priv->is_dragged)
        return FALSE;

    tol = priv->tol;

    dx = fmod (priv->x, tol);
    dy = fmod (priv->y, tol);
    dz = fmod (priv->z, tol);

    if (dx + dy + dz > 1.5 * tol)
    {
        dx = dx - tol;
        dy = dy - tol;
        dz = dz - tol;
    }

    px = (fabs(dx)-tol)*(fabs(dx)-tol) + dy*dy + dz*dz;
    py = dx*dx + (fabs(dy)-tol)*(fabs(dy)-tol) + dz*dz;
    pz = dx*dx + dy*dy + (fabs(dz)-tol)*(fabs(dz)-tol);

    priv->x = priv->x - dx;
    priv->y = priv->y - dy;
    priv->z = priv->z - dz;

    if (px < py && px < pz)
        priv->x = priv->x + dx+dy+dz;
    else if (py < pz)
        priv->y = priv->y + dx+dy+dz;
    else
        priv->z = priv->z + dx+dy+dz;

    dx = priv->x;
    dy = priv->y;
    dz = priv->z;

    gtk_widget_queue_draw (plot);

    priv->is_dragged = FALSE;

    g_signal_emit (plot, signals[POINT_CHANGED], 0, priv->x, priv->y, priv->z);

    return FALSE;
}

GtkWidget * ternary_plot_new (void)
{
    return g_object_new (TERNARY_TYPE_PLOT, NULL);
}

void ternary_plot_set_xlabel (TernaryPlot *plot, const gchar *xlabel)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (priv->xlabel != NULL)
        g_free (priv->xlabel);
    priv->xlabel = g_strdup (xlabel);
}

void ternary_plot_set_ylabel (TernaryPlot *plot, const gchar *ylabel)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (priv->ylabel != NULL)
        g_free (priv->ylabel);
    priv->ylabel = g_strdup (ylabel);
}

void ternary_plot_set_zlabel (TernaryPlot *plot, const gchar *zlabel)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (priv->zlabel != NULL)
        g_free (priv->zlabel);
    priv->zlabel = g_strdup (zlabel);
}

void ternary_plot_set_point (TernaryPlot *plot, gdouble x, gdouble y, gdouble z)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    priv->x = fabs (x) / (fabs (x) + fabs (y) + fabs (z));
    priv->y = fabs (y) / (fabs (x) + fabs (y) + fabs (z));
    priv->z = fabs (z) / (fabs (x) + fabs (y) + fabs (z));

    g_signal_emit (plot, signals[POINT_CHANGED], 0, priv->x, priv->y, priv->z);
}

void ternary_plot_set_tolerance (TernaryPlot *plot, gdouble tol)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    priv->tol = CLAMP(tol, 0.1, 100.0) / 100.0;
}

const gchar* ternary_plot_get_xlabel (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->xlabel;
}

const gchar* ternary_plot_get_ylabel (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->ylabel;
}

const gchar* ternary_plot_get_zlabel (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->zlabel;
}

void ternary_plot_get_point (TernaryPlot *plot, double *x, double *y, double *z)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    if (x)
        *x = priv->x;
    if (y)
        *y = priv->y;
    if (z)
        *z = priv->z;
}

gdouble ternary_plot_get_tolerance (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->tol * 100.0;
}

static gdouble ternary_plot_dot_to_line_distance (gdouble x, gdouble y,
    gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
    gdouble d, dx, dy;

    dx = x2 - x1;
    dy = y2 - y1;
    d = ((y1 - y) * dx - (x1 - x) * dy) / sqrt (dx * dx + dy * dy);

    return d;
}