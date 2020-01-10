
/*
 * gedit-file-browser-message-add-context-item.h
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Jesse van den Kieboom
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_H__
#define __GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM            (gedit_file_browser_message_add_context_item_get_type ())
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItem))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItem const))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItemClass))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM))
#define GEDIT_IS_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM))
#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                                     GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,\
                                                                     GeditFileBrowserMessageAddContextItemClass))

typedef struct _GeditFileBrowserMessageAddContextItem        GeditFileBrowserMessageAddContextItem;
typedef struct _GeditFileBrowserMessageAddContextItemClass   GeditFileBrowserMessageAddContextItemClass;
typedef struct _GeditFileBrowserMessageAddContextItemPrivate GeditFileBrowserMessageAddContextItemPrivate;

struct _GeditFileBrowserMessageAddContextItem
{
	GeditMessage parent;

	GeditFileBrowserMessageAddContextItemPrivate *priv;
};

struct _GeditFileBrowserMessageAddContextItemClass
{
	GeditMessageClass parent_class;
};

GType gedit_file_browser_message_add_context_item_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_H__ */
