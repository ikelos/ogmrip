/* OGMDvd - A wrapper library around libdvdread
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

#ifndef __OGMDVD_TITLE_H__
#define __OGMDVD_TITLE_H__

#include <ogmdvd-types.h>

G_BEGIN_DECLS

typedef void (* OGMDvdTitleCallback) (OGMDvdTitle *title,
                                      gdouble     percent,
                                      gpointer    user_data);

void                ogmdvd_title_ref                  (OGMDvdTitle         *title);
void                ogmdvd_title_unref                (OGMDvdTitle         *title);

gboolean            ogmdvd_title_open                 (OGMDvdTitle         *title,
                                                       GError              **error);
void                ogmdvd_title_close                (OGMDvdTitle         *title);
gboolean            ogmdvd_title_is_open              (OGMDvdTitle         *title);

OGMDvdDisc *        ogmdvd_title_get_disc             (OGMDvdTitle         *title);
gint64              ogmdvd_title_get_vts_size         (OGMDvdTitle         *title);
gint                ogmdvd_title_get_nr               (OGMDvdTitle         *title);
gint                ogmdvd_title_get_ts_nr            (OGMDvdTitle         *title);
gboolean            ogmdvd_title_get_progressive      (OGMDvdTitle         *title);
gboolean            ogmdvd_title_get_telecine         (OGMDvdTitle         *title);
gboolean            ogmdvd_title_get_interlaced       (OGMDvdTitle         *title);
gdouble             ogmdvd_title_get_length           (OGMDvdTitle         *title,
                                                       OGMRipTime          *length);
gdouble             ogmdvd_title_get_chapters_length  (OGMDvdTitle         *title, 
                                                       guint               start,
                                                       gint                end,
                                                       OGMRipTime          *length);
const guint *       ogmdvd_title_get_palette          (OGMDvdTitle         *title);
gint                ogmdvd_title_get_n_angles         (OGMDvdTitle         *title);
gint                ogmdvd_title_get_n_chapters       (OGMDvdTitle         *title);
OGMDvdVideoStream * ogmdvd_title_get_video_stream     (OGMDvdTitle         *title);
gint                ogmdvd_title_get_n_audio_streams  (OGMDvdTitle         *title);
OGMDvdAudioStream * ogmdvd_title_get_nth_audio_stream (OGMDvdTitle         *title,
                                                       guint               nr);
GList *             ogmdvd_title_get_audio_streams    (OGMDvdTitle         *title);
gint                ogmdvd_title_get_n_subp_streams   (OGMDvdTitle         *title);
OGMDvdSubpStream *  ogmdvd_title_get_nth_subp_stream  (OGMDvdTitle         *title,
                                                       guint               nr);
GList *             ogmdvd_title_get_subp_streams     (OGMDvdTitle         *title);
gboolean            ogmdvd_title_analyze              (OGMDvdTitle         *title,
                                                       GCancellable        *cancellable,
                                                       OGMDvdTitleCallback callback,
                                                       gpointer            user_data,
                                                       GError              **error);

G_END_DECLS

#endif /* __OGMDVD_TITLE_H__ */

