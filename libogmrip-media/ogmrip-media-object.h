/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2014 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MEDIA_OBJECT_H__
#define __OGMRIP_MEDIA_OBJECT_H__

#include <ogmrip-media-types.h>

G_BEGIN_DECLS

#define OGMRIP_MEDIA_ERROR ogmrip_media_error_quark ()

typedef enum
{
  OGMRIP_MEDIA_ERROR_ID,
  OGMRIP_MEDIA_ERROR_OPEN,
  OGMRIP_MEDIA_ERROR_TITLE,
  OGMRIP_MEDIA_ERROR_VIDEO,
  OGMRIP_MEDIA_ERROR_AUDIO,
  OGMRIP_MEDIA_ERROR_SUBP,
  OGMRIP_MEDIA_ERROR_FORMAT,
  OGMRIP_MEDIA_ERROR_UNKNOWN
} OGMRipMediaError;

#define OGMRIP_TYPE_MEDIA            (ogmrip_media_get_type ())
#define OGMRIP_MEDIA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_MEDIA, OGMRipMedia))
#define OGMRIP_IS_MEDIA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_MEDIA))
#define OGMRIP_MEDIA_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_MEDIA, OGMRipMediaInterface))

typedef struct _OGMRipMediaInterface OGMRipMediaInterface;

struct _OGMRipMediaInterface
{
  GTypeInterface base_iface;

  gboolean      (* open)          (OGMRipMedia         *media,
                                   GCancellable        *cancellable,
                                   OGMRipMediaCallback callback,
                                   gpointer            user_data,
                                   GError              **error);
  void          (* close)         (OGMRipMedia         *media);
  gboolean      (* is_open)       (OGMRipMedia         *media);
  const gchar * (* get_label)     (OGMRipMedia         *media);
  const gchar * (* get_id)        (OGMRipMedia         *media);
  const gchar * (* get_uri)       (OGMRipMedia         *media);
  gint64        (* get_size)      (OGMRipMedia         *media);
  gint          (* get_n_titles)  (OGMRipMedia         *media);
  OGMRipTitle * (* get_title)     (OGMRipMedia         *media,
                                   guint               id);
  GList *       (* get_titles)    (OGMRipMedia         *media);
  OGMRipMedia * (* copy)          (OGMRipMedia         *media,
                                   const gchar         *path,
                                   GCancellable        *cancellable,
                                   OGMRipMediaCallback callback,
                                   gpointer            user_data,
                                   GError              **error);
  gboolean      (* is_copy)       (OGMRipMedia         *media,
                                   OGMRipMedia         *copy);
};

GQuark        ogmrip_media_error_quark   (void);

GType         ogmrip_media_get_type      (void) G_GNUC_CONST;
OGMRipMedia * ogmrip_media_new           (const gchar         *path,
                                          GError              **error);
gboolean      ogmrip_media_open          (OGMRipMedia         *media,
                                          GCancellable        *cancellable,
                                          OGMRipMediaCallback callback,
                                          gpointer            user_data,
                                          GError              **error);
void          ogmrip_media_close         (OGMRipMedia         *media);
gboolean      ogmrip_media_is_open       (OGMRipMedia         *media);
const gchar * ogmrip_media_get_label     (OGMRipMedia         *media);
const gchar * ogmrip_media_get_id        (OGMRipMedia         *media);
const gchar * ogmrip_media_get_uri       (OGMRipMedia         *media);
gint64        ogmrip_media_get_size      (OGMRipMedia         *media);
gint          ogmrip_media_get_n_titles  (OGMRipMedia         *media);
OGMRipTitle * ogmrip_media_get_title     (OGMRipMedia         *media,
                                          guint               id);
GList *       ogmrip_media_get_titles    (OGMRipMedia         *media);
OGMRipMedia * ogmrip_media_copy          (OGMRipMedia         *media,
                                          const gchar         *path,
                                          GCancellable        *cancellable,
                                          OGMRipMediaCallback callback,
                                          gpointer            user_data,
                                          GError              **error);
gboolean      ogmrip_media_is_copy       (OGMRipMedia         *media,
                                          OGMRipMedia         *copy);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_OBJECT_H__ */

