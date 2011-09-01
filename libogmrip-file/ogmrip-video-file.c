/* OGMRip - A wrapper library around libdvdread
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

#include "ogmrip-video-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

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
  stream->priv->standard = OGMRIP_STANDARD_UNDEFINED;
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
  const gchar *str;

  gobject = G_OBJECT_CLASS (ogmrip_video_file_parent_class)->constructor (type, n_properties, properties);

  info = ogmrip_media_info_get_default ();

  if (!info || !OGMRIP_FILE (gobject)->priv->path ||
      !ogmrip_media_info_open (info, OGMRIP_FILE (gobject)->priv->uri))
  {
    g_object_unref (gobject);
    return NULL;
  }

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
  OGMRIP_VIDEO_FILE (gobject)->priv->bitrate = str ? atoi (str) : -1;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Width");
  OGMRIP_VIDEO_FILE (gobject)->priv->width = str ? atoi (str) : 0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Height");
  OGMRIP_VIDEO_FILE (gobject)->priv->height = str ? atoi (str) : 0;

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_VIDEO, 0, "Standard");
  if (!str)
    OGMRIP_VIDEO_FILE (gobject)->priv->standard = -1;
  else if (g_str_equal (str, "PAL"))
    OGMRIP_VIDEO_FILE (gobject)->priv->standard = OGMRIP_STANDARD_PAL;
  else if (g_str_equal (str, "NTSC"))
    OGMRIP_VIDEO_FILE (gobject)->priv->standard = OGMRIP_STANDARD_NTSC;

  /*
   * TODO framerate, aspect, aspect ratio
   */

  ogmrip_media_info_close (info);

  return gobject;
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
  iface->get_bitrate = ogmrip_video_file_get_bitrate;
  iface->get_crop_size = ogmrip_video_file_get_crop_size;
  iface->get_standard = ogmrip_video_file_get_standard;
  iface->get_resolution = ogmrip_video_file_get_resolution;
}

OGMRipMedia *
ogmrip_video_file_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return g_object_new (OGMRIP_TYPE_VIDEO_FILE, "uri", uri, NULL);
}

