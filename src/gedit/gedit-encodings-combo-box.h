/*
 * gedit-encodings-combo-box.h
 * This file is part of gedit
 *
 * Copyright (C) 2003-2005 - Paolo Maggi
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

#ifndef __GEDIT_ENCODINGS_COMBO_BOX_H__
#define __GEDIT_ENCODINGS_COMBO_BOX_H__

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_ENCODINGS_COMBO_BOX             (gedit_encodings_combo_box_get_type ())
#define GEDIT_ENCODINGS_COMBO_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_ENCODINGS_COMBO_BOX, GeditEncodingsComboBox))
#define GEDIT_ENCODINGS_COMBO_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_ENCODINGS_COMBO_BOX, GeditEncodingsComboBoxClass))
#define GEDIT_IS_ENCODINGS_COMBO_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_ENCODINGS_COMBO_BOX))
#define GEDIT_IS_ENCODINGS_COMBO_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_ENCODINGS_COMBO_BOX))
#define GEDIT_ENCODINGS_COMBO_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_ENCODINGS_COMBO_BOX, GeditEncodingsComboBoxClass))

typedef struct _GeditEncodingsComboBox		GeditEncodingsComboBox;
typedef struct _GeditEncodingsComboBoxClass	GeditEncodingsComboBoxClass;
typedef struct _GeditEncodingsComboBoxPrivate	GeditEncodingsComboBoxPrivate;

struct _GeditEncodingsComboBox
{
	GtkComboBox parent;

	GeditEncodingsComboBoxPrivate *priv;
};

struct _GeditEncodingsComboBoxClass
{
	GtkComboBoxClass parent_class;
};

GType			 gedit_encodings_combo_box_get_type			(void) G_GNUC_CONST;

GtkWidget		*gedit_encodings_combo_box_new 				(gboolean save_mode);

const GtkSourceEncoding	*gedit_encodings_combo_box_get_selected_encoding	(GeditEncodingsComboBox *menu);

void			 gedit_encodings_combo_box_set_selected_encoding	(GeditEncodingsComboBox  *menu,
										 const GtkSourceEncoding *encoding);

G_END_DECLS

#endif /* __GEDIT_ENCODINGS_COMBO_BOX_H__ */

/* ex:set ts=8 noet: */
