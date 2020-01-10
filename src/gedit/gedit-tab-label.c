/*
 * gedit-tab-label.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Paolo Borelli
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

#include "gedit-tab-label.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gedit-small-button.h"

/* Signals */
enum
{
	CLOSE_CLICKED,
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_TAB
};

struct _GeditTabLabelPrivate
{
	GeditTab *tab;

	GtkWidget *close_button;
	GtkWidget *spinner;
	GtkWidget *icon;
	GtkWidget *label;

	gboolean close_button_sensitive;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GeditTabLabel, gedit_tab_label, GTK_TYPE_BOX)

static void
gedit_tab_label_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_tab_label_parent_class)->finalize (object);
}

static void
gedit_tab_label_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	switch (prop_id)
	{
		case PROP_TAB:
			tab_label->priv->tab = GEDIT_TAB (g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_tab_label_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	switch (prop_id)
	{
		case PROP_TAB:
			g_value_set_object (value, tab_label->priv->tab);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
close_button_clicked_cb (GtkWidget     *widget,
			 GeditTabLabel *tab_label)
{
	g_signal_emit (tab_label, signals[CLOSE_CLICKED], 0, NULL);
}

static void
sync_tip (GeditTab *tab,
	  GeditTabLabel *tab_label)
{
	gchar *str;

	str = _gedit_tab_get_tooltip (tab);
	g_return_if_fail (str != NULL);

	gtk_widget_set_tooltip_markup (GTK_WIDGET (tab_label), str);
	g_free (str);
}

static void
sync_name (GeditTab      *tab,
	   GParamSpec    *pspec,
	   GeditTabLabel *tab_label)
{
	gchar *str;

	g_return_if_fail (tab == tab_label->priv->tab);

	str = _gedit_tab_get_name (tab);
	g_return_if_fail (str != NULL);

	gtk_label_set_text (GTK_LABEL (tab_label->priv->label), str);
	g_free (str);

	sync_tip (tab, tab_label);
}

static void
sync_state (GeditTab      *tab,
	    GParamSpec    *pspec,
	    GeditTabLabel *tab_label)
{
	GeditTabState  state;

	g_return_if_fail (tab == tab_label->priv->tab);

	state = gedit_tab_get_state (tab);

	gtk_widget_set_sensitive (tab_label->priv->close_button,
				  tab_label->priv->close_button_sensitive &&
				  (state != GEDIT_TAB_STATE_CLOSING) &&
				  (state != GEDIT_TAB_STATE_SAVING)  &&
				  (state != GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW) &&
				  (state != GEDIT_TAB_STATE_SAVING_ERROR));

	if ((state == GEDIT_TAB_STATE_LOADING)   ||
	    (state == GEDIT_TAB_STATE_SAVING)    ||
	    (state == GEDIT_TAB_STATE_REVERTING))
	{
		gtk_widget_hide (tab_label->priv->icon);

		gtk_widget_show (tab_label->priv->spinner);
		gtk_spinner_start (GTK_SPINNER (tab_label->priv->spinner));
	}
	else
	{
		GdkPixbuf *pixbuf;

		pixbuf = _gedit_tab_get_icon (tab);

		if (pixbuf != NULL)
		{
			gtk_image_set_from_pixbuf (GTK_IMAGE (tab_label->priv->icon),
			                           pixbuf);

			g_clear_object (&pixbuf);

			gtk_widget_show (tab_label->priv->icon);
		}
		else
		{
			gtk_widget_hide (tab_label->priv->icon);
		}

		gtk_spinner_stop (GTK_SPINNER (tab_label->priv->spinner));
		gtk_widget_hide (tab_label->priv->spinner);
	}

	/* sync tip since encoding is known only after load/save end */
	sync_tip (tab, tab_label);
}

static void
gedit_tab_label_constructed (GObject *object)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	if (!tab_label->priv->tab)
	{
		g_critical ("The tab label was not properly constructed");
		return;
	}

	sync_name (tab_label->priv->tab, NULL, tab_label);
	sync_state (tab_label->priv->tab, NULL, tab_label);

	g_signal_connect_object (tab_label->priv->tab,
				 "notify::name",
				 G_CALLBACK (sync_name),
				 tab_label,
				 0);

	g_signal_connect_object (tab_label->priv->tab,
				 "notify::state",
				 G_CALLBACK (sync_state),
				 tab_label,
				 0);

	G_OBJECT_CLASS (gedit_tab_label_parent_class)->constructed (object);
}

static void
gedit_tab_label_class_init (GeditTabLabelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = gedit_tab_label_finalize;
	object_class->set_property = gedit_tab_label_set_property;
	object_class->get_property = gedit_tab_label_get_property;
	object_class->constructed = gedit_tab_label_constructed;

	signals[CLOSE_CLICKED] =
		g_signal_new ("close-clicked",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditTabLabelClass, close_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	g_object_class_install_property (object_class,
					 PROP_TAB,
					 g_param_spec_object ("tab",
							      "Tab",
							      "The GeditTab",
							      GEDIT_TYPE_TAB,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-tab-label.ui");
	gtk_widget_class_bind_template_child_private (widget_class, GeditTabLabel, spinner);
	gtk_widget_class_bind_template_child_private (widget_class, GeditTabLabel, close_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditTabLabel, icon);
	gtk_widget_class_bind_template_child_private (widget_class, GeditTabLabel, label);
}

static void
gedit_tab_label_init (GeditTabLabel *tab_label)
{
	tab_label->priv = gedit_tab_label_get_instance_private (tab_label);

	tab_label->priv->close_button_sensitive = TRUE;

	gtk_widget_init_template (GTK_WIDGET (tab_label));

	g_signal_connect (tab_label->priv->close_button,
	                  "clicked",
	                  G_CALLBACK (close_button_clicked_cb),
	                  tab_label);
}

void
gedit_tab_label_set_close_button_sensitive (GeditTabLabel *tab_label,
					    gboolean       sensitive)
{
	GeditTabState state;

	g_return_if_fail (GEDIT_IS_TAB_LABEL (tab_label));

	sensitive = (sensitive != FALSE);

	if (sensitive == tab_label->priv->close_button_sensitive)
		return;

	tab_label->priv->close_button_sensitive = sensitive;

	state = gedit_tab_get_state (tab_label->priv->tab);

	gtk_widget_set_sensitive (tab_label->priv->close_button,
				  tab_label->priv->close_button_sensitive &&
				  (state != GEDIT_TAB_STATE_CLOSING) &&
				  (state != GEDIT_TAB_STATE_SAVING)  &&
				  (state != GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW) &&
				  (state != GEDIT_TAB_STATE_PRINTING) &&
				  (state != GEDIT_TAB_STATE_PRINT_PREVIEWING) &&
				  (state != GEDIT_TAB_STATE_SAVING_ERROR));
}

GeditTab *
gedit_tab_label_get_tab (GeditTabLabel *tab_label)
{
	g_return_val_if_fail (GEDIT_IS_TAB_LABEL (tab_label), NULL);

	return tab_label->priv->tab;
}

GtkWidget *
gedit_tab_label_new (GeditTab *tab)
{
	GeditTabLabel *tab_label;

	tab_label = g_object_new (GEDIT_TYPE_TAB_LABEL,
				  "tab", tab,
				  NULL);

	return GTK_WIDGET (tab_label);
}

/* ex:set ts=8 noet: */
