/* OGMRipBluray - A bluray library for OGMBr
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
  OGMBR_MAKEMKV_ERROR_MEM
} OGMBrMakeMKVError;

GQuark ogmbr_makemkv_error_quark (void);

typedef struct
{
  guint id;
  gchar *name;
  gchar *device;
  gchar *title;
} OGMBrDrive;

typedef enum
{
  OGMBR_MAKEMKV_UNKNOWN=0,
  OGMBR_MAKEMKV_TYPE=1,
  OGMBR_MAKEMKV_NAME=2,
  OGMBR_MAKEMKV_LANGCODE=3,
  OGMBR_MAKEMKV_LANGNAME=4,
  OGMBR_MAKEMKV_CODECID=5,
  OGMBR_MAKEMKV_CODECSHORT=6,
  OGMBR_MAKEMKV_CODECLONG=7,
  OGMBR_MAKEMKV_CHAPTERCOUNT=8,
  OGMBR_MAKEMKV_DURATION=9,
  OGMBR_MAKEMKV_DISCSIZE=10,
  OGMBR_MAKEMKV_DISCSIZEBYTES=11,
  OGMBR_MAKEMKV_STREAMTYPEEXTENSION=12,
  OGMBR_MAKEMKV_BITRATE=13,
  OGMBR_MAKEMKV_AUDIOCHANNELSCOUNT=14,
  OGMBR_MAKEMKV_ANGLEINFO=15,
  OGMBR_MAKEMKV_SOURCEFILENAME=16,
  OGMBR_MAKEMKV_AUDIOSAMPLERATE=17,
  OGMBR_MAKEMKV_AUDIOSAMPLESIZE=18,
  OGMBR_MAKEMKV_VIDEOSIZE=19,
  OGMBR_MAKEMKV_VIDEOASPECTRATIO=20,
  OGMBR_MAKEMKV_VIDEOFRAMERATE=21,
  OGMBR_MAKEMKV_STREAMFLAGS=22,
  OGMBR_MAKEMKV_DATETIME=23,
  OGMBR_MAKEMKV_ORIGINALTITLEID=24,
  OGMBR_MAKEMKV_SEGMENTSCOUNT=25,
  OGMBR_MAKEMKV_SEGMENTSMAP=26,
  OGMBR_MAKEMKV_MAXVALUE
} OGMBrMakeMKVInfo;

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

typedef void (* OGMBrMakeMKVProgress) (gdouble  fraction,
                                        gpointer user_data);

GType          ogmbr_makemkv_get_type             (void);
OGMBrMakeMKV * ogmbr_makemkv_get_default          (void);
GList *        ogmbr_makemkv_update_drives        (OGMBrMakeMKV         *mmkv,
                                                   GCancellable         *cancellable,
                                                   GError               **error);
void           ogmbr_makemkv_update_drives_async  (OGMBrMakeMKV         *mmkv,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);
GList *        ogmbr_makemkv_update_drives_finish (OGMBrMakeMKV         *mmkv,
                                                   GSimpleAsyncResult   *res,
                                                   GError               **error);
gboolean       ogmbr_makemkv_open_disc            (OGMBrMakeMKV         *mmkv,
                                                   OGMBrDisc            *disc,
                                                   GCancellable         *cancellable,
                                                   OGMBrMakeMKVProgress progress_cb,
                                                   gpointer             progress_data,
                                                   GError               **error);
void           ogmbr_makemkv_open_disc_async      (OGMBrMakeMKV         *mmkv,
                                                   OGMBrDisc            *disc,
                                                   GCancellable         *cancellable,
                                                   OGMBrMakeMKVProgress progress_cb,
                                                   gpointer             progress_data,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);
gboolean       ogmbr_makemkv_open_disc_finish     (OGMBrMakeMKV         *mmkv,
                                                   GSimpleAsyncResult   *res,
                                                   GError               **error);
gboolean       ogmbr_makemkv_close_disc           (OGMBrMakeMKV         *mmkv,
                                                   GCancellable         *cancellable,
                                                   GError               **error);
void           ogmbr_makemkv_close_disc_async     (OGMBrMakeMKV         *mmkv,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);
gboolean       ogmbr_makemkv_close_disc_finish    (OGMBrMakeMKV         *mmkv,
                                                   GSimpleAsyncResult   *res,
                                                   GError               **error);
gboolean       ogmbr_makemkv_eject_disc           (OGMBrMakeMKV         *mmkv,
                                                   OGMBrDisc            *disc,
                                                   GCancellable         *cancellable,
                                                   GError               **error);
void           ogmbr_makemkv_eject_disc_async     (OGMBrMakeMKV         *mmkv,
                                                   OGMBrDisc            *disc,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);
gboolean       ogmbr_makemkv_eject_disc_finish    (OGMBrMakeMKV         *mmkv,
                                                   GSimpleAsyncResult   *res,
                                                   GError               **error);
gboolean       ogmbr_makemkv_backup_disc          (OGMBrMakeMKV         *mmkv,
                                                   OGMBrDisc            *disc,
                                                   const gchar          *folder,
                                                   gboolean             decrypt,
                                                   GCancellable         *cancellable,
                                                   OGMBrMakeMKVProgress progress_cb,
                                                   gpointer             progress_data,
                                                   GError               **error);
void           ogmbr_makemkv_backup_disc_async    (OGMBrMakeMKV         *mmkv,
                                                   OGMBrDisc            *disc,
                                                   const gchar          *folder,
                                                   gboolean             decrypt,
                                                   GCancellable         *cancellable,
                                                   OGMBrMakeMKVProgress progress_cb,
                                                   gpointer             progress_data,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);
gboolean       ogmbr_makemkv_backup_disc_finish   (OGMBrMakeMKV         *mmkv,
                                                   GSimpleAsyncResult   *res,
                                                   GError               **error);

G_END_DECLS

#endif /* __OGMBR_MAKEMKV_H__ */

