/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-preferences-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2005 Paolo Maggi
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

#include "gedit-preferences-dialog.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtksourceview/gtksource.h>
#include <libpeas-gtk/peas-gtk.h>

#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"
#include "gedit-utils.h"
#include "gedit-file-chooser-dialog.h"

/*
 * gedit-preferences dialog is a singleton since we don't
 * want two dialogs showing an inconsistent state of the
 * preferences.
 * When gedit_show_preferences_dialog is called and there
 * is already a prefs dialog dialog open, it is reparented
 * and shown.
 */

static GtkWidget *preferences_dialog = NULL;

#define GEDIT_SCHEME_ROW_ID_KEY "gedit-scheme-row-id"

enum
{
	ID_COLUMN = 0,
	NAME_COLUMN,
	DESC_COLUMN,
	NUM_COLUMNS
};

enum
{
	CLOSE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _GeditPreferencesDialogPrivate
{
	GSettings	*editor;
	GSettings	*uisettings; /* unfortunately our settings are split for historical reasons */

	GtkWidget	*notebook;

	/* Font */
	GtkWidget	*default_font_checkbutton;
	GtkWidget	*font_button;
	GtkWidget	*font_grid;

	/* Style Scheme */
	GtkWidget	*schemes_list;
	GtkWidget	*install_scheme_button;
	GtkWidget	*uninstall_scheme_button;
	GtkWidget	*schemes_scrolled_window;
	GtkWidget	*schemes_toolbar;

	GeditFileChooserDialog *
			 install_scheme_file_schooser;

	/* Tabs */
	GtkWidget	*tabs_width_spinbutton;
	GtkWidget	*insert_spaces_checkbutton;

	/* Auto indentation */
	GtkWidget	*auto_indent_checkbutton;

	/* Text Wrapping */
	GtkWidget	*wrap_text_checkbutton;
	GtkWidget	*split_checkbutton;

	/* File Saving */
	GtkWidget	*backup_copy_checkbutton;
	GtkWidget	*auto_save_checkbutton;
	GtkWidget	*auto_save_spinbutton;

	GtkWidget	*display_line_numbers_checkbutton;
	GtkWidget	*display_statusbar_checkbutton;

	/* Right margin */
	GtkWidget	*right_margin_checkbutton;
	GtkWidget       *right_margin_position_grid;
	GtkWidget	*right_margin_position_spinbutton;

	/* Highlighting */
	GtkWidget	*highlight_current_line_checkbutton;
	GtkWidget	*bracket_matching_checkbutton;

	/* Plugin manager */
	GtkWidget	*plugin_manager;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditPreferencesDialog, gedit_preferences_dialog, GTK_TYPE_WINDOW)

static void
gedit_preferences_dialog_dispose (GObject *object)
{
	GeditPreferencesDialog *dlg = GEDIT_PREFERENCES_DIALOG (object);

	g_clear_object (&dlg->priv->editor);
	g_clear_object (&dlg->priv->uisettings);

	G_OBJECT_CLASS (gedit_preferences_dialog_parent_class)->dispose (object);
}

static void
gedit_preferences_dialog_close (GeditPreferencesDialog *dialog)
{
	gtk_window_close (GTK_WINDOW (dialog));
}

static void
gedit_preferences_dialog_class_init (GeditPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkBindingSet *binding_set;

	/* Otherwise libpeas-gtk might not be linked */
	g_type_ensure (PEAS_GTK_TYPE_PLUGIN_MANAGER);

	object_class->dispose = gedit_preferences_dialog_dispose;
	klass->close = gedit_preferences_dialog_close;

	signals[CLOSE] = g_signal_new ("close",
	                               G_OBJECT_CLASS_TYPE (klass),
	                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                               G_STRUCT_OFFSET (GeditPreferencesDialogClass, close),
	                               NULL, NULL, NULL,
	                               G_TYPE_NONE, 0);

	binding_set = gtk_binding_set_by_class (klass);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "close", 0);

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-preferences-dialog.ui");
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, notebook);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, display_line_numbers_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, display_statusbar_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, right_margin_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, right_margin_position_grid);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, right_margin_position_spinbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, highlight_current_line_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, bracket_matching_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, wrap_text_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, split_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, tabs_width_spinbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, insert_spaces_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, auto_indent_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, backup_copy_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, auto_save_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, auto_save_spinbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, default_font_checkbutton);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, font_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, font_grid);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, schemes_list);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, schemes_scrolled_window);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, install_scheme_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, uninstall_scheme_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, schemes_toolbar);
	gtk_widget_class_bind_template_child_private (widget_class, GeditPreferencesDialog, plugin_manager);
}

static void
setup_editor_page (GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	/* Connect signal */
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_TABS_SIZE,
			 dlg->priv->tabs_width_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_INSERT_SPACES,
			 dlg->priv->insert_spaces_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_AUTO_INDENT,
			 dlg->priv->auto_indent_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_CREATE_BACKUP_COPY,
			 dlg->priv->backup_copy_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_BRACKET_MATCHING,
			 dlg->priv->bracket_matching_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_AUTO_SAVE_INTERVAL,
			 dlg->priv->auto_save_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_AUTO_SAVE,
			 dlg->priv->auto_save_spinbutton,
			 "sensitive",
			 G_SETTINGS_BIND_GET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_AUTO_SAVE,
			 dlg->priv->auto_save_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
}

static void
wrap_mode_checkbutton_toggled (GtkToggleButton        *button,
			       GeditPreferencesDialog *dlg)
{
	GtkWrapMode mode;

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton)))
	{
		mode = GTK_WRAP_NONE;

		gtk_widget_set_sensitive (dlg->priv->split_checkbutton,
					  FALSE);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->split_checkbutton,
					  TRUE);

		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), FALSE);


		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton)))
		{
			g_settings_set_enum (dlg->priv->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_WORD);

			mode = GTK_WRAP_WORD;
		}
		else
		{
			g_settings_set_enum (dlg->priv->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_CHAR);

			mode = GTK_WRAP_CHAR;
		}
	}

	g_settings_set_enum (dlg->priv->editor,
			     GEDIT_SETTINGS_WRAP_MODE,
			     mode);
}

static void
setup_view_page (GeditPreferencesDialog *dlg)
{
	GtkWrapMode wrap_mode;
	GtkWrapMode last_split_mode;
	gboolean display_right_margin;
	guint right_margin_position;

	gedit_debug (DEBUG_PREFS);

	/* Get values */
	display_right_margin = g_settings_get_boolean (dlg->priv->editor,
						       GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN);
	g_settings_get (dlg->priv->editor, GEDIT_SETTINGS_RIGHT_MARGIN_POSITION,
			"u", &right_margin_position);

	/* Set initial state */
	wrap_mode = g_settings_get_enum (dlg->priv->editor,
					 GEDIT_SETTINGS_WRAP_MODE);

	switch (wrap_mode)
	{
		case GTK_WRAP_WORD:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), TRUE);

			g_settings_set_enum (dlg->priv->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_WORD);
			break;
		case GTK_WRAP_CHAR:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), FALSE);

			g_settings_set_enum (dlg->priv->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_CHAR);
			break;
		default:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton), FALSE);

			last_split_mode = g_settings_get_enum (dlg->priv->editor,
			                                       GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton),
			                              last_split_mode == GTK_WRAP_WORD);

			gtk_toggle_button_set_inconsistent (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), TRUE);
	}

	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (dlg->priv->right_margin_checkbutton),
		display_right_margin);

	/* Set widgets sensitivity */
	gtk_widget_set_sensitive (dlg->priv->split_checkbutton,
				  (wrap_mode != GTK_WRAP_NONE));
	/* Connect signals */
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS,
			 dlg->priv->display_line_numbers_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE,
			 dlg->priv->highlight_current_line_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->uisettings,
	                 GEDIT_SETTINGS_STATUSBAR_VISIBLE,
	                 dlg->priv->display_statusbar_checkbutton,
	                 "active",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
	                 GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN,
	                 dlg->priv->right_margin_checkbutton,
	                 "active",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
	                 GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN,
	                 dlg->priv->right_margin_position_grid,
	                 "sensitive",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_RIGHT_MARGIN_POSITION,
			 dlg->priv->right_margin_position_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_AUTO_SAVE_INTERVAL,
			 dlg->priv->auto_save_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_signal_connect (dlg->priv->wrap_text_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->split_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dlg);
}

static void
setup_font_colors_page_font_section (GeditPreferencesDialog *dlg)
{
	GObject *settings;
	gchar *system_font = NULL;
	gchar *label;

	gedit_debug (DEBUG_PREFS);

	gtk_widget_set_tooltip_text (dlg->priv->font_button,
			 _("Click on this button to select the font to be used by the editor"));

	gedit_utils_set_atk_relation (dlg->priv->font_button,
				      dlg->priv->default_font_checkbutton,
				      ATK_RELATION_CONTROLLED_BY);
	gedit_utils_set_atk_relation (dlg->priv->default_font_checkbutton,
				      dlg->priv->font_button,
				      ATK_RELATION_CONTROLLER_FOR);

	/* Get values */
	settings = _gedit_app_get_settings (GEDIT_APP (g_application_get_default ()));
	system_font = gedit_settings_get_system_font (GEDIT_SETTINGS (settings));

	label = g_strdup_printf(_("_Use the system fixed width font (%s)"),
				system_font);
	gtk_button_set_label (GTK_BUTTON (dlg->priv->default_font_checkbutton),
			      label);
	g_free (system_font);
	g_free (label);

	/* Bind settings */
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_USE_DEFAULT_FONT,
			 dlg->priv->default_font_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_USE_DEFAULT_FONT,
			 dlg->priv->font_grid,
			 "sensitive",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET | G_SETTINGS_BIND_INVERT_BOOLEAN);
	g_settings_bind (dlg->priv->editor,
			 GEDIT_SETTINGS_EDITOR_FONT,
			 dlg->priv->font_button,
			 "font-name",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
}

static gboolean
is_gedit_user_style_scheme (const gchar *scheme_id)
{
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *scheme;
	gboolean res = FALSE;

	manager = gtk_source_style_scheme_manager_get_default ();
	scheme = gtk_source_style_scheme_manager_get_scheme (manager, scheme_id);
	if (scheme != NULL)
	{
		const gchar *filename;

		filename = gtk_source_style_scheme_get_filename (scheme);
		if (filename != NULL)
		{
			res = g_str_has_prefix (filename, gedit_dirs_get_user_styles_dir ());
		}
	}

	return res;
}

static void
set_buttons_sensisitivity_according_to_scheme (GeditPreferencesDialog *dlg,
					       const gchar            *scheme_id)
{
	gboolean editable;

	editable = ((scheme_id != NULL) && is_gedit_user_style_scheme (scheme_id));

	gtk_widget_set_sensitive (dlg->priv->uninstall_scheme_button,
				  editable);
}

static void
style_scheme_changed (GtkListBox             *list_box,
                      GtkListBoxRow          *row,
                      GeditPreferencesDialog *dlg)
{
	if (row != NULL)
	{
		const gchar *id;

		id = g_object_get_data (G_OBJECT (row), GEDIT_SCHEME_ROW_ID_KEY);
		g_return_if_fail (id != NULL);

		g_settings_set_string (dlg->priv->editor, GEDIT_SETTINGS_SCHEME, id);
		set_buttons_sensisitivity_according_to_scheme (dlg, id);
	}
}

static const gchar *
ensure_color_scheme_id (GeditPreferencesDialog *dlg,
			const gchar            *id)
{
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *scheme = NULL;

	manager = gtk_source_style_scheme_manager_get_default ();
	if (id == NULL)
	{
		gchar *pref_id;

		pref_id = g_settings_get_string (dlg->priv->editor,
						 GEDIT_SETTINGS_SCHEME);

		scheme = gtk_source_style_scheme_manager_get_scheme (manager,
								     pref_id);
		g_free (pref_id);
	}
	else
	{
		scheme = gtk_source_style_scheme_manager_get_scheme (manager,
								     id);
	}

	if (scheme == NULL)
	{
		/* Fall-back to classic style scheme */
		scheme = gtk_source_style_scheme_manager_get_scheme (manager,
								     "classic");
	}

	if (scheme == NULL)
	{
		/* Cannot determine default style scheme -> broken GtkSourceView installation */
		return NULL;
	}

	return 	gtk_source_style_scheme_get_id (scheme);
}

static const gchar *
populate_color_scheme_list (GeditPreferencesDialog *dlg, const gchar *def_id)
{
	GtkSourceStyleSchemeManager *manager;
	const gchar * const *ids;
	gint i;

	gtk_container_foreach (GTK_CONTAINER (dlg->priv->schemes_list),
	                       (GtkCallback) gtk_widget_destroy,
	                       NULL);

	def_id = ensure_color_scheme_id (dlg, def_id);
	if (def_id == NULL)
	{
		g_warning ("Cannot build the list of available color schemes.\n"
		           "Please check your GtkSourceView installation.");
		return NULL;
	}

	manager = gtk_source_style_scheme_manager_get_default ();
	ids = gtk_source_style_scheme_manager_get_scheme_ids (manager);
	for (i = 0; ids[i] != NULL; i++)
	{
		GtkSourceStyleScheme *scheme;
		const gchar *name;
		const gchar *description;
		gchar *text;
		GtkWidget *label;
		GtkWidget *row;

		scheme = gtk_source_style_scheme_manager_get_scheme (manager, ids[i]);

		name = gtk_source_style_scheme_get_name (scheme);
		description = gtk_source_style_scheme_get_description (scheme);

		if (description != NULL)
		{
			text = g_markup_printf_escaped ("<b>%s</b> - %s",
							name,
							description);
		}
		else
		{
			text = g_markup_printf_escaped ("<b>%s</b>", name);
		}

		label = gtk_label_new (text);
		g_free (text);

		gtk_widget_set_halign (label, GTK_ALIGN_START);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

		row = gtk_list_box_row_new ();
		gtk_container_add (GTK_CONTAINER (row), label);
		gtk_widget_show_all (row);

		g_object_set_data_full (G_OBJECT (row),
		                        GEDIT_SCHEME_ROW_ID_KEY,
		                        g_strdup (ids[i]),
		                        (GDestroyNotify) g_free);

		gtk_list_box_insert (GTK_LIST_BOX (dlg->priv->schemes_list), row, i);

		if (strcmp (ids[i], def_id) == 0)
		{
			gtk_list_box_select_row (GTK_LIST_BOX (dlg->priv->schemes_list),
						 GTK_LIST_BOX_ROW (row));
		}
	}

	return def_id;
}

/*
 * file_copy:
 * @name: a pointer to a %NULL-terminated string, that names
 * the file to be copied, in the GLib file name encoding
 * @dest_name: a pointer to a %NULL-terminated string, that is the
 * name for the destination file, in the GLib file name encoding
 * @error: return location for a #GError, or %NULL
 *
 * Copies file @name to @dest_name.
 *
 * If the call was successful, it returns %TRUE. If the call was not
 * successful, it returns %FALSE and sets @error. The error domain
 * is #G_FILE_ERROR. Possible error
 * codes are those in the #GFileError enumeration.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 */
static gboolean
file_copy (const gchar  *name,
	   const gchar  *dest_name,
	   GError      **error)
{
	gchar *contents;
	gsize length;
	gchar *dest_dir;

	/* FIXME - Paolo (Aug. 13, 2007):
	 * Since the style scheme files are relatively small, we can implement
	 * file copy getting all the content of the source file in a buffer and
	 * then write the content to the destination file. In this way we
	 * can use the g_file_get_contents and g_file_set_contents and avoid to
	 * write custom code to copy the file (with sane error management).
	 * If needed we can improve this code later. */

	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (dest_name != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* Note: we allow to copy a file to itself since this is not a problem
	 * in our use case */

	/* Ensure the destination directory exists */
	dest_dir = g_path_get_dirname (dest_name);

	errno = 0;
	if (g_mkdir_with_parents (dest_dir, 0755) != 0)
	{
		gint save_errno = errno;
		gchar *display_filename = g_filename_display_name (dest_dir);

		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (save_errno),
			     _("Directory '%s' could not be created: g_mkdir_with_parents() failed: %s"),
			     display_filename,
			     g_strerror (save_errno));

		g_free (dest_dir);
		g_free (display_filename);

		return FALSE;
	}

	g_free (dest_dir);

	if (!g_file_get_contents (name, &contents, &length, error))
		return FALSE;

	if (!g_file_set_contents (dest_name, contents, length, error))
	{
		g_free (contents);
		return FALSE;
	}

	g_free (contents);

	return TRUE;
}

/*
 * install_style_scheme:
 * @manager: a #GtkSourceStyleSchemeManager
 * @fname: the file name of the style scheme to be installed
 *
 * Install a new user scheme.
 * This function copies @fname in #GEDIT_STYLES_DIR and ask the style manager to
 * recompute the list of available style schemes. It then checks if a style
 * scheme with the right file name exists.
 *
 * If the call was succesful, it returns the id of the installed scheme
 * otherwise %NULL.
 *
 * Return value: the id of the installed scheme, %NULL otherwise.
 */
static const gchar *
install_style_scheme (const gchar *fname)
{
	GtkSourceStyleSchemeManager *manager;
	gchar *new_file_name = NULL;
	gchar *dirname;
	const gchar *styles_dir;
	GError *error = NULL;
	gboolean copied = FALSE;
	const gchar* const *ids;

	g_return_val_if_fail (fname != NULL, NULL);

	manager = gtk_source_style_scheme_manager_get_default ();

	dirname = g_path_get_dirname (fname);
	styles_dir = gedit_dirs_get_user_styles_dir ();

	if (strcmp (dirname, styles_dir) != 0)
	{
		gchar *basename;

		basename = g_path_get_basename (fname);
		new_file_name = g_build_filename (styles_dir, basename, NULL);
		g_free (basename);

		/* Copy the style scheme file into GEDIT_STYLES_DIR */
		if (!file_copy (fname, new_file_name, &error))
		{
			g_free (new_file_name);
			g_free (dirname);

			g_message ("Cannot install style scheme:\n%s",
				   error->message);

			g_error_free (error);

			return NULL;
		}

		copied = TRUE;
	}
	else
	{
		new_file_name = g_strdup (fname);
	}

	g_free (dirname);

	/* Reload the available style schemes */
	gtk_source_style_scheme_manager_force_rescan (manager);

	/* Check the new style scheme has been actually installed */
	ids = gtk_source_style_scheme_manager_get_scheme_ids (manager);

	while (*ids != NULL)
	{
		GtkSourceStyleScheme *scheme;
		const gchar *filename;

		scheme = gtk_source_style_scheme_manager_get_scheme (manager, *ids);

		filename = gtk_source_style_scheme_get_filename (scheme);

		if (filename && (strcmp (filename, new_file_name) == 0))
		{
			/* The style scheme has been correctly installed */
			g_free (new_file_name);

			return gtk_source_style_scheme_get_id (scheme);
		}
		++ids;
	}

	/* The style scheme has not been correctly installed */
	if (copied)
		g_unlink (new_file_name);

	g_free (new_file_name);

	return NULL;
}

/**
 * uninstall_style_scheme:
 * @manager: a #GtkSourceStyleSchemeManager
 * @id: the id of the style scheme to be uninstalled
 *
 * Uninstall a user scheme.
 *
 * If the call was succesful, it returns %TRUE
 * otherwise %FALSE.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 */
static gboolean
uninstall_style_scheme (const gchar *id)
{
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *scheme;
	const gchar *filename;

	g_return_val_if_fail (id != NULL, FALSE);

	manager = gtk_source_style_scheme_manager_get_default ();

	scheme = gtk_source_style_scheme_manager_get_scheme (manager, id);
	if (scheme == NULL)
		return FALSE;

	filename = gtk_source_style_scheme_get_filename (scheme);
	if (filename == NULL)
		return FALSE;

	if (g_unlink (filename) == -1)
		return FALSE;

	/* Reload the available style schemes */
	gtk_source_style_scheme_manager_force_rescan (manager);

	return TRUE;
}

static void
add_scheme_chooser_response_cb (GeditFileChooserDialog *chooser,
				gint                    res_id,
				GeditPreferencesDialog *dlg)
{
	GFile *file;
	gchar *filename;
	const gchar *scheme_id;

	if (res_id != GTK_RESPONSE_ACCEPT)
	{
		gedit_file_chooser_dialog_hide (chooser);
		return;
	}

	file = gedit_file_chooser_dialog_get_file (chooser);

	if (file == NULL)
	{
		return;
	}

	filename = g_file_get_path (file);
	g_object_unref (file);

	if (filename == NULL)
	{
		return;
	}

	gedit_file_chooser_dialog_hide (chooser);

	scheme_id = install_style_scheme (filename);
	g_free (filename);

	if (scheme_id == NULL)
	{
		gedit_warning (GTK_WINDOW (dlg),
			       _("The selected color scheme cannot be installed."));

		return;
	}

	g_settings_set_string (dlg->priv->editor, GEDIT_SETTINGS_SCHEME,
			       scheme_id);

	scheme_id = populate_color_scheme_list (dlg, scheme_id);

	set_buttons_sensisitivity_according_to_scheme (dlg, scheme_id);
}

static void
install_scheme_clicked (GtkButton              *button,
			GeditPreferencesDialog *dlg)
{
	GeditFileChooserDialog *chooser;

	if (dlg->priv->install_scheme_file_schooser != NULL)
	{
		gedit_file_chooser_dialog_show (dlg->priv->install_scheme_file_schooser);
		return;
	}

	chooser = gedit_file_chooser_dialog_create (_("Add Scheme"),
						    GTK_WINDOW (dlg),
						    GEDIT_FILE_CHOOSER_OPEN,
						    NULL,
						    _("_Cancel"), GTK_RESPONSE_CANCEL,
						    _("A_dd Scheme"), GTK_RESPONSE_ACCEPT);

	/* Filters */
	gedit_file_chooser_dialog_add_pattern_filter (chooser,
	                                              _("Color Scheme Files"),
	                                              "*.xml");

	gedit_file_chooser_dialog_add_pattern_filter (chooser,
	                                              _("All Files"),
	                                              "*");

	g_signal_connect (chooser,
			  "response",
			  G_CALLBACK (add_scheme_chooser_response_cb),
			  dlg);

	dlg->priv->install_scheme_file_schooser = chooser;

	g_object_add_weak_pointer (G_OBJECT (chooser),
				   (gpointer) &dlg->priv->install_scheme_file_schooser);

	gedit_file_chooser_dialog_show (chooser);
}

static void
uninstall_scheme_clicked (GtkButton              *button,
			  GeditPreferencesDialog *dlg)
{
	GtkListBoxRow *row;

	row = gtk_list_box_get_selected_row (GTK_LIST_BOX (dlg->priv->schemes_list));
	if (row != NULL)
	{
		const gchar *id;

		id = g_object_get_data (G_OBJECT (row), GEDIT_SCHEME_ROW_ID_KEY);
		g_return_if_fail (id != NULL);

		if (!uninstall_style_scheme (id))
		{
			gedit_warning (GTK_WINDOW (dlg),
				       _("Could not remove color scheme \"%s\"."),
				       id);
		}
		else
		{
			gint i;

			/* select the next item or the previous one if
			 * this is the last row
			 */
			i = gtk_list_box_row_get_index (row);

			gtk_widget_destroy (GTK_WIDGET (row));

			row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (dlg->priv->schemes_list), i);
			if (row == NULL && i > 0)
			{
				row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (dlg->priv->schemes_list),
				                                    i - 1);
			}

			/* Select the row and let it trigger the scheme change */
			gtk_list_box_select_row (GTK_LIST_BOX (dlg->priv->schemes_list), row);
		}
	}
}

static void
setup_font_colors_page_style_scheme_section (GeditPreferencesDialog *dlg)
{
	GtkStyleContext *context;
	const gchar *def_id;

	gedit_debug (DEBUG_PREFS);

	def_id = populate_color_scheme_list (dlg, NULL);

	/* junction between the scrolled window and the toolbar */
	context = gtk_widget_get_style_context (dlg->priv->schemes_scrolled_window);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);
	context = gtk_widget_get_style_context (dlg->priv->schemes_toolbar);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);

	/* Connect signals */
	g_signal_connect (dlg->priv->schemes_list,
			  "row-selected",
			  G_CALLBACK (style_scheme_changed),
			  dlg);
	g_signal_connect (dlg->priv->install_scheme_button,
			  "clicked",
			  G_CALLBACK (install_scheme_clicked),
			  dlg);
	g_signal_connect (dlg->priv->uninstall_scheme_button,
			  "clicked",
			  G_CALLBACK (uninstall_scheme_clicked),
			  dlg);

	/* Set initial widget sensitivity */
	set_buttons_sensisitivity_according_to_scheme (dlg, def_id);
}

static void
setup_font_colors_page (GeditPreferencesDialog *dlg)
{
	setup_font_colors_page_font_section (dlg);
	setup_font_colors_page_style_scheme_section (dlg);
}

static void
setup_plugins_page (GeditPreferencesDialog *dlg)
{
	gtk_widget_show_all (dlg->priv->plugin_manager);
}

static void
gedit_preferences_dialog_init (GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	dlg->priv = gedit_preferences_dialog_get_instance_private (dlg);
	dlg->priv->editor = g_settings_new ("org.gnome.gedit.preferences.editor");
	dlg->priv->uisettings = g_settings_new ("org.gnome.gedit.preferences.ui");

	gtk_widget_init_template (GTK_WIDGET (dlg));

	setup_editor_page (dlg);
	setup_view_page (dlg);
	setup_font_colors_page (dlg);
	setup_plugins_page (dlg);
}

void
gedit_show_preferences_dialog (GeditWindow *parent)
{
	gedit_debug (DEBUG_PREFS);

	if (preferences_dialog == NULL)
	{
		preferences_dialog = GTK_WIDGET (g_object_new (GEDIT_TYPE_PREFERENCES_DIALOG,
							       "application", g_application_get_default (),
							       NULL));
		g_signal_connect (preferences_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &preferences_dialog);
	}

	if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (preferences_dialog)))
	{
		gtk_window_set_transient_for (GTK_WINDOW (preferences_dialog),
					      GTK_WINDOW (parent));
	}

	gtk_window_present (GTK_WINDOW (preferences_dialog));
}

/* ex:set ts=8 noet: */
