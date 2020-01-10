/*
 * gedit-replace-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 Paolo Maggi
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

#ifndef __GEDIT_REPLACE_DIALOG_H__
#define __GEDIT_REPLACE_DIALOG_H__

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include "gedit-window.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_REPLACE_DIALOG              (gedit_replace_dialog_get_type())
#define GEDIT_REPLACE_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialog))
#define GEDIT_REPLACE_DIALOG_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialog const))
#define GEDIT_REPLACE_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialogClass))
#define GEDIT_IS_REPLACE_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_REPLACE_DIALOG))
#define GEDIT_IS_REPLACE_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_REPLACE_DIALOG))
#define GEDIT_REPLACE_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialogClass))

/* Private structure type */
typedef struct _GeditReplaceDialogPrivate GeditReplaceDialogPrivate;

/*
 * Main object structure
 */
typedef struct _GeditReplaceDialog GeditReplaceDialog;

struct _GeditReplaceDialog
{
	GtkDialog dialog;

	/*< private > */
	GeditReplaceDialogPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditReplaceDialogClass GeditReplaceDialogClass;

struct _GeditReplaceDialogClass
{
	GtkDialogClass parent_class;
};

enum
{
	GEDIT_REPLACE_DIALOG_FIND_RESPONSE = 100,
	GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE,
	GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE
};

/*
 * Public methods
 */
GType			 gedit_replace_dialog_get_type			(void) G_GNUC_CONST;

GtkWidget		*gedit_replace_dialog_new			(GeditWindow        *window);

void			 gedit_replace_dialog_present_with_time		(GeditReplaceDialog *dialog,
									 guint32             timestamp);

const gchar		*gedit_replace_dialog_get_search_text		(GeditReplaceDialog *dialog);

const gchar		*gedit_replace_dialog_get_replace_text		(GeditReplaceDialog *dialog);

gboolean		 gedit_replace_dialog_get_backwards		(GeditReplaceDialog *dialog);

void			 gedit_replace_dialog_set_replace_error		(GeditReplaceDialog *dialog,
									 const gchar        *error_msg);

G_END_DECLS

#endif  /* __GEDIT_REPLACE_DIALOG_H__  */

/* ex:set ts=8 noet: */
