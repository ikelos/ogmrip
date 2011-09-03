/* OGMRipFile - A file library for OGMRip
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

#include "ogmrip-video-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

#include <stdio.h>
#include <stdlib.h>

static void ogmrip_video_iface_init  (OGMRipVideoStreamInterface *iface);
static GObject * ogmrip_video_file_constructor  (GType type,
                                                 guint n_properties,
                                                 GObjectConstructParam *properties);

G_DEFINE_TYPE_WITH_CODE (OGMRipVideoFile, ogmrip_video_file, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmrip_video_iface_init));

static void
ogmrip_video_file_init (OGMRipVideoFile *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMRIP_TYPE_VIDEO_FILE, OGMRipVideoFilePriv);

  stream->priv->bitrate = -1;
  stream->priv->framerate_denom = 1;
  stream->priv->standard = OGMRIP_STANDARD_UNDEFINED;
  stream->priv->aspect = OGMRIP_ASPECT_UNDEFINED;
}

static void
ogmrip_video_file_class_init (OGMRipVideoFileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = ogmrip_video_file_constructor;

  g_type_class_add_private (klass, sizeof (OGMRipVideoFilePriv));
}

static GObject *
ogmrip_video_file_constructor (GType type, guint n_properties, GObjectConstructParam *properties)
{
  GObject *gobject;
  OGMRipMediaInfo *info;

  gobject = G_OBJECT_CLASS (ogmrip_video_file_parent_class)->constructor (type, n_properties, properties);

  info = ogmrip_media_info_get_default ();
  if (!info || !OGMRIP_FILE (gobject)->priv->path)
  {
    g_object_unref (gobject);
    return NULL;
  }

  if (g_file_test (OGMRIP_FILE (gobject)->priv->path, G_FILE_TEST_EXISTS) &&
      ogmrip_media_info_open (info, OGMRIP_FILE (gobject)->priv->path))
  {
    OGMRipVideoFile *video = OGMRIP_VIDEO_FILE (gobject);
    const gchar *str;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "VideoCount");
    if (!str || !g_str_equal (str, "1"))
    {
      g_object_unref (info);
      g_object_unref (gobject);
      return NULL;
    }

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Duration");
    OGMRIP_FILE (gobject)->priv->length = str ? atoi (str) / 1000. : -1.0;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "StreamSize");
    OGMRIP_FILE (gobject)->priv->size = str ? atoll (str) : -1;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "Title");
    OGMRIP_FILE (gobject)->priv->label = str ? g_strdup (str) : NULL;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "BitRate");
    video->priv->bitrate = str ? atoi (str) : -1;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Width");
    video->priv->width = str ? atoi (str) : 0;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Height");
    video->priv->height = str ? atoi (str) : 0;

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Standard");
    if (str)
    {
      if (g_str_equal (str, "PAL"))
        video->priv->standard = OGMRIP_STANDARD_PAL;
      else if (g_str_equal (str, "NTSC"))
        video->priv->standard = OGMRIP_STANDARD_NTSC;
    }

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "FrameRate");
    if (str)
    {
      if (g_str_equal (str, "25.000"))
        video->priv->framerate_num = 25;
      else if (g_str_equal (str, "23.976"))
      {
        video->priv->framerate_num = 24000;
        video->priv->framerate_num = 1001;
      }
      else if (g_str_equal (str, "29.970"))
      {
        video->priv->framerate_num = 30000;
        video->priv->framerate_num = 1001;
      }
      else
      {
        gdouble framerate;
        
        framerate = strtod (str, NULL);
        video->priv->framerate_num = framerate * 1000;
        video->priv->framerate_num = 1000;
      }
    }

    str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "DisplayAspectRatio");
    if (str)
    {
      if (sscanf (str, "%u:%u", &video->priv->aspect_num, &video->priv->aspect_denom) != 2)
      {
        gdouble aspect;

        aspect = strtod (str, NULL);
        video->priv->aspect_num = aspect * 1000;
        video->priv->aspect_denom = 1000;
      }

      if (video->priv->aspect_num == 16 && video->priv->aspect_denom == 9)
        video->priv->aspect = OGMRIP_ASPECT_16_9;
      else if (video->priv->aspect_num == 16 && video->priv->aspect_denom == 9)
        video->priv->aspect = OGMRIP_ASPECT_4_3;
    }

    /*
     * TODO delay
     */

    ogmrip_media_info_close (info);
  }

  return gobject;
}

static gint
ogmrip_video_file_get_aspect (OGMRipVideoStream *video)
{
  return OGMRIP_VIDEO_FILE (video)->priv->aspect;
}

static void
ogmrip_video_file_get_aspect_ratio (OGMRipVideoStream *video, guint *num, guint *denom)
{
  if (num)
    *num = OGMRIP_VIDEO_FILE (video)->priv->aspect_num;

  if (denom)
    *denom = OGMRIP_VIDEO_FILE (video)->priv->aspect_denom;
}

static gint
ogmrip_video_file_get_bitrate (OGMRipVideoStream *video)
{
  return OGMRIP_VIDEO_FILE (video)->priv->bitrate;
}

static void
ogmrip_video_file_get_crop_size (OGMRipVideoStream *video, guint *x, guint *y, guint *w, guint *h)
{
  if (x)
    *x = 0;

  if (y)
    *y = 0;

  if (w)
    *w = OGMRIP_VIDEO_FILE (video)->priv->width;

  if (h)
    *h = OGMRIP_VIDEO_FILE (video)->priv->height;
}

static void
ogmrip_video_file_get_framerate (OGMRipVideoStream *video, guint *n, guint *d)
{
  if (n)
    *n = OGMRIP_VIDEO_FILE (video)->priv->framerate_num;

  if (d)
    *d = OGMRIP_VIDEO_FILE (video)->priv->framerate_denom;
}

static gint
ogmrip_video_file_get_standard (OGMRipVideoStream *video)
{
  return OGMRIP_VIDEO_FILE (video)->priv->standard;
}

static void
ogmrip_video_file_get_resolution (OGMRipVideoStream *video, guint *w, guint *h)
{
  if (w)
    *w = OGMRIP_VIDEO_FILE (video)->priv->width;

  if (h)
    *h = OGMRIP_VIDEO_FILE (video)->priv->height;
}

static void
ogmrip_video_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_aspect = ogmrip_video_file_get_aspect;
  iface->get_aspect_ratio = ogmrip_video_file_get_aspect_ratio;
  iface->get_bitrate = ogmrip_video_file_get_bitrate;
  iface->get_crop_size = ogmrip_video_file_get_crop_size;
  iface->get_framerate = ogmrip_video_file_get_framerate;
  iface->get_standard = ogmrip_video_file_get_standard;
  iface->get_resolution = ogmrip_video_file_get_resolution;
}

OGMRipMedia *
ogmrip_video_file_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return g_object_new (OGMRIP_TYPE_VIDEO_FILE, "uri", uri, NULL);
}

