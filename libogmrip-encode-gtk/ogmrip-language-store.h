/* OGMRip - A wrapper library around libdvdread
 * Copyright (C) 2004-2010 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_LANGUAGE_STORE_H__
#define __OGMRIP_LANGUAGE_STORE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
{
  OGMRIP_LANGUAGE_STORE_NAME_COLUMN,
  OGMRIP_LANGUAGE_STORE_CODE_COLUMN,
  OGMRIP_LANGUAGE_STORE_N_COLUMNS
} OGMRipLanguageStoreColumns;

#define OGMRIP_TYPE_LANGUAGE_STORE            (ogmrip_language_store_get_type ())
#define OGMRIP_LANGUAGE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_LANGUAGE_STORE, OGMRipLanguageStore))
#define OGMRIP_LANGUAGE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGMRIP_TYPE_LANGUAGE_STORE, OGMRipLanguageStoreClass))
#define OGMRIP_IS_LANGUAGE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, OGMRIP_TYPE_LANGUAGE_STORE))
#define OGMRIP_IS_LANGUAGE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMRIP_TYPE_LANGUAGE_STORE))

typedef struct _OGMRipLanguageStore      OGMRipLanguageStore;
typedef struct _OGMRipLanguageStoreClass OGMRipLanguageStoreClass;

struct _OGMRipLanguageStore
{
  GtkListStore parent_instance;
};

struct _OGMRipLanguageStoreClass
{
  GtkListStoreClass parent_class;
};

GType                 ogmrip_language_store_get_type (void);
OGMRipLanguageStore * ogmrip_language_store_new      (void);

G_END_DECLS

#endif /* __OGMRIP_LANGUAGE_STORE_H__ */

