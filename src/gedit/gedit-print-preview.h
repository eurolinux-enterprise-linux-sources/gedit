/*
 * gedit-print-preview.h
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

#ifndef __GEDIT_PRINT_PREVIEW_H__
#define __GEDIT_PRINT_PREVIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PRINT_PREVIEW            (gedit_print_preview_get_type ())
#define GEDIT_PRINT_PREVIEW(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), GEDIT_TYPE_PRINT_PREVIEW, GeditPrintPreview))
#define GEDIT_PRINT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_PRINT_PREVIEW, GeditPrintPreviewClass))
#define GEDIT_IS_PRINT_PREVIEW(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), GEDIT_TYPE_PRINT_PREVIEW))
#define GEDIT_IS_PRINT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PRINT_PREVIEW))
#define GEDIT_PRINT_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_PRINT_PREVIEW, GeditPrintPreviewClass))

typedef struct _GeditPrintPreview        GeditPrintPreview;
typedef struct _GeditPrintPreviewPrivate GeditPrintPreviewPrivate;
typedef struct _GeditPrintPreviewClass   GeditPrintPreviewClass;

struct _GeditPrintPreview
{
	GtkGrid parent;

	GeditPrintPreviewPrivate *priv;
};

struct _GeditPrintPreviewClass
{
	GtkGridClass parent_class;

	void (* close)		(GeditPrintPreview          *preview);
};


GType		 gedit_print_preview_get_type	(void) G_GNUC_CONST;

GtkWidget	*gedit_print_preview_new	(GtkPrintOperation		*op,
						 GtkPrintOperationPreview	*gtk_preview,
						 GtkPrintContext		*context);

G_END_DECLS

#endif /* __GEDIT_PRINT_PREVIEW_H__ */

/* ex:set ts=8 noet: */
