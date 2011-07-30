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

#ifndef __OGMDVD_VIDEO_H__
#define __OGMDVD_VIDEO_H__

#include <ogmdvd-types.h>

G_BEGIN_DECLS

gint     ogmdvd_video_stream_get_bitrate        (OGMDvdVideoStream *stream);
void     ogmdvd_video_stream_get_framerate      (OGMDvdVideoStream *stream,
                                                 guint             *numerator,
                                                 guint             *denominator);
void     ogmdvd_video_stream_get_resolution     (OGMDvdVideoStream *stream,
                                                 guint             *width,
                                                 guint             *height);
void     ogmdvd_video_stream_get_crop_size      (OGMDvdVideoStream *stream,
                                                 guint             *x,
                                                 guint             *y,
                                                 guint             *width,
                                                 guint             *height);
gint     ogmdvd_video_stream_get_display_format (OGMDvdVideoStream *stream);
gint     ogmdvd_video_stream_get_display_aspect (OGMDvdVideoStream *stream);
void     ogmdvd_video_stream_get_aspect_ratio   (OGMDvdVideoStream *stream,
                                                 guint             *numerator,
                                                 guint             *denominator);
gboolean ogmdvd_video_stream_get_telecine       (OGMDvdVideoStream *stream);
gboolean ogmdvd_video_stream_get_interlaced     (OGMDvdVideoStream *stream);
gboolean ogmdvd_video_stream_get_progressive    (OGMDvdVideoStream *stream);

G_END_DECLS

#endif /* __OGMDVD_H__ */

