/* OGMRipBluray - A bluray library for OGMRip
 * Copyright (C) 2012 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMBR_MAKEMKV_H__
#define __OGMBR_MAKEMKV_H__

#include <ogmrip-bluray-disc.h>

G_BEGIN_DECLS

#define OGMBR_MAKEMKV_ERROR ogmbr_makemkv_error_quark ()

typedef enum
{
  OGMBR_MAKEMKV_ERROR_COMM,
  OGMBR_MAKEMKV_ERROR_MEM,
  OGMBR_MAKEMKV_ERROR_NO_DISC
} OGMBrMakeMKVError;

GQuark ogmbr_makemkv_error_quark (void);

#define OGMBR_TYPE_MAKEMKV             (ogmbr_makemkv_get_type ())
#define OGMBR_MAKEMKV(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMBR_TYPE_MAKEMKV, OGMBrMakeMKV))
#define OGMBR_IS_MAKEMKV(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMBR_TYPE_MAKEMKV))
#define OGMBR_MAKEMKV_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OGMBR_TYPE_MAKEMKV, OGMBrMakeMKVClass))
#define OGMBR_IS_MAKEMKV_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OGMBR_TYPE_MAKEMKV))
#define OGMBR_MAKEMKV_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OGMBR_TYPE_MAKEMKV, OGMBrMakeMKVClass))

typedef struct _OGMBrMakeMKV      OGMBrMakeMKV;
typedef struct _OGMBrMakeMKVClass OGMBrMakeMKVClass;
typedef struct _OGMBrMakeMKVPriv  OGMBrMakeMKVPriv;

struct _OGMBrMakeMKV
{
  GObject parent_instance;

  OGMBrMakeMKVPriv *priv;
};

struct _OGMBrMakeMKVClass
{
  GObjectClass parent_class;
};

typedef void (* OGMBrProgress) (gdouble  fraction,
                                gpointer user_data);

GType          ogmbr_makemkv_get_type             (void);
OGMBrMakeMKV * ogmbr_makemkv_get_default          (void);
gboolean       ogmbr_makemkv_has_drive            (OGMBrMakeMKV        *mmkv,
                                                   const gchar         *device);
gboolean       ogmbr_makemkv_update_drives        (OGMBrMakeMKV        *mmkv,
                                                   GCancellable        *cancellable,
                                                   GError              **error);
void           ogmbr_makemkv_update_drives_async  (OGMBrMakeMKV        *mmkv,
                                                   GCancellable        *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer            user_data);
gboolean       ogmbr_makemkv_update_drives_finish (OGMBrMakeMKV        *mmkv,
                                                   GSimpleAsyncResult  *res,
                                                   GError              **error);
gboolean       ogmbr_makemkv_open_disc            (OGMBrMakeMKV        *mmkv,
                                                   OGMBrDisc           *disc,
                                                   GCancellable        *cancellable,
                                                   OGMBrProgress       progress_cb,
                                                   gpointer            progress_data,
                                                   GError              **error);
void           ogmbr_makemkv_open_disc_async      (OGMBrMakeMKV        *mmkv,
                                                   OGMBrDisc           *disc,
                                                   GCancellable        *cancellable,
                                                   OGMBrProgress       progress_cb,
                                                   gpointer            progress_data,
                                                   GAsyncReadyCallback callback,
                                                   gpointer            user_data);
gboolean       ogmbr_makemkv_open_disc_finish     (OGMBrMakeMKV        *mmkv,
                                                   GSimpleAsyncResult  *res,
                                                   GError              **error);
gboolean       ogmbr_makemkv_close_disc           (OGMBrMakeMKV        *mmkv,
                                                   GCancellable        *cancellable,
                                                   GError              **error);
void           ogmbr_makemkv_close_disc_async     (OGMBrMakeMKV        *mmkv,
                                                   GCancellable        *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer            user_data);
gboolean       ogmbr_makemkv_close_disc_finish    (OGMBrMakeMKV        *mmkv,
                                                   GSimpleAsyncResult  *res,
                                                   GError              **error);
gboolean       ogmbr_makemkv_eject_disc           (OGMBrMakeMKV        *mmkv,
                                                   OGMBrDisc           *disc,
                                                   GCancellable        *cancellable,
                                                   GError              **error);
void           ogmbr_makemkv_eject_disc_async     (OGMBrMakeMKV        *mmkv,
                                                   OGMBrDisc           *disc,
                                                   GCancellable        *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer            user_data);
gboolean       ogmbr_makemkv_eject_disc_finish    (OGMBrMakeMKV        *mmkv,
                                                   GSimpleAsyncResult  *res,
                                                   GError              **error);
gboolean       ogmbr_makemkv_backup_disc          (OGMBrMakeMKV        *mmkv,
                                                   OGMBrDisc           *disc,
                                                   const gchar         *folder,
                                                   gboolean            decrypt,
                                                   GCancellable        *cancellable,
                                                   OGMBrProgress       progress_cb,
                                                   gpointer            progress_data,
                                                   GError              **error);
void           ogmbr_makemkv_backup_disc_async    (OGMBrMakeMKV        *mmkv,
                                                   OGMBrDisc           *disc,
                                                   const gchar         *folder,
                                                   gboolean            decrypt,
                                                   GCancellable        *cancellable,
                                                   OGMBrProgress       progress_cb,
                                                   gpointer            progress_data,
                                                   GAsyncReadyCallback callback,
                                                   gpointer            user_data);
gboolean       ogmbr_makemkv_backup_disc_finish   (OGMBrMakeMKV        *mmkv,
                                                   GSimpleAsyncResult  *res,
                                                   GError              **error);

G_END_DECLS

#endif /* __OGMBR_MAKEMKV_H__ */

