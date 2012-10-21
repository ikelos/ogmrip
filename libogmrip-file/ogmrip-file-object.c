/* OGMRipFile - A file library for OGMRip
 * Copyright (C) 2010-2012 Olivier Rolland <billl@users.sourceforge.net>
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

#include "ogmrip-file-object.h"
#include "ogmrip-file-priv.h"

enum
{
  PROP_0,
  PROP_URI
};

static void ogmrip_media_iface_init  (OGMRipMediaInterface  *iface);
static void ogmrip_title_iface_init  (OGMRipTitleInterface  *iface);
static void ogmrip_stream_iface_init (OGMRipStreamInterface *iface);
static void ogmrip_file_finalize     (GObject               *gobject);
static void ogmrip_file_get_property (GObject               *gobject,
                                      guint                 property_id,
                                      GValue                *value,
                                      GParamSpec            *pspec);
static void ogmrip_file_set_property (GObject               *gobject,
                                      guint                 property_id,
                                      const GValue          *value,
                                      GParamSpec            *pspec);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (OGMRipFile, ogmrip_file, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_MEDIA, ogmrip_media_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_TITLE, ogmrip_title_iface_init)
    G_IMPLEMENT_INTERFACE (OGMRIP_TYPE_STREAM, ogmrip_stream_iface_init));

static void
ogmrip_file_init (OGMRipFile *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, OGMRIP_TYPE_FILE, OGMRipFilePriv);

  stream->priv->length = -1.0;
  stream->priv->format = OGMRIP_FORMAT_UNDEFINED;
}

static void
ogmrip_file_class_init (OGMRipFileClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ogmrip_file_finalize;
  gobject_class->get_property = ogmrip_file_get_property;
  gobject_class->set_property = ogmrip_file_set_property;

  g_object_class_override_property (gobject_class, PROP_URI, "uri");

  g_type_class_add_private (klass, sizeof (OGMRipFilePriv));
}

static void
ogmrip_file_finalize (GObject *gobject)
{
  OGMRipFile *file = OGMRIP_FILE (gobject);

  g_free (file->priv->id);
  g_free (file->priv->uri);
  g_free (file->priv->path);
  g_free (file->priv->label);

  G_OBJECT_CLASS (ogmrip_file_parent_class)->finalize (gobject);
}

static void
ogmrip_file_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  OGMRipFile *file = OGMRIP_FILE (gobject);

  switch (property_id)
  {
    case PROP_URI:
      g_value_set_string (value, file->priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
ogmrip_file_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  OGMRipFile *file = OGMRIP_FILE (gobject);
  const gchar *str;

  switch (property_id)
  {
    case PROP_URI:
      str = g_value_get_string (value);
      if (g_str_has_prefix (str, "file://"))
      {
        file->priv->uri = g_strdup (str);
        file->priv->path = g_filename_from_uri (file->priv->uri, NULL, NULL);
      }
      else
      {
        file->priv->uri = g_strdup_printf ("file://%s", str);
        file->priv->path = g_strdup (str);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static const gchar *
ogmrip_file_get_media_id (OGMRipMedia *media)
{
  return OGMRIP_FILE (media)->priv->id;
}


static const gchar *
ogmrip_file_get_media_label (OGMRipMedia *media)
{
  return OGMRIP_FILE (media)->priv->label;
}

static const gchar *
ogmrip_file_get_uri (OGMRipMedia *media)
{
  return OGMRIP_FILE (media)->priv->uri;
}

static gint64
ogmrip_file_get_media_size (OGMRipMedia *media)
{
  return OGMRIP_FILE (media)->priv->media_size;
}

static gint
ogmrip_file_get_n_titles (OGMRipMedia *media)
{
  return 1;
}

static OGMRipTitle *
ogmrip_file_get_nth_title (OGMRipMedia *media, guint nr)
{
  return OGMRIP_TITLE (media);
}

static void
ogmrip_media_iface_init (OGMRipMediaInterface *iface)
{
  iface->get_id = ogmrip_file_get_media_id;
  iface->get_label = ogmrip_file_get_media_label;
  iface->get_uri = ogmrip_file_get_uri;
  iface->get_size = ogmrip_file_get_media_size;
  iface->get_n_titles = ogmrip_file_get_n_titles;
  iface->get_nth_title = ogmrip_file_get_nth_title;
}

static OGMRipMedia *
ogmrip_file_get_media (OGMRipTitle *title)
{
  return OGMRIP_MEDIA (title);
}

static gint64
ogmrip_file_get_title_size (OGMRipTitle *title)
{
  return OGMRIP_FILE (title)->priv->title_size;
}

static gdouble
ogmrip_file_get_length (OGMRipTitle *title, OGMRipTime *time_)
{
  gdouble length;

  length = OGMRIP_FILE (title)->priv->length;
  if (time_ != NULL && length >= 0.0)
    ogmrip_msec_to_time (length, time_);

  return length;
}

static OGMRipVideoStream *
ogmrip_file_get_video_stream (OGMRipTitle *title)
{
  OGMRipFile *file = OGMRIP_FILE (title);

  if (!OGMRIP_IS_VIDEO_STREAM (file))
    return NULL;

  return OGMRIP_VIDEO_STREAM (file);
}

static gint
ogmrip_file_get_n_audio_streams (OGMRipTitle *title)
{
  OGMRipFile *file = OGMRIP_FILE (title);

  if (!OGMRIP_IS_AUDIO_STREAM (file))
    return 0;

  return 1;
}

static OGMRipAudioStream *
ogmrip_file_get_nth_audio_stream (OGMRipTitle *title, guint nr)
{
  OGMRipFile *file = OGMRIP_FILE (title);

  if (!OGMRIP_IS_AUDIO_STREAM (file))
    return NULL;

  return OGMRIP_AUDIO_STREAM (file);
}

static gint
ogmrip_file_get_n_subp_streams (OGMRipTitle *title)
{
  OGMRipFile *file = OGMRIP_FILE (title);

  if (!OGMRIP_IS_SUBP_STREAM (file))
    return 0;

  return 1;
}

static OGMRipSubpStream *
ogmrip_file_get_nth_subp_stream (OGMRipTitle *title, guint nr)
{
  OGMRipFile *file = OGMRIP_FILE (title);

  if (!OGMRIP_IS_SUBP_STREAM (file))
    return NULL;

  return OGMRIP_SUBP_STREAM (file);
}

static void
ogmrip_title_iface_init (OGMRipTitleInterface *iface)
{
  iface->get_media = ogmrip_file_get_media;
  iface->get_size = ogmrip_file_get_title_size;
  iface->get_length = ogmrip_file_get_length;
  iface->get_video_stream = ogmrip_file_get_video_stream;
  iface->get_n_audio_streams = ogmrip_file_get_n_audio_streams;
  iface->get_nth_audio_stream = ogmrip_file_get_nth_audio_stream;
  iface->get_n_subp_streams = ogmrip_file_get_n_subp_streams;
  iface->get_nth_subp_stream = ogmrip_file_get_nth_subp_stream;
}

static gint
ogmrip_file_get_format (OGMRipStream *stream)
{
  return OGMRIP_FILE (stream)->priv->format;
}

OGMRipTitle *
ogmrip_file_get_title (OGMRipStream *stream)
{
  return OGMRIP_TITLE (stream);
}

static void
ogmrip_stream_iface_init (OGMRipStreamInterface *iface)
{
  iface->get_format = ogmrip_file_get_format;
  iface->get_title  = ogmrip_file_get_title;
}

const gchar *
ogmrip_file_get_path (OGMRipFile *file)
{
  g_return_val_if_fail (OGMRIP_IS_FILE (file), NULL);

  return file->priv->path;
}

