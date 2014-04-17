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

#ifndef __OGMRIP_MEDIA_VIDEO_H__
#define __OGMRIP_MEDIA_VIDEO_H__

#include <ogmrip-media-types.h>

G_BEGIN_DECLS

#define OGMRIP_TYPE_VIDEO_STREAM            (ogmrip_video_stream_get_type ())
#define OGMRIP_VIDEO_STREAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OGMRIP_TYPE_VIDEO_STREAM, OGMRipVideoStream))
#define OGMRIP_IS_VIDEO_STREAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGMRIP_TYPE_VIDEO_STREAM))
#define OGMRIP_VIDEO_STREAM_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OGMRIP_TYPE_VIDEO_STREAM, OGMRipVideoStreamInterface))

typedef struct _OGMRipVideoStreamInterface OGMRipVideoStreamInterface;

struct _OGMRipVideoStreamInterface
{
  GTypeInterface base_iface;

  void         (* get_aspect_ratio) (OGMRipVideoStream   *video,
                                     guint               *numerator,
                                     guint               *denominator);
  gint         (* get_bitrate)      (OGMRipVideoStream   *video);
  void         (* get_crop_size)    (OGMRipVideoStream   *video,
                                     guint               *x,
                                     guint               *y,
                                     guint               *width,
                                     guint               *height);
  void         (* get_framerate)    (OGMRipVideoStream   *video,
                                     guint               *numerator,
                                     guint               *denominator);
  void         (* get_resolution)   (OGMRipVideoStream   *video,
                                     guint               *width,
                                     guint               *height);
  gint         (* get_standard)     (OGMRipVideoStream   *video);
  gint         (* get_start_delay)  (OGMRipVideoStream   *video);
};

GType        ogmrip_video_stream_get_type         (void) G_GNUC_CONST;
void         ogmrip_video_stream_get_aspect_ratio (OGMRipVideoStream   *video,
                                                   guint               *numerator,
                                                   guint               *denominator);
gint         ogmrip_video_stream_get_bitrate      (OGMRipVideoStream   *video);
void         ogmrip_video_stream_get_crop_size    (OGMRipVideoStream   *video,
                                                   guint               *x,
                                                   guint               *y,
                                                   guint               *width,
                                                   guint               *height);
void         ogmrip_video_stream_get_framerate    (OGMRipVideoStream   *video,
                                                   guint               *numerator,
                                                   guint               *denominator);
void         ogmrip_video_stream_get_resolution   (OGMRipVideoStream   *video,
                                                   guint               *width,
                                                   guint               *height);
gint         ogmrip_video_stream_get_standard     (OGMRipVideoStream   *video);
gint         ogmrip_video_stream_get_start_delay  (OGMRipVideoStream   *video);
gboolean     ogmrip_video_stream_equal            (OGMRipVideoStream   *video1,
                                                   OGMRipVideoStream   *video2);

G_END_DECLS

#endif /* __OGMRIP_MEDIA_VIDEO_H__ */

