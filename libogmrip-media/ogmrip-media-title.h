/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2013 Olivier Rolland <billl@users.sourceforge.net>
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

#ifndef __OGMRIP_MEDIA_TITLE_H__
#define __OGMRIP_MEDIA_TITLE_H__

#include <ogmrip-media-time.h>
#include <ogmrip-media-types.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_TITLE            (ogmrip_title_get_type ())
#define OGMRIP_TITLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_TITLE, OGMRipTitle))
#define OGMRIP_IS_TITLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_TITLE))
#define OGMRIP_TITLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_TITLE, OGMRipTitleInterface))

typedef struct _OGMRipTitleInterface OGMRipTitleInterface;

struct _OGMRipTitleInterface
{
  GTypeInterface base_iface;

  gboolean            (* open)                 (OGMRipTitle         *title,
                                                GCancellable        *cancellable,
                                                OGMRipTitleCallback callback,
                                                gpointer            user_data,
                                                GError              **error);
  void                (* close)                (OGMRipTitle         *title);
  gboolean            (* is_open)              (OGMRipTitle         *title);
  OGMRipMedia *       (* get_media)            (OGMRipTitle         *title);
  gint64              (* get_size)             (OGMRipTitle         *title);
  gint                (* get_id)               (OGMRipTitle         *title);
  gboolean            (* get_progressive)      (OGMRipTitle         *title);
  gboolean            (* get_telecine)         (OGMRipTitle         *title);
  gboolean            (* get_interlaced)       (OGMRipTitle         *title);
  gdouble             (* get_length)           (OGMRipTitle         *title,
                                                OGMRipTime          *length);
  gdouble             (* get_chapters_length)  (OGMRipTitle         *title, 
                                                guint               start,
                                                guint               end,
                                                OGMRipTime          *length);
  const guint *       (* get_palette)          (OGMRipTitle         *title);
  gint                (* get_n_angles)         (OGMRipTitle         *title);
  gint                (* get_n_chapters)       (OGMRipTitle         *title);
  OGMRipVideoStream * (* get_video_stream)     (OGMRipTitle         *title);
  gint                (* get_n_audio_streams)  (OGMRipTitle         *title);
  OGMRipAudioStream * (* get_audio_stream)     (OGMRipTitle         *title,
                                                guint               id);
  GList *             (* get_audio_streams)    (OGMRipTitle         *title);
  gint                (* get_n_subp_streams)   (OGMRipTitle         *title);
  OGMRipSubpStream *  (* get_subp_stream)      (OGMRipTitle         *title,
                                                guint               id);
  GList *             (* get_subp_streams)     (OGMRipTitle         *title);
  gboolean            (* analyze)              (OGMRipTitle         *title,
                                                GCancellable        *cancellable,
                                                OGMRipTitleCallback callback,
                                                gpointer            user_data,
                                                GError              **error);
  OGMRipMedia *       (* copy)                 (OGMRipTitle         *title,
                                                const gchar         *path,
                                                GCancellable        *cancellable,
                                                OGMRipTitleCallback callback,
                                                gpointer            user_data,
                                                GError              **error);
  gboolean            (* is_copy)              (OGMRipTitle         *title,
                                                OGMRipTitle         *copy);
};

GType               ogmrip_title_get_type             (void) G_GNUC_CONST;
gboolean            ogmrip_title_open                 (OGMRipTitle         *title,
                                                       GCancellable        *cancellable,
                                                       OGMRipTitleCallback callback,
                                                       gpointer            user_data,
                                                       GError              **error);
void                ogmrip_title_close                (OGMRipTitle         *title);
gboolean            ogmrip_title_is_open              (OGMRipTitle         *title);
OGMRipMedia *       ogmrip_title_get_media            (OGMRipTitle         *title);
gint64              ogmrip_title_get_size             (OGMRipTitle         *title);
gint                ogmrip_title_get_id               (OGMRipTitle         *title);
gboolean            ogmrip_title_get_progressive      (OGMRipTitle         *title);
gboolean            ogmrip_title_get_telecine         (OGMRipTitle         *title);
gboolean            ogmrip_title_get_interlaced       (OGMRipTitle         *title);
gdouble             ogmrip_title_get_length           (OGMRipTitle         *title,
                                                       OGMRipTime          *length);
gdouble             ogmrip_title_get_chapters_length  (OGMRipTitle         *title, 
                                                       guint               start,
                                                       gint                end,
                                                       OGMRipTime          *length);
const guint *       ogmrip_title_get_palette          (OGMRipTitle         *title);
gint                ogmrip_title_get_n_angles         (OGMRipTitle         *title);
gint                ogmrip_title_get_n_chapters       (OGMRipTitle         *title);
OGMRipVideoStream * ogmrip_title_get_video_stream     (OGMRipTitle         *title);
gint                ogmrip_title_get_n_audio_streams  (OGMRipTitle         *title);
OGMRipAudioStream * ogmrip_title_get_audio_stream     (OGMRipTitle         *title,
                                                       guint               id);
GList *             ogmrip_title_get_audio_streams    (OGMRipTitle         *title);
gint                ogmrip_title_get_n_subp_streams   (OGMRipTitle         *title);
OGMRipSubpStream *  ogmrip_title_get_subp_stream      (OGMRipTitle         *title,
                                                       guint               id);
GList *             ogmrip_title_get_subp_streams     (OGMRipTitle         *title);
gboolean            ogmrip_title_analyze              (OGMRipTitle         *title,
                                                       GCancellable        *cancellable,
                                                       OGMRipTitleCallback callback,
                                                       gpointer            user_data,
                                                       GError              **error);
OGMRipMedia *       ogmrip_title_copy                 (OGMRipTitle         *title,
                                                       const gchar         *path,
                                                       GCancellable        *cancellable,
                                                       OGMRipTitleCallback callback,
                                                       gpointer            user_data,
                                                       GError              **error);
gboolean            ogmrip_title_is_copy              (OGMRipTitle         *title,
                                                       OGMRipTitle         *copy);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_TITLE_H__ */

