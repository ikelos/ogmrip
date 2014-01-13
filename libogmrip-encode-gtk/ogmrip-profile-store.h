/* OGMRip - A library for media ripping and encoding
 * Copyright (C) 2004-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_PROFILE_STORE_H__
#define __OGMRIP_PROFILE_STORE_H__

#include <gtk/gtk.h>

#include <ogmrip-profile-engine.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_PROFILE_STORE_NAME_COLUMN,
  OGMRIP_PROFILE_STORE_PROFILE_COLUMN,
  OGMRIP_PROFILE_STORE_N_COLUMNS
} OGMRipProfileStoreColumns;

#define OGMRIP_TYPE_PROFILE_STORE            (ogmrip_profile_store_get_type ())
#define OGMRIP_PROFILE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_PROFILE_STORE, OGMRipProfileStore))
#define OGMRIP_PROFILE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_PROFILE_STORE, OGMRipProfileStoreClass))
#define OGMRIP_IS_PROFILE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_PROFILE_STORE))
#define OGMRIP_IS_PROFILE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_PROFILE_STORE))

typedef struct _OGMRipProfileStore      OGMRipProfileStore;
typedef struct _OGMRipProfileStoreClass OGMRipProfileStoreClass;
typedef struct _OGMRipProfileStorePriv  OGMRipProfileStorePriv;

struct _OGMRipProfileStore
{
  GtkListStore parent_instance;

  OGMRipProfileStorePriv *priv;
};

struct _OGMRipProfileStoreClass
{
  GtkListStoreClass parent_class;
};

GType                ogmrip_profile_store_get_type    (void);
OGMRipProfileStore * ogmrip_profile_store_new         (OGMRipProfileEngine *engine,
                                                       gboolean            valid_only);
void                 ogmrip_profile_store_reload      (OGMRipProfileStore  *store);
OGMRipProfile *      ogmrip_profile_store_get_profile (OGMRipProfileStore  *store,
                                                       GtkTreeIter         *iter);
gboolean             ogmrip_profile_store_get_iter    (OGMRipProfileStore  *store,
                                                       GtkTreeIter         *iter,
                                                       OGMRipProfile       *profile);

G_END_DECLS

#endif /* __OGMRIP_PROFILE_STORE_H__ */

