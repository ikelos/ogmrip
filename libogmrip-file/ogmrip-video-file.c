/* OGMRipFile - A file library for OGMRip
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ogmrip-video-file.h"
#include "ogmrip-media-info.h"
#include "ogmrip-file-priv.h"

#include <stdlib.h>

#include <glib/gi18n-lib.h>

static void g_initable_iface_init           (GInitableIface             *iface);
static void ogmrip_video_stream_iface_init  (OGMRipVideoStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (OGMRipVideoFile, ogmrip_video_file, OGMRIP_TYPE_FILE,
    G_ADD_PRIVATE (OGMRipVideoFile)
    G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, g_initable_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_VIDEO_STREAM, ogmrip_video_stream_iface_init));

static void
ogmrip_video_file_init (OGMRipVideoFile *stream)
{
  stream->priv = ogmrip_video_file_get_instance_private (stream);

  stream->priv->bitrate = -1;
  stream->priv->framerate_denom = 1;
  stream->priv->standard = OGMRIP_STANDARD_UNDEFINED;
}

static void
ogmrip_video_file_class_init (OGMRipVideoFileClass *klass)
{
}

static gboolean
ogmrip_video_file_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  GFile *file;
  OGMRipMediaInfo *info;
  const gchar *str;

  if (!g_str_has_prefix (OGMRIP_FILE (initable)->priv->uri, "file://"))
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_SCHEME,
        _("Unknown scheme for '%s'"), OGMRIP_FILE (initable)->priv->uri);
    return FALSE;
  }

  info = ogmrip_media_info_get_default ();
  if (!info)
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_FILE_ERROR_INFO,
        _("Cannot get media info handle"));
    return FALSE;
  }

  file = g_file_new_for_path (OGMRIP_FILE (initable)->priv->path);
  OGMRIP_FILE (initable)->priv->id = g_file_get_id (file);
  g_object_unref (file);

  if (!OGMRIP_FILE (initable)->priv->id)
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_ID,
        _("Cannot retrieve identifier for '%s'"), OGMRIP_FILE (initable)->priv->uri);
    return FALSE;
  }

  if (!ogmrip_media_info_open (info, OGMRIP_FILE (initable)->priv->path))
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_OPEN,
        _("Cannot open '%s'"), OGMRIP_FILE (initable)->priv->uri);
    return FALSE;
  }

  str = ogmrip_media_info_get (info, OGMRIP_CATEGORY_GENERAL, 0, "VideoCount");
  if (!str || !g_str_equal (str, "1"))
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_VIDEO,
        _("No video found for '%s'"), OGMRIP_FILE (initable)->priv->uri);
    g_object_unref (info);
    return FALSE;
  }

  OGMRIP_FILE (initable)->priv->format = ogmrip_media_info_get_video_format (info, 0);
  if (OGMRIP_FILE (initable)->priv->format < 0)
  {
    g_set_error (error, OGMRIP_MEDIA_ERROR, OGMRIP_MEDIA_ERROR_VIDEO,
        _("Unknown video format for '%s'"), OGMRIP_FILE (initable)->priv->uri);
    g_object_unref (info);
    return FALSE;
  }

  ogmrip_media_info_get_file_info (info,  OGMRIP_FILE (initable)->priv);
  ogmrip_media_info_get_video_info (info, 0, OGMRIP_VIDEO_FILE (initable)->priv);

  OGMRIP_FILE (initable)->priv->title_size = OGMRIP_VIDEO_FILE (initable)->priv->size;

  ogmrip_media_info_close (info);

  return TRUE;
}

static void
g_initable_iface_init (GInitableIface *iface)
{
  iface->init = ogmrip_video_file_initable_init;
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

static gboolean
ogmrip_video_file_get_progressive (OGMRipVideoStream *video)
{
  return OGMRIP_VIDEO_FILE (video)->priv->progressive;
}

static void
ogmrip_video_stream_iface_init (OGMRipVideoStreamInterface *iface)
{
  iface->get_aspect_ratio = ogmrip_video_file_get_aspect_ratio;
  iface->get_bitrate = ogmrip_video_file_get_bitrate;
  iface->get_crop_size = ogmrip_video_file_get_crop_size;
  iface->get_framerate = ogmrip_video_file_get_framerate;
  iface->get_standard = ogmrip_video_file_get_standard;
  iface->get_resolution = ogmrip_video_file_get_resolution;
  iface->get_progressive = ogmrip_video_file_get_progressive;
}

OGMRipMedia *
ogmrip_video_file_new (const gchar *uri, GError **error)
{
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return g_initable_new (OGMRIP_TYPE_VIDEO_FILE, NULL, error, "uri", uri, NULL);
}

