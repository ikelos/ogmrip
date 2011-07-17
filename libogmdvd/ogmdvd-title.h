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

void                ogmdvd_title_ref                  (OGMDvdTitle  *title);
void                ogmdvd_title_unref                (OGMDvdTitle  *title);

gboolean            ogmdvd_title_open                 (OGMDvdTitle  *title,
                                                       GError       **error);
void                ogmdvd_title_close                (OGMDvdTitle  *title);
gboolean            ogmdvd_title_is_open              (OGMDvdTitle  *title);

gboolean            ogmdvd_title_analyze              (OGMDvdTitle  *title);
OGMDvdDisc *        ogmdvd_title_get_disc             (OGMDvdTitle  *title);
gint64              ogmdvd_title_get_vts_size         (OGMDvdTitle  *title);
gint                ogmdvd_title_get_nr               (OGMDvdTitle  *title);
gint                ogmdvd_title_get_ts_nr            (OGMDvdTitle  *title);
gdouble             ogmdvd_title_get_length           (OGMDvdTitle  *title,
                                                       OGMDvdTime   *length);
gdouble             ogmdvd_title_get_chapters_length  (OGMDvdTitle  *title, 
                                                       guint        start,
                                                       gint         end,
                                                       OGMDvdTime   *length);
void                ogmdvd_title_get_framerate        (OGMDvdTitle  *title,
                                                       guint        *numerator,
                                                       guint        *denominator);
void                ogmdvd_title_get_size             (OGMDvdTitle  *title,
                                                       guint        *width,
                                                       guint        *height);
gint                ogmdvd_title_get_video_format     (OGMDvdTitle  *title);
gint                ogmdvd_title_get_display_aspect   (OGMDvdTitle  *title);
gint                ogmdvd_title_get_display_format   (OGMDvdTitle  *title);
void                ogmdvd_title_get_aspect_ratio     (OGMDvdTitle  *title,
                                                       guint        *numerator,
                                                       guint        *denominator);
G_CONST_RETURN
guint *             ogmdvd_title_get_palette          (OGMDvdTitle  *title);
gint                ogmdvd_title_get_n_angles         (OGMDvdTitle  *title);
gint                ogmdvd_title_get_n_chapters       (OGMDvdTitle  *title);
gint                ogmdvd_title_get_n_audio_streams  (OGMDvdTitle  *title);
OGMDvdAudioStream * ogmdvd_title_get_nth_audio_stream (OGMDvdTitle  *title,
                                                       guint        nr);
GSList *            ogmdvd_title_get_audio_streams    (OGMDvdTitle  *title);
OGMDvdAudioStream * ogmdvd_title_find_audio_stream    (OGMDvdTitle  *title,
                                                       GCompareFunc func,
                                                       gpointer     data);
gint                ogmdvd_title_get_n_subp_streams   (OGMDvdTitle  *title);
OGMDvdSubpStream *  ogmdvd_title_get_nth_subp_stream  (OGMDvdTitle  *title,
                                                       guint        nr);
GSList *            ogmdvd_title_get_subp_streams     (OGMDvdTitle  *title);
OGMDvdSubpStream  * ogmdvd_title_find_subp_stream     (OGMDvdTitle  *title,
                                                       GCompareFunc func,
                                                       gpointer     data);

G_END_DECLS

#endif /* __OGMDVD_TITLE_H__ */

