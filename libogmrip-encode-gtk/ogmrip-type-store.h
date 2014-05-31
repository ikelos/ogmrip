/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_TYPE_STORE_H__
#define __OGMRIP_TYPE_STORE_H__

#include <gtk/gtk.h>
#include <ogmrip-base.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_TYPE_STORE_DESCRIPTION_COLUMN,
  OGMRIP_TYPE_STORE_TYPE_COLUMN,
  OGMRIP_TYPE_STORE_N_COLUMNS
} OGMRipTypeStoreColumns;

#define OGMRIP_TYPE_TYPE_STORE            (ogmrip_type_store_get_type ())
#define OGMRIP_TYPE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_TYPE_STORE, OGMRipTypeStore))
#define OGMRIP_TYPE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_TYPE_STORE, OGMRipTypeStoreClass))
#define OGMRIP_IS_TYPE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_TYPE_STORE))
#define OGMRIP_IS_TYPE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_TYPE_STORE))

typedef struct _OGMRipTypeStore      OGMRipTypeStore;
typedef struct _OGMRipTypeStoreClass OGMRipTypeStoreClass;
typedef struct _OGMRipTypeStorePriv  OGMRipTypeStorePriv;

struct _OGMRipTypeStore
{
  GtkListStore parent_instance;

  OGMRipTypeStorePriv *priv;
};

struct _OGMRipTypeStoreClass
{
  GtkListStoreClass parent_class;
};

GType             ogmrip_type_store_get_type  (void);
OGMRipTypeStore * ogmrip_type_store_new       (GType           parent);
void              ogmrip_type_store_reload    (OGMRipTypeStore *store);
GType             ogmrip_type_store_get_gtype (OGMRipTypeStore *store,
                                               GtkTreeIter     *iter);
gboolean          ogmrip_type_store_get_iter  (OGMRipTypeStore *store,
                                               GtkTreeIter     *iter,
                                               GType           gtype);

G_END_DECLS

#endif /* __OGMRIP_TYPE_STORE_H__ */

