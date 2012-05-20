/* OGMRip - A library for DVD ripping and encoding
 * Copyright (C) 2004-2012 Olivier Rolland <billl@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __OGMRIP_LIST_ITEM_H__
#define __OGMRIP_LIST_ITEM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_LIST_ITEM               (ogmrip_list_item_get_type ())
#define OGMRIP_LIST_ITEM(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_LIST_ITEM, OGMRipListItem))
#define OGMRIP_LIST_ITEM_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_LIST_ITEM, OGMRipListItemClass))
#define OGMRIP_IS_LIST_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_LIST_ITEM))
#define OGMRIP_IS_LIST_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_LIST_ITEM))

typedef struct _OGMRipListItem      OGMRipListItem;
typedef struct _OGMRipListItemClass OGMRipListItemClass;
typedef struct _OGMRipListItemPriv  OGMRipListItemPriv;

struct _OGMRipListItem
{
  GtkBox parent_instance;

  OGMRipListItemPriv *priv;
};

struct _OGMRipListItemClass
{
  GtkBoxClass parent_class;

  void (* add_clicked)    (OGMRipListItem *item);
  void (* remove_clicked) (OGMRipListItem *item);
};

GType          ogmrip_list_item_get_type       (void);
GtkWidget *    ogmrip_list_item_new            (void);
GtkSizeGroup * ogmrip_list_item_get_size_group (OGMRipListItem *list);

G_END_DECLS

#endif /* __OGMRIP_LIST_ITEM_H__ */

