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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ogmdvd-video
 * @title: OGMDvdVideo
 * @include: ogmdvd-video.h
 * @short_description: Structure describing an video stream
 */

#include "ogmdvd-video.h"
#include "ogmdvd-enums.h"
#include "ogmdvd-priv.h"

/**
 * ogmdvd_video_stream_get_framerate:
 * @stream: An #OGMDvdVideoStream
 * @numerator: A pointer to set the framerate numerator, or NULL
 * @denominator: A pointer to set the framerate denominator, or NULL
 
 * Gets the framerate of the DVD stream in the form of a fraction.
 */
void
ogmdvd_video_stream_get_framerate (OGMDvdVideoStream *stream, guint *numerator, guint *denominator)
{
  g_return_if_fail (stream != NULL);
  g_return_if_fail (numerator != NULL);
  g_return_if_fail (denominator != NULL);

  switch ((OGMDVD_STREAM (stream)->title->playback_time.frame_u & 0xc0) >> 6)
  {
    case 1:
      *numerator = 25;
      *denominator = 1;
      break;
    case 3:
      *numerator = 30000;
      *denominator = 1001;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/**
 * ogmdvd_video_stream_get_resolution:
 * @stream: An #OGMDvdVideoStream
 * @width: A pointer to set the width of the picture, or NULL
 * @height: A pointer to set the height of the picture, or NULL
 
 * Gets the resolution of the picture.
 */
void
ogmdvd_video_stream_get_resolution (OGMDvdVideoStream *stream, guint *width, guint *height)
{
  g_return_if_fail (stream != NULL);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  *width = 0;
  *height = 480;
  if (stream->video_format != 0)
    *height = 576;

  switch (stream->picture_size)
  {
    case 0:
      *width = 720;
      break;
    case 1:
      *width = 704;
      break;
    case 2:
      *width = 352;
      break;
    case 3:
      *width = 352;
      *width /= 2;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

void
ogmdvd_video_stream_get_crop_size (OGMDvdVideoStream *stream, guint *x, guint *y, guint *width, guint *height)
{
  g_return_if_fail (stream != NULL);

  if (x)
    *x = stream->crop_x;

  if (y)
    *y = stream->crop_y;

  if (width)
    *width = stream->crop_w;

  if (height)
    *height = stream->crop_h;
}

/**
 * ogmdvd_video_stream_get_aspect_ratio:
 * @stream: An #OGMDvdVideoStream
 * @numerator: A pointer to set the aspect ratio numerator, or NULL
 * @denominator: A pointer to set the aspect ratio denominator, or NULL
 
 * Gets the aspect ratio of the DVD stream in the form of a fraction.
 */
void
ogmdvd_video_stream_get_aspect_ratio  (OGMDvdVideoStream *stream, guint *numerator, guint *denominator)
{
  g_return_if_fail (stream != NULL);
  g_return_if_fail (numerator != NULL);
  g_return_if_fail (denominator != NULL);

  switch (stream->display_aspect_ratio)
  {
    case 0:
      *numerator = 4;
      *denominator = 3;
      break;
    case 1:
    case 3:
      *numerator = 16;
      *denominator = 9;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

/**
 * ogmdvd_video_stream_get_video_format:
 * @stream: An #OGMDvdVideoStream
 *
 * Returns the video format of the movie.
 *
 * Returns: #OGMDvdVideoFormat, or -1
 */
gint
ogmdvd_video_stream_get_video_format (OGMDvdVideoStream *stream)
{
  g_return_val_if_fail (stream != NULL, -1);

  return stream->video_format;
}

/**
 * ogmdvd_video_stream_get_display_aspect:
 * @stream: An #OGMDvdVideoStream
 *
 * Returns the display aspect of the movie.
 *
 * Returns: #OGMDvdDisplayAspect, or -1
 */
gint
ogmdvd_video_stream_get_display_aspect (OGMDvdVideoStream *stream)
{
  g_return_val_if_fail (stream != NULL, -1);

  switch (stream->display_aspect_ratio)
  {
    case 0:
      return OGMDVD_DISPLAY_ASPECT_4_3;
    case 1:
    case 3:
      return OGMDVD_DISPLAY_ASPECT_16_9;
    default:
      return -1;
  }
}

/**
 * ogmdvd_video_stream_get_display_format:
 * @stream: An #OGMDvdVideoStream
 *
 * Returns the display format of the movie.
 *
 * Returns: #OGMDvdDisplayFormat, or -1
 */
gint
ogmdvd_video_stream_get_display_format (OGMDvdVideoStream *stream)
{
  g_return_val_if_fail (stream != NULL, -1);

  return stream->video_format;
}

