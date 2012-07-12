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

#define GETTEXT_PACKAGE "ternaryplot"
#include <glib/gi18n.h>

#define SENSITIVITY_THRESH 5

#define TERNARY_PLOT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                       TERNARY_TYPE_PLOT, TernaryPlotPrivate))

typedef struct _TernaryPlotPrivate TernaryPlotPrivate;

struct _TernaryPlotPrivate
{
    gdouble x1, y1, x2, y2, x3, y3; /* vertices */
    gdouble radius; /* radius */
    gdouble x, y, z; /* x-,y-,z-values */
    gchar *xlabel, *ylabel, *zlabel; /* x-,y-,z-labels */
    gdouble grid_step; /* grid step */
    gchar is_dragged; /* is pointer being dragged */
};

G_DEFINE_TYPE (TernaryPlot, ternary_plot, GTK_TYPE_DRAWING_AREA);

enum {
    PROP_0,
    PROP_TOLERANCE
};

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
static void     ternary_plot_size_allocate (GtkWidget *widget, GdkRectangle *allocation);
static void     ternary_plot_finalize (GObject *object);

static void     ternary_plot_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec);
static void      ternary_plot_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);

/* utility functions */
static gdouble   ternary_plot_dot_to_line_distance (gdouble x, gdouble y,
    gdouble x1, gdouble y1, gdouble x2, gdouble y2);

static void ternary_plot_class_init (TernaryPlotClass *class)
{
    GObjectClass *obj_class;
    GtkWidgetClass *widget_class;

    obj_class = G_OBJECT_CLASS (class);
    widget_class = GTK_WIDGET_CLASS (class);

    obj_class->finalize = ternary_plot_finalize;
    obj_class->set_property = ternary_plot_set_property;
    obj_class->get_property = ternary_plot_get_property;

    g_object_class_install_property (obj_class,
        PROP_TOLERANCE,
        g_param_spec_double ("tolerance",
            _("Rounding tolerance in percents"),
            _("Current value is rounded to value proportional to the tolerance"),
            0.1, 100.0, 10.0, (G_PARAM_READABLE | G_PARAM_WRITABLE)));

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
    widget_class->size_allocate = ternary_plot_size_allocate;

    g_type_class_add_private (obj_class, sizeof (TernaryPlotPrivate));
}

static void ternary_plot_init (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    priv->x = 1./3;
    priv->y = 1./3;
    priv->z = 1./3;

    plot->tol = 0.1;
    priv->grid_step = 0.1; /* 10% */

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

static void ternary_plot_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
    TernaryPlot *plot = TERNARY_PLOT (object);

    switch (prop_id)
    {
    case PROP_TOLERANCE:
        g_value_set_double (value, plot->tol);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void ternary_plot_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
    TernaryPlot *plot = TERNARY_PLOT (object);

    switch (prop_id)
    {
    case PROP_TOLERANCE:
        ternary_plot_set_tolerance (plot, g_value_get_double (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void draw_field (GtkWidget *plot, cairo_t *cr)
{
    gdouble frac;
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* large triangle */
    cairo_move_to (cr, priv->x1, priv->y1);
    cairo_line_to (cr, priv->x2, priv->y2);
    cairo_line_to (cr, priv->x3, priv->y3);
    cairo_close_path (cr);
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

    for (frac = priv->grid_step; frac < 0.5; frac += priv->grid_step)
    {
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
}

static void draw_label (cairo_t *cr, const char *label, gdouble x, gdouble y,
    gdouble angle)
{
    cairo_text_extents_t extents;

    cairo_text_extents (cr, label, &extents);
    cairo_move_to (cr, x, y);
    cairo_rotate (cr, angle);
    cairo_rel_move_to (cr, -extents.width/2,
                       angle == 0.0 ?
                       (extents.height + 2) :
                       (-extents.height - extents.y_bearing - 2));
    cairo_show_text (cr, label);
    cairo_rotate (cr, -angle);
}

static void draw_labels (GtkWidget *plot, cairo_t *cr)
{
    gchar label[100];
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* font settings */
    cairo_set_font_size (cr, 12);
    cairo_select_font_face (cr, "Nimbus Sans L", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    g_snprintf (label, sizeof(label), "%s: %.1f%%",
        priv->xlabel != NULL ? priv->xlabel : "", 100 * priv->x);
    draw_label (cr, label,
                (priv->x2 + priv->x3) / 2, (priv->y2 + priv->y3) / 2, 0.0);

    g_snprintf (label, sizeof(label), "%s: %.1f%%",
        priv->ylabel != NULL ? priv->ylabel : "", 100 * priv->y);
    draw_label (cr, label,
                (priv->x1 + priv->x3) / 2, (priv->y1 + priv->y3) / 2, -M_PI / 3);

    g_snprintf (label, sizeof(label), "%s: %.1f%%",
        priv->zlabel != NULL ? priv->zlabel : "", 100 * priv->z);
    draw_label (cr, label,
                (priv->x1 + priv->x2) / 2, (priv->y1 + priv->y2) / 2, M_PI / 3);
}

static void draw_pointer (GtkWidget *plot, cairo_t *cr)
{
    gdouble px, py;
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* altitudes */
    px = priv->x * priv->x1 + priv->y * priv->x2 + priv->z * priv->x3;
    py = priv->x * priv->y1 + priv->y * priv->y2 + priv->z * priv->y3;
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_move_to (cr,
        px + priv->x * ((priv->x2 + priv->x3) / 2 - priv->x1),
        py + priv->x * ((priv->y2 + priv->y3) / 2 - priv->y1));
    cairo_line_to (cr, px, py);
    cairo_rel_line_to (cr,
        priv->y * ((priv->x1 + priv->x3) / 2 - priv->x2),
        priv->y * ((priv->y1 + priv->y3) / 2 - priv->y2));
    cairo_stroke (cr);
    cairo_move_to (cr, px, py);
    cairo_rel_line_to (cr,
        priv->z * ((priv->x1 + priv->x2) / 2 - priv->x3),
        priv->z * ((priv->y1 + priv->y2) / 2 - priv->y3));
    cairo_stroke (cr);

    /* pointer */
    cairo_arc (cr, px, py, 5, 0, 2 * M_PI);
    cairo_close_path(cr);
    cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
    cairo_fill_preserve (cr);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_stroke (cr);
}

static void ternary_plot_size_allocate (GtkWidget *plot,
    GdkRectangle *allocation)
{
    gdouble xc, yc;
    TernaryPlotPrivate *priv;

    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* radius and center */
    priv->radius = (MIN (allocation->width,
                         (allocation->height - 15) / sin (M_PI / 3)) - 5)*
                         sin (M_PI / 3) * 2 / 3;
    xc = allocation->x + allocation->width / 2;
    yc = allocation->y + (allocation->height - 15) * 2 / 3;

    /* vertices */
    priv->x1 = xc + priv->radius * 0; /* cos (-M_PI/2) */
    priv->y1 = yc + priv->radius * -1; /* sin (-M_PI/2) */
    priv->x2 = xc + priv->radius * sqrt (3)/2; /* cos (-11*M_PI/6) */
    priv->y2 = yc + priv->radius * 0.5; /* sin (-11*M_PI/6) */
    priv->x3 = xc + priv->radius * -sqrt (3)/2; /* cos (-7*M_PI/6) */
    priv->y3 = yc + priv->radius * 0.5; /* sin (-7*M_PI/6) */

    GTK_WIDGET_CLASS (ternary_plot_parent_class)->size_allocate (plot, allocation);
}

static gboolean ternary_plot_expose (GtkWidget* plot, GdkEventExpose *event)
{
    cairo_t *cr;
    TernaryPlotPrivate *priv;

    /* update private data */
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    /* get a cairo_t */
    cr = gdk_cairo_create (plot->window);

    /* set a clip region for the expose event */
    cairo_rectangle (cr,
        event->area.x, event->area.y,
        event->area.width, event->area.height);
    cairo_clip (cr);

    draw_field (plot, cr);
    draw_labels (plot, cr);
    draw_pointer (plot, cr);

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
    dx = priv->x * priv->x1 + priv->y * priv->x2 + priv->z * priv->x3 - event->x;
    dy = priv->x * priv->y1 + priv->y * priv->y2 + priv->z * priv->y3 - event->y;

    /* if closer than SENSITIVITY_THRESH pixels, start dragging */
    if (dx * dx + dy * dy < SENSITIVITY_THRESH * SENSITIVITY_THRESH)
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

static gboolean ternary_plot_button_release (GtkWidget *widget, GdkEventButton *event)
{
    TernaryPlot *plot;
    TernaryPlotPrivate *priv;
    gdouble round_err;
    gdouble new_x, new_y, new_z;
    gdouble tol;

    plot = TERNARY_PLOT (widget);
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (event->button != 1 || !priv->is_dragged)
        return FALSE;

    tol = plot->tol;

    new_x = roundf (priv->x / tol) * tol;
    new_y = roundf (priv->y / tol) * tol;
    new_z = roundf (priv->z / tol) * tol;

    round_err = 1.0 - (new_x + new_y + new_z);
    if (fabs (round_err) > tol / 2)
    {
        gdouble dx, dy, dz;
        dx = (priv->x - new_x) / round_err;
        dy = (priv->y - new_y) / round_err;
        dz = (priv->z - new_z) / round_err;

        if (dx > dy && dx > dz)
            new_x += round_err;
        else if (dy > dz)
            new_y += round_err;
        else
            new_z += round_err;

    }

    priv->x = new_x;
    priv->y = new_y;
    priv->z = new_z;

    gtk_widget_queue_draw (widget);

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

    g_return_if_fail (TERNARY_IS_PLOT (plot));
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (priv->xlabel != NULL)
        g_free (priv->xlabel);
    priv->xlabel = g_strdup (xlabel);
}

void ternary_plot_set_ylabel (TernaryPlot *plot, const gchar *ylabel)
{
    TernaryPlotPrivate *priv;

    g_return_if_fail (TERNARY_IS_PLOT (plot));
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (priv->ylabel != NULL)
        g_free (priv->ylabel);
    priv->ylabel = g_strdup (ylabel);
}

void ternary_plot_set_zlabel (TernaryPlot *plot, const gchar *zlabel)
{
    TernaryPlotPrivate *priv;

    g_return_if_fail (TERNARY_IS_PLOT (plot));
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    if (priv->zlabel != NULL)
        g_free (priv->zlabel);
    priv->zlabel = g_strdup (zlabel);
}

void ternary_plot_set_point (TernaryPlot *plot, gdouble x, gdouble y, gdouble z)
{
    TernaryPlotPrivate *priv;

    g_return_if_fail (TERNARY_IS_PLOT (plot));
    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    priv->x = fabs (x) / (fabs (x) + fabs (y) + fabs (z));
    priv->y = fabs (y) / (fabs (x) + fabs (y) + fabs (z));
    priv->z = fabs (z) / (fabs (x) + fabs (y) + fabs (z));

    g_signal_emit (plot, signals[POINT_CHANGED], 0, priv->x, priv->y, priv->z);
}

void ternary_plot_set_tolerance (TernaryPlot *plot, gdouble tol)
{
    gdouble tolerance;
    TernaryPlotPrivate *priv;

    g_return_if_fail (TERNARY_IS_PLOT (plot));
    priv = TERNARY_PLOT_GET_PRIVATE (plot);

    tolerance = CLAMP(tol, 0.1, 100.0) / 100.0;
    if (plot->tol != tolerance) {
        plot->tol = tolerance;
        g_object_notify (G_OBJECT (plot), "tolerance");
    }
}

const gchar* ternary_plot_get_xlabel (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    g_return_val_if_fail (TERNARY_IS_PLOT (plot), NULL);
    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->xlabel;
}

const gchar* ternary_plot_get_ylabel (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    g_return_val_if_fail (TERNARY_IS_PLOT (plot), NULL);
    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->ylabel;
}

const gchar* ternary_plot_get_zlabel (TernaryPlot *plot)
{
    TernaryPlotPrivate *priv;

    g_return_val_if_fail (TERNARY_IS_PLOT (plot), NULL);
    priv = TERNARY_PLOT_GET_PRIVATE (plot);
    return priv->zlabel;
}

void ternary_plot_get_point (TernaryPlot *plot, gdouble *x, gdouble *y, gdouble *z)
{
    TernaryPlotPrivate *priv;

    g_return_if_fail (TERNARY_IS_PLOT (plot));
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
    g_return_val_if_fail (TERNARY_IS_PLOT (plot), 0.0);

    return plot->tol * 100.0;
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
