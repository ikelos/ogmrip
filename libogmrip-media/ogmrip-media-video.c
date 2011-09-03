/* OGMRipMedia - A media library for OGMRip
 * Copyright (C) 2010-2011 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-media-video.h"
#include "ogmrip-media-stream.h"
#include "ogmrip-media-enums.h"

G_DEFINE_INTERFACE_WITH_CODE (OGMRipVideoStream, ogmrip_video_stream, G_TYPE_OBJECT,
    g_type_interface_add_prerequisite (g_define_type_id, OGMRIP_TYPE_STREAM);)

static void
ogmrip_video_stream_default_init (OGMRipVideoStreamInterface *iface)
{
}

gint
ogmrip_video_stream_get_bitrate (OGMRipVideoStream *video)
{
  OGMRipVideoStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_STREAM (video), -1);

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (!iface->get_bitrate)
    return 0;

  return iface->get_bitrate (video);
}

void 
ogmrip_video_stream_get_framerate (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMRipVideoStreamInterface *iface;

  g_return_if_fail (OGMRIP_IS_VIDEO_STREAM (video));

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (iface->get_framerate)
    iface->get_framerate (video, numerator, denominator);
}

void 
ogmrip_video_stream_get_resolution (OGMRipVideoStream *video, guint *width, guint *height)
{
  OGMRipVideoStreamInterface *iface;

  g_return_if_fail (OGMRIP_IS_VIDEO_STREAM (video));

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (iface->get_resolution)
    iface->get_resolution (video, width, height);
}

void
ogmrip_video_stream_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *width, guint *height)
{
  OGMRipVideoStreamInterface *iface;

  g_return_if_fail (OGMRIP_IS_VIDEO_STREAM (video));
  g_return_if_fail (x != NULL && y != NULL && width != NULL && height != NULL);

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (iface->get_crop_size)
    iface->get_crop_size (video, x, y, width, height);
  else
  {
    *x = *y = 0;
    ogmrip_video_stream_get_resolution (video, width, height);
  }
}

gint 
ogmrip_video_stream_get_standard (OGMRipVideoStream *video)
{
  OGMRipVideoStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_STREAM (video), -1);

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (!iface->get_standard)
    return OGMRIP_STANDARD_UNDEFINED;

  return iface->get_standard (video);
}

gint 
ogmrip_video_stream_get_aspect (OGMRipVideoStream *video)
{
  OGMRipVideoStreamInterface *iface;

  g_return_val_if_fail (OGMRIP_IS_VIDEO_STREAM (video), -1);

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (!iface->get_aspect)
    return OGMRIP_ASPECT_UNDEFINED;

  return iface->get_aspect (video);
}

void 
ogmrip_video_stream_get_aspect_ratio (OGMRipVideoStream *video, guint *numerator, guint *denominator)
{
  OGMRipVideoStreamInterface *iface;

  g_return_if_fail (OGMRIP_IS_VIDEO_STREAM (video));

  iface = OGMRIP_VIDEO_STREAM_GET_IFACE (video);

  if (iface->get_aspect_ratio)
    iface->get_aspect_ratio (video, numerator, denominator);
}

