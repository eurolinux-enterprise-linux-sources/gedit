/*
 * gedit-statusbar.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Borelli
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gedit-statusbar.h"

#define GEDIT_STATUSBAR_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object),\
					    GEDIT_TYPE_STATUSBAR,\
					    GeditStatusbarPrivate))

struct _GeditStatusbarPrivate
{
	GtkWidget     *overwrite_mode_label;
	GtkWidget     *cursor_position_label;

	GtkWidget     *state_frame;
	GtkWidget     *load_image;
	GtkWidget     *save_image;
	GtkWidget     *print_image;

	GtkWidget     *error_frame;
	GtkWidget     *error_event_box;

	/* tmp flash timeout data */
	guint          flash_timeout;
	guint          flash_context_id;
	guint          flash_message_id;
};

G_DEFINE_TYPE(GeditStatusbar, gedit_statusbar, GTK_TYPE_STATUSBAR)

static gchar *
get_overwrite_mode_string (gboolean overwrite)
{
	/* Use spaces to leave padding proportional to the font size */
	return g_strdup_printf ("  %s  ", overwrite ? _("OVR") : _("INS"));
}

static gint
get_overwrite_mode_length (void)
{
	return 4 + MAX (g_utf8_strlen (_("OVR"), -1), g_utf8_strlen (_("INS"), -1));
}

static void
gedit_statusbar_dispose (GObject *object)
{
	GeditStatusbar *statusbar = GEDIT_STATUSBAR (object);

	if (statusbar->priv->flash_timeout > 0)
	{
		g_source_remove (statusbar->priv->flash_timeout);
		statusbar->priv->flash_timeout = 0;
	}

	G_OBJECT_CLASS (gedit_statusbar_parent_class)->dispose (object);
}

static void
gedit_statusbar_class_init (GeditStatusbarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_statusbar_dispose;

	g_type_class_add_private (object_class, sizeof (GeditStatusbarPrivate));
}

#define CURSOR_POSITION_LABEL_WIDTH_CHARS 18

static void
gedit_statusbar_init (GeditStatusbar *statusbar)
{
	GtkWidget *hbox;
	GtkWidget *error_image;

	statusbar->priv = GEDIT_STATUSBAR_GET_PRIVATE (statusbar);

	statusbar->priv->overwrite_mode_label = gtk_label_new (NULL);
	gtk_label_set_width_chars (GTK_LABEL (statusbar->priv->overwrite_mode_label),
				   get_overwrite_mode_length ());
	gtk_widget_show (statusbar->priv->overwrite_mode_label);
	gtk_box_pack_end (GTK_BOX (statusbar),
			  statusbar->priv->overwrite_mode_label,
			  FALSE, TRUE, 0);

	statusbar->priv->cursor_position_label = gtk_label_new (NULL);
	gtk_label_set_width_chars (GTK_LABEL (statusbar->priv->cursor_position_label),
				  CURSOR_POSITION_LABEL_WIDTH_CHARS);
	gtk_widget_show (statusbar->priv->cursor_position_label);
	gtk_box_pack_end (GTK_BOX (statusbar),
			  statusbar->priv->cursor_position_label,
			  FALSE, TRUE, 0);

	statusbar->priv->state_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (statusbar->priv->state_frame),
				   GTK_SHADOW_IN);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->state_frame), hbox);

	statusbar->priv->load_image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
	statusbar->priv->save_image = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU);
	statusbar->priv->print_image = gtk_image_new_from_stock (GTK_STOCK_PRINT, GTK_ICON_SIZE_MENU);

	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (hbox),
			    statusbar->priv->load_image,
			    FALSE, TRUE, 4);
	gtk_box_pack_start (GTK_BOX (hbox),
			    statusbar->priv->save_image,
			    FALSE, TRUE, 4);
	gtk_box_pack_start (GTK_BOX (hbox),
			    statusbar->priv->print_image,
			    FALSE, TRUE, 4);

	gtk_box_pack_start (GTK_BOX (statusbar),
			    statusbar->priv->state_frame,
			    FALSE, TRUE, 0);

	statusbar->priv->error_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (statusbar->priv->error_frame),
				   GTK_SHADOW_IN);

	error_image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR,
					        GTK_ICON_SIZE_MENU);
	gtk_misc_set_padding (GTK_MISC (error_image), 4, 0);
	gtk_widget_show (error_image);

	statusbar->priv->error_event_box = gtk_event_box_new ();
	gtk_event_box_set_visible_window  (GTK_EVENT_BOX (statusbar->priv->error_event_box),
					   FALSE);
	gtk_widget_show (statusbar->priv->error_event_box);

	gtk_container_add (GTK_CONTAINER (statusbar->priv->error_frame),
			   statusbar->priv->error_event_box);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->error_event_box),
			   error_image);

	gtk_box_pack_start (GTK_BOX (statusbar),
			    statusbar->priv->error_frame,
			    FALSE, TRUE, 0);

	gtk_box_reorder_child (GTK_BOX (statusbar),
			       statusbar->priv->error_frame,
			       0);
}

/**
 * gedit_statusbar_new:
 *
 * Creates a new #GeditStatusbar.
 *
 * Return value: the new #GeditStatusbar object
 **/
GtkWidget *
gedit_statusbar_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_STATUSBAR, NULL));
}

/**
 * gedit_statusbar_set_overwrite:
 * @statusbar: a #GeditStatusbar
 * @overwrite: if the overwrite mode is set
 *
 * Sets the overwrite mode on the statusbar.
 **/
void
gedit_statusbar_set_overwrite (GeditStatusbar *statusbar,
                               gboolean        overwrite)
{
	gchar *msg;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));

	msg = get_overwrite_mode_string (overwrite);

	gtk_label_set_text (GTK_LABEL (statusbar->priv->overwrite_mode_label), msg);

	g_free (msg);
}

void
gedit_statusbar_clear_overwrite (GeditStatusbar *statusbar)
{
	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));

	gtk_label_set_text (GTK_LABEL (statusbar->priv->overwrite_mode_label), NULL);
}

/**
 * gedit_statusbar_cursor_position:
 * @statusbar: an #GeditStatusbar
 * @line: line position
 * @col: column position
 *
 * Sets the cursor position on the statusbar.
 **/
void
gedit_statusbar_set_cursor_position (GeditStatusbar *statusbar,
				     gint            line,
				     gint            col)
{
	gchar *msg = NULL;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));

	if ((line >= 0) || (col >= 0))
	{
		/* Translators: "Ln" is an abbreviation for "Line", Col is an abbreviation for "Column". Please,
		use abbreviations if possible to avoid space problems. */
		msg = g_strdup_printf (_("  Ln %d, Col %d"), line, col);
	}

	gtk_label_set_text (GTK_LABEL (statusbar->priv->cursor_position_label), msg);

	g_free (msg);
}

static gboolean
remove_message_timeout (GeditStatusbar *statusbar)
{
	gtk_statusbar_remove (GTK_STATUSBAR (statusbar),
			      statusbar->priv->flash_context_id,
			      statusbar->priv->flash_message_id);

	/* remove the timeout */
	statusbar->priv->flash_timeout = 0;
  	return FALSE;
}

/* FIXME this is an issue for introspection */
/**
 * gedit_statusbar_flash_message:
 * @statusbar: a #GeditStatusbar
 * @context_id: message context_id
 * @format: message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar.
 */
void
gedit_statusbar_flash_message (GeditStatusbar *statusbar,
			       guint           context_id,
			       const gchar    *format, ...)
{
	const guint32 flash_length = 3000; /* three seconds */
	va_list args;
	gchar *msg;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));
	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	/* remove a currently ongoing flash message */
	if (statusbar->priv->flash_timeout > 0)
	{
		g_source_remove (statusbar->priv->flash_timeout);
		statusbar->priv->flash_timeout = 0;

		gtk_statusbar_remove (GTK_STATUSBAR (statusbar),
				      statusbar->priv->flash_context_id,
				      statusbar->priv->flash_message_id);
	}

	statusbar->priv->flash_context_id = context_id;
	statusbar->priv->flash_message_id = gtk_statusbar_push (GTK_STATUSBAR (statusbar),
								context_id,
								msg);

	statusbar->priv->flash_timeout = g_timeout_add (flash_length,
							(GSourceFunc) remove_message_timeout,
							statusbar);

	g_free (msg);
}

void
gedit_statusbar_set_window_state (GeditStatusbar   *statusbar,
				  GeditWindowState  state,
				  gint              num_of_errors)
{
	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));

	gtk_widget_hide (statusbar->priv->state_frame);
	gtk_widget_hide (statusbar->priv->save_image);
	gtk_widget_hide (statusbar->priv->load_image);
	gtk_widget_hide (statusbar->priv->print_image);

	if (state & GEDIT_WINDOW_STATE_SAVING)
	{
		gtk_widget_show (statusbar->priv->state_frame);
		gtk_widget_show (statusbar->priv->save_image);
	}
	if (state & GEDIT_WINDOW_STATE_LOADING)
	{
		gtk_widget_show (statusbar->priv->state_frame);
		gtk_widget_show (statusbar->priv->load_image);
	}
	if (state & GEDIT_WINDOW_STATE_PRINTING)
	{
		gtk_widget_show (statusbar->priv->state_frame);
		gtk_widget_show (statusbar->priv->print_image);
	}
	if (state & GEDIT_WINDOW_STATE_ERROR)
	{
	 	gchar *tip;

 		tip = g_strdup_printf (ngettext("There is a tab with errors",
						"There are %d tabs with errors",
						num_of_errors),
			       		num_of_errors);

		gtk_widget_set_tooltip_text (statusbar->priv->error_event_box,
					     tip);
		g_free (tip);

		gtk_widget_show (statusbar->priv->error_frame);
	}
	else
	{
		gtk_widget_hide (statusbar->priv->error_frame);
	}
}

/* ex:set ts=8 noet: */
