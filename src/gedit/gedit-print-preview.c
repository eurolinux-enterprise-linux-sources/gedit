/*
 * gedit-print-preview.c
 *
 * Copyright (C) 2008 Paolo Borelli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-print-preview.h"

#include <math.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <cairo-pdf.h>

#define PRINTER_DPI (72.)
#define TOOLTIP_THRESHOLD 20

struct _GeditPrintPreviewPrivate
{
	GtkPrintOperation *operation;
	GtkPrintContext *context;
	GtkPrintOperationPreview *gtk_preview;

	GtkToolItem *next;
	GtkToolItem *prev;
	GtkWidget   *page_entry;
	GtkWidget   *last;
	GtkToolItem *multi;
	GtkToolItem *zoom_one;
	GtkToolItem *zoom_fit;
	GtkToolItem *zoom_in;
	GtkToolItem *zoom_out;
	GtkToolItem *close;

	GtkWidget *layout;

	/* real size of the page in inches */
	double paper_w;
	double paper_h;
	double dpi;

	double scale;

	/* size of the tile of a page (including padding
	 * and drop shadow) in pixels */
	gint tile_w;
	gint tile_h;

	/* multipage support */
	gint rows;
	gint cols;

	guint n_pages;
	guint cur_page;
	gint cursor_x;
	gint cursor_y;

	gboolean has_tooltip;

};

G_DEFINE_TYPE_WITH_PRIVATE (GeditPrintPreview, gedit_print_preview, GTK_TYPE_GRID)

static void
gedit_print_preview_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	/* GeditPrintPreview *preview = GEDIT_PRINT_PREVIEW (object); */

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_print_preview_set_property (GObject      *object,
				  guint	        prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	/* GeditPrintPreview *preview = GEDIT_PRINT_PREVIEW (object); */

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_print_preview_finalize (GObject *object)
{
	/* GeditPrintPreview *preview = GEDIT_PRINT_PREVIEW (object); */

	G_OBJECT_CLASS (gedit_print_preview_parent_class)->finalize (object);
}

static void
gedit_print_preview_grab_focus (GtkWidget *widget)
{
	GeditPrintPreview *preview;

	preview = GEDIT_PRINT_PREVIEW (widget);

	gtk_widget_grab_focus (GTK_WIDGET (preview->priv->layout));
}

static void
gedit_print_preview_class_init (GeditPrintPreviewClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (klass);
	widget_class = GTK_WIDGET_CLASS (klass);

	object_class->get_property = gedit_print_preview_get_property;
	object_class->set_property = gedit_print_preview_set_property;
	object_class->finalize = gedit_print_preview_finalize;

	widget_class->grab_focus = gedit_print_preview_grab_focus;

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-print-preview.ui");
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, prev);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, next);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, multi);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, page_entry);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, last);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, zoom_one);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, zoom_fit);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, zoom_in);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, zoom_out);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, close);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPrintPreview, layout);
}

static void
get_adjustments (GeditPrintPreview  *preview,
		 GtkAdjustment     **hadj,
		 GtkAdjustment     **vadj)
{
	GeditPrintPreviewPrivate *priv;

	priv = preview->priv;

	*hadj = gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (priv->layout));
	*vadj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (priv->layout));
}

static void
update_layout_size (GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;

	priv = preview->priv;

	/* force size of the drawing area to make the scrolled window work */
	gtk_layout_set_size (GTK_LAYOUT (priv->layout),
			     priv->tile_w * priv->cols,
			     priv->tile_h * priv->rows);

	gtk_widget_queue_draw (preview->priv->layout);
}

static void
set_rows_and_cols (GeditPrintPreview *preview,
		   gint	              rows,
		   gint	              cols)
{
	/* TODO: set the zoom appropriately */

	preview->priv->rows = rows;
	preview->priv->cols = cols;
	update_layout_size (preview);
}

/* get the paper size in points: these must be used only
 * after the widget has been mapped and the dpi is known */

static double
get_paper_width (GeditPrintPreview *preview)
{
	return preview->priv->paper_w * preview->priv->dpi;
}

static double
get_paper_height (GeditPrintPreview *preview)
{
	return preview->priv->paper_h * preview->priv->dpi;
}

#define PAGE_PAD 12
#define PAGE_SHADOW_OFFSET 5

/* The tile size is the size of the area where a page
 * will be drawn including the padding and idependent
 * of the orientation */

/* updates the tile size to the current zoom and page size */
static void
update_tile_size (GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;
	gint w, h;

	priv = preview->priv;

	w = 2 * PAGE_PAD + floor (priv->scale * get_paper_width (preview) + 0.5);
	h = 2 * PAGE_PAD + floor (priv->scale * get_paper_height (preview) + 0.5);

	priv->tile_w = w;
	priv->tile_h = h;
}

/* Zoom should always be set with one of these two function
 * so that the tile size is properly updated */

static void
set_zoom_factor (GeditPrintPreview *preview,
		 double	            zoom)
{
	GeditPrintPreviewPrivate *priv;

	priv = preview->priv;

	priv->scale = zoom;

	update_tile_size (preview);
	update_layout_size (preview);
}

static void
set_zoom_fit_to_size (GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;
	GtkAdjustment *hadj, *vadj;
	double width, height;
	double p_width, p_height;
	double zoomx, zoomy;

	priv = preview->priv;

	get_adjustments (preview, &hadj, &vadj);

	g_object_get (hadj, "page-size", &width, NULL);
	g_object_get (vadj, "page-size", &height, NULL);

	width /= priv->cols;
	height /= priv->rows;

	p_width = get_paper_width (preview);
	p_height = get_paper_height (preview);

	zoomx = MAX (1, width - 2 * PAGE_PAD) / p_width;
	zoomy = MAX (1, height - 2 * PAGE_PAD) / p_height;

	if (zoomx <= zoomy)
	{
		priv->tile_w = width;
		priv->tile_h = floor (0.5 + width * (p_height / p_width));
		priv->scale = zoomx;
	}
	else
	{
		priv->tile_w = floor (0.5 + height * (p_width / p_height));
		priv->tile_h = height;
		priv->scale = zoomy;
	}

	update_layout_size (preview);
}

#define ZOOM_IN_FACTOR (1.2)
#define ZOOM_OUT_FACTOR (1.0 / ZOOM_IN_FACTOR)

static void
zoom_in (GeditPrintPreview *preview)
{
	set_zoom_factor (preview,
			 preview->priv->scale * ZOOM_IN_FACTOR);
}

static void
zoom_out (GeditPrintPreview *preview)
{
	set_zoom_factor (preview,
			 preview->priv->scale * ZOOM_OUT_FACTOR);
}

static void
goto_page (GeditPrintPreview *preview,
	   gint               page)
{
	gchar c[32];

	g_snprintf (c, 32, "%d", page + 1);
	gtk_entry_set_text (GTK_ENTRY (preview->priv->page_entry), c);

	gtk_widget_set_sensitive (GTK_WIDGET (preview->priv->prev),
				  (page > 0) && (preview->priv->n_pages > 1));
	gtk_widget_set_sensitive (GTK_WIDGET (preview->priv->next),
				  (page != (preview->priv->n_pages - 1)) &&
				  (preview->priv->n_pages > 1));

	if (page != preview->priv->cur_page)
	{
		preview->priv->cur_page = page;
		if (preview->priv->n_pages > 0)
			gtk_widget_queue_draw (preview->priv->layout);
	}
}

static void
prev_button_clicked (GtkWidget         *button,
		     GeditPrintPreview *preview)
{
	GdkEvent *event;
	gint page;

	event = gtk_get_current_event ();

	if (event->button.state & GDK_SHIFT_MASK)
		page = 0;
	else
		page = preview->priv->cur_page - preview->priv->rows * preview->priv->cols;

 	goto_page (preview, MAX (page, 0));

	gdk_event_free (event);
}

static void
next_button_clicked (GtkWidget         *button,
		     GeditPrintPreview *preview)
{
	GdkEvent *event;
	gint page;

	event = gtk_get_current_event ();

	if (event->button.state & GDK_SHIFT_MASK)
		page = preview->priv->n_pages - 1;
	else
		page = preview->priv->cur_page + preview->priv->rows * preview->priv->cols;

 	goto_page (preview, MIN (page, preview->priv->n_pages - 1));

	gdk_event_free (event);
}

static void
page_entry_activated (GtkEntry          *entry,
		      GeditPrintPreview *preview)
{
	const gchar *text;
	gint page;

	text = gtk_entry_get_text (entry);

	page = CLAMP (atoi (text), 1, preview->priv->n_pages) - 1;
	goto_page (preview, page);

	gtk_widget_grab_focus (GTK_WIDGET (preview->priv->layout));
}

static void
page_entry_insert_text (GtkEditable *editable,
			const gchar *text,
			gint         length,
			gint        *position)
{
	gunichar c;
	const gchar *p;
 	const gchar *end;

	p = text;
	end = text + length;

	while (p != end)
	{
		const gchar *next;
		next = g_utf8_next_char (p);

		c = g_utf8_get_char (p);

		if (!g_unichar_isdigit (c))
		{
			g_signal_stop_emission_by_name (editable, "insert-text");
			break;
		}

		p = next;
	}
}

static gboolean
page_entry_focus_out (GtkWidget         *widget,
		      GdkEventFocus     *event,
		      GeditPrintPreview *preview)
{
	const gchar *text;
	gint page;

	text = gtk_entry_get_text (GTK_ENTRY (widget));
	page = atoi (text) - 1;

	/* Reset the page number only if really needed */
	if (page != preview->priv->cur_page)
	{
		gchar *str;

		str = g_strdup_printf ("%d", preview->priv->cur_page + 1);
		gtk_entry_set_text (GTK_ENTRY (widget), str);
		g_free (str);
	}

	return FALSE;
}

static void
on_1x1_clicked (GtkMenuItem       *i,
		GeditPrintPreview *preview)
{
	set_rows_and_cols (preview, 1, 1);
}

static void
on_1x2_clicked (GtkMenuItem       *i,
		GeditPrintPreview *preview)
{
	set_rows_and_cols (preview, 1, 2);
}

static void
on_2x1_clicked (GtkMenuItem       *i,
		GeditPrintPreview *preview)
{
	set_rows_and_cols (preview, 2, 1);
}

static void
on_2x2_clicked (GtkMenuItem       *i,
		GeditPrintPreview *preview)
{
	set_rows_and_cols (preview, 2, 2);
}

static void
multi_button_clicked (GtkWidget	        *button,
		      GeditPrintPreview *preview)
{
	GtkWidget *m, *i;

	m = gtk_menu_new ();
	gtk_widget_show (m);
	g_signal_connect (m,
			 "selection_done",
			  G_CALLBACK (gtk_widget_destroy),
			  m);

	i = gtk_menu_item_new_with_label ("1x1");
	gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 0, 1, 0, 1);
	g_signal_connect (i, "activate", G_CALLBACK (on_1x1_clicked), preview);

	i = gtk_menu_item_new_with_label ("2x1");
	gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 0, 1, 1, 2);
	g_signal_connect (i, "activate", G_CALLBACK (on_2x1_clicked), preview);

	i = gtk_menu_item_new_with_label ("1x2");
	gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 1, 2, 0, 1);
	g_signal_connect (i, "activate", G_CALLBACK (on_1x2_clicked), preview);

	i = gtk_menu_item_new_with_label ("2x2");
	gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 1, 2, 1, 2);
	g_signal_connect (i, "activate", G_CALLBACK (on_2x2_clicked), preview);

	gtk_menu_popup (GTK_MENU (m),
			NULL, NULL, NULL, preview, 0,
			GDK_CURRENT_TIME);
}

static void
zoom_one_button_clicked (GtkWidget         *button,
			 GeditPrintPreview *preview)
{
	set_zoom_factor (preview, 1);
}

static void
zoom_fit_button_clicked (GtkWidget         *button,
			 GeditPrintPreview *preview)
{
	set_zoom_fit_to_size (preview);
}

static void
zoom_in_button_clicked (GtkWidget         *button,
			GeditPrintPreview *preview)
{
	zoom_in (preview);
}

static void
zoom_out_button_clicked (GtkWidget         *button,
			 GeditPrintPreview *preview)
{
	zoom_out (preview);
}

static void
close_button_clicked (GtkWidget         *button,
		      GeditPrintPreview *preview)
{
	gtk_widget_destroy (GTK_WIDGET (preview));
}

static gint
get_first_page_displayed (GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;

	priv = preview->priv;

	return priv->cur_page - priv->cur_page % (priv->cols * priv->rows);
}

/* returns the page number (starting from 0) or -1 if no page */
static gint
get_page_at_coords (GeditPrintPreview *preview,
		    gint               x,
		    gint               y)
{
	GeditPrintPreviewPrivate *priv;
	GtkAdjustment *hadj, *vadj;
	gint r, c, pg;

	priv = preview->priv;

	if (priv->tile_h <= 0 || priv->tile_w <= 0)
		return -1;

	get_adjustments (preview, &hadj, &vadj);

	x += gtk_adjustment_get_value (hadj);
	y += gtk_adjustment_get_value (vadj);

	r = 1 + y / (priv->tile_h);
	c = 1 + x / (priv->tile_w);

	if (c > priv->cols)
		return -1;

	pg = get_first_page_displayed (preview) - 1;
	pg += (r - 1) * priv->cols + c;

	if (pg >= priv->n_pages)
		return -1;

	/* FIXME: we could try to be picky and check
	 * if we actually are inside the page */
	return pg;
}

static gboolean
on_preview_layout_motion_notify (GtkWidget         *widget,
                                 GdkEvent          *event,
                                 GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;
	gint temp_x;
	gint temp_y;
	gint diff_x;
	gint diff_y;
	priv = preview->priv;

	temp_x = ((GdkEventMotion*)event)->x;
	temp_y = ((GdkEventMotion*)event)->y;
	diff_x = abs (temp_x - priv->cursor_x);
	diff_y = abs (temp_y - priv->cursor_y);

	if ((diff_x >= TOOLTIP_THRESHOLD) || (diff_y >= TOOLTIP_THRESHOLD))

	{
		priv->has_tooltip = FALSE;
		priv->cursor_x = temp_x;
		priv->cursor_y = temp_y;
	}
	else
	{
		priv->has_tooltip = TRUE;

	}

	return GDK_EVENT_STOP;
}

static gboolean
preview_layout_query_tooltip (GtkWidget         *widget,
			      gint               x,
			      gint               y,
			      gboolean           keyboard_tip,
			      GtkTooltip        *tooltip,
			      GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;
	gint pg;
	gchar *tip;

	priv = preview->priv;

	if (priv->has_tooltip == TRUE)
	{
		pg = get_page_at_coords (preview, x, y);
		if (pg < 0)
			return FALSE;

		tip = g_strdup_printf (_("Page %d of %d"), pg + 1, preview->priv->n_pages);
		gtk_tooltip_set_text (tooltip, tip);
		g_free (tip);

		return TRUE;
	}
	else
	{
		priv->has_tooltip = TRUE;
		return FALSE;
	}

}

static gint
preview_layout_key_press (GtkWidget         *widget,
			  GdkEventKey       *event,
			  GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;
	GtkAdjustment *hadj, *vadj;
	double x, y;
	guint h, w;
	double hlower, hupper, vlower, vupper;
	double hpage, vpage;
	double hstep, vstep;
	gboolean domove = FALSE;
	gboolean ret = TRUE;

	priv = preview->priv;

	get_adjustments (preview, &hadj, &vadj);

	x = gtk_adjustment_get_value (hadj);
	y = gtk_adjustment_get_value (vadj);

	g_object_get (hadj,
		      "lower", &hlower,
		      "upper", &hupper,
		      "page-size", &hpage,
		      NULL);
	g_object_get (vadj,
		      "lower", &vlower,
		      "upper", &vupper,
		      "page-size", &vpage,
		      NULL);

	gtk_layout_get_size (GTK_LAYOUT (priv->layout), &w, &h);

	hstep = 10;
	vstep = 10;

	switch (event->keyval)
	{
		case '1':
			set_zoom_fit_to_size (preview);
			break;
		case '+':
		case '=':
		case GDK_KEY_KP_Add:
			zoom_in (preview);
			break;
		case '-':
		case '_':
		case GDK_KEY_KP_Subtract:
			zoom_out (preview);
			break;
		case GDK_KEY_KP_Right:
		case GDK_KEY_Right:
			if (event->state & GDK_SHIFT_MASK)
				x = hupper - hpage;
			else
				x = MIN (hupper - hpage, x + hstep);
			domove = TRUE;
			break;
		case GDK_KEY_KP_Left:
		case GDK_KEY_Left:
			if (event->state & GDK_SHIFT_MASK)
				x = hlower;
			else
				x = MAX (hlower, x - hstep);
			domove = TRUE;
			break;
		case GDK_KEY_KP_Up:
		case GDK_KEY_Up:
			if (event->state & GDK_SHIFT_MASK)
				goto page_up;
			y = MAX (vlower, y - vstep);
			domove = TRUE;
			break;
		case GDK_KEY_KP_Down:
		case GDK_KEY_Down:
			if (event->state & GDK_SHIFT_MASK)
				goto page_down;
			y = MIN (vupper - vpage, y + vstep);
			domove = TRUE;
			break;
		case GDK_KEY_KP_Page_Up:
		case GDK_KEY_Page_Up:
		case GDK_KEY_Delete:
		case GDK_KEY_KP_Delete:
		case GDK_KEY_BackSpace:
		page_up:
			if (y <= vlower)
			{
				if (preview->priv->cur_page > 0)
				{
					goto_page (preview, preview->priv->cur_page - 1);
					y = (vupper - vpage);
				}
			}
			else
			{
				y = vlower;
			}
			domove = TRUE;
			break;
		case GDK_KEY_KP_Page_Down:
		case GDK_KEY_Page_Down:
		case ' ':
		page_down:
			if (y >= (vupper - vpage))
			{
				if (preview->priv->cur_page < preview->priv->n_pages - 1)
				{
					goto_page (preview, preview->priv->cur_page + 1);
					y = vlower;
				}
			}
			else
			{
				y = (vupper - vpage);
			}
			domove = TRUE;
			break;
		case GDK_KEY_KP_Home:
		case GDK_KEY_Home:
			goto_page (preview, 0);
			y = 0;
			domove = TRUE;
			break;
		case GDK_KEY_KP_End:
		case GDK_KEY_End:
			goto_page (preview, preview->priv->n_pages - 1);
			y = 0;
			domove = TRUE;
			break;
		case GDK_KEY_Escape:
			gtk_widget_destroy (GTK_WIDGET (preview));
			break;
		case 'c':
			if (event->state & GDK_MOD1_MASK)
			{
				gtk_widget_destroy (GTK_WIDGET (preview));
			}
			break;
		case 'p':
			if (event->state & GDK_MOD1_MASK)
			{
				gtk_widget_grab_focus (preview->priv->page_entry);
			}
			break;
		default:
			/* by default do not stop the default handler */
			ret = FALSE;
	}

	if (domove)
	{
		gtk_adjustment_set_value (hadj, x);
		gtk_adjustment_set_value (vadj, y);

		gtk_adjustment_value_changed (hadj);
		gtk_adjustment_value_changed (vadj);
	}

	return ret;
}

static void
gedit_print_preview_init (GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;

	priv = gedit_print_preview_get_instance_private (preview);
	preview->priv = priv;

	priv->operation = NULL;
	priv->context = NULL;
	priv->gtk_preview = NULL;

	gtk_widget_init_template (GTK_WIDGET (preview));

	g_signal_connect (priv->prev,
			  "clicked",
			  G_CALLBACK (prev_button_clicked),
			  preview);
	g_signal_connect (priv->next,
			  "clicked",
			  G_CALLBACK (next_button_clicked),
			  preview);
	g_signal_connect (priv->page_entry,
			  "activate",
			  G_CALLBACK (page_entry_activated),
			  preview);
	g_signal_connect (priv->page_entry,
			  "insert-text",
			  G_CALLBACK (page_entry_insert_text),
			  NULL);
	g_signal_connect (priv->page_entry,
			  "focus-out-event",
			  G_CALLBACK (page_entry_focus_out),
			  preview);
	g_signal_connect (priv->multi,
			  "clicked",
			  G_CALLBACK (multi_button_clicked),
			  preview);
	g_signal_connect (priv->zoom_one,
			  "clicked",
			  G_CALLBACK (zoom_one_button_clicked),
			  preview);
	g_signal_connect (priv->zoom_fit,
			  "clicked",
			  G_CALLBACK (zoom_fit_button_clicked),
			  preview);
	g_signal_connect (priv->zoom_in,
			  "clicked",
			  G_CALLBACK (zoom_in_button_clicked),
			  preview);
	g_signal_connect (priv->zoom_out,
			  "clicked",
			  G_CALLBACK (zoom_out_button_clicked),
			  preview);
	g_signal_connect (priv->close,
			  "clicked",
			  G_CALLBACK (close_button_clicked),
			  preview);

	g_object_set (priv->layout, "has-tooltip", TRUE, NULL);
  	g_signal_connect (priv->layout,
			  "query-tooltip",
			  G_CALLBACK (preview_layout_query_tooltip),
			  preview);
  	g_signal_connect (priv->layout,
			  "key-press-event",
			  G_CALLBACK (preview_layout_key_press),
			  preview);

	/* hide the tooltip once we move the cursor, since gtk does not do it for us */
	g_signal_connect (priv->layout,
			  "motion-notify-event",
			  G_CALLBACK (on_preview_layout_motion_notify),
			  preview);
	gtk_widget_grab_focus (GTK_WIDGET (priv->layout));

	/* FIXME */
	priv->cur_page = 0;
	priv->paper_w = 0;
	priv->paper_h = 0;
	priv->dpi = PRINTER_DPI;
	priv->scale = 1.0;
	priv->rows = 1;
	priv->cols = 1;
	priv->cursor_x = 0;
	priv->cursor_y = 0;
	priv->has_tooltip = TRUE;
}

static void
draw_page_content (cairo_t            *cr,
		   gint	               page_number,
		   GeditPrintPreview  *preview)
{
	/* scale to the desired size */
	cairo_scale (cr, preview->priv->scale, preview->priv->scale);

	gtk_print_context_set_cairo_context (preview->priv->context,
					     cr,
					     preview->priv->dpi,
					     preview->priv->dpi);

	gtk_print_operation_preview_render_page (preview->priv->gtk_preview,
						 page_number);
}

/* For the frame, we scale and rotate manually, since
 * the line width should not depend on the zoom and
 * the drop shadow should be on the bottom right no matter
 * the orientation */
static void
draw_page_frame (cairo_t            *cr,
		 GeditPrintPreview  *preview)
{
	double w, h;

	w = get_paper_width (preview);
	h = get_paper_height (preview);

	w *= preview->priv->scale;
	h *= preview->priv->scale;

	/* drop shadow */
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr,
			 PAGE_SHADOW_OFFSET, PAGE_SHADOW_OFFSET,
			 w, h);
	cairo_fill (cr);

	/* page frame */
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_rectangle (cr,
			 0, 0,
			 w, h);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_set_line_width (cr, 1);
	cairo_stroke (cr);
}

static void
draw_page (cairo_t           *cr,
	   double             x,
	   double             y,
	   gint	              page_number,
	   GeditPrintPreview *preview)
{
	cairo_save (cr);

	/* move to the page top left corner */
	cairo_translate (cr, x + PAGE_PAD, y + PAGE_PAD);

	draw_page_frame (cr, preview);
	draw_page_content (cr, page_number, preview);

	cairo_restore (cr);
}

static gboolean
preview_draw (GtkWidget         *widget,
	      cairo_t           *cr,
	      GeditPrintPreview *preview)
{
	GeditPrintPreviewPrivate *priv;
	GdkWindow *bin_window;
	gint pg;
	gint i, j;

	priv = preview->priv;

	bin_window = gtk_layout_get_bin_window (GTK_LAYOUT (priv->layout));

	if (gtk_cairo_should_draw_window (cr, bin_window))
	{
		cairo_save (cr);

		gtk_cairo_transform_to_window (cr, widget, bin_window);

		/* get the first page to display */
		pg = get_first_page_displayed (preview);

		for (i = 0; i < priv->cols; ++i)
		{
			for (j = 0; j < priv->rows; ++j)
			{
				if (!gtk_print_operation_preview_is_selected (priv->gtk_preview,
									      pg))
				{
					continue;
				}

				if (pg == priv->n_pages)
					break;

				draw_page (cr,
					   j * priv->tile_w,
					   i * priv->tile_h,
					   pg,
					   preview);

				++pg;
			}
		}

		cairo_restore (cr);
	}

	return TRUE;
}

static double
get_screen_dpi (GeditPrintPreview *preview)
{
	GdkScreen *screen;
	double dpi;

	screen = gtk_widget_get_screen (GTK_WIDGET (preview));

	dpi = gdk_screen_get_resolution (screen);
	if (dpi < 30. || 600. < dpi)
	{
		g_warning ("Invalid the x-resolution for the screen, assuming 96dpi");
		dpi = 96.;
	}

	return dpi;
}

static void
set_n_pages (GeditPrintPreview *preview,
	     gint               n_pages)
{
	gchar *str;

	preview->priv->n_pages = n_pages;

	/* FIXME: count the visible pages */

	str =  g_strdup_printf ("%d", n_pages);
	gtk_label_set_markup (GTK_LABEL (preview->priv->last), str);
	g_free (str);
}

static void
preview_ready (GtkPrintOperationPreview *gtk_preview,
	       GtkPrintContext          *context,
	       GeditPrintPreview        *preview)
{
	gint n_pages;

	g_object_get (preview->priv->operation, "n-pages", &n_pages, NULL);
	set_n_pages (preview, n_pages);
	goto_page (preview, 0);

	/* figure out the dpi */
	preview->priv->dpi = get_screen_dpi (preview);

	set_zoom_factor (preview, 1.0);

	/* let the default gtklayout handler clear the background */
	g_signal_connect_after (preview->priv->layout,
				"draw",
				G_CALLBACK (preview_draw),
				preview);

	gtk_widget_queue_draw (preview->priv->layout);
}

static void
update_paper_size (GeditPrintPreview *preview,
		   GtkPageSetup      *page_setup)
{
	preview->priv->paper_w = gtk_page_setup_get_paper_width (page_setup, GTK_UNIT_INCH);
	preview->priv->paper_h = gtk_page_setup_get_paper_height (page_setup, GTK_UNIT_INCH);
}

static void
preview_got_page_size (GtkPrintOperationPreview *gtk_preview,
		       GtkPrintContext          *context,
		       GtkPageSetup             *page_setup,
		       GeditPrintPreview        *preview)
{
	update_paper_size (preview, page_setup);
}

/* HACK: we need a dummy surface to paginate... can we use something simpler? */

static cairo_status_t
dummy_write_func (G_GNUC_UNUSED gpointer      closure,
		  G_GNUC_UNUSED const guchar *data,
		  G_GNUC_UNUSED guint         length)
{
    return CAIRO_STATUS_SUCCESS;
}

#define PRINTER_DPI (72.)

static cairo_surface_t *
create_preview_surface_platform (GtkPaperSize *paper_size,
				 double       *dpi_x,
				 double       *dpi_y)
{
    double width, height;
    cairo_surface_t *sf;

    width = gtk_paper_size_get_width (paper_size, GTK_UNIT_POINTS);
    height = gtk_paper_size_get_height (paper_size, GTK_UNIT_POINTS);

    *dpi_x = *dpi_y = PRINTER_DPI;

    sf = cairo_pdf_surface_create_for_stream (dummy_write_func, NULL,
					      width, height);
    return sf;
}

static cairo_surface_t *
create_preview_surface (GeditPrintPreview *preview,
			double	  *dpi_x,
			double	  *dpi_y)
{
    GtkPageSetup *page_setup;
    GtkPaperSize *paper_size;

    page_setup = gtk_print_context_get_page_setup (preview->priv->context);
    /* gtk_page_setup_get_paper_size swaps width and height for landscape */
    paper_size = gtk_page_setup_get_paper_size (page_setup);

    return create_preview_surface_platform (paper_size, dpi_x, dpi_y);
}

GtkWidget *
gedit_print_preview_new (GtkPrintOperation        *op,
			 GtkPrintOperationPreview *gtk_preview,
			 GtkPrintContext          *context)
{
	GeditPrintPreview *preview;
	GtkPageSetup *page_setup;
	cairo_surface_t *surface;
	cairo_t *cr;
	double dpi_x, dpi_y;

	g_return_val_if_fail (GTK_IS_PRINT_OPERATION (op), NULL);
	g_return_val_if_fail (GTK_IS_PRINT_OPERATION_PREVIEW (gtk_preview), NULL);

	preview = g_object_new (GEDIT_TYPE_PRINT_PREVIEW, NULL);

	preview->priv->operation = g_object_ref (op);
	preview->priv->gtk_preview = g_object_ref (gtk_preview);
	preview->priv->context = g_object_ref (context);

	/* FIXME: is this legal?? */
	gtk_print_operation_set_unit (op, GTK_UNIT_POINTS);

	g_signal_connect (gtk_preview, "ready",
			  G_CALLBACK (preview_ready), preview);
	g_signal_connect (gtk_preview, "got-page-size",
			  G_CALLBACK (preview_got_page_size), preview);

	page_setup = gtk_print_context_get_page_setup (preview->priv->context);
	update_paper_size (preview, page_setup);

	/* FIXME: we need a cr to paginate... but we can't get the drawing
	 * area surface because it's not there yet... for now I create
	 * a dummy pdf surface */

	surface = create_preview_surface (preview, &dpi_x, &dpi_y);
	cr = cairo_create (surface);
	gtk_print_context_set_cairo_context (context, cr, dpi_x, dpi_y);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	return GTK_WIDGET (preview);
}

/* ex:set ts=8 noet: */
